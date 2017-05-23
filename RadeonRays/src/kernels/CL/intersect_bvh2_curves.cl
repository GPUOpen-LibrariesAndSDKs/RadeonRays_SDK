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

/*************************************************************************
DEFINES
**************************************************************************/
#define PI 3.14159265358979323846f
#define STARTIDX(x)     (((int)(x.pmin.w)) >> 4)
#define NUMPRIMS(x)     (((int)(x.pmin.w)) & 0xF)
#define LEAFNODE(x)     (((x).pmin.w) != -1.f)
#define NEXT(x)     ((int)((x).pmax.w))

///////// DEBUGGING!
#pragma OPENCL EXTENSION cl_amd_printf : enable

/*************************************************************************
 TYPE DEFINITIONS
 **************************************************************************/
typedef bbox bvh_node;

typedef struct
{
	int idx[2];
	int shape_mask;
	int shape_id;
	int prim_id;
} Segment;

__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL 
void intersect_main(
    GLOBAL bvh_node const* restrict nodes,   // BVH nodes
    GLOBAL float4 const* restrict vertices,  // Curve vertices
    GLOBAL Segment const* restrict segments, // Segment indices
    GLOBAL ray const* restrict rays,         // Rays
    GLOBAL int const* restrict num_rays,     // Number of rays
    GLOBAL Intersection* hits                // Hit data
)
{
    int global_id = get_global_id(0);
	if (global_id < *num_rays)
    {
        // Fetch ray
        ray const r = rays[global_id];

        if (ray_is_active(&r))
        {
            // Precompute inverse direction and origin / dir for bbox testing
            float3 const invdir = safe_invdir(r);
            float3 const oxinvdir = -r.o.xyz * invdir;

            // Intersection parametric distance
            float t_max = r.o.w;

            // Current node address
            int addr = 0;

            // Current closest segment index
            int isect_idx = INVALID_IDX;
			float U_COORD = 0.f;

            while (addr != INVALID_IDX)
            {
                // Fetch next node
                bvh_node node = nodes[addr];
				 
                // Intersect against bbox
                float2 s = fast_intersect_bbox1(node, invdir, oxinvdir, t_max);
                if (s.x <= s.y)
                { 
                    // Check if the node is a leaf
                    if (LEAFNODE(node))
                    {
                        int const segment_idx = STARTIDX(node);
						Segment const segment = segments[segment_idx];
                        float4 const v1 = vertices[segment.idx[0]];
                        float4 const v2 = vertices[segment.idx[1]];

                        // Intersect capsule
                        float const f = intersect_capsule(r, v1, v2, t_max, &U_COORD);

                        // If hit update closest hit distance and index
                        if (f < t_max)
                        {
                            t_max = f;
                            isect_idx = segment_idx;
                        }
                    }
                    else
                    {
                        // Move to next node otherwise.
                        // Left child is always at addr + 1
                        ++addr;
                        continue;
                    }
                }

                addr = NEXT(node);
            }

            // Check if we have found an intersection
            if (isect_idx != INVALID_IDX)
            {
                // Fetch the node & vertices
				Segment const segment = segments[isect_idx];
                float4 const v1 = vertices[segment.idx[0]];
                float4 const v2 = vertices[segment.idx[1]];

                // Update hit information
                hits[global_id].shape_id = segment.shape_id;
                hits[global_id].prim_id = segment.prim_id;
                hits[global_id].uvwt = make_float4(U_COORD, 0.f, 0.f, t_max);
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

__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL 
void occluded_main(
	GLOBAL bvh_node const* restrict nodes,   // BVH nodes
	GLOBAL float4 const* restrict vertices,  // Curve vertices
	GLOBAL Segment const* restrict segments, // Segment indices
	GLOBAL ray const* restrict rays,         // Rays
	GLOBAL int const* restrict num_rays,     // Number of rays
	GLOBAL int* hits                         // Hit data
)
{
	int global_id = get_global_id(0);

    // Handle only working subset
    if (global_id < *num_rays)
    {
        // Fetch ray
        ray const r = rays[global_id];

        if (ray_is_active(&r))
        {
            // Precompute inverse direction and origin / dir for bbox testing
            float3 const invdir = safe_invdir(r);
            float3 const oxinvdir = -r.o.xyz * invdir;

            // Intersection parametric distance
            float t_max = r.o.w;

            // Current node address
            int addr = 0;
			while (addr != INVALID_IDX)
            {
                // Fetch next node
                bvh_node node = nodes[addr];

                // Intersect against bbox
                float2 s = fast_intersect_bbox1(node, invdir, oxinvdir, t_max);
                if (s.x <= s.y)
                {
                    // Check if the node is a leaf
                    if (LEAFNODE(node))
                    {
						int const segment_idx = STARTIDX(node);
						Segment const segment = segments[segment_idx];
						float4 const v1 = vertices[segment.idx[0]];
						float4 const v2 = vertices[segment.idx[1]];

						// Intersect capsule
						float u;
						float const f = intersect_capsule(r, v1, v2, t_max, &u);

                        // If hit store the result and bail out
                        if (f < t_max)
                        {
                            hits[global_id] = HIT_MARKER;
                            return;
                        }
                    }
                    else
                    {
                        // Move to next node otherwise.
                        // Left child is always at addr + 1
                        ++addr;
                        continue;
                    }
                }

                addr = NEXT(node);
            }

            // Finished traversal, but no intersection found
            hits[global_id] = MISS_MARKER;
        }
    }
}