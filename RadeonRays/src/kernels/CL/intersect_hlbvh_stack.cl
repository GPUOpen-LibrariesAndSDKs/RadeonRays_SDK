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
    \file intersect_hlbvh_stack.cl
    \author Dmitry Kozlov
    \version 1.0
    \brief HLBVH build implementation

    IntersectorHlbvh implementation is based on the following paper:
    "HLBVH: Hierarchical LBVH Construction for Real-Time Ray Tracing"
    Jacopo Pantaleoni (NVIDIA), David Luebke (NVIDIA), in High Performance Graphics 2010, June 2010
    https://research.nvidia.com/sites/default/files/publications/HLBVH-final.pdf

    Pros:
        -Very fast to build and update.
    Cons:
        -Poor BVH quality, slow traversal.
 */

 /*************************************************************************
  INCLUDES
  **************************************************************************/
#include <../RadeonRays/src/kernels/CL/common.cl>

 /*************************************************************************
   EXTENSIONS
**************************************************************************/



/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/
#define STARTIDX(x)     (((int)((x).child0)))
#define LEAFNODE(x)     (((x).child0) == ((x).child1))
#define GLOBAL_STACK_SIZE 32
#define SHORT_STACK_SIZE 16
#define WAVEFRONT_SIZE 64

typedef struct
{
    int parent;
    int child0;
    int child1;
    int next;
} bvh_node;

typedef struct
{
    // Vertex indices
    int idx[3];
    // Shape ID
    int shape_id;
    // Primitive ID
    int prim_id;
} Face;

__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL void
occluded_main(
    // Bvh nodes
    GLOBAL bvh_node const * restrict nodes,
    // Bounding boxes
    GLOBAL bbox const* restrict bounds,
    // Triangles vertices
    GLOBAL float3 const * restrict vertices,
    // Triangle indices
    GLOBAL Face const* faces,
    // Rays
    GLOBAL ray const * restrict rays,
    // Number of rays in rays buffer
    GLOBAL int const * restrict num_rays,
    // Stack memory
    GLOBAL int* stack,
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
            // Allocate stack in global memory 
            __global int* gm_stack_base = stack + (group_id * WAVEFRONT_SIZE + local_id) * GLOBAL_STACK_SIZE;
            __global int* gm_stack = gm_stack_base;
            // Allocate stack in LDS
            __local int lds[SHORT_STACK_SIZE * WAVEFRONT_SIZE];
            __local int* lm_stack_base = lds + local_id;
            __local int* lm_stack = lm_stack_base;

            // Precompute inverse direction and origin / dir for bbox testing
            float3 const invdir = safe_invdir(r);
            float3 const oxinvdir = -r.o.xyz * invdir;
            // Intersection parametric distance
            float const t_max = r.o.w;

            // Current node address
            int addr = 0;
            // Current closest intersection leaf index
            int isect_idx = INVALID_IDX;

            //  Initalize local stack
            *lm_stack = INVALID_IDX;
            lm_stack += WAVEFRONT_SIZE;

            // Start from 0 node (root)
            while (addr != INVALID_IDX)
            {
                // Fetch next node
                bvh_node const node = nodes[addr];

                // Check if it is a leaf
                if (LEAFNODE(node))
                {
                    Face face = faces[STARTIDX(node)];
#ifdef RR_RAY_MASK
                    if (ray_get_mask(&r) != face.shape_id)
                    {
#endif // RR_RAY_MASK
                        // Leafs directly store vertex indices
                        // so we load vertices directly
                        float3 const v1 = vertices[face.idx[0]];
                        float3 const v2 = vertices[face.idx[1]];
                        float3 const v3 = vertices[face.idx[2]];
                        // Intersect triangle
                        float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);
                        // If hit update closest hit distance and index
                        if (f < t_max)
                        {
                            hits[global_id] = HIT_MARKER;
                            return;
                        }
#ifdef RR_RAY_MASK
                    }
#endif // RR_RAY_MASK
                }
                else
                {
                    // It is internal node, so intersect vs both children bounds
                    float2 const s0 = fast_intersect_bbox1(bounds[node.child0], invdir, oxinvdir, t_max);
                    float2 const s1 = fast_intersect_bbox1(bounds[node.child1], invdir, oxinvdir, t_max);

                    // Determine which one to traverse
                    bool const traverse_c0 = (s0.x <= s0.y);
                    bool const traverse_c1 = (s1.x <= s1.y);
                    bool const c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        int deferred = -1;

                        // Determine which one to traverse first
                        if (c1first || !traverse_c0)
                        {
                            // Right one is closer or left one not travesed
                            addr = node.child1;
                            deferred = node.child0;
                        }
                        else
                        {
                            // Traverse left node otherwise
                            addr = node.child0;
                            deferred = node.child1;
                        }

                        // If we traverse both children we need to postpone the node
                        if (traverse_c0 && traverse_c1)
                        {
                            // If short stack is full, we offload it into global memory
                            if (lm_stack - lm_stack_base >= SHORT_STACK_SIZE * WAVEFRONT_SIZE)
                            {
                                for (int i = 1; i < SHORT_STACK_SIZE; ++i)
                                {
                                    gm_stack[i] = lm_stack_base[i * WAVEFRONT_SIZE];
                                }

                                gm_stack += SHORT_STACK_SIZE;
                                lm_stack = lm_stack_base + WAVEFRONT_SIZE;
                            }

                            *lm_stack = deferred;
                            lm_stack += WAVEFRONT_SIZE;
                        }

                        // Continue traversal
                        continue;
                    }
                }

                // Try popping from local stack
                lm_stack -= WAVEFRONT_SIZE;
                addr = *(lm_stack);

                // If we popped INVALID_IDX then check global stack
                if (addr == INVALID_IDX && gm_stack > gm_stack_base)
                {
                    // Adjust stack pointer
                    gm_stack -= SHORT_STACK_SIZE;
                    // Copy data from global memory to LDS
                    for (int i = 1; i < SHORT_STACK_SIZE; ++i)
                    {
                        lm_stack_base[i * WAVEFRONT_SIZE] = gm_stack[i];
                    }
                    // Point local stack pointer to the end
                    lm_stack = lm_stack_base + (SHORT_STACK_SIZE - 1) * WAVEFRONT_SIZE;
                    addr = lm_stack_base[WAVEFRONT_SIZE * (SHORT_STACK_SIZE - 1)];
                }
            }

            // Finished traversal, but no intersection found
            hits[global_id] = MISS_MARKER;
        }
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL void intersect_main(
    // Bvh nodes
    GLOBAL bvh_node const* restrict nodes,
    // Bounding boxes
    GLOBAL bbox const* restrict bounds,
    // Triangles vertices
    GLOBAL float3 const* restrict vertices,
    // Faces
    GLOBAL Face const* restrict faces,
    // Rays
    GLOBAL ray const* restrict rays,
    // Number of rays in rays buffer
    GLOBAL int const* restrict num_rays,
    // Stack memory
    GLOBAL int* stack,
    // Hit data
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
            // Allocate stack in global memory 
            __global int* gm_stack_base = stack + (group_id * WAVEFRONT_SIZE + local_id) * GLOBAL_STACK_SIZE;
            __global int* gm_stack = gm_stack_base;
            // Allocate stack in LDS
            __local int lds[SHORT_STACK_SIZE * WAVEFRONT_SIZE];
            __local int* lm_stack_base = lds + local_id;
            __local int* lm_stack = lm_stack_base;

            // Precompute inverse direction and origin / dir for bbox testing
            float3 const invdir = safe_invdir(r);
            float3 const oxinvdir = -r.o.xyz * invdir;
            // Intersection parametric distance
            float t_max = r.o.w;

            // Current node address
            int addr = 0;
            // Current closest intersection leaf index
            int isect_idx = INVALID_IDX;

            //  Initalize local stack
            *lm_stack = INVALID_IDX;
            lm_stack += WAVEFRONT_SIZE;

            // Start from 0 node (root)
            while (addr != INVALID_IDX)
            {
                // Fetch next node
                bvh_node const node = nodes[addr];

                // Check if it is a leaf
                if (LEAFNODE(node))
                {
                    Face face = faces[STARTIDX(node)];
#ifdef RR_RAY_MASK
                    if (ray_get_mask(&r) != face.shape_id)
                    {
#endif // RR_RAY_MASK
                        // Leafs directly store vertex indices
                        // so we load vertices directly
                        float3 const v1 = vertices[face.idx[0]];
                        float3 const v2 = vertices[face.idx[1]];
                        float3 const v3 = vertices[face.idx[2]];
                        // Intersect triangle
                        float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);
                        // If hit update closest hit distance and index
                        if (f < t_max)
                        {
                            t_max = f;
                            isect_idx = STARTIDX(node);
                        }
#ifdef RR_RAY_MASK
                    }
#endif // RR_RAY_MASK
                }
                else
                {
                    // It is internal node, so intersect vs both children bounds
                    float2 const s0 = fast_intersect_bbox1(bounds[node.child0], invdir, oxinvdir, t_max);
                    float2 const s1 = fast_intersect_bbox1(bounds[node.child1], invdir, oxinvdir, t_max);

                    // Determine which one to traverse
                    bool const traverse_c0 = (s0.x <= s0.y);
                    bool const traverse_c1 = (s1.x <= s1.y);
                    bool const c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        int deferred = -1;

                        // Determine which one to traverse first
                        if (c1first || !traverse_c0)
                        {
                            // Right one is closer or left one not travesed
                            addr = node.child1;
                            deferred = node.child0;
                        }
                        else
                        {
                            // Traverse left node otherwise
                            addr = node.child0;
                            deferred = node.child1;
                        }

                        // If we traverse both children we need to postpone the node
                        if (traverse_c0 && traverse_c1)
                        {
                            // If short stack is full, we offload it into global memory
                            if ( lm_stack - lm_stack_base >= SHORT_STACK_SIZE * WAVEFRONT_SIZE)
                            {
                                for (int i = 1; i < SHORT_STACK_SIZE; ++i)
                                {
                                    gm_stack[i] = lm_stack_base[i * WAVEFRONT_SIZE];
                                }

                                gm_stack += SHORT_STACK_SIZE;
                                lm_stack = lm_stack_base + WAVEFRONT_SIZE;
                            }

                            *lm_stack = deferred;
                            lm_stack += WAVEFRONT_SIZE;
                        }

                        // Continue traversal
                        continue;
                    }
                }

                // Try popping from local stack
                lm_stack -= WAVEFRONT_SIZE;
                addr = *(lm_stack);

                // If we popped INVALID_IDX then check global stack
                if (addr == INVALID_IDX && gm_stack > gm_stack_base)
                {
                    // Adjust stack pointer
                    gm_stack -= SHORT_STACK_SIZE;
                    // Copy data from global memory to LDS
                    for (int i = 1; i < SHORT_STACK_SIZE; ++i)
                    {
                        lm_stack_base[i * WAVEFRONT_SIZE] = gm_stack[i];
                    }
                    // Point local stack pointer to the end
                    lm_stack = lm_stack_base + (SHORT_STACK_SIZE - 1) * WAVEFRONT_SIZE;
                    addr = lm_stack_base[WAVEFRONT_SIZE * (SHORT_STACK_SIZE - 1)];
                }
            }

            // Check if we have found an intersection
            if (isect_idx != INVALID_IDX)
            {
                // Fetch the node & vertices
                Face const face = faces[isect_idx];
                float3 const v1 = vertices[face.idx[0]];
                float3 const v2 = vertices[face.idx[1]];
                float3 const v3 = vertices[face.idx[2]];
                // Calculate hit position
                float3 const p = r.o.xyz + r.d.xyz * t_max;
                // Calculte barycentric coordinates
                float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3);
                // Update hit information
                hits[global_id].shape_id = face.shape_id;
                hits[global_id].prim_id = face.prim_id;
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



