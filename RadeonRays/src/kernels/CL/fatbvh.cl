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
#define SHORT_STACK_SIZE 8

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
                return true;
            }
        }
    }

    return false;
}

#ifndef GLOBAL_STACK
// intersect Ray against the whole BVH structure
inline
bool IntersectSceneClosest(SceneData const* scenedata, ray const* r, Intersection* isect, __global int* stack, __local int* ldsstack)
{
    const float3 invdir = native_recip(r->d.xyz);

    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w);
    isect->shapeid = -1;
    isect->primid = -1;

    __global int* gsptr = stack;
    __local  int* lsptr = ldsstack;

    *lsptr = -1;
    lsptr += 64;

    int idx = 0;
    FatBvhNode node;

    int2 deferred[8];
    int cnt = 0;

    while (idx > -1)
    {
        while (idx > -1 && cnt < 5)
        {
            node = scenedata->nodes[idx];

            bool l0 = LEAFNODE(node.lbound);
            bool l1 = LEAFNODE(node.rbound);
            float d0 = -1.f;
            float d1 = -1.f;

            if (l0)
            {
                //IntersectLeafClosest(scenedata, STARTIDX(node.lbound), NUMPRIMS(node.lbound), r, isect);
                deferred[cnt].x = STARTIDX(node.lbound);
                deferred[cnt].y = NUMPRIMS(node.lbound);
                cnt++;
            }
            else
            {
                d0 = IntersectBoxF(r, invdir, node.lbound, isect->uvwt.w);
            }

            if (l1)
            {
                //IntersectLeafClosest(scenedata, STARTIDX(node.rbound), NUMPRIMS(node.rbound), r, isect);
                deferred[cnt].x = STARTIDX(node.rbound);
                deferred[cnt].y = NUMPRIMS(node.rbound);
                cnt++;
            }
            else
            {
                d1 = IntersectBoxF(r, invdir, node.rbound, isect->uvwt.w);
            }

            if (d0 > 0 && d1 > 0)
            {
                int deferred = -1;
                if (d0 > d1)
                {
                    idx = (int)node.rbound.pmax.w;
                    deferred = (int)node.lbound.pmax.w;;
                }
                else
                {
                    idx = (int)node.lbound.pmax.w;
                    deferred = (int)node.rbound.pmax.w;
                }

                if (lsptr - ldsstack >= SHORT_STACK_SIZE * 64)
                {
                    for (int i = 1; i < SHORT_STACK_SIZE; ++i)
                    {
                        gsptr[i] = ldsstack[i * 64];
                    }

                    gsptr += SHORT_STACK_SIZE;
                    lsptr = ldsstack + 64;
                }

                *lsptr = deferred;
                lsptr += 64;

                continue;
            }
            else if (d0 > 0)
            {
                idx = (int)node.lbound.pmax.w;
                continue;
            }
            else if (d1 > 0)
            {
                idx = (int)node.rbound.pmax.w;
                continue;
            }

            lsptr -= 64;
            idx = *(lsptr);
        }

        if (gsptr > stack)
        {
            gsptr -= SHORT_STACK_SIZE;

            for (int i = 1; i < SHORT_STACK_SIZE; ++i)
            {
                ldsstack[i * 64] = gsptr[i];
            }

            lsptr = ldsstack + (SHORT_STACK_SIZE - 1) * 64;
            idx = ldsstack[64 * (SHORT_STACK_SIZE - 1)];
        }

        for (int i = 0; i < cnt; ++i)
        {
            IntersectLeafClosest(scenedata, deferred[i].x, deferred[i].y, r, isect);
        }

        cnt = 0;
    }

    return isect->shapeid >= 0;
}
#else
// intersect Ray against the whole BVH structure
inline
bool IntersectSceneClosest(SceneData const* scenedata, ray const* r, Intersection* isect)
{
    const float3 invdir = native_recip(r->d.xyz);

    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w);
    isect->shapeid = -1;
    isect->primid = -1;

    int stack[32];
    int* sptr = stack;
    *sptr++ = -1;

    int idx = 0;
    FatBvhNode node;
    //int2 deferred[6];
    //int cnt = 0;

    //while (idx > -1)
    //{
        while (idx > -1 /*&& cnt < 4*/)
        {
            node = scenedata->nodes[idx];

            float2 d = -1.f;

            for (int i = 0; i < 2; ++i)
            {
                if (LEAFNODE(node.bounds[i]))
                {
                    IntersectLeafClosest(scenedata, STARTIDX(node.bounds[i]), NUMPRIMS(node.bounds[i]), r, isect);
                    //deferred[cnt].x = STARTIDX(node.bounds[i]);
                    //deferred[cnt].y = NUMPRIMS(node.bounds[i]);
                    //cnt++;
                }
                else
                {
                    d[i] = IntersectBoxF(r, invdir, node.bounds[i], isect->uvwt.w);
                }
            }

            if (d[0] > 0.f && d[1] > 0.f)
            {
                int deferred = -1;
                if (d[0] < d[1])
                {
                    idx = (int)node.bounds[0].pmax.w;
                    deferred = (int)node.bounds[1].pmax.w;
                }
                else
                {
                    idx = (int)node.bounds[1].pmax.w;
                    deferred = (int)node.bounds[0].pmax.w;
                }

                *sptr++ = deferred;
                //continue;
            }
            else if (d[0] > 0.f)
            {
                idx = (int)node.bounds[0].pmax.w;
                //continue;

            }
            else
            {
                idx = (int)node.bounds[1].pmax.w;
                //continue;

            }

            if (d[0] < 0.f && d[1] < 0.f)
                idx = *--sptr;
        }

       /* for (int i = 0; i < cnt; ++i)
        {
            IntersectLeafClosest(scenedata, deferred[i].x, deferred[i].y, r, isect);
        }

        cnt = 0;*/
    //}

    return isect->shapeid >= 0;
}
#endif

#ifndef GLOBAL_STACK
// intersect Ray against the whole BVH structure
inline
bool IntersectSceneAny(SceneData const* scenedata, ray const* r, __global int* stack, __local int* ldsstack)
{
    const float3 invdir = native_recip(r->d.xyz);

    __global int* gsptr = stack;
    __local  int* lsptr = ldsstack;

    if (r->o.w < 0.f)
        return false;

    *lsptr = -1;
    lsptr += 64;

    int idx = 0;
    FatBvhNode node;

    bool leftleaf = false;
    bool rightleaf = false;
    float lefthit = 0.f;
    float righthit = 0.f;

    while (idx > -1)
    {
        while (idx > -1)
        {
            node = scenedata->nodes[idx];

            leftleaf = LEAFNODE(node.lbound);
            rightleaf = LEAFNODE(node.rbound);

            lefthit = leftleaf ? -1.f : IntersectBoxF(r, invdir, node.lbound, r->o.w);
            righthit = rightleaf ? -1.f : IntersectBoxF(r, invdir, node.rbound, r->o.w);

            if (leftleaf)
            {
                if (IntersectLeafAny(scenedata, STARTIDX(node.lbound), NUMPRIMS(node.lbound), r))
                                 return true;
            }

            if (rightleaf)
            {
                if (IntersectLeafAny(scenedata, STARTIDX(node.rbound), NUMPRIMS(node.rbound), r))
                                return true;
            }

            if (lefthit > 0.f && righthit > 0.f)
            {
                int deferred = -1;
                if (lefthit > righthit)
                {
                    idx = (int)node.rbound.pmax.w;
                    deferred = (int)node.lbound.pmax.w;;

                }
                else
                {
                    idx = (int)node.lbound.pmax.w;
                    deferred = (int)node.rbound.pmax.w;
                }

                if (lsptr - ldsstack >= SHORT_STACK_SIZE * 64)
                {
                    for (int i = 1; i < SHORT_STACK_SIZE; ++i)
                    {
                        gsptr[i] = ldsstack[i * 64];
                    }

                    gsptr += SHORT_STACK_SIZE;
                    lsptr = ldsstack + 64;
                }

                *lsptr = deferred;
                lsptr += 64;
                continue;
            }
            else if (lefthit > 0)
            {
                idx = (int)node.lbound.pmax.w;
                continue;
            }
            else if (righthit > 0)
            {
                idx = (int)node.rbound.pmax.w;
                continue;
            }

            lsptr -= 64;
            idx = *(lsptr);
        }

        if (gsptr > stack)
        {
            gsptr -= SHORT_STACK_SIZE;

            for (int i = 1; i < SHORT_STACK_SIZE; ++i)
            {
                ldsstack[i * 64] = gsptr[i];
            }

            lsptr = ldsstack + (SHORT_STACK_SIZE - 1) * 64;
            idx = ldsstack[64 * (SHORT_STACK_SIZE - 1)];
        }
    }

    return false;
}
#else
// intersect Ray against the whole BVH structure
bool IntersectSceneAny(SceneData const* scenedata, ray const* r)
{
    const float3 invdir = native_recip(r->d.xyz);

    if (r->o.w < 0.f)
        return false;
    
    int stack[32];

    int* sptr = stack;
    *sptr++ = -1;

    int idx = 0;
    FatBvhNode node;

    bool leftleaf = false;
    bool rightleaf = false;
    float lefthit = 0.f;
    float righthit = 0.f;
    int step = 0;

    bool found = false;

    while (idx > -1)
    {
        node = scenedata->nodes[idx];

        leftleaf = LEAFNODE(node.lbound);
        rightleaf = LEAFNODE(node.rbound);

        lefthit = leftleaf ? -1.f : IntersectBoxF(r, invdir, node.lbound, r->o.w);
        righthit = rightleaf ? -1.f : IntersectBoxF(r, invdir, node.rbound, r->o.w);

        if (leftleaf)
        {
            if (IntersectLeafAny(scenedata, STARTIDX(node.lbound), NUMPRIMS(node.lbound), r))
            {
                found = true;
                break;
            }
        }
        
        if (rightleaf)
        {
            if (IntersectLeafAny(scenedata, STARTIDX(node.rbound), NUMPRIMS(node.rbound), r))
                    {
                        found = true;
                        break;
                    }
        }

        if (lefthit > 0.f && righthit > 0.f)
        {
            int deferred = -1;
            if (lefthit > righthit)
            {
                idx = (int)node.rbound.pmax.w;
                deferred = (int)node.lbound.pmax.w;;
            }
            else
            {
                idx = (int)node.lbound.pmax.w;
                deferred = (int)node.rbound.pmax.w;
            }

                    *sptr++ = deferred;
        }
        else if (lefthit > 0)
        {
            idx = (int)node.lbound.pmax.w;
        }
        else if (righthit > 0)
        {
            idx = (int)node.rbound.pmax.w;
        }

        if (lefthit <= 0.f && righthit <= 0.f)
            idx = *--sptr;
    }

    return found;
}

#endif

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
    __global Intersection* hits // Hit datas
    , __global int* stack
    )
{
    __local int ldsstack[SHORT_STACK_SIZE * 64];

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
            // Calculate closest hit
            Intersection isect;
#ifndef GLOBAL_STACK 
            IntersectSceneClosest(&scenedata, &r, &isect, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id);
#else
            IntersectSceneClosest(&scenedata, &r, &isect);
#endif

            // Write data back in case of a hit
            hits[global_id] = isect;
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
    __global int* hitresults  // Hit results
    , __global int* stack
    )
{

    __local int ldsstack[SHORT_STACK_SIZE * 64];
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
#ifndef GLOBAL_STACK 
            hitresults[global_id] = IntersectSceneAny(&scenedata, &r, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id) ? 1 : -1;
#else
            hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1;
#endif
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
    __global Intersection* hits // Hit datas
    , __global int* stack
    )
{
    __local int ldsstack[SHORT_STACK_SIZE * 64];

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
#ifndef GLOBAL_STACK 
            IntersectSceneClosest(&scenedata, &r, &isect, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id);
#else
            IntersectSceneClosest(&scenedata, &r, &isect);
#endif
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
    __global int* hitresults   // Hit results
    , __global int* stack
    )
{
    __local int ldsstack[SHORT_STACK_SIZE * 64];
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
#ifndef GLOBAL_STACK 
            hitresults[global_id] = IntersectSceneAny(&scenedata, &r, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id) ? 1 : -1;
#else
            hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1;
#endif
        }
    }
}
