/**********************************************************************
Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef BVH2IL_HLSL
#define BVH2IL_HLSL

#ifndef HOST
#include "aabb.hlsl"
#endif

#define BVH_NODE_SIZE 64
#define BVH_NODE_STRIDE_SHIFT 6
#define BVH_NODE_COUNT(N) (2 * (N)-1)
#define BVH_NODE_BYTE_OFFSET(i) ((i) << BVH_NODE_STRIDE_SHIFT)

#define LEAF_NODE_INDEX(i, n) ((n - 1) + i)
#define INTERNAL_NODE_INDEX(i, n) (i)
#define IS_INTERNAL_NODE(i, n) ((i < n - 1) ? true : false)
#define IS_LEAF_NODE(i, n) (!IS_INTERNAL_NODE(i, n))

//< BVH2 node.
struct BvhNode
{
    uint child0;
    uint child1;
    uint parent;
    uint update;

    float3 aabb0_min_or_v0;
    float3 aabb0_max_or_v1;
    float3 aabb1_min_or_v2;
    float3 aabb1_max_or_v3;
};

#ifndef HOST
#ifdef TOP_LEVEL
//< Calculate BVH2 node bounding box.
Aabb GetNodeAabb(in BvhNode node, bool internal)
{
    Aabb aabb = CreateEmptyAabb();

    if (internal)
    {
        // 3 vertices or 3 points for both internal and leafs.
        GrowAabb(node.aabb0_min_or_v0, aabb);
        GrowAabb(node.aabb0_max_or_v1, aabb);
        GrowAabb(node.aabb1_min_or_v2, aabb);
        GrowAabb(node.aabb1_max_or_v3, aabb);
    } else
    {
        aabb.pmin = node.aabb0_min_or_v0;
        aabb.pmax = node.aabb0_max_or_v1;
    }

    return aabb;
}
#else
//< Calculate BVH2 node bounding box.
Aabb GetNodeAabb(in BvhNode node, bool internal)
{
    Aabb aabb = CreateEmptyAabb();

    // 3 vertices or 3 points for both internal and leafs.
    GrowAabb(node.aabb0_min_or_v0, aabb);
    GrowAabb(node.aabb0_max_or_v1, aabb);
    GrowAabb(node.aabb1_min_or_v2, aabb);

    if (internal)
    {
        // Internal node - one more point.
        GrowAabb(node.aabb1_max_or_v3, aabb);
    }

    return aabb;
}
#endif
#endif

#endif  // BVH2IL_HLSL