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
EXTENSIONS
**************************************************************************/


/*************************************************************************
DEFINES
**************************************************************************/
#define PI 3.14159265358979323846f

#define LEAFNODE(x)     (((x).typecnt & 0xF) == 0)
#define NUMPRIMS(x)     (((x).typecnt) >> 4)


/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/
typedef struct _bbox
{
    float3 pmin;
    float3 pmax;
} bbox;

typedef struct _ray
{
    float3 o;
    float3 d;
    float2 range;
    float  time;
    float  padding;
} ray;

typedef struct _Intersection
{
    float2 uv;
    int instanceid;
    int shapeid;
    int primid;
    int extra;
} Intersection;


typedef struct _BvhNode
{
    bbox bounds;
    // 4-bit mask + prim count
    uint typecnt;

    union
    {
        // Start primitive index in case of a leaf node
        uint start;
        // Children indices
        uint c[3];
    };

    // Next node during traversal
    uint next;
    uint padding;
    uint padding1;
    uint padding2;
} BvhNode;

typedef struct _Face
{
    // Vertex indices
    int idx[4];
    // Primitive ID
    int id;
    // Idx count
    int cnt;
} Face;

#define NUMPRIMS(x)     (((x).typecnt) >> 1)
#define BVHROOTIDX(x)   (((x).start))
#define TRANSFROMIDX(x) (((x).typecnt) >> 1)
#define LEAFNODE(x)     (((x).typecnt & 0x1) == 0)

typedef struct 
{
    // BVH structure
    __global BvhNode*       nodes;
    // Scene positional data
    __global float3*        vertices;
    // Scene indices
    __global Face*          faces;
    // Shape IDs
    __global int*           shapeids;

} SceneData;


/*************************************************************************
HELPER FUNCTIONS
**************************************************************************/

#ifndef APPLE

float4 make_float4(float x, float y, float z, float w)
{
    float4 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    return res;
}

float3 make_float3(float x, float y, float z)
{
    float3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}

float2 make_float2(float x, float y)
{
    float2 res;
    res.x = x;
    res.y = y;
    return res;
}

int2 make_int2(int x, int y)
{
    int2 res;
    res.x = x;
    res.y = y;
    return res;
}

int3 make_int3(int x, int y, int z)
{
    int3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}

#endif

float3 transform_point(float3 p, float16 m)
{
    float3 res;
    res.x = m.s0 * p.x + m.s1 * p.y + m.s2 * p.z + m.s3;
    res.y = m.s4 * p.x + m.s5 * p.y + m.s6 * p.z + m.s7;
    res.z = m.s8 * p.x + m.s9 * p.y + m.sA * p.z + m.sB;
    return res;
}

float3 transform_vector(float3 p, float16 m)
{
    float3 res;
    res.x = m.s0 * p.x + m.s1 * p.y + m.s2 * p.z;
    res.y = m.s4 * p.x + m.s5 * p.y + m.s6 * p.z;
    res.z = m.s8 * p.x + m.s9 * p.y + m.sA * p.z;
    return res;
}

ray transform_ray(ray r, float16 m)
{
    ray res;
    res.o = transform_point(r.o, m);
    res.d = transform_vector(r.d, m);
    res.range = r.range;
    res.time = r.time;
    return res;
}

// Intersect Ray against triangle
bool IntersectTriangle(ray* r, float3 v1, float3 v2, float3 v3, float* a, float* b)
{
    float3 e1 = v2 - v1;
    float3 e2 = v3 - v1;
    float3 s1 = cross(r->d, e2);
    float  invd = 1.f / dot(s1, e1);
    float3 d = r->o - v1;
    float  b1 = dot(d, s1) * invd;
    float3 s2 = cross(d, e1);
    float  b2 = dot(r->d, s2) * invd;   
    float temp = dot(e2, s2) * invd;

    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f 
        || temp < r->range.x || temp > r->range.y)
    {
        return false;
    }
    else
    {
        r->range.y = temp;
        *a = b1;
        *b = b2;
        return true;
    }
}

bool IntersectTriangleP(ray* r, float3 v1, float3 v2, float3 v3)
{
    float3 e1 = v2 - v1;
    float3 e2 = v3 - v1;
    float3 s1 = cross(r->d, e2);
    float  invd = 1.f / dot(s1, e1);
    float3 d = r->o - v1;
    float  b1 = dot(d, s1) * invd;
    float3 s2 = cross(d, e1);
    float  b2 = dot(r->d, s2) * invd;   
    float temp = dot(e2, s2) * invd;

    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f 
        || temp < r->range.x || temp > r->range.y)
    {
        return false;
    }

    return true;
}

// Intersect ray with the axis-aligned box
bool IntersectBox(ray* r, float3 invdir, bbox box)
{
    float3 f = (box.pmax - r->o) * invdir;
    float3 n = (box.pmin - r->o) * invdir;

    float3 tmax = max(f, n); 
    float3 tmin = min(f, n); 

    float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), r->range.y);
    float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), r->range.x);

    return (t1 >= t0);
}

//  intersect a ray with leaf BVH node
bool IntersectLeafClosest( 
    SceneData* scenedata,
    BvhNode* node,
    ray* r,                      // ray to instersect
    Intersection* isect          // Intersection structure
    )
{
    bool hit = false;
    int thishit = 0;
    float a;
    float b;
    float3 v1, v2, v3, v4;
    Face face;

    for (int i=0; i<4; ++i)
    {
        thishit = false;

        face = scenedata->faces[node->start + i];

        v1 = scenedata->vertices[face.idx[0]];
        v2 = scenedata->vertices[face.idx[1]];
        v3 = scenedata->vertices[face.idx[2]];

        switch (face.cnt)
        {
        case 3:
            {
                // Triangle
                if (IntersectTriangle(r, v1, v2, v3, &a, &b))
                {
                    thishit = 1;
                }

                break;
            }
        case 4:
            {
                // Quad
                v4 = scenedata->vertices[face.idx[3]];
                if (IntersectTriangle(r, v1, v2, v3, &a, &b))
                {
                   thishit = 1;
                }
                else if (IntersectTriangle(r, v3, v4, v1, &a, &b))
                {
                    thishit = 2;
                }

                break;
            }
        }

        if (thishit > 0)
        {
            hit = true;
            isect->uv = make_float2(a,b);
            isect->primid = face.id;
            isect->shapeid = scenedata->shapeids[node->start + i];
            isect->extra = thishit - 1;
            isect->instanceid = -1;
        }

        if (i + 1 >= NUMPRIMS(*node)) return hit;
    }

    return hit;
}

//  intersect a ray with leaf BVH node
bool IntersectLeafAny(
    SceneData* scenedata,
    BvhNode* node,
    ray* r                      // ray to instersect
    )
{
    float3 v1, v2, v3, v4;
    Face face; 

    for (int i = 0; i < 4; ++i)
    {
        face = scenedata->faces[node->start + i];

        v1 = scenedata->vertices[face.idx[0]];
        v2 = scenedata->vertices[face.idx[1]];
        v3 = scenedata->vertices[face.idx[2]];

        switch (face.cnt)
        {
        case 3:
            {
                // Triangle
                if (IntersectTriangleP(r, v1, v2, v3))
                {
                    return true;
                }

                break;
            }
        case 4:
            {
                v4 = scenedata->vertices[face.idx[3]];
                // Quad
                if (IntersectTriangleP(r, v1, v2, v3) || IntersectTriangleP(r, v3, v4, v1))
                {
                    return true;
                }

                break;
            }
        }

        if (i + 1 >= NUMPRIMS(*node)) break;
    }

    return false;
}

// intersect Ray against the whole BVH structure
bool IntersectSceneClosest(SceneData* scenedata,  ray* r, Intersection* isect)
{
    bool hit = false;
    float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d;

    uint idx = 0;
    while (idx != 0xFFFFFFFF)
    {
        // Try intersecting against current node's bounding box.
        // If this is the leaf try to intersect against contained triangle.
        BvhNode node = scenedata->nodes[idx];
        if (IntersectBox(r, invdir, node.bounds))
        {
            if (LEAFNODE(node))
            {
                hit |= IntersectLeafClosest(scenedata, &node, r, isect);
                idx = node.next;
                continue;
            }
            // Traverse child nodes otherwise.
            else
            {
                idx = idx + 1;
                continue;
            }
        }
        else
        {
            idx = node.next;
        }
    };

    return hit;
}

// intersect Ray against the whole BVH structure
bool IntersectSceneAny(SceneData* scenedata,  ray* r)
{
    float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d;

    uint idx = 0;
    while (idx != 0xFFFFFFFF)
    {
        // Try intersecting against current node's bounding box.
        // If this is the leaf try to intersect against contained triangle.
        BvhNode node = scenedata->nodes[idx];
        if (IntersectBox(r, invdir, node.bounds))
        {
            if (LEAFNODE(node))
            {
                if (IntersectLeafAny(scenedata, &node, r))
                {
                    return true;
                }
                else
                {
                    idx = node.next;
                    continue;
                }
            }
            // Traverse child nodes otherwise.
            else
            {
                idx = idx + 1;
                continue;
            }
        }
        else
        {
            idx = node.next;
        }
    };

    return false;
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectClosest(
    // Input
    __global BvhNode* nodes,   // BVH nodes
    __global float3* vertices, // Scene positional data
    __global Face* faces,    // Scene indices
    __global int* shapeids,    // Shape IDs
    __global ray* rays,        // Ray workload
    int offset,                // Offset in rays array
    int numrays,               // Number of rays to process					
    __global int* hitresults,  // Hit results
    __global Intersection* hits // Hit datas
    )
{

    int global_id  = get_global_id(0);

    // Fill scene data 
    SceneData scenedata = 
    {
        nodes,
        vertices,
        faces,
        shapeids
    };

    // Handle only working subset
    if (global_id < numrays)
    {
        // Fetch ray
        int idx = offset + global_id;
        ray r = rays[idx];

        // Calculate closest hit
        Intersection isect;
        int hit = (int)IntersectSceneClosest(&scenedata, &r, &isect);

        // Write data back in case of a hit
        if (hit)
        {
            hitresults[idx] = 1;
            rays[idx] = r;
            hits[idx] = isect;
        }
        else
        {
            hitresults[idx] = 0;
        }
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectAny(
    // Input
    __global BvhNode* nodes,   // BVH nodes
    __global float3* vertices, // Scene positional data
    __global Face* faces,    // Scene indices
    __global int* shapeids,    // Shape IDs
    __global ray* rays,        // Ray workload
    int offset,                // Offset in rays array
    int numrays,               // Number of rays to process					
    __global int* hitresults   // Hit results
    )
{
    int global_id  = get_global_id(0);

    // Fill scene data 
    SceneData scenedata = 
    {
        nodes,
        vertices,
        faces,
        shapeids
    };

    // Handle only working subset
    if (global_id < numrays)
    {
        // Fetch ray
        ray r = rays[offset + global_id];

        // Calculate any intersection
        hitresults[offset + global_id] = (int)IntersectSceneAny(&scenedata, &r);
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectClosestRC(
    // Input
    __global BvhNode* nodes,   // BVH nodes
    __global float3* vertices, // Scene positional data
    __global Face* faces,    // Scene indices
    __global int* shapeids,    // Shape IDs
    __global ray* rays,        // Ray workload
    __global int* rayindices,
    int offset,                // Offset in rays array
    __global int* numrays,               // Number of rays to process					
    __global Intersection* hits // Hit datas
    )
{

    int global_id  = get_global_id(0);

    // Fill scene data 
    SceneData scenedata = 
    {
        nodes,
        vertices,
        faces,
        shapeids
    };

    // Handle only working subset
    if (global_id < *numrays)
    {
        // Fetch ray
        int idx = offset + global_id;
        ray r = rays[idx];

        // Calculate closest hit
        Intersection isect;
        int hit = (int)IntersectSceneClosest(&scenedata, &r, &isect);

        // Write data back in case of a hit
        if (hit)
        {
            hitresults[idx] = 1;
            rays[idx] = r;
            hits[idx] = isect;
        }
        else
        {
            hitresults[idx] = 0;
        }
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectAnyRC(
    // Input
    __global BvhNode* nodes,   // BVH nodes
    __global float3* vertices, // Scene positional data
    __global Face* faces,      // Scene indices
    __global int* shapeids,    // Shape IDs
    __global ray* rays,        // Ray workload
    int offset,                // Offset in rays array
    __global int* numrays,     // Number of rays to process
    __global int* hitresults   // Hit results
    )
{
    int global_id  = get_global_id(0);

    // Fill scene data 
    SceneData scenedata = 
    {
        nodes,
        vertices,
        faces,
        shapeids
    };

    // Handle only working subset
    if (global_id < *numrays)
    {
        // Fetch ray
        ray r = rays[offset + global_id];

        // Calculate any intersection
        hitresults[offset + global_id] = (int)IntersectSceneAny(&scenedata, &r);
    }
}