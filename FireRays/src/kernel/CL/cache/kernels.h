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
static const char cl_hlbvh[]= \
"// Placeholder \n"\
;
static const char cl_hlbvh_build[]= \
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
"    __global float3* positions, \n"\
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
"        mortoncodes[globalid] = CalculateMortonCode(positions[globalid]); \n"\
"    } \n"\
"} \n"\
" \n"\
"typedef struct \n"\
"{ \n"\
"    float3 pmin; \n"\
"    float3 pmax; \n"\
"} bbox; \n"\
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
"    return leftcode != rightcode ? clz(leftcode ^ rightcode) : clz(left ^ right); \n"\
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
" \n"\
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
"            // Chek node's flag \n"\
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
static const char cl_qbvh[]= \
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
static const char cl_bvh2l[]= \
"/************************************************************************* \n"\
" INCLUDES \n"\
" **************************************************************************/ \n"\
"typedef struct _bbox \n"\
"    { \n"\
"        float4 pmin; \n"\
"        float4 pmax; \n"\
"    } bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"    { \n"\
"        float4 o; \n"\
"        float4 d; \n"\
"    } ray; \n"\
" \n"\
"typedef struct __attribute__ ((packed)) _Intersection \n"\
"    { \n"\
"        float4 uvwt; \n"\
"        int shapeid; \n"\
"        int primid; \n"\
"    } Intersection; \n"\
" \n"\
" \n"\
"typedef struct __attribute__ ((packed)) _BvhNode \n"\
"    { \n"\
"        bbox bounds; \n"\
"        // Lower 16 bits node type: 1 - internal, 0 - leaf \n"\
"        // Higher 16 bits primitive count in case of a leaf node \n"\
"        uint typecnt; \n"\
"        uint padding; \n"\
"    } BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"    { \n"\
"        // Vertex indices \n"\
"        int idx[4]; \n"\
"        // Primitive ID \n"\
"        int id; \n"\
"        // Idx count \n"\
"        int cnt; \n"\
"    } Face; \n"\
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
"    res.o.xyz = transform_point(r.o.xyz, m); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m); \n"\
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
"bool IntersectTriangle(ray* r, float3 v1, float3 v2, float3 v3, Intersection* isect) \n"\
"{ \n"\
"    float3 e1 = v2 - v1; \n"\
"    float3 e2 = v3 - v1; \n"\
"    float3 s1 = cross(r->d.xyz, e2); \n"\
"    float  invd = 1.f / dot(s1, e1); \n"\
"    float3 d = r->o.xyz - v1; \n"\
"    float  b1 = dot(d, s1) * invd; \n"\
"    float3 s2 = cross(d, e1); \n"\
"    float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > isect->uvwt.w) \n"\
"    { \n"\
"        return false; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        isect->uvwt = make_float4(b1, b2, 0.f, temp); \n"\
"        return true; \n"\
"    } \n"\
"} \n"\
" \n"\
"bool IntersectTriangleP(ray* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 e1 = v2 - v1; \n"\
"    float3 e2 = v3 - v1; \n"\
"    float3 s1 = cross(r->d.xyz, e2); \n"\
"    float  invd = 1.f / dot(s1, e1); \n"\
"    float3 d = r->o.xyz - v1; \n"\
"    float  b1 = dot(d, s1) * invd; \n"\
"    float3 s2 = cross(d, e1); \n"\
"    float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > r->o.w) \n"\
"    { \n"\
"        return false; \n"\
"    } \n"\
"     \n"\
"    return true; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"bool IntersectBox(ray* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
"     \n"\
"    float3 tmax = max(f, n); \n"\
"    float3 tmin = min(f, n); \n"\
"     \n"\
"    float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
"     \n"\
"    return (t1 >= t0); \n"\
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
"typedef struct __attribute__ ((packed)) _ShapeData \n"\
"{ \n"\
"    float16 minv; \n"\
"    float4  linearvelocity; \n"\
"    float4  angularvelocity; \n"\
"} ShapeData; \n"\
" \n"\
"#define NUMPRIMS(x)     (((x).typecnt) >> 1) \n"\
"#define BVHROOTIDX(x)   ((int)((x).bounds.pmin.w)) \n"\
"#define SHAPEDATAIDX(x) (((x).typecnt) >> 1) \n"\
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
"    // Transforms \n"\
"    __global ShapeData*     shapedata; \n"\
"    // Root BVH idx \n"\
"    int rootidx; \n"\
"} SceneData; \n"\
" \n"\
" \n"\
"/************************************************************************* \n"\
"BVH FUNCTIONS \n"\
"**************************************************************************/ \n"\
"//  intersect a ray with leaf BVH node \n"\
"bool IntersectLeafClosest( \n"\
"    SceneData* scenedata, \n"\
"    BvhNode* node, \n"\
"    ray* r,                      // ray to instersect \n"\
"    Intersection* isect          // Intersection structure \n"\
"    ) \n"\
"{ \n"\
"    bool hit = false; \n"\
"    int thishit = 0; \n"\
"    float3 v1, v2, v3, v4; \n"\
"    Face face; \n"\
" \n"\
"    int start = (int)(node->bounds.pmin.w); \n"\
"    for (int i = 0; i<4; ++i) \n"\
"    { \n"\
"        thishit = false; \n"\
"        face = scenedata->faces[start + i]; \n"\
"        v1 = scenedata->vertices[face.idx[0]]; \n"\
"        v2 = scenedata->vertices[face.idx[1]]; \n"\
"        v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"        switch (face.cnt) \n"\
"        { \n"\
"        case 3: \n"\
"        { \n"\
"                  // Triangle \n"\
"                  if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"                  { \n"\
"                      thishit = 1; \n"\
"                  } \n"\
" \n"\
"                  break; \n"\
"        } \n"\
"        case 4: \n"\
"        { \n"\
"                  // Quad \n"\
"                  v4 = scenedata->vertices[face.idx[3]]; \n"\
"                  if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"                  { \n"\
"                      thishit = 1; \n"\
"                  } \n"\
"                  else if (IntersectTriangle(r, v3, v4, v1, isect)) \n"\
"                  { \n"\
"                      thishit = 2; \n"\
"                  } \n"\
" \n"\
"                  break; \n"\
"        } \n"\
"        } \n"\
" \n"\
"        if (thishit > 0) \n"\
"        { \n"\
"            hit = true; \n"\
"            isect->primid = face.id; \n"\
"            isect->uvwt.z = thishit - 1; \n"\
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
"    Face face; \n"\
" \n"\
"    int start = (int)(node->bounds.pmin.w); \n"\
"    for (int i = 0; i < 4; ++i) \n"\
"    { \n"\
"         \n"\
"        face = scenedata->faces[start + i]; \n"\
"        v1 = scenedata->vertices[face.idx[0]]; \n"\
"        v2 = scenedata->vertices[face.idx[1]]; \n"\
"        v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"        switch (face.cnt) \n"\
"        { \n"\
"        case 3: \n"\
"        { \n"\
"                  // Triangle \n"\
"                  if (IntersectTriangleP(r, v1, v2, v3)) \n"\
"                  { \n"\
"                      return true; \n"\
"                  } \n"\
" \n"\
"                  break; \n"\
"        } \n"\
"        case 4: \n"\
"        { \n"\
"                  v4 = scenedata->vertices[face.idx[3]]; \n"\
"                  // Quad \n"\
"                  if (IntersectTriangleP(r, v1, v2, v3) || IntersectTriangleP(r, v3, v4, v1)) \n"\
"                  { \n"\
"                      return true; \n"\
"                  } \n"\
" \n"\
"                  break; \n"\
"        } \n"\
"        } \n"\
" \n"\
"        if (i + 1 >= NUMPRIMS(*node)) return false; \n"\
"    } \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
" \n"\
"// intersect Ray against the whole BVH2L structure \n"\
"bool IntersectSceneClosest2L(SceneData* scenedata,  ray* r, Intersection* isect) \n"\
"{ \n"\
"    // Init intersection \n"\
"    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w); \n"\
"    isect->shapeid = -1; \n"\
"    isect->primid = -1; \n"\
" \n"\
"    // Precompute invdir for bbox testing \n"\
"    float3 invdir  = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"    float3 invdirtop = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"    // We need to keep original ray around for returns from bottom hierarchy \n"\
"    ray topray = *r; \n"\
"     \n"\
"    // Fetch top level BVH index \n"\
"    int idx = scenedata->rootidx; \n"\
"    // 0xFFFFFFFF indicates we are traversing top level \n"\
"    int topidx = -1; \n"\
"    while (idx != -1) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node.bounds, isect->uvwt.w)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy) \n"\
"                // or containing another BVH (top level hierarhcy) \n"\
"                if (topidx != -1) \n"\
"                { \n"\
"                    // This is bottom level, so intersect with a primitives \n"\
"                    if (IntersectLeafClosest(scenedata, &node, r, isect)) \n"\
"                    { \n"\
"                        // Adjust shapeid as it might be instance \n"\
"                        isect->shapeid = scenedata->nodes[topidx].padding; \n"\
"                    } \n"\
" \n"\
"                    // And goto next node \n"\
"                    idx = (int)(node.bounds.pmax.w); \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // This is top level hierarchy leaf \n"\
"                    // Save top node index for return \n"\
"                    topidx = idx; \n"\
"                    // Fetch bottom level BVH index \n"\
"                    idx = BVHROOTIDX(node); \n"\
"                    // Fetch BVH transform \n"\
"                    float16 wmi = scenedata->shapedata[SHAPEDATAIDX(node)].minv; \n"\
"                    // Apply linear motion blur (world coordinates) \n"\
"                    float4 lmv = scenedata->shapedata[SHAPEDATAIDX(node)].linearvelocity; \n"\
"                    //float4 amv = scenedata->shapedata[SHAPEDATAIDX(node)].angularvelocity; \n"\
"                     r->o.xyz -= (lmv.xyz*r->d.w); \n"\
"                    // Transfrom the ray \n"\
"                    *r = transform_ray(*r, wmi); \n"\
"                    //rotate_ray(r, amv); \n"\
"                    // Recalc invdir \n"\
"                    invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"                    // And continue traversal of the bottom level BVH \n"\
"                    continue; \n"\
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
"            idx = (int)(node.bounds.pmax.w); \n"\
"        } \n"\
"         \n"\
"        // Here check if we ended up traversing bottom level BVH \n"\
"        // in this case idx = 0xFFFFFFFF and topidx has valid value \n"\
"        if (idx == -1 && topidx != -1) \n"\
"        { \n"\
"            //  Proceed to next top level node \n"\
"            idx = (int)(scenedata->nodes[topidx].bounds.pmax.w); \n"\
"            // Set topidx \n"\
"            topidx = -1; \n"\
"            // Restore ray here \n"\
"            r->o = topray.o; \n"\
"            r->d = topray.d; \n"\
"            // Restore invdir \n"\
"            invdir = invdirtop; \n"\
"        } \n"\
"    } \n"\
"     \n"\
"    return isect->shapeid >= 0; \n"\
"} \n"\
" \n"\
"// intersect Ray against the whole BVH2L structure \n"\
"bool IntersectSceneAny2L(SceneData* scenedata,  ray* r) \n"\
"{ \n"\
"    // Precompute invdir for bbox testing \n"\
"    float3 invdir  = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"    float3 invdirtop = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"    // We need to keep original ray around for returns from bottom hierarchy \n"\
"    ray topray = *r; \n"\
"     \n"\
"    // Fetch top level BVH index \n"\
"    int idx = scenedata->rootidx; \n"\
"    // -1 indicates we are traversing top level \n"\
"    int topidx = -1; \n"\
"    while (idx != -1) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node.bounds, r->o.w)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                // If this is the leaf it can be either a leaf containing primitives (bottom hierarchy) \n"\
"                // or containing another BVH (top level hierarhcy) \n"\
"                if (topidx != -1) \n"\
"                { \n"\
"                    // This is bottom level, so intersect with a primitives \n"\
"                    if(IntersectLeafAny(scenedata, &node, r)) \n"\
"                        return true; \n"\
"                    // And goto next node \n"\
"                    idx = (int)(node.bounds.pmax.w); \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    // This is top level hierarchy leaf \n"\
"                    // Save top node index for return \n"\
"                    topidx = idx; \n"\
"                    // Fetch bottom level BVH index \n"\
"                    idx = BVHROOTIDX(node); \n"\
"                    // Fetch BVH transform \n"\
"                    float16 wmi = scenedata->shapedata[SHAPEDATAIDX(node)].minv; \n"\
"					// Apply linear motion blur (world coordinates) \n"\
"					float4 lmv = scenedata->shapedata[SHAPEDATAIDX(node)].linearvelocity; \n"\
"					//float4 amv = scenedata->shapedata[SHAPEDATAIDX(node)].angularvelocity; \n"\
"					r->o.xyz -= (lmv.xyz*r->d.w); \n"\
"                    // Transfrom the ray \n"\
"                    *r = transform_ray(*r, wmi); \n"\
"					//rotate_ray(r, amv); \n"\
"                    // Recalc invdir \n"\
"                    invdir = make_float3(1.f, 1.f, 1.f) / r->d.xyz; \n"\
"                    // And continue traversal of the bottom level BVH \n"\
"                    continue; \n"\
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
"            idx = (int)(node.bounds.pmax.w); \n"\
"        } \n"\
"         \n"\
"        // Here check if we ended up traversing bottom level BVH \n"\
"        // in this case idx = 0xFFFFFFFF and topidx has valid value \n"\
"        if (idx == -1 && topidx != -1) \n"\
"        { \n"\
"            //  Proceed to next top level node \n"\
"            idx = (int)(scenedata->nodes[topidx].bounds.pmax.w); \n"\
"            // Set topidx \n"\
"            topidx = -1; \n"\
"            // Restore ray here \n"\
"            *r = topray; \n"\
"            // Restore invdir \n"\
"            invdir = invdirtop; \n"\
"        } \n"\
"    } \n"\
"     \n"\
"    return false; \n"\
"} \n"\
" \n"\
" \n"\
"// 2 level variants \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectClosest2L( \n"\
"                               // Input \n"\
"                               __global BvhNode* nodes,   // BVH nodes \n"\
"                               __global float3* vertices, // Scene positional data \n"\
"                               __global Face* faces,    // Scene indices \n"\
"                               __global int* shapeids,    // Shape IDs \n"\
"                               __global ShapeData* shapedata, // Transforms \n"\
"                               int rootidx,               // BVH root idx \n"\
"                               __global ray* rays,        // Ray workload \n"\
"                               int offset,                // Offset in rays array \n"\
"                               int numrays,               // Number of rays to process \n"\
"                               __global Intersection* hits // Hit datas \n"\
"                               ) \n"\
"{ \n"\
"     \n"\
"    int global_id  = get_global_id(0); \n"\
"     \n"\
"    // Fill scene data \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids, \n"\
"        shapedata, \n"\
"        rootidx \n"\
"    }; \n"\
"     \n"\
"    // Handle only working subset \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        int idx = offset + global_id; \n"\
"        ray r = rays[idx]; \n"\
"         \n"\
"        // Calculate closest hit \n"\
"        Intersection isect; \n"\
"        IntersectSceneClosest2L(&scenedata, &r, &isect); \n"\
" \n"\
"        // Write data back in case of a hit \n"\
"        hits[idx] = isect; \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"__kernel void IntersectAny2L( \n"\
"                           // Input \n"\
"                           __global BvhNode* nodes,   // BVH nodes \n"\
"                           __global float3* vertices, // Scene positional data \n"\
"                           __global Face* faces,    // Scene indices \n"\
"                           __global int* shapeids,    // Shape IDs \n"\
"                           __global ShapeData* shapedata, // Transforms \n"\
"                           int rootidx,               // BVH root idx \n"\
"                           __global ray* rays,        // Ray workload \n"\
"                           int offset,                // Offset in rays array \n"\
"                           int numrays,               // Number of rays to process \n"\
"                           __global int* hitresults   // Hit results \n"\
"                           ) \n"\
"{ \n"\
"    int global_id  = get_global_id(0); \n"\
"     \n"\
"    // Fill scene data \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids, \n"\
"        shapedata, \n"\
"        rootidx \n"\
"    }; \n"\
"     \n"\
"    // Handle only working subset \n"\
"    if (global_id < numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[offset + global_id]; \n"\
"         \n"\
"        // Calculate any intersection \n"\
"        hitresults[offset + global_id] = IntersectSceneAny2L(&scenedata, &r) ? 1 : -1; \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectClosestRC2L( \n"\
"                                 // Input \n"\
"                                 __global BvhNode* nodes,   // BVH nodes \n"\
"                                 __global float3* vertices, // Scene positional data \n"\
"                                 __global Face* faces,    // Scene indices \n"\
"                                 __global int* shapeids,     // Shape IDs \n"\
"                                 __global ShapeData* shapedata, // Transforms \n"\
"                                 int rootidx,               // BVH root idx \n"\
"                                 __global ray* rays,        // Ray workload \n"\
"                                 __global int* numrays,     // Number of rays in the workload \n"\
"                                 int offset,                // Offset in rays array \n"\
"                                 __global Intersection* hits // Hit datas \n"\
"                                 ) \n"\
"{ \n"\
"    int global_id  = get_global_id(0); \n"\
" \n"\
"    // Fill scene data \n"\
"    SceneData scenedata = \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids, \n"\
"        shapedata, \n"\
"        rootidx \n"\
"    }; \n"\
"     \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        int idx = offset + global_id; \n"\
"        ray r = rays[idx]; \n"\
"         \n"\
"        // Calculate closest hit \n"\
"        Intersection isect; \n"\
"        IntersectSceneClosest2L(&scenedata, &r, &isect); \n"\
"         \n"\
"        hits[idx] = isect; \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectAnyRC2L( \n"\
"                             // Input \n"\
"                             __global BvhNode* nodes,   // BVH nodes \n"\
"                             __global float3* vertices, // Scene positional data \n"\
"                             __global Face* faces,    // Scene indices \n"\
"                             __global int* shapeids,     // Shape IDs \n"\
"                             __global ShapeData* shapedata, // Transforms \n"\
"                             int rootidx,               // BVH root idx \n"\
"                             __global ray* rays,        // Ray workload \n"\
"                             __global int* numrays,     // Number of rays in the workload \n"\
"                             int offset,                // Offset in rays array \n"\
"                             __global int* hitresults   // Hit results \n"\
"                             ) \n"\
"{ \n"\
"    int global_id  = get_global_id(0); \n"\
"     \n"\
"    // Fill scene data  \n"\
"    SceneData scenedata =  \n"\
"    { \n"\
"        nodes, \n"\
"        vertices, \n"\
"        faces, \n"\
"        shapeids, \n"\
"        shapedata, \n"\
"        rootidx \n"\
"    }; \n"\
"     \n"\
"    // Handle only working subset \n"\
"    if (global_id < *numrays) \n"\
"    { \n"\
"        // Fetch ray \n"\
"        ray r = rays[offset + global_id]; \n"\
"         \n"\
"        // Calculate any intersection \n"\
"        hitresults[offset + global_id] = IntersectSceneAny2L(&scenedata, &r) ? 1 : -1; \n"\
"    } \n"\
"} \n"\
;
static const char cl_bvh[]= \
"/************************************************************************* \n"\
" INCLUDES \n"\
" **************************************************************************/ \n"\
"typedef struct _bbox \n"\
"    { \n"\
"        float4 pmin; \n"\
"        float4 pmax; \n"\
"    } bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"    { \n"\
"        float4 o; \n"\
"        float4 d; \n"\
"    } ray; \n"\
" \n"\
"typedef struct __attribute__ ((packed)) _Intersection \n"\
"    { \n"\
"        float4 uvwt; \n"\
"        int shapeid; \n"\
"        int primid; \n"\
"    } Intersection; \n"\
" \n"\
" \n"\
"typedef struct __attribute__ ((packed)) _BvhNode \n"\
"    { \n"\
"        bbox bounds; \n"\
"        // Lower 16 bits node type: 1 - internal, 0 - leaf \n"\
"        // Higher 16 bits primitive count in case of a leaf node \n"\
"        uint typecnt; \n"\
"        uint padding; \n"\
"    } BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"    { \n"\
"        // Vertex indices \n"\
"        int idx[4]; \n"\
"        // Primitive ID \n"\
"        int id; \n"\
"        // Idx count \n"\
"        int cnt; \n"\
"    } Face; \n"\
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
"    res.o.xyz = transform_point(r.o.xyz, m); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m); \n"\
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
"bool IntersectTriangle(ray* r, float3 v1, float3 v2, float3 v3, Intersection* isect) \n"\
"{ \n"\
"    float3 e1 = v2 - v1; \n"\
"    float3 e2 = v3 - v1; \n"\
"    float3 s1 = cross(r->d.xyz, e2); \n"\
"    float  invd = 1.f / dot(s1, e1); \n"\
"    float3 d = r->o.xyz - v1; \n"\
"    float  b1 = dot(d, s1) * invd; \n"\
"    float3 s2 = cross(d, e1); \n"\
"    float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > isect->uvwt.w) \n"\
"    { \n"\
"        return false; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        isect->uvwt = make_float4(b1, b2, 0.f, temp); \n"\
"        return true; \n"\
"    } \n"\
"} \n"\
" \n"\
"bool IntersectTriangleP(ray* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 e1 = v2 - v1; \n"\
"    float3 e2 = v3 - v1; \n"\
"    float3 s1 = cross(r->d.xyz, e2); \n"\
"    float  invd = 1.f / dot(s1, e1); \n"\
"    float3 d = r->o.xyz - v1; \n"\
"    float  b1 = dot(d, s1) * invd; \n"\
"    float3 s2 = cross(d, e1); \n"\
"    float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > r->o.w) \n"\
"    { \n"\
"        return false; \n"\
"    } \n"\
"     \n"\
"    return true; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"bool IntersectBox(ray* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
"     \n"\
"    float3 tmax = max(f, n); \n"\
"    float3 tmin = min(f, n); \n"\
"     \n"\
"    float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
"     \n"\
"    return (t1 >= t0); \n"\
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
"#define NUMPRIMS(x)     (((x).typecnt) >> 1) \n"\
"#define BVHROOTIDX(x)   ((int)((x).bounds.pmin.w)) \n"\
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
"bool IntersectLeafClosest(  \n"\
"    SceneData* scenedata, \n"\
"    BvhNode* node, \n"\
"    ray* r,                      // ray to instersect \n"\
"    Intersection* isect          // Intersection structure \n"\
"    ) \n"\
"{ \n"\
"    bool hit = false; \n"\
"    int thishit = 0; \n"\
"    float3 v1, v2, v3, v4; \n"\
"    Face face; \n"\
" \n"\
"    int start = (int)(node->bounds.pmin.w); \n"\
"    for (int i=0; i<4; ++i) \n"\
"    { \n"\
"        thishit = false; \n"\
" \n"\
"        face = scenedata->faces[start + i]; \n"\
"        v1 = scenedata->vertices[face.idx[0]]; \n"\
"        v2 = scenedata->vertices[face.idx[1]]; \n"\
"        v3 = scenedata->vertices[face.idx[2]]; \n"\
" \n"\
"        switch (face.cnt) \n"\
"        { \n"\
"        case 3: \n"\
"            { \n"\
"                // Triangle \n"\
"                if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
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
"                if (IntersectTriangle(r, v1, v2, v3, isect)) \n"\
"                { \n"\
"                   thishit = 1; \n"\
"                } \n"\
"                else if (IntersectTriangle(r, v3, v4, v1, isect)) \n"\
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
"            isect->primid = face.id; \n"\
"            isect->shapeid = scenedata->shapeids[start + i]; \n"\
"            isect->uvwt.z = thishit - 1; \n"\
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
"    int start = (int)(node->bounds.pmin.w); \n"\
"    for (int i = 0; i < 4; ++i) \n"\
"    { \n"\
"        face = scenedata->faces[start + i]; \n"\
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
"        if (i + 1 >= NUMPRIMS(*node)) return false; \n"\
"    } \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneClosest(SceneData* scenedata,  ray* r, Intersection* isect) \n"\
"{ \n"\
"    float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d.xyz; \n"\
" \n"\
"    isect->uvwt = make_float4(0.f, 0.f, 0.f, r->o.w); \n"\
"    isect->shapeid = -1; \n"\
"    isect->primid = -1; \n"\
" \n"\
"    uint idx = 0; \n"\
"    while (idx != -1) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        // If this is the leaf try to intersect against contained triangle. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node.bounds, isect->uvwt.w)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                IntersectLeafClosest(scenedata, &node, r, isect); \n"\
"                idx = (int)(node.bounds.pmax.w); \n"\
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
"            idx = (int)(node.bounds.pmax.w); \n"\
"        } \n"\
"    }; \n"\
" \n"\
"    return isect->shapeid >= 0; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"// intersect Ray against the whole BVH structure \n"\
"bool IntersectSceneAny(SceneData* scenedata,  ray* r) \n"\
"{ \n"\
"    float3 invdir  = make_float3(1.f, 1.f, 1.f)/r->d.xyz; \n"\
" \n"\
"    uint idx = 0; \n"\
"    while (idx != -1) \n"\
"    { \n"\
"        // Try intersecting against current node's bounding box. \n"\
"        // If this is the leaf try to intersect against contained triangle. \n"\
"        BvhNode node = scenedata->nodes[idx]; \n"\
"        if (IntersectBox(r, invdir, node.bounds, r->o.w)) \n"\
"        { \n"\
"            if (LEAFNODE(node)) \n"\
"            { \n"\
"                if (IntersectLeafAny(scenedata, &node, r)) \n"\
"                { \n"\
"                    return true; \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    idx = (int)(node.bounds.pmax.w); \n"\
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
"            idx = (int)(node.bounds.pmax.w); \n"\
"        } \n"\
"    }; \n"\
" \n"\
"    return false; \n"\
"} \n"\
" \n"\
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
"    int numrays,               // Number of rays to process \n"\
"    __global Intersection* hits // Hit datas \n"\
"    ) \n"\
"{ \n"\
"    int global_id  = get_global_id(0); \n"\
"    int local_id  = get_local_id(0); \n"\
"    int global_size = get_global_size(0); \n"\
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
"        IntersectSceneClosest(&scenedata, &r, &isect); \n"\
" \n"\
"        // Write data back in case of a hit \n"\
"        hits[idx] = isect; \n"\
"    } \n"\
"} \n"\
" \n"\
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
"        hitresults[offset + global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectClosestRC( \n"\
"    __global BvhNode* nodes,   // BVH nodes \n"\
"    __global float3* vertices, // Scene positional data \n"\
"    __global Face* faces,      // Scene indices \n"\
"    __global int* shapeids,     // Shape IDs \n"\
"    __global ray* rays,        // Ray workload \n"\
"    int offset,                // Offset in rays array \n"\
"    __global int* numrays,     // Number of rays in the workload \n"\
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
"        IntersectSceneClosest(&scenedata, &r, &isect); \n"\
" \n"\
"        // Write data back in case of a hit \n"\
"        hits[idx] = isect; \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(64, 1, 1))) \n"\
"// Version with range check \n"\
"__kernel void IntersectAnyRC( \n"\
"    // Input \n"\
"    __global BvhNode* nodes,   // BVH nodes \n"\
"    __global float3* vertices, // Scene positional data \n"\
"    __global Face* faces,    // Scene indices \n"\
"    __global int* shapeids,     // Shape IDs \n"\
"    __global ray* rays,        // Ray workload \n"\
"    int offset,                // Offset in rays array \n"\
"    __global int* numrays,     // Number of rays in the workload \n"\
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
"        hitresults[offset + global_id] = IntersectSceneAny(&scenedata, &r) ? 1 : -1; \n"\
"    } \n"\
"} \n"\
;
static const char cl_common[]= \
"typedef struct _bbox \n"\
"    { \n"\
"        float4 pmin; \n"\
"        float4 pmax; \n"\
"    } bbox; \n"\
" \n"\
"typedef struct _ray \n"\
"    { \n"\
"        float4 o; \n"\
"        float4 d; \n"\
"    } ray; \n"\
" \n"\
"typedef struct __attribute__ ((packed)) _Intersection \n"\
"    { \n"\
"        float4 uvwt; \n"\
"        int shapeid; \n"\
"        int primid; \n"\
"    } Intersection; \n"\
" \n"\
" \n"\
"typedef struct __attribute__ ((packed)) _BvhNode \n"\
"    { \n"\
"        bbox bounds; \n"\
"        // Lower 16 bits node type: 1 - internal, 0 - leaf \n"\
"        // Higher 16 bits primitive count in case of a leaf node \n"\
"        uint typecnt; \n"\
"        uint padding; \n"\
"    } BvhNode; \n"\
" \n"\
"typedef struct _Face \n"\
"    { \n"\
"        // Vertex indices \n"\
"        int idx[4]; \n"\
"        // Primitive ID \n"\
"        int id; \n"\
"        // Idx count \n"\
"        int cnt; \n"\
"    } Face; \n"\
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
"    res.o.xyz = transform_point(r.o.xyz, m); \n"\
"    res.d.xyz = transform_vector(r.d.xyz, m); \n"\
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
"bool IntersectTriangle(ray* r, float3 v1, float3 v2, float3 v3, Intersection* isect) \n"\
"{ \n"\
"    float3 e1 = v2 - v1; \n"\
"    float3 e2 = v3 - v1; \n"\
"    float3 s1 = cross(r->d.xyz, e2); \n"\
"    float  invd = 1.f / dot(s1, e1); \n"\
"    float3 d = r->o.xyz - v1; \n"\
"    float  b1 = dot(d, s1) * invd; \n"\
"    float3 s2 = cross(d, e1); \n"\
"    float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > isect->uvwt.w) \n"\
"    { \n"\
"        return false; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        isect->uvwt = make_float4(b1, b2, 0.f, temp); \n"\
"        return true; \n"\
"    } \n"\
"} \n"\
" \n"\
"bool IntersectTriangleP(ray* r, float3 v1, float3 v2, float3 v3) \n"\
"{ \n"\
"    float3 e1 = v2 - v1; \n"\
"    float3 e2 = v3 - v1; \n"\
"    float3 s1 = cross(r->d.xyz, e2); \n"\
"    float  invd = 1.f / dot(s1, e1); \n"\
"    float3 d = r->o.xyz - v1; \n"\
"    float  b1 = dot(d, s1) * invd; \n"\
"    float3 s2 = cross(d, e1); \n"\
"    float  b2 = dot(r->d.xyz, s2) * invd; \n"\
"    float temp = dot(e2, s2) * invd; \n"\
"     \n"\
"    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f \n"\
"        || temp < 0.f || temp > r->o.w) \n"\
"    { \n"\
"        return false; \n"\
"    } \n"\
"     \n"\
"    return true; \n"\
"} \n"\
" \n"\
"// Intersect ray with the axis-aligned box \n"\
"bool IntersectBox(ray* r, float3 invdir, bbox box, float maxt) \n"\
"{ \n"\
"    float3 f = (box.pmax.xyz - r->o.xyz) * invdir; \n"\
"    float3 n = (box.pmin.xyz - r->o.xyz) * invdir; \n"\
"     \n"\
"    float3 tmax = max(f, n); \n"\
"    float3 tmin = min(f, n); \n"\
"     \n"\
"    float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt); \n"\
"    float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f); \n"\
"     \n"\
"    return (t1 >= t0); \n"\
"} \n"\
;
