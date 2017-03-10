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

typedef struct
{
    // BVH structure
    __global FatBvhNode const *nodes;
    // Scene positional data
    __global float3 const *vertices;
    // Scene indices
    __global Face const *faces;
    // Shape IDs
    __global ShapeData const *shapes;
    // Extra data
    __global int const *extra;
} SceneData;

/*************************************************************************
HELPER FUNCTIONS
**************************************************************************/



__attribute__((always_inline)) 
float IntersectTriangleLight(ray r, float3 v1, float3 v2, float3 v3, float t_max)
{
    const float3 e1 = v2 - v1;
    const float3 e2 = v3 - v1;
    const float3 s1 = cross(r.d.xyz, e2);
    const float invd = native_recip(dot(s1, e1));
    const float3 d = r.o.xyz - v1;
    const float b1 = dot(d, s1) * invd;
    const float3 s2 = cross(d, e1);
    const float b2 = dot(r.d.xyz, s2) * invd;
    const float temp = dot(e2, s2) * invd;

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
float intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max)
{
    const float3 f = mad(box.pmax.xyz, invdir, oxinvdir);
    const float3 n = mad(box.pmin.xyz, invdir, oxinvdir);

    const float3 tmax = max(f, n);
    const float3 tmin = min(f, n);

#ifndef AMD_MEDIA_OPS
    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), t_max);
    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f);
#else
    const float t1 = min(amd_min3(tmax.x, tmax.y, tmax.z), t_max);
    const float t0 = amd_max3(tmin.x, tmin.y, tmin.z);
#endif

    if (t1 >= t0)
    {
        return t0 > 0.f ? t0 : t1;
    }
    else
    {
        return -1.f;
    }
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

    SceneData scenedata =
    {
        nodes,
        vertices,
        faces,
        shapes,
        0 };

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
                    float f = IntersectTriangleLight(r, v1, v2, v3, t_max);
                    if (f < t_max)
                    {
                        faceidx = addr;
                        t_max = f;
                    }
                }
                else
                {
                    float d0 = intersect_bbox1(node.lbound, invdir, oxinvdir, t_max);
                    float d1 = intersect_bbox1(node.rbound, invdir, oxinvdir, t_max);

                    if (d0 > 0 || d1 > 0)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;
                    }

                    if (d0 > 0 && d1 > 0)
                    {

                        bittrail = bittrail ^ 0x1;

                        if (d0 > d1)
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
                    else if (d0 > 0)
                    {
                        addr = node.left;
                        continue;

                    }
                    else if (d1 > 0)
                    {
                        nodeidx = nodeidx ^ 0x1;
                        addr = node.right;
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
                float3 v1, v2, v3;
                hits[global_id].shapeid = node.shapeid;
                hits[global_id].primid = node.primid;
                v1 = vertices[node.i1];
                v2 = vertices[node.i2];
                v3 = vertices[node.i3];

                const float3 e1 = v2 - v1;
                const float3 e2 = v3 - v1;
                const float3 s1 = cross(r.d.xyz, e2);
                const float invd = native_recip(dot(s1, e1));
                const float3 d = r.o.xyz - v1;
                const float b1 = dot(d, s1) * invd;
                const float3 s2 = cross(d, e1);
                const float b2 = dot(r.d.xyz, s2) * invd;
                hits[global_id].uvwt = make_float4(b1, b2, 0.f, t_max);
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

    // Fill scene data
    SceneData scenedata =
        {
            nodes,
            vertices,
            faces,
            shapes,
            0};

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

            while (addr > -1)
            {
                FatBvhNode node = nodes[addr];

                if (LEAFNODE(node))
                {
                    float3 v1, v2, v3;
                    v1 = vertices[node.i1];
                    v2 = vertices[node.i2];
                    v3 = vertices[node.i3];
                    float f = IntersectTriangleLight(r, v1, v2, v3, t_max);
                    if (f < t_max)
                    {
                        hitresults[global_id] = 1;
                        return;
                    }
                }
                else
                {
                    float d0 = intersect_bbox1(node.lbound, invdir, oxinvdir, t_max);
                    float d1 = intersect_bbox1(node.rbound, invdir, oxinvdir, t_max);

                    if (d0 > 0 || d1 > 0)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;
                    }

                    if (d0 > 0 && d1 > 0)
                    {

                        bittrail = bittrail ^ 0x1;

                        if (d0 > d1)
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
                    else if (d0 > 0)
                    {
                        addr = node.left;
                        continue;

                    }
                    else if (d1 > 0)
                    {
                        nodeidx = nodeidx ^ 0x1;
                        addr = node.right;
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

    // Fill scene data
    SceneData scenedata =
        {
            nodes,
            vertices,
            faces,
            shapes,
            0};

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

            while (addr > -1)
            {
                FatBvhNode node = nodes[addr];

                if (LEAFNODE(node))
                {
                    float3 v1, v2, v3;
                    v1 = vertices[node.i1];
                    v2 = vertices[node.i2];
                    v3 = vertices[node.i3];
                    float f = IntersectTriangleLight(r, v1, v2, v3, t_max);
                    if (f < t_max)
                    {
                        hitresults[global_id] = 1;
                        return;
                    }
                }
                else
                {
                    float d0 = intersect_bbox1(node.lbound, invdir, oxinvdir, t_max);
                    float d1 = intersect_bbox1(node.rbound, invdir, oxinvdir, t_max);

                    if (d0 > 0 || d1 > 0)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;
                    }

                    if (d0 > 0 && d1 > 0)
                    {

                        bittrail = bittrail ^ 0x1;

                        if (d0 > d1)
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
                    else if (d0 > 0)
                    {
                        addr = node.left;
                        continue;

                    }
                    else if (d1 > 0)
                    {
                        nodeidx = nodeidx ^ 0x1;
                        addr = node.right;
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
                    float f = IntersectTriangleLight(r, v1, v2, v3, t_max);
                    if (f < t_max)
                    {
                        faceidx = addr;
                        t_max = f;
                    }
                }
                else
                {
                    float d0 = intersect_bbox1(node.lbound, invdir, oxinvdir, t_max);
                    float d1 = intersect_bbox1(node.rbound, invdir, oxinvdir, t_max);

                    if (d0 > 0 || d1 > 0)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;
                    }

                    if (d0 > 0 && d1 > 0)
                    {

                        bittrail = bittrail ^ 0x1;

                        if (d0 > d1)
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
                    else if (d0 > 0)
                    {
                        addr = node.left;
                        continue;

                    }
                    else if (d1 > 0)
                    {
                        nodeidx = nodeidx ^ 0x1;
                        addr = node.right;
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
                float3 v1, v2, v3;
                hits[global_id].shapeid = node.shapeid;
                hits[global_id].primid = node.primid;
                v1 = vertices[node.i1];
                v2 = vertices[node.i2];
                v3 = vertices[node.i3];

                const float3 e1 = v2 - v1;
                const float3 e2 = v3 - v1;
                const float3 s1 = cross(r.d.xyz, e2);
                const float invd = native_recip(dot(s1, e1));
                const float3 d = r.o.xyz - v1;
                const float b1 = dot(d, s1) * invd;
                const float3 s2 = cross(d, e1);
                const float b2 = dot(r.d.xyz, s2) * invd;
                hits[global_id].uvwt = make_float4(b1, b2, 0.f, t_max);
            }
            else
            {
                hits[global_id].shapeid = -1;
                hits[global_id].primid = -1;
            }
        }
    }
}
