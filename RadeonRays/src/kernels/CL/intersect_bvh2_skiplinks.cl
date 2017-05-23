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
    \file intersect_bvh2_skiplinks.cl
    \author Dmitry Kozlov
    \version 1.0
    \brief Intersector implementation based on BVH with skip links.

    IntersectorSkipLinks implementation is based on the following paper:
    "Efficiency Issues for Ray Tracing" Brian Smits
    http://www.cse.chalmers.se/edu/year/2016/course/course/TDA361/EfficiencyIssuesForRayTracing.pdf

    Intersector is using binary BVH with a single bounding box per node. BVH layout guarantees
    that left child of an internal node lies right next to it in memory. Each BVH node has a 
    skip link to the node traversed next. The traversal pseude code is

        while(addr is valid)
        {
            node <- fetch next node at addr
            if (rays intersects with node bbox)
            {
                if (node is leaf)
                    intersect leaf
                else
                {
                    addr <- addr + 1 (follow left child)
                    continue
                }
            }

            addr <- skiplink at node (follow next)
        }

    Pros:
        -Simple and efficient kernel with low VGPR pressure.
        -Can traverse trees of arbitrary depth.
    Cons:
        -Travesal order is fixed, so poor algorithmic characteristics.
        -Does not benefit from BVH quality optimizations.
 */

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

#define SHAPETYPE_TRI 0
#define SHAPETYPE_SEG 1

/*************************************************************************
 TYPE DEFINITIONS
 **************************************************************************/
typedef bbox bvh_node;

typedef struct
{
	int idx[3]; // vertex indices
	int shape_mask;
	int shape_id;
	int prim_id;
	int type_id; // Type ID (SHAPETYPE_TRI or SHAPETYPE_SEG)
} Primitive;

__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL 
void intersect_main(
	GLOBAL bvh_node const* restrict nodes,        // BVH nodes
	GLOBAL float3 const* restrict mesh_vertices,  // Mesh vertices
	GLOBAL float4 const* restrict curve_vertices, // Curve vertices
	GLOBAL Primitive const* restrict primitives,  // Primitive indices
	GLOBAL ray const* restrict rays,              // Rays
	GLOBAL int const* restrict num_rays,          // Number of rays
	GLOBAL Intersection* hits                     // Hit data
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

            // Current closest primitive index
            int isect_idx = INVALID_IDX;
			int isect_type = -1;
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
                        int const primitive_idx = STARTIDX(node);
						Primitive const prim = primitives[primitive_idx];
                        
						if (prim.type_id == SHAPETYPE_TRI)
						{
							float3 const v1 = mesh_vertices[prim.idx[0]];
							float3 const v2 = mesh_vertices[prim.idx[1]];
							float3 const v3 = mesh_vertices[prim.idx[2]];

							// Intersect triangle
							float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);

							// If hit update closest hit distance and index
							if (f < t_max)
							{
								t_max = f;
								isect_idx = face_idx;
								isect_type = SHAPETYPE_TRI;
							}
						}
						else
						{
							float4 const v1 = curve_vertices[prim.idx[0]];
							float4 const v2 = curve_vertices[prim.idx[1]];

							// Intersect capsule
							float const f = intersect_capsule(r, v1, v2, t_max, &U_COORD);

							// If hit update closest hit distance and index
							if (f < t_max)
							{
								t_max = f;
								isect_idx = segment_idx;
								isect_type = SHAPETYPE_SEG;
							}
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
				Primitive const prim = primitives[isect_idx];
				if (isect_type == SHAPETYPE_TRI)
				{
					float3 const v1 = mesh_vertices[prim.idx[0]];
					float3 const v2 = mesh_vertices[prim.idx[1]];
					float3 const v3 = mesh_vertices[prim.idx[2]];
					float3 const p = r.o.xyz + r.d.xyz * t_max;
					float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3);
					hits[global_id].shape_id = prim.shape_id;
					hits[global_id].prim_id = prim.prim_id;
					hits[global_id].uvwt = make_float4(uv.x, uv.y, 0.f, t_max);
				}
				else
				{
					hits[global_id].shape_id = segment.shape_id;
					hits[global_id].prim_id = segment.prim_id;
					hits[global_id].uvwt = make_float4(U_COORD, 0.f, 0.f, t_max)
				}
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
	GLOBAL bvh_node const* restrict nodes,        // BVH nodes
	GLOBAL float3 const* restrict mesh_vertices,  // Mesh vertices
	GLOBAL float4 const* restrict curve_vertices, // Curve vertices
	GLOBAL Primitive const* restrict primitives,  // Primitive indices
	GLOBAL ray const* restrict rays,              // Rays
	GLOBAL int const* restrict num_rays,          // Number of rays
	GLOBAL Intersection* hits                     // Hit data
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
						int const primitive_idx = STARTIDX(node);
						Primitive const prim = primitives[primitive_idx];

						if (prim.type_id == SHAPETYPE_TRI)
						{
							float3 const v1 = mesh_vertices[prim.idx[0]];
							float3 const v2 = mesh_vertices[prim.idx[1]];
							float3 const v3 = mesh_vertices[prim.idx[2]];

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
							float4 const v1 = curve_vertices[prim.idx[0]];
							float4 const v2 = curve_vertices[prim.idx[1]];

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