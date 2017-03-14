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
EXTENSIONS
**************************************************************************/
#ifdef AMD_MEDIA_OPS
#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable
#endif

/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/
#define KERNEL __kernel
#define GLOBAL __global
#define INLINE __attribute__((always_inline))
#define HIT_MARKER 1
#define MISS_MARKER -1
#define INVALID_IDX -1
#define LEAFNODE(x) (((x).child0) == -1)

// BVH node
typedef struct
{
    union 
    {
        struct
        {
            // Child bounds
            bbox bounds[2];
        };

        struct
        {
            // If node is a leaf we keep vertex indices here
            int i0, i1, i2;
            // Address of a left child
            int child0;
            // Shape mask
            int shape_mask;
            // Shape ID
            int shape_id;
            // Primitive ID
            int prim_id;
            // Address of a right child
            int child1;
        };
    };

} bvh_node;

/*************************************************************************
HELPER FUNCTIONS
**************************************************************************/
INLINE float min3(float a, float b, float c)
{
#ifdef AMD_MEDIA_OPS
    return amd_min3(a, b, c);
#else
    return min(min(a,b), c);
#endif
}

INLINE float max3(float a, float b, float c)
{
#ifdef AMD_MEDIA_OPS
    return amd_max3(a, b, c);
#else
    return max(max(a,b), c);
#endif
}


// Intersect ray against a triangle and return intersection interval value if it is in
// (0, t_max], return t_max otherwise.
INLINE
float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max)
{
    float3 const e1 = v2 - v1;
    float3 const e2 = v3 - v1;
    float3 const s1 = cross(r.d.xyz, e2);
    float const invd = native_recip(dot(s1, e1));
    float3 const d = r.o.xyz - v1;
    float const b1 = dot(d, s1) * invd;
    float3 const s2 = cross(d, e1);
    float const b2 = dot(r.d.xyz, s2) * invd;
    float const temp = dot(e2, s2) * invd;

    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max)
    {
        return t_max;
    }
    else
    {
        return temp;
    }
}

// Intersect rays vs bbox and return intersection span. 
// Intersection criteria is ret.x <= ret.y
INLINE
float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max)
{
    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir);
    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir);
    float3 const tmax = max(f, n);
    float3 const tmin = min(f, n);
    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max);
    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.001f);
    return make_float2(t0, t1);
}

// Given a point in triangle plane, calculate its barycentrics
INLINE
float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3)
{
    float3 const e1 = v2 - v1;
    float3 const e2 = v3 - v1;
    float3 const e = p - v1;
    float const d00 = dot(e1, e1);
    float const d01 = dot(e1, e2);
    float const d11 = dot(e2, e2);
    float const d20 = dot(e, e1);
    float const d21 = dot(e, e2);
    float const invdenom = native_recip(d00 * d11 - d01 * d01);
    float const b1 = (d11 * d20 - d01 * d21) * invdenom;
    float const b2 = (d00 * d21 - d01 * d20) * invdenom;
    return make_float2(b1, b2);
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
KERNEL void
occluded_main(
    // Bvh nodes
    GLOBAL bvh_node const * restrict nodes,
    // Triangles vertices
    GLOBAL float3 const * restrict vertices,
    // Rays
    GLOBAL ray const * restrict rays,
    // Number of rays in rays buffer
    GLOBAL int const * restrict num_rays,
    // Displacement table for perfect hashing
    GLOBAL int const * restrict displacement_table,
    // Hash table for perfect hashing
    GLOBAL int const * restrict hash_table,
    // Displacement table size
    int const displacement_table_size,
    // Hit results: 1 for hit and -1 for miss
    GLOBAL int* hits
    )
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);
    int group_id = get_group_id(0);

    // Handle only working set
    if (global_id < *num_rays)
    {
        ray const r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            // Precompute inverse direction and origin / dir for bbox testing
            float3 const invdir = native_recip(r.d.xyz);
            float3 const oxinvdir = -r.o.xyz * invdir;
            // Intersection parametric distance
            float const t_max = r.o.w;

            // Bit tail to track traversal
            int bit_trail = 0;
            // Current node index (complete tree enumeration)
            int node_idx = 1;
            // Current node address
            int addr = 0;

            // Start from 0 node (root)
            while (addr != INVALID_IDX)
            {
                // Fetch next node
                bvh_node const node = nodes[addr];

                // Check if it is a leaf
                if (LEAFNODE(node))
                {
                    // Leafs directly store vertex indices
                    // so we load vertices directly
                    float3 const v1 = vertices[node.i0];
                    float3 const v2 = vertices[node.i1];
                    float3 const v3 = vertices[node.i2];
                    // Intersect triangle
                    float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);
                    // If hit store the result and bail out
                    if (f < t_max)
                    {
                        hits[global_id] = HIT_MARKER;
                        return;
                    }
                }
                else
                {
                    // It is internal node, so intersect vs both children bounds
                    float2 const s0 = fast_intersect_bbox1(node.bounds[0], invdir, oxinvdir, t_max);
                    float2 const s1 = fast_intersect_bbox1(node.bounds[1], invdir, oxinvdir, t_max);

                    // Determine which one to traverse
                    bool const traverse_c0 = (s0.x <= s0.y);
                    bool const traverse_c1 = (s1.x <= s1.y);
                    bool const c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        // Go one level down => shift bit trail
                        bit_trail = bit_trail << 1;
                        // idx = idx * 2 (this is for left child)
                        node_idx = node_idx << 1;

                        // If we postpone one node here we 
                        // set last bit in bit trail
                        if (traverse_c0 && traverse_c1)
                        {
                            bit_trail = bit_trail ^ 0x1;
                        }

                        // Determine which one to traverse first
                        if (c1first || !traverse_c0)
                        {
                            // Right one is closer or left one not travesed
                            addr = node.child1;
                            // Fix index
                            // idx = 2 * idx + 1 for right one
                            node_idx = node_idx ^ 0x1;
                        }
                        else
                        {
                            // Traverse left node otherwise
                            addr = node.child0;
                        }

                        // Continue traversal
                        continue;
                    }
                }

                // Here we need to either backtrack or
                // stop traversal.
                // If bit trail is zero, there is nothing 
                // to traverse.
                if (bit_trail == 0)
                {
                    addr = INVALID_IDX;
                    continue;
                }
                
                // Backtrack
                // Calculate where we postponed the last node.
                // = number of trailing zeroes in bit_trail
                int const num_levels = 31 - clz(bit_trail & -bit_trail);
                // Update bit trail (shift and unset last bit)
                bit_trail = (bit_trail >> num_levels) ^ 0x1;
                // Calculate postponed index
                node_idx = (node_idx >> num_levels) ^ 0x1;

                // Calculate node address using perfect hasing of node indices
                int const displacement = displacement_table[node_idx / displacement_table_size];
                addr = hash_table[displacement + (node_idx & (displacement_table_size - 1))];
            }

            // Finished traversal, but no intersection found
            hits[global_id] = MISS_MARKER;
        }
    }
}


__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
KERNEL void intersect_main(
    // Bvh nodes
    GLOBAL bvh_node const * restrict nodes,
    // Triangles vertices
    GLOBAL float3 const * restrict vertices,
    // Rays
    GLOBAL ray const * restrict rays,
    // Number of rays in rays buffer
    GLOBAL int const * restrict num_rays,
    // Displacement table for perfect hashing
    GLOBAL int const * restrict displacement_table,
    // Hash table for perfect hashing
    GLOBAL int const * restrict hash_table,
    // Displacement table size
    int const displacement_table_size,
    // Hit results: 1 for hit and -1 for miss
    GLOBAL Intersection* hits)
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);
    int group_id = get_group_id(0);

    // Handle only working subset
    if (global_id < *num_rays)
    {
        ray const r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            // Precompute inverse direction and origin / dir for bbox testing
            float3 const invdir = native_recip(r.d.xyz);
            float3 const oxinvdir = -r.o.xyz * invdir;
            // Intersection parametric distance
            float t_max = r.o.w;

            // Bit tail to track traversal
            int bit_trail = 0;
            // Current node index (complete tree enumeration)
            int node_idx = 1;
            // Current node address
            int addr = 0;
            // Current closest intersection leaf index
            int isect_idx = INVALID_IDX;

            // Start from 0 node (root)
            while (addr != INVALID_IDX)
            {
                // Fetch next node
                bvh_node const node = nodes[addr];

                // Check if it is a leaf
                if (LEAFNODE(node))
                {
                    // Leafs directly store vertex indices
                    // so we load vertices directly
                    float3 const v1 = vertices[node.i0];
                    float3 const v2 = vertices[node.i1];
                    float3 const v3 = vertices[node.i2];
                    // Intersect triangle
                    float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);
                    // If hit update closest hit distance and index
                    if (f < t_max)
                    {
                        t_max = f;
                        isect_idx = addr;
                    }
                }
                else
                {
                    // It is internal node, so intersect vs both children bounds
                    float2 const s0 = fast_intersect_bbox1(node.bounds[0], invdir, oxinvdir, t_max);
                    float2 const s1 = fast_intersect_bbox1(node.bounds[1], invdir, oxinvdir, t_max);

                    // Determine which one to traverse
                    bool const traverse_c0 = (s0.x <= s0.y);
                    bool const traverse_c1 = (s1.x <= s1.y);
                    bool const c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        // Go one level down => shift bit trail
                        bit_trail = bit_trail << 1;
                        // idx = idx * 2 (this is for left child)
                        node_idx = node_idx << 1;

                        // If we postpone one node here we 
                        // set last bit in bit trail
                        if (traverse_c0 && traverse_c1)
                        {
                            bit_trail = bit_trail ^ 0x1;
                        }

                        // Determine which one to traverse first
                        if (c1first || !traverse_c0)
                        {
                            // Right one is closer or left one not travesed
                            addr = node.child1;
                            // Fix index
                            // idx = 2 * idx + 1 for right one
                            node_idx = node_idx ^ 0x1;
                        }
                        else
                        {
                            // Traverse left node otherwise
                            addr = node.child0;
                        }

                        // Continue traversal
                        continue;
                    }
                }

                // Here we need to either backtrack or
                // stop traversal.
                // If bit trail is zero, there is nothing 
                // to traverse.
                if (bit_trail == 0)
                {
                    addr = INVALID_IDX;
                    continue;
                }

                // Backtrack
                // Calculate where we postponed the last node.
                // = number of trailing zeroes in bit_trail
                int num_levels = 31 - clz(bit_trail & -bit_trail);
                // Update bit trail (shift and unset last bit)
                bit_trail = (bit_trail >> num_levels) ^ 0x1;
                // Calculate postponed index
                node_idx = (node_idx >> num_levels) ^ 0x1;

                // Calculate node address using perfect hasing of node indices
                int displacement = displacement_table[node_idx / displacement_table_size];
                addr = hash_table[displacement + (node_idx & (displacement_table_size - 1))];
            }

            // Check if we have found an intersection
            if (isect_idx != INVALID_IDX)
            {
                // Fetch the node & vertices
                bvh_node const node = nodes[isect_idx];
                float3 const v1 = vertices[node.i0];
                float3 const v2 = vertices[node.i1];
                float3 const v3 = vertices[node.i2];
                // Calculate hit position
                float3 const p = r.o.xyz + r.d.xyz * t_max;
                // Calculte barycentric coordinates
                float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3);
                // Update hit information
                hits[global_id].shapeid = node.shape_id;
                hits[global_id].primid = node.prim_id;
                hits[global_id].uvwt = make_float4(uv.x, uv.y, 0.f, t_max);
            }
            else
            {
                // Miss here
                hits[global_id].shapeid = MISS_MARKER;
                hits[global_id].primid = MISS_MARKER;
            }
        }
    }
}
