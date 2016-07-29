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
#include <../RadeonRays/src/kernel/CL/common.cl>
  /*************************************************************************
   EXTENSIONS
   **************************************************************************/



   /*************************************************************************
	TYPE DEFINITIONS
	**************************************************************************/
#define STARTIDX(x)     (((int)((x).left)))
#define LEAFNODE(x)     (((x).left) == ((x).right))
#define STACK_SIZE 64
#define SHORT_STACK_SIZE 16


typedef struct
{
	int parent;
	int left;
	int right;
	int next;
} HlbvhNode;

typedef struct
{
	// BVH structure
	__global HlbvhNode const* nodes;
	// Scene bounds
	__global bbox const* bounds;
	// Scene positional data
	__global float3 const* vertices;
	// Scene indices
	__global Face const* faces;
	// Shape IDs
	__global ShapeData const* shapes;
	// Extra data
	__global int const* extra;
} SceneData;

/*************************************************************************
 HELPER FUNCTIONS
 **************************************************************************/



 /*************************************************************************
  BVH FUNCTIONS
  **************************************************************************/
  //  intersect a ray with leaf BVH node
bool IntersectLeafClosest(
	SceneData const* scenedata,
	int faceidx,
	ray const* r,                // ray to instersect
	Intersection* isect          // Intersection structure
	)
{
	float3 v1, v2, v3;
	Face face;

	face = scenedata->faces[faceidx];
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
			return true;
		}
	}

	return false;
}

//  intersect a ray with leaf BVH node
bool IntersectLeafAny(
	SceneData const* scenedata,
	int faceidx,
	ray const* r                      // ray to instersect
	)
{
	float3 v1, v2, v3;
	Face face;

	face = scenedata->faces[faceidx];
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

#define LDS_BUG

#ifdef LDS_BUG
// intersect Ray against the whole BVH structure
bool IntersectSceneClosest(SceneData const* scenedata, ray const* r, Intersection* isect)
{
	const float3 invdir = native_recip(r->d.xyz);

	isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w);
	isect->shapeid = -1;
	isect->primid = -1;

	int stack[STACK_SIZE];
	int* ptr = stack;

	*ptr++ = -1;

	int idx = 0;

	HlbvhNode node;
	bbox lbox;
	bbox rbox;

	float lefthit = 0.f;
	float righthit = 0.f;

	while (idx > -1)
	{
		node = scenedata->nodes[idx];

		if (LEAFNODE(node))
		{
			IntersectLeafClosest(scenedata, STARTIDX(node), r, isect);
		}
		else
		{
			lbox = scenedata->bounds[node.left];
			rbox = scenedata->bounds[node.right];

			lefthit = IntersectBoxF(r, invdir, lbox, isect->uvwt.w);
			righthit = IntersectBoxF(r, invdir, rbox, isect->uvwt.w);

			if (lefthit > 0.f && righthit > 0.f)
			{
				int deferred = -1;
				if (lefthit > righthit)
				{
					idx = node.right;
					deferred = node.left;
				}
				else
				{
					idx = node.left;
					deferred = node.right;
				}

				*ptr++ = deferred;
				continue;
			}
			else if (lefthit > 0)
			{
				idx = node.left;
				continue;
			}
			else if (righthit > 0)
			{
				idx = node.right;
				continue;
			}
		}

		idx = *--ptr;
	}


	return isect->shapeid >= 0;
}



// intersect Ray against the whole BVH structure
bool IntersectSceneAny(SceneData const* scenedata, ray const* r)
{
	//return false;

	const float3 invdir = native_recip(r->d.xyz);

	//if (get_global_id(0) == 0)
	//{

	//}

	int stack[STACK_SIZE];
	int* ptr = stack;

	*ptr++ = -1;

	int idx = 0;

	HlbvhNode node;
	bbox lbox;
	bbox rbox;

	float lefthit = 0.f;
	float righthit = 0.f;
	bool hit = false;

	int step = 0;
	//if (get_global_id(0) == 1)
		//printf("Starting %d\n", get_global_id(0) );
	while (idx > -1)
	{
		//printf("%d", get_global_id(0));
		step++;
		//if (get_global_id(0) == 1)
		//{
			//printf("Node %d %d\n", idx, step );
		//}

		if (step > 10000)
			return false;

		node = scenedata->nodes[idx];

		if (LEAFNODE(node))
		{
			if (IntersectLeafAny(scenedata, STARTIDX(node), r))
			{
				hit = true;
				break;
			}
		}
		else
		{
			lbox = scenedata->bounds[node.left];
			rbox = scenedata->bounds[node.right];

			lefthit = IntersectBoxF(r, invdir, lbox, r->o.w);
			righthit = IntersectBoxF(r, invdir, rbox, r->o.w);

			if (lefthit > 0.f && righthit > 0.f)
			{
				int deferred = -1;
				if (lefthit > righthit)
				{
					idx = node.right;
					deferred = node.left;
				}
				else
				{
					idx = node.left;
					deferred = node.right;
				}

				*ptr++ = deferred;
				continue;
			}
			else if (lefthit > 0)
			{
				idx = node.left;
				continue;
			}
			else if (righthit > 0)
			{
				idx = node.right;
				continue;
			}
		}

		idx = *--ptr;
	}

	//if (get_global_id(0) == 1)
	//printf("Exiting %d\n", get_global_id(0) );

	return hit;
}

#else

// intersect Ray against the whole BVH structure
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

	HlbvhNode node;
	bbox lbox;
	bbox rbox;

	float lefthit = 0.f;
	float righthit = 0.f;

	while (idx > -1)
	{
		while (idx > -1)
		{
			node = scenedata->nodes[idx];

			if (LEAFNODE(node))
			{
				IntersectLeafClosest(scenedata, STARTIDX(node), r, isect);
			}
			else
			{
				lbox = scenedata->bounds[node.left];
				rbox = scenedata->bounds[node.right];

				lefthit = IntersectBoxF(r, invdir, lbox, isect->uvwt.w);
				righthit = IntersectBoxF(r, invdir, rbox, isect->uvwt.w);

				if (lefthit > 0.f && righthit > 0.f)
				{
					int deferred = -1;
					if (lefthit > righthit)
					{
						idx = node.right;
						deferred = node.left;
					}
					else
					{
						idx = node.left;
						deferred = node.right;
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
					idx = node.left;
					continue;
				}
				else if (righthit > 0)
				{
					idx = node.right;
					continue;
				}
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


	return isect->shapeid >= 0;
}



// intersect Ray against the whole BVH structure
bool IntersectSceneAny(SceneData const* scenedata, ray const* r, __global int* stack, __local int* ldsstack)
{
	const float3 invdir = native_recip(r->d.xyz);

	__global int* gsptr = stack;
	__local  int* lsptr = ldsstack;

	*lsptr = -1;
	lsptr += 64;

	int idx = 0;

	HlbvhNode node;
	bbox lbox;
	bbox rbox;

	float lefthit = 0.f;
	float righthit = 0.f;

	while (idx > -1)
	{
		while (idx > -1)
		{
			node = scenedata->nodes[idx];

			if (LEAFNODE(node))
			{
				if (IntersectLeafAny(scenedata, STARTIDX(node), r))
					return true;
			}
			else
			{
				lbox = scenedata->bounds[node.left];
				rbox = scenedata->bounds[node.right];

				lefthit = IntersectBoxF(r, invdir, lbox, r->o.w);
				righthit = IntersectBoxF(r, invdir, rbox, r->o.w);

				if (lefthit > 0.f && righthit > 0.f)
				{
					int deferred = -1;
					if (lefthit > righthit)
					{
						idx = node.right;
						deferred = node.left;
					}
					else
					{
						idx = node.left;
						deferred = node.right;
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
					idx = node.left;
					continue;
				}
				else if (righthit > 0)
				{
					idx = node.right;
					continue;
				}
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


#endif

__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void IntersectClosest(
	// Input
	__global HlbvhNode const* nodes,   // BVH nodes
	__global bbox const* bounds,   // BVH nodes
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
#ifndef LDS_BUG
	__local int ldsstack[SHORT_STACK_SIZE * 64];
#endif

	int global_id = get_global_id(0);
	int local_id = get_local_id(0);
	int group_id = get_group_id(0);

	// Fill scene data
	SceneData scenedata =
	{
		nodes,
		bounds,
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
#ifndef LDS_BUG
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
	// Input
	__global HlbvhNode const* nodes,   // BVH nodes
	__global bbox const* bounds,   // BVH nodes
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

#ifndef LDS_BUG
	__local int ldsstack[SHORT_STACK_SIZE * 64];
#endif

	int global_id = get_global_id(0);
	int local_id = get_local_id(0);
	int group_id = get_group_id(0);

	// Fill scene data 
	SceneData scenedata =
	{
		nodes,
		bounds,
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
#ifndef LDS_BUG
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
	// Input
	__global HlbvhNode const* nodes,   // BVH nodes
	__global bbox const* bounds,   // BVH nodes
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
#ifndef LDS_BUG
	__local int ldsstack[SHORT_STACK_SIZE * 64];
#endif

	int global_id = get_global_id(0);
	int local_id = get_local_id(0);
	int group_id = get_group_id(0);

	// Fill scene data 
	SceneData scenedata =
	{
		nodes,
		bounds,
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
#ifndef LDS_BUG
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
	__global HlbvhNode const* nodes,   // BVH nodes
	__global bbox const* bounds,   // BVH nodes
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
#ifndef LDS_BUG
	__local int ldsstack[SHORT_STACK_SIZE * 64];
#endif
	int global_id = get_global_id(0);
	int local_id = get_local_id(0);
	int group_id = get_group_id(0);

	// Fill scene data 
	SceneData scenedata =
	{
		nodes,
		bounds,
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
#ifndef LDS_BUG
			hitresults[global_id] = IntersectSceneAny(&scenedata, &r, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id) ? 1 : -1;
#else
			hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1;
#endif
		}
	}
}