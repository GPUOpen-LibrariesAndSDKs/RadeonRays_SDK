/* This is an auto-generated file. Do not edit manually*/

static const char g_bvh_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
" INCLUDES \n"\
" **************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"typedef struct _bbox \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int2 padding; \n"\
"} ray; \n"\
" \n"\
"typedef struct _Intersection \n"\
"{ \n"\
"    int shapeid; \n"\
"    int primid; \n"\
"    int padding0; \n"\
"    int padding1; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
"typedef struct _ShapeData \n"\
"{ \n"\
"    int id; \n"\
"    int bvhidx; \n"\
"    int mask; \n"\
"    int padding1; \n"\
"    float4 m0; \n"\
"    float4 m1; \n"\
"    float4 m2; \n"\
"    float4 m3; \n"\
"    float4  linearvelocity; \n"\
"    float4  angularvelocity; \n"\
"} ShapeData; \n"\
" \n"\
"typedef bbox BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[3]; \n"\
"    int shapeidx; \n"\
"    // Primitive ID \n"\
"    int id; \n"\
"    // Idx count \n"\
"    int cnt; \n"\
" \n"\
"    int2 padding; \n"\
"} Face; \n"\
" \n"\
"#ifndef APPLE \n"\
" \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
"float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"ray transform_ray(ray r, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    ray res; \n"\
"    res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3); \n"\
"    res.o.w = r.o.w; \n"\
"    res.d.w = r.d.w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_mul(float4 q1, float4 q2) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x; \n"\
"    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y; \n"\
"    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z; \n"\
"    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_conjugate(float4 q) \n"\
"{ \n"\
"    return make_float4(-q.x, -q.y, -q.z, q.w); \n"\
"} \n"\
" \n"\
"float4 quaternion_inverse(float4 q) \n"\
"{ \n"\
"    float sqnorm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w; \n"\
"     \n"\
"    if (sqnorm != 0.f) \n"\
"    { \n"\
"        return quaternion_conjugate(q) / sqnorm; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"    } \n"\
"} \n"\
" \n"\
"void rotate_ray(ray* r, float4 q) \n"\
"{ \n"\
"    float4 qinv = quaternion_inverse(q); \n"\
"    float4 v = make_float4(r->o.x, r->o.y, r->o.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->o.xyz = v.xyz; \n"\
"    v = make_float4(r->d.x, r->d.y, r->d.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->d.xyz = v.xyz; \n"\
"} \n"\
" \n"\
"// Intersect Ray against triangle \n"\
"int IntersectTriangle(ray const* r, float3 v1, float3 v2, float3 v3, Intersection* isect) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > isect->uvwt.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        isect->uvwt = make_float4(b1, b2, 0.f, temp); \n"\
"        return 1; \n"\
"    } \n"\
"} \n"\
" \n"\
"int IntersectTriangleP(ray const* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > r->o.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"     \n"\
"    return 1; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"int IntersectBox(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? 1 : 0; \n"\
"} \n"\
" \n"\
"float IntersectBoxF(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.f; \n"\
"} \n"\
" \n"\
"int Ray_GetMask(ray const* r) \n"\
"{ \n"\
"	return r->extra.x; \n"\
"} \n"\
" \n"\
"int Ray_IsActive(ray const* r) \n"\
"{ \n"\
"	return r->extra.y; \n"\
"} \n"\
" \n"\
"float Ray_GetMaxT(ray const* r) \n"\
"{ \n"\
"	return r->o.w; \n"\
"} \n"\
" \n"\
"float Ray_GetTime(ray const* r) \n"\
"{ \n"\
"	return r->d.w; \n"\
"} \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"/************************************************************************* \n"\
" TYPE DEFINITIONS \n"\
" **************************************************************************/ \n"\
"#define STARTIDX(x)     (((int)(x->pmin.w))) \n"\
"#define LEAFNODE(x)     (((x).pmin.w) != -1.f) \n"\
" \n"\
"typedef struct  \n"\
"{ \n"\
"    // BVH structure \n"\
"    __global BvhNode const*       nodes; \n"\
"    // Scene positional data \n"\
"    __global float3 const*        vertices; \n"\
"    // Scene indices \n"\
"    __global Face const*          faces; \n"\
"    // Shape data \n"\
"    __global ShapeData const*     shapes; \n"\
"    // Extra data \n"\
"    __global int const*           extra; \n"\
"} SceneData; \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
" \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"BVH FUNCTIONS \n"\
"**************************************************************************/ \n"\
"//  intersect a ray with leaf BVH node \n"\
"void IntersectLeafClosest( \n"\
"    SceneData const* scenedata, \n"\
"    BvhNode const* node, \n"\
"    ray const* r,                // ray to instersect \n"\
"    Intersection* isect          // Intersection structure \n"\
"    ) \n"\
"{ \n"\
"    float3 v1, v2, v3; \n"\
"    Face face; \n"\
" \n"\
"    int start = STARTIDX(node); \n"\
"    face = scenedata->faces[start]; \n"\
"    v1 = scenedata->vertices[face.idx[0]]; \n"\
"    v2 = scenedata->vertices[face.idx[1]]; \n"\
"    v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"    int shapemask = scenedata->shapes[face.shapeidx].mask; \n"\
" \n"\
"    if (Ray_GetMask(r) & shapemask) \n"\
"    { \n"\
"        if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"        { \n"\
"            isect->primid = face.id; \n"\
"            isect->shapeid = scenedata->shapes[face.shapeidx].id; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafAny( \n"\
"    SceneData const* scenedata, \n"\
"    BvhNode const* node, \n"\
"    ray const* r                      // ray to instersect \n"\
"    ) \n"\
"{ \n"\
"    float3 v1, v2, v3; \n"\
"    Face face; \n"\
" \n"\
"    int start = STARTIDX(node); \n"\
"    face = scenedata->faces[start]; \n"\
"    v1 = scenedata->vertices[face.idx[0]]; \n"\
"    v2 = scenedata->vertices[face.idx[1]]; \n"\
"    v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"	int shapemask = scenedata->shapes[face.shapeidx].mask; \n"\
" \n"\
"	if (Ray_GetMask(r) & shapemask) \n"\
"	{ \n"\
"		if (IntersectTriangleP(r, v1, v2, v3)) \n"\
"		{ \n"\
"			return true; \n"\
"		} \n"\
"	} \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"void IntersectSceneClosest(SceneData const* scenedata,  ray const* r, Intersection* isect) \n"\
"{ \n"\
"    const float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d.xyz; \n"\
" \n"\
"    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w); \n"\
"    isect->shapeid = -1; \n"\
"    isect->primid = -1; \n"\
" \n"\
"    int idx = 0; \n"\
" \n"\
"    while (idx != -1) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        // If this is the leaf try to intersect against contained triangle. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node, isect->uvwt.w)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                IntersectLeafClosest(scenedata, &node, r, isect); \n"\
"                idx = (int)(node.pmax.w); \n"\
"            } \n"\
"            // Traverse child nodes otherwise. \n"\
"            else \n"\
"            { \n"\
"                ++idx; \n"\
"            } \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            idx = (int)(node.pmax.w); \n"\
"        } \n"\
"    }; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny(SceneData const* scenedata,  ray const* r) \n"\
"{ \n"\
"    float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d.xyz; \n"\
" \n"\
"    int idx = 0; \n"\
"    while (idx != -1) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        // If this is the leaf try to intersect against contained triangle. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node, r->o.w)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                if (IntersectLeafAny(scenedata, &node, r)) \n"\
"                { \n"\
"                    return true; \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    idx = (int)(node.pmax.w); \n"\
"                } \n"\
"            } \n"\
"            // Traverse child nodes otherwise. \n"\
"            else \n"\
"            { \n"\
"                ++idx; \n"\
"            } \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            idx = (int)(node.pmax.w); \n"\
"        } \n"\
"    }; \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectClosestAMD( \n"\
"// Input \n"\
"__global BvhNode const* nodes,   // BVH nodes \n"\
"__global float3 const* vertices, // Scene positional data \n"\
"__global Face const* faces,    // Scene indices \n"\
"__global ShapeData const* shapes,     // Shapes \n"\
"__global ray const* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"int numrays,               // Number of rays to process \n"\
"__global Intersection* hits, // Hit datas \n"\
"__global int*          raycnt  \n"\
"    ) \n"\
"{ \n"\
"    __local int nextrayidx; \n"\
" \n"\
"    int global_id  = get_global_id(0); \n"\
"    int local_id  = get_local_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        0 \n"\
"    }; \n"\
" \n"\
"    if (local_id == 0) \n"\
"    { \n"\
"        nextrayidx = 0; \n"\
"    } \n"\
" \n"\
"    int ridx = 0; \n"\
"    Intersection isect; \n"\
" \n"\
"    while (ridx < numrays) \n"\
"    { \n"\
"        if (local_id == 0) \n"\
"        { \n"\
"            nextrayidx = atomic_add(raycnt, 64); \n"\
"        } \n"\
" \n"\
"        ridx = nextrayidx + local_id; \n"\
" \n"\
"        if (ridx >= numrays) \n"\
"            break; \n"\
" \n"\
"        // Fetch ray \n"\
"        ray r = rays[ridx]; \n"\
" \n"\
"        if (Ray_IsActive(&r)) \n"\
"        { \n"\
"        	// Calculate closest hit \n"\
"        	IntersectSceneClosest(&scenedata, &r, &isect); \n"\
" \n"\
"        	// Write data back in case of a hit \n"\
"        	hits[ridx] = isect; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectAnyAMD( \n"\
"    // Input \n"\
"    __global BvhNode const* nodes,   // BVH nodes \n"\
"    __global float3 const* vertices, // Scene positional data \n"\
"    __global Face const* faces,    // Scene indices \n"\
"	__global ShapeData const* shapes,     // Shapes \n"\
"    __global ray const* rays,        // Ray workload \n"\
"    int offset,                // Offset in rays array \n"\
"    int numrays,               // Number of rays to process					 \n"\
"    __global int* hitresults,  // Hit results \n"\
"    __global int* raycnt \n"\
"    ) \n"\
"{ \n"\
"    __local int nextrayidx; \n"\
" \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        0 \n"\
"    }; \n"\
" \n"\
"    if (local_id == 0) \n"\
"    { \n"\
"        nextrayidx = 0; \n"\
"    } \n"\
" \n"\
"    int ridx = 0; \n"\
"    while (ridx < numrays) \n"\
"    { \n"\
"        if (local_id == 0) \n"\
"        { \n"\
"            nextrayidx = atomic_add(raycnt, 64); \n"\
"        } \n"\
" \n"\
"        ridx = nextrayidx + local_id; \n"\
" \n"\
"        if (ridx >= numrays) \n"\
"            break; \n"\
" \n"\
"        // Fetch ray \n"\
"        ray r = rays[ridx]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"			hitresults[ridx] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"		} \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectClosestRCAMD( \n"\
"    __global BvhNode const* nodes,   // BVH nodes \n"\
"    __global float3 const* vertices, // Scene positional data \n"\
"    __global Face const* faces,      // Scene indices \n"\
"	__global ShapeData const* shapes,     // Shapes \n"\
"    __global ray const* rays,        // Ray workload \n"\
"    int offset,                // Offset in rays array \n"\
"    __global int const* numrays,     // Number of rays in the workload \n"\
"    __global Intersection* hits, // Hit datas \n"\
"    __global int* raycnt \n"\
"    ) \n"\
"{ \n"\
"    __local int nextrayidx; \n"\
" \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        0 \n"\
"    }; \n"\
" \n"\
"    if (local_id == 0) \n"\
"    { \n"\
"        nextrayidx = 0; \n"\
"    } \n"\
" \n"\
"    int ridx = 0; \n"\
"    Intersection isect; \n"\
" \n"\
"    while (ridx < *numrays) \n"\
"    { \n"\
"        if (local_id == 0) \n"\
"        { \n"\
"            nextrayidx = atomic_add(raycnt, 64); \n"\
"        } \n"\
" \n"\
"        ridx = nextrayidx + local_id; \n"\
" \n"\
"        if (ridx >= *numrays) \n"\
"            break; \n"\
" \n"\
"        // Fetch ray \n"\
"		ray r = rays[ridx]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect); \n"\
"			// Write data back in case of a hit \n"\
"			hits[ridx] = isect; \n"\
"		} \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectAnyRCAMD( \n"\
"    // Input \n"\
"    __global BvhNode const* nodes,   // BVH nodes \n"\
"    __global float3 const* vertices, // Scene positional data \n"\
"    __global Face const* faces,    // Scene indices \n"\
"	__global ShapeData const* shapes,     // Shapes \n"\
"    __global ray const* rays,        // Ray workload \n"\
"    int offset,                // Offset in rays array \n"\
"    __global int const* numrays,     // Number of rays in the workload \n"\
"    __global int* hitresults,   // Hit results \n"\
"    __global int* raycnt \n"\
"    ) \n"\
"{ \n"\
"    __local int nextrayidx; \n"\
" \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        0 \n"\
"    }; \n"\
" \n"\
"    if (local_id == 0) \n"\
"    { \n"\
"        nextrayidx = 0; \n"\
"    } \n"\
" \n"\
"    int ridx = 0; \n"\
"    while (ridx < *numrays) \n"\
"    { \n"\
"        if (local_id == 0) \n"\
"        { \n"\
"            nextrayidx = atomic_add(raycnt, 64); \n"\
"        } \n"\
" \n"\
"        ridx = nextrayidx + local_id; \n"\
" \n"\
"        if (ridx >= *numrays) \n"\
"            break; \n"\
" \n"\
"        // Fetch ray \n"\
"        ray r = rays[ridx]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"			hitresults[ridx] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"		} \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectClosest( \n"\
"// Input \n"\
"__global BvhNode const* nodes,   // BVH nodes \n"\
"__global float3 const* vertices, // Scene positional data \n"\
"__global Face const* faces,    // Scene indices \n"\
"__global ShapeData const* shapes,     // Shapes \n"\
"__global ray const* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"int numrays,               // Number of rays to process \n"\
"__global Intersection* hits // Hit datas \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        0 \n"\
"    }; \n"\
" \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect); \n"\
" \n"\
"			// Write data back in case of a hit \n"\
"			hits[global_id] = isect; \n"\
"		} \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectAny( \n"\
"// Input \n"\
"__global BvhNode const* nodes,   // BVH nodes \n"\
"__global float3 const* vertices, // Scene positional data \n"\
"__global Face const* faces,    // Scene indices \n"\
"__global ShapeData const* shapes,     // Shapes \n"\
"__global ray const* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"int numrays,               // Number of rays to process					 \n"\
"__global int* hitresults  // Hit results \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        0 \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[offset + global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"			hitresults[offset + global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"		} \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectClosestRC( \n"\
"__global BvhNode const* nodes,   // BVH nodes \n"\
"__global float3 const* vertices, // Scene positional data \n"\
"__global Face const* faces,      // Scene indices \n"\
"__global ShapeData const* shapes,     // Shapes \n"\
"__global ray const* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"__global int const* numrays,     // Number of rays in the workload \n"\
"__global Intersection* hits // Hit datas \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        0 \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        int idx = offset + global_id; \n"\
"        ray r = rays[idx]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect); \n"\
"			// Write data back in case of a hit \n"\
"			hits[idx] = isect; \n"\
"		} \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectAnyRC( \n"\
"// Input \n"\
"__global BvhNode const* nodes,   // BVH nodes \n"\
"__global float3 const* vertices, // Scene positional data \n"\
"__global Face const* faces,    // Scene indices \n"\
"__global ShapeData const* shapes,     // Shapes \n"\
"__global ray const* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"__global int const* numrays,     // Number of rays in the workload \n"\
"__global int* hitresults   // Hit results \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        0 \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[offset + global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"			hitresults[offset + global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"		} \n"\
"    } \n"\
"} \n"\
;
static const char g_bvh2l_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
" INCLUDES \n"\
" **************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"typedef struct _bbox \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int2 padding; \n"\
"} ray; \n"\
" \n"\
"typedef struct _Intersection \n"\
"{ \n"\
"    int shapeid; \n"\
"    int primid; \n"\
"    int padding0; \n"\
"    int padding1; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
"typedef struct _ShapeData \n"\
"{ \n"\
"    int id; \n"\
"    int bvhidx; \n"\
"    int mask; \n"\
"    int padding1; \n"\
"    float4 m0; \n"\
"    float4 m1; \n"\
"    float4 m2; \n"\
"    float4 m3; \n"\
"    float4  linearvelocity; \n"\
"    float4  angularvelocity; \n"\
"} ShapeData; \n"\
" \n"\
"typedef bbox BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[3]; \n"\
"    int shapeidx; \n"\
"    // Primitive ID \n"\
"    int id; \n"\
"    // Idx count \n"\
"    int cnt; \n"\
" \n"\
"    int2 padding; \n"\
"} Face; \n"\
" \n"\
"#ifndef APPLE \n"\
" \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
"float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"ray transform_ray(ray r, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    ray res; \n"\
"    res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3); \n"\
"    res.o.w = r.o.w; \n"\
"    res.d.w = r.d.w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_mul(float4 q1, float4 q2) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x; \n"\
"    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y; \n"\
"    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z; \n"\
"    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_conjugate(float4 q) \n"\
"{ \n"\
"    return make_float4(-q.x, -q.y, -q.z, q.w); \n"\
"} \n"\
" \n"\
"float4 quaternion_inverse(float4 q) \n"\
"{ \n"\
"    float sqnorm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w; \n"\
"     \n"\
"    if (sqnorm != 0.f) \n"\
"    { \n"\
"        return quaternion_conjugate(q) / sqnorm; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"    } \n"\
"} \n"\
" \n"\
"void rotate_ray(ray* r, float4 q) \n"\
"{ \n"\
"    float4 qinv = quaternion_inverse(q); \n"\
"    float4 v = make_float4(r->o.x, r->o.y, r->o.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->o.xyz = v.xyz; \n"\
"    v = make_float4(r->d.x, r->d.y, r->d.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->d.xyz = v.xyz; \n"\
"} \n"\
" \n"\
"// Intersect Ray against triangle \n"\
"int IntersectTriangle(ray const* r, float3 v1, float3 v2, float3 v3, Intersection* isect) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > isect->uvwt.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        isect->uvwt = make_float4(b1, b2, 0.f, temp); \n"\
"        return 1; \n"\
"    } \n"\
"} \n"\
" \n"\
"int IntersectTriangleP(ray const* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > r->o.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"     \n"\
"    return 1; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"int IntersectBox(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? 1 : 0; \n"\
"} \n"\
" \n"\
"float IntersectBoxF(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.f; \n"\
"} \n"\
" \n"\
"int Ray_GetMask(ray const* r) \n"\
"{ \n"\
"	return r->extra.x; \n"\
"} \n"\
" \n"\
"int Ray_IsActive(ray const* r) \n"\
"{ \n"\
"	return r->extra.y; \n"\
"} \n"\
" \n"\
"float Ray_GetMaxT(ray const* r) \n"\
"{ \n"\
"	return r->o.w; \n"\
"} \n"\
" \n"\
"float Ray_GetTime(ray const* r) \n"\
"{ \n"\
"	return r->d.w; \n"\
"} \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
" \n"\
" \n"\
" \n"\
"#define STARTIDX(x)     (((int)(x->pmin.w))) \n"\
"#define SHAPEIDX(x)     (((int)(x.pmin.w))) \n"\
"#define LEAFNODE(x)     (((x).pmin.w) != -1.f) \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    // BVH structure \n"\
"    __global BvhNode*  const        nodes; \n"\
"    // Scene positional data \n"\
"    __global float3*  const              vertices; \n"\
"    // Scene indices \n"\
"    __global Face* const                faces; \n"\
"    // Transforms \n"\
"    __global ShapeData* const      shapes; \n"\
" \n"\
"    // Root BVH idx \n"\
"    int rootidx; \n"\
"} SceneData; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"BVH FUNCTIONS \n"\
"**************************************************************************/ \n"\
"//  intersect a ray with leaf BVH node \n"\
"void IntersectLeafClosest( \n"\
"    SceneData const* scenedata, \n"\
"    BvhNode const* node, \n"\
"    ray const* r,                // ray to instersect \n"\
"    int const shapeid, \n"\
"    Intersection* isect          // Intersection structure \n"\
"    ) \n"\
"{ \n"\
"    float3 v1, v2, v3; \n"\
"    Face face; \n"\
" \n"\
"    int start = STARTIDX(node); \n"\
"    face = scenedata->faces[start]; \n"\
"    v1 = scenedata->vertices[face.idx[0]]; \n"\
"    v2 = scenedata->vertices[face.idx[1]]; \n"\
"    v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"    if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"    { \n"\
"        isect->primid = face.id; \n"\
"        isect->shapeid = shapeid; \n"\
"    } \n"\
"} \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafAny( \n"\
"    SceneData const* scenedata, \n"\
"    BvhNode const* node, \n"\
"    ray const* r                      // ray to instersect \n"\
"    ) \n"\
"{ \n"\
"    float3 v1, v2, v3; \n"\
"    Face face; \n"\
" \n"\
"    int start = STARTIDX(node); \n"\
"    face = scenedata->faces[start]; \n"\
"    v1 = scenedata->vertices[face.idx[0]]; \n"\
"    v2 = scenedata->vertices[face.idx[1]]; \n"\
"    v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"	if (IntersectTriangleP(r, v1, v2, v3)) \n"\
"	{ \n"\
"		return true; \n"\
"	} \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
" \n"\
"// intersect Ray against the whole BVH2L structure \n"\
"bool IntersectSceneClosest2L(SceneData* scenedata, ray* r, Intersection* isect) \n"\
"{ \n"\
"    // Init intersection \n"\
"    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w); \n"\
"    isect->shapeid = -1; \n"\
"    isect->primid = -1; \n"\
" \n"\
"    // Precompute invdir for bbox testing \n"\
"    float3 invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"    float3 invdirtop = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"    // We need to keep original ray around for returns from bottom hierarchy \n"\
"    ray topray = *r; \n"\
" \n"\
"    // Fetch top level BVH index \n"\
"    int idx = scenedata->rootidx; \n"\
"    // -1 indicates we are traversing top level \n"\
"    int topidx = -1; \n"\
"    // Current shape id \n"\
"    int shapeid = -1; \n"\
"    while (idx != -1) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node, isect->uvwt.w)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy) \n"\
"                // or containing another BVH (top level hierarhcy) \n"\
"                if (topidx != -1) \n"\
"                { \n"\
"                    // This is bottom level, so intersect with a primitives \n"\
"                    IntersectLeafClosest(scenedata, &node, r, shapeid, isect); \n"\
" \n"\
"                    // And goto next node \n"\
"                    idx = (int)(node.pmax.w); \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // This is top level hierarchy leaf \n"\
"                    // Save top node index for return \n"\
"                    topidx = idx; \n"\
"                    // Get shape descrition struct index \n"\
"                    int shapeidx = SHAPEIDX(node); \n"\
"                    // Get shape mask \n"\
"                    int shapemask = scenedata->shapes[shapeidx].mask; \n"\
"                    // Drill into 2nd level BVH only if the geometry is not masked vs current ray \n"\
"                    // otherwise skip the subtree \n"\
"                    if (Ray_GetMask(r) && shapemask) \n"\
"                    { \n"\
"                        // Fetch bottom level BVH index \n"\
"                        idx = scenedata->shapes[shapeidx].bvhidx; \n"\
"                        shapeid = scenedata->shapes[shapeidx].id; \n"\
" \n"\
"                        // Fetch BVH transform \n"\
"                        float4 wmi0 = scenedata->shapes[shapeidx].m0; \n"\
"                        float4 wmi1 = scenedata->shapes[shapeidx].m1; \n"\
"                        float4 wmi2 = scenedata->shapes[shapeidx].m2; \n"\
"                        float4 wmi3 = scenedata->shapes[shapeidx].m3; \n"\
" \n"\
"                        // Apply linear motion blur (world coordinates) \n"\
"                        //float4 lmv = scenedata->shapes[shapeidx].linearvelocity; \n"\
"                        //float4 amv = scenedata->shapes[SHAPEDATAIDX(node)].angularvelocity; \n"\
"                        //r->o.xyz -= (lmv.xyz*r->d.w); \n"\
"                        // Transfrom the ray \n"\
"                        *r = transform_ray(*r, wmi0, wmi1, wmi2, wmi3); \n"\
"                        // rotate_ray(r, amv); \n"\
"                        // Recalc invdir \n"\
"                        invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"                        // And continue traversal of the bottom level BVH \n"\
"                        continue; \n"\
"                    } \n"\
"                    else \n"\
"                    { \n"\
"                        idx = -1; \n"\
"                    } \n"\
"                } \n"\
"            } \n"\
"            // Traverse child nodes otherwise. \n"\
"            else \n"\
"            { \n"\
"                // This is an internal node, proceed to left child (it is at current + 1 index) \n"\
"                idx = idx + 1; \n"\
"            } \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            // We missed the node, goto next one \n"\
"            idx = (int)(node.pmax.w); \n"\
"        } \n"\
" \n"\
"        // Here check if we ended up traversing bottom level BVH \n"\
"        // in this case idx = -1 and topidx has valid value \n"\
"        if (idx == -1 && topidx != -1) \n"\
"        { \n"\
"            //  Proceed to next top level node \n"\
"            idx = (int)(scenedata->nodes[topidx].pmax.w); \n"\
"            // Set topidx \n"\
"            topidx = -1; \n"\
"            // Restore ray here \n"\
"            r->o = topray.o; \n"\
"            r->d = topray.d; \n"\
"            // Restore invdir \n"\
"            invdir = invdirtop; \n"\
"        } \n"\
"    } \n"\
" \n"\
"    return isect->shapeid >= 0; \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole BVH2L structure \n"\
"bool IntersectSceneAny2L(SceneData* scenedata, ray* r) \n"\
"{ \n"\
"    // Precompute invdir for bbox testing \n"\
"    float3 invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"    float3 invdirtop = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"    // We need to keep original ray around for returns from bottom hierarchy \n"\
"    ray topray = *r; \n"\
" \n"\
"    // Fetch top level BVH index \n"\
"    int idx = scenedata->rootidx; \n"\
"    // -1 indicates we are traversing top level \n"\
"    int topidx = -1; \n"\
"    while (idx != -1) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node, r->o.w)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy) \n"\
"                // or containing another BVH (top level hierarhcy) \n"\
"                if (topidx != -1) \n"\
"                { \n"\
"                    // This is bottom level, so intersect with a primitives \n"\
"                    if (IntersectLeafAny(scenedata, &node, r)) \n"\
"                        return true; \n"\
"                    // And goto next node \n"\
"                    idx = (int)(node.pmax.w); \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // This is top level hierarchy leaf \n"\
"                    // Save top node index for return \n"\
"                    topidx = idx; \n"\
"                    // Get shape descrition struct index \n"\
"                    int shapeidx = SHAPEIDX(node); \n"\
"                    // Get shape mask \n"\
"                    int shapemask = scenedata->shapes[shapeidx].mask; \n"\
"                    // Drill into 2nd level BVH only if the geometry is not masked vs current ray \n"\
"                    // otherwise skip the subtree \n"\
"                    if (Ray_GetMask(r) && shapemask) \n"\
"                    { \n"\
"                        // Fetch bottom level BVH index \n"\
"                        idx = scenedata->shapes[shapeidx].bvhidx; \n"\
" \n"\
"                        // Fetch BVH transform \n"\
"                        float4 wmi0 = scenedata->shapes[shapeidx].m0; \n"\
"                        float4 wmi1 = scenedata->shapes[shapeidx].m1; \n"\
"                        float4 wmi2 = scenedata->shapes[shapeidx].m2; \n"\
"                        float4 wmi3 = scenedata->shapes[shapeidx].m3; \n"\
" \n"\
"                        // Apply linear motion blur (world coordinates) \n"\
"                        //float4 lmv = scenedata->shapes[shapeidx].linearvelocity; \n"\
"                        //float4 amv = scenedata->shapes[SHAPEDATAIDX(node)].angularvelocity; \n"\
"                        //r->o.xyz -= (lmv.xyz*r->d.w); \n"\
"                        // Transfrom the ray \n"\
"                        *r = transform_ray(*r, wmi0, wmi1, wmi2, wmi3); \n"\
"                        //rotate_ray(r, amv); \n"\
"                        // Recalc invdir \n"\
"                        invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"                        // And continue traversal of the bottom level BVH \n"\
"                        continue; \n"\
"                    } \n"\
"                    else \n"\
"                    { \n"\
"                        // Skip the subtree \n"\
"                        idx = -1; \n"\
"                    } \n"\
"                } \n"\
"            } \n"\
"            // Traverse child nodes otherwise. \n"\
"            else \n"\
"            { \n"\
"                // This is an internal node, proceed to left child (it is at current + 1 index) \n"\
"                idx = idx + 1; \n"\
"            } \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            // We missed the node, goto next one \n"\
"            idx = (int)(node.pmax.w); \n"\
"        } \n"\
" \n"\
"        // Here check if we ended up traversing bottom level BVH \n"\
"        // in this case idx = 0xFFFFFFFF and topidx has valid value \n"\
"        if (idx == -1 && topidx != -1) \n"\
"        { \n"\
"            //  Proceed to next top level node \n"\
"            idx = (int)(scenedata->nodes[topidx].pmax.w); \n"\
"            // Set topidx \n"\
"            topidx = -1; \n"\
"            // Restore ray here \n"\
"            *r = topray; \n"\
"            // Restore invdir \n"\
"            invdir = invdirtop; \n"\
"        } \n"\
"    } \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
" \n"\
"// 2 level variants \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectClosest2L( \n"\
"// Input \n"\
"__global BvhNode* nodes,   // BVH nodes \n"\
"__global float3* vertices, // Scene positional data \n"\
"__global Face* faces,    // Scene indices \n"\
"__global ShapeData* shapes, // Transforms \n"\
"int rootidx,               // BVH root idx \n"\
"__global ray* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"int numrays,               // Number of rays to process \n"\
"__global Intersection* hits // Hit datas \n"\
") \n"\
"{ \n"\
" \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Fill scene data \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        rootidx \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        int idx = offset + global_id; \n"\
"        ray r = rays[idx]; \n"\
" \n"\
"        if (Ray_IsActive(&r)) \n"\
"        { \n"\
"        	// Calculate closest hit \n"\
"        	Intersection isect; \n"\
"        	IntersectSceneClosest2L(&scenedata, &r, &isect); \n"\
" \n"\
"        	// Write data back in case of a hit \n"\
"        	hits[idx] = isect; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectAny2L( \n"\
"// Input \n"\
"__global BvhNode* nodes,   // BVH nodes \n"\
"__global float3* vertices, // Scene positional data \n"\
"__global Face* faces,    // Scene indices \n"\
"__global ShapeData* shapes, // Transforms \n"\
"int rootidx,               // BVH root idx \n"\
"__global ray* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"int numrays,               // Number of rays to process \n"\
"__global int* hitresults   // Hit results \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Fill scene data \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        rootidx \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[offset + global_id]; \n"\
" \n"\
"        if (Ray_IsActive(&r)) \n"\
"        { \n"\
"        	// Calculate any intersection \n"\
"        	hitresults[offset + global_id] = IntersectSceneAny2L(&scenedata, &r) ? 1 : -1; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectClosestRC2L( \n"\
"// Input \n"\
"__global BvhNode* nodes,   // BVH nodes \n"\
"__global float3* vertices, // Scene positional data \n"\
"__global Face* faces,    // Scene indices \n"\
"__global ShapeData* shapes, // Transforms \n"\
"int rootidx,               // BVH root idx \n"\
"__global ray* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"__global int* numrays,     // Number of rays in the workload \n"\
"__global Intersection* hits // Hit datas \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Fill scene data \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        rootidx \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        int idx = offset + global_id; \n"\
"        ray r = rays[idx]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
"			IntersectSceneClosest2L(&scenedata, &r, &isect); \n"\
" \n"\
"			hits[idx] = isect; \n"\
"		} \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectAnyRC2L( \n"\
"// Input \n"\
"__global BvhNode* nodes,   // BVH nodes \n"\
"__global float3* vertices, // Scene positional data \n"\
"__global Face* faces,    // Scene indices \n"\
"__global ShapeData* shapes, // Transforms \n"\
"int rootidx,               // BVH root idx \n"\
"__global ray* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"__global int* numrays,     // Number of rays in the workload \n"\
"__global int* hitresults   // Hit results \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapes, \n"\
"        rootidx \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[offset + global_id]; \n"\
" \n"\
"        if (Ray_IsActive(&r)) \n"\
"        { \n"\
"            // Calculate any intersection \n"\
"            hitresults[offset + global_id] = IntersectSceneAny2L(&scenedata, &r) ? 1 : -1; \n"\
"        } \n"\
"    } \n"\
"} \n"\
;
static const char g_CLW_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable \n"\
"#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable \n"\
" \n"\
" \n"\
"// --------------------- HELPERS ------------------------ \n"\
"//#define INT_MAX 0x7FFFFFFF \n"\
" \n"\
"// -------------------- MACRO -------------------------- \n"\
"// Apple OCL compiler has this by default,  \n"\
"// so embrace with #ifdef in the future \n"\
"#define DEFINE_MAKE_4(type)\\ \n"\
"    type##4 make_##type##4(type x, type y, type z, type w)\\ \n"\
"{\\ \n"\
"    type##4 res;\\ \n"\
"    res.x = x;\\ \n"\
"    res.y = y;\\ \n"\
"    res.z = z;\\ \n"\
"    res.w = w;\\ \n"\
"    return res;\\ \n"\
"} \n"\
" \n"\
"// Multitype macros to handle parallel primitives \n"\
"#define DEFINE_SAFE_LOAD_4(type)\\ \n"\
"    type##4 safe_load_##type##4(__global type##4* source, uint idx, uint sizeInTypeUnits)\\ \n"\
"{\\ \n"\
"    type##4 res = make_##type##4(0, 0, 0, 0);\\ \n"\
"    if (((idx + 1) << 2)  <= sizeInTypeUnits)\\ \n"\
"    res = source[idx];\\ \n"\
"    else\\ \n"\
"    {\\ \n"\
"    if ((idx << 2) < sizeInTypeUnits) res.x = source[idx].x;\\ \n"\
"    if ((idx << 2) + 1 < sizeInTypeUnits) res.y = source[idx].y;\\ \n"\
"    if ((idx << 2) + 2 < sizeInTypeUnits) res.z = source[idx].z;\\ \n"\
"    }\\ \n"\
"    return res;\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SAFE_STORE_4(type)\\ \n"\
"    void safe_store_##type##4(type##4 val, __global type##4* dest, uint idx, uint sizeInTypeUnits)\\ \n"\
"{\\ \n"\
"    if ((idx + 1) * 4  <= sizeInTypeUnits)\\ \n"\
"    dest[idx] = val;\\ \n"\
"    else\\ \n"\
"    {\\ \n"\
"    if (idx*4 < sizeInTypeUnits) dest[idx].x = val.x;\\ \n"\
"    if (idx*4 + 1 < sizeInTypeUnits) dest[idx].y = val.y;\\ \n"\
"    if (idx*4 + 2 < sizeInTypeUnits) dest[idx].z = val.z;\\ \n"\
"    }\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_GROUP_SCAN_EXCLUSIVE(type)\\ \n"\
"    void group_scan_exclusive_##type(int localId, int groupSize, __local type* shmem)\\ \n"\
"{\\ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"    if (localId == 0)\\ \n"\
"    shmem[groupSize - 1] = 0;\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        type temp = shmem[(2*localId + 1)*stride-1];\\ \n"\
"        shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_GROUP_SCAN_EXCLUSIVE_SUM(type)\\ \n"\
"    void group_scan_exclusive_sum_##type(int localId, int groupSize, __local type* shmem, type* sum)\\ \n"\
"{\\ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"    *sum = shmem[groupSize - 1];\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    if (localId == 0){\\ \n"\
"    shmem[groupSize - 1] = 0;}\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        type temp = shmem[(2*localId + 1)*stride-1];\\ \n"\
"        shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"} \n"\
" \n"\
" \n"\
"#define DEFINE_GROUP_SCAN_EXCLUSIVE_PART(type)\\ \n"\
"    type group_scan_exclusive_part_##type( int localId, int groupSize, __local type* shmem)\\ \n"\
"{\\ \n"\
"    type sum = 0;\\ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"    if (localId == 0)\\ \n"\
"    {\\ \n"\
"    sum = shmem[groupSize - 1];\\ \n"\
"    shmem[groupSize - 1] = 0;\\ \n"\
"    }\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        type temp = shmem[(2*localId + 1)*stride-1];\\ \n"\
"        shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"    return sum;\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SCAN_EXCLUSIVE(type)\\ \n"\
"    __kernel void scan_exclusive_##type(__global type const* in_array, __global type* out_array, __local type* shmem)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int localId   = get_local_id(0);\\ \n"\
"    int groupSize = get_local_size(0);\\ \n"\
"    int groupId   = get_group_id(0);\\ \n"\
"    shmem[localId] = in_array[2*globalId] + in_array[2*globalId + 1];\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    group_scan_exclusive_##type(localId, groupSize, shmem);\\ \n"\
"    out_array[2 * globalId + 1] = shmem[localId] + in_array[2*globalId];\\ \n"\
"    out_array[2 * globalId] = shmem[localId];\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SCAN_EXCLUSIVE_4(type)\\ \n"\
"    __attribute__((reqd_work_group_size(64, 1, 1)))\\ \n"\
"    __kernel void scan_exclusive_##type##4(__global type##4 const* in_array, __global type##4* out_array, uint numElems, __local type* shmem)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int localId   = get_local_id(0);\\ \n"\
"    int groupSize = get_local_size(0);\\ \n"\
"    type##4 v1 = safe_load_##type##4(in_array, 2*globalId, numElems);\\ \n"\
"    type##4 v2 = safe_load_##type##4(in_array, 2*globalId + 1, numElems);\\ \n"\
"    v1.y += v1.x; v1.w += v1.z; v1.w += v1.y;\\ \n"\
"    v2.y += v2.x; v2.w += v2.z; v2.w += v2.y;\\ \n"\
"    v2.w += v1.w;\\ \n"\
"    shmem[localId] = v2.w;\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    group_scan_exclusive_##type(localId, groupSize, shmem);\\ \n"\
"    v2.w = shmem[localId];\\ \n"\
"    type t = v1.w; v1.w = v2.w; v2.w += t;\\ \n"\
"    t = v1.y; v1.y = v1.w; v1.w += t;\\ \n"\
"    t = v2.y; v2.y = v2.w; v2.w += t;\\ \n"\
"    t = v1.x; v1.x = v1.y; v1.y += t;\\ \n"\
"    t = v2.x; v2.x = v2.y; v2.y += t;\\ \n"\
"    t = v1.z; v1.z = v1.w; v1.w += t;\\ \n"\
"    t = v2.z; v2.z = v2.w; v2.w += t;\\ \n"\
"    safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\\ \n"\
"    safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SCAN_EXCLUSIVE_4_V1(type)\\ \n"\
"    __attribute__((reqd_work_group_size(64, 1, 1)))\\ \n"\
"    __kernel void scan_exclusive_##type##4##_v1(__global type##4 const* in_array, __global type##4* out_array, uint numElems, __local type* shmem)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int localId   = get_local_id(0);\\ \n"\
"    int groupSize = get_local_size(0);\\ \n"\
"    type##4 v1 = safe_load_##type##4(in_array, 2*globalId, numElems);\\ \n"\
"    type##4 v2 = safe_load_##type##4(in_array, 2*globalId + 1, numElems);\\ \n"\
"    shmem[localId] = v1.x + v1.y + v1.z + v1.w + v2.x + v2.y + v2.z + v2.w;\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    group_scan_exclusive_##type(localId, groupSize, shmem);\\ \n"\
"    type offset = shmem[localId];\\ \n"\
"    type t = v1.x; v1.x = offset; offset += t;\\ \n"\
"    t = v1.y; v1.y = offset; offset += t;\\ \n"\
"    t = v1.z; v1.z = offset; offset += t;\\ \n"\
"    t = v1.w; v1.w = offset; offset += t;\\ \n"\
"    t = v2.x; v2.x = offset; offset += t;\\ \n"\
"    t = v2.y; v2.y = offset; offset += t;\\ \n"\
"    t = v2.z; v2.z = offset; offset += t;\\ \n"\
"    v2.w = offset;\\ \n"\
"    safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\\ \n"\
"    safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SCAN_EXCLUSIVE_PART_4(type)\\ \n"\
"    __attribute__((reqd_work_group_size(64, 1, 1)))\\ \n"\
"    __kernel void scan_exclusive_part_##type##4(__global type##4 const* in_array, __global type##4* out_array, uint numElems, __global type* out_sums, __local type* shmem)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int localId   = get_local_id(0);\\ \n"\
"    int groupId   = get_group_id(0);\\ \n"\
"    int groupSize = get_local_size(0);\\ \n"\
"    type##4 v1 = safe_load_##type##4(in_array, 2*globalId, numElems);\\ \n"\
"    type##4 v2 = safe_load_##type##4(in_array, 2*globalId + 1, numElems);\\ \n"\
"    v1.y += v1.x; v1.w += v1.z; v1.w += v1.y;\\ \n"\
"    v2.y += v2.x; v2.w += v2.z; v2.w += v2.y;\\ \n"\
"    v2.w += v1.w;\\ \n"\
"    shmem[localId] = v2.w;\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    type sum = group_scan_exclusive_part_##type(localId, groupSize, shmem);\\ \n"\
"    if (localId == 0) out_sums[groupId] = sum;\\ \n"\
"    v2.w = shmem[localId];\\ \n"\
"    type t = v1.w; v1.w = v2.w; v2.w += t;\\ \n"\
"    t = v1.y; v1.y = v1.w; v1.w += t;\\ \n"\
"    t = v2.y; v2.y = v2.w; v2.w += t;\\ \n"\
"    t = v1.x; v1.x = v1.y; v1.y += t;\\ \n"\
"    t = v2.x; v2.x = v2.y; v2.y += t;\\ \n"\
"    t = v1.z; v1.z = v1.w; v1.w += t;\\ \n"\
"    t = v2.z; v2.z = v2.w; v2.w += t;\\ \n"\
"    safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\\ \n"\
"    safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_GROUP_REDUCE(type)\\ \n"\
"    void group_reduce_##type(int localId, int groupSize, __local type* shmem)\\ \n"\
"{\\ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_DISTRIBUTE_PART_SUM_4(type)\\ \n"\
"    __kernel void distribute_part_sum_##type##4( __global type* in_sums, __global type##4* inout_array, uint numElems)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int groupId   = get_group_id(0);\\ \n"\
"    type##4 v1 = safe_load_##type##4(inout_array, globalId, numElems);\\ \n"\
"    type    sum = in_sums[groupId >> 1];\\ \n"\
"    v1.xyzw += sum;\\ \n"\
"    safe_store_##type##4(v1, inout_array, globalId, numElems);\\ \n"\
"} \n"\
" \n"\
" \n"\
"// These are already defined in Apple OCL runtime \n"\
"#ifndef APPLE \n"\
"DEFINE_MAKE_4(int) \n"\
"DEFINE_MAKE_4(float) \n"\
"#endif \n"\
" \n"\
"DEFINE_SAFE_LOAD_4(int) \n"\
"DEFINE_SAFE_LOAD_4(float) \n"\
" \n"\
"DEFINE_SAFE_STORE_4(int) \n"\
"DEFINE_SAFE_STORE_4(float) \n"\
" \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE(int) \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE(uint) \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE(float) \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE(short) \n"\
" \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE_SUM(uint) \n"\
" \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE_PART(int) \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE_PART(float) \n"\
" \n"\
"DEFINE_SCAN_EXCLUSIVE(int) \n"\
"DEFINE_SCAN_EXCLUSIVE(float) \n"\
" \n"\
"DEFINE_SCAN_EXCLUSIVE_4(int) \n"\
"DEFINE_SCAN_EXCLUSIVE_4(float) \n"\
" \n"\
"DEFINE_SCAN_EXCLUSIVE_PART_4(int) \n"\
"DEFINE_SCAN_EXCLUSIVE_PART_4(float) \n"\
" \n"\
"DEFINE_DISTRIBUTE_PART_SUM_4(int) \n"\
"DEFINE_DISTRIBUTE_PART_SUM_4(float) \n"\
" \n"\
"/// Specific function for radix-sort needs \n"\
"/// Group exclusive add multiscan on 4 arrays of shorts in parallel \n"\
"/// with 4x reduction in registers \n"\
"void group_scan_short_4way(int localId, int groupSize, \n"\
"    short4 mask0, \n"\
"    short4 mask1, \n"\
"    short4 mask2, \n"\
"    short4 mask3, \n"\
"    __local short* shmem0, \n"\
"    __local short* shmem1, \n"\
"    __local short* shmem2, \n"\
"    __local short* shmem3, \n"\
"    short4* offset0, \n"\
"    short4* offset1, \n"\
"    short4* offset2, \n"\
"    short4* offset3, \n"\
"    short4* histogram) \n"\
"{ \n"\
"    short4 v1 = mask0; \n"\
"    v1.y += v1.x; v1.w += v1.z; v1.w += v1.y; \n"\
"    shmem0[localId] = v1.w; \n"\
" \n"\
"    short4 v2 = mask1; \n"\
"    v2.y += v2.x; v2.w += v2.z; v2.w += v2.y; \n"\
"    shmem1[localId] = v2.w; \n"\
" \n"\
"    short4 v3 = mask2; \n"\
"    v3.y += v3.x; v3.w += v3.z; v3.w += v3.y; \n"\
"    shmem2[localId] = v3.w; \n"\
" \n"\
"    short4 v4 = mask3; \n"\
"    v4.y += v4.x; v4.w += v4.z; v4.w += v4.y; \n"\
"    shmem3[localId] = v4.w; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            shmem0[2 * (localId + 1)*stride - 1] = shmem0[2 * (localId + 1)*stride - 1] + shmem0[(2 * localId + 1)*stride - 1]; \n"\
"            shmem1[2 * (localId + 1)*stride - 1] = shmem1[2 * (localId + 1)*stride - 1] + shmem1[(2 * localId + 1)*stride - 1]; \n"\
"            shmem2[2 * (localId + 1)*stride - 1] = shmem2[2 * (localId + 1)*stride - 1] + shmem2[(2 * localId + 1)*stride - 1]; \n"\
"            shmem3[2 * (localId + 1)*stride - 1] = shmem3[2 * (localId + 1)*stride - 1] + shmem3[(2 * localId + 1)*stride - 1]; \n"\
"        } \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    short4 total; \n"\
"    total.s0 = shmem0[groupSize - 1]; \n"\
"    total.s1 = shmem1[groupSize - 1]; \n"\
"    total.s2 = shmem2[groupSize - 1]; \n"\
"    total.s3 = shmem3[groupSize - 1]; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        shmem0[groupSize - 1] = 0; \n"\
"        shmem1[groupSize - 1] = 0; \n"\
"        shmem2[groupSize - 1] = 0; \n"\
"        shmem3[groupSize - 1] = 0; \n"\
"    } \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem0[(2 * localId + 1)*stride - 1]; \n"\
"            shmem0[(2 * localId + 1)*stride - 1] = shmem0[2 * (localId + 1)*stride - 1]; \n"\
"            shmem0[2 * (localId + 1)*stride - 1] = shmem0[2 * (localId + 1)*stride - 1] + temp; \n"\
" \n"\
"            temp = shmem1[(2 * localId + 1)*stride - 1]; \n"\
"            shmem1[(2 * localId + 1)*stride - 1] = shmem1[2 * (localId + 1)*stride - 1]; \n"\
"            shmem1[2 * (localId + 1)*stride - 1] = shmem1[2 * (localId + 1)*stride - 1] + temp; \n"\
" \n"\
"            temp = shmem2[(2 * localId + 1)*stride - 1]; \n"\
"            shmem2[(2 * localId + 1)*stride - 1] = shmem2[2 * (localId + 1)*stride - 1]; \n"\
"            shmem2[2 * (localId + 1)*stride - 1] = shmem2[2 * (localId + 1)*stride - 1] + temp; \n"\
" \n"\
"            temp = shmem3[(2 * localId + 1)*stride - 1]; \n"\
"            shmem3[(2 * localId + 1)*stride - 1] = shmem3[2 * (localId + 1)*stride - 1]; \n"\
"            shmem3[2 * (localId + 1)*stride - 1] = shmem3[2 * (localId + 1)*stride - 1] + temp; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    v1.w = shmem0[localId]; \n"\
" \n"\
"    short t = v1.y; v1.y = v1.w; v1.w += t; \n"\
"    t = v1.x; v1.x = v1.y; v1.y += t; \n"\
"    t = v1.z; v1.z = v1.w; v1.w += t; \n"\
"    *offset0 = v1; \n"\
" \n"\
"    v2.w = shmem1[localId]; \n"\
" \n"\
"    t = v2.y; v2.y = v2.w; v2.w += t; \n"\
"    t = v2.x; v2.x = v2.y; v2.y += t; \n"\
"    t = v2.z; v2.z = v2.w; v2.w += t; \n"\
"    *offset1 = v2; \n"\
" \n"\
"    v3.w = shmem2[localId]; \n"\
" \n"\
"    t = v3.y; v3.y = v3.w; v3.w += t; \n"\
"    t = v3.x; v3.x = v3.y; v3.y += t; \n"\
"    t = v3.z; v3.z = v3.w; v3.w += t; \n"\
"    *offset2 = v3; \n"\
" \n"\
"    v4.w = shmem3[localId]; \n"\
" \n"\
"    t = v4.y; v4.y = v4.w; v4.w += t; \n"\
"    t = v4.x; v4.x = v4.y; v4.y += t; \n"\
"    t = v4.z; v4.z = v4.w; v4.w += t; \n"\
"    *offset3 = v4; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    *histogram = total; \n"\
"} \n"\
" \n"\
"// Calculate bool radix mask \n"\
"short4 radix_mask(int offset, uchar digit, int4 val) \n"\
"{ \n"\
"    short4 res; \n"\
"    res.x = ((val.x >> offset) & 3) == digit ? 1 : 0; \n"\
"    res.y = ((val.y >> offset) & 3) == digit ? 1 : 0; \n"\
"    res.z = ((val.z >> offset) & 3) == digit ? 1 : 0; \n"\
"    res.w = ((val.w >> offset) & 3) == digit ? 1 : 0; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"// Choose offset based on radix mask value  \n"\
"short offset_4way(int val, int offset, short offset0, short offset1, short offset2, short offset3, short4 hist) \n"\
"{ \n"\
"    switch ((val >> offset) & 3) \n"\
"    { \n"\
"    case 0: \n"\
"        return offset0; \n"\
"    case 1: \n"\
"        return offset1 + hist.x; \n"\
"    case 2: \n"\
"        return offset2 + hist.x + hist.y; \n"\
"    case 3: \n"\
"        return offset3 + hist.x + hist.y + hist.z; \n"\
"    } \n"\
" \n"\
"    return 0; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"// Perform group split using 2-bits pass \n"\
"void group_split_radix_2bits( \n"\
"    int localId, \n"\
"    int groupSize, \n"\
"    int offset, \n"\
"    int4 val, \n"\
"    __local short* shmem, \n"\
"    int4* localOffset, \n"\
"    short4* histogram) \n"\
"{ \n"\
"    /// Pointers to radix flag arrays \n"\
"    __local short* shmem0 = shmem; \n"\
"    __local short* shmem1 = shmem0 + groupSize; \n"\
"    __local short* shmem2 = shmem1 + groupSize; \n"\
"    __local short* shmem3 = shmem2 + groupSize; \n"\
" \n"\
"    /// Radix masks for each digit \n"\
"    short4 mask0 = radix_mask(offset, 0, val); \n"\
"    short4 mask1 = radix_mask(offset, 1, val); \n"\
"    short4 mask2 = radix_mask(offset, 2, val); \n"\
"    short4 mask3 = radix_mask(offset, 3, val); \n"\
" \n"\
"    /// Resulting offsets \n"\
"    short4 offset0; \n"\
"    short4 offset1; \n"\
"    short4 offset2; \n"\
"    short4 offset3; \n"\
" \n"\
"    group_scan_short_4way(localId, groupSize, \n"\
"        mask0, mask1, mask2, mask3, \n"\
"        shmem0, shmem1, shmem2, shmem3, \n"\
"        &offset0, &offset1, &offset2, &offset3, \n"\
"        histogram); \n"\
" \n"\
"    (*localOffset).x = offset_4way(val.x, offset, offset0.x, offset1.x, offset2.x, offset3.x, *histogram); \n"\
"    (*localOffset).y = offset_4way(val.y, offset, offset0.y, offset1.y, offset2.y, offset3.y, *histogram); \n"\
"    (*localOffset).z = offset_4way(val.z, offset, offset0.z, offset1.z, offset2.z, offset3.z, *histogram); \n"\
"    (*localOffset).w = offset_4way(val.w, offset, offset0.w, offset1.w, offset2.w, offset3.w, *histogram); \n"\
"} \n"\
" \n"\
"int4 safe_load_int4_intmax(__global int4* source, uint idx, uint sizeInInts) \n"\
"{ \n"\
"    int4 res = make_int4(INT_MAX, INT_MAX, INT_MAX, INT_MAX); \n"\
"    if (((idx + 1) << 2) <= sizeInInts) \n"\
"        res = source[idx]; \n"\
"    else \n"\
"    { \n"\
"        if ((idx << 2) < sizeInInts) res.x = source[idx].x; \n"\
"        if ((idx << 2) + 1 < sizeInInts) res.y = source[idx].y; \n"\
"        if ((idx << 2) + 2 < sizeInInts) res.z = source[idx].z; \n"\
"    } \n"\
"    return res; \n"\
"} \n"\
" \n"\
"void safe_store_int(int val, __global int* dest, uint idx, uint sizeInInts) \n"\
"{ \n"\
"    if (idx < sizeInInts) \n"\
"        dest[idx] = val; \n"\
"} \n"\
" \n"\
"// Split kernel launcher \n"\
"__kernel void split4way(int bitshift, __global int4* in_array, uint numElems, __global int* out_histograms, __global int4* out_array, \n"\
"    __global int* out_local_histograms, \n"\
"    __global int4* out_debug_offset, \n"\
"    __local short* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
"    int numGroups = get_global_size(0) / groupSize; \n"\
" \n"\
"    /// Load single int4 value \n"\
"    int4 val = safe_load_int4_intmax(in_array, globalId, numElems); \n"\
" \n"\
"    int4 localOffset; \n"\
"    short4 localHistogram; \n"\
"    group_split_radix_2bits(localId, groupSize, bitshift, val, shmem, &localOffset, \n"\
"        &localHistogram); \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    __local int* sharedData = (__local int*)shmem; \n"\
"    __local int4* sharedData4 = (__local int4*)shmem; \n"\
" \n"\
"    sharedData[localOffset.x] = val.x; \n"\
"    sharedData[localOffset.y] = val.y; \n"\
"    sharedData[localOffset.z] = val.z; \n"\
"    sharedData[localOffset.w] = val.w; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    // Now store to memory \n"\
"    if (((globalId + 1) << 2) <= numElems) \n"\
"    { \n"\
"        out_array[globalId] = sharedData4[localId]; \n"\
"        out_debug_offset[globalId] = localOffset; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        if ((globalId << 2) < numElems) out_array[globalId].x = sharedData4[localId].x; \n"\
"        if ((globalId << 2) + 1 < numElems) out_array[globalId].y = sharedData4[localId].y; \n"\
"        if ((globalId << 2) + 2 < numElems) out_array[globalId].z = sharedData4[localId].z; \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        out_histograms[groupId] = localHistogram.x; \n"\
"        out_histograms[groupId + numGroups] = localHistogram.y; \n"\
"        out_histograms[groupId + 2 * numGroups] = localHistogram.z; \n"\
"        out_histograms[groupId + 3 * numGroups] = localHistogram.w; \n"\
" \n"\
"        out_local_histograms[groupId] = 0; \n"\
"        out_local_histograms[groupId + numGroups] = localHistogram.x; \n"\
"        out_local_histograms[groupId + 2 * numGroups] = localHistogram.x + localHistogram.y; \n"\
"        out_local_histograms[groupId + 3 * numGroups] = localHistogram.x + localHistogram.y + localHistogram.z; \n"\
"    } \n"\
"} \n"\
" \n"\
"#define GROUP_SIZE 64 \n"\
"#define NUMBER_OF_BLOCKS_PER_GROUP 8 \n"\
"#define NUM_BINS 16 \n"\
" \n"\
"// The kernel computes 16 bins histogram of the 256 input elements. \n"\
"// The bin is determined by (in_array[tid] >> bitshift) & 0xF \n"\
"__kernel \n"\
"__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1))) \n"\
"void BitHistogram( \n"\
"    // Number of bits to shift \n"\
"    int bitshift, \n"\
"    // Input array \n"\
"    __global int const* restrict in_array, \n"\
"    // Number of elements in input array \n"\
"    uint numelems, \n"\
"    // Output histograms in column layout \n"\
"    // [bin0_group0, bin0_group1, ... bin0_groupN, bin1_group0, bin1_group1, ... bin1_groupN, ...] \n"\
"    __global int* restrict out_histogram \n"\
"    ) \n"\
"{ \n"\
"    // Histogram storage \n"\
"    __local int histogram[NUM_BINS * GROUP_SIZE]; \n"\
" \n"\
"    int globalid = get_global_id(0); \n"\
"    int localid = get_local_id(0); \n"\
"    int groupsize = get_local_size(0); \n"\
"    int groupid = get_group_id(0); \n"\
"    int numgroups = get_global_size(0) / groupsize; \n"\
" \n"\
"    /// Clear local histogram \n"\
"    for (int i = 0; i < NUM_BINS; ++i) \n"\
"    { \n"\
"        histogram[i*GROUP_SIZE + localid] = 0; \n"\
"    } \n"\
" \n"\
"    // Make sure everything is up to date \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    const int numblocks_per_group = NUMBER_OF_BLOCKS_PER_GROUP; \n"\
"    const int numelems_per_group = numblocks_per_group * GROUP_SIZE; \n"\
" \n"\
"    int numblocks_total = (numelems + GROUP_SIZE * 4 - 1) / (GROUP_SIZE * 4); \n"\
"    int maxblocks = numblocks_total - groupid * numblocks_per_group; \n"\
" \n"\
"    int loadidx = groupid * numelems_per_group + localid; \n"\
"    for (int block = 0; block < min(numblocks_per_group, maxblocks); ++block, loadidx += GROUP_SIZE) \n"\
"    { \n"\
"        /// Load single int4 value \n"\
"        int4 value = safe_load_int4_intmax(in_array, loadidx, numelems); \n"\
" \n"\
"        /// Handle value adding histogram bins \n"\
"        /// for all 4 elements \n"\
"        int4 bin = ((value >> bitshift) & 0xF); \n"\
"        //++histogram[localid*kNumBins + bin]; \n"\
"        atom_inc(&histogram[bin.x*GROUP_SIZE + localid]); \n"\
"        //bin = ((value.y >> bitshift) & 0xF); \n"\
"        //++histogram[localid*kNumBins + bin]; \n"\
"        atom_inc(&histogram[bin.y*GROUP_SIZE + localid]); \n"\
"        //bin = ((value.z >> bitshift) & 0xF); \n"\
"        //++histogram[localid*kNumBins + bin]; \n"\
"        atom_inc(&histogram[bin.z*GROUP_SIZE + localid]); \n"\
"        //bin = ((value.w >> bitshift) & 0xF); \n"\
"        //++histogram[localid*kNumBins + bin]; \n"\
"        atom_inc(&histogram[bin.w*GROUP_SIZE + localid]); \n"\
"    } \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    int sum = 0; \n"\
"    if (localid < NUM_BINS) \n"\
"    { \n"\
"        for (int i = 0; i < GROUP_SIZE; ++i) \n"\
"        { \n"\
"            sum += histogram[localid * GROUP_SIZE + i]; \n"\
"        } \n"\
" \n"\
"        out_histogram[numgroups*localid + groupid] = sum; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__kernel \n"\
"__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1))) \n"\
"void ScatterKeys(// Number of bits to shift \n"\
"    int bitshift, \n"\
"    // Input keys \n"\
"    __global int4 const* restrict in_keys, \n"\
"    // Number of input keys \n"\
"    uint           numelems, \n"\
"    // Scanned histograms \n"\
"    __global int const* restrict  in_histograms, \n"\
"    // Output keys \n"\
"    __global int* restrict  out_keys \n"\
"    ) \n"\
"{ \n"\
"    // Local memory for offsets counting \n"\
"    __local int  keys[GROUP_SIZE * 4]; \n"\
"    __local int  scanned_histogram[NUM_BINS]; \n"\
" \n"\
"    int globalid = get_global_id(0); \n"\
"    int localid = get_local_id(0); \n"\
"    int groupsize = get_local_size(0); \n"\
"    int groupid = get_group_id(0); \n"\
"    int numgroups = get_global_size(0) / groupsize; \n"\
" \n"\
"    __local uint* histogram = (__local uint*)keys; \n"\
" \n"\
"    int numblocks_per_group = NUMBER_OF_BLOCKS_PER_GROUP; \n"\
"    int numelems_per_group = numblocks_per_group * GROUP_SIZE; \n"\
"    int numblocks_total = (numelems + GROUP_SIZE * 4 - 1) / (GROUP_SIZE * 4); \n"\
"    int maxblocks = numblocks_total - groupid * numblocks_per_group; \n"\
" \n"\
"    // Copy scanned histogram for the group to local memory for fast indexing \n"\
"    if (localid < NUM_BINS) \n"\
"    { \n"\
"        scanned_histogram[localid] = in_histograms[groupid + localid * numgroups]; \n"\
"    } \n"\
" \n"\
"    // Make sure everything is up to date \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    int loadidx = groupid * numelems_per_group + localid; \n"\
"    for (int block = 0; block < min(numblocks_per_group, maxblocks); ++block, loadidx += GROUP_SIZE) \n"\
"    { \n"\
"        // Load single int4 value \n"\
"        int4 localvals = safe_load_int4_intmax(in_keys, loadidx, numelems); \n"\
" \n"\
"        // Clear the histogram \n"\
"        histogram[localid] = 0; \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Do 2 bits per pass \n"\
"        for (int bit = 0; bit <= 2; bit += 2) \n"\
"        { \n"\
"            // Count histogram \n"\
"            int4 b = ((localvals >> bitshift) >> bit) & 0x3; \n"\
" \n"\
"            int4 p; \n"\
"            p.x = 1 << (8 * b.x); \n"\
"            p.y = 1 << (8 * b.y); \n"\
"            p.z = 1 << (8 * b.z); \n"\
"            p.w = 1 << (8 * b.w); \n"\
" \n"\
"            // Pack the histogram \n"\
"            uint packed_key = (uint)(p.x + p.y + p.z + p.w); \n"\
" \n"\
"            // Put into LDS \n"\
"            histogram[localid] = packed_key; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Scan the histogram in LDS with 4-way plus scan \n"\
"            uint total = 0; \n"\
"            group_scan_exclusive_sum_uint(localid, GROUP_SIZE, histogram, &total); \n"\
" \n"\
"            // Load value back \n"\
"            packed_key = histogram[localid]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Scan total histogram (4 chars) \n"\
"            total = (total << 8) + (total << 16) + (total << 24); \n"\
"            uint offset = total + packed_key; \n"\
" \n"\
"            int4 newoffset; \n"\
" \n"\
"            int t = p.y + p.x; \n"\
"            p.w = p.z + t; \n"\
"            p.z = t; \n"\
"            p.y = p.x; \n"\
"            p.x = 0; \n"\
" \n"\
"            p += (int)offset; \n"\
"            newoffset = (p >> (b * 8)) & 0xFF; \n"\
" \n"\
"            keys[newoffset.x] = localvals.x; \n"\
"            keys[newoffset.y] = localvals.y; \n"\
"            keys[newoffset.z] = localvals.z; \n"\
"            keys[newoffset.w] = localvals.w; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Reload values back to registers for the second bit pass \n"\
"            localvals.x = keys[localid << 2]; \n"\
"            localvals.y = keys[(localid << 2) + 1]; \n"\
"            localvals.z = keys[(localid << 2) + 2]; \n"\
"            localvals.w = keys[(localid << 2) + 3]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
" \n"\
"        // Clear LDS \n"\
"        histogram[localid] = 0; \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Reconstruct 16 bins histogram \n"\
"        int4 bin = (localvals >> bitshift) & 0xF; \n"\
"        atom_inc(&histogram[bin.x]); \n"\
"        atom_inc(&histogram[bin.y]); \n"\
"        atom_inc(&histogram[bin.z]); \n"\
"        atom_inc(&histogram[bin.w]); \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        int sum = 0; \n"\
"        if (localid < NUM_BINS) \n"\
"        { \n"\
"            sum = histogram[localid]; \n"\
"        } \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Scan reconstructed histogram \n"\
"        group_scan_exclusive_uint(localid, 16, histogram); \n"\
" \n"\
"        // Put data back to global memory \n"\
"        int offset = scanned_histogram[bin.x] + (localid << 2) - histogram[bin.x]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localvals.x; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.y] + (localid << 2) + 1 - histogram[bin.y]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localvals.y; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.z] + (localid << 2) + 2 - histogram[bin.z]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localvals.z; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.w] + (localid << 2) + 3 - histogram[bin.w]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localvals.w; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        if (localid < NUM_BINS) \n"\
"        { \n"\
"            scanned_histogram[localid] += sum; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"__kernel \n"\
"__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1))) \n"\
"void ScatterKeysAndValues(// Number of bits to shift \n"\
"    int bitshift, \n"\
"    // Input keys \n"\
"    __global int4 const* restrict in_keys, \n"\
"    // Input values \n"\
"    __global int4 const* restrict in_values, \n"\
"    // Number of input keys \n"\
"    uint           numelems, \n"\
"    // Scanned histograms \n"\
"    __global int const* restrict  in_histograms, \n"\
"    // Output keys \n"\
"    __global int* restrict  out_keys, \n"\
"    // Output values \n"\
"    __global int* restrict  out_values \n"\
"    ) \n"\
"{ \n"\
"    // Local memory for offsets counting \n"\
"    __local int  keys[GROUP_SIZE * 4]; \n"\
"    __local int  scanned_histogram[NUM_BINS]; \n"\
" \n"\
"    int globalid = get_global_id(0); \n"\
"    int localid = get_local_id(0); \n"\
"    int groupsize = get_local_size(0); \n"\
"    int groupid = get_group_id(0); \n"\
"    int numgroups = get_global_size(0) / groupsize; \n"\
" \n"\
"    __local uint* histogram = (__local uint*)keys; \n"\
" \n"\
"    int numblocks_per_group = NUMBER_OF_BLOCKS_PER_GROUP; \n"\
"    int numelems_per_group = numblocks_per_group * GROUP_SIZE; \n"\
"    int numblocks_total = (numelems + GROUP_SIZE * 4 - 1) / (GROUP_SIZE * 4); \n"\
"    int maxblocks = numblocks_total - groupid * numblocks_per_group; \n"\
" \n"\
"    // Copy scanned histogram for the group to local memory for fast indexing \n"\
"    if (localid < NUM_BINS) \n"\
"    { \n"\
"        scanned_histogram[localid] = in_histograms[groupid + localid * numgroups]; \n"\
"    } \n"\
" \n"\
"    // Make sure everything is up to date \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    int loadidx = groupid * numelems_per_group + localid; \n"\
"    for (int block = 0; block < min(numblocks_per_group, maxblocks); ++block, loadidx += GROUP_SIZE) \n"\
"    { \n"\
"        // Load single int4 value \n"\
"        int4 localkeys = safe_load_int4_intmax(in_keys, loadidx, numelems); \n"\
"        int4 localvals = safe_load_int4_intmax(in_values, loadidx, numelems); \n"\
" \n"\
"        // Clear the histogram \n"\
"        histogram[localid] = 0; \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Do 2 bits per pass \n"\
"        for (int bit = 0; bit <= 2; bit += 2) \n"\
"        { \n"\
"            // Count histogram \n"\
"            int4 b = ((localkeys >> bitshift) >> bit) & 0x3; \n"\
" \n"\
"            int4 p; \n"\
"            p.x = 1 << (8 * b.x); \n"\
"            p.y = 1 << (8 * b.y); \n"\
"            p.z = 1 << (8 * b.z); \n"\
"            p.w = 1 << (8 * b.w); \n"\
" \n"\
"            // Pack the histogram \n"\
"            uint packed_key = (uint)(p.x + p.y + p.z + p.w); \n"\
" \n"\
"            // Put into LDS \n"\
"            histogram[localid] = packed_key; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Scan the histogram in LDS with 4-way plus scan \n"\
"            uint total = 0; \n"\
"            group_scan_exclusive_sum_uint(localid, GROUP_SIZE, histogram, &total); \n"\
" \n"\
"            // Load value back \n"\
"            packed_key = histogram[localid]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Scan total histogram (4 chars) \n"\
"            total = (total << 8) + (total << 16) + (total << 24); \n"\
"            uint offset = total + packed_key; \n"\
" \n"\
"            int4 newoffset; \n"\
" \n"\
"            int t = p.y + p.x; \n"\
"            p.w = p.z + t; \n"\
"            p.z = t; \n"\
"            p.y = p.x; \n"\
"            p.x = 0; \n"\
" \n"\
"            p += (int)offset; \n"\
"            newoffset = (p >> (b * 8)) & 0xFF; \n"\
" \n"\
"            keys[newoffset.x] = localkeys.x; \n"\
"            keys[newoffset.y] = localkeys.y; \n"\
"            keys[newoffset.z] = localkeys.z; \n"\
"            keys[newoffset.w] = localkeys.w; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Reload values back to registers for the second bit pass \n"\
"            localkeys = ((__local int4*)keys)[localid]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            keys[newoffset.x] = localvals.x; \n"\
"            keys[newoffset.y] = localvals.y; \n"\
"            keys[newoffset.z] = localvals.z; \n"\
"            keys[newoffset.w] = localvals.w; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Reload values back to registers for the second bit pass \n"\
"            localvals = ((__local int4*)keys)[localid]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
" \n"\
"        // Clear LDS \n"\
"        histogram[localid] = 0; \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Reconstruct 16 bins histogram \n"\
"        int4 bin = (localkeys >> bitshift) & 0xF; \n"\
"        atom_inc(&histogram[bin.x]); \n"\
"        atom_inc(&histogram[bin.y]); \n"\
"        atom_inc(&histogram[bin.z]); \n"\
"        atom_inc(&histogram[bin.w]); \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        int sum = 0; \n"\
"        if (localid < NUM_BINS) \n"\
"        { \n"\
"            sum = histogram[localid]; \n"\
"        } \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Scan reconstructed histogram \n"\
"        group_scan_exclusive_uint(localid, 16, histogram); \n"\
" \n"\
"        // Put data back to global memory \n"\
"        int offset = scanned_histogram[bin.x] + (localid << 2) - histogram[bin.x]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localkeys.x; \n"\
"            out_values[offset] = localvals.x; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.y] + (localid << 2) + 1 - histogram[bin.y]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localkeys.y; \n"\
"            out_values[offset] = localvals.y; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.z] + (localid << 2) + 2 - histogram[bin.z]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localkeys.z; \n"\
"            out_values[offset] = localvals.z; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.w] + (localid << 2) + 3 - histogram[bin.w]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localkeys.w; \n"\
"            out_values[offset] = localvals.w; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        scanned_histogram[localid] += sum; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__kernel void compact_int(__global int* in_predicate, __global int* in_address, \n"\
"    __global int* in_input, uint in_size, \n"\
"    __global int* out_output) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    if (global_id < in_size) \n"\
"    { \n"\
"        if (in_predicate[global_id]) \n"\
"        { \n"\
"            out_output[in_address[global_id]] = in_input[global_id]; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__kernel void compact_int_1(__global int* in_predicate, __global int* in_address, \n"\
"    __global int* in_input, uint in_size, \n"\
"    __global int* out_output, \n"\
"    __global int* out_size) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    if (global_id < in_size) \n"\
"    { \n"\
"        if (in_predicate[global_id]) \n"\
"        { \n"\
"            out_output[in_address[global_id]] = in_input[global_id]; \n"\
"        } \n"\
"    } \n"\
" \n"\
"    if (global_id == 0) \n"\
"    { \n"\
"        *out_size = in_address[in_size - 1] + in_predicate[in_size - 1]; \n"\
"    } \n"\
"} \n"\
" \n"\
"__kernel void copy(__global int4* in_input, \n"\
"    uint  in_size, \n"\
"    __global int4* out_output) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int4 value = safe_load_int4(in_input, global_id, in_size); \n"\
"    safe_store_int4(value, out_output, global_id, in_size); \n"\
"} \n"\
" \n"\
" \n"\
"#define FLAG(x) (flags[(x)] & 0x1) \n"\
"#define FLAG_COMBINED(x) (flags[(x)]) \n"\
"#define FLAG_ORIG(x) ((flags[(x)] >> 1) & 0x1) \n"\
" \n"\
"void group_segmented_scan_exclusive_int( \n"\
"    int localId, \n"\
"    int groupSize, \n"\
"    __local int* shmem, \n"\
"    __local char* flags \n"\
"    ) \n"\
"{ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            if (FLAG(2 * (localId + 1)*stride - 1) == 0) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + shmem[(2 * localId + 1)*stride - 1]; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED(2 * (localId + 1)*stride - 1) = FLAG_COMBINED(2 * (localId + 1)*stride - 1) | FLAG((2 * localId + 1)*stride - 1); \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"        shmem[groupSize - 1] = 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem[(2 * localId + 1)*stride - 1]; \n"\
"            shmem[(2 * localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1]; \n"\
" \n"\
"            // optimize with a conditional = operator \n"\
"            if (FLAG_ORIG((2 * localId + 1)*stride) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = 0; \n"\
"            } \n"\
"            else if (FLAG((2 * localId + 1)*stride - 1) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = temp; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + temp; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED((2 * localId + 1)*stride - 1) = FLAG_COMBINED((2 * localId + 1)*stride - 1) & 2; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
"void group_segmented_scan_exclusive_int_nocut( \n"\
"    int localId, \n"\
"    int groupSize, \n"\
"    __local int* shmem, \n"\
"    __local char* flags \n"\
"    ) \n"\
"{ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            if (FLAG(2 * (localId + 1)*stride - 1) == 0) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + shmem[(2 * localId + 1)*stride - 1]; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED(2 * (localId + 1)*stride - 1) = FLAG_COMBINED(2 * (localId + 1)*stride - 1) | FLAG((2 * localId + 1)*stride - 1); \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"        shmem[groupSize - 1] = 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem[(2 * localId + 1)*stride - 1]; \n"\
"            shmem[(2 * localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1]; \n"\
" \n"\
"            if (FLAG((2 * localId + 1)*stride - 1) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = temp; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + temp; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED((2 * localId + 1)*stride - 1) = FLAG_COMBINED((2 * localId + 1)*stride - 1) & 2; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"void group_segmented_scan_exclusive_int_part( \n"\
"    int localId, \n"\
"    int groupId, \n"\
"    int groupSize, \n"\
"    __local int* shmem, \n"\
"    __local char* flags, \n"\
"    __global int* part_sums, \n"\
"    __global int* part_flags \n"\
"    ) \n"\
"{ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            if (FLAG(2 * (localId + 1)*stride - 1) == 0) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + shmem[(2 * localId + 1)*stride - 1]; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED(2 * (localId + 1)*stride - 1) = FLAG_COMBINED(2 * (localId + 1)*stride - 1) | FLAG((2 * localId + 1)*stride - 1); \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        part_sums[groupId] = shmem[groupSize - 1]; \n"\
"        part_flags[groupId] = FLAG(groupSize - 1); \n"\
"        shmem[groupSize - 1] = 0; \n"\
"    } \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem[(2 * localId + 1)*stride - 1]; \n"\
"            shmem[(2 * localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1]; \n"\
" \n"\
"            // optimize with a conditional = operator \n"\
"            if (FLAG_ORIG((2 * localId + 1)*stride) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = 0; \n"\
"            } \n"\
"            else if (FLAG((2 * localId + 1)*stride - 1) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = temp; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + temp; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED((2 * localId + 1)*stride - 1) = FLAG_COMBINED((2 * localId + 1)*stride - 1) & 2; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
"void group_segmented_scan_exclusive_int_nocut_part( \n"\
"    int localId, \n"\
"    int groupId, \n"\
"    int groupSize, \n"\
"    __local int* shmem, \n"\
"    __local char* flags, \n"\
"    __global int* part_sums, \n"\
"    __global int* part_flags \n"\
"    ) \n"\
"{ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            if (FLAG(2 * (localId + 1)*stride - 1) == 0) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + shmem[(2 * localId + 1)*stride - 1]; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED(2 * (localId + 1)*stride - 1) = FLAG_COMBINED(2 * (localId + 1)*stride - 1) | FLAG((2 * localId + 1)*stride - 1); \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        part_sums[groupId] = shmem[groupSize - 1]; \n"\
"        part_flags[groupId] = FLAG(groupSize - 1); \n"\
"        shmem[groupSize - 1] = 0; \n"\
"    } \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem[(2 * localId + 1)*stride - 1]; \n"\
"            shmem[(2 * localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1]; \n"\
" \n"\
"            if (FLAG((2 * localId + 1)*stride - 1) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = temp; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + temp; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED((2 * localId + 1)*stride - 1) = FLAG_COMBINED((2 * localId + 1)*stride - 1) & 2; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__kernel void segmented_scan_exclusive_int_nocut(__global int const* in_array, \n"\
"    __global int const* in_segment_heads_array, \n"\
"    int numelems, \n"\
"    __global int* out_array, \n"\
"    __local int* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    __local int* keys = shmem; \n"\
"    __local char* flags = (__local char*)(keys + groupSize); \n"\
" \n"\
"    keys[localId] = globalId < numelems ? in_array[globalId] : 0; \n"\
"    flags[localId] = globalId < numelems ? (in_segment_heads_array[globalId] ? 3 : 0) : 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_segmented_scan_exclusive_int_nocut(localId, groupSize, keys, flags); \n"\
" \n"\
"    out_array[globalId] = keys[localId]; \n"\
"} \n"\
" \n"\
"__kernel void segmented_scan_exclusive_int(__global int const* in_array, \n"\
"    __global int const* in_segment_heads_array, \n"\
"    int numelems, \n"\
"    __global int* out_array, \n"\
"    __local int* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    __local int* keys = shmem; \n"\
"    __local char* flags = (__local char*)(keys + groupSize); \n"\
" \n"\
"    keys[localId] = globalId < numelems ? in_array[globalId] : 0; \n"\
"    flags[localId] = globalId < numelems ? (in_segment_heads_array[globalId] ? 3 : 0) : 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_segmented_scan_exclusive_int(localId, groupSize, keys, flags); \n"\
" \n"\
"    out_array[globalId] = keys[localId]; \n"\
"} \n"\
" \n"\
"__kernel void segmented_scan_exclusive_int_part(__global int const* in_array, \n"\
"    __global int const* in_segment_heads_array, \n"\
"    int numelems, \n"\
"    __global int* out_array, \n"\
"    __global int* out_part_sums, \n"\
"    __global int* out_part_flags, \n"\
"    __local int* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    __local int* keys = shmem; \n"\
"    __local char* flags = (__local char*)(keys + groupSize); \n"\
" \n"\
"    keys[localId] = globalId < numelems ? in_array[globalId] : 0; \n"\
"    flags[localId] = globalId < numelems ? (in_segment_heads_array[globalId] ? 3 : 0) : 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_segmented_scan_exclusive_int_part(localId, groupId, groupSize, keys, flags, out_part_sums, out_part_flags); \n"\
" \n"\
"    out_array[globalId] = keys[localId]; \n"\
"} \n"\
" \n"\
"__kernel void segmented_scan_exclusive_int_nocut_part(__global int const* in_array, \n"\
"    __global int const* in_segment_heads_array, \n"\
"    int numelems, \n"\
"    __global int* out_array, \n"\
"    __global int* out_part_sums, \n"\
"    __global int* out_part_flags, \n"\
"    __local int* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    __local int* keys = shmem; \n"\
"    __local char* flags = (__local char*)(keys + groupSize); \n"\
" \n"\
"    keys[localId] = globalId < numelems ? in_array[globalId] : 0; \n"\
"    flags[localId] = globalId < numelems ? (in_segment_heads_array[globalId] ? 3 : 0) : 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_segmented_scan_exclusive_int_nocut_part(localId, groupId, groupSize, keys, flags, out_part_sums, out_part_flags); \n"\
" \n"\
"    out_array[globalId] = keys[localId]; \n"\
"} \n"\
" \n"\
" \n"\
"__kernel void segmented_distribute_part_sum_int( \n"\
"    __global int* inout_array, \n"\
"    __global int* in_flags, \n"\
"    int numelems, \n"\
"    __global int* in_sums \n"\
"    ) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    int sum = in_sums[groupId]; \n"\
"    //inout_array[globalId] += sum; \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        for (int i = 0; in_flags[globalId + i] == 0 && i < groupSize; ++i) \n"\
"        { \n"\
"            if (globalId + i < numelems) \n"\
"            { \n"\
"                inout_array[globalId + i] += sum; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__kernel void segmented_distribute_part_sum_int_nocut( \n"\
"    __global int* inout_array, \n"\
"    __global int* in_flags, \n"\
"    int numelems, \n"\
"    __global int* in_sums \n"\
"    ) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    int sum = in_sums[groupId]; \n"\
"    bool stop = false; \n"\
"    //inout_array[globalId] += sum; \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        for (int i = 0; i < groupSize; ++i) \n"\
"        { \n"\
"            if (globalId + i < numelems) \n"\
"            { \n"\
"                if (in_flags[globalId + i] == 0) \n"\
"                { \n"\
"                    inout_array[globalId + i] += sum; \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    if (stop) \n"\
"                    { \n"\
"                        break; \n"\
"                    } \n"\
"                    else \n"\
"                    { \n"\
"                        inout_array[globalId + i] += sum; \n"\
"                        stop = true; \n"\
"                    } \n"\
"                } \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
;
static const char g_common_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"typedef struct _bbox \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int2 padding; \n"\
"} ray; \n"\
" \n"\
"typedef struct _Intersection \n"\
"{ \n"\
"    int shapeid; \n"\
"    int primid; \n"\
"    int padding0; \n"\
"    int padding1; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
"typedef struct _ShapeData \n"\
"{ \n"\
"    int id; \n"\
"    int bvhidx; \n"\
"    int mask; \n"\
"    int padding1; \n"\
"    float4 m0; \n"\
"    float4 m1; \n"\
"    float4 m2; \n"\
"    float4 m3; \n"\
"    float4  linearvelocity; \n"\
"    float4  angularvelocity; \n"\
"} ShapeData; \n"\
" \n"\
"typedef bbox BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[3]; \n"\
"    int shapeidx; \n"\
"    // Primitive ID \n"\
"    int id; \n"\
"    // Idx count \n"\
"    int cnt; \n"\
" \n"\
"    int2 padding; \n"\
"} Face; \n"\
" \n"\
"#ifndef APPLE \n"\
" \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
"float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"ray transform_ray(ray r, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    ray res; \n"\
"    res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3); \n"\
"    res.o.w = r.o.w; \n"\
"    res.d.w = r.d.w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_mul(float4 q1, float4 q2) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x; \n"\
"    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y; \n"\
"    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z; \n"\
"    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_conjugate(float4 q) \n"\
"{ \n"\
"    return make_float4(-q.x, -q.y, -q.z, q.w); \n"\
"} \n"\
" \n"\
"float4 quaternion_inverse(float4 q) \n"\
"{ \n"\
"    float sqnorm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w; \n"\
"     \n"\
"    if (sqnorm != 0.f) \n"\
"    { \n"\
"        return quaternion_conjugate(q) / sqnorm; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"    } \n"\
"} \n"\
" \n"\
"void rotate_ray(ray* r, float4 q) \n"\
"{ \n"\
"    float4 qinv = quaternion_inverse(q); \n"\
"    float4 v = make_float4(r->o.x, r->o.y, r->o.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->o.xyz = v.xyz; \n"\
"    v = make_float4(r->d.x, r->d.y, r->d.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->d.xyz = v.xyz; \n"\
"} \n"\
" \n"\
"// Intersect Ray against triangle \n"\
"int IntersectTriangle(ray const* r, float3 v1, float3 v2, float3 v3, Intersection* isect) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > isect->uvwt.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        isect->uvwt = make_float4(b1, b2, 0.f, temp); \n"\
"        return 1; \n"\
"    } \n"\
"} \n"\
" \n"\
"int IntersectTriangleP(ray const* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > r->o.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"     \n"\
"    return 1; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"int IntersectBox(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? 1 : 0; \n"\
"} \n"\
" \n"\
"float IntersectBoxF(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.f; \n"\
"} \n"\
" \n"\
"int Ray_GetMask(ray const* r) \n"\
"{ \n"\
"	return r->extra.x; \n"\
"} \n"\
" \n"\
"int Ray_IsActive(ray const* r) \n"\
"{ \n"\
"	return r->extra.y; \n"\
"} \n"\
" \n"\
"float Ray_GetMaxT(ray const* r) \n"\
"{ \n"\
"	return r->o.w; \n"\
"} \n"\
" \n"\
"float Ray_GetTime(ray const* r) \n"\
"{ \n"\
"	return r->d.w; \n"\
"} \n"\
;
static const char g_fatbvh_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"INCLUDES \n"\
"**************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"typedef struct _bbox \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int2 padding; \n"\
"} ray; \n"\
" \n"\
"typedef struct _Intersection \n"\
"{ \n"\
"    int shapeid; \n"\
"    int primid; \n"\
"    int padding0; \n"\
"    int padding1; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
"typedef struct _ShapeData \n"\
"{ \n"\
"    int id; \n"\
"    int bvhidx; \n"\
"    int mask; \n"\
"    int padding1; \n"\
"    float4 m0; \n"\
"    float4 m1; \n"\
"    float4 m2; \n"\
"    float4 m3; \n"\
"    float4  linearvelocity; \n"\
"    float4  angularvelocity; \n"\
"} ShapeData; \n"\
" \n"\
"typedef bbox BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[3]; \n"\
"    int shapeidx; \n"\
"    // Primitive ID \n"\
"    int id; \n"\
"    // Idx count \n"\
"    int cnt; \n"\
" \n"\
"    int2 padding; \n"\
"} Face; \n"\
" \n"\
"#ifndef APPLE \n"\
" \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
"float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"ray transform_ray(ray r, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    ray res; \n"\
"    res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3); \n"\
"    res.o.w = r.o.w; \n"\
"    res.d.w = r.d.w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_mul(float4 q1, float4 q2) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x; \n"\
"    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y; \n"\
"    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z; \n"\
"    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_conjugate(float4 q) \n"\
"{ \n"\
"    return make_float4(-q.x, -q.y, -q.z, q.w); \n"\
"} \n"\
" \n"\
"float4 quaternion_inverse(float4 q) \n"\
"{ \n"\
"    float sqnorm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w; \n"\
"     \n"\
"    if (sqnorm != 0.f) \n"\
"    { \n"\
"        return quaternion_conjugate(q) / sqnorm; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"    } \n"\
"} \n"\
" \n"\
"void rotate_ray(ray* r, float4 q) \n"\
"{ \n"\
"    float4 qinv = quaternion_inverse(q); \n"\
"    float4 v = make_float4(r->o.x, r->o.y, r->o.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->o.xyz = v.xyz; \n"\
"    v = make_float4(r->d.x, r->d.y, r->d.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->d.xyz = v.xyz; \n"\
"} \n"\
" \n"\
"// Intersect Ray against triangle \n"\
"int IntersectTriangle(ray const* r, float3 v1, float3 v2, float3 v3, Intersection* isect) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > isect->uvwt.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        isect->uvwt = make_float4(b1, b2, 0.f, temp); \n"\
"        return 1; \n"\
"    } \n"\
"} \n"\
" \n"\
"int IntersectTriangleP(ray const* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > r->o.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"     \n"\
"    return 1; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"int IntersectBox(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? 1 : 0; \n"\
"} \n"\
" \n"\
"float IntersectBoxF(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.f; \n"\
"} \n"\
" \n"\
"int Ray_GetMask(ray const* r) \n"\
"{ \n"\
"	return r->extra.x; \n"\
"} \n"\
" \n"\
"int Ray_IsActive(ray const* r) \n"\
"{ \n"\
"	return r->extra.y; \n"\
"} \n"\
" \n"\
"float Ray_GetMaxT(ray const* r) \n"\
"{ \n"\
"	return r->o.w; \n"\
"} \n"\
" \n"\
"float Ray_GetTime(ray const* r) \n"\
"{ \n"\
"	return r->d.w; \n"\
"} \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
" \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
"#define STARTIDX(x)     (((int)((x).pmin.w))) \n"\
"#define LEAFNODE(x)     (((x).pmin.w) != -1.f) \n"\
"#define SHORT_STACK_SIZE 16 \n"\
" \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"	bbox lbound; \n"\
"	bbox rbound; \n"\
"} FatBvhNode; \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"	// BVH structure \n"\
"	__global FatBvhNode const* 	nodes; \n"\
"	// Scene positional data \n"\
"	__global float3 const* 		vertices; \n"\
"	// Scene indices \n"\
"	__global Face const* 		faces; \n"\
"	// Shape IDs \n"\
"	__global ShapeData const* 	shapes; \n"\
"	// Extra data \n"\
"	__global int const* 			extra; \n"\
"} SceneData; \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
" \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"BVH FUNCTIONS \n"\
"**************************************************************************/ \n"\
"//  intersect a ray with leaf BVH node \n"\
"void IntersectLeafClosest( \n"\
"	SceneData const* scenedata, \n"\
"	int faceidx, \n"\
"	ray const* r,                // ray to instersect \n"\
"	Intersection* isect          // Intersection structure \n"\
"	) \n"\
"{ \n"\
"	float3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	face = scenedata->faces[faceidx]; \n"\
"	v1 = scenedata->vertices[face.idx[0]]; \n"\
"	v2 = scenedata->vertices[face.idx[1]]; \n"\
"	v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"	int shapemask = scenedata->shapes[face.shapeidx].mask; \n"\
" \n"\
"	if (Ray_GetMask(r) & shapemask) \n"\
"	{ \n"\
"		if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"		{ \n"\
"			isect->primid = face.id; \n"\
"			isect->shapeid = scenedata->shapes[face.shapeidx].id; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafAny( \n"\
"	SceneData const* scenedata, \n"\
"	int faceidx, \n"\
"	ray const* r                      // ray to instersect \n"\
"	) \n"\
"{ \n"\
"	float3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	face = scenedata->faces[faceidx]; \n"\
"	v1 = scenedata->vertices[face.idx[0]]; \n"\
"	v2 = scenedata->vertices[face.idx[1]]; \n"\
"	v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"	int shapemask = scenedata->shapes[face.shapeidx].mask; \n"\
" \n"\
"	if (Ray_GetMask(r) & shapemask) \n"\
"	{ \n"\
"		if (IntersectTriangleP(r, v1, v2, v3)) \n"\
"		{ \n"\
"			return true; \n"\
"		} \n"\
"	} \n"\
" \n"\
"	return false; \n"\
"} \n"\
" \n"\
"#ifndef GLOBAL_STACK \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneClosest(SceneData const* scenedata, ray const* r, Intersection* isect, __global int* stack, __local int* ldsstack) \n"\
"{ \n"\
"	const float3 invdir = native_recip(r->d.xyz); \n"\
" \n"\
"	isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w); \n"\
"	isect->shapeid = -1; \n"\
"	isect->primid = -1; \n"\
" \n"\
"	if (r->o.w < 0.f) \n"\
"		return false; \n"\
" \n"\
"	__global int* gsptr = stack; \n"\
"	__local  int* lsptr = ldsstack; \n"\
" \n"\
"	*lsptr = -1; \n"\
"	lsptr += 64; \n"\
" \n"\
"	int idx = 0; \n"\
"	FatBvhNode node; \n"\
" \n"\
"	bool leftleaf = false; \n"\
"	bool rightleaf = false; \n"\
"	float lefthit = 0.f; \n"\
"	float righthit = 0.f; \n"\
"	int step = 0; \n"\
" \n"\
"	while (idx > -1) \n"\
"	{ \n"\
"		while (idx > -1) \n"\
"		{ \n"\
"			node = scenedata->nodes[idx]; \n"\
" \n"\
"			leftleaf = LEAFNODE(node.lbound); \n"\
"			rightleaf = LEAFNODE(node.rbound); \n"\
" \n"\
"			lefthit = leftleaf ? -1.f : IntersectBoxF(r, invdir, node.lbound, isect->uvwt.w); \n"\
"			righthit = rightleaf ? -1.f : IntersectBoxF(r, invdir, node.rbound, isect->uvwt.w); \n"\
" \n"\
"			if (leftleaf) \n"\
"			{ \n"\
"				IntersectLeafClosest(scenedata, STARTIDX(node.lbound), r, isect); \n"\
"			} \n"\
" \n"\
"			if (rightleaf) \n"\
"			{ \n"\
"				IntersectLeafClosest(scenedata, STARTIDX(node.rbound), r, isect); \n"\
"			} \n"\
" \n"\
"			if (lefthit > 0.f && righthit > 0.f) \n"\
"			{ \n"\
"				int deferred = -1; \n"\
"				if (lefthit > righthit) \n"\
"				{ \n"\
"					idx = (int)node.rbound.pmax.w; \n"\
"					deferred = (int)node.lbound.pmax.w;; \n"\
"				} \n"\
"				else \n"\
"				{ \n"\
"					idx = (int)node.lbound.pmax.w; \n"\
"					deferred = (int)node.rbound.pmax.w; \n"\
"				} \n"\
" \n"\
"				if (lsptr - ldsstack >= SHORT_STACK_SIZE * 64) \n"\
"				{ \n"\
"					for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"					{ \n"\
"						gsptr[i] = ldsstack[i * 64]; \n"\
"					} \n"\
" \n"\
"					gsptr += SHORT_STACK_SIZE; \n"\
"					lsptr = ldsstack + 64; \n"\
"				} \n"\
" \n"\
"				*lsptr = deferred; \n"\
"				lsptr += 64; \n"\
" \n"\
"				continue; \n"\
"			} \n"\
"			else if (lefthit > 0) \n"\
"			{ \n"\
"				idx = (int)node.lbound.pmax.w; \n"\
"				continue; \n"\
"			} \n"\
"			else if (righthit > 0) \n"\
"			{ \n"\
"				idx = (int)node.rbound.pmax.w; \n"\
"				continue; \n"\
"			} \n"\
" \n"\
"			lsptr -= 64; \n"\
"			idx = *(lsptr); \n"\
"		} \n"\
" \n"\
"		if (gsptr > stack) \n"\
"		{ \n"\
"			gsptr -= SHORT_STACK_SIZE; \n"\
" \n"\
"			for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"			{ \n"\
"				ldsstack[i * 64] = gsptr[i]; \n"\
"			} \n"\
" \n"\
"			lsptr = ldsstack + (SHORT_STACK_SIZE - 1) * 64; \n"\
"			idx = ldsstack[64 * (SHORT_STACK_SIZE - 1)]; \n"\
"		} \n"\
"	} \n"\
" \n"\
"	return isect->shapeid >= 0; \n"\
"} \n"\
"#else \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneClosest(SceneData const* scenedata, ray const* r, Intersection* isect) \n"\
"{ \n"\
"	const float3 invdir = native_recip(r->d.xyz); \n"\
" \n"\
"	isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w); \n"\
"	isect->shapeid = -1; \n"\
"	isect->primid = -1; \n"\
" \n"\
"	if (r->o.w < 0.f) \n"\
"		return false; \n"\
"     \n"\
"	int stack[32]; \n"\
" \n"\
"	int* sptr = stack; \n"\
"	*sptr++ = -1; \n"\
" \n"\
"	int idx = 0; \n"\
"	FatBvhNode node; \n"\
" \n"\
"	bool leftleaf = false; \n"\
"	bool rightleaf = false; \n"\
"	float lefthit = 0.f; \n"\
"	float righthit = 0.f; \n"\
"	int step = 0; \n"\
" \n"\
"	while (idx > -1) \n"\
"	{ \n"\
"		node = scenedata->nodes[idx]; \n"\
" \n"\
"		leftleaf = LEAFNODE(node.lbound); \n"\
"		rightleaf = LEAFNODE(node.rbound); \n"\
" \n"\
"		lefthit = leftleaf ? -1.f : IntersectBoxF(r, invdir, node.lbound, isect->uvwt.w); \n"\
"		righthit = rightleaf ? -1.f : IntersectBoxF(r, invdir, node.rbound, isect->uvwt.w); \n"\
" \n"\
"		if (leftleaf) \n"\
"		{ \n"\
"			IntersectLeafClosest(scenedata, STARTIDX(node.lbound), r, isect); \n"\
"		} \n"\
" \n"\
"		if (rightleaf) \n"\
"		{ \n"\
"			IntersectLeafClosest(scenedata, STARTIDX(node.rbound), r, isect); \n"\
"		} \n"\
" \n"\
"		if (lefthit > 0.f && righthit > 0.f) \n"\
"		{ \n"\
"			int deferred = -1; \n"\
"			if (lefthit > righthit) \n"\
"			{ \n"\
"				idx = (int)node.rbound.pmax.w; \n"\
"				deferred = (int)node.lbound.pmax.w;; \n"\
"			} \n"\
"			else \n"\
"			{ \n"\
"				idx = (int)node.lbound.pmax.w; \n"\
"				deferred = (int)node.rbound.pmax.w; \n"\
"			} \n"\
" \n"\
"		            *sptr++ = deferred; \n"\
"			continue; \n"\
"		} \n"\
"		else if (lefthit > 0) \n"\
"		{ \n"\
"			idx = (int)node.lbound.pmax.w; \n"\
"			continue; \n"\
"		} \n"\
"		else if (righthit > 0) \n"\
"		{ \n"\
"			idx = (int)node.rbound.pmax.w; \n"\
"			continue; \n"\
"		} \n"\
" \n"\
"        		idx = *--sptr; \n"\
"	} \n"\
" \n"\
"	return isect->shapeid >= 0; \n"\
"} \n"\
"#endif \n"\
" \n"\
"#ifndef GLOBAL_STACK \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny(SceneData const* scenedata, ray const* r, __global int* stack, __local int* ldsstack) \n"\
"{ \n"\
"	const float3 invdir = native_recip(r->d.xyz); \n"\
" \n"\
"	__global int* gsptr = stack; \n"\
"	__local  int* lsptr = ldsstack; \n"\
" \n"\
"	if (r->o.w < 0.f) \n"\
"		return false; \n"\
" \n"\
"	*lsptr = -1; \n"\
"	lsptr += 64; \n"\
" \n"\
"	int idx = 0; \n"\
"	FatBvhNode node; \n"\
" \n"\
"	bool leftleaf = false; \n"\
"	bool rightleaf = false; \n"\
"	float lefthit = 0.f; \n"\
"	float righthit = 0.f; \n"\
" \n"\
"	while (idx > -1) \n"\
"	{ \n"\
"		while (idx > -1) \n"\
"		{ \n"\
"			node = scenedata->nodes[idx]; \n"\
" \n"\
"			leftleaf = LEAFNODE(node.lbound); \n"\
"			rightleaf = LEAFNODE(node.rbound); \n"\
" \n"\
"			lefthit = leftleaf ? -1.f : IntersectBoxF(r, invdir, node.lbound, r->o.w); \n"\
"			righthit = rightleaf ? -1.f : IntersectBoxF(r, invdir, node.rbound, r->o.w); \n"\
" \n"\
"			if (leftleaf) \n"\
"			{ \n"\
"				if (IntersectLeafAny(scenedata, STARTIDX(node.lbound), r)) \n"\
"                    				return true; \n"\
"			} \n"\
" \n"\
"			if (rightleaf) \n"\
"			{ \n"\
"				if (IntersectLeafAny(scenedata, STARTIDX(node.rbound), r)) \n"\
"			                    return true; \n"\
"			} \n"\
" \n"\
"			if (lefthit > 0.f && righthit > 0.f) \n"\
"			{ \n"\
"				int deferred = -1; \n"\
"				if (lefthit > righthit) \n"\
"				{ \n"\
"					idx = (int)node.rbound.pmax.w; \n"\
"					deferred = (int)node.lbound.pmax.w;; \n"\
" \n"\
"				} \n"\
"				else \n"\
"				{ \n"\
"					idx = (int)node.lbound.pmax.w; \n"\
"					deferred = (int)node.rbound.pmax.w; \n"\
"				} \n"\
" \n"\
"				if (lsptr - ldsstack >= SHORT_STACK_SIZE * 64) \n"\
"				{ \n"\
"					for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"					{ \n"\
"						gsptr[i] = ldsstack[i * 64]; \n"\
"					} \n"\
" \n"\
"					gsptr += SHORT_STACK_SIZE; \n"\
"					lsptr = ldsstack + 64; \n"\
"				} \n"\
" \n"\
"				*lsptr = deferred; \n"\
"				lsptr += 64; \n"\
"				continue; \n"\
"			} \n"\
"			else if (lefthit > 0) \n"\
"			{ \n"\
"				idx = (int)node.lbound.pmax.w; \n"\
"				continue; \n"\
"			} \n"\
"			else if (righthit > 0) \n"\
"			{ \n"\
"				idx = (int)node.rbound.pmax.w; \n"\
"				continue; \n"\
"			} \n"\
" \n"\
"			lsptr -= 64; \n"\
"			idx = *(lsptr); \n"\
"		} \n"\
" \n"\
"		if (gsptr > stack) \n"\
"		{ \n"\
"			gsptr -= SHORT_STACK_SIZE; \n"\
" \n"\
"			for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"			{ \n"\
"				ldsstack[i * 64] = gsptr[i]; \n"\
"			} \n"\
" \n"\
"			lsptr = ldsstack + (SHORT_STACK_SIZE - 1) * 64; \n"\
"			idx = ldsstack[64 * (SHORT_STACK_SIZE - 1)]; \n"\
"		} \n"\
"	} \n"\
" \n"\
"	return false; \n"\
"} \n"\
"#else \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny(SceneData const* scenedata, ray const* r) \n"\
"{ \n"\
"	const float3 invdir = native_recip(r->d.xyz); \n"\
" \n"\
"	if (r->o.w < 0.f) \n"\
"		return false; \n"\
"     \n"\
"	int stack[32]; \n"\
" \n"\
"	int* sptr = stack; \n"\
"	*sptr++ = -1; \n"\
" \n"\
"	int idx = 0; \n"\
"	FatBvhNode node; \n"\
" \n"\
"	bool leftleaf = false; \n"\
"	bool rightleaf = false; \n"\
"	float lefthit = 0.f; \n"\
"	float righthit = 0.f; \n"\
"	int step = 0; \n"\
" \n"\
"	bool found = false; \n"\
" \n"\
"	while (idx > -1) \n"\
"	{ \n"\
"		node = scenedata->nodes[idx]; \n"\
" \n"\
"		leftleaf = LEAFNODE(node.lbound); \n"\
"		rightleaf = LEAFNODE(node.rbound); \n"\
" \n"\
"		lefthit = leftleaf ? -1.f : IntersectBoxF(r, invdir, node.lbound, r->o.w); \n"\
"		righthit = rightleaf ? -1.f : IntersectBoxF(r, invdir, node.rbound, r->o.w); \n"\
" \n"\
"		if (leftleaf) \n"\
"		{ \n"\
"			if (IntersectLeafAny(scenedata, STARTIDX(node.lbound), r)) \n"\
"			{ \n"\
"			    found = true; \n"\
"			    break; \n"\
"			} \n"\
"		} \n"\
"		 \n"\
"		if (rightleaf) \n"\
"		{ \n"\
"			if (IntersectLeafAny(scenedata, STARTIDX(node.rbound), r)) \n"\
"		            { \n"\
"		                found = true; \n"\
"		                break; \n"\
"		            } \n"\
"		} \n"\
" \n"\
"		if (lefthit > 0.f && righthit > 0.f) \n"\
"		{ \n"\
"			int deferred = -1; \n"\
"			if (lefthit > righthit) \n"\
"			{ \n"\
"				idx = (int)node.rbound.pmax.w; \n"\
"				deferred = (int)node.lbound.pmax.w;; \n"\
"			} \n"\
"			else \n"\
"			{ \n"\
"				idx = (int)node.lbound.pmax.w; \n"\
"				deferred = (int)node.rbound.pmax.w; \n"\
"			} \n"\
" \n"\
"		            *sptr++ = deferred; \n"\
"		} \n"\
"		else if (lefthit > 0) \n"\
"		{ \n"\
"			idx = (int)node.lbound.pmax.w; \n"\
"		} \n"\
"		else if (righthit > 0) \n"\
"		{ \n"\
"			idx = (int)node.rbound.pmax.w; \n"\
"		} \n"\
" \n"\
"		if (lefthit <= 0.f && righthit <= 0.f) \n"\
"			idx = *--sptr; \n"\
"	} \n"\
" \n"\
"	return found; \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectClosest( \n"\
"	// Input \n"\
"	__global FatBvhNode const* nodes,   // BVH nodes \n"\
"	__global float3 const* vertices, // Scene positional data \n"\
"	__global Face const* faces,    // Scene indices \n"\
"	__global ShapeData const* shapes, // Shape data \n"\
"	__global ray const* rays,        // Ray workload \n"\
"	int offset,                // Offset in rays array \n"\
"	int numrays,               // Number of rays to process \n"\
"	__global Intersection* hits // Hit datas \n"\
"	, __global int* stack \n"\
"	) \n"\
"{ \n"\
"	__local int ldsstack[SHORT_STACK_SIZE * 64]; \n"\
" \n"\
"	int global_id = get_global_id(0); \n"\
"	int local_id = get_local_id(0); \n"\
"	int group_id = get_group_id(0); \n"\
" \n"\
"	// Fill scene data  \n"\
"	SceneData scenedata = \n"\
"	{ \n"\
"		nodes, \n"\
"		vertices, \n"\
"		faces, \n"\
"		shapes, \n"\
"		0 \n"\
"	}; \n"\
" \n"\
"	if (global_id < numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		ray r = rays[global_id]; \n"\
"		 \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
"#ifndef GLOBAL_STACK  \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id); \n"\
"#else \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect); \n"\
"#endif \n"\
" \n"\
"			// Write data back in case of a hit \n"\
"			hits[global_id] = isect; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectAny( \n"\
"	// Input \n"\
"	__global FatBvhNode const* nodes,   // BVH nodes \n"\
"	__global float3 const* vertices, // Scene positional data \n"\
"	__global Face const* faces,    // Scene indices \n"\
"	__global ShapeData const* shapes,     // Shape data \n"\
"	__global ray const* rays,        // Ray workload \n"\
"	int offset,                // Offset in rays array \n"\
"	int numrays,               // Number of rays to process					 \n"\
"	__global int* hitresults  // Hit results \n"\
"	, __global int* stack \n"\
"	) \n"\
"{ \n"\
" \n"\
"	__local int ldsstack[SHORT_STACK_SIZE * 64]; \n"\
"	int global_id = get_global_id(0); \n"\
"	int local_id = get_local_id(0); \n"\
"	int group_id = get_group_id(0); \n"\
" \n"\
"	// Fill scene data  \n"\
"	SceneData scenedata = \n"\
"	{ \n"\
"		nodes, \n"\
"		vertices, \n"\
"		faces, \n"\
"		shapes, \n"\
"		0 \n"\
"	}; \n"\
" \n"\
"	if (global_id < numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		ray r = rays[global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"#ifndef GLOBAL_STACK  \n"\
"			hitresults[global_id] = IntersectSceneAny(&scenedata, &r, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id) ? 1 : -1; \n"\
"#else \n"\
"			hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"#endif \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectClosestRC( \n"\
"	__global FatBvhNode const* nodes,   // BVH nodes \n"\
"	__global float3 const* vertices, // Scene positional data \n"\
"	__global Face const* faces,      // Scene indices \n"\
"	__global ShapeData const* shapes,     // Shape data \n"\
"	__global ray const* rays,        // Ray workload \n"\
"	int offset,                // Offset in rays array \n"\
"	__global int const* numrays,     // Number of rays in the workload \n"\
"	__global Intersection* hits // Hit datas \n"\
"	, __global int* stack \n"\
"	) \n"\
"{ \n"\
"	__local int ldsstack[SHORT_STACK_SIZE * 64]; \n"\
" \n"\
"	int global_id = get_global_id(0); \n"\
"	int local_id = get_local_id(0); \n"\
"	int group_id = get_group_id(0); \n"\
" \n"\
"	// Fill scene data  \n"\
"	SceneData scenedata = \n"\
"	{ \n"\
"		nodes, \n"\
"		vertices, \n"\
"		faces, \n"\
"		shapes, \n"\
"		0 \n"\
"	}; \n"\
" \n"\
"	// Handle only working subset \n"\
"	if (global_id < *numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		ray r = rays[global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
"#ifndef GLOBAL_STACK  \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id); \n"\
"#else \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect); \n"\
"#endif \n"\
"			// Write data back in case of a hit \n"\
"			hits[global_id] = isect; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectAnyRC( \n"\
"	// Input \n"\
"	__global FatBvhNode const* nodes,   // BVH nodes \n"\
"	__global float3 const* vertices, // Scene positional data \n"\
"	__global Face const* faces,    // Scene indices \n"\
"	__global ShapeData const* shapes,     // Shape data \n"\
"	__global ray const* rays,        // Ray workload \n"\
"	int offset,                // Offset in rays array \n"\
"	__global int const* numrays,     // Number of rays in the workload \n"\
"	__global int* hitresults   // Hit results \n"\
"	, __global int* stack \n"\
"	) \n"\
"{ \n"\
"	__local int ldsstack[SHORT_STACK_SIZE * 64]; \n"\
"	int global_id = get_global_id(0); \n"\
"	int local_id = get_local_id(0); \n"\
"	int group_id = get_group_id(0); \n"\
" \n"\
"	// Fill scene data  \n"\
"	SceneData scenedata = \n"\
"	{ \n"\
"		nodes, \n"\
"		vertices, \n"\
"		faces, \n"\
"		shapes, \n"\
"		0 \n"\
"	}; \n"\
" \n"\
"	// Handle only working subset \n"\
"	if (global_id < *numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		ray r = rays[global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"#ifndef GLOBAL_STACK  \n"\
"            hitresults[global_id] = IntersectSceneAny(&scenedata, &r, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id) ? 1 : -1; \n"\
"#else \n"\
"            hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"#endif \n"\
"		} \n"\
"	} \n"\
"} \n"\
;
static const char g_grid_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"INCLUDES \n"\
"**************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"typedef struct _bbox \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int2 padding; \n"\
"} ray; \n"\
" \n"\
"typedef struct _Intersection \n"\
"{ \n"\
"    int shapeid; \n"\
"    int primid; \n"\
"    int padding0; \n"\
"    int padding1; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
"typedef struct _ShapeData \n"\
"{ \n"\
"    int id; \n"\
"    int bvhidx; \n"\
"    int mask; \n"\
"    int padding1; \n"\
"    float4 m0; \n"\
"    float4 m1; \n"\
"    float4 m2; \n"\
"    float4 m3; \n"\
"    float4  linearvelocity; \n"\
"    float4  angularvelocity; \n"\
"} ShapeData; \n"\
" \n"\
"typedef bbox BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[3]; \n"\
"    int shapeidx; \n"\
"    // Primitive ID \n"\
"    int id; \n"\
"    // Idx count \n"\
"    int cnt; \n"\
" \n"\
"    int2 padding; \n"\
"} Face; \n"\
" \n"\
"#ifndef APPLE \n"\
" \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
"float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"ray transform_ray(ray r, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    ray res; \n"\
"    res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3); \n"\
"    res.o.w = r.o.w; \n"\
"    res.d.w = r.d.w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_mul(float4 q1, float4 q2) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x; \n"\
"    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y; \n"\
"    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z; \n"\
"    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_conjugate(float4 q) \n"\
"{ \n"\
"    return make_float4(-q.x, -q.y, -q.z, q.w); \n"\
"} \n"\
" \n"\
"float4 quaternion_inverse(float4 q) \n"\
"{ \n"\
"    float sqnorm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w; \n"\
"     \n"\
"    if (sqnorm != 0.f) \n"\
"    { \n"\
"        return quaternion_conjugate(q) / sqnorm; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"    } \n"\
"} \n"\
" \n"\
"void rotate_ray(ray* r, float4 q) \n"\
"{ \n"\
"    float4 qinv = quaternion_inverse(q); \n"\
"    float4 v = make_float4(r->o.x, r->o.y, r->o.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->o.xyz = v.xyz; \n"\
"    v = make_float4(r->d.x, r->d.y, r->d.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->d.xyz = v.xyz; \n"\
"} \n"\
" \n"\
"// Intersect Ray against triangle \n"\
"int IntersectTriangle(ray const* r, float3 v1, float3 v2, float3 v3, Intersection* isect) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > isect->uvwt.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        isect->uvwt = make_float4(b1, b2, 0.f, temp); \n"\
"        return 1; \n"\
"    } \n"\
"} \n"\
" \n"\
"int IntersectTriangleP(ray const* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > r->o.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"     \n"\
"    return 1; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"int IntersectBox(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? 1 : 0; \n"\
"} \n"\
" \n"\
"float IntersectBoxF(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.f; \n"\
"} \n"\
" \n"\
"int Ray_GetMask(ray const* r) \n"\
"{ \n"\
"	return r->extra.x; \n"\
"} \n"\
" \n"\
"int Ray_IsActive(ray const* r) \n"\
"{ \n"\
"	return r->extra.y; \n"\
"} \n"\
" \n"\
"float Ray_GetMaxT(ray const* r) \n"\
"{ \n"\
"	return r->o.w; \n"\
"} \n"\
" \n"\
"float Ray_GetTime(ray const* r) \n"\
"{ \n"\
"	return r->d.w; \n"\
"} \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    int startidx; \n"\
"    int numprims; \n"\
"} Voxel; \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    // Cached bounds \n"\
"    bbox bounds; \n"\
"    // Voxel sizes \n"\
"    float3 voxelsize; \n"\
"    // Voxel size inverse \n"\
"    float3 voxelsizeinv; \n"\
"    // Grid resolution in each dimension \n"\
"    int4 gridres; \n"\
"} GridDesc; \n"\
" \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    // Grid voxels structure \n"\
"    __global Voxel const* voxels; \n"\
"    // Scene positional data \n"\
"    __global float3 const* vertices; \n"\
"    // Scene indices \n"\
"    __global Face const* faces; \n"\
"    // Shape IDs \n"\
"    __global int const* shapeids; \n"\
"    // Extra data \n"\
"    __global int const* indices; \n"\
"    // Grid desc \n"\
"    __global GridDesc const* grid; \n"\
"} SceneData; \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"float3 PosToVoxel(__global GridDesc const* grid,  float3 p) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = (int)clamp(floor((p.x - grid->bounds.pmin.x) * grid->voxelsizeinv.x), 0.f, (float)(grid->gridres.x - 1)); \n"\
"    res.y = (int)clamp(floor((p.y - grid->bounds.pmin.y) * grid->voxelsizeinv.y), 0.f, (float)(grid->gridres.y - 1)); \n"\
"    res.z = (int)clamp(floor((p.z - grid->bounds.pmin.z) * grid->voxelsizeinv.z), 0.f, (float)(grid->gridres.z - 1)); \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 VoxelToPos(__global GridDesc const* grid, float3 v) \n"\
"{ \n"\
"    return make_float3(grid->bounds.pmin.x + v.x * grid->voxelsize.x, \n"\
"        grid->bounds.pmin.y + v.y * grid->voxelsize.y, \n"\
"        grid->bounds.pmin.z + v.z * grid->voxelsize.z); \n"\
"} \n"\
" \n"\
"/*float  VoxelToPos(int x, int axis) const \n"\
"{ \n"\
"    return bounds_.pmin[axis] + x * voxelsize_[axis]; \n"\
"}*/ \n"\
" \n"\
"int VoxelPlainIndex(__global GridDesc const* grid, float3 v) \n"\
"{ \n"\
"    return (int)(v.z * grid->gridres.x * grid->gridres.y + v.y * grid->gridres.x + v.x); \n"\
"} \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"BVH FUNCTIONS \n"\
"**************************************************************************/ \n"\
"//  intersect a ray with grid voxel \n"\
"bool IntersectVoxelClosest( \n"\
"    SceneData const* scenedata, \n"\
"    Voxel const* voxel, \n"\
"    ray const* r,                // ray to instersect \n"\
"    Intersection* isect          // Intersection structure \n"\
"    ) \n"\
"{ \n"\
"    float3 v1, v2, v3; \n"\
"    Face face; \n"\
"    bool hit = false; \n"\
" \n"\
"    for (int i = voxel->startidx; i < voxel->startidx + voxel->numprims; ++i) \n"\
"    { \n"\
"        face = scenedata->faces[scenedata->indices[i]]; \n"\
" \n"\
"        v1 = scenedata->vertices[face.idx[0]]; \n"\
"        v2 = scenedata->vertices[face.idx[1]]; \n"\
"        v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"        if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"        { \n"\
"            isect->primid = face.id; \n"\
"            isect->shapeid = scenedata->shapeids[scenedata->indices[i]]; \n"\
"            isect->uvwt.z = 0; \n"\
"            hit = true; \n"\
"        } \n"\
"        else if (face.cnt == 4) \n"\
"        { \n"\
"            v2 = scenedata->vertices[face.idx[3]]; \n"\
"            if (IntersectTriangle(r, v3, v2, v1, isect)) \n"\
"            { \n"\
"                isect->primid = face.id; \n"\
"                isect->shapeid = scenedata->shapeids[scenedata->indices[i]]; \n"\
"                isect->uvwt.z = 1; \n"\
"                hit = true; \n"\
"            } \n"\
"        } \n"\
" \n"\
"    } \n"\
" \n"\
"    return hit; \n"\
"} \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectVoxelAny( \n"\
"    SceneData const* scenedata, \n"\
"    Voxel const* voxel, \n"\
"    ray const* r                      // ray to instersect \n"\
"    ) \n"\
"{ \n"\
"    float3 v1, v2, v3; \n"\
"    Face face; \n"\
" \n"\
"    for (int i = voxel->startidx; i < voxel->startidx + voxel->numprims; ++i) \n"\
"    { \n"\
"        face = scenedata->faces[scenedata->indices[i]]; \n"\
" \n"\
"        v1 = scenedata->vertices[face.idx[0]]; \n"\
"        v2 = scenedata->vertices[face.idx[1]]; \n"\
"        v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"        if (IntersectTriangleP(r, v1, v2, v3)) \n"\
"        { \n"\
"            return true; \n"\
"        } \n"\
"        else if (face.cnt == 4) \n"\
"        { \n"\
"            v2 = scenedata->vertices[face.idx[3]]; \n"\
"            if (IntersectTriangleP(r, v3, v2, v1)) \n"\
"            { \n"\
"                return true; \n"\
"            } \n"\
"        } \n"\
" \n"\
"    } \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole grid structure \n"\
"bool IntersectSceneClosest(SceneData const* scenedata, ray const* r, Intersection* isect) \n"\
"{ \n"\
"    const float3 invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w); \n"\
"    isect->shapeid = -1; \n"\
"    isect->primid = -1; \n"\
" \n"\
"    // Check the ray vs scene bounds \n"\
"    float t = IntersectBoxF(r, invdir, scenedata->grid->bounds, r->o.w); \n"\
" \n"\
"    // If no bail out \n"\
"    if (t < 0.f) \n"\
"    { \n"\
"        //printf(\"Bailing out at initial bbox test % d\\n\", get_global_id(0)); \n"\
"        return false; \n"\
"    } \n"\
" \n"\
"    // Find starting point \n"\
"    float3 isectpoint = r->o.xyz + r->d.xyz * t; \n"\
"    float3 gridres = make_float3(scenedata->grid->gridres.x, scenedata->grid->gridres.y, scenedata->grid->gridres.z); \n"\
" \n"\
"    // Perpare the stepping data \n"\
"    float3 nexthit; \n"\
"    float3 dt; \n"\
"    float3 out; \n"\
" \n"\
"    //for (int axis = 0; axis < 3; ++axis) \n"\
"    //{ \n"\
"    // Voxel position \n"\
"    float3 voxelidx = PosToVoxel(scenedata->grid, isectpoint); \n"\
" \n"\
"    //assert(voxel[0] < gridres_[0] && voxel[1] < gridres_[1] && voxel[2] < gridres_[2]); \n"\
" \n"\
"    float3 step = r->d.xyz >= 0.f ? 1.f : -1.f; \n"\
"    float3 step01 = (step + 1.f) / 2.f; \n"\
" \n"\
"    // Next hit along this axis in parametric units \n"\
"    nexthit = t + (VoxelToPos(scenedata->grid, voxelidx + step01) - isectpoint) * invdir; \n"\
" \n"\
"    // Delta T \n"\
"    dt = scenedata->grid->voxelsize * invdir * step; \n"\
"    dt = isinf(dt) ? 0.f : dt; \n"\
"    //printf(\"Dt %f %f %f\\n\", dt.x, dt.y, dt.z); \n"\
" \n"\
"    // Last voxel \n"\
"    out = step > 0.f ? gridres : -1.f; \n"\
" \n"\
"    bool hit = false; \n"\
"    while ((int)voxelidx.x != (int)out.x &&  \n"\
"        (int)voxelidx.y != (int)out.y &&  \n"\
"        (int)voxelidx.z != (int)out.z) \n"\
"    { \n"\
"        //  \n"\
"        //printf(\"Testing voxel %d %d %d -----> \", (int)voxelidx.x, (int)voxelidx.y, (int)voxelidx.z); \n"\
" \n"\
"        // Check current voxel \n"\
"        int idx = VoxelPlainIndex(scenedata->grid, voxelidx); \n"\
" \n"\
"        Voxel voxel = scenedata->voxels[idx]; \n"\
" \n"\
"        if (voxel.startidx != -1) \n"\
"        { \n"\
"            hit = IntersectVoxelClosest(scenedata, &voxel, r, isect); \n"\
"            //printf(\" %d\\n\", hit); \n"\
"        } \n"\
" \n"\
"        // Advance to next one \n"\
"        int bits = (((nexthit.x < nexthit.y) ? 1 : 0) << 2) + \n"\
"            (((nexthit.x < nexthit.z) ? 1 : 0) << 1) + \n"\
"            (((nexthit.y < nexthit.z) ? 1 : 0)); \n"\
" \n"\
"        const float3 cmptoaxis[8] = {  \n"\
"            { 0.f, 0.f, 1.f }, \n"\
"            { 0.f, 1.f, 0.f }, \n"\
"            { 0.f, 0.f, 1.f }, \n"\
"            { 0.f, 1.f, 0.f }, \n"\
"            { 0.f, 0.f, 1.f }, \n"\
"            { 0.f, 0.f, 1.f }, \n"\
"            { 1.f, 0.f, 0.f }, \n"\
"            { 1.f, 0.f, 0.f } \n"\
"        }; \n"\
" \n"\
"        // Choose axis \n"\
"        float3 stepaxis = cmptoaxis[bits]; \n"\
"        float3 nexthittest = isinf(nexthit) ? 0.f : nexthit; \n"\
" \n"\
"       // printf(\"Next hit %f %f %f\\n\", nexthit.x, nexthit.y, nexthit.z); \n"\
"        //printf(\"Step axis %f %f %f\\n\", stepaxis.x, stepaxis.y, stepaxis.z); \n"\
" \n"\
" \n"\
" \n"\
"        // Early out \n"\
"        if (isect->uvwt.w < dot(nexthittest, stepaxis)) \n"\
"        { \n"\
"            //printf(\"Bailing out at early out step % d\\n\", get_global_id(0)); \n"\
"            break; \n"\
"        } \n"\
" \n"\
"        // Advance current position \n"\
"        voxelidx += step * stepaxis; \n"\
" \n"\
"        // Advance next hit \n"\
"        nexthit += dt * stepaxis; \n"\
"    } \n"\
" \n"\
"    return hit; \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole grid structure \n"\
"bool IntersectSceneAny(SceneData const* scenedata, ray const* r) \n"\
"{ \n"\
"    const float3 invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
" \n"\
"    // Check the ray vs scene bounds \n"\
"    float t = IntersectBoxF(r, invdir, scenedata->grid->bounds, r->o.w); \n"\
" \n"\
"    // If no bail out \n"\
"    if (t < 0.f) \n"\
"    { \n"\
"        //printf(\"Bailing out at initial bbox test % d\\n\", get_global_id(0)); \n"\
"        return false; \n"\
"    } \n"\
" \n"\
"    // Find starting point \n"\
"    float3 isectpoint = r->o.xyz + r->d.xyz * t; \n"\
"    float3 gridres = make_float3(scenedata->grid->gridres.x, scenedata->grid->gridres.y, scenedata->grid->gridres.z); \n"\
" \n"\
"    // Perpare the stepping data \n"\
"    float3 nexthit; \n"\
"    float3 dt; \n"\
"    float3 out; \n"\
" \n"\
"    //for (int axis = 0; axis < 3; ++axis) \n"\
"    //{ \n"\
"    // Voxel position \n"\
"    float3 voxelidx = PosToVoxel(scenedata->grid, isectpoint); \n"\
" \n"\
"    //assert(voxel[0] < gridres_[0] && voxel[1] < gridres_[1] && voxel[2] < gridres_[2]); \n"\
" \n"\
"    float3 step = r->d.xyz >= 0.f ? 1.f : -1.f; \n"\
"    float3 step01 = (step + 1.f) / 2.f; \n"\
" \n"\
"    // Next hit along this axis in parametric units \n"\
"    nexthit = t + (VoxelToPos(scenedata->grid, voxelidx + step01) - isectpoint) * invdir; \n"\
" \n"\
"    // Delta T \n"\
"    dt = scenedata->grid->voxelsize * invdir * step; \n"\
"    dt = isinf(dt) ? 0.f : dt; \n"\
"    //printf(\"Dt %f %f %f\\n\", dt.x, dt.y, dt.z); \n"\
" \n"\
"    // Last voxel \n"\
"    out = step > 0.f ? gridres : -1.f; \n"\
" \n"\
"    while ((int)voxelidx.x != (int)out.x && \n"\
"        (int)voxelidx.y != (int)out.y && \n"\
"        (int)voxelidx.z != (int)out.z) \n"\
"    { \n"\
"        //  \n"\
"        //printf(\"Testing voxel %d %d %d -----> \", (int)voxelidx.x, (int)voxelidx.y, (int)voxelidx.z); \n"\
" \n"\
"        // Check current voxel \n"\
"        int idx = VoxelPlainIndex(scenedata->grid, voxelidx); \n"\
" \n"\
"        Voxel voxel = scenedata->voxels[idx]; \n"\
" \n"\
"        if (voxel.startidx != -1) \n"\
"        { \n"\
"            if (IntersectVoxelAny(scenedata, &voxel, r)) \n"\
"                return true; \n"\
"        } \n"\
" \n"\
"        // Advance to next one \n"\
"        int bits = (((nexthit.x < nexthit.y) ? 1 : 0) << 2) + \n"\
"            (((nexthit.x < nexthit.z) ? 1 : 0) << 1) + \n"\
"            (((nexthit.y < nexthit.z) ? 1 : 0)); \n"\
" \n"\
"        const float3 cmptoaxis[8] = { \n"\
"            { 0.f, 0.f, 1.f }, \n"\
"            { 0.f, 1.f, 0.f }, \n"\
"            { 0.f, 0.f, 1.f }, \n"\
"            { 0.f, 1.f, 0.f }, \n"\
"            { 0.f, 0.f, 1.f }, \n"\
"            { 0.f, 0.f, 1.f }, \n"\
"            { 1.f, 0.f, 0.f }, \n"\
"            { 1.f, 0.f, 0.f } \n"\
"        }; \n"\
" \n"\
"        // Choose axis \n"\
"        float3 stepaxis = cmptoaxis[bits]; \n"\
" \n"\
"        // printf(\"Next hit %f %f %f\\n\", nexthit.x, nexthit.y, nexthit.z); \n"\
"        //printf(\"Step axis %f %f %f\\n\", stepaxis.x, stepaxis.y, stepaxis.z); \n"\
" \n"\
"        // Advance current position \n"\
"        voxelidx += step * stepaxis; \n"\
" \n"\
"        // Advance next hit \n"\
"        nexthit += dt * stepaxis; \n"\
"    } \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectClosest( \n"\
"// Input \n"\
"__global Voxel const* voxels,   // Grid voxels \n"\
"__global GridDesc const* griddesc, // Grid description \n"\
"__global float3 const* vertices, // Scene positional data \n"\
"__global Face const* faces,    // Scene indices \n"\
"__global int const* shapeids,    // Shape IDs \n"\
"__global int const* indices,    // Grid indices \n"\
"__global ray const* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"int numrays,               // Number of rays to process \n"\
"__global Intersection* hits // Hit datas \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        voxels, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids, \n"\
"        indices, \n"\
"        griddesc \n"\
"    }; \n"\
" \n"\
"    Intersection isect; \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[global_id + offset]; \n"\
" \n"\
"        // Calculate closest hit \n"\
"        IntersectSceneClosest(&scenedata, &r, &isect); \n"\
" \n"\
"        // Write data back in case of a hit \n"\
"        hits[global_id + offset] = isect; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectAny( \n"\
"// Input \n"\
"__global Voxel const* voxels,   // Grid voxels \n"\
"__global GridDesc const* griddesc, // Grid description \n"\
"__global float3 const* vertices, // Scene positional data \n"\
"__global Face const* faces,    // Scene indices \n"\
"__global int const* shapeids,    // Shape IDs \n"\
"__global int const* indices,    // Grid indices \n"\
"__global ray const* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"int numrays,               // Number of rays to process \n"\
"__global int* hitresults  // Hit results \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int local_id = get_local_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        voxels, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids, \n"\
"        indices, \n"\
"        griddesc \n"\
"    }; \n"\
" \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[global_id]; \n"\
" \n"\
"        // Calculate any intersection \n"\
"        hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectClosestRC( \n"\
"// Input \n"\
"__global Voxel const* voxels,   // Grid voxels \n"\
"__global GridDesc const* griddesc, // Grid description \n"\
"__global float3 const* vertices, // Scene positional data \n"\
"__global Face const* faces,    // Scene indices \n"\
"__global int const* shapeids,    // Shape IDs \n"\
"__global int const* indices,    // Grid indices \n"\
"__global ray const* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"__global int const* numrays,  // Number of rays to process \n"\
"__global Intersection* hits // Hit datas \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        voxels, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids, \n"\
"        indices, \n"\
"        griddesc \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[global_id]; \n"\
" \n"\
"        // Calculate closest hit \n"\
"        Intersection isect; \n"\
"        IntersectSceneClosest(&scenedata, &r, &isect); \n"\
" \n"\
"        // Write data back in case of a hit \n"\
"        hits[global_id] = isect; \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectAnyRC( \n"\
"// Input \n"\
"__global Voxel const* voxels,   // Grid voxels \n"\
"__global GridDesc const* griddesc, // Grid description \n"\
"__global float3 const* vertices, // Scene positional data \n"\
"__global Face const* faces,    // Scene indices \n"\
"__global int const* shapeids,    // Shape IDs \n"\
"__global int const* indices,    // Grid indices \n"\
"__global ray const* rays,        // Ray workload \n"\
"int offset,                // Offset in rays array \n"\
"__global int const* numrays,  // Number of rays to process \n"\
"__global int* hitresults // Hit datas \n"\
") \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        voxels, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids, \n"\
"        indices, \n"\
"        griddesc \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[global_id]; \n"\
" \n"\
"        // Calculate any intersection \n"\
"        hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"    } \n"\
"} \n"\
;
static const char g_hlbvh_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
" /************************************************************************* \n"\
"  INCLUDES \n"\
"  **************************************************************************/ \n"\
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"typedef struct _bbox \n"\
"{ \n"\
"    float4 pmin; \n"\
"    float4 pmax; \n"\
"} bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"    int2 extra; \n"\
"    int2 padding; \n"\
"} ray; \n"\
" \n"\
"typedef struct _Intersection \n"\
"{ \n"\
"    int shapeid; \n"\
"    int primid; \n"\
"    int padding0; \n"\
"    int padding1; \n"\
" \n"\
"    float4 uvwt; \n"\
"} Intersection; \n"\
" \n"\
"typedef struct _ShapeData \n"\
"{ \n"\
"    int id; \n"\
"    int bvhidx; \n"\
"    int mask; \n"\
"    int padding1; \n"\
"    float4 m0; \n"\
"    float4 m1; \n"\
"    float4 m2; \n"\
"    float4 m3; \n"\
"    float4  linearvelocity; \n"\
"    float4  angularvelocity; \n"\
"} ShapeData; \n"\
" \n"\
"typedef bbox BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[3]; \n"\
"    int shapeidx; \n"\
"    // Primitive ID \n"\
"    int id; \n"\
"    // Idx count \n"\
"    int cnt; \n"\
" \n"\
"    int2 padding; \n"\
"} Face; \n"\
" \n"\
"#ifndef APPLE \n"\
" \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
"float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"ray transform_ray(ray r, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    ray res; \n"\
"    res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3); \n"\
"    res.o.w = r.o.w; \n"\
"    res.d.w = r.d.w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_mul(float4 q1, float4 q2) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x; \n"\
"    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y; \n"\
"    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z; \n"\
"    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_conjugate(float4 q) \n"\
"{ \n"\
"    return make_float4(-q.x, -q.y, -q.z, q.w); \n"\
"} \n"\
" \n"\
"float4 quaternion_inverse(float4 q) \n"\
"{ \n"\
"    float sqnorm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w; \n"\
"     \n"\
"    if (sqnorm != 0.f) \n"\
"    { \n"\
"        return quaternion_conjugate(q) / sqnorm; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"    } \n"\
"} \n"\
" \n"\
"void rotate_ray(ray* r, float4 q) \n"\
"{ \n"\
"    float4 qinv = quaternion_inverse(q); \n"\
"    float4 v = make_float4(r->o.x, r->o.y, r->o.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->o.xyz = v.xyz; \n"\
"    v = make_float4(r->d.x, r->d.y, r->d.z, 0); \n"\
"    v = quaternion_mul(qinv, quaternion_mul(v, q)); \n"\
"    r->d.xyz = v.xyz; \n"\
"} \n"\
" \n"\
"// Intersect Ray against triangle \n"\
"int IntersectTriangle(ray const* r, float3 v1, float3 v2, float3 v3, Intersection* isect) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > isect->uvwt.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        isect->uvwt = make_float4(b1, b2, 0.f, temp); \n"\
"        return 1; \n"\
"    } \n"\
"} \n"\
" \n"\
"int IntersectTriangleP(ray const* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    const float3 e1 = v2 - v1; \n"\
"    const float3 e2 = v3 - v1; \n"\
"    const float3 s1 = cross(r->d.xyz, e2); \n"\
"    const float  invd = native_recip(dot(s1, e1)); \n"\
"    const float3 d = r->o.xyz - v1; \n"\
"    const float  b1 = dot(d, s1) * invd; \n"\
"    const float3 s2 = cross(d, e1); \n"\
"    const float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    const float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > r->o.w) \n"\
"    { \n"\
"        return 0; \n"\
"    } \n"\
"     \n"\
"    return 1; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"int IntersectBox(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? 1 : 0; \n"\
"} \n"\
" \n"\
"float IntersectBoxF(ray const* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
" \n"\
"    const float3 tmax = max(f, n); \n"\
"    const float3 tmin = min(f, n); \n"\
" \n"\
"    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"    return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.f; \n"\
"} \n"\
" \n"\
"int Ray_GetMask(ray const* r) \n"\
"{ \n"\
"	return r->extra.x; \n"\
"} \n"\
" \n"\
"int Ray_IsActive(ray const* r) \n"\
"{ \n"\
"	return r->extra.y; \n"\
"} \n"\
" \n"\
"float Ray_GetMaxT(ray const* r) \n"\
"{ \n"\
"	return r->o.w; \n"\
"} \n"\
" \n"\
"float Ray_GetTime(ray const* r) \n"\
"{ \n"\
"	return r->d.w; \n"\
"} \n"\
"  /************************************************************************* \n"\
"   EXTENSIONS \n"\
"   **************************************************************************/ \n"\
" \n"\
" \n"\
" \n"\
"   /************************************************************************* \n"\
"	TYPE DEFINITIONS \n"\
"	**************************************************************************/ \n"\
"#define STARTIDX(x)     (((int)((x).left))) \n"\
"#define LEAFNODE(x)     (((x).left) == ((x).right)) \n"\
"#define STACK_SIZE 64 \n"\
"#define SHORT_STACK_SIZE 16 \n"\
" \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"	int parent; \n"\
"	int left; \n"\
"	int right; \n"\
"	int next; \n"\
"} HlbvhNode; \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"	// BVH structure \n"\
"	__global HlbvhNode const* nodes; \n"\
"	// Scene bounds \n"\
"	__global bbox const* bounds; \n"\
"	// Scene positional data \n"\
"	__global float3 const* vertices; \n"\
"	// Scene indices \n"\
"	__global Face const* faces; \n"\
"	// Shape IDs \n"\
"	__global ShapeData const* shapes; \n"\
"	// Extra data \n"\
"	__global int const* extra; \n"\
"} SceneData; \n"\
" \n"\
"/************************************************************************* \n"\
" HELPER FUNCTIONS \n"\
" **************************************************************************/ \n"\
" \n"\
" \n"\
" \n"\
" /************************************************************************* \n"\
"  BVH FUNCTIONS \n"\
"  **************************************************************************/ \n"\
"  //  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafClosest( \n"\
"	SceneData const* scenedata, \n"\
"	int faceidx, \n"\
"	ray const* r,                // ray to instersect \n"\
"	Intersection* isect          // Intersection structure \n"\
"	) \n"\
"{ \n"\
"	float3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	face = scenedata->faces[faceidx]; \n"\
"	v1 = scenedata->vertices[face.idx[0]]; \n"\
"	v2 = scenedata->vertices[face.idx[1]]; \n"\
"	v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"	int shapemask = scenedata->shapes[face.shapeidx].mask; \n"\
" \n"\
"	if (Ray_GetMask(r) & shapemask) \n"\
"	{ \n"\
"		if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"		{ \n"\
"			isect->primid = face.id; \n"\
"			isect->shapeid = scenedata->shapes[face.shapeidx].id; \n"\
"			return true; \n"\
"		} \n"\
"	} \n"\
" \n"\
"	return false; \n"\
"} \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafAny( \n"\
"	SceneData const* scenedata, \n"\
"	int faceidx, \n"\
"	ray const* r                      // ray to instersect \n"\
"	) \n"\
"{ \n"\
"	float3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	face = scenedata->faces[faceidx]; \n"\
"	v1 = scenedata->vertices[face.idx[0]]; \n"\
"	v2 = scenedata->vertices[face.idx[1]]; \n"\
"	v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"	int shapemask = scenedata->shapes[face.shapeidx].mask; \n"\
" \n"\
"	if (Ray_GetMask(r) & shapemask) \n"\
"	{ \n"\
"		if (IntersectTriangleP(r, v1, v2, v3)) \n"\
"		{ \n"\
"			return true; \n"\
"		} \n"\
"	} \n"\
" \n"\
"	return false; \n"\
"} \n"\
" \n"\
"#define LDS_BUG \n"\
" \n"\
"#ifdef LDS_BUG \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneClosest(SceneData const* scenedata, ray const* r, Intersection* isect) \n"\
"{ \n"\
"	const float3 invdir = native_recip(r->d.xyz); \n"\
" \n"\
"	isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w); \n"\
"	isect->shapeid = -1; \n"\
"	isect->primid = -1; \n"\
" \n"\
"	int stack[STACK_SIZE]; \n"\
"	int* ptr = stack; \n"\
" \n"\
"	*ptr++ = -1; \n"\
" \n"\
"	int idx = 0; \n"\
" \n"\
"	HlbvhNode node; \n"\
"	bbox lbox; \n"\
"	bbox rbox; \n"\
" \n"\
"	float lefthit = 0.f; \n"\
"	float righthit = 0.f; \n"\
" \n"\
"	while (idx > -1) \n"\
"	{ \n"\
"		node = scenedata->nodes[idx]; \n"\
" \n"\
"		if (LEAFNODE(node)) \n"\
"		{ \n"\
"			IntersectLeafClosest(scenedata, STARTIDX(node), r, isect); \n"\
"		} \n"\
"		else \n"\
"		{ \n"\
"			lbox = scenedata->bounds[node.left]; \n"\
"			rbox = scenedata->bounds[node.right]; \n"\
" \n"\
"			lefthit = IntersectBoxF(r, invdir, lbox, isect->uvwt.w); \n"\
"			righthit = IntersectBoxF(r, invdir, rbox, isect->uvwt.w); \n"\
" \n"\
"			if (lefthit > 0.f && righthit > 0.f) \n"\
"			{ \n"\
"				int deferred = -1; \n"\
"				if (lefthit > righthit) \n"\
"				{ \n"\
"					idx = node.right; \n"\
"					deferred = node.left; \n"\
"				} \n"\
"				else \n"\
"				{ \n"\
"					idx = node.left; \n"\
"					deferred = node.right; \n"\
"				} \n"\
" \n"\
"				*ptr++ = deferred; \n"\
"				continue; \n"\
"			} \n"\
"			else if (lefthit > 0) \n"\
"			{ \n"\
"				idx = node.left; \n"\
"				continue; \n"\
"			} \n"\
"			else if (righthit > 0) \n"\
"			{ \n"\
"				idx = node.right; \n"\
"				continue; \n"\
"			} \n"\
"		} \n"\
" \n"\
"		idx = *--ptr; \n"\
"	} \n"\
" \n"\
" \n"\
"	return isect->shapeid >= 0; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny(SceneData const* scenedata, ray const* r) \n"\
"{ \n"\
"	//return false; \n"\
" \n"\
"	const float3 invdir = native_recip(r->d.xyz); \n"\
" \n"\
"	//if (get_global_id(0) == 0) \n"\
"	//{ \n"\
" \n"\
"	//} \n"\
" \n"\
"	int stack[STACK_SIZE]; \n"\
"	int* ptr = stack; \n"\
" \n"\
"	*ptr++ = -1; \n"\
" \n"\
"	int idx = 0; \n"\
" \n"\
"	HlbvhNode node; \n"\
"	bbox lbox; \n"\
"	bbox rbox; \n"\
" \n"\
"	float lefthit = 0.f; \n"\
"	float righthit = 0.f; \n"\
"	bool hit = false; \n"\
" \n"\
"	int step = 0; \n"\
"	//if (get_global_id(0) == 1) \n"\
"		//printf(\"Starting %d\\n\", get_global_id(0) ); \n"\
"	while (idx > -1) \n"\
"	{ \n"\
"		//printf(\"%d\", get_global_id(0)); \n"\
"		step++; \n"\
"		//if (get_global_id(0) == 1) \n"\
"		//{ \n"\
"			//printf(\"Node %d %d\\n\", idx, step ); \n"\
"		//} \n"\
" \n"\
"		if (step > 10000) \n"\
"			return false; \n"\
" \n"\
"		node = scenedata->nodes[idx]; \n"\
" \n"\
"		if (LEAFNODE(node)) \n"\
"		{ \n"\
"			if (IntersectLeafAny(scenedata, STARTIDX(node), r)) \n"\
"			{ \n"\
"				hit = true; \n"\
"				break; \n"\
"			} \n"\
"		} \n"\
"		else \n"\
"		{ \n"\
"			lbox = scenedata->bounds[node.left]; \n"\
"			rbox = scenedata->bounds[node.right]; \n"\
" \n"\
"			lefthit = IntersectBoxF(r, invdir, lbox, r->o.w); \n"\
"			righthit = IntersectBoxF(r, invdir, rbox, r->o.w); \n"\
" \n"\
"			if (lefthit > 0.f && righthit > 0.f) \n"\
"			{ \n"\
"				int deferred = -1; \n"\
"				if (lefthit > righthit) \n"\
"				{ \n"\
"					idx = node.right; \n"\
"					deferred = node.left; \n"\
"				} \n"\
"				else \n"\
"				{ \n"\
"					idx = node.left; \n"\
"					deferred = node.right; \n"\
"				} \n"\
" \n"\
"				*ptr++ = deferred; \n"\
"				continue; \n"\
"			} \n"\
"			else if (lefthit > 0) \n"\
"			{ \n"\
"				idx = node.left; \n"\
"				continue; \n"\
"			} \n"\
"			else if (righthit > 0) \n"\
"			{ \n"\
"				idx = node.right; \n"\
"				continue; \n"\
"			} \n"\
"		} \n"\
" \n"\
"		idx = *--ptr; \n"\
"	} \n"\
" \n"\
"	//if (get_global_id(0) == 1) \n"\
"	//printf(\"Exiting %d\\n\", get_global_id(0) ); \n"\
" \n"\
"	return hit; \n"\
"} \n"\
" \n"\
"#else \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneClosest(SceneData const* scenedata, ray const* r, Intersection* isect, __global int* stack, __local int* ldsstack) \n"\
"{ \n"\
"	const float3 invdir = native_recip(r->d.xyz); \n"\
" \n"\
"	isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w); \n"\
"	isect->shapeid = -1; \n"\
"	isect->primid = -1; \n"\
" \n"\
"	__global int* gsptr = stack; \n"\
"	__local  int* lsptr = ldsstack; \n"\
" \n"\
"	*lsptr = -1; \n"\
"	lsptr += 64; \n"\
" \n"\
"	int idx = 0; \n"\
" \n"\
"	HlbvhNode node; \n"\
"	bbox lbox; \n"\
"	bbox rbox; \n"\
" \n"\
"	float lefthit = 0.f; \n"\
"	float righthit = 0.f; \n"\
" \n"\
"	while (idx > -1) \n"\
"	{ \n"\
"		while (idx > -1) \n"\
"		{ \n"\
"			node = scenedata->nodes[idx]; \n"\
" \n"\
"			if (LEAFNODE(node)) \n"\
"			{ \n"\
"				IntersectLeafClosest(scenedata, STARTIDX(node), r, isect); \n"\
"			} \n"\
"			else \n"\
"			{ \n"\
"				lbox = scenedata->bounds[node.left]; \n"\
"				rbox = scenedata->bounds[node.right]; \n"\
" \n"\
"				lefthit = IntersectBoxF(r, invdir, lbox, isect->uvwt.w); \n"\
"				righthit = IntersectBoxF(r, invdir, rbox, isect->uvwt.w); \n"\
" \n"\
"				if (lefthit > 0.f && righthit > 0.f) \n"\
"				{ \n"\
"					int deferred = -1; \n"\
"					if (lefthit > righthit) \n"\
"					{ \n"\
"						idx = node.right; \n"\
"						deferred = node.left; \n"\
"					} \n"\
"					else \n"\
"					{ \n"\
"						idx = node.left; \n"\
"						deferred = node.right; \n"\
"					} \n"\
" \n"\
"					if (lsptr - ldsstack >= SHORT_STACK_SIZE * 64) \n"\
"					{ \n"\
"						for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"						{ \n"\
"							gsptr[i] = ldsstack[i * 64]; \n"\
"						} \n"\
" \n"\
"						gsptr += SHORT_STACK_SIZE; \n"\
"						lsptr = ldsstack + 64; \n"\
"					} \n"\
" \n"\
"					*lsptr = deferred; \n"\
"					lsptr += 64; \n"\
" \n"\
"					continue; \n"\
"				} \n"\
"				else if (lefthit > 0) \n"\
"				{ \n"\
"					idx = node.left; \n"\
"					continue; \n"\
"				} \n"\
"				else if (righthit > 0) \n"\
"				{ \n"\
"					idx = node.right; \n"\
"					continue; \n"\
"				} \n"\
"			} \n"\
" \n"\
"			lsptr -= 64; \n"\
"			idx = *(lsptr); \n"\
"		} \n"\
" \n"\
"		if (gsptr > stack) \n"\
"		{ \n"\
"			gsptr -= SHORT_STACK_SIZE; \n"\
" \n"\
"			for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"			{ \n"\
"				ldsstack[i * 64] = gsptr[i]; \n"\
"			} \n"\
" \n"\
"			lsptr = ldsstack + (SHORT_STACK_SIZE - 1) * 64; \n"\
"			idx = ldsstack[64 * (SHORT_STACK_SIZE - 1)]; \n"\
"		} \n"\
"	} \n"\
" \n"\
" \n"\
"	return isect->shapeid >= 0; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny(SceneData const* scenedata, ray const* r, __global int* stack, __local int* ldsstack) \n"\
"{ \n"\
"	const float3 invdir = native_recip(r->d.xyz); \n"\
" \n"\
"	__global int* gsptr = stack; \n"\
"	__local  int* lsptr = ldsstack; \n"\
" \n"\
"	*lsptr = -1; \n"\
"	lsptr += 64; \n"\
" \n"\
"	int idx = 0; \n"\
" \n"\
"	HlbvhNode node; \n"\
"	bbox lbox; \n"\
"	bbox rbox; \n"\
" \n"\
"	float lefthit = 0.f; \n"\
"	float righthit = 0.f; \n"\
" \n"\
"	while (idx > -1) \n"\
"	{ \n"\
"		while (idx > -1) \n"\
"		{ \n"\
"			node = scenedata->nodes[idx]; \n"\
" \n"\
"			if (LEAFNODE(node)) \n"\
"			{ \n"\
"				if (IntersectLeafAny(scenedata, STARTIDX(node), r)) \n"\
"					return true; \n"\
"			} \n"\
"			else \n"\
"			{ \n"\
"				lbox = scenedata->bounds[node.left]; \n"\
"				rbox = scenedata->bounds[node.right]; \n"\
" \n"\
"				lefthit = IntersectBoxF(r, invdir, lbox, r->o.w); \n"\
"				righthit = IntersectBoxF(r, invdir, rbox, r->o.w); \n"\
" \n"\
"				if (lefthit > 0.f && righthit > 0.f) \n"\
"				{ \n"\
"					int deferred = -1; \n"\
"					if (lefthit > righthit) \n"\
"					{ \n"\
"						idx = node.right; \n"\
"						deferred = node.left; \n"\
"					} \n"\
"					else \n"\
"					{ \n"\
"						idx = node.left; \n"\
"						deferred = node.right; \n"\
"					} \n"\
" \n"\
"					if (lsptr - ldsstack >= SHORT_STACK_SIZE * 64) \n"\
"					{ \n"\
"						for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"						{ \n"\
"							gsptr[i] = ldsstack[i * 64]; \n"\
"						} \n"\
" \n"\
"						gsptr += SHORT_STACK_SIZE; \n"\
"						lsptr = ldsstack + 64; \n"\
"					} \n"\
" \n"\
"					*lsptr = deferred; \n"\
"					lsptr += 64; \n"\
" \n"\
"					continue; \n"\
"				} \n"\
"				else if (lefthit > 0) \n"\
"				{ \n"\
"					idx = node.left; \n"\
"					continue; \n"\
"				} \n"\
"				else if (righthit > 0) \n"\
"				{ \n"\
"					idx = node.right; \n"\
"					continue; \n"\
"				} \n"\
"			} \n"\
" \n"\
"			lsptr -= 64; \n"\
"			idx = *(lsptr); \n"\
"		} \n"\
" \n"\
"		if (gsptr > stack) \n"\
"		{ \n"\
"			gsptr -= SHORT_STACK_SIZE; \n"\
" \n"\
"			for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"			{ \n"\
"				ldsstack[i * 64] = gsptr[i]; \n"\
"			} \n"\
" \n"\
"			lsptr = ldsstack + (SHORT_STACK_SIZE - 1) * 64; \n"\
"			idx = ldsstack[64 * (SHORT_STACK_SIZE - 1)]; \n"\
"		} \n"\
"	} \n"\
" \n"\
"	return false; \n"\
"} \n"\
" \n"\
" \n"\
"#endif \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectClosest( \n"\
"	// Input \n"\
"	__global HlbvhNode const* nodes,   // BVH nodes \n"\
"	__global bbox const* bounds,   // BVH nodes \n"\
"	__global float3 const* vertices, // Scene positional data \n"\
"	__global Face const* faces,    // Scene indices \n"\
"	__global ShapeData const* shapes, // Shape data \n"\
"	__global ray const* rays,        // Ray workload \n"\
"	int offset,                // Offset in rays array \n"\
"	int numrays,               // Number of rays to process \n"\
"	__global Intersection* hits // Hit datas \n"\
"	, __global int* stack \n"\
"	) \n"\
"{ \n"\
"#ifndef LDS_BUG \n"\
"	__local int ldsstack[SHORT_STACK_SIZE * 64]; \n"\
"#endif \n"\
" \n"\
"	int global_id = get_global_id(0); \n"\
"	int local_id = get_local_id(0); \n"\
"	int group_id = get_group_id(0); \n"\
" \n"\
"	// Fill scene data \n"\
"	SceneData scenedata = \n"\
"	{ \n"\
"		nodes, \n"\
"		bounds, \n"\
"		vertices, \n"\
"		faces, \n"\
"		shapes, \n"\
"		0 \n"\
"	}; \n"\
" \n"\
"	if (global_id < numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		ray r = rays[global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
"#ifndef LDS_BUG \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id); \n"\
"#else \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect); \n"\
"#endif \n"\
"			// Write data back in case of a hit \n"\
"			hits[global_id] = isect; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectAny( \n"\
"	// Input \n"\
"	// Input \n"\
"	__global HlbvhNode const* nodes,   // BVH nodes \n"\
"	__global bbox const* bounds,   // BVH nodes \n"\
"	__global float3 const* vertices, // Scene positional data \n"\
"	__global Face const* faces,    // Scene indices \n"\
"	__global ShapeData const* shapes,     // Shape data \n"\
"	__global ray const* rays,        // Ray workload \n"\
"	int offset,                // Offset in rays array \n"\
"	int numrays,               // Number of rays to process					 \n"\
"	__global int* hitresults  // Hit results \n"\
"	, __global int* stack \n"\
"	) \n"\
"{ \n"\
" \n"\
"#ifndef LDS_BUG \n"\
"	__local int ldsstack[SHORT_STACK_SIZE * 64]; \n"\
"#endif \n"\
" \n"\
"	int global_id = get_global_id(0); \n"\
"	int local_id = get_local_id(0); \n"\
"	int group_id = get_group_id(0); \n"\
" \n"\
"	// Fill scene data  \n"\
"	SceneData scenedata = \n"\
"	{ \n"\
"		nodes, \n"\
"		bounds, \n"\
"		vertices, \n"\
"		faces, \n"\
"		shapes, \n"\
"		0 \n"\
"	}; \n"\
" \n"\
"	if (global_id < numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		ray r = rays[global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"#ifndef LDS_BUG \n"\
"			hitresults[global_id] = IntersectSceneAny(&scenedata, &r, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id) ? 1 : -1; \n"\
"#else \n"\
"			hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"#endif \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectClosestRC( \n"\
"	// Input \n"\
"	__global HlbvhNode const* nodes,   // BVH nodes \n"\
"	__global bbox const* bounds,   // BVH nodes \n"\
"	__global float3 const* vertices, // Scene positional data \n"\
"	__global Face const* faces,      // Scene indices \n"\
"	__global ShapeData const* shapes,     // Shape data \n"\
"	__global ray const* rays,        // Ray workload \n"\
"	int offset,                // Offset in rays array \n"\
"	__global int const* numrays,     // Number of rays in the workload \n"\
"	__global Intersection* hits // Hit datas \n"\
"	, __global int* stack \n"\
"	) \n"\
"{ \n"\
"#ifndef LDS_BUG \n"\
"	__local int ldsstack[SHORT_STACK_SIZE * 64]; \n"\
"#endif \n"\
" \n"\
"	int global_id = get_global_id(0); \n"\
"	int local_id = get_local_id(0); \n"\
"	int group_id = get_group_id(0); \n"\
" \n"\
"	// Fill scene data  \n"\
"	SceneData scenedata = \n"\
"	{ \n"\
"		nodes, \n"\
"		bounds, \n"\
"		vertices, \n"\
"		faces, \n"\
"		shapes, \n"\
"		0 \n"\
"	}; \n"\
" \n"\
"	// Handle only working subset \n"\
"	if (global_id < *numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		ray r = rays[global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
"#ifndef LDS_BUG \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id); \n"\
"#else \n"\
"			IntersectSceneClosest(&scenedata, &r, &isect); \n"\
"#endif \n"\
" \n"\
"			// Write data back in case of a hit \n"\
"			hits[global_id] = isect; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectAnyRC( \n"\
"	// Input \n"\
"	__global HlbvhNode const* nodes,   // BVH nodes \n"\
"	__global bbox const* bounds,   // BVH nodes \n"\
"	__global float3 const* vertices, // Scene positional data \n"\
"	__global Face const* faces,    // Scene indices \n"\
"	__global ShapeData const* shapes,     // Shape data \n"\
"	__global ray const* rays,        // Ray workload \n"\
"	int offset,                // Offset in rays array \n"\
"	__global int const* numrays,     // Number of rays in the workload \n"\
"	__global int* hitresults   // Hit results \n"\
"	, __global int* stack \n"\
"	) \n"\
"{ \n"\
"#ifndef LDS_BUG \n"\
"	__local int ldsstack[SHORT_STACK_SIZE * 64]; \n"\
"#endif \n"\
"	int global_id = get_global_id(0); \n"\
"	int local_id = get_local_id(0); \n"\
"	int group_id = get_group_id(0); \n"\
" \n"\
"	// Fill scene data  \n"\
"	SceneData scenedata = \n"\
"	{ \n"\
"		nodes, \n"\
"		bounds, \n"\
"		vertices, \n"\
"		faces, \n"\
"		shapes, \n"\
"		0 \n"\
"	}; \n"\
" \n"\
"	// Handle only working subset \n"\
"	if (global_id < *numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		ray r = rays[global_id]; \n"\
" \n"\
"		if (Ray_IsActive(&r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"#ifndef LDS_BUG \n"\
"			hitresults[global_id] = IntersectSceneAny(&scenedata, &r, stack + group_id * 64 * 32 + local_id * 32, ldsstack + local_id) ? 1 : -1; \n"\
"#else \n"\
"			hitresults[global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"#endif \n"\
"		} \n"\
"	} \n"\
"} \n"\
;
static const char g_hlbvh_build_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"	float3 pmin; \n"\
"	float3 pmax; \n"\
"} bbox; \n"\
" \n"\
"// The following two functions are from \n"\
"// http://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/ \n"\
"// Expands a 10-bit integer into 30 bits \n"\
"// by inserting 2 zeros after each bit. \n"\
"static unsigned int ExpandBits(unsigned int v) \n"\
"{ \n"\
"    v = (v * 0x00010001u) & 0xFF0000FFu; \n"\
"    v = (v * 0x00000101u) & 0x0F00F00Fu; \n"\
"    v = (v * 0x00000011u) & 0xC30C30C3u; \n"\
"    v = (v * 0x00000005u) & 0x49249249u; \n"\
"    return v; \n"\
"} \n"\
" \n"\
"// Calculates a 30-bit Morton code for the \n"\
"// given 3D point located within the unit cube [0,1]. \n"\
"unsigned int CalculateMortonCode(float3 p) \n"\
"{ \n"\
"    float x = min(max(p.x * 1024.0f, 0.0f), 1023.0f); \n"\
"    float y = min(max(p.y * 1024.0f, 0.0f), 1023.0f); \n"\
"    float z = min(max(p.z * 1024.0f, 0.0f), 1023.0f); \n"\
"    unsigned int xx = ExpandBits((unsigned int)x); \n"\
"    unsigned int yy = ExpandBits((unsigned int)y); \n"\
"    unsigned int zz = ExpandBits((unsigned int)z); \n"\
"    return xx * 4 + yy * 2 + zz; \n"\
"} \n"\
" \n"\
"// Assign Morton codes to each of positions \n"\
"__kernel void CalcMortonCode( \n"\
"    // Centers of primitive bounding boxes \n"\
"    __global bbox const* bounds, \n"\
"    // Number of primitives \n"\
"    int numpositions, \n"\
"    // Morton codes \n"\
"    __global int* mortoncodes \n"\
"    ) \n"\
"{ \n"\
"    int globalid = get_global_id(0); \n"\
" \n"\
"    if (globalid < numpositions) \n"\
"    { \n"\
"		bbox bound = bounds[globalid]; \n"\
"		float3 center = 0.5f * (bound.pmax + bound.pmin); \n"\
"        mortoncodes[globalid] = CalculateMortonCode(center); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"bbox bboxunion(bbox b1, bbox b2) \n"\
"{ \n"\
"    bbox res; \n"\
"    res.pmin = min(b1.pmin, b2.pmin); \n"\
"    res.pmax = max(b1.pmax, b2.pmax); \n"\
"    return res; \n"\
"} \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"        int parent; \n"\
"        int left; \n"\
"        int right; \n"\
"        int next; \n"\
"} HlbvhNode; \n"\
" \n"\
"#define LEAFIDX(i) ((numprims-1) + i) \n"\
"#define NODEIDX(i) (i) \n"\
" \n"\
"// Calculates longest common prefix length of bit representations \n"\
"// if  representations are equal we consider sucessive indices \n"\
"int delta(__global int* mortoncodes, int numprims, int i1, int i2) \n"\
"{ \n"\
"    // Select left end \n"\
"    int left = min(i1, i2); \n"\
"    // Select right end \n"\
"    int right = max(i1, i2); \n"\
"    // This is to ensure the node breaks if the index is out of bounds \n"\
"    if (left < 0 || right >= numprims)  \n"\
"    { \n"\
"        return -1; \n"\
"    } \n"\
"    // Fetch Morton codes for both ends \n"\
"    int leftcode = mortoncodes[left]; \n"\
"    int rightcode = mortoncodes[right]; \n"\
" \n"\
"    // Special handling of duplicated codes: use their indices as a fallback \n"\
"    return leftcode != rightcode ? clz(leftcode ^ rightcode) : (32 + clz(left ^ right)); \n"\
"} \n"\
" \n"\
"// Shortcut for delta evaluation \n"\
"#define DELTA(i,j) delta(mortoncodes,numprims,i,j) \n"\
" \n"\
"// Find span occupied by internal node with index idx \n"\
"int2 FindSpan(__global int* mortoncodes, int numprims, int idx) \n"\
"{ \n"\
"    // Find the direction of the range \n"\
"    int d = sign((float)(DELTA(idx, idx+1) - DELTA(idx, idx-1))); \n"\
" \n"\
"    // Find minimum number of bits for the break on the other side \n"\
"    int deltamin = DELTA(idx, idx-d); \n"\
" \n"\
"    // Search conservative far end \n"\
"    int lmax = 2; \n"\
"    while (DELTA(idx,idx + lmax * d) > deltamin) \n"\
"        lmax *= 2; \n"\
" \n"\
"    // Search back to find exact bound \n"\
"    // with binary search \n"\
"    int l = 0; \n"\
"    int t = lmax; \n"\
"    do \n"\
"    { \n"\
"        t /= 2; \n"\
"        if(DELTA(idx, idx + (l + t)*d) > deltamin) \n"\
"        { \n"\
"            l = l + t; \n"\
"        } \n"\
"    } \n"\
"    while (t > 1); \n"\
" \n"\
"    // Pack span  \n"\
"    int2 span; \n"\
"    span.x = min(idx, idx + l*d); \n"\
"    span.y = max(idx, idx + l*d); \n"\
"    return span; \n"\
"} \n"\
" \n"\
"// Find split idx within the span \n"\
"int FindSplit(__global int* mortoncodes, int numprims, int2 span) \n"\
"{ \n"\
"    // Fetch codes for both ends \n"\
"    int left = span.x; \n"\
"    int right = span.y; \n"\
" \n"\
"    // Calculate the number of identical bits from higher end \n"\
"    int numidentical = DELTA(left, right); \n"\
" \n"\
"    do \n"\
"    { \n"\
"        // Proposed split \n"\
"        int newsplit = (right + left) / 2; \n"\
" \n"\
"        // If it has more equal leading bits than left and right accept it \n"\
"        if (DELTA(left, newsplit) > numidentical) \n"\
"        { \n"\
"            left = newsplit; \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            right = newsplit; \n"\
"        } \n"\
"    } \n"\
"    while (right > left + 1); \n"\
" \n"\
"    return left; \n"\
"} \n"\
" \n"\
"// Set parent-child relationship \n"\
"__kernel void BuildHierarchy( \n"\
"    // Sorted Morton codes of the primitives \n"\
"    __global int* mortoncodes, \n"\
"    // Bounds \n"\
"    __global bbox* bounds, \n"\
"    // Primitive indices \n"\
"    __global int* indices, \n"\
"    // Number of primitives \n"\
"    int numprims, \n"\
"    // Nodes \n"\
"    __global HlbvhNode* nodes, \n"\
"    // Leaf bounds \n"\
"    __global bbox* boundssorted \n"\
"    ) \n"\
"{ \n"\
"    int globalid = get_global_id(0); \n"\
" \n"\
"    // Set child \n"\
"    if (globalid < numprims) \n"\
"    { \n"\
"        nodes[LEAFIDX(globalid)].left = nodes[LEAFIDX(globalid)].right = indices[globalid]; \n"\
"        boundssorted[LEAFIDX(globalid)] = bounds[indices[globalid]]; \n"\
"    } \n"\
"     \n"\
"    // Set internal nodes \n"\
"    if (globalid < numprims - 1) \n"\
"    { \n"\
"        // Find span occupied by the current node \n"\
"        int2 range = FindSpan(mortoncodes, numprims, globalid); \n"\
" \n"\
"        // Find split position inside the range \n"\
"        int  split = FindSplit(mortoncodes, numprims, range); \n"\
" \n"\
"        // Create child nodes if needed \n"\
"        int c1idx = (split == range.x) ? LEAFIDX(split) : NODEIDX(split); \n"\
"        int c2idx = (split + 1 == range.y) ? LEAFIDX(split + 1) : NODEIDX(split + 1); \n"\
" \n"\
"        nodes[NODEIDX(globalid)].left = c1idx; \n"\
"        nodes[NODEIDX(globalid)].right = c2idx; \n"\
"        //nodes[NODEIDX(globalid)].next = (range.y + 1 < numprims) ? range.y + 1 : -1; \n"\
"        nodes[c1idx].parent = NODEIDX(globalid); \n"\
"        //nodes[c1idx].next = c2idx; \n"\
"        nodes[c2idx].parent = NODEIDX(globalid); \n"\
"        //nodes[c2idx].next = nodes[NODEIDX(globalid)].next; \n"\
"    } \n"\
"} \n"\
" \n"\
"// Propagate bounds up to the root \n"\
"__kernel void RefitBounds(__global bbox* bounds, \n"\
"                          int numprims, \n"\
"                          __global HlbvhNode* nodes, \n"\
"                          __global int* flags \n"\
"                          ) \n"\
"{ \n"\
"    int globalid = get_global_id(0); \n"\
" \n"\
"    // Start from leaf nodes \n"\
"    if (globalid < numprims) \n"\
"    { \n"\
"        // Get my leaf index \n"\
"        int idx = LEAFIDX(globalid); \n"\
" \n"\
"        do \n"\
"        { \n"\
"            // Move to parent node \n"\
"            idx = nodes[idx].parent; \n"\
" \n"\
"            // Check node's flag \n"\
"            if (atomic_cmpxchg(flags + idx, 0, 1) == 1) \n"\
"            { \n"\
"                // If the flag was 1 the second child is ready and  \n"\
"                // this thread calculates bbox for the node \n"\
" \n"\
"                // Fetch kids \n"\
"                int lc = nodes[idx].left; \n"\
"                int rc = nodes[idx].right; \n"\
" \n"\
"                // Calculate bounds \n"\
"                bbox b = bboxunion(bounds[lc], bounds[rc]); \n"\
" \n"\
"                // Write bounds \n"\
"                bounds[idx] = b; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                // If the flag was 0 set it to 1 and bail out. \n"\
"                // The thread handling the second child will \n"\
"                // handle this node. \n"\
"                break; \n"\
"            } \n"\
"        } \n"\
"        while (idx != 0); \n"\
"    } \n"\
"} \n"\
;
static const char g_qbvh_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"/************************************************************************* \n"\
"EXTENSIONS \n"\
"**************************************************************************/ \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"DEFINES \n"\
"**************************************************************************/ \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"#define LEAFNODE(x)     (((x).typecnt & 0xF) == 0) \n"\
"#define NUMPRIMS(x)     (((x).typecnt) >> 4) \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
"typedef struct _bbox \n"\
"{ \n"\
"    float3 pmin; \n"\
"    float3 pmax; \n"\
"} bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"{ \n"\
"    float3 o; \n"\
"    float3 d; \n"\
"    float2 range; \n"\
"    float  time; \n"\
"    float  padding; \n"\
"} ray; \n"\
" \n"\
"typedef struct _Intersection \n"\
"{ \n"\
"    float2 uv; \n"\
"    int instanceid; \n"\
"    int shapeid; \n"\
"    int primid; \n"\
"    int extra; \n"\
"} Intersection; \n"\
" \n"\
" \n"\
"typedef struct _BvhNode \n"\
"{ \n"\
"    bbox bounds; \n"\
"    // 4-bit mask + prim count \n"\
"    uint typecnt; \n"\
" \n"\
"    union \n"\
"    { \n"\
"        // Start primitive index in case of a leaf node \n"\
"        uint start; \n"\
"        // Children indices \n"\
"        uint c[3]; \n"\
"    }; \n"\
" \n"\
"    // Next node during traversal \n"\
"    uint next; \n"\
"    uint padding; \n"\
"    uint padding1; \n"\
"    uint padding2; \n"\
"} BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"{ \n"\
"    // Vertex indices \n"\
"    int idx[4]; \n"\
"    // Primitive ID \n"\
"    int id; \n"\
"    // Idx count \n"\
"    int cnt; \n"\
"} Face; \n"\
" \n"\
"#define NUMPRIMS(x)     (((x).typecnt) >> 1) \n"\
"#define BVHROOTIDX(x)   (((x).start)) \n"\
"#define TRANSFROMIDX(x) (((x).typecnt) >> 1) \n"\
"#define LEAFNODE(x)     (((x).typecnt & 0x1) == 0) \n"\
" \n"\
"typedef struct  \n"\
"{ \n"\
"    // BVH structure \n"\
"    __global BvhNode*       nodes; \n"\
"    // Scene positional data \n"\
"    __global float3*        vertices; \n"\
"    // Scene indices \n"\
"    __global Face*          faces; \n"\
"    // Shape IDs \n"\
"    __global int*           shapeids; \n"\
" \n"\
"} SceneData; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"HELPER FUNCTIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"#ifndef APPLE \n"\
" \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int3 make_int3(int x, int y, int z) \n"\
"{ \n"\
"    int3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
"float3 transform_point(float3 p, float16 m) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m.s0 * p.x + m.s1 * p.y + m.s2 * p.z + m.s3; \n"\
"    res.y = m.s4 * p.x + m.s5 * p.y + m.s6 * p.z + m.s7; \n"\
"    res.z = m.s8 * p.x + m.s9 * p.y + m.sA * p.z + m.sB; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 transform_vector(float3 p, float16 m) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m.s0 * p.x + m.s1 * p.y + m.s2 * p.z; \n"\
"    res.y = m.s4 * p.x + m.s5 * p.y + m.s6 * p.z; \n"\
"    res.z = m.s8 * p.x + m.s9 * p.y + m.sA * p.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"ray transform_ray(ray r, float16 m) \n"\
"{ \n"\
"    ray res; \n"\
"    res.o = transform_point(r.o, m); \n"\
"    res.d = transform_vector(r.d, m); \n"\
"    res.range = r.range; \n"\
"    res.time = r.time; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"// Intersect Ray against triangle \n"\
"bool IntersectTriangle(ray* r, float3 v1, float3 v2, float3 v3, float* a, float* b) \n"\
"{ \n"\
"    float3 e1 = v2 - v1; \n"\
"    float3 e2 = v3 - v1; \n"\
"    float3 s1 = cross(r->d, e2); \n"\
"    float  invd = 1.f / dot(s1, e1); \n"\
"    float3 d = r->o - v1; \n"\
"    float  b1 = dot(d, s1) * invd; \n"\
"    float3 s2 = cross(d, e1); \n"\
"    float  b2 = dot(r->d, s2) * invd;    \n"\
"    float temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f  \n"\
"        || temp < r->range.x || temp > r->range.y) \n"\
"    { \n"\
"        return false; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        r->range.y = temp; \n"\
"        *a = b1; \n"\
"        *b = b2; \n"\
"        return true; \n"\
"    } \n"\
"} \n"\
" \n"\
"bool IntersectTriangleP(ray* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 e1 = v2 - v1; \n"\
"    float3 e2 = v3 - v1; \n"\
"    float3 s1 = cross(r->d, e2); \n"\
"    float  invd = 1.f / dot(s1, e1); \n"\
"    float3 d = r->o - v1; \n"\
"    float  b1 = dot(d, s1) * invd; \n"\
"    float3 s2 = cross(d, e1); \n"\
"    float  b2 = dot(r->d, s2) * invd;    \n"\
"    float temp = dot(e2, s2) * invd; \n"\
" \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f  \n"\
"        || temp < r->range.x || temp > r->range.y) \n"\
"    { \n"\
"        return false; \n"\
"    } \n"\
" \n"\
"    return true; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"bool IntersectBox(ray* r, float3 invdir, bbox box) \n"\
"{ \n"\
"    float3 f = (box.pmax - r->o) * invdir; \n"\
"    float3 n = (box.pmin - r->o) * invdir; \n"\
" \n"\
"    float3 tmax = max(f, n);  \n"\
"    float3 tmin = min(f, n);  \n"\
" \n"\
"    float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), r->range.y); \n"\
"    float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), r->range.x); \n"\
" \n"\
"    return (t1 >= t0); \n"\
"} \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafClosest(  \n"\
"    SceneData* scenedata, \n"\
"    BvhNode* node, \n"\
"    ray* r,                      // ray to instersect \n"\
"    Intersection* isect          // Intersection structure \n"\
"    ) \n"\
"{ \n"\
"    bool hit = false; \n"\
"    int thishit = 0; \n"\
"    float a; \n"\
"    float b; \n"\
"    float3 v1, v2, v3, v4; \n"\
"    Face face; \n"\
" \n"\
"    for (int i=0; i<4; ++i) \n"\
"    { \n"\
"        thishit = false; \n"\
" \n"\
"        face = scenedata->faces[node->start + i]; \n"\
" \n"\
"        v1 = scenedata->vertices[face.idx[0]]; \n"\
"        v2 = scenedata->vertices[face.idx[1]]; \n"\
"        v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"        switch (face.cnt) \n"\
"        { \n"\
"        case 3: \n"\
"            { \n"\
"                // Triangle \n"\
"                if (IntersectTriangle(r, v1, v2, v3, &a, &b)) \n"\
"                { \n"\
"                    thishit = 1; \n"\
"                } \n"\
" \n"\
"                break; \n"\
"            } \n"\
"        case 4: \n"\
"            { \n"\
"                // Quad \n"\
"                v4 = scenedata->vertices[face.idx[3]]; \n"\
"                if (IntersectTriangle(r, v1, v2, v3, &a, &b)) \n"\
"                { \n"\
"                   thishit = 1; \n"\
"                } \n"\
"                else if (IntersectTriangle(r, v3, v4, v1, &a, &b)) \n"\
"                { \n"\
"                    thishit = 2; \n"\
"                } \n"\
" \n"\
"                break; \n"\
"            } \n"\
"        } \n"\
" \n"\
"        if (thishit > 0) \n"\
"        { \n"\
"            hit = true; \n"\
"            isect->uv = make_float2(a,b); \n"\
"            isect->primid = face.id; \n"\
"            isect->shapeid = scenedata->shapeids[node->start + i]; \n"\
"            isect->extra = thishit - 1; \n"\
"            isect->instanceid = -1; \n"\
"        } \n"\
" \n"\
"        if (i + 1 >= NUMPRIMS(*node)) return hit; \n"\
"    } \n"\
" \n"\
"    return hit; \n"\
"} \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafAny( \n"\
"    SceneData* scenedata, \n"\
"    BvhNode* node, \n"\
"    ray* r                      // ray to instersect \n"\
"    ) \n"\
"{ \n"\
"    float3 v1, v2, v3, v4; \n"\
"    Face face;  \n"\
" \n"\
"    for (int i = 0; i < 4; ++i) \n"\
"    { \n"\
"        face = scenedata->faces[node->start + i]; \n"\
" \n"\
"        v1 = scenedata->vertices[face.idx[0]]; \n"\
"        v2 = scenedata->vertices[face.idx[1]]; \n"\
"        v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"        switch (face.cnt) \n"\
"        { \n"\
"        case 3: \n"\
"            { \n"\
"                // Triangle \n"\
"                if (IntersectTriangleP(r, v1, v2, v3)) \n"\
"                { \n"\
"                    return true; \n"\
"                } \n"\
" \n"\
"                break; \n"\
"            } \n"\
"        case 4: \n"\
"            { \n"\
"                v4 = scenedata->vertices[face.idx[3]]; \n"\
"                // Quad \n"\
"                if (IntersectTriangleP(r, v1, v2, v3) || IntersectTriangleP(r, v3, v4, v1)) \n"\
"                { \n"\
"                    return true; \n"\
"                } \n"\
" \n"\
"                break; \n"\
"            } \n"\
"        } \n"\
" \n"\
"        if (i + 1 >= NUMPRIMS(*node)) break; \n"\
"    } \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneClosest(SceneData* scenedata,  ray* r, Intersection* isect) \n"\
"{ \n"\
"    bool hit = false; \n"\
"    float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d; \n"\
" \n"\
"    uint idx = 0; \n"\
"    while (idx != 0xFFFFFFFF) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        // If this is the leaf try to intersect against contained triangle. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node.bounds)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                hit |= IntersectLeafClosest(scenedata, &node, r, isect); \n"\
"                idx = node.next; \n"\
"                continue; \n"\
"            } \n"\
"            // Traverse child nodes otherwise. \n"\
"            else \n"\
"            { \n"\
"                idx = idx + 1; \n"\
"                continue; \n"\
"            } \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            idx = node.next; \n"\
"        } \n"\
"    }; \n"\
" \n"\
"    return hit; \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny(SceneData* scenedata,  ray* r) \n"\
"{ \n"\
"    float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d; \n"\
" \n"\
"    uint idx = 0; \n"\
"    while (idx != 0xFFFFFFFF) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        // If this is the leaf try to intersect against contained triangle. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node.bounds)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                if (IntersectLeafAny(scenedata, &node, r)) \n"\
"                { \n"\
"                    return true; \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    idx = node.next; \n"\
"                    continue; \n"\
"                } \n"\
"            } \n"\
"            // Traverse child nodes otherwise. \n"\
"            else \n"\
"            { \n"\
"                idx = idx + 1; \n"\
"                continue; \n"\
"            } \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            idx = node.next; \n"\
"        } \n"\
"    }; \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectClosest( \n"\
"    // Input \n"\
"    __global BvhNode* nodes,   // BVH nodes \n"\
"    __global float3* vertices, // Scene positional data \n"\
"    __global Face* faces,    // Scene indices \n"\
"    __global int* shapeids,    // Shape IDs \n"\
"    __global ray* rays,        // Ray workload \n"\
"    int offset,                // Offset in rays array \n"\
"    int numrays,               // Number of rays to process					 \n"\
"    __global int* hitresults,  // Hit results \n"\
"    __global Intersection* hits // Hit datas \n"\
"    ) \n"\
"{ \n"\
" \n"\
"    int global_id  = get_global_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata =  \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        int idx = offset + global_id; \n"\
"        ray r = rays[idx]; \n"\
" \n"\
"        // Calculate closest hit \n"\
"        Intersection isect; \n"\
"        int hit = (int)IntersectSceneClosest(&scenedata, &r, &isect); \n"\
" \n"\
"        // Write data back in case of a hit \n"\
"        if (hit) \n"\
"        { \n"\
"            hitresults[idx] = 1; \n"\
"            rays[idx] = r; \n"\
"            hits[idx] = isect; \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            hitresults[idx] = 0; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectAny( \n"\
"    // Input \n"\
"    __global BvhNode* nodes,   // BVH nodes \n"\
"    __global float3* vertices, // Scene positional data \n"\
"    __global Face* faces,    // Scene indices \n"\
"    __global int* shapeids,    // Shape IDs \n"\
"    __global ray* rays,        // Ray workload \n"\
"    int offset,                // Offset in rays array \n"\
"    int numrays,               // Number of rays to process					 \n"\
"    __global int* hitresults   // Hit results \n"\
"    ) \n"\
"{ \n"\
"    int global_id  = get_global_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata =  \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[offset + global_id]; \n"\
" \n"\
"        // Calculate any intersection \n"\
"        hitresults[offset + global_id] = (int)IntersectSceneAny(&scenedata, &r); \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectClosestRC( \n"\
"    // Input \n"\
"    __global BvhNode* nodes,   // BVH nodes \n"\
"    __global float3* vertices, // Scene positional data \n"\
"    __global Face* faces,    // Scene indices \n"\
"    __global int* shapeids,    // Shape IDs \n"\
"    __global ray* rays,        // Ray workload \n"\
"    __global int* rayindices, \n"\
"    int offset,                // Offset in rays array \n"\
"    __global int* numrays,               // Number of rays to process					 \n"\
"    __global Intersection* hits // Hit datas \n"\
"    ) \n"\
"{ \n"\
" \n"\
"    int global_id  = get_global_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata =  \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        int idx = offset + global_id; \n"\
"        ray r = rays[idx]; \n"\
" \n"\
"        // Calculate closest hit \n"\
"        Intersection isect; \n"\
"        int hit = (int)IntersectSceneClosest(&scenedata, &r, &isect); \n"\
" \n"\
"        // Write data back in case of a hit \n"\
"        if (hit) \n"\
"        { \n"\
"            hitresults[idx] = 1; \n"\
"            rays[idx] = r; \n"\
"            hits[idx] = isect; \n"\
"        } \n"\
"        else \n"\
"        { \n"\
"            hitresults[idx] = 0; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectAnyRC( \n"\
"    // Input \n"\
"    __global BvhNode* nodes,   // BVH nodes \n"\
"    __global float3* vertices, // Scene positional data \n"\
"    __global Face* faces,      // Scene indices \n"\
"    __global int* shapeids,    // Shape IDs \n"\
"    __global ray* rays,        // Ray workload \n"\
"    int offset,                // Offset in rays array \n"\
"    __global int* numrays,     // Number of rays to process \n"\
"    __global int* hitresults   // Hit results \n"\
"    ) \n"\
"{ \n"\
"    int global_id  = get_global_id(0); \n"\
" \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata =  \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids \n"\
"    }; \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[offset + global_id]; \n"\
" \n"\
"        // Calculate any intersection \n"\
"        hitresults[offset + global_id] = (int)IntersectSceneAny(&scenedata, &r); \n"\
"    } \n"\
"} \n"\
;
