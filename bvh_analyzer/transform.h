/**********************************************************************
Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.

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

#pragma once
#include <queue>

#include "bvh_node.h"
#include "triangle.h"

namespace bvh
{
struct VkBvhNode
{
    float    aabb0_min_or_v0[3];
    uint32_t child0;
    float    aabb0_max_or_v1[3];
    uint32_t child1;
    float    aabb1_min_or_v2[3];
    uint32_t parent;
    float    aabb1_max_or_v3[3];
    uint32_t update;
};

struct DxBvhNode
{
    uint32_t child0;
    uint32_t child1;
    uint32_t parent;
    uint32_t update;

    float aabb0_min_or_v0[3];
    float aabb0_max_or_v1[3];
    float aabb1_min_or_v2[3];
    float aabb1_max_or_v3[3];
};

template <typename Node>
void Transform2(void const* in_nodes, void const*, BvhNode<2>* nodes, Triangle* triangles, size_t internal_size, size_t)
{
    auto                                                           bvh_nodes = reinterpret_cast<Node const*>(in_nodes);
    std::queue<std::tuple<uint32_t, uint32_t, uint32_t, uint32_t>> q;
    uint32_t                                                       out_node_idx     = 0;
    uint32_t                                                       out_triangle_idx = 0;
    q.push({0u, kInvalidID, out_node_idx++, kInvalidID});
    while (!q.empty())
    {
        auto addr = q.front();
        q.pop();
        auto in_addr    = std::get<0>(addr);
        auto out_addr   = std::get<2>(addr);
        auto out_parent = std::get<3>(addr);
        // current node is internal
        if (in_addr < internal_size)
        {
            nodes[out_addr].children_count   = 2;
            nodes[out_addr].children_aabb[0] = {float3(bvh_nodes[in_addr].aabb0_min_or_v0[0],
                                                       bvh_nodes[in_addr].aabb0_min_or_v0[1],
                                                       bvh_nodes[in_addr].aabb0_min_or_v0[2]),
                                                float3(bvh_nodes[in_addr].aabb0_max_or_v1[0],
                                                       bvh_nodes[in_addr].aabb0_max_or_v1[1],
                                                       bvh_nodes[in_addr].aabb0_max_or_v1[2])};
            nodes[out_addr].children_aabb[1] = {float3(bvh_nodes[in_addr].aabb1_min_or_v2[0],
                                                       bvh_nodes[in_addr].aabb1_min_or_v2[1],
                                                       bvh_nodes[in_addr].aabb1_min_or_v2[2]),
                                                float3(bvh_nodes[in_addr].aabb1_max_or_v3[0],
                                                       bvh_nodes[in_addr].aabb1_max_or_v3[1],
                                                       bvh_nodes[in_addr].aabb1_max_or_v3[2])};
            nodes[out_addr].parent           = out_parent;
            nodes[out_addr].flag             = bvh_nodes[in_addr].update;
            if (bvh_nodes[in_addr].child0 < internal_size)
            {
                nodes[out_addr].children_addr[0]    = out_node_idx;
                nodes[out_addr].children_is_prim[0] = false;
                q.push({bvh_nodes[in_addr].child0, in_addr, out_node_idx++, out_addr});
            } else
            {
                nodes[out_addr].children_addr[0]    = out_triangle_idx;
                nodes[out_addr].children_is_prim[0] = true;
                q.push({bvh_nodes[in_addr].child0, in_addr, out_triangle_idx++, out_addr});
            }
            if (bvh_nodes[in_addr].child1 < internal_size)
            {
                nodes[out_addr].children_addr[1]    = out_node_idx;
                nodes[out_addr].children_is_prim[1] = false;
                q.push({bvh_nodes[in_addr].child1, in_addr, out_node_idx++, out_addr});
            } else
            {
                nodes[out_addr].children_addr[1]    = out_triangle_idx;
                nodes[out_addr].children_is_prim[1] = true;
                q.push({bvh_nodes[in_addr].child1, in_addr, out_triangle_idx++, out_addr});
            }
        } else
        {
            triangles[out_addr].v0      = float3(bvh_nodes[in_addr].aabb0_min_or_v0[0],
                                            bvh_nodes[in_addr].aabb0_min_or_v0[1],
                                            bvh_nodes[in_addr].aabb0_min_or_v0[2]);
            triangles[out_addr].v1      = float3(bvh_nodes[in_addr].aabb0_max_or_v1[0],
                                            bvh_nodes[in_addr].aabb0_max_or_v1[1],
                                            bvh_nodes[in_addr].aabb0_max_or_v1[2]);
            triangles[out_addr].v2      = float3(bvh_nodes[in_addr].aabb1_min_or_v2[0],
                                            bvh_nodes[in_addr].aabb1_min_or_v2[1],
                                            bvh_nodes[in_addr].aabb1_min_or_v2[2]);
            triangles[out_addr].prim_id = bvh_nodes[in_addr].child1;
        }
    }
}

}  // namespace bvh
