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
#include <../Resources/kernels/CL/common.cl>
/*************************************************************************
EXTENSIONS
**************************************************************************/

/*************************************************************************
DEFINES
**************************************************************************/
#define PI 3.14159265358979323846f

/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/

typedef struct
{
    int startidx;
    int numprims;
} Voxel;

typedef struct
{
    // Cached bounds
    bbox bounds;
    // Voxel sizes
    float3 voxelsize;
    // Voxel size inverse
    float3 voxelsizeinv;
    // Grid resolution in each dimension
    int4 gridres;
} GridDesc;


typedef struct
{
    // Grid voxels structure
    __global Voxel const* voxels;
    // Scene positional data
    __global float3 const* vertices;
    // Scene indices
    __global Face const* faces;
    // Shape IDs
    __global int const* shapeids;
    // Extra data
    __global int const* indices;
    // Grid desc
    __global GridDesc const* grid;
} SceneData;

/*************************************************************************
HELPER FUNCTIONS
**************************************************************************/

float3 PosToVoxel(__global GridDesc const* grid,  float3 p)
{
    float3 res;
    res.x = (int)clamp(floor((p.x - grid->bounds.pmin.x) * grid->voxelsizeinv.x), 0.f, (float)(grid->gridres.x - 1));
    res.y = (int)clamp(floor((p.y - grid->bounds.pmin.y) * grid->voxelsizeinv.y), 0.f, (float)(grid->gridres.y - 1));
    res.z = (int)clamp(floor((p.z - grid->bounds.pmin.z) * grid->voxelsizeinv.z), 0.f, (float)(grid->gridres.z - 1));
    return res;
}

float3 VoxelToPos(__global GridDesc const* grid, float3 v)
{
    return make_float3(grid->bounds.pmin.x + v.x * grid->voxelsize.x,
        grid->bounds.pmin.y + v.y * grid->voxelsize.y,
        grid->bounds.pmin.z + v.z * grid->voxelsize.z);
}

/*float  VoxelToPos(int x, int axis) const
{
    return bounds_.pmin[axis] + x * voxelsize_[axis];
}*/

int VoxelPlainIndex(__global GridDesc const* grid, float3 v)
{
    return (int)(v.z * grid->gridres.x * grid->gridres.y + v.y * grid->gridres.x + v.x);
}


/*************************************************************************
BVH FUNCTIONS
**************************************************************************/
//  intersect a ray with grid voxel
bool IntersectVoxelClosest(
    SceneData const* scenedata,
    Voxel const* voxel,
    ray const* r,                // ray to instersect
    Intersection* isect          // Intersection structure
    )
{
    float3 v1, v2, v3;
    Face face;
    bool hit = false;

    for (int i = voxel->startidx; i < voxel->startidx + voxel->numprims; ++i)
    {
        face = scenedata->faces[scenedata->indices[i]];

        v1 = scenedata->vertices[face.idx[0]];
        v2 = scenedata->vertices[face.idx[1]];
        v3 = scenedata->vertices[face.idx[2]];

        if (IntersectTriangle(r, v1, v2, v3, isect))
        {
            isect->primid = face.id;
            isect->shapeid = scenedata->shapeids[scenedata->indices[i]];
            isect->uvwt.z = 0;
            hit = true;
        }
        else if (face.cnt == 4)
        {
            v2 = scenedata->vertices[face.idx[3]];
            if (IntersectTriangle(r, v3, v2, v1, isect))
            {
                isect->primid = face.id;
                isect->shapeid = scenedata->shapeids[scenedata->indices[i]];
                isect->uvwt.z = 1;
                hit = true;
            }
        }

    }

    return hit;
}

//  intersect a ray with leaf BVH node
bool IntersectVoxelAny(
    SceneData const* scenedata,
    Voxel const* voxel,
    ray const* r                      // ray to instersect
    )
{
    float3 v1, v2, v3;
    Face face;

    for (int i = voxel->startidx; i < voxel->startidx + voxel->numprims; ++i)
    {
        face = scenedata->faces[scenedata->indices[i]];

        v1 = scenedata->vertices[face.idx[0]];
        v2 = scenedata->vertices[face.idx[1]];
        v3 = scenedata->vertices[face.idx[2]];

        if (IntersectTriangleP(r, v1, v2, v3))
        {
            return true;
        }
        else if (face.cnt == 4)
        {
            v2 = scenedata->vertices[face.idx[3]];
            if (IntersectTriangleP(r, v3, v2, v1))
            {
                return true;
            }
        }

    }

    return false;
}

// intersect Ray against the whole grid structure
bool IntersectSceneClosest(SceneData const* scenedata, ray const* r, Intersection* isect)
{
    const float3 invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz;
    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w);
    isect->shapeid = -1;
    isect->primid = -1;

    // Check the ray vs scene bounds
    float t = IntersectBoxF(r, invdir, scenedata->grid->bounds, r->o.w);

    // If no bail out
    if (t < 0.f)
    {
        //printf("Bailing out at initial bbox test % d\n", get_global_id(0));
        return false;
    }

    // Find starting point
    float3 isectpoint = r->o.xyz + r->d.xyz * t;
    float3 gridres = make_float3(scenedata->grid->gridres.x, scenedata->grid->gridres.y, scenedata->grid->gridres.z);

    // Perpare the stepping data
    float3 nexthit;
    float3 dt;
    float3 out;

    //for (int axis = 0; axis < 3; ++axis)
    //{
    // Voxel position
    float3 voxelidx = PosToVoxel(scenedata->grid, isectpoint);

    //assert(voxel[0] < gridres_[0] && voxel[1] < gridres_[1] && voxel[2] < gridres_[2]);

    float3 step = r->d.xyz >= 0.f ? 1.f : -1.f;
    float3 step01 = (step + 1.f) / 2.f;

    // Next hit along this axis in parametric units
    nexthit = t + (VoxelToPos(scenedata->grid, voxelidx + step01) - isectpoint) * invdir;

    // Delta T
    dt = scenedata->grid->voxelsize * invdir * step;
    dt = isinf(dt) ? 0.f : dt;
    //printf("Dt %f %f %f\n", dt.x, dt.y, dt.z);

    // Last voxel
    out = step > 0.f ? gridres : -1.f;

    bool hit = false;
    while ((int)voxelidx.x != (int)out.x && 
        (int)voxelidx.y != (int)out.y && 
        (int)voxelidx.z != (int)out.z)
    {
        // 
        //printf("Testing voxel %d %d %d -----> ", (int)voxelidx.x, (int)voxelidx.y, (int)voxelidx.z);

        // Check current voxel
        int idx = VoxelPlainIndex(scenedata->grid, voxelidx);

        Voxel voxel = scenedata->voxels[idx];

        if (voxel.startidx != -1)
        {
            hit = IntersectVoxelClosest(scenedata, &voxel, r, isect);
            //printf(" %d\n", hit);
        }

        // Advance to next one
        int bits = (((nexthit.x < nexthit.y) ? 1 : 0) << 2) +
            (((nexthit.x < nexthit.z) ? 1 : 0) << 1) +
            (((nexthit.y < nexthit.z) ? 1 : 0));

        const float3 cmptoaxis[8] = { 
            { 0.f, 0.f, 1.f },
            { 0.f, 1.f, 0.f },
            { 0.f, 0.f, 1.f },
            { 0.f, 1.f, 0.f },
            { 0.f, 0.f, 1.f },
            { 0.f, 0.f, 1.f },
            { 1.f, 0.f, 0.f },
            { 1.f, 0.f, 0.f }
        };

        // Choose axis
        float3 stepaxis = cmptoaxis[bits];
        float3 nexthittest = isinf(nexthit) ? 0.f : nexthit;

       // printf("Next hit %f %f %f\n", nexthit.x, nexthit.y, nexthit.z);
        //printf("Step axis %f %f %f\n", stepaxis.x, stepaxis.y, stepaxis.z);



        // Early out
        if (isect->uvwt.w < dot(nexthittest, stepaxis))
        {
            //printf("Bailing out at early out step % d\n", get_global_id(0));
            break;
        }

        // Advance current position
        voxelidx += step * stepaxis;

        // Advance next hit
        nexthit += dt * stepaxis;
    }

    return hit;
}

// intersect Ray against the whole grid structure
bool IntersectSceneAny(SceneData const* scenedata, ray const* r)
{
    const float3 invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz;

    // Check the ray vs scene bounds
    float t = IntersectBoxF(r, invdir, scenedata->grid->bounds, r->o.w);

    // If no bail out
    if (t < 0.f)
    {
        //printf("Bailing out at initial bbox test % d\n", get_global_id(0));
        return false;
    }

    // Find starting point
    float3 isectpoint = r->o.xyz + r->d.xyz * t;
    float3 gridres = make_float3(scenedata->grid->gridres.x, scenedata->grid->gridres.y, scenedata->grid->gridres.z);

    // Perpare the stepping data
    float3 nexthit;
    float3 dt;
    float3 out;

    //for (int axis = 0; axis < 3; ++axis)
    //{
    // Voxel position
    float3 voxelidx = PosToVoxel(scenedata->grid, isectpoint);

    //assert(voxel[0] < gridres_[0] && voxel[1] < gridres_[1] && voxel[2] < gridres_[2]);

    float3 step = r->d.xyz >= 0.f ? 1.f : -1.f;
    float3 step01 = (step + 1.f) / 2.f;

    // Next hit along this axis in parametric units
    nexthit = t + (VoxelToPos(scenedata->grid, voxelidx + step01) - isectpoint) * invdir;

    // Delta T
    dt = scenedata->grid->voxelsize * invdir * step;
    dt = isinf(dt) ? 0.f : dt;
    //printf("Dt %f %f %f\n", dt.x, dt.y, dt.z);

    // Last voxel
    out = step > 0.f ? gridres : -1.f;

    while ((int)voxelidx.x != (int)out.x &&
        (int)voxelidx.y != (int)out.y &&
        (int)voxelidx.z != (int)out.z)
    {
        // 
        //printf("Testing voxel %d %d %d -----> ", (int)voxelidx.x, (int)voxelidx.y, (int)voxelidx.z);

        // Check current voxel
        int idx = VoxelPlainIndex(scenedata->grid, voxelidx);

        Voxel voxel = scenedata->voxels[idx];

        if (voxel.startidx != -1)
        {
            if (IntersectVoxelAny(scenedata, &voxel, r))
                return true;
        }

        // Advance to next one
        int bits = (((nexthit.x < nexthit.y) ? 1 : 0) << 2) +
            (((nexthit.x < nexthit.z) ? 1 : 0) << 1) +
            (((nexthit.y < nexthit.z) ? 1 : 0));

        const float3 cmptoaxis[8] = {
            { 0.f, 0.f, 1.f },
            { 0.f, 1.f, 0.f },
            { 0.f, 0.f, 1.f },
            { 0.f, 1.f, 0.f },
            { 0.f, 0.f, 1.f },
            { 0.f, 0.f, 1.f },
            { 1.f, 0.f, 0.f },
            { 1.f, 0.f, 0.f }
        };

        // Choose axis
        float3 stepaxis = cmptoaxis[bits];

        // printf("Next hit %f %f %f\n", nexthit.x, nexthit.y, nexthit.z);
        //printf("Step axis %f %f %f\n", stepaxis.x, stepaxis.y, stepaxis.z);

        // Advance current position
        voxelidx += step * stepaxis;

        // Advance next hit
        nexthit += dt * stepaxis;
    }

    return false;
}


__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectClosest(
// Input
__global Voxel const* voxels,   // Grid voxels
__global GridDesc const* griddesc, // Grid description
__global float3 const* vertices, // Scene positional data
__global Face const* faces,    // Scene indices
__global int const* shapeids,    // Shape IDs
__global int const* indices,    // Grid indices
__global ray const* rays,        // Ray workload
int offset,                // Offset in rays array
int numrays,               // Number of rays to process
__global Intersection* hits // Hit datas
)
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);

    // Fill scene data 
    SceneData scenedata =
    {
        voxels,
        vertices,
        faces,
        shapeids,
        indices,
        griddesc
    };

    Intersection isect;
    if (global_id < numrays)
    {
        // Fetch ray
        ray r = rays[global_id + offset];

        // Calculate closest hit
        IntersectSceneClosest(&scenedata, &r, &isect);

        // Write data back in case of a hit
        hits[global_id + offset] = isect;
    }
}


__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectAny(
// Input
__global Voxel const* voxels,   // Grid voxels
__global GridDesc const* griddesc, // Grid description
__global float3 const* vertices, // Scene positional data
__global Face const* faces,    // Scene indices
__global int const* shapeids,    // Shape IDs
__global int const* indices,    // Grid indices
__global ray const* rays,        // Ray workload
int offset,                // Offset in rays array
int numrays,               // Number of rays to process
__global int* hitresults  // Hit results
)
{
    int global_id = get_global_id(0);
    int local_id = get_local_id(0);

    // Fill scene data 
    SceneData scenedata =
    {
        voxels,
        vertices,
        faces,
        shapeids,
        indices,
        griddesc
    };

    if (global_id < numrays)
    {
        // Fetch ray
        ray r = rays[global_id];

        // Calculate any intersection
        hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1;
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectClosestRC(
// Input
__global Voxel const* voxels,   // Grid voxels
__global GridDesc const* griddesc, // Grid description
__global float3 const* vertices, // Scene positional data
__global Face const* faces,    // Scene indices
__global int const* shapeids,    // Shape IDs
__global int const* indices,    // Grid indices
__global ray const* rays,        // Ray workload
int offset,                // Offset in rays array
__global int const* numrays,  // Number of rays to process
__global Intersection* hits // Hit datas
)
{
    int global_id = get_global_id(0);

    // Fill scene data 
    SceneData scenedata =
    {
        voxels,
        vertices,
        faces,
        shapeids,
        indices,
        griddesc
    };

    // Handle only working subset
    if (global_id < *numrays)
    {
        // Fetch ray
        ray r = rays[global_id];

        // Calculate closest hit
        Intersection isect;
        IntersectSceneClosest(&scenedata, &r, &isect);

        // Write data back in case of a hit
        hits[global_id] = isect;
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectAnyRC(
// Input
__global Voxel const* voxels,   // Grid voxels
__global GridDesc const* griddesc, // Grid description
__global float3 const* vertices, // Scene positional data
__global Face const* faces,    // Scene indices
__global int const* shapeids,    // Shape IDs
__global int const* indices,    // Grid indices
__global ray const* rays,        // Ray workload
int offset,                // Offset in rays array
__global int const* numrays,  // Number of rays to process
__global int* hitresults // Hit datas
)
{
    int global_id = get_global_id(0);

    // Fill scene data 
    SceneData scenedata =
    {
        voxels,
        vertices,
        faces,
        shapeids,
        indices,
        griddesc
    };

    // Handle only working subset
    if (global_id < *numrays)
    {
        // Fetch ray
        ray r = rays[global_id];

        // Calculate any intersection
        hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1;
    }
}