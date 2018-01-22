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
/**
    \file intersect_bvh2_bittrail.cl
    \author Dmitry Kozlov
    \version 1.0
    \brief Intersector implementation based on BVH stackless traversal using bit trail and perfect hashing.

    Intersector is using binary BVH with two bounding boxes per node.
    Traversal is using bit trail and perfect hashing for backtracing and based on the following paper:

    "Efficient stackless hierarchy traversal on GPUs with backtracking in constant time""
    Nikolaus Binder, Alexander Keller
    http://dl.acm.org/citation.cfm?id=2977343

    Traversal pseudocode:

        while(addr is valid)
        {
            node <- fetch next node at addr
            index <- 1
            trail <- 0
            if (node is leaf)
                intersect leaf
            else
            {
                intersect ray vs left child
                intersect ray vs right child
                if (intersect any of children)
                {
                    index <- index << 1
                    trail <- trail << 1
                    determine closer child
                    if intersect both
                    {
                        trail <- trail ^ 1
                        addr = closer child
                    }
                    else
                    {
                        addr = intersected child
                    }
                    if addr is right
                        index <- index ^ 1
                    continue
                }
            }

            if (trail == 0)
            {
                break
            }

            num_levels = count trailing zeroes in trail
            trail <- (trail << num_levels) & 1
            index <- (index << num_levels) & 1

            addr = hash[index]
        }

    Pros:
        -Very fast traversal.
        -Benefits from BVH quality optimization.
        -Low VGPR pressure
    Cons:
        -Depth is limited.
        -Generates global memory traffic.
 */

/*************************************************************************
INCLUDES
**************************************************************************/
#include <../RadeonRays/src/kernels/CL/common.cl>


/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/

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


__attribute__((reqd_work_group_size(64, 1, 1)))
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

        if (ray_is_active(&r))
        {
            // Precompute inverse direction and origin / dir for bbox testing
            float3 const invdir = safe_invdir(r);
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

        if (ray_is_active(&r))
        {
            // Precompute inverse direction and origin / dir for bbox testing
            float3 const invdir = safe_invdir(r);
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
                // Calculate barycentric coordinates
                float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3);
                // Update hit information
                hits[global_id].shape_id = node.shape_id;
                hits[global_id].prim_id = node.prim_id;
                hits[global_id].uvwt = make_float4(uv.x, uv.y, 0.f, t_max);
            }
            else
            {
                // Miss here
                hits[global_id].shape_id = MISS_MARKER;
                hits[global_id].prim_id = MISS_MARKER;
            }
        }
    }
}
