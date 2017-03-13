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
TYPE DEFINITIONS
**************************************************************************/
#define LEAFNODE(x) (((x).left) == -1)
#define SHORT_STACK_SIZE 16
#define WAVEFRONT_SIZE 64

//#define GLOBAL_STACK

typedef struct
{
    union {
            struct
            {
                // node's bounding box
                bbox lbound;
                bbox rbound;
            };

            struct
            {
                int i1, i2, i3, left;
                int shapeid, primid, extra, right;
            };

        struct
        {
            bbox bounds[2];
        };
    };

} FatBvhNode;

/*************************************************************************
HELPER FUNCTIONS
**************************************************************************/



__attribute__((always_inline)) 
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

__attribute__((always_inline)) 
float fast_intersect_bbox(bbox box, float3 invdir, float3 oxinvdir, float t_max)
{
    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir);
    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir);
    float3 const tmax = max(f, n);
    float3 const tmin = min(f, n);
    float const t1 = min(amd_min3(tmax.x, tmax.y, tmax.z), t_max);
    float const t0 = max(amd_max3(tmin.x, tmin.y, tmin.z), 0.f);

    if (t1 >= t0)
    {
        return t1;
    }
    else
    {
        return -1.f;
    }
}

__attribute__((always_inline)) 
float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max)
{
    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir);
    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir);
    float3 const tmax = max(f, n);
    float3 const tmin = min(f, n);
    float const t1 = min(amd_min3(tmax.x, tmax.y, tmax.z), t_max);
    float const t0 = max(amd_max3(tmin.x, tmin.y, tmin.z), 0.001f);
    return make_float2(t0, t1);
}

__attribute__((always_inline)) 
float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3)
{
    float3 const e1 = v2 - v1;
    float3 const e2 = v3 - v1;
    float3 const e = p - v1;
    float const d00 = dot(e1, e1);
    float const d01 = dot(e1,e2);
    float const d11 = dot(e2, e2);
    float const d20 = dot(e, e1);
    float const d21 = dot(e, e2);
    float const denom = d00 * d11 - d01 * d01;
    float const b1 = (d11 * d20 - d01 * d21) / denom;
    float const b2 = (d00 * d21 - d01 * d20) / denom;
    return make_float2(b1, b2);
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void
IntersectClosest(
    // Input
    __global FatBvhNode const *nodes, // BVH nodes
    __global float3 const *vertices,  // Scene positional data
    __global Face const *faces,       // Scene indices
    __global ShapeData const *shapes, // Shape data
    __global ray const *rays,         // Ray workload
    int offset,                       // Offset in rays array
    int numrays,                      // Number of rays to process
    __global int const *displacement,
    __global int const *hashmap,
    int t,
    __global Intersection *hits // Hit datas
)
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);
    int group_id = get_group_id(0);

    if (global_id < numrays)
    {
        ray r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            float3 const invdir = native_recip(r.d.xyz);
            float3 const oxinvdir = -r.o.xyz * invdir;
            float t_max = r.o.w;

            int bittrail = 0;
            int nodeidx = 1;
            int addr = 0;
            int faceidx = -1;

            while (addr > -1)
            {
                FatBvhNode node = nodes[addr];
                if (LEAFNODE(node))
                {
                    float3 v1, v2, v3;
                    v1 = vertices[node.i1];
                    v2 = vertices[node.i2];
                    v3 = vertices[node.i3];
                    float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);
                    if (f < t_max)
                    {
                        faceidx = addr;
                        t_max = f;
                    }
                }
                else
                {
                    float2 s0 = fast_intersect_bbox1(node.lbound, invdir, oxinvdir, t_max);
                    float2 s1 = fast_intersect_bbox1(node.rbound, invdir, oxinvdir, t_max);

                    bool traverse_c0 = (s0.x <= s0.y);
                    bool traverse_c1 = (s1.x <= s1.y);
                    bool c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;

                        if (traverse_c0 && traverse_c1)
                        {
                            bittrail = bittrail ^ 0x1;
                        }

                        if (c1first || !traverse_c0)
                        {
                            addr = node.right;
                            nodeidx = nodeidx ^ 0x1;
                        }
                        else
                        {
                            addr = node.left;
                        }

                        continue;
                    }
                }

                if (bittrail == 0)
                {
                    addr = -1;
                    continue;
                }

                int num_levels = 31 - clz(bittrail & -bittrail);
                bittrail = (bittrail >> num_levels) ^ 0x1;
                nodeidx = (nodeidx >> num_levels) ^ 0x1;

                int d = displacement[nodeidx / t];
                addr = hashmap[d + (nodeidx & (t - 1))];
            }

            if (faceidx != -1)
            {
                FatBvhNode node = nodes[faceidx];
                float3 v1 = vertices[node.i1];
                float3 v2 = vertices[node.i2];
                float3 v3 = vertices[node.i3];

                float3 const p = r.o.xyz + r.d.xyz * t_max;
                float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3);

                hits[global_id].shapeid = node.shapeid;
                hits[global_id].primid = node.primid;
                hits[global_id].uvwt = make_float4(uv.x, uv.y, 0.f, t_max);
            }
            else
            {
                hits[global_id].shapeid = -1;
                hits[global_id].primid = -1;
            }
        }
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void
IntersectAny(
    // Input
    __global FatBvhNode const *nodes, // BVH nodes
    __global float3 const *vertices,  // Scene positional data
    __global Face const *faces,       // Scene indices
    __global ShapeData const *shapes, // Shape data
    __global ray const *rays,         // Ray workload
    int offset,                       // Offset in rays array
    int numrays,                      // Number of rays to process
    __global int const *displacement,
    __global int const *hashmap,
    int t,
    __global int *hitresults // Hit results
    )
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);
    int group_id = get_group_id(0);

    if (global_id < numrays)
    {
        ray r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            float3 const invdir = native_recip(r.d.xyz);
            float3 const oxinvdir = -r.o.xyz * invdir;
            float t_max = r.o.w;

            int bittrail = 0;
            int nodeidx = 1;
            int addr = 0;
            int faceidx = -1;

            while (addr > -1)
            {
                FatBvhNode node = nodes[addr];
                if (LEAFNODE(node))
                {
                    float3 v1, v2, v3;
                    v1 = vertices[node.i1];
                    v2 = vertices[node.i2];
                    v3 = vertices[node.i3];
                    float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);
                    if (f < t_max)
                    {
                        hitresults[global_id] = 1;
                        return;
                    }
                }
                else
                {
                    float2 s0 = fast_intersect_bbox1(node.lbound, invdir, oxinvdir, t_max);
                    float2 s1 = fast_intersect_bbox1(node.rbound, invdir, oxinvdir, t_max);

                    bool traverse_c0 = (s0.x <= s0.y);
                    bool traverse_c1 = (s1.x <= s1.y);
                    bool c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;

                        if (traverse_c0 && traverse_c1)
                        {
                            bittrail = bittrail ^ 0x1;
                        }

                        if (c1first || !traverse_c0)
                        {
                            addr = node.right;
                            nodeidx = nodeidx ^ 0x1;
                        }
                        else
                        {
                            addr = node.left;
                        }

                        continue;
                    }
                }

                if (bittrail == 0)
                {
                    addr = -1;
                    continue;
                }

                int num_levels = 31 - clz(bittrail & -bittrail);
                bittrail = (bittrail >> num_levels) ^ 0x1;
                nodeidx = (nodeidx >> num_levels) ^ 0x1;

                int d = displacement[nodeidx / t];
                addr = hashmap[d + (nodeidx & (t - 1))];
            }

            hitresults[global_id] = -1;
        }
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void
IntersectAnyRC(
    // Input
    __global FatBvhNode const *nodes, // BVH nodes
    __global float3 const *vertices,  // Scene positional data
    __global Face const *faces,       // Scene indices
    __global ShapeData const *shapes, // Shape data
    __global ray const *rays,         // Ray workload
    int offset,                       // Offset in rays array
    __global int const *numrays,      // Number of rays in the workload
    __global int const *displacement,
    __global int const *hashmap,
    int t,
    __global int *hitresults // Hit results
    )
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);
    int group_id = get_group_id(0);

    // Handle only working subset
    if (global_id < *numrays)
    {
         ray r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            float3 const invdir = native_recip(r.d.xyz);
            float3 const oxinvdir = -r.o.xyz * invdir;
            float t_max = r.o.w;

            int bittrail = 0;
            int nodeidx = 1;
            int addr = 0;
            int faceidx = -1;

            while (addr > -1)
            {
                FatBvhNode node = nodes[addr];
                if (LEAFNODE(node))
                {
                    float3 v1, v2, v3;
                    v1 = vertices[node.i1];
                    v2 = vertices[node.i2];
                    v3 = vertices[node.i3];
                    float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);
                    if (f < t_max)
                    {
                        hitresults[global_id] = 1;
                        return;
                    }
                }
                else
                {
                    float2 s0 = fast_intersect_bbox1(node.lbound, invdir, oxinvdir, t_max);
                    float2 s1 = fast_intersect_bbox1(node.rbound, invdir, oxinvdir, t_max);

                    bool traverse_c0 = (s0.x <= s0.y);
                    bool traverse_c1 = (s1.x <= s1.y);
                    bool c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;

                        if (traverse_c0 && traverse_c1)
                        {
                            bittrail = bittrail ^ 0x1;
                        }

                        if (c1first || !traverse_c0)
                        {
                            addr = node.right;
                            nodeidx = nodeidx ^ 0x1;
                        }
                        else
                        {
                            addr = node.left;
                        }

                        continue;
                    }
                }

                if (bittrail == 0)
                {
                    addr = -1;
                    continue;
                }

                int num_levels = 31 - clz(bittrail & -bittrail);
                bittrail = (bittrail >> num_levels) ^ 0x1;
                nodeidx = (nodeidx >> num_levels) ^ 0x1;

                int d = displacement[nodeidx / t];
                addr = hashmap[d + (nodeidx & (t - 1))];
            }

            hitresults[global_id] = -1;
        }
    }
}




__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void
IntersectClosestRC(
    __global FatBvhNode const *nodes, // BVH nodes
    __global float3 const *vertices,  // Scene positional data
    __global Face const *faces,       // Scene indices
    __global ShapeData const *shapes, // Shape data
    __global ray const *rays,         // Ray workload
    int offset,                       // Offset in rays array
    __global int const *numrays,      // Number of rays in the workload
    __global int const *displacement,
    __global int const *hashmap,
    int t,
    __global Intersection *hits)
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);
    int group_id = get_group_id(0);

    // Handle only working subset
    if (global_id < *numrays)
    {
        ray r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            float3 const invdir = native_recip(r.d.xyz);
            float3 const oxinvdir = -r.o.xyz * invdir;
            float t_max = r.o.w;

            int bittrail = 0;
            int nodeidx = 1;
            int addr = 0;
            int faceidx = -1;

            while (addr > -1)
            {
                FatBvhNode node = nodes[addr];
                if (LEAFNODE(node))
                {
                    float3 v1, v2, v3;
                    v1 = vertices[node.i1];
                    v2 = vertices[node.i2];
                    v3 = vertices[node.i3];
                    float const f = fast_intersect_triangle(r, v1, v2, v3, t_max);
                    if (f < t_max)
                    {
                        faceidx = addr;
                        t_max = f;
                    }
                }
                else
                {
                    float2 s0 = fast_intersect_bbox1(node.lbound, invdir, oxinvdir, t_max);
                    float2 s1 = fast_intersect_bbox1(node.rbound, invdir, oxinvdir, t_max);

                    bool traverse_c0 = (s0.x <= s0.y);
                    bool traverse_c1 = (s1.x <= s1.y);
                    bool c1first = traverse_c1 && (s0.x > s1.x);

                    if (traverse_c0 || traverse_c1)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;

                        if (traverse_c0 && traverse_c1)
                        {
                            bittrail = bittrail ^ 0x1;
                        }

                        if (c1first || !traverse_c0)
                        {
                            addr = node.right;
                            nodeidx = nodeidx ^ 0x1;
                        }
                        else
                        {
                            addr = node.left;
                        }

                        continue;
                    }
                }

                if (bittrail == 0)
                {
                    addr = -1;
                    continue;
                }

                int num_levels = 31 - clz(bittrail & -bittrail);
                bittrail = (bittrail >> num_levels) ^ 0x1;
                nodeidx = (nodeidx >> num_levels) ^ 0x1;

                int d = displacement[nodeidx / t];
                addr = hashmap[d + (nodeidx & (t - 1))];
            }

            if (faceidx != -1)
            {
                FatBvhNode node = nodes[faceidx];
                float3 v1 = vertices[node.i1];
                float3 v2 = vertices[node.i2];
                float3 v3 = vertices[node.i3];

                float3 const p = r.o.xyz + r.d.xyz * t_max;
                float2 const uv = triangle_calculate_barycentrics(p, v1, v2, v3);

                hits[global_id].shapeid = node.shapeid;
                hits[global_id].primid = node.primid;
                hits[global_id].uvwt = make_float4(uv.x, uv.y, 0.f, t_max);
            }
            else
            {
                hits[global_id].shapeid = -1;
                hits[global_id].primid = -1;
            }
        }
    }
}
