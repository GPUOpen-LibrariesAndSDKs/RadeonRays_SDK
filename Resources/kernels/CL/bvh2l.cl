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
#include <kernels/CL/common.cl>

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
#define SHAPEIDX(x)     (((int)(x.pmin.w)))
#define LEAFNODE(x)     (((x).pmin.w) != -1.f)

typedef struct
{
    // BVH structure
    __global BvhNode*       nodes;
    // Scene positional data
    __global float3*        vertices;
    // Scene indices
    __global Face*          faces;
    // Transforms
    __global ShapeData*     shapes;
    // Root BVH idx
    int rootidx;
} SceneData;


/*************************************************************************
BVH FUNCTIONS
**************************************************************************/
//  intersect a ray with leaf BVH node
void IntersectLeafClosest(
    SceneData const* scenedata,
    BvhNode const* node,
    ray const* r,                // ray to instersect
    int const shapeid,
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

    if (IntersectTriangle(r, v1, v2, v3, isect))
    {
        isect->primid = face.id;
        isect->shapeid = shapeid;
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

	if (IntersectTriangleP(r, v1, v2, v3))
	{
		return true;
	}

    return false;
}


// intersect Ray against the whole BVH2L structure
bool IntersectSceneClosest2L(SceneData* scenedata, ray* r, Intersection* isect)
{
    // Init intersection
    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w);
    isect->shapeid = -1;
    isect->primid = -1;

    // Precompute invdir for bbox testing
    float3 invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz;
    float3 invdirtop = make_float3(1.f, 1.f, 1.f) / r->d.xyz;
    // We need to keep original ray around for returns from bottom hierarchy
    ray topray = *r;

    // Fetch top level BVH index
    int idx = scenedata->rootidx;
    // -1 indicates we are traversing top level
    int topidx = -1;
    // Current shape id
    int shapeid = -1;
    while (idx != -1)
    {
        // Try intersecting against current node's bounding box.
        BvhNode node = scenedata->nodes[idx];
        if (IntersectBox(r, invdir, node, isect->uvwt.w))
        {
            if (LEAFNODE(node))
            {
                // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy)
                // or containing another BVH (top level hierarhcy)
                if (topidx != -1)
                {
                    // This is bottom level, so intersect with a primitives
                    IntersectLeafClosest(scenedata, &node, r, shapeid, isect);

                    // And goto next node
                    idx = (int)(node.pmax.w);
                }
                else
                {
                    // This is top level hierarchy leaf
                    // Save top node index for return
                    topidx = idx;
                    // Get shape descrition struct index
                    int shapeidx = SHAPEIDX(node);
					// Get shape mask
					int shapemask = scenedata->shapes[shapeidx].mask;
					// Drill into 2nd level BVH only if the geometry is not masked vs current ray
					// otherwise skip the subtree
					if (Ray_GetMask(r) && shapemask)
					{
						// Fetch bottom level BVH index
						idx = scenedata->shapes[shapeidx].bvhidx;
						shapeid = scenedata->shapes[shapeidx].id;

						// Fetch BVH transform
						float4 wmi0 = scenedata->shapes[shapeidx].m0;
						float4 wmi1 = scenedata->shapes[shapeidx].m1;
						float4 wmi2 = scenedata->shapes[shapeidx].m2;
						float4 wmi3 = scenedata->shapes[shapeidx].m3;

						// Apply linear motion blur (world coordinates)
						//float4 lmv = scenedata->shapes[shapeidx].linearvelocity;
						//float4 amv = scenedata->shapes[SHAPEDATAIDX(node)].angularvelocity;
						//r->o.xyz -= (lmv.xyz*r->d.w);
						// Transfrom the ray
						*r = transform_ray(*r, wmi0, wmi1, wmi2, wmi3);
						// rotate_ray(r, amv);
						// Recalc invdir
						invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz;
						// And continue traversal of the bottom level BVH
						continue;
					}
					else
					{
						idx = -1;
					}
                }
            }
            // Traverse child nodes otherwise.
            else
            {
                // This is an internal node, proceed to left child (it is at current + 1 index)
                idx = idx + 1;
            }
        }
        else
        {
            // We missed the node, goto next one
            idx = (int)(node.pmax.w);
        }

        // Here check if we ended up traversing bottom level BVH
        // in this case idx = -1 and topidx has valid value
        if (idx == -1 && topidx != -1)
        {
            //  Proceed to next top level node
            idx = (int)(scenedata->nodes[topidx].pmax.w);
            // Set topidx
            topidx = -1;
            // Restore ray here
            r->o = topray.o;
            r->d = topray.d;
            // Restore invdir
            invdir = invdirtop;
        }
    }

    return isect->shapeid >= 0;
}

// intersect Ray against the whole BVH2L structure
bool IntersectSceneAny2L(SceneData* scenedata, ray* r)
{
    // Precompute invdir for bbox testing
    float3 invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz;
    float3 invdirtop = make_float3(1.f, 1.f, 1.f) / r->d.xyz;
    // We need to keep original ray around for returns from bottom hierarchy
    ray topray = *r;

    // Fetch top level BVH index
    int idx = scenedata->rootidx;
    // -1 indicates we are traversing top level
    int topidx = -1;
    while (idx != -1)
    {
        // Try intersecting against current node's bounding box.
        BvhNode node = scenedata->nodes[idx];
        if (IntersectBox(r, invdir, node, r->o.w))
        {
            if (LEAFNODE(node))
            {
                // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy)
                // or containing another BVH (top level hierarhcy)
                if (topidx != -1)
                {
                    // This is bottom level, so intersect with a primitives
                    if (IntersectLeafAny(scenedata, &node, r))
                        return true;
                    // And goto next node
                    idx = (int)(node.pmax.w);
                }
                else
                {
                    // This is top level hierarchy leaf
                    // Save top node index for return
                    topidx = idx;
                    // Get shape descrition struct index
                    int shapeidx = SHAPEIDX(node);
					// Get shape mask
					int shapemask = scenedata->shapes[shapeidx].mask;
					// Drill into 2nd level BVH only if the geometry is not masked vs current ray
					// otherwise skip the subtree
					if (Ray_GetMask(r) && shapemask)
					{
						// Fetch bottom level BVH index
						idx = scenedata->shapes[shapeidx].bvhidx;

						// Fetch BVH transform
						float4 wmi0 = scenedata->shapes[shapeidx].m0;
						float4 wmi1 = scenedata->shapes[shapeidx].m1;
						float4 wmi2 = scenedata->shapes[shapeidx].m2;
						float4 wmi3 = scenedata->shapes[shapeidx].m3;

						// Apply linear motion blur (world coordinates)
						//float4 lmv = scenedata->shapes[shapeidx].linearvelocity;
						//float4 amv = scenedata->shapes[SHAPEDATAIDX(node)].angularvelocity;
						//r->o.xyz -= (lmv.xyz*r->d.w);
						// Transfrom the ray
						*r = transform_ray(*r, wmi0, wmi1, wmi2, wmi3);
						//rotate_ray(r, amv);
						// Recalc invdir
						invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz;
						// And continue traversal of the bottom level BVH
						continue;
					}
					else
					{
						// Skip the subtree
						idx = -1;
					}
                }
            }
            // Traverse child nodes otherwise.
            else
            {
                // This is an internal node, proceed to left child (it is at current + 1 index)
                idx = idx + 1;
            }
        }
        else
        {
            // We missed the node, goto next one
            idx = (int)(node.pmax.w);
        }

        // Here check if we ended up traversing bottom level BVH
        // in this case idx = 0xFFFFFFFF and topidx has valid value
        if (idx == -1 && topidx != -1)
        {
            //  Proceed to next top level node
            idx = (int)(scenedata->nodes[topidx].pmax.w);
            // Set topidx
            topidx = -1;
            // Restore ray here
            *r = topray;
            // Restore invdir
            invdir = invdirtop;
        }
    }

    return false;
}


// 2 level variants
__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectClosest2L(
// Input
__global BvhNode* nodes,   // BVH nodes
__global float3* vertices, // Scene positional data
__global Face* faces,    // Scene indices
__global ShapeData* shapes, // Transforms
int rootidx,               // BVH root idx
__global ray* rays,        // Ray workload
int offset,                // Offset in rays array
int numrays,               // Number of rays to process
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
        rootidx
    };

    // Handle only working subset
    if (global_id < numrays)
    {
        // Fetch ray
        int idx = offset + global_id;
        ray r = rays[idx];

		if (Ray_IsActive(&r))
		{
			// Calculate closest hit
			Intersection isect;
			IntersectSceneClosest2L(&scenedata, &r, &isect);

			// Write data back in case of a hit
			hits[idx] = isect;
		}
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectAny2L(
// Input
__global BvhNode* nodes,   // BVH nodes
__global float3* vertices, // Scene positional data
__global Face* faces,    // Scene indices
__global ShapeData* shapes, // Transforms
int rootidx,               // BVH root idx
__global ray* rays,        // Ray workload
int offset,                // Offset in rays array
int numrays,               // Number of rays to process
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
        rootidx
    };

    // Handle only working subset
    if (global_id < numrays)
    {
        // Fetch ray
        ray r = rays[offset + global_id];

		if (Ray_IsActive(&r))
		{
			// Calculate any intersection
			hitresults[offset + global_id] = IntersectSceneAny2L(&scenedata, &r) ? 1 : -1;
		}
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectClosestRC2L(
// Input
__global BvhNode* nodes,   // BVH nodes
__global float3* vertices, // Scene positional data
__global Face* faces,    // Scene indices
__global ShapeData* shapes, // Transforms
int rootidx,               // BVH root idx
__global ray* rays,        // Ray workload
__global int* numrays,     // Number of rays in the workload
int offset,                // Offset in rays array
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
        rootidx
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
			IntersectSceneClosest2L(&scenedata, &r, &isect);

			hits[idx] = isect;
		}
    }
}

__attribute__((reqd_work_group_size(64, 1, 1)))
// Version with range check
__kernel void IntersectAnyRC2L(
// Input
__global BvhNode* nodes,   // BVH nodes
__global float3* vertices, // Scene positional data
__global Face* faces,    // Scene indices
__global ShapeData* shapes, // Transforms
int rootidx,               // BVH root idx
__global ray* rays,        // Ray workload
__global int* numrays,     // Number of rays in the workload
int offset,                // Offset in rays array
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
        rootidx
    };

    // Handle only working subset
    if (global_id < *numrays)
    {
        // Fetch ray
        ray r = rays[offset + global_id];

		if (Ray_IsActive(&r))
		{
			// Calculate any intersection
			hitresults[offset + global_id] = IntersectSceneAny2L(&scenedata, &r) ? 1 : -1;
		}
    }
}