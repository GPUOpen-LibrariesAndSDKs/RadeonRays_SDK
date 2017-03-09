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
#define STARTIDX(x)     (((int)(x.pmin.w)) >> 4)
#define NUMPRIMS(x)     (((int)(x.pmin.w)) & 0xF)
#define LEAFNODE(x)     (((x).pmin.w) != -1.f)
#define CHILD(x,i)         ((int)x.bounds[i].pmax.w)
#define SHORT_STACK_SIZE 16
#define WAVEFRONT_SIZE 64

//#define GLOBAL_STACK

typedef struct
{
    union
    {
        struct
        {
            bbox lbound;
            bbox rbound;
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
    __global FatBvhNode const*     nodes;
    // Scene positional data
    __global float3 const*         vertices;
    // Scene indices
    __global Face const*         faces;
    // Shape IDs
    __global ShapeData const*     shapes;
    // Extra data
    __global int const*             extra;
} SceneData;

/*************************************************************************
HELPER FUNCTIONS
**************************************************************************/



/*************************************************************************
BVH FUNCTIONS
**************************************************************************/
//  intersect a ray with leaf BVH node
inline
void IntersectLeafClosest(
    SceneData const* scenedata,
    int startidx,
    int numprims,
    ray const* r,                // ray to instersect
    Intersection* isect          // Intersection structure
    )
{
    float3 v1, v2, v3;
    Face face;
    for (int i = 0; i < numprims; ++i)
    {
        face = scenedata->faces[startidx + i];
        v1 = scenedata->vertices[face.idx[0]];
        v2 = scenedata->vertices[face.idx[1]];
        v3 = scenedata->vertices[face.idx[2]];

#ifdef RR_RAY_MASK
        int shapemask = scenedata->shapes[face.shapeidx].mask;

        if (Ray_GetMask(r) & shapemask)
#endif
        {
            if (IntersectTriangle(r, v1, v2, v3, isect))
            {
                isect->primid = face.id;
                isect->shapeid = face.shapeidx;
            }
        }
    }
}

//  intersect a ray with leaf BVH node
inline
bool IntersectLeafAny(
    SceneData const* scenedata,
    int startidx,
    int numprims,
    ray const* r                      // ray to instersect
    )
{
    float3 v1, v2, v3;
    Face face;

    bool hit = false;
    for (int i = 0; i < numprims; ++i)
    {
        face = scenedata->faces[startidx + i];
        v1 = scenedata->vertices[face.idx[0]];
        v2 = scenedata->vertices[face.idx[1]];
        v3 = scenedata->vertices[face.idx[2]];

#ifdef RR_RAY_MASK
        int shapemask = scenedata->shapes[face.shapeidx].mask;

        if (Ray_GetMask(r) & shapemask)
#endif
        {
            if (IntersectTriangleP(r, v1, v2, v3))
            {
                hit =  true;
                break;
            }
        }
    }

    return hit;
}


inline
bool intersect(
    SceneData const* scenedata,
    ray const* r,
    Intersection* isect,
    __global int* displacement,
    __global int* hash,
    int t
    )
{
    const float3 invdir = native_recip(r->d.xyz);
    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w);
    isect->shapeid = -1;
    isect->primid = -1;

    int bittrail = 0;
    int nodeidx = 1;
    int addr = 0;

    FatBvhNode node;

    while (addr > -1)
    {
        node = scenedata->nodes[addr];
        bool l0 = LEAFNODE(node.lbound);
        bool l1 = LEAFNODE(node.rbound);
        float d0 = -1.f;
        float d1 = -1.f;

        if (l0)
        {
            IntersectLeafClosest(scenedata, STARTIDX(node.lbound), NUMPRIMS(node.lbound), r, isect);
        }
        else
        {
            d0 = IntersectBoxF(r, invdir, node.lbound, isect->uvwt.w);
        }

        if (l1)
        {
            IntersectLeafClosest(scenedata, STARTIDX(node.rbound), NUMPRIMS(node.rbound), r, isect);
        }
        else
        {
            d1 = IntersectBoxF(r, invdir, node.rbound, isect->uvwt.w);
        }

        if (d0 > 0 && d1 > 0)
        {
            bittrail = bittrail << 1;
            nodeidx = nodeidx << 1;
            bittrail = bittrail ^ 0x1;

            if (d0 > d1)
            {
                addr = CHILD(node, 1);
                nodeidx = nodeidx ^ 0x1;
            }
            else
            {
                addr = CHILD(node, 0);
            }
            continue;
        }
        else if (d0 > 0)
        {
            bittrail = bittrail << 1;
            nodeidx = nodeidx << 1;
            addr = CHILD(node, 0);
            continue;
        }
        else if (d1 > 0)
        {
            bittrail = bittrail << 1;
            nodeidx = nodeidx << 1;
            nodeidx = nodeidx ^ 0x1;
            addr = CHILD(node, 1);
            continue;
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
        addr = hash[d + (nodeidx & (t - 1))];
    }

    return isect->shapeid >= 0;
}

// intersect Ray against the whole BVH structure
inline
bool occluded(SceneData const* scenedata, ray const* r)
{
    const float3 invdir = native_recip(r->d.xyz);

    int stack[32];
    int* sptr = stack;
    *sptr++ = -1;

    int idx = 0;
    FatBvhNode node;

    while (idx > -1)
    {
        while (idx > -1)
        {
            node = scenedata->nodes[idx];

            bool l0 = LEAFNODE(node.lbound);
            bool l1 = LEAFNODE(node.rbound);
            float d0 = -1.f;
            float d1 = -1.f;

            if (l0)
            {
                bool hit = IntersectLeafAny(scenedata, STARTIDX(node.lbound), NUMPRIMS(node.lbound), r);
                if (hit)
                {
                    return true;
                }
            }
            else
            {
                d0 = IntersectBoxF(r, invdir, node.lbound, r->o.w);
            }

            if (l1)
            {
                bool hit = IntersectLeafAny(scenedata, STARTIDX(node.rbound), NUMPRIMS(node.rbound), r);
                if (hit)
                {
                    return true;
                }
            }
            else
            {
                d1 = IntersectBoxF(r, invdir, node.rbound, r->o.w);
            }

            if (d0 > 0 && d1 > 0)
            {
                int deferred = -1;
                if (d0 > d1)
                {
                    idx = CHILD(node, 1);
                    deferred = CHILD(node, 0);
                }
                else
                {
                    idx = CHILD(node, 0);
                    deferred = CHILD(node, 1);
                }

                *sptr++ = deferred;
                continue;
            }
            else if (d0 > 0)
            {
                idx = CHILD(node, 0);
                continue;
            }
            else if (d1 > 0)
            {
                idx = CHILD(node, 1);
                continue;
            }

            idx = *--sptr;
        }
    }

    return false;
}

__attribute__((always_inline))
float IntersectTriangleLight(ray r, float3 v1, float3 v2, float3 v3, float t_max)
{
    const float3 e1 = v2 - v1;
    const float3 e2 = v3 - v1;
    const float3 s1 = cross(r.d.xyz, e2);
    const float  invd = native_recip(dot(s1, e1));
    const float3 d = r.o.xyz - v1;
    const float  b1 = dot(d, s1) * invd;
    const float3 s2 = cross(d, e1);
    const float  b2 = dot(r.d.xyz, s2) * invd;
    const float temp = dot(e2, s2) * invd;
    
    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f
        || temp < 0.f || temp > t_max)
    {
        return t_max;
    }
    else
    {
        return temp;
    }
}


__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectClosest(
    // Input
    __global FatBvhNode const* nodes,   // BVH nodes
    __global float3 const* vertices, // Scene positional data
    __global Face const* faces,    // Scene indices
    __global ShapeData const* shapes, // Shape data
    __global ray const* rays,        // Ray workload
    int offset,                // Offset in rays array
    int numrays,               // Number of rays to process
    __global int const* displacement,
    __global int const* hashmap,
    int t,
    __global Intersection* hits // Hit datas
    )
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);
    int group_id = get_group_id(0);

    if (global_id < numrays)
    {
        // Fetch ray
        ray r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            // Calculate closest hit
            //intersect(&scenedata, &r, &isect, displacement, hashmap, t);
            float3 const invdir = native_recip(r.d.xyz);
            float3 const oxinvdir = r.o.xyz * invdir;
            float t_max = r.o.w;

            int bittrail = 0;
            int nodeidx = 1;
            int addr = 0;
            int faceidx = -1;
            int postponed = -1;
            //int deferred[3];
            //int cnt = 0;
            bool found = false;
            int p1 = -1;
            int p2 = -1;

            float3 v1, v2, v3;
            Face face;
            FatBvhNode node;

            while (addr > -1)
            {
                while (addr > -1 && !found)
                {
                    node = nodes[addr];
                    bool l0 = LEAFNODE(node.lbound);
                    bool l1 = LEAFNODE(node.rbound);
                    float d0 = -1.f;
                    float d1 = -1.f;

                    if (l0)
                    {
                        p1 = STARTIDX(node.lbound);
                        found = true;
                    }
                    else
                    {
                        const float3 f = mad(node.lbound.pmax.xyz, invdir, oxinvdir);
                        const float3 n = mad(node.lbound.pmin.xyz, invdir, oxinvdir);
                        const float3 tmax = max(f, n);
                        const float3 tmin = min(f, n);
                        const float t1 = amd_min3(tmax.x, tmax.y, tmax.z);
                        const float t0 = amd_max3(tmin.x, tmin.y, tmin.z);
                        if (t0 < t1)
                        {
                            d0 = t0 > 0.f ? t0 : min(t1, t_max); 
                        }
                    }

                    if (l1)
                    {
                        if (found)
                            p2 = STARTIDX(node.rbound);
                        else
                            p1 = STARTIDX(node.rbound);

                        found = true;
                    }
                    else
                    {
                        const float3 f = mad(node.rbound.pmax.xyz, invdir, oxinvdir);
                        const float3 n = mad(node.rbound.pmin.xyz, invdir, oxinvdir);
                        const float3 tmax = max(f, n);
                        const float3 tmin = min(f, n);
                        const float t1 = amd_min3(tmax.x, tmax.y, tmax.z);
                        const float t0 = amd_max3(tmin.x, tmin.y, tmin.z);
                        if (t0 < t1)
                        {
                            d1 = t0 > 0.f ? t0 : min(t1, t_max); 
                        }
                    }

                    if (d0 > 0 && d1 > 0)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;
                        bittrail = bittrail ^ 0x1;

                        if (d0 > d1)
                        {
                            addr = CHILD(node, 1);
                            nodeidx = nodeidx ^ 0x1;
                            postponed = CHILD(node, 0);
                        }
                        else
                        {
                            addr = CHILD(node, 0);
                            postponed = CHILD(node, 1);
                        }
                        continue;
                    }
                    else if (d0 > 0)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;
                        addr = CHILD(node, 0);
                        continue;
                    }
                    else if (d1 > 0)
                    {
                        bittrail = bittrail << 1;
                        nodeidx = nodeidx << 1;
                        nodeidx = nodeidx ^ 0x1;
                        addr = CHILD(node, 1);
                        continue;
                    }
                }

                if (found)
                {
                    face = faces[p1];
                    v1 = vertices[face.idx[0]];
                    v2 = vertices[face.idx[1]];
                    v3 = vertices[face.idx[2]];

                    float f = IntersectTriangleLight(r, v1, v2, v3, t_max);
                    if (f < t_max)
                    {
                        faceidx = p1;
                        t_max = f;
                    }

                    if (p2 != -1)
                    {
                        face = faces[p2];
                        v1 = vertices[face.idx[0]];
                        v2 = vertices[face.idx[1]];
                        v3 = vertices[face.idx[2]];

                        float f = IntersectTriangleLight(r, v1, v2, v3, t_max);
                        if (f < t_max)
                        {
                            faceidx = p2;
                            t_max = f;
                        }
                    }

                    p1 = p2 = -1;
                    found = false;
                }

                if (bittrail == 0)
                {
                    addr = -1;
                    continue;
                }

                int num_levels = 31 - clz(bittrail & -bittrail);
                bittrail = (bittrail >> num_levels) ^ 0x1;
                nodeidx = (nodeidx >> num_levels) ^ 0x1;

                if (postponed != -1)
                {
                    addr = postponed;
                    postponed = -1;
                    continue;
                }

                int d = displacement[nodeidx / t];
                addr = hashmap[d + (nodeidx & (t - 1))];
            }

            if (faceidx != -1)
            {
                face = faces[p2];
                hits[global_id].shapeid = face.shapeidx;
                hits[global_id].primid = face.id;

                v1 = vertices[face.idx[0]];
                v2 = vertices[face.idx[1]];
                v3 = vertices[face.idx[2]];

                const float3 e1 = v2 - v1;
                const float3 e2 = v3 - v1;
                const float3 s1 = cross(r.d.xyz, e2);
                const float  invd = native_recip(dot(s1, e1));
                const float3 d = r.o.xyz - v1;
                const float  b1 = dot(d, s1) * invd;
                const float3 s2 = cross(d, e1);
                const float  b2 = dot(r.d.xyz, s2) * invd;
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
__kernel void IntersectAny(
    // Input
    __global FatBvhNode const* nodes,   // BVH nodes
    __global float3 const* vertices, // Scene positional data
    __global Face const* faces,    // Scene indices
    __global ShapeData const* shapes,     // Shape data
    __global ray const* rays,        // Ray workload
    int offset,                // Offset in rays array
    int numrays,               // Number of rays to process
    __global int const* displacement,
    __global int const* hashmap,
    int t,
    __global int* hitresults  // Hit results
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
        0
    };

    if (global_id < numrays)
    {
        // Fetch ray
        ray r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            // Calculate any intersection
            hitresults[global_id] = occluded(&scenedata, &r) ? 1 : -1;
        }
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectClosestRC(
    __global FatBvhNode const* nodes,   // BVH nodes
    __global float3 const* vertices, // Scene positional data
    __global Face const* faces,      // Scene indices
    __global ShapeData const* shapes,     // Shape data
    __global ray const* rays,        // Ray workload
    int offset,                // Offset in rays array
    __global int const* numrays,     // Number of rays in the workload
    __global int const* displacement,
    __global int const* hashmap,
    int t,
    __global Intersection* hits
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
        0
    };

    // Handle only working subset
    if (global_id < *numrays)
    {
        // Fetch ray
        ray r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            // Calculate closest hit
            Intersection isect;
            intersect(&scenedata, &r, &isect, displacement, hashmap, t);
            // Write data back in case of a hit
            hits[global_id] = isect;
        }
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectAnyRC(
    // Input
    __global FatBvhNode const* nodes,   // BVH nodes
    __global float3 const* vertices, // Scene positional data
    __global Face const* faces,    // Scene indices
    __global ShapeData const* shapes,     // Shape data
    __global ray const* rays,        // Ray workload
    int offset,                // Offset in rays array
    __global int const* numrays,     // Number of rays in the workload
    __global int const* displacement,
    __global int const* hashmap,
    int t,
    __global int* hitresults   // Hit results
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
        0
    };

    // Handle only working subset
    if (global_id < *numrays)
    {
        // Fetch ray
        ray r = rays[global_id];

        if (Ray_IsActive(&r))
        {
            // Calculate any intersection
            hitresults[global_id] = occluded(&scenedata, &r) ? 1 : -1;
        }
    }
}
