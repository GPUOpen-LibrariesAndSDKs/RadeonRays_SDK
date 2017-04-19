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
    \file intersect_2level_skiplinkscl
    \author Dmitry Kozlov
    \version 1.0
    \brief Intersector implementation based on 2-level BVH with skip links.

    IntersectorSkipLinks implementation is based on the modification of the following paper:
    "Efficiency Issues for Ray Tracing" Brian Smits
    http://www.cse.chalmers.se/edu/year/2016/course/course/TDA361/EfficiencyIssuesForRayTracing.pdf

    Intersector is using binary BVH with a single bounding box per node. BVH layout guarantees
    that left child of an internal node lies right next to it in memory. Each BVH node has a 
    skip link to the node traversed next. Intersector builds its own BVH for each scene object 
    and then top level BVH across all bottom level BVHs. Top level leafs keep object transforms and
    might reference other leafs making instancing possible.


    Pros:
        -Simple and efficient kernel with low VGPR pressure.
        -Can traverse trees of arbitrary depth.
        -Supports motion blur.
        -Supports instancing.
        -Fast to refit.
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
#define SHAPEIDX(x)     (((int)(x.pmin.w)) >> 4)
#define LEAFNODE(x)     (((x).pmin.w) != -1.f)
#define NEXT(x)     ((int)((x).pmax.w))

/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/

typedef bbox bvh_node;

typedef struct
{
    // Shape ID
    int id;
    // Shape BVH index (bottom level)
    int bvh_idx;
    // Shape mask
    int mask;
    int padding1;
    // Transform
    float4 m0;
    float4 m1;
    float4 m2;
    float4 m3;
    // Motion blur params
    float4 velocity_linear;
    float4 velocity_angular;
} Shape;

typedef struct
{
    // Vertex indices
    int idx[3];
    // Shape maks
    int shape_mask;
    // Shape ID
    int shape_id;
    // Primitive ID
    int prim_id;
} Face;


INLINE float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3)
{
    float3 res;
    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3;
    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3;
    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3;
    return res;
}

INLINE float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3)
{
    float3 res;
    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z;
    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z;
    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z;
    return res;
}

INLINE ray transform_ray(ray r, float4 m0, float4 m1, float4 m2, float4 m3)
{
    ray res;
    res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3);
    res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3);
    res.o.w = r.o.w;
    res.d.w = r.d.w;
    return res;
}


__attribute__((reqd_work_group_size(64, 1, 1)))
KERNEL void intersect_main(
    // BVH nodes
    GLOBAL bvh_node const* restrict nodes,
    // Vertices
    GLOBAL float3 const* restrict vertices,
    // Faces
    GLOBAL Face const* restrict faces,
    // Shapes
    GLOBAL Shape const* restrict shapes,
    // BVH root index
    int root_idx,              
    // Rays
    GLOBAL ray const* restrict rays,
    // Number of rays in ray buffer
    GLOBAL int const* restrict num_rays,
    // Hits 
    GLOBAL Intersection* hits
)
{
    int global_id = get_global_id(0);

    // Handle only working subset
    if (global_id < *num_rays)
    {
        // Fetch ray
        ray r = rays[global_id];

        if (ray_is_active(&r))
        {
            // Precompute invdir for bbox testing
            float3 invdir = safe_invdir(r);
            float3 invdirtop = invdir;
            float t_max = r.o.w;

            // We need to keep original ray around for returns from bottom hierarchy
            ray top_ray = r;
            // Fetch top level BVH index
            int addr = root_idx;

            // Set top index
            int top_addr = INVALID_IDX;
            // Current shape ID
            int shape_id = INVALID_IDX;
            // Closest shape ID
            int closest_shape_id = INVALID_IDX;
            int closest_prim_id = INVALID_IDX;
            float2 closest_barycentrics;
            while (addr != INVALID_IDX)
            {
                // Fetch next node
                bvh_node node = nodes[addr];

                // Intersect against bbox
                float2 s = fast_intersect_bbox1(node, invdir, -r.o.xyz * invdir, t_max);

                if (s.x <= s.y)
                {
                    if (LEAFNODE(node))
                    {
                        // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy)
                        // or containing another BVH (top level hierarhcy)
                        if (top_addr != INVALID_IDX)
                        {
                            // Intersect leaf here
                            //
                            int const face_idx = STARTIDX(node);
                            Face const face = faces[face_idx];
                            float3 const v1 = vertices[face.idx[0]];
                            float3 const v2 = vertices[face.idx[1]];
                            float3 const v3 = vertices[face.idx[2]];

                            // Intersect triangle
                            float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);
                            // If hit update closest hit distance and index
                            if (f < t_max)
                            {
                                t_max = f;
                                closest_prim_id = face.prim_id;
                                closest_shape_id = shape_id;

                                float3 const p = r.o.xyz + r.d.xyz * t_max;
                                // Calculte barycentric coordinates
                                closest_barycentrics = triangle_calculate_barycentrics(p, v1, v2, v3);
                            }

                            // And goto next node
                            addr = NEXT(node);
                        }
                        else
                        {
                            // This is top level hierarchy leaf
                            // Save top node index for return
                            top_addr = addr;
                            // Get shape descrition struct index
                            int shape_idx = SHAPEIDX(node);
                            // Get shape mask
                            int shape_mask = shapes[shape_idx].mask;
                            // Drill into 2nd level BVH only if the geometry is not masked vs current ray
                            // otherwise skip the subtree
                            if (ray_get_mask(&r) & shape_mask)
                            {
                                // Fetch bottom level BVH index
                                addr = shapes[shape_idx].bvh_idx;
                                shape_id = shapes[shape_idx].id;

                                // Fetch BVH transform
                                float4 wmi0 = shapes[shape_idx].m0;
                                float4 wmi1 = shapes[shape_idx].m1;
                                float4 wmi2 = shapes[shape_idx].m2;
                                float4 wmi3 = shapes[shape_idx].m3;

                                r = transform_ray(r, wmi0, wmi1, wmi2, wmi3);
                                // Recalc invdir
                                invdir = safe_invdir(r);
                                // And continue traversal of the bottom level BVH
                                continue;
                            }
                            else
                            {
                                addr = INVALID_IDX;
                            }
                        }
                    }
                    // Traverse child nodes otherwise.
                    else
                    {
                        // This is an internal node, proceed to left child (it is at current + 1 index)
                        addr = addr + 1;
                    }
                }
                else
                {
                    // We missed the node, goto next one
                    addr = NEXT(node);
                }

                // Here check if we ended up traversing bottom level BVH
                // in this case idx = -1 and topidx has valid value
                if (addr == INVALID_IDX && top_addr != INVALID_IDX)
                {
                    //  Proceed to next top level node
                    addr = NEXT(nodes[top_addr]);
                    // Set topidx
                    top_addr = INVALID_IDX;
                    // Restore ray here
                    r = top_ray;
                    // Restore invdir
                    invdir = invdirtop;
                }
            }

            // Check if we have found an intersection
            if (closest_shape_id != INVALID_IDX)
            {
                // Update hit information
                hits[global_id].shape_id = closest_shape_id;
                hits[global_id].prim_id = closest_prim_id;
                hits[global_id].uvwt = make_float4(closest_barycentrics.x, closest_barycentrics.y, 0.f, t_max);
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
KERNEL void occluded_main(
    // BVH nodes
    GLOBAL bvh_node const* restrict nodes,
    // Vertices
    GLOBAL float3 const* restrict vertices,
    // Faces
    GLOBAL Face const* restrict faces,
    // Shapes
    GLOBAL Shape const* restrict shapes,
    // BVH root index
    int root_idx,
    // Rays
    GLOBAL ray const* restrict rays,
    // Number of rays in ray buffer
    GLOBAL int const* restrict num_rays,
    // Hits 
    GLOBAL int* hits
)
{
    int global_id = get_global_id(0);

    // Handle only working subset
    if (global_id < *num_rays)
    {
        // Fetch ray
        ray r = rays[global_id];

        if (ray_is_active(&r))
        {
            // Precompute invdir for bbox testing
            float3 invdir = safe_invdir(r);
            float3 invdirtop = invdir;
            float const t_max = r.o.w;

            // We need to keep original ray around for returns from bottom hierarchy
            ray top_ray = r;

            // Fetch top level BVH index
            int addr = root_idx;
            // Set top index
            int top_addr = INVALID_IDX;

            while (addr != INVALID_IDX)
            {
                // Fetch next node
                bvh_node node = nodes[addr];
                // Intersect against bbox
                float2 s = fast_intersect_bbox1(node, invdir, -r.o.xyz * invdir, t_max);

                if (s.x <= s.y)
                {
                    if (LEAFNODE(node))
                    {
                        // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy)
                        // or containing another BVH (top level hierarhcy)
                        if (top_addr != INVALID_IDX)
                        {
                            // Intersect leaf here
                            //
                            int const face_idx = STARTIDX(node);
                            Face const face = faces[face_idx];
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

                            // And goto next node
                            addr = NEXT(node);
                        }
                        else
                        {
                            // This is top level hierarchy leaf
                            // Save top node index for return
                            top_addr = addr;
                            // Get shape descrition struct index
                            int shape_idx = SHAPEIDX(node);
                            // Get shape mask
                            int shape_mask = shapes[shape_idx].mask;
                            // Drill into 2nd level BVH only if the geometry is not masked vs current ray
                            // otherwise skip the subtree
                            if (ray_get_mask(&r) & shape_mask)
                            {
                                // Fetch bottom level BVH index
                                addr = shapes[shape_idx].bvh_idx;

                                // Fetch BVH transform
                                float4 wmi0 = shapes[shape_idx].m0;
                                float4 wmi1 = shapes[shape_idx].m1;
                                float4 wmi2 = shapes[shape_idx].m2;
                                float4 wmi3 = shapes[shape_idx].m3;

                                r = transform_ray(r, wmi0, wmi1, wmi2, wmi3);
                                // Recalc invdir
                                invdir = safe_invdir(r);;
                                // And continue traversal of the bottom level BVH
                                continue;
                            }
                            else
                            {
                                addr = INVALID_IDX;
                            }
                        }
                    }
                    // Traverse child nodes otherwise.
                    else
                    {
                        // This is an internal node, proceed to left child (it is at current + 1 index)
                        addr = addr + 1;
                    }
                }
                else
                {
                    // We missed the node, goto next one
                    addr = NEXT(node);
                }

                // Here check if we ended up traversing bottom level BVH
                // in this case idx = -1 and topidx has valid value
                if (addr == INVALID_IDX && top_addr != INVALID_IDX)
                {
                    //  Proceed to next top level node
                    addr = NEXT(nodes[top_addr]);
                    // Set topidx
                    top_addr = INVALID_IDX;
                    // Restore ray here
                    r = top_ray;
                    // Restore invdir
                    invdir = invdirtop;
                }
            }

            hits[global_id] = MISS_MARKER;
        }
    }
}