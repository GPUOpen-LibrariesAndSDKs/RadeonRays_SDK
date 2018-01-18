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

    half2 t_zero = (half2)(0.0, 0.0);
    half2 t_max2 = (half2)(t_max, t_max);
    half2 t1 = min(mymin3(t_max_x, t_max_y, t_max_z), t_max2);
    half2 t0 = max(mymax3(t_min_x, t_min_y, t_min_z), t_zero);

    return (half4)(t0, t1);
}

__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL void intersect_main(
    // Bvh nodes
    GLOBAL const bvh_node *restrict nodes,
    // Rays
    GLOBAL const ray *restrict rays,
    // Number of rays in rays buffer
    GLOBAL const int *restrict num_rays,
    // Stack memory
    GLOBAL int *stack,
    // Hit data
    GLOBAL Intersection* hits)
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

                    //
                }

                break;
            }
        }
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL void occluded_main()
{
    //
}
