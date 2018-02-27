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

/*************************************************************************
INCLUDES
**************************************************************************/
#include <../RadeonRays/src/kernels/CL/common.cl>

/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/

#define INVALID_ADDR 0xffffffffu
#define INTERNAL_NODE(node) (GetAddrLeft(node) != INVALID_ADDR)

#define GROUP_SIZE 64
#define STACK_SIZE 32
#define LDS_STACK_SIZE 16

// BVH node
typedef struct
{
    float4 aabb_left_min_or_v0_and_addr_left;
    float4 aabb_left_max_or_v1_and_mesh_id;
    float4 aabb_right_min_or_v2_and_addr_right;
    float4 aabb_right_max_and_prim_id;

} bvh_node;

#define GetAddrLeft(node)   as_uint((node).aabb_left_min_or_v0_and_addr_left.w)
#define GetAddrRight(node)  as_uint((node).aabb_right_min_or_v2_and_addr_right.w)
#define GetMeshId(node)     as_uint((node).aabb_left_max_or_v1_and_mesh_id.w)
#define GetPrimId(node)     as_uint((node).aabb_right_max_and_prim_id.w)

INLINE float2 fast_intersect_bbox2(float3 pmin, float3 pmax, float3 invdir, float3 oxinvdir, float t_max)
{
    const float3 f = mad(pmax.xyz, invdir, oxinvdir);
    const float3 n = mad(pmin.xyz, invdir, oxinvdir);
    const float3 tmax = max(f, n);
    const float3 tmin = min(f, n);
    const float t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max);
    const float t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f);
    return (float2)(t0, t1);
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
    GLOBAL uint *stack,
    // Hit data
    GLOBAL Intersection *hits)
{
    __local uint lds_stack[GROUP_SIZE * LDS_STACK_SIZE];

    uint index = get_global_id(0);
    uint local_index = get_local_id(0);

    // Handle only working subset
    if (index < *num_rays)
    {
        const ray my_ray = rays[index];

        if (ray_is_active(&my_ray))
        {
            const float3 invDir = safe_invdir(my_ray);
            const float3 oxInvDir = -my_ray.o.xyz * invDir;

            // Intersection parametric distance
            float closest_t = my_ray.o.w;

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
                    float2 s0 = fast_intersect_bbox2(
                        node.aabb_left_min_or_v0_and_addr_left.xyz,
                        node.aabb_left_max_or_v1_and_mesh_id.xyz,
                        invDir, oxInvDir, closest_t);
                    float2 s1 = fast_intersect_bbox2(
                        node.aabb_right_min_or_v2_and_addr_right.xyz,
                        node.aabb_right_max_and_prim_id.xyz,
                        invDir, oxInvDir, closest_t);

                    bool traverse_c0 = (s0.x <= s0.y);
                    bool traverse_c1 = (s1.x <= s1.y);
                    bool c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        uint deferred = INVALID_ADDR;

                        if (c1first || !traverse_c0) 
                        {
                            addr = GetAddrRight(node);
                            deferred = GetAddrLeft(node);
                        }
                        else
                        {
                            addr = GetAddrLeft(node);
                            deferred = GetAddrRight(node);
                        }

                        if (traverse_c0 && traverse_c1)
                        {
                            if (lds_sptr - lds_stack_bottom >= LDS_STACK_SIZE)
                            {
                                for (int i = 1; i < LDS_STACK_SIZE; ++i)
                                {
                                    stack[sptr + i] = lds_stack[lds_stack_bottom + i];
                                }

                                sptr += LDS_STACK_SIZE;
                                lds_sptr = lds_stack_bottom + 1;
                            }

                            lds_stack[lds_sptr++] = deferred;
                        }

                        continue;
                    }
                }
                else
                {
                    float t = fast_intersect_triangle(
                        my_ray,
                        node.aabb_left_min_or_v0_and_addr_left.xyz,
                        node.aabb_left_max_or_v1_and_mesh_id.xyz,
                        node.aabb_right_min_or_v2_and_addr_right.xyz,
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
                const float3 p = my_ray.o.xyz + closest_t * my_ray.d.xyz;

                // Calculate barycentric coordinates
                const float2 uv = triangle_calculate_barycentrics(
                    p,
                    node.aabb_left_min_or_v0_and_addr_left.xyz,
                    node.aabb_left_max_or_v1_and_mesh_id.xyz,
                    node.aabb_right_min_or_v2_and_addr_right.xyz);

                // Update hit information
                hits[index].prim_id = GetPrimId(node);
                hits[index].shape_id = GetMeshId(node);
                hits[index].uvwt = (float4)(uv.x, uv.y, 0.0f, closest_t);
            }
            else
            {
                // Miss here
                hits[index].prim_id = MISS_MARKER;
                hits[index].shape_id = MISS_MARKER;
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
    GLOBAL uint *stack,
    // Hit results: 1 for hit and -1 for miss
    GLOBAL int *hits)
{
    __local uint lds_stack[GROUP_SIZE * LDS_STACK_SIZE];

    uint index = get_global_id(0);
    uint local_index = get_local_id(0);

    // Handle only working subset
    if (index < *num_rays)
    {
        const ray my_ray = rays[index];

        if (ray_is_active(&my_ray))
        {
            const float3 invDir = safe_invdir(my_ray);
            const float3 oxInvDir = -my_ray.o.xyz * invDir;

            // Current node address
            uint addr = 0;
            // Intersection parametric distance
            const float closest_t = my_ray.o.w;

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
                    float2 s0 = fast_intersect_bbox2(
                        node.aabb_left_min_or_v0_and_addr_left.xyz,
                        node.aabb_left_max_or_v1_and_mesh_id.xyz,
                        invDir, oxInvDir, closest_t);
                    float2 s1 = fast_intersect_bbox2(
                        node.aabb_right_min_or_v2_and_addr_right.xyz,
                        node.aabb_right_max_and_prim_id.xyz,
                        invDir, oxInvDir, closest_t);

                    bool traverse_c0 = (s0.x <= s0.y);
                    bool traverse_c1 = (s1.x <= s1.y);
                    bool c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        uint deferred = INVALID_ADDR;

                        if (c1first || !traverse_c0)
                        {
                            addr = GetAddrRight(node);
                            deferred = GetAddrLeft(node);
                        }
                        else
                        {
                            addr = GetAddrLeft(node);
                            deferred = GetAddrRight(node);
                        }

                        if (traverse_c0 && traverse_c1)
                        {
                            if (lds_sptr - lds_stack_bottom >= LDS_STACK_SIZE)
                            {
                                for (int i = 1; i < LDS_STACK_SIZE; ++i)
                                {
                                    stack[sptr + i] = lds_stack[lds_stack_bottom + i];
                                }

                                sptr += LDS_STACK_SIZE;
                                lds_sptr = lds_stack_bottom + 1;
                            }

                            lds_stack[lds_sptr++] = deferred;
                        }

                        continue;
                    }
                }
                else
                {
                    float t = fast_intersect_triangle(
                        my_ray,
                        node.aabb_left_min_or_v0_and_addr_left.xyz,
                        node.aabb_left_max_or_v1_and_mesh_id.xyz,
                        node.aabb_right_min_or_v2_and_addr_right.xyz,
                        closest_t);

                    if (t < closest_t)
                    {
                        hits[index] = HIT_MARKER;
                        return;
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

            // Finished traversal, but no intersection found
            hits[index] = MISS_MARKER;
        }
    }
}
