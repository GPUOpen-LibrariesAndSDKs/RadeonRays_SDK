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
#define STARTIDX(x)     (((int)(x->pmin.w)))
#define LEAFNODE(x)     (((x).pmin.w) != -1.f)

typedef struct 
{
    // BVH structure
    __global BvhNode const*       nodes;
    // Scene positional data
    __global float3 const*        vertices;
    // Scene indices
    __global Face const*          faces;
    // Shape data
    __global ShapeData const*     shapes;
    // Extra data
    __global int const*           extra;
} SceneData;

/*************************************************************************
HELPER FUNCTIONS
**************************************************************************/



/*************************************************************************
BVH FUNCTIONS
**************************************************************************/
//  intersect a ray with leaf BVH node
void IntersectLeafClosest(
    SceneData const* scenedata,
    BvhNode const* node,
    ray const* r,                // ray to instersect
    Intersection* isect          // Intersection structure
    )
{
    float3 v1, v2, v3;
    Face face;

    int start = STARTIDX(node);
    face = scenedata->faces[start];
    v1 = scenedata->vertices[face.idx[0]];
    v2 = scenedata->vertices[face.idx[1]];
    v3 = scenedata->vertices[face.idx[2]];

    int shapemask = scenedata->shapes[face.shapeidx].mask;

    if (Ray_GetMask(r) & shapemask)
    {
        if (IntersectTriangle(r, v1, v2, v3, isect))
        {
            isect->primid = face.id;
            isect->shapeid = scenedata->shapes[face.shapeidx].id;
        }
    }
}

//  intersect a ray with leaf BVH node
bool IntersectLeafAny(
    SceneData const* scenedata,
    BvhNode const* node,
    ray const* r                      // ray to instersect
    )
{
    float3 v1, v2, v3;
    Face face;

    int start = STARTIDX(node);
    face = scenedata->faces[start];
    v1 = scenedata->vertices[face.idx[0]];
    v2 = scenedata->vertices[face.idx[1]];
    v3 = scenedata->vertices[face.idx[2]];

	int shapemask = scenedata->shapes[face.shapeidx].mask;

	if (Ray_GetMask(r) & shapemask)
	{
		if (IntersectTriangleP(r, v1, v2, v3))
		{
			return true;
		}
	}

    return false;
}


// intersect Ray against the whole BVH structure
void IntersectSceneClosest(SceneData const* scenedata,  ray const* r, Intersection* isect)
{
    const float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d.xyz;

    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w);
    isect->shapeid = -1;
    isect->primid = -1;

    int idx = 0;

    while (idx != -1)
    {
        // Try intersecting against current node's bounding box.
        // If this is the leaf try to intersect against contained triangle.
        BvhNode node = scenedata->nodes[idx];
        if (IntersectBox(r, invdir, node, isect->uvwt.w))
        {
            if (LEAFNODE(node))
            {
                IntersectLeafClosest(scenedata, &node, r, isect);
                idx = (int)(node.pmax.w);
            }
            // Traverse child nodes otherwise.
            else
            {
                ++idx;
            }
        }
        else
        {
            idx = (int)(node.pmax.w);
        }
    };
}



// intersect Ray against the whole BVH structure
bool IntersectSceneAny(SceneData const* scenedata,  ray const* r)
{
    float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d.xyz;

    int idx = 0;
    while (idx != -1)
    {
        // Try intersecting against current node's bounding box.
        // If this is the leaf try to intersect against contained triangle.
        BvhNode node = scenedata->nodes[idx];
        if (IntersectBox(r, invdir, node, r->o.w))
        {
            if (LEAFNODE(node))
            {
                if (IntersectLeafAny(scenedata, &node, r))
                {
                    return true;
                }
                else
                {
                    idx = (int)(node.pmax.w);
                }
            }
            // Traverse child nodes otherwise.
            else
            {
                ++idx;
            }
        }
        else
        {
            idx = (int)(node.pmax.w);
        }
    };

    return false;
}


__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectClosestAMD(
// Input
__global BvhNode const* nodes,   // BVH nodes
__global float3 const* vertices, // Scene positional data
__global Face const* faces,    // Scene indices
__global ShapeData const* shapes,     // Shapes
__global ray const* rays,        // Ray workload
int offset,                // Offset in rays array
int numrays,               // Number of rays to process
__global Intersection* hits, // Hit datas
__global int*          raycnt 
    )
{
    __local int nextrayidx;

    int global_id  = get_global_id(0);
    int local_id  = get_local_id(0);

    // Fill scene data 
    SceneData scenedata =
    {
        nodes,
        vertices,
        faces,
        shapes,
        0
    };

    if (local_id == 0)
    {
        nextrayidx = 0;
    }

    int ridx = 0;
    Intersection isect;

    while (ridx < numrays)
    {
        if (local_id == 0)
        {
            nextrayidx = atomic_add(raycnt, 64);
        }

        ridx = nextrayidx + local_id;

        if (ridx >= numrays)
            break;

        // Fetch ray
        ray r = rays[ridx];

        if (Ray_IsActive(&r))
        {
        	// Calculate closest hit
        	IntersectSceneClosest(&scenedata, &r, &isect);

        	// Write data back in case of a hit
        	hits[ridx] = isect;
        }
    }
}


__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectAnyAMD(
    // Input
    __global BvhNode const* nodes,   // BVH nodes
    __global float3 const* vertices, // Scene positional data
    __global Face const* faces,    // Scene indices
	__global ShapeData const* shapes,     // Shapes
    __global ray const* rays,        // Ray workload
    int offset,                // Offset in rays array
    int numrays,               // Number of rays to process					
    __global int* hitresults,  // Hit results
    __global int* raycnt
    )
{
    __local int nextrayidx;

    int global_id = get_global_id(0);
    int local_id = get_local_id(0);

    // Fill scene data 
    SceneData scenedata =
    {
        nodes,
        vertices,
        faces,
        shapes,
        0
    };

    if (local_id == 0)
    {
        nextrayidx = 0;
    }

    int ridx = 0;
    while (ridx < numrays)
    {
        if (local_id == 0)
        {
            nextrayidx = atomic_add(raycnt, 64);
        }

        ridx = nextrayidx + local_id;

        if (ridx >= numrays)
            break;

        // Fetch ray
        ray r = rays[ridx];

		if (Ray_IsActive(&r))
		{
			// Calculate any intersection
			hitresults[ridx] = IntersectSceneAny(&scenedata, &r) ? 1 : -1;
		}
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectClosestRCAMD(
    __global BvhNode const* nodes,   // BVH nodes
    __global float3 const* vertices, // Scene positional data
    __global Face const* faces,      // Scene indices
	__global ShapeData const* shapes,     // Shapes
    __global ray const* rays,        // Ray workload
    int offset,                // Offset in rays array
    __global int const* numrays,     // Number of rays in the workload
    __global Intersection* hits, // Hit datas
    __global int* raycnt
    )
{
    __local int nextrayidx;

    int global_id = get_global_id(0);
    int local_id = get_local_id(0);

    // Fill scene data 
    SceneData scenedata =
    {
        nodes,
        vertices,
        faces,
        shapes,
        0
    };

    if (local_id == 0)
    {
        nextrayidx = 0;
    }

    int ridx = 0;
    Intersection isect;

    while (ridx < *numrays)
    {
        if (local_id == 0)
        {
            nextrayidx = atomic_add(raycnt, 64);
        }

        ridx = nextrayidx + local_id;

        if (ridx >= *numrays)
            break;

        // Fetch ray
		ray r = rays[ridx];

		if (Ray_IsActive(&r))
		{
			// Calculate closest hit
			IntersectSceneClosest(&scenedata, &r, &isect);
			// Write data back in case of a hit
			hits[ridx] = isect;
		}
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectAnyRCAMD(
    // Input
    __global BvhNode const* nodes,   // BVH nodes
    __global float3 const* vertices, // Scene positional data
    __global Face const* faces,    // Scene indices
	__global ShapeData const* shapes,     // Shapes
    __global ray const* rays,        // Ray workload
    int offset,                // Offset in rays array
    __global int const* numrays,     // Number of rays in the workload
    __global int* hitresults,   // Hit results
    __global int* raycnt
    )
{
    __local int nextrayidx;

    int global_id = get_global_id(0);
    int local_id = get_local_id(0);

    // Fill scene data 
    SceneData scenedata =
    {
        nodes,
        vertices,
        faces,
        shapes,
        0
    };

    if (local_id == 0)
    {
        nextrayidx = 0;
    }

    int ridx = 0;
    while (ridx < *numrays)
    {
        if (local_id == 0)
        {
            nextrayidx = atomic_add(raycnt, 64);
        }

        ridx = nextrayidx + local_id;

        if (ridx >= *numrays)
            break;

        // Fetch ray
        ray r = rays[ridx];

		if (Ray_IsActive(&r))
		{
			// Calculate any intersection
			hitresults[ridx] = IntersectSceneAny(&scenedata, &r) ? 1 : -1;
		}
    }
}


__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectClosest(
// Input
__global BvhNode const* nodes,   // BVH nodes
__global float3 const* vertices, // Scene positional data
__global Face const* faces,    // Scene indices
__global ShapeData const* shapes,     // Shapes
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
			IntersectSceneClosest(&scenedata, &r, &isect);

			// Write data back in case of a hit
			hits[global_id] = isect;
		}
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectAny(
// Input
__global BvhNode const* nodes,   // BVH nodes
__global float3 const* vertices, // Scene positional data
__global Face const* faces,    // Scene indices
__global ShapeData const* shapes,     // Shapes
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
        nodes,
        vertices,
        faces,
        shapes,
        0
    };

    // Handle only working subset
    if (global_id < numrays)
    {
        // Fetch ray
        ray r = rays[offset + global_id];

		if (Ray_IsActive(&r))
		{
			// Calculate any intersection
			hitresults[offset + global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1;
		}
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectClosestRC(
__global BvhNode const* nodes,   // BVH nodes
__global float3 const* vertices, // Scene positional data
__global Face const* faces,      // Scene indices
__global ShapeData const* shapes,     // Shapes
__global ray const* rays,        // Ray workload
int offset,                // Offset in rays array
__global int const* numrays,     // Number of rays in the workload
__global Intersection* hits // Hit datas
)
{
    int global_id = get_global_id(0);

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
        int idx = offset + global_id;
        ray r = rays[idx];

		if (Ray_IsActive(&r))
		{
			// Calculate closest hit
			Intersection isect;
			IntersectSceneClosest(&scenedata, &r, &isect);
			// Write data back in case of a hit
			hits[idx] = isect;
		}
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectAnyRC(
// Input
__global BvhNode const* nodes,   // BVH nodes
__global float3 const* vertices, // Scene positional data
__global Face const* faces,    // Scene indices
__global ShapeData const* shapes,     // Shapes
__global ray const* rays,        // Ray workload
int offset,                // Offset in rays array
__global int const* numrays,     // Number of rays in the workload
__global int* hitresults   // Hit results
)
{
    int global_id = get_global_id(0);

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
        ray r = rays[offset + global_id];

		if (Ray_IsActive(&r))
		{
			// Calculate any intersection
			hitresults[offset + global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1;
		}
    }
}