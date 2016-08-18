/* This is an auto-generated file. Do not edit manually*/

static const char g_bvh_vulkan[]= \
"#version 430 \n"\
" \n"\
"// Note Anvil define system assumes first line is alway a #version so don't rearrange \n"\
" \n"\
"// \n"\
"// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
"// \n"\
"// Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"// of this software and associated documentation files (the \"Software\"), to deal \n"\
"// in the Software without restriction, including without limitation the rights \n"\
"// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"// copies of the Software, and to permit persons to whom the Software is \n"\
"// furnished to do so, subject to the following conditions: \n"\
"// \n"\
"// The above copyright notice and this permission notice shall be included in \n"\
"// all copies or substantial portions of the Software. \n"\
"// \n"\
"// THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"// THE SOFTWARE. \n"\
"// \n"\
" \n"\
"layout( local_size_x = 64, local_size_y = 1, local_size_z = 1 ) in; \n"\
" \n"\
"struct bbox \n"\
"{ \n"\
"	vec4 pmin; \n"\
"	vec4 pmax; \n"\
"}; \n"\
" \n"\
"#define BvhNode bbox \n"\
" \n"\
"struct ray \n"\
"{ \n"\
"	vec4 o; \n"\
"	vec4 d; \n"\
"	ivec2 extra; \n"\
"	ivec2 padding; \n"\
"}; \n"\
" \n"\
"struct ShapeData \n"\
"{ \n"\
"	int id; \n"\
"	int bvhidx; \n"\
"	int mask; \n"\
"	int padding1; \n"\
"	vec4 m0; \n"\
"	vec4 m1; \n"\
"	vec4 m2; \n"\
"	vec4 m3; \n"\
"	vec4  linearvelocity; \n"\
"	vec4  angularvelocity; \n"\
"}; \n"\
" \n"\
"struct Face \n"\
"{ \n"\
"	// Vertex indices \n"\
"	int idx0; \n"\
"	int idx1; \n"\
"	int idx2; \n"\
"	int shapeidx; \n"\
"	// Primitive ID \n"\
"	int id; \n"\
"	// Idx count \n"\
"	int cnt; \n"\
" \n"\
"	ivec2 padding; \n"\
"}; \n"\
" \n"\
"struct Intersection \n"\
"{ \n"\
"	int shapeid; \n"\
"	int primid; \n"\
"	ivec2 padding; \n"\
" \n"\
"	vec4 uvwt; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 0 ) buffer restrict readonly NodesBlock \n"\
"{ \n"\
"	bbox Nodes[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 1 ) buffer restrict readonly VerticesBlock \n"\
"{ \n"\
"	vec4 Vertices[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 2 ) buffer restrict readonly FacesBlock \n"\
"{ \n"\
"	Face Faces[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 3 ) buffer restrict readonly ShapesBlock \n"\
"{ \n"\
"	ShapeData Shapes[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 4 ) buffer restrict readonly RaysBlock \n"\
"{ \n"\
"	ray Rays[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 5 ) buffer restrict readonly OffsetBlock \n"\
"{ \n"\
"	int Offset; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 6 ) buffer restrict readonly NumraysBlock \n"\
"{ \n"\
"	int Numrays; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 7 ) buffer restrict writeonly HitsBlock \n"\
"{ \n"\
"	Intersection Hits[]; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 7 ) buffer restrict writeonly HitsResults \n"\
"{ \n"\
"	int Hitresults[]; \n"\
"}; \n"\
" \n"\
"bool Ray_IsActive( in ray r ) \n"\
"{ \n"\
"	return 0 != r.extra.y ; \n"\
"} \n"\
" \n"\
"int Ray_GetMask( in ray r ) \n"\
"{ \n"\
"	return r.extra.x; \n"\
"} \n"\
" \n"\
"bool IntersectSceneAny( in ray r ); \n"\
"void IntersectSceneClosest( in ray r, inout Intersection isect); \n"\
" \n"\
"void IntersectAny() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
" \n"\
"	// Handle only working subset \n"\
"	if (globalID < Numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		ray r = Rays[Offset + globalID]; \n"\
" \n"\
"		if (Ray_IsActive(r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"			Hitresults[Offset + globalID] = IntersectSceneAny( r ) ? 1 : -1; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"void IntersectAnyRC() \n"\
"{ \n"\
"    IntersectAny(); \n"\
"} \n"\
" \n"\
"void IntersectClosest() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
" \n"\
"	// Handle only working subset \n"\
"	if (globalID < Numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		int idx = Offset + int(globalID); \n"\
"		ray r = Rays[idx]; \n"\
" \n"\
"		if (Ray_IsActive( r )) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
" \n"\
"			IntersectSceneClosest( r, isect); \n"\
" \n"\
"			// Write data back in case of a hit \n"\
"			Hits[idx] = isect; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"void IntersectClosestRC() \n"\
"{ \n"\
"    IntersectClosest(); \n"\
"} \n"\
" \n"\
" \n"\
"#define STARTIDX(x)     ((int(x.pmin.w))) \n"\
"#define LEAFNODE(x)     (((x).pmin.w) != -1.f) \n"\
" \n"\
"bool IntersectBox(in ray r, in vec3 invdir, in bbox box, in float maxt) \n"\
"{ \n"\
"	const vec3 f = (box.pmax.xyz - r.o.xyz) * invdir; \n"\
"	const vec3 n = (box.pmin.xyz - r.o.xyz) * invdir; \n"\
"		   \n"\
"	const vec3 tmax = max(f, n); \n"\
"	const vec3 tmin = min(f, n); \n"\
" \n"\
"	const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"	const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"	return (t1 >= t0) ? true : false; \n"\
"} \n"\
" \n"\
"bool IntersectTriangle( in ray r, in vec3 v1, in vec3 v2, in vec3 v3, inout Intersection isect) \n"\
"{ \n"\
"	const vec3 e1 = v2 - v1; \n"\
"	const vec3 e2 = v3 - v1; \n"\
"	const vec3 s1 = cross(r.d.xyz, e2); \n"\
"	const float  invd = 1.0f/(dot(s1, e1)); \n"\
"	const vec3 d = r.o.xyz - v1; \n"\
"	const float  b1 = dot(d, s1) * invd; \n"\
"	const vec3 s2 = cross(d, e1); \n"\
"	const float  b2 = dot(r.d.xyz, s2) * invd; \n"\
"	const float temp = dot(e2, s2) * invd; \n"\
"	 \n"\
"	if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > isect.uvwt.w) \n"\
"	{ \n"\
"		return false; \n"\
"	} \n"\
"	else \n"\
"	{ \n"\
"		isect.uvwt = vec4(b1, b2, 0.f, temp); \n"\
"		return true; \n"\
"	} \n"\
"} \n"\
" \n"\
"bool IntersectTriangleP( in ray r, in vec3 v1, in vec3 v2, in vec3 v3 ) \n"\
"{ \n"\
"	const vec3 e1 = v2 - v1; \n"\
"	const vec3 e2 = v3 - v1; \n"\
"	const vec3 s1 = cross(r.d.xyz, e2); \n"\
"	const float  invd = 1.0f/(dot(s1, e1)); \n"\
"	const vec3 d = r.o.xyz - v1; \n"\
"	const float  b1 = dot(d, s1) * invd; \n"\
"	const vec3 s2 = cross(d, e1); \n"\
"	const float  b2 = dot(r.d.xyz, s2) * invd; \n"\
"	const float temp = dot(e2, s2) * invd; \n"\
" \n"\
"	if ( b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > r.o.w ) \n"\
"	{ \n"\
"		return false; \n"\
"	} \n"\
" \n"\
"	else \n"\
"	{ \n"\
"		return true; \n"\
"	} \n"\
"} \n"\
" \n"\
"void IntersectLeafClosest( in BvhNode node, in ray r, inout Intersection isect ) \n"\
"{ \n"\
"	vec3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	int start = STARTIDX(node); \n"\
"	face = Faces[start]; \n"\
"	v1 = Vertices[face.idx0].xyz; \n"\
"	v2 = Vertices[face.idx1].xyz; \n"\
"	v3 = Vertices[face.idx2].xyz; \n"\
" \n"\
"	int shapemask = Shapes[face.shapeidx].mask; \n"\
" \n"\
"	if ( ( Ray_GetMask(r) & shapemask ) != 0 ) \n"\
"	{ \n"\
"		if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"		{ \n"\
"            		isect.primid = face.id; \n"\
"		            isect.shapeid = Shapes[face.shapeidx].id; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"bool IntersectLeafAny( in BvhNode node, in ray r ) \n"\
"{ \n"\
"	vec3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	int start = STARTIDX(node); \n"\
"	face = Faces[start]; \n"\
"	v1 = Vertices[face.idx0].xyz; \n"\
"	v2 = Vertices[face.idx1].xyz; \n"\
"	v3 = Vertices[face.idx2].xyz; \n"\
" \n"\
"	int shapemask = Shapes[face.shapeidx].mask; \n"\
" \n"\
"	if ( (Ray_GetMask(r) & shapemask) != 0 ) \n"\
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
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny( in ray r ) \n"\
"{ \n"\
"	vec3 invdir  = vec3(1.f, 1.f, 1.f)/r.d.xyz; \n"\
" \n"\
"	int idx = 0; \n"\
"	while (idx != -1) \n"\
"	{ \n"\
"		// Try intersecting against current node's bounding box. \n"\
"		// If this is the leaf try to intersect against contained triangle. \n"\
"		BvhNode node = Nodes[idx]; \n"\
"		if (IntersectBox(r, invdir, node, r.o.w)) \n"\
"		{ \n"\
"			if (LEAFNODE(node)) \n"\
"			{ \n"\
"				if (IntersectLeafAny( node, r ) ) \n"\
"				{ \n"\
"					return true; \n"\
"				} \n"\
"				else \n"\
"				{ \n"\
"					idx = int(node.pmax.w); \n"\
"				} \n"\
"			} \n"\
"			// Traverse child nodes otherwise. \n"\
"			else \n"\
"			{ \n"\
"				++idx; \n"\
"			} \n"\
"		} \n"\
"		else \n"\
"		{ \n"\
"			idx = int(node.pmax.w); \n"\
"		} \n"\
"	}; \n"\
" \n"\
"	return false; \n"\
"} \n"\
" \n"\
"void IntersectSceneClosest( in ray r, inout Intersection isect) \n"\
"{ \n"\
"	const vec3 invdir  = vec3(1.f, 1.f, 1.f)/r.d.xyz; \n"\
" \n"\
"	isect.uvwt = vec4(0.f, 0.f, 0.f, r.o.w); \n"\
"	isect.shapeid = -1; \n"\
"	isect.primid = -1; \n"\
" \n"\
"	int idx = 0; \n"\
" \n"\
"	isect.padding.x = 667; \n"\
"	isect.padding.y = r.padding.x; \n"\
" \n"\
"	while (idx != -1) \n"\
"	{ \n"\
"		// Try intersecting against current node's bounding box. \n"\
"		// If this is the leaf try to intersect against contained triangle. \n"\
"		BvhNode node = Nodes[idx]; \n"\
"		if (IntersectBox(r, invdir, node, isect.uvwt.w)) \n"\
"		{ \n"\
"			if (LEAFNODE(node)) \n"\
"			{ \n"\
"				IntersectLeafClosest( node, r, isect ); \n"\
"				idx = int(node.pmax.w); \n"\
"			} \n"\
"			// Traverse child nodes otherwise. \n"\
"			else \n"\
"			{ \n"\
"				++idx; \n"\
"			} \n"\
"		} \n"\
"		else \n"\
"		{ \n"\
"			idx = int(node.pmax.w); \n"\
"		} \n"\
"	}; \n"\
"} \n"\
;
static const char g_bvh2l_vulkan[]= \
"#version 430 \n"\
" \n"\
"// Note Anvil define system assumes first line is alway a #version so don't rearrange \n"\
" \n"\
"// \n"\
"// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
"// \n"\
"// Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"// of this software and associated documentation files (the \"Software\"), to deal \n"\
"// in the Software without restriction, including without limitation the rights \n"\
"// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"// copies of the Software, and to permit persons to whom the Software is \n"\
"// furnished to do so, subject to the following conditions: \n"\
"// \n"\
"// The above copyright notice and this permission notice shall be included in \n"\
"// all copies or substantial portions of the Software. \n"\
"// \n"\
"// THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"// THE SOFTWARE. \n"\
"// \n"\
" \n"\
"layout( local_size_x = 64, local_size_y = 1, local_size_z = 1 ) in; \n"\
" \n"\
"struct bbox \n"\
"{ \n"\
"	vec4 pmin; \n"\
"	vec4 pmax; \n"\
"}; \n"\
" \n"\
"#define BvhNode bbox \n"\
" \n"\
"struct ray \n"\
"{ \n"\
"	vec4 o; \n"\
"	vec4 d; \n"\
"	ivec2 extra; \n"\
"	ivec2 padding; \n"\
"}; \n"\
" \n"\
"struct ShapeData \n"\
"{ \n"\
"	int id; \n"\
"	int bvhidx; \n"\
"	int mask; \n"\
"	int padding1; \n"\
"	vec4 m0; \n"\
"	vec4 m1; \n"\
"	vec4 m2; \n"\
"	vec4 m3; \n"\
"	vec4  linearvelocity; \n"\
"	vec4  angularvelocity; \n"\
"}; \n"\
" \n"\
"struct Face \n"\
"{ \n"\
"	// Vertex indices \n"\
"	int idx0; \n"\
"	int idx1; \n"\
"	int idx2; \n"\
"	int shapeidx; \n"\
"	// Primitive ID \n"\
"	int id; \n"\
"	// Idx count \n"\
"	int cnt; \n"\
" \n"\
"	ivec2 padding; \n"\
"}; \n"\
" \n"\
"struct Intersection \n"\
"{ \n"\
"	int shapeid; \n"\
"	int primid; \n"\
"	ivec2 padding; \n"\
"	vec4 uvwt; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 0 ) buffer restrict readonly NodesBlock \n"\
"{ \n"\
"	bbox Nodes[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 1 ) buffer restrict readonly VerticesBlock \n"\
"{ \n"\
"	vec4 Vertices[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 2 ) buffer restrict readonly FacesBlock \n"\
"{ \n"\
"	Face Faces[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 3 ) buffer restrict readonly ShapesBlock \n"\
"{ \n"\
"	ShapeData Shapes[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 4 ) buffer restrict readonly RootidxBlock \n"\
"{ \n"\
"	int Rootidx; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 5 ) buffer restrict readonly RaysBlock \n"\
"{ \n"\
"	ray Rays[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 6 ) buffer restrict readonly OffsetBlock \n"\
"{ \n"\
"	int Offset; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 7 ) buffer restrict readonly NumraysBlock \n"\
"{ \n"\
"	int Numrays; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 8 ) buffer restrict writeonly HitsBlock \n"\
"{ \n"\
"	Intersection Hits[]; \n"\
"}; \n"\
"layout( std430, binding = 8 ) buffer restrict writeonly HitsResultBlock \n"\
"{ \n"\
"	int Hitresults[]; \n"\
"}; \n"\
" \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"bool Ray_IsActive( in ray r ) \n"\
"{ \n"\
"	return 0 != r.extra.y ; \n"\
"} \n"\
" \n"\
"int Ray_GetMask( in ray r ) \n"\
"{ \n"\
"	return r.extra.x; \n"\
"} \n"\
" \n"\
"bool IntersectSceneAny2L( in ray r ); \n"\
"void IntersectSceneClosest2L( in ray r, inout Intersection isect); \n"\
" \n"\
"void IntersectAny2L() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
" \n"\
"	// Handle only working subset \n"\
"	if (globalID < Numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		int idx = Offset + int(globalID); \n"\
"		ray r = Rays[idx]; \n"\
" \n"\
"		if (Ray_IsActive(r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"			Hitresults[idx] = IntersectSceneAny2L( r ) ? 1 : -1; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"void IntersectAnyRC2L() \n"\
"{ \n"\
"    IntersectAny2L(); \n"\
"} \n"\
" \n"\
"void IntersectClosest2L() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
" \n"\
"	// Handle only working subset \n"\
"	if (globalID < Numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		int idx = Offset + int(globalID); \n"\
"		ray r = Rays[idx]; \n"\
" \n"\
"		if (Ray_IsActive( r )) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
" \n"\
"			IntersectSceneClosest2L( r, isect); \n"\
" \n"\
"			// Write data back in case of a hit \n"\
"			Hits[idx] = isect; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"void IntersectClosestRC2L() \n"\
"{ \n"\
"    IntersectClosest2L(); \n"\
"} \n"\
" \n"\
" \n"\
"#define STARTIDX(x)     ((int(x.pmin.w))) \n"\
"#define SHAPEIDX(x)     ((int(x.pmin.w))) \n"\
"#define LEAFNODE(x)     ((x.pmin.w) != -1.f) \n"\
" \n"\
"vec3 transform_point(in vec3 p, in vec4 m0, in vec4 m1, in vec4 m2, in vec4 m3) \n"\
"{ \n"\
"	vec3 res; \n"\
"	res.x = m0.x * p.x + m0.y * p.y + m0.z * p.z + m0.w; \n"\
"	res.y = m1.x * p.x + m1.y * p.y + m1.z * p.z + m1.w; \n"\
"	res.z = m2.x * p.x + m2.y * p.y + m2.z * p.z + m2.w; \n"\
"	return res; \n"\
"} \n"\
" \n"\
"vec3 transform_vector(in vec3 p, in vec4 m0, in vec4 m1, in vec4 m2, in vec4 m3) \n"\
"{ \n"\
"	vec3 res; \n"\
"	res.x = m0.x * p.x + m0.y * p.y + m0.z * p.z; \n"\
"	res.y = m1.x * p.x + m1.y * p.y + m1.z * p.z; \n"\
"	res.z = m2.x * p.x + m2.y * p.y + m2.z * p.z; \n"\
"	return res; \n"\
"} \n"\
" \n"\
" \n"\
"ray transform_ray( in ray r, in vec4 m0, in vec4 m1, in vec4 m2, in vec4 m3) \n"\
"{ \n"\
"	ray res; \n"\
"	res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3); \n"\
"	res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3); \n"\
"	res.o.w = r.o.w; \n"\
"	res.d.w = r.d.w; \n"\
"	return res; \n"\
"} \n"\
" \n"\
"bool IntersectBox(in ray r, in vec3 invdir, in bbox box, in float maxt) \n"\
"{ \n"\
"	const vec3 f = (box.pmax.xyz - r.o.xyz) * invdir; \n"\
"	const vec3 n = (box.pmin.xyz - r.o.xyz) * invdir; \n"\
"		   \n"\
"	const vec3 tmax = max(f, n); \n"\
"	const vec3 tmin = min(f, n); \n"\
" \n"\
"	const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"	const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"	return (t1 >= t0) ? true : false; \n"\
"} \n"\
" \n"\
"bool IntersectTriangleP( in ray r, in vec3 v1, in vec3 v2, in vec3 v3 ) \n"\
"{ \n"\
"	const vec3 e1 = v2 - v1; \n"\
"	const vec3 e2 = v3 - v1; \n"\
"	const vec3 s1 = cross(r.d.xyz, e2); \n"\
"	const float  invd = 1.0f/(dot(s1, e1)); \n"\
"	const vec3 d = r.o.xyz - v1; \n"\
"	const float  b1 = dot(d, s1) * invd; \n"\
"	const vec3 s2 = cross(d, e1); \n"\
"	const float  b2 = dot(r.d.xyz, s2) * invd; \n"\
"	const float temp = dot(e2, s2) * invd; \n"\
"	 \n"\
"	if ( b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > r.o.w ) \n"\
"	{ \n"\
"		return false; \n"\
"	} \n"\
" \n"\
"	else \n"\
"	{ \n"\
"		return true; \n"\
"	} \n"\
"} \n"\
" \n"\
"bool IntersectTriangle( in ray r, in vec3 v1, in vec3 v2, in vec3 v3, inout Intersection isect) \n"\
"{ \n"\
"	const vec3 e1 = v2 - v1; \n"\
"	const vec3 e2 = v3 - v1; \n"\
"	const vec3 s1 = cross(r.d.xyz, e2); \n"\
"	const float  invd = 1.0f/(dot(s1, e1)); \n"\
"	const vec3 d = r.o.xyz - v1; \n"\
"	const float  b1 = dot(d, s1) * invd; \n"\
"	const vec3 s2 = cross(d, e1); \n"\
"	const float  b2 = dot(r.d.xyz, s2) * invd; \n"\
"	const float temp = dot(e2, s2) * invd; \n"\
" \n"\
"	if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > isect.uvwt.w) \n"\
"	{ \n"\
"		return false; \n"\
"	} \n"\
"	else \n"\
"	{ \n"\
"		isect.uvwt = vec4(b1, b2, 0.f, temp); \n"\
"		return true; \n"\
"	} \n"\
"} \n"\
" \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafAny( in BvhNode node, in ray r ) \n"\
"{ \n"\
"	vec3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	int start = STARTIDX(node); \n"\
"	face = Faces[start]; \n"\
"	v1 = Vertices[face.idx0].xyz; \n"\
"	v2 = Vertices[face.idx1].xyz; \n"\
"	v3 = Vertices[face.idx2].xyz; \n"\
" \n"\
"	if (IntersectTriangleP(r, v1, v2, v3)) \n"\
"	{ \n"\
"		return true; \n"\
"	} \n"\
" \n"\
"	return false; \n"\
"} \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"void IntersectLeafClosest( in BvhNode node, in ray r, in int shapeid, inout Intersection isect ) \n"\
"{ \n"\
"	vec3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	int start = STARTIDX(node); \n"\
"	face = Faces[start]; \n"\
"	v1 = Vertices[face.idx0].xyz; \n"\
"	v2 = Vertices[face.idx1].xyz; \n"\
"	v3 = Vertices[face.idx2].xyz; \n"\
" \n"\
"	if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"	{ \n"\
"		isect.primid = face.id; \n"\
"		isect.shapeid = shapeid; \n"\
"	} \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole BVH2L structure \n"\
"bool IntersectSceneAny2L(in ray r) \n"\
"{ \n"\
"	// Precompute invdir for bbox testing \n"\
"	vec3 invdir = vec3(1.f, 1.f, 1.f) / r.d.xyz; \n"\
"	vec3 invdirtop = vec3(1.f, 1.f, 1.f) / r.d.xyz; \n"\
"	// We need to keep original ray around for returns from bottom hierarchy \n"\
"	ray topray = r; \n"\
" \n"\
"	// Fetch top level BVH index \n"\
"	int idx = Rootidx; \n"\
"	// -1 indicates we are traversing top level \n"\
"	int topidx = -1; \n"\
"	while (idx != -1) \n"\
"	{ \n"\
"		// Try intersecting against current node's bounding box. \n"\
"		BvhNode node = Nodes[idx]; \n"\
"		if (IntersectBox(r, invdir, node, r.o.w)) \n"\
"		{ \n"\
"			if (LEAFNODE(node)) \n"\
"			{ \n"\
"				// If this is the leaf it can be either a leaf containing primitives (bottom hierarchy) \n"\
"				// or containing another BVH (top level hierarhcy) \n"\
"				if (topidx != -1) \n"\
"				{ \n"\
"					// This is bottom level, so intersect with a primitives \n"\
"					IntersectLeafAny(node, r); \n"\
" \n"\
"					// And goto next node \n"\
"					idx = int(node.pmax.w); \n"\
"				} \n"\
"				else \n"\
"				{ \n"\
"					// This is top level hierarchy leaf \n"\
"					// Save top node index for return \n"\
"					topidx = idx; \n"\
"					// Get shape descrition struct index \n"\
"					int shapeidx = SHAPEIDX(node); \n"\
"					// Get shape mask \n"\
"					int shapemask = Shapes[shapeidx].mask; \n"\
"					// Drill into 2nd level BVH only if the geometry is not masked vs current ray \n"\
"					// otherwise skip the subtree \n"\
"					if ( ( Ray_GetMask(r) & shapemask) != 0 ) \n"\
"					{ \n"\
"						// Fetch bottom level BVH index \n"\
"						idx = Shapes[shapeidx].bvhidx; \n"\
" \n"\
"						// Fetch BVH transform \n"\
"						vec4 wmi0 = Shapes[shapeidx].m0; \n"\
"						vec4 wmi1 = Shapes[shapeidx].m1; \n"\
"						vec4 wmi2 = Shapes[shapeidx].m2; \n"\
"						vec4 wmi3 = Shapes[shapeidx].m3; \n"\
" \n"\
"						// Apply linear motion blur (world coordinates) \n"\
"						//vec4 lmv = Shapes[shapeidx].linearvelocity; \n"\
"						//vec4 amv = Shapes[SHAPEDATAIDX(node)].angularvelocity; \n"\
"						//r.o.xyz -= (lmv.xyz*r.d.w); \n"\
"						// Transfrom the ray \n"\
"						r = transform_ray(r, wmi0, wmi1, wmi2, wmi3); \n"\
"						//rotate_ray(r, amv); \n"\
"						// Recalc invdir \n"\
"						invdir = vec3(1.f, 1.f, 1.f) / r.d.xyz; \n"\
"						// And continue traversal of the bottom level BVH \n"\
"						continue; \n"\
"					} \n"\
"					else \n"\
"					{ \n"\
"						// Skip the subtree \n"\
"						idx = -1; \n"\
"					} \n"\
"				} \n"\
"			} \n"\
"			// Traverse child nodes otherwise. \n"\
"			else \n"\
"			{ \n"\
"				// This is an internal node, proceed to left child (it is at current + 1 index) \n"\
"				idx = idx + 1; \n"\
"			} \n"\
"		} \n"\
"		else \n"\
"		{ \n"\
"			// We missed the node, goto next one \n"\
"			idx = int(node.pmax.w); \n"\
"		} \n"\
" \n"\
"		// Here check if we ended up traversing bottom level BVH \n"\
"		// in this case idx = 0xFFFFFFFF and topidx has valid value \n"\
"		if (idx == -1 && topidx != -1) \n"\
"		{ \n"\
"			//  Proceed to next top level node \n"\
"			idx = int(Nodes[topidx].pmax.w); \n"\
"			// Set topidx \n"\
"			topidx = -1; \n"\
"			// Restore ray here \n"\
"			r = topray; \n"\
"			// Restore invdir \n"\
"			invdir = invdirtop; \n"\
"		} \n"\
"	} \n"\
" \n"\
"	return false; \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole BVH2L structure \n"\
"void IntersectSceneClosest2L(in ray r, inout Intersection isect) \n"\
"{ \n"\
"	// Init intersection \n"\
"	isect.uvwt = vec4(0.f, 0.f, 0.f, r.o.w); \n"\
"	isect.shapeid = -1; \n"\
"	isect.primid = -1; \n"\
" \n"\
"	isect.padding.x = 667; \n"\
"	isect.padding.y = r.padding.x; \n"\
" \n"\
"	// Precompute invdir for bbox testing \n"\
"	vec3 invdir = vec3(1.f, 1.f, 1.f) / r.d.xyz; \n"\
"	vec3 invdirtop = vec3(1.f, 1.f, 1.f) / r.d.xyz; \n"\
"	// We need to keep original ray around for returns from bottom hierarchy \n"\
"	ray topray = r; \n"\
" \n"\
"	// Fetch top level BVH index \n"\
"	int idx = Rootidx; \n"\
"	// -1 indicates we are traversing top level \n"\
"	int topidx = -1; \n"\
"	// Current shape id \n"\
"	int shapeid = -1; \n"\
"	while (idx != -1) \n"\
"	{ \n"\
"		// Try intersecting against current node's bounding box. \n"\
"		BvhNode node = Nodes[idx]; \n"\
"		if (IntersectBox(r, invdir, node, isect.uvwt.w)) \n"\
"		{ \n"\
"			if (LEAFNODE(node)) \n"\
"			{ \n"\
"				// If this is the leaf it can be either a leaf containing primitives (bottom hierarchy) \n"\
"				// or containing another BVH (top level hierarhcy) \n"\
"				if (topidx != -1) \n"\
"				{ \n"\
"					// This is bottom level, so intersect with a primitives \n"\
"					IntersectLeafClosest(  node, r, shapeid, isect ); \n"\
" \n"\
"					// And goto next node \n"\
"					idx = int(node.pmax.w); \n"\
"				} \n"\
"				else \n"\
"				{ \n"\
"					// This is top level hierarchy leaf \n"\
"					// Save top node index for return \n"\
"					topidx = idx; \n"\
"					// Get shape descrition struct index \n"\
"					int shapeidx = SHAPEIDX(node); \n"\
"					// Get shape mask \n"\
"					int shapemask = Shapes[shapeidx].mask; \n"\
"					// Drill into 2nd level BVH only if the geometry is not masked vs current ray \n"\
"					// otherwise skip the subtree \n"\
"					if ( ( Ray_GetMask(r) & shapemask ) != 0 ) \n"\
"					{ \n"\
"						// Fetch bottom level BVH index \n"\
"						idx = Shapes[shapeidx].bvhidx; \n"\
"						shapeid = Shapes[shapeidx].id; \n"\
" \n"\
"						// Fetch BVH transform \n"\
"						vec4 wmi0 = Shapes[shapeidx].m0; \n"\
"						vec4 wmi1 = Shapes[shapeidx].m1; \n"\
"						vec4 wmi2 = Shapes[shapeidx].m2; \n"\
"						vec4 wmi3 = Shapes[shapeidx].m3; \n"\
" \n"\
"						// Apply linear motion blur (world coordinates) \n"\
"						//vec4 lmv = Shapes[shapeidx].linearvelocity; \n"\
"						//vec4 amv = Shapes[SHAPEDATAIDX(node)].angularvelocity; \n"\
"						//r.o.xyz -= (lmv.xyz*r.d.w); \n"\
"						// Transfrom the ray \n"\
"						r = transform_ray(r, wmi0, wmi1, wmi2, wmi3); \n"\
"						// rotate_ray(r, amv); \n"\
"						// Recalc invdir \n"\
"						invdir = vec3(1.f, 1.f, 1.f) / r.d.xyz; \n"\
"						// And continue traversal of the bottom level BVH \n"\
"						continue; \n"\
"					} \n"\
"					else \n"\
"					{ \n"\
"						idx = -1; \n"\
"					} \n"\
"				} \n"\
"			} \n"\
"			// Traverse child nodes otherwise. \n"\
"			else \n"\
"			{ \n"\
"				// This is an internal node, proceed to left child (it is at current + 1 index) \n"\
"				idx = idx + 1; \n"\
"			} \n"\
"		} \n"\
"		else \n"\
"		{ \n"\
"			// We missed the node, goto next one \n"\
"			idx = int(node.pmax.w); \n"\
"		} \n"\
" \n"\
"		// Here check if we ended up traversing bottom level BVH \n"\
"		// in this case idx = -1 and topidx has valid value \n"\
"		if (idx == -1 && topidx != -1) \n"\
"		{ \n"\
"			//  Proceed to next top level node \n"\
"			idx = int(Nodes[topidx].pmax.w); \n"\
"			// Set topidx \n"\
"			topidx = -1; \n"\
"			// Restore ray here \n"\
"			r.o = topray.o; \n"\
"			r.d = topray.d; \n"\
"			// Restore invdir \n"\
"			invdir = invdirtop; \n"\
"		} \n"\
"	} \n"\
"} \n"\
;
static const char g_fatbvh_vulkan[]= \
"#version 430 \n"\
" \n"\
"// Note Anvil define system assumes first line is alway a #version so don't rearrange \n"\
" \n"\
"// \n"\
"// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
"// \n"\
"// Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"// of this software and associated documentation files (the \"Software\"), to deal \n"\
"// in the Software without restriction, including without limitation the rights \n"\
"// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"// copies of the Software, and to permit persons to whom the Software is \n"\
"// furnished to do so, subject to the following conditions: \n"\
"// \n"\
"// The above copyright notice and this permission notice shall be included in \n"\
"// all copies or substantial portions of the Software. \n"\
"// \n"\
"// THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"// THE SOFTWARE. \n"\
"// \n"\
" \n"\
"layout( local_size_x = 64, local_size_y = 1, local_size_z = 1 ) in; \n"\
" \n"\
"struct bbox \n"\
"{ \n"\
"	vec4 pmin; \n"\
"	vec4 pmax; \n"\
"}; \n"\
" \n"\
"struct FatBvhNode \n"\
"{ \n"\
"	bbox lbound; \n"\
"	bbox rbound; \n"\
"}; \n"\
" \n"\
"struct ray \n"\
"{ \n"\
"	vec4 o; \n"\
"	vec4 d; \n"\
"	ivec2 extra; \n"\
"	ivec2 padding; \n"\
"}; \n"\
" \n"\
"struct ShapeData \n"\
"{ \n"\
"	int id; \n"\
"	int bvhidx; \n"\
"	int mask; \n"\
"	int padding1; \n"\
" \n"\
"	vec4 m0; \n"\
"	vec4 m1; \n"\
"	vec4 m2; \n"\
"	vec4 m3; \n"\
"	vec4  linearvelocity; \n"\
"	vec4  angularvelocity; \n"\
"}; \n"\
" \n"\
"struct Face \n"\
"{ \n"\
"	// Vertex indices \n"\
"	int idx0; \n"\
"	int idx1; \n"\
"	int idx2; \n"\
"	int shapeidx; \n"\
"	// Primitive ID \n"\
"	int id; \n"\
"	// Idx count \n"\
"	int cnt; \n"\
" \n"\
"	ivec2 padding; \n"\
"}; \n"\
" \n"\
"struct Intersection \n"\
"{ \n"\
"	int shapeid; \n"\
"	int primid; \n"\
"	ivec2 padding; \n"\
"	vec4 uvwt; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 0 ) buffer restrict readonly NodesBlock \n"\
"{ \n"\
"	FatBvhNode Nodes[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 1 ) buffer restrict readonly VerticesBlock \n"\
"{ \n"\
"	vec4 Vertices[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 2 ) buffer restrict readonly FacesBlock \n"\
"{ \n"\
"	Face Faces[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 3 ) buffer restrict readonly ShapesBlock \n"\
"{ \n"\
"	ShapeData Shapes[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 4 ) buffer restrict readonly RaysBlock \n"\
"{ \n"\
"	ray Rays[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 5 ) buffer restrict readonly OffsetBlock \n"\
"{ \n"\
"	int Offset; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 6 ) buffer restrict readonly NumraysBlock \n"\
"{ \n"\
"	int Numrays; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 7 ) buffer restrict writeonly HitsBlock \n"\
"{ \n"\
"	Intersection Hits[]; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 7 ) buffer restrict writeonly HitsResults \n"\
"{ \n"\
"	int Hitresults[]; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 8 ) buffer StackBlock \n"\
"{ \n"\
"	int GlobalStack[]; \n"\
"}; \n"\
" \n"\
" \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"bool Ray_IsActive( in ray r ) \n"\
"{ \n"\
"	return 0 != r.extra.y ; \n"\
"} \n"\
" \n"\
"int Ray_GetMask( in ray r ) \n"\
"{ \n"\
"	return r.extra.x; \n"\
"} \n"\
" \n"\
"#define STARTIDX(x)     ((int(x.pmin.w))) \n"\
"#define LEAFNODE(x)     ((x.pmin.w) != -1.f) \n"\
"#define SHORT_STACK_SIZE 16 \n"\
" \n"\
"shared int LDSStack[ SHORT_STACK_SIZE * 64 ]; \n"\
" \n"\
"bool IntersectSceneAny( in ray r, in int stack, in int ldsstack ); \n"\
"void IntersectSceneClosest( in ray r, inout Intersection isect, int stack, in int ldsstack ); \n"\
" \n"\
"void IntersectAny() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
"	uint localID = gl_LocalInvocationID.x; \n"\
"	uint groupID = gl_WorkGroupID.x; \n"\
" \n"\
"	if (globalID < Numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		int idx = Offset + int(globalID); \n"\
"		ray r = Rays[globalID]; \n"\
" \n"\
"		if (Ray_IsActive(r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"			Hitresults[idx] = IntersectSceneAny( r, int( groupID * 64 * 32 + localID * 32 ), int( localID ) ) ? 1 : -1; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"void IntersectAnyRC() \n"\
"{ \n"\
"    IntersectAny(); \n"\
"} \n"\
" \n"\
"void IntersectClosest() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
"	uint localID = gl_LocalInvocationID.x; \n"\
"	uint groupID = gl_WorkGroupID.x; \n"\
" \n"\
"	if (globalID < Numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		int idx = Offset + int(globalID); \n"\
"		ray r = Rays[idx]; \n"\
" \n"\
"		if (Ray_IsActive(r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
"			IntersectSceneClosest(r, isect, int( groupID * 64 * 32 + localID * 32 ), int( localID ) ); \n"\
" \n"\
"			// Write data back in case of a hit \n"\
"			Hits[idx] = isect; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"void IntersectClosestRC() \n"\
"{ \n"\
"    IntersectClosest(); \n"\
"} \n"\
" \n"\
"bool IntersectTriangle( in ray r, in vec3 v1, in vec3 v2, in vec3 v3, inout Intersection isect) \n"\
"{ \n"\
"	const vec3 e1 = v2 - v1; \n"\
"	const vec3 e2 = v3 - v1; \n"\
"	const vec3 s1 = cross(r.d.xyz, e2); \n"\
"	const float  invd = 1.0f/(dot(s1, e1)); \n"\
"	const vec3 d = r.o.xyz - v1; \n"\
"	const float  b1 = dot(d, s1) * invd; \n"\
"	const vec3 s2 = cross(d, e1); \n"\
"	const float  b2 = dot(r.d.xyz, s2) * invd; \n"\
"	const float temp = dot(e2, s2) * invd; \n"\
"	 \n"\
"	if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > isect.uvwt.w) \n"\
"	{ \n"\
"		return false; \n"\
"	} \n"\
"	else \n"\
"	{ \n"\
"		isect.uvwt = vec4(b1, b2, 0.f, temp); \n"\
"		return true; \n"\
"	} \n"\
"} \n"\
" \n"\
"bool IntersectTriangleP( in ray r, in vec3 v1, in vec3 v2, in vec3 v3 ) \n"\
"{ \n"\
"	const vec3 e1 = v2 - v1; \n"\
"	const vec3 e2 = v3 - v1; \n"\
"	const vec3 s1 = cross(r.d.xyz, e2); \n"\
"	const float  invd = 1.0f/(dot(s1, e1)); \n"\
"	const vec3 d = r.o.xyz - v1; \n"\
"	const float  b1 = dot(d, s1) * invd; \n"\
"	const vec3 s2 = cross(d, e1); \n"\
"	const float  b2 = dot(r.d.xyz, s2) * invd; \n"\
"	const float temp = dot(e2, s2) * invd; \n"\
"	 \n"\
"	if ( b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > r.o.w ) \n"\
"	{ \n"\
"		return false; \n"\
"	} \n"\
"	else \n"\
"	{ \n"\
"		return true; \n"\
"	} \n"\
"} \n"\
" \n"\
"float IntersectBoxF( in ray r, in vec3 invdir, in bbox box, in float maxt ) \n"\
"{ \n"\
"	const vec3 f = (box.pmax.xyz - r.o.xyz) * invdir; \n"\
"	const vec3 n = (box.pmin.xyz - r.o.xyz) * invdir; \n"\
" \n"\
"	const vec3 tmax = max(f, n); \n"\
"	const vec3 tmin = min(f, n); \n"\
" \n"\
"	const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"	const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"	return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.f; \n"\
"} \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafAny( in int Faceidx, in ray r ) \n"\
"{ \n"\
"	vec3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	face = Faces[Faceidx]; \n"\
"	v1 = Vertices[face.idx0].xyz; \n"\
"	v2 = Vertices[face.idx1].xyz; \n"\
"	v3 = Vertices[face.idx2].xyz; \n"\
" \n"\
"	int shapemask = Shapes[face.shapeidx].mask; \n"\
" \n"\
"	if ( ( Ray_GetMask(r) & shapemask) != 0 ) \n"\
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
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"void IntersectLeafClosest( in int faceidx, in ray r, inout Intersection isect ) \n"\
"{ \n"\
"	vec3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	face = Faces[faceidx]; \n"\
"	v1 = Vertices[face.idx0].xyz; \n"\
"	v2 = Vertices[face.idx1].xyz; \n"\
"	v3 = Vertices[face.idx2].xyz; \n"\
" \n"\
"	int shapemask = Shapes[face.shapeidx].mask; \n"\
" \n"\
"	if ( ( Ray_GetMask(r) & shapemask) != 0 ) \n"\
"	{ \n"\
"		if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"		{ \n"\
"			isect.primid = face.id; \n"\
"			isect.shapeid = Shapes[face.shapeidx].id; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny( in ray r, in int stack, in int ldsstack ) \n"\
"{ \n"\
"	const vec3 invdir = 1.0f / (r.d.xyz); \n"\
" \n"\
"	//__global int* gsptr = stack; \n"\
"	//__local  int* lsptr = ldsstack; \n"\
"	 \n"\
"	int gsptr = stack; \n"\
"	int lsptr = ldsstack; \n"\
"	 \n"\
"	if (r.o.w < 0.f) \n"\
"		return false; \n"\
" \n"\
"	//*lsptr = -1; \n"\
"	LDSStack[ lsptr ] = -1; \n"\
"	lsptr += 64; \n"\
"	 \n"\
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
"			node = Nodes[idx]; \n"\
" \n"\
"			leftleaf = LEAFNODE(node.lbound); \n"\
"			rightleaf = LEAFNODE(node.rbound); \n"\
" \n"\
"			lefthit = leftleaf ? -1.f : IntersectBoxF(r, invdir, node.lbound, r.o.w); \n"\
"			righthit = rightleaf ? -1.f : IntersectBoxF(r, invdir, node.rbound, r.o.w); \n"\
" \n"\
"			if (leftleaf) \n"\
"			{ \n"\
"				if (IntersectLeafAny(STARTIDX(node.lbound), r)) \n"\
"					return true; \n"\
"			} \n"\
" \n"\
"			if (rightleaf) \n"\
"			{ \n"\
"				if (IntersectLeafAny(STARTIDX(node.rbound), r)) \n"\
"					return true; \n"\
"			} \n"\
" \n"\
"			if (lefthit > 0.f && righthit > 0.f) \n"\
"			{ \n"\
"				int deferred = -1; \n"\
"				if (lefthit > righthit) \n"\
"				{ \n"\
"					idx = int(node.rbound.pmax.w); \n"\
"					deferred = int(node.lbound.pmax.w); \n"\
" \n"\
"				} \n"\
"				else \n"\
"				{ \n"\
"					idx = int(node.lbound.pmax.w); \n"\
"					deferred = int(node.rbound.pmax.w); \n"\
"				} \n"\
" \n"\
"				if (lsptr - ldsstack >= SHORT_STACK_SIZE * 64) \n"\
"				{ \n"\
"					for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"					{ \n"\
"						//gsptr[i] = ldsstack[i * 64]; \n"\
"						GlobalStack[ gsptr + i ] = LDSStack[ ldsstack + i * 64 ]; \n"\
"					} \n"\
" \n"\
"					gsptr += SHORT_STACK_SIZE; \n"\
"					lsptr = ldsstack + 64; \n"\
"					 \n"\
"				} \n"\
" \n"\
"				//*lsptr = deferred; \n"\
"				LDSStack[ lsptr ] = deferred; \n"\
"				lsptr += 64; \n"\
" \n"\
"				continue; \n"\
"			} \n"\
"			else if (lefthit > 0) \n"\
"			{ \n"\
"				idx = int(node.lbound.pmax.w); \n"\
"				continue; \n"\
"			} \n"\
"			else if (righthit > 0) \n"\
"			{ \n"\
"				idx = int(node.rbound.pmax.w); \n"\
"				continue; \n"\
"			} \n"\
" \n"\
"			lsptr -= 64; \n"\
"			//idx = *(lsptr); \n"\
"			idx = LDSStack[ lsptr ]; \n"\
"		} \n"\
" \n"\
"		if (gsptr > stack) \n"\
"		{ \n"\
"			gsptr -= SHORT_STACK_SIZE; \n"\
" \n"\
"			for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"			{ \n"\
"				//ldsstack[i * 64] = gsptr[i]; \n"\
"				LDSStack[ ldsstack + i * 64 ] = GlobalStack[ gsptr + i ]; \n"\
"			} \n"\
" \n"\
"			lsptr = ldsstack + (SHORT_STACK_SIZE - 1) * 64; \n"\
"			//idx = ldsstack[64 * (SHORT_STACK_SIZE - 1)]; \n"\
"			idx = LDSStack[ ldsstack + 64 * (SHORT_STACK_SIZE - 1)]; \n"\
"		} \n"\
"	} \n"\
" \n"\
"	return false; \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"void IntersectSceneClosest(in ray r, inout Intersection isect, in int stack, in int ldsstack) \n"\
"{ \n"\
"	const vec3 invdir = 1.0f/(r.d.xyz); \n"\
" \n"\
"	isect.uvwt = vec4(0.f, 0.f, 0.f, r.o.w); \n"\
"	isect.shapeid = -1; \n"\
"	isect.primid = -1; \n"\
" \n"\
"	if (r.o.w < 0.f) return; \n"\
" \n"\
"	//__global int* gsptr = stack; \n"\
"	//__local  int* lsptr = ldsstack; \n"\
"	int gsptr = stack; \n"\
"	int lsptr = ldsstack; \n"\
" \n"\
"	//*lsptr = -1; \n"\
"	LDSStack[ lsptr ]= -1; \n"\
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
"		node = Nodes[idx]; \n"\
" \n"\
"		leftleaf = LEAFNODE(node.lbound); \n"\
"		rightleaf = LEAFNODE(node.rbound); \n"\
" \n"\
"		lefthit = leftleaf ? -1.f : IntersectBoxF(r, invdir, node.lbound, isect.uvwt.w); \n"\
"		righthit = rightleaf ? -1.f : IntersectBoxF(r, invdir, node.rbound, isect.uvwt.w); \n"\
" \n"\
"		if (leftleaf) \n"\
"		{ \n"\
"			IntersectLeafClosest(STARTIDX(node.lbound), r, isect); \n"\
"		} \n"\
" \n"\
"		if (rightleaf) \n"\
"		{ \n"\
"			IntersectLeafClosest(STARTIDX(node.rbound), r, isect); \n"\
"		} \n"\
" \n"\
"		if (lefthit > 0.f && righthit > 0.f) \n"\
"		{ \n"\
"			int deferred = -1; \n"\
"			if (lefthit > righthit) \n"\
"			{ \n"\
"				idx = int(node.rbound.pmax.w); \n"\
"				deferred = int(node.lbound.pmax.w); \n"\
"			} \n"\
"			else \n"\
"			{ \n"\
"				idx = int(node.lbound.pmax.w); \n"\
"				deferred = int(node.rbound.pmax.w); \n"\
"			} \n"\
" \n"\
"			if (lsptr - ldsstack >= SHORT_STACK_SIZE * 64) \n"\
"			{ \n"\
"				for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"				{ \n"\
"					//gsptr[i] = ldsstack[i * 64]; \n"\
"					GlobalStack[ gsptr + i ] = LDSStack[ ldsstack + i * 64 ]; \n"\
"				} \n"\
" \n"\
"				gsptr += SHORT_STACK_SIZE; \n"\
"				lsptr = ldsstack + 64; \n"\
"			} \n"\
" \n"\
"			//*lsptr = deferred; \n"\
"			LDSStack[ lsptr ] = deferred; \n"\
"			lsptr += 64; \n"\
" \n"\
"			continue; \n"\
"		} \n"\
"		else if (lefthit > 0) \n"\
"		{ \n"\
"			idx = int(node.lbound.pmax.w); \n"\
"			continue; \n"\
"		} \n"\
"		else if (righthit > 0) \n"\
"		{ \n"\
"			idx = int(node.rbound.pmax.w); \n"\
"			continue; \n"\
"		} \n"\
" \n"\
"		lsptr -= 64; \n"\
"		//idx = *(lsptr); \n"\
"		idx = LDSStack[ lsptr ]; \n"\
" \n"\
"		if (gsptr > stack) \n"\
"		{ \n"\
"			gsptr -= SHORT_STACK_SIZE; \n"\
" \n"\
"			for (int i = 1; i < SHORT_STACK_SIZE; ++i) \n"\
"			{ \n"\
"				//ldsstack[i * 64] = gsptr[i]; \n"\
"				LDSStack[ ldsstack + i * 64 ] = GlobalStack[ gsptr + i ]; \n"\
"			} \n"\
" \n"\
"			lsptr = ldsstack + (SHORT_STACK_SIZE - 1) * 64; \n"\
"			//idx = ldsstack[64 * (SHORT_STACK_SIZE - 1)]; \n"\
"			idx = LDSStack[ ldsstack + 64 * (SHORT_STACK_SIZE - 1) ]; \n"\
"		} \n"\
"	} \n"\
"} \n"\
;
static const char g_hlbvh_vulkan[]= \
"#version 430 \n"\
" \n"\
"// Note Anvil define system assumes first line is alway a #version so don't rearrange \n"\
" \n"\
"// \n"\
"// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
"// \n"\
"// Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"// of this software and associated documentation files (the \"Software\"), to deal \n"\
"// in the Software without restriction, including without limitation the rights \n"\
"// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"// copies of the Software, and to permit persons to whom the Software is \n"\
"// furnished to do so, subject to the following conditions: \n"\
"// \n"\
"// The above copyright notice and this permission notice shall be included in \n"\
"// all copies or substantial portions of the Software. \n"\
"// \n"\
"// THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"// THE SOFTWARE. \n"\
"// \n"\
" \n"\
"layout( local_size_x = 64, local_size_y = 1, local_size_z = 1 ) in; \n"\
" \n"\
"struct bbox \n"\
"{ \n"\
"	vec4 pmin; \n"\
"	vec4 pmax; \n"\
"}; \n"\
" \n"\
"struct HlbvhNode \n"\
"{ \n"\
"	int parent; \n"\
"	int left; \n"\
"	int right; \n"\
"	int next; \n"\
"}; \n"\
" \n"\
"struct ray \n"\
"{ \n"\
"	vec4 o; \n"\
"	vec4 d; \n"\
"	ivec2 extra; \n"\
"	ivec2 padding; \n"\
"}; \n"\
" \n"\
"struct ShapeData \n"\
"{ \n"\
"	int id; \n"\
"	int bvhidx; \n"\
"	int mask; \n"\
"	int padding1; \n"\
"	vec4 m0; \n"\
"	vec4 m1; \n"\
"	vec4 m2; \n"\
"	vec4 m3; \n"\
"	vec4  linearvelocity; \n"\
"	vec4  angularvelocity; \n"\
"}; \n"\
" \n"\
"struct Face \n"\
"{ \n"\
"	// Vertex indices \n"\
"	int idx0; \n"\
"	int idx1; \n"\
"	int idx2; \n"\
"	int shapeidx; \n"\
"	// Primitive ID \n"\
"	int id; \n"\
"	// Idx count \n"\
"	int cnt; \n"\
" \n"\
"	ivec2 padding; \n"\
"}; \n"\
" \n"\
"struct Intersection \n"\
"{ \n"\
"	int shapeid; \n"\
"	int primid; \n"\
"	ivec2 padding; \n"\
" \n"\
"	vec4 uvwt; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 0 ) buffer restrict readonly NodesBlock \n"\
"{ \n"\
"	HlbvhNode Nodes[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 1 ) buffer restrict readonly BoundsBlock \n"\
"{ \n"\
"	bbox Bounds[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 2 ) buffer restrict readonly VerticesBlock \n"\
"{ \n"\
"	vec4 Vertices[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 3 ) buffer restrict readonly FacesBlock \n"\
"{ \n"\
"	Face Faces[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 4 ) buffer restrict readonly ShapesBlock \n"\
"{ \n"\
"	ShapeData Shapes[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 5 ) buffer restrict readonly RaysBlock \n"\
"{ \n"\
"	ray Rays[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 6 ) buffer restrict readonly OffsetBlock \n"\
"{ \n"\
"	int Offset; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 7 ) buffer restrict readonly NumraysBlock \n"\
"{ \n"\
"	int Numrays; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 8 ) buffer restrict writeonly HitsBlock \n"\
"{ \n"\
"	Intersection Hits[]; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 8 ) buffer restrict writeonly HitresultBlock \n"\
"{ \n"\
"	int Hitresults[]; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 9 ) buffer StackBlock \n"\
"{ \n"\
"	int GlobalStack[]; \n"\
"}; \n"\
" \n"\
" \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"/************************************************************************* \n"\
"TYPE DEFINITIONS \n"\
"**************************************************************************/ \n"\
" \n"\
"bool Ray_IsActive( in ray r ) \n"\
"{ \n"\
"	return 0 != r.extra.y ; \n"\
"} \n"\
" \n"\
"int Ray_GetMask( in ray r ) \n"\
"{ \n"\
"	return r.extra.x; \n"\
"} \n"\
" \n"\
"bool IntersectSceneAny( in ray r ); \n"\
"void IntersectSceneClosest( in ray r, inout Intersection isect); \n"\
" \n"\
"void IntersectAny() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
" \n"\
"	if (globalID < Numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		int idx = Offset + int(globalID); \n"\
"		ray r = Rays[idx]; \n"\
" \n"\
"		if (Ray_IsActive(r)) \n"\
"		{ \n"\
"			// Calculate any intersection \n"\
"			Hitresults[idx] = IntersectSceneAny(r) ? 1 : -1; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"void IntersectAnyRC() \n"\
"{ \n"\
"    IntersectAny(); \n"\
"} \n"\
" \n"\
"void IntersectClosest() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
" \n"\
"	if (globalID < Numrays) \n"\
"	{ \n"\
"		// Fetch ray \n"\
"		int idx = Offset + int(globalID); \n"\
"		ray r = Rays[idx]; \n"\
" \n"\
"		if (Ray_IsActive(r)) \n"\
"		{ \n"\
"			// Calculate closest hit \n"\
"			Intersection isect; \n"\
"			IntersectSceneClosest(r, isect); \n"\
" \n"\
"			// Write data back in case of a hit \n"\
"			Hits[idx] = isect; \n"\
"		} \n"\
"	} \n"\
"} \n"\
"void IntersectClosestRC() \n"\
"{ \n"\
"    IntersectClosest(); \n"\
"} \n"\
" \n"\
"float IntersectBoxF( in ray r, in vec3 invdir, in bbox box, in float maxt ) \n"\
"{ \n"\
"	const vec3 f = (box.pmax.xyz - r.o.xyz) * invdir; \n"\
"	const vec3 n = (box.pmin.xyz - r.o.xyz) * invdir; \n"\
" \n"\
"	const vec3 tmax = max(f, n); \n"\
"	const vec3 tmin = min(f, n); \n"\
" \n"\
"	const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"	const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"	return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.f; \n"\
"} \n"\
" \n"\
" \n"\
"bool IntersectBox(in ray r, in vec3 invdir, in bbox box, in float maxt) \n"\
"{ \n"\
"	const vec3 f = (box.pmax.xyz - r.o.xyz) * invdir; \n"\
"	const vec3 n = (box.pmin.xyz - r.o.xyz) * invdir; \n"\
"		   \n"\
"	const vec3 tmax = max(f, n); \n"\
"	const vec3 tmin = min(f, n); \n"\
" \n"\
"	const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"	const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
" \n"\
"	return (t1 >= t0) ? true : false; \n"\
"} \n"\
" \n"\
"bool IntersectTriangle( in ray r, in vec3 v1, in vec3 v2, in vec3 v3, inout Intersection isect) \n"\
"{ \n"\
"	const vec3 e1 = v2 - v1; \n"\
"	const vec3 e2 = v3 - v1; \n"\
"	const vec3 s1 = cross(r.d.xyz, e2); \n"\
"	const float  invd = 1.0f/(dot(s1, e1)); \n"\
"	const vec3 d = r.o.xyz - v1; \n"\
"	const float  b1 = dot(d, s1) * invd; \n"\
"	const vec3 s2 = cross(d, e1); \n"\
"	const float  b2 = dot(r.d.xyz, s2) * invd; \n"\
"	const float temp = dot(e2, s2) * invd; \n"\
"	 \n"\
"	if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > isect.uvwt.w) \n"\
"	{ \n"\
"		return false; \n"\
"	} \n"\
"	else \n"\
"	{ \n"\
"		isect.uvwt = vec4(b1, b2, 0.f, temp); \n"\
"		return true; \n"\
"	} \n"\
"} \n"\
" \n"\
"bool IntersectTriangleP( in ray r, in vec3 v1, in vec3 v2, in vec3 v3 ) \n"\
"{ \n"\
"	const vec3 e1 = v2 - v1; \n"\
"	const vec3 e2 = v3 - v1; \n"\
"	const vec3 s1 = cross(r.d.xyz, e2); \n"\
"	const float  invd = 1.0f/(dot(s1, e1)); \n"\
"	const vec3 d = r.o.xyz - v1; \n"\
"	const float  b1 = dot(d, s1) * invd; \n"\
"	const vec3 s2 = cross(d, e1); \n"\
"	const float  b2 = dot(r.d.xyz, s2) * invd; \n"\
"	const float temp = dot(e2, s2) * invd; \n"\
" \n"\
"	if ( b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > r.o.w ) \n"\
"	{ \n"\
"		return false; \n"\
"	} \n"\
" \n"\
"	else \n"\
"	{ \n"\
"		return true; \n"\
"	} \n"\
"} \n"\
" /************************************************************************* \n"\
"  BVH FUNCTIONS \n"\
"  **************************************************************************/ \n"\
" \n"\
"#define STARTIDX(x)     ((int((x).left))) \n"\
"#define LEAFNODE(x)     (((x).left) == ((x).right)) \n"\
"#define STACK_SIZE 64 \n"\
"#define SHORT_STACK_SIZE 16 \n"\
" \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafAny( in int faceidx, in ray r ) \n"\
"{ \n"\
"	vec3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	face = Faces[faceidx]; \n"\
"	v1 = Vertices[face.idx0].xyz; \n"\
"	v2 = Vertices[face.idx1].xyz; \n"\
"	v3 = Vertices[face.idx2].xyz; \n"\
" \n"\
"	int shapemask = Shapes[face.shapeidx].mask; \n"\
" \n"\
"	if ( ( Ray_GetMask(r) & shapemask ) != 0 ) \n"\
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
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny( in ray r ) \n"\
"{ \n"\
"	const vec3 invdir = 1.0f/(r.d.xyz); \n"\
" \n"\
"	int stack[STACK_SIZE]; \n"\
"	int ptr = 0; \n"\
" \n"\
"	//*ptr++ = -1; \n"\
"	stack[ ptr++ ] = -1; \n"\
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
"	while (idx > -1) \n"\
"	{ \n"\
"		step++; \n"\
" \n"\
"		if (step > 10000) \n"\
"			return false; \n"\
" \n"\
"		node = Nodes[idx]; \n"\
" \n"\
"		if (LEAFNODE(node)) \n"\
"		{ \n"\
"			if (IntersectLeafAny(STARTIDX(node), r)) \n"\
"			{ \n"\
"				hit = true; \n"\
"				break; \n"\
"			} \n"\
"		} \n"\
"		else \n"\
"		{ \n"\
"			lbox = Bounds[node.left]; \n"\
"			rbox = Bounds[node.right]; \n"\
" \n"\
"			lefthit = IntersectBoxF(r, invdir, lbox, r.o.w); \n"\
"			righthit = IntersectBoxF(r, invdir, rbox, r.o.w); \n"\
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
"				//*ptr++ = deferred; \n"\
"				stack[ ptr++ ] = deferred; \n"\
" \n"\
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
"		//idx = *--ptr; \n"\
"		idx = stack[ --ptr ]; \n"\
"	} \n"\
" \n"\
"	return hit; \n"\
"} \n"\
" \n"\
"  //  intersect a ray with leaf BVH node \n"\
"void IntersectLeafClosest( in int faceidx, in ray r, inout Intersection isect ) \n"\
"{ \n"\
"	vec3 v1, v2, v3; \n"\
"	Face face; \n"\
" \n"\
"	face = Faces[faceidx]; \n"\
"	v1 = Vertices[face.idx0].xyz; \n"\
"	v2 = Vertices[face.idx1].xyz; \n"\
"	v3 = Vertices[face.idx2].xyz; \n"\
" \n"\
"	int shapemask = Shapes[face.shapeidx].mask; \n"\
" \n"\
"	if (( Ray_GetMask(r) & shapemask) != 0 ) \n"\
"	{ \n"\
"		if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"		{ \n"\
"			isect.primid = face.id; \n"\
"			isect.shapeid = Shapes[face.shapeidx].id; \n"\
"		} \n"\
"	} \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"void IntersectSceneClosest( in ray r, inout Intersection isect ) \n"\
"{ \n"\
"	const vec3 invdir = 1.0f / (r.d.xyz); \n"\
" \n"\
"	isect.uvwt = vec4(0.f, 0.f, 0.f, r.o.w); \n"\
"	isect.shapeid = -1; \n"\
"	isect.primid = -1; \n"\
" \n"\
"	int stack[STACK_SIZE]; \n"\
"	//int* ptr = stack; \n"\
"	int ptr = 0; \n"\
" \n"\
"	//*ptr++ = -1; \n"\
"	stack[ ptr++ ] = -1; \n"\
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
"		node = Nodes[idx]; \n"\
" \n"\
"		if (LEAFNODE(node)) \n"\
"		{ \n"\
"			IntersectLeafClosest(STARTIDX(node), r, isect); \n"\
"		} \n"\
"		else \n"\
"		{ \n"\
"			lbox = Bounds[node.left]; \n"\
"			rbox = Bounds[node.right]; \n"\
" \n"\
"			lefthit = IntersectBoxF(r, invdir, lbox, isect.uvwt.w); \n"\
"			righthit = IntersectBoxF(r, invdir, rbox, isect.uvwt.w); \n"\
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
"				//*ptr++ = deferred; \n"\
"				stack[ ptr++ ] = deferred; \n"\
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
"		//idx = *--ptr; \n"\
"		idx = stack[ --ptr ]; \n"\
"	} \n"\
"} \n"\
;
static const char g_hlbvh_build_vulkan[]= \
"#version 430 \n"\
" \n"\
"// Note Anvil define system assumes first line is alway a #version so don't rearrange \n"\
" \n"\
"// \n"\
"// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
"// \n"\
"// Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"// of this software and associated documentation files (the \"Software\"), to deal \n"\
"// in the Software without restriction, including without limitation the rights \n"\
"// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"// copies of the Software, and to permit persons to whom the Software is \n"\
"// furnished to do so, subject to the following conditions: \n"\
"// \n"\
"// The above copyright notice and this permission notice shall be included in \n"\
"// all copies or substantial portions of the Software. \n"\
"// \n"\
"// THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"// THE SOFTWARE. \n"\
"// \n"\
" \n"\
"layout( local_size_x = 64, local_size_y = 1, local_size_z = 1 ) in; \n"\
" \n"\
"struct bbox \n"\
"{ \n"\
"	vec3 pmin; \n"\
"	vec3 pmax; \n"\
"}; \n"\
" \n"\
"struct HlbvhNode \n"\
"{ \n"\
"	uint parent; \n"\
"	uint left; \n"\
"	uint right; \n"\
"	uint next; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 0 ) buffer restrict MortoncodesBlock \n"\
"{ \n"\
"	uint Mortoncodes[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 1 ) buffer restrict BoundsBlock \n"\
"{ \n"\
"	bbox Bounds[]; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 2 ) buffer restrict readonly NumBlock \n"\
"{ \n"\
"	uint Num; \n"\
"}; \n"\
" \n"\
"layout( std140, binding = 3) buffer restrict readonly IndicesBlock \n"\
"{ \n"\
"	uint Indices[]; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 4 ) buffer NodesBlock \n"\
"{ \n"\
"	HlbvhNode Nodes[]; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 5 ) buffer FlagsBlock \n"\
"{ \n"\
"	uint Flags[]; \n"\
"}; \n"\
" \n"\
"layout( std430, binding = 6) buffer BoundssortedBlock \n"\
"{ \n"\
"	bbox Boundssorted[]; \n"\
"}; \n"\
" \n"\
"uvec2 FindSpan(uint idx); \n"\
"uint FindSplit(uvec2 span); \n"\
" \n"\
"// The following two functions are from \n"\
"// http://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/ \n"\
"// Expands a 10-bit integer into 30 bits \n"\
"// by inserting 2 zeros after each bit. \n"\
"uint ExpandBits( in uint v) \n"\
"{ \n"\
"	v = (v * 0x00010001u) & 0xFF0000FFu; \n"\
"	v = (v * 0x00000101u) & 0x0F00F00Fu; \n"\
"	v = (v * 0x00000011u) & 0xC30C30C3u; \n"\
"	v = (v * 0x00000005u) & 0x49249249u; \n"\
"	return v; \n"\
"} \n"\
" \n"\
"// Calculates a 30-bit Morton code for the \n"\
"// given 3D point located within the unit cube [0,1]. \n"\
"uint CalculateMortonCode( in vec3 p) \n"\
"{ \n"\
"	float x = min(max(p.x * 1024.0f, 0.0f), 1023.0f); \n"\
"	float y = min(max(p.y * 1024.0f, 0.0f), 1023.0f); \n"\
"	float z = min(max(p.z * 1024.0f, 0.0f), 1023.0f); \n"\
"	uint xx = ExpandBits(uint(x)); \n"\
"	uint yy = ExpandBits(uint(y)); \n"\
"	uint zz = ExpandBits(uint(z)); \n"\
"	return xx * 4 + yy * 2 + zz; \n"\
"} \n"\
" \n"\
"void CalcMortonCode() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
" \n"\
"	if (globalID < Num) \n"\
"	{ \n"\
"		bbox bound = Bounds[globalID]; \n"\
"		vec3 center = 0.5f * (bound.pmax + bound.pmin); \n"\
"		Mortoncodes[globalID] = CalculateMortonCode(center); \n"\
"	} \n"\
"} \n"\
" \n"\
"#define LEAFIDX(i) ((Num-1) + i) \n"\
"#define NODEIDX(i) (i) \n"\
" \n"\
"void BuildHierarchy() \n"\
"{ \n"\
"	uint globalID = gl_GlobalInvocationID.x; \n"\
" \n"\
"	// Set child \n"\
"	if (globalID < Num) \n"\
"	{ \n"\
"		Nodes[LEAFIDX(globalID)].left = Nodes[LEAFIDX(globalID)].right = Indices[globalID]; \n"\
"		Boundssorted[LEAFIDX(globalID)] = Bounds[Indices[globalID]]; \n"\
"	} \n"\
"	 \n"\
"	// Set internal nodes \n"\
"	if (globalID < Num - 1) \n"\
"	{ \n"\
"		// Find span occupied by the current node \n"\
"		uvec2 range = FindSpan(globalID); \n"\
" \n"\
"		// Find split position inside the range \n"\
"		uint  split = FindSplit(range); \n"\
" \n"\
"		// Create child nodes if needed \n"\
"		uint c1idx = (split == range.x) ? LEAFIDX(split) : NODEIDX(split); \n"\
"		uint c2idx = (split + 1 == range.y) ? LEAFIDX(split + 1) : NODEIDX(split + 1); \n"\
" \n"\
"		Nodes[NODEIDX(globalID)].left = c1idx; \n"\
"		Nodes[NODEIDX(globalID)].right = c2idx; \n"\
"		Nodes[NODEIDX(globalID)].next = (range.y + 1 < Num) ? range.y + 1 : -1; \n"\
"		Nodes[c1idx].parent = NODEIDX(globalID); \n"\
"		Nodes[c1idx].next = c2idx; \n"\
"		Nodes[c2idx].parent = NODEIDX(globalID); \n"\
"		Nodes[c2idx].next = Nodes[NODEIDX(globalID)].next; \n"\
"	} \n"\
"} \n"\
" \n"\
"bbox bboxunion(bbox b1, bbox b2) \n"\
"{ \n"\
"    bbox res; \n"\
"    res.pmin = min(b1.pmin, b2.pmin); \n"\
"    res.pmax = max(b1.pmax, b2.pmax); \n"\
"    return res; \n"\
"} \n"\
" \n"\
"// Propagate bounds up to the root \n"\
"void RefitBounds() \n"\
"{ \n"\
"    uint globalID = gl_GlobalInvocationID.x; \n"\
" \n"\
"    // Start from leaf nodes \n"\
"    if (globalID < Num) \n"\
"    { \n"\
"        // Get my leaf index \n"\
"        uint idx = LEAFIDX(globalID); \n"\
" \n"\
"        do \n"\
"        { \n"\
"            // Move to parent node \n"\
"            idx = Nodes[idx].parent; \n"\
" \n"\
"            // Check node's flag \n"\
"            if (atomicCompSwap(Flags[ idx ], 0, 1) == 1) \n"\
"            { \n"\
"                // If the flag was 1 the second child is ready and  \n"\
"                // this thread calculates bbox for the node \n"\
" \n"\
"                // Fetch kids \n"\
"                uint lc = Nodes[idx].left; \n"\
"                uint rc = Nodes[idx].right; \n"\
" \n"\
"                // Calculate bounds \n"\
"                bbox b = bboxunion(Bounds[lc], Bounds[rc]); \n"\
" \n"\
"                // Write bounds \n"\
"                Bounds[idx] = b; \n"\
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
" \n"\
"// Calculates longest common prefix length of bit representations \n"\
"// if  representations are equal we consider sucessive indices \n"\
"int delta( in uint i1, in uint i2) \n"\
"{ \n"\
"	// Select left end \n"\
"	uint left = min(i1, i2); \n"\
"	// Select right end \n"\
"	uint right = max(i1, i2); \n"\
"	// This is to ensure the node breaks if the index is out of bounds \n"\
"	if (left < 0 || right >= Num)  \n"\
"	{ \n"\
"		return -1; \n"\
"	} \n"\
"	// Fetch Morton codes for both ends \n"\
"	uint leftcode = Mortoncodes[left]; \n"\
"	uint rightcode = Mortoncodes[right]; \n"\
" \n"\
"	// Special handling of duplicated codes: use their indices as a fallback \n"\
"	return leftcode != rightcode ? findMSB(leftcode ^ rightcode) : (32 + findMSB(left ^ right)); \n"\
"} \n"\
" \n"\
"// Find span occupied by internal node with index idx \n"\
"uvec2 FindSpan(uint idx) \n"\
"{ \n"\
"	// Find the direction of the range \n"\
"	int d = sign( delta(idx, idx+1) - delta(idx, idx-1) ); \n"\
" \n"\
"	// Find minimum number of bits for the break on the other side \n"\
"	int deltamin = delta(idx, idx-d); \n"\
" \n"\
"	// Search conservative far end \n"\
"	uint lmax = 2; \n"\
"	while (delta(idx,idx + lmax * d) > deltamin) \n"\
"		lmax *= 2; \n"\
" \n"\
"	// Search back to find exact bound \n"\
"	// with binary search \n"\
"	uint l = 0; \n"\
"	uint t = lmax; \n"\
"	do \n"\
"	{ \n"\
"		t /= 2; \n"\
"		if(delta(idx, idx + (l + t)*d) > deltamin) \n"\
"		{ \n"\
"			l = l + t; \n"\
"		} \n"\
"	} \n"\
"	while (t > 1); \n"\
" \n"\
"	// Pack span  \n"\
"	uvec2 span; \n"\
"	span.x = min(idx, idx + l*d); \n"\
"	span.y = max(idx, idx + l*d); \n"\
"	return span; \n"\
"} \n"\
" \n"\
" \n"\
"// Find split idx within the span \n"\
"uint FindSplit(uvec2 span) \n"\
"{ \n"\
"	// Fetch codes for both ends \n"\
"	uint left = span.x; \n"\
"	uint right = span.y; \n"\
" \n"\
"	// Calculate the number of identical bits from higher end \n"\
"	uint numidentical = delta(left, right); \n"\
" \n"\
"	do \n"\
"	{ \n"\
"		// Proposed split \n"\
"		uint newsplit = (right + left) / 2; \n"\
" \n"\
"		// If it has more equal leading bits than left and right accept it \n"\
"		if (delta(left, newsplit) > numidentical) \n"\
"		{ \n"\
"			left = newsplit; \n"\
"		} \n"\
"		else \n"\
"		{ \n"\
"			right = newsplit; \n"\
"		} \n"\
"	} \n"\
"	while (right > left + 1); \n"\
" \n"\
"	return left; \n"\
"} \n"\
"/* \n"\
"// Set parent-child relationship \n"\
"__kernel void BuildHierarchy( \n"\
"	// Sorted Morton codes of the primitives \n"\
"	__global int* mortoncodes, \n"\
"	// Bounds \n"\
"	__global bbox* bounds, \n"\
"	// Primitive indices \n"\
"	__global int* indices, \n"\
"	// Number of primitives \n"\
"	int numprims, \n"\
"	// Nodes \n"\
"	__global HlbvhNode* nodes, \n"\
"	// Leaf bounds \n"\
"	__global bbox* boundssorted \n"\
"	) \n"\
"{ \n"\
"	int globalid = get_global_id(0); \n"\
" \n"\
"	// Set child \n"\
"	if (globalid < numprims) \n"\
"	{ \n"\
"		nodes[LEAFIDX(globalid)].left = nodes[LEAFIDX(globalid)].right = indices[globalid]; \n"\
"		boundssorted[LEAFIDX(globalid)] = bounds[indices[globalid]]; \n"\
"	} \n"\
"	 \n"\
"	// Set internal nodes \n"\
"	if (globalid < numprims - 1) \n"\
"	{ \n"\
"		// Find span occupied by the current node \n"\
"		int2 range = FindSpan(mortoncodes, numprims, globalid); \n"\
" \n"\
"		// Find split position inside the range \n"\
"		int  split = FindSplit(mortoncodes, numprims, range); \n"\
" \n"\
"		// Create child nodes if needed \n"\
"		int c1idx = (split == range.x) ? LEAFIDX(split) : NODEIDX(split); \n"\
"		int c2idx = (split + 1 == range.y) ? LEAFIDX(split + 1) : NODEIDX(split + 1); \n"\
" \n"\
"		nodes[NODEIDX(globalid)].left = c1idx; \n"\
"		nodes[NODEIDX(globalid)].right = c2idx; \n"\
"		//nodes[NODEIDX(globalid)].next = (range.y + 1 < numprims) ? range.y + 1 : -1; \n"\
"		nodes[c1idx].parent = NODEIDX(globalid); \n"\
"		//nodes[c1idx].next = c2idx; \n"\
"		nodes[c2idx].parent = NODEIDX(globalid); \n"\
"		//nodes[c2idx].next = nodes[NODEIDX(globalid)].next; \n"\
"	} \n"\
"} \n"\
" \n"\
"// Propagate bounds up to the root \n"\
"__kernel void RefitBounds(__global bbox* bounds, \n"\
"						  int numprims, \n"\
"						  __global HlbvhNode* nodes, \n"\
"						  __global int* flags \n"\
"						  ) \n"\
"{ \n"\
"	int globalid = get_global_id(0); \n"\
" \n"\
"	// Start from leaf nodes \n"\
"	if (globalid < numprims) \n"\
"	{ \n"\
"		// Get my leaf index \n"\
"		int idx = LEAFIDX(globalid); \n"\
" \n"\
"		do \n"\
"		{ \n"\
"			// Move to parent node \n"\
"			idx = nodes[idx].parent; \n"\
" \n"\
"			// Check node's flag \n"\
"			if (atomic_cmpxchg(flags + idx, 0, 1) == 1) \n"\
"			{ \n"\
"				// If the flag was 1 the second child is ready and  \n"\
"				// this thread calculates bbox for the node \n"\
" \n"\
"				// Fetch kids \n"\
"				int lc = nodes[idx].left; \n"\
"				int rc = nodes[idx].right; \n"\
" \n"\
"				// Calculate bounds \n"\
"				bbox b = bboxunion(bounds[lc], bounds[rc]); \n"\
" \n"\
"				// Write bounds \n"\
"				bounds[idx] = b; \n"\
"			} \n"\
"			else \n"\
"			{ \n"\
"				// If the flag was 0 set it to 1 and bail out. \n"\
"				// The thread handling the second child will \n"\
"				// handle this node. \n"\
"				break; \n"\
"			} \n"\
"		} \n"\
"		while (idx != 0); \n"\
"	} \n"\
"} \n"\
" \n"\
"*/ \n"\
;
