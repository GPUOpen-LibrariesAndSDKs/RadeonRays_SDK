/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

#pragma OPENCL EXTENSION cl_khr_fp16 : enable

/*************************************************************************
INCLUDES
**************************************************************************/
#include <../RadeonRays/src/kernels/CL/common.cl>

/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/

#define INVALID_ADDR 0xffffffffu
#define INTERNAL_NODE(node) ((node).addr0 != INVALID_ADDR)

#define GROUP_SIZE 64
#define STACK_SIZE 32
#define LDS_STACK_SIZE 16

// BVH node
typedef struct
{
    uint3 aabb01_min_or_v0;
    uint  addr0;

    uint3 aabb01_max_or_v1;
    uint  addr1_or_mesh_id;

    uint3 aabb23_min_or_v2;
    uint  addr2_or_prim_id;

    uint3 aabb23_max;
    uint  addr3;
} bvh_node;

#define mymin3(a, b, c) min(min((a), (b)), (c))
#define mymax3(a, b, c) max(max((a), (b)), (c))

INLINE half2 unpackFloat2x16(uint v)
{
    return (half2)
        (as_half(convert_ushort(v >> 16)),
         as_half(convert_ushort(v & 0xFFFF)));
}

INLINE half4 fast_intersect_bbox2(uint3 pmin, uint3 pmax, half3 invdir, half3 oxinvdir, float t_max)
{
    half2 pmin_x = unpackFloat2x16(pmin.x);
    half2 pmin_y = unpackFloat2x16(pmin.y);
    half2 pmin_z = unpackFloat2x16(pmin.z);
    half2 pmax_x = unpackFloat2x16(pmax.x);
    half2 pmax_y = unpackFloat2x16(pmax.y);
    half2 pmax_z = unpackFloat2x16(pmax.z);

    half2 f_x = fma(pmax_x, invdir.xx, oxinvdir.xx);
    half2 f_y = fma(pmax_y, invdir.yy, oxinvdir.yy);
    half2 f_z = fma(pmax_z, invdir.zz, oxinvdir.zz);

    half2 n_x = fma(pmin_x, invdir.xx, oxinvdir.xx);
    half2 n_y = fma(pmin_y, invdir.yy, oxinvdir.yy);
    half2 n_z = fma(pmin_z, invdir.zz, oxinvdir.zz);

    half2 t_max_x = max(f_x, n_x);
    half2 t_max_y = max(f_y, n_y);
    half2 t_max_z = max(f_z, n_z);

    half2 t_min_x = min(f_x, n_x);
    half2 t_min_y = min(f_y, n_y);
    half2 t_min_z = min(f_z, n_z);

    half2 t_zero = (half2)(0.0f, 0.0f);
    half2 t_max2 = (half2)(t_max, t_max);
    half2 t1 = min(mymin3(t_max_x, t_max_y, t_max_z), t_max2);
    half2 t0 = max(mymax3(t_min_x, t_min_y, t_min_z), t_zero);

    return (half4)(t0, t1);
}

void stack_push(
    __local uint *lds_stack,
    __private uint *lds_sptr,
    uint lds_stack_bottom,
    __global uint *stack,
    __private uint *sptr,
    uint idx)
{
    if (*lds_sptr - lds_stack_bottom >= LDS_STACK_SIZE)
    {
        for (int i = 1; i < LDS_STACK_SIZE; ++i)
        {
            stack[*sptr + i] = lds_stack[lds_stack_bottom + i];
        }

        *sptr = *sptr + LDS_STACK_SIZE;
        *lds_sptr = lds_stack_bottom + 1;
    }

    lds_stack[*lds_sptr] = idx;
    *lds_sptr = *lds_sptr + 1;
}

// TODO: use GLOBAL, etc. macros (gboisse)

__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL void intersect_main(
    // Bvh nodes
    GLOBAL const bvh_node *restrict nodes,
    // Rays
    GLOBAL const ray *restrict rays,
    // Number of rays in rays buffer
    GLOBAL const int *restrict num_rays,
    // Stack memory
    GLOBAL uint *stack,
    // Hit data
    GLOBAL Intersection *hits)
{
    uint index = get_global_id(0);
    uint local_index = get_local_id(0);

    // Handle only working subset
    if (index < *num_rays)
    {
        const ray myRay = rays[index];

        if (ray_is_active(&myRay))
        {
            __local uint lds_stack[GROUP_SIZE * LDS_STACK_SIZE];

            // Precompute inverse direction and origin / dir for bbox testing
            const float3 invDir32 = safe_invdir(myRay);
            const half3 invDir = convert_half3(invDir32);
            const half3 oxInvDir = convert_half3(-myRay.o.xyz * invDir32);

            // Intersection parametric distance
            float closest_t = myRay.o.w;

            // Current node address
            uint addr = 0;
            // Current closest address
            uint closest_addr = INVALID_ADDR;

            uint stack_bottom = STACK_SIZE * index;
            uint sptr = stack_bottom;
            uint lds_stack_bottom = local_index * LDS_STACK_SIZE;
            uint lds_sptr = lds_stack_bottom;

            lds_stack[lds_sptr++] = INVALID_ADDR;

            while (addr != INVALID_ADDR)
            {
                const bvh_node node = nodes[addr];

                if (INTERNAL_NODE(node))
                {
                    half4 s01 = fast_intersect_bbox2(
                        node.aabb01_min_or_v0,
                        node.aabb01_max_or_v1,
                        invDir, oxInvDir, closest_t);
                    half4 s23 = fast_intersect_bbox2(
                        node.aabb23_min_or_v2,
                        node.aabb23_max,
                        invDir, oxInvDir, closest_t);

                    bool traverse_c0 = (s01.x <= s01.z);
                    bool traverse_c1 = (node.addr1_or_mesh_id != INVALID_ADDR) && (s01.y <= s01.w);
                    bool traverse_c2 = (s23.x <= s23.z);
                    bool traverse_c3 = (node.addr3 != INVALID_ADDR) && (s23.y <= s23.w);

                    if (traverse_c0 || traverse_c1 || traverse_c2 || traverse_c3)
                    {
                        uint a = INVALID_ADDR;
                        half d = 100000000.0f;

                        if (traverse_c0)
                        {
                            a = node.addr0;
                            d = s01.x;
                        }

                        if (traverse_c1)
                        {
                            if (a == INVALID_ADDR)
                                a = node.addr1_or_mesh_id;
                            else
                            {
                                uint topush = s01.y < d ? a : node.addr1_or_mesh_id;
                                d = min(s01.y, d);
                                a = topush == a ? node.addr1_or_mesh_id : a;
                                stack_push(lds_stack, &lds_sptr, lds_stack_bottom, stack, &sptr, topush);
                            }
                        }

                        if (traverse_c2)
                        {
                            if (a == INVALID_ADDR)
                                a = node.addr2_or_prim_id;
                            else
                            {
                                uint topush = s23.x < d ? a : node.addr2_or_prim_id;
                                d = min(s23.x, d);
                                a = topush == a ? node.addr2_or_prim_id : a;
                                stack_push(lds_stack, &lds_sptr, lds_stack_bottom, stack, &sptr, topush);
                            }
                        }

                        if (traverse_c3)
                        {
                            if (a == INVALID_ADDR)
                                a = node.addr3;
                            else
                            {
                                uint topush = s23.y < d ? a : node.addr3;
                                d = min(s23.y, d);
                                a = topush == a ? node.addr3 : a;
                                stack_push(lds_stack, &lds_sptr, lds_stack_bottom, stack, &sptr, topush);
                            }
                        }

                        addr = a;
                        continue;
                    }
                }
                else
                {
                    float t = fast_intersect_triangle(
                        myRay,
                        as_float3(node.aabb01_min_or_v0),
                        as_float3(node.aabb01_max_or_v1),
                        as_float3(node.aabb23_min_or_v2),
                        closest_t);

                    if (t < closest_t)
                    {
                        closest_t = t;
                        closest_addr = addr;
                    }
                }

                addr = lds_stack[--lds_sptr];

                if (addr == INVALID_ADDR && sptr > stack_bottom)
                {
                    sptr -= LDS_STACK_SIZE;
                    for (int i = 1; i < LDS_STACK_SIZE; ++i)
                    {
                        lds_stack[lds_stack_bottom + i] = stack[sptr + i];
                    }

                    lds_sptr = lds_stack_bottom + LDS_STACK_SIZE - 1;
                    addr = lds_stack[lds_sptr];
                }
            }

            // Check if we have found an intersection
            if (closest_addr != INVALID_ADDR)
            {
                // Calculate hit position
                const bvh_node node = nodes[closest_addr];
                const float3 p = myRay.o.xyz + closest_t * myRay.d.xyz;

                // Calculte barycentric coordinates
                const float2 uv = triangle_calculate_barycentrics(
                    p,
                    as_float3(node.aabb01_min_or_v0),
                    as_float3(node.aabb01_max_or_v1),
                    as_float3(node.aabb23_min_or_v2));

                // Update hit information
                hits[index].prim_id = node.addr2_or_prim_id;
                hits[index].shape_id = node.addr1_or_mesh_id;
                hits[index].uvwt = make_float4(uv.x, uv.y, 0.f, closest_t);
            }
            else
            {
                // Miss here
                hits[index].prim_id = INVALID_ADDR;
                hits[index].shape_id = INVALID_ADDR;
            }
        }
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL void occluded_main(
    // Bvh nodes
    GLOBAL const bvh_node *restrict nodes,
    // Rays
    GLOBAL const ray *restrict rays,
    // Number of rays in rays buffer
    GLOBAL const int *restrict num_rays,
    // Stack memory
    GLOBAL int *stack,
    // Hit results: 1 for hit and -1 for miss
    GLOBAL int *hits)
{
    // TODO: ... (gboisse)
}
