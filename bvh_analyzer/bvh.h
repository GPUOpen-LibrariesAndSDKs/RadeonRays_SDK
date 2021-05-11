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
#include <set>
#include <stack>
#include <tuple>
#include <vector>

#include "bvh_node.h"
#include "common.h"
#include "stb_image_write.h"
#include "triangle.h"

namespace bvh
{
template <uint32_t FACTOR>
struct BvhIntersect;

template <uint32_t FACTOR>
class Bvh
{
public:
    Bvh(size_t internal_size, size_t primitive_size)
        : nodes(std::vector<BvhNode<FACTOR>>(internal_size)), primitives(std::vector<Triangle>(primitive_size))
    {
    }
    template <typename TransformF>
    void TransformBvh(TransformF func, void const* in_nodes, void const* in_primitives = nullptr)
    {
        func(in_nodes, in_primitives, nodes.data(), primitives.data(), nodes.size(), primitives.size());
    }

    QualityStats                        CheckQuality(std::vector<Ray> const& rays,
                                                     uint32_t                width,
                                                     uint32_t                height,
                                                     QueryType               type,
                                                     std::vector<Hit>&       hits);
    std::vector<BvhNode<FACTOR>> const& Nodes() const { return nodes; }
    std::vector<Triangle> const&        Primitives() const { return primitives; }
    uint32_t                            Root() const { return 0u; }

private:
    bool  IsValid() const;
    float CalculateSAH(float cprim = 1.f, float cnode = 1.f) const;

    std::vector<BvhNode<FACTOR>> nodes;
    std::vector<Triangle>        primitives;
};

template <uint32_t FACTOR>
inline QualityStats Bvh<FACTOR>::CheckQuality(std::vector<Ray> const& rays,
                                              uint32_t                width,
                                              uint32_t                height,
                                              QueryType               type,
                                              std::vector<Hit>&       hits)
{
    QualityStats stats;
    stats.is_valid = IsValid();
    if (!stats.is_valid)
    {
        return stats;
    }
    stats.sah_estimation = CalculateSAH();
    hits.resize(rays.size());
    std::vector<TraversalStats> traversal_stats(rays.size());
    TraversalStats              overall_stats;
#pragma omp parallel for
    for (int i = 0; i < (int)rays.size(); i++)
    {
        const auto&          ray = rays[i];
        BvhIntersect<FACTOR> intersector(*this, type);
        hits[i]            = intersector(ray);
        traversal_stats[i] = intersector.stats();
#pragma omp critical
        {
            overall_stats.Add(traversal_stats[i]);
        }
    }

    std::vector<uint32_t> data_image(width * height);
    std::vector<uint32_t> data_tests(width * height);
    for (uint32_t y = 0; y < height; ++y)
    {
        for (uint32_t x = 0; x < width; ++x)
        {
            uint32_t wi = width * (height - 1 - y) + x;
            uint32_t i  = width * y + x;

            if (hits[i].inst_id != kInvalidID)
            {
                data_image[wi] =
                    0xff000000 | (uint32_t(hits[i].uv[0] * 255) << 8) | (uint32_t(hits[i].uv[1] * 255) << 16);
            } else
            {
                data_image[wi] = 0xff101010;
            }
            data_tests[wi] = 0xff000000 | (uint32_t)traversal_stats[i].num_aabb_tests;
        }
    }
    stbi_write_jpg("isect_result.jpg", width, height, 4, data_image.data(), 120);
    stbi_write_jpg("isect_tests.jpg", width, height, 4, data_tests.data(), 120);

    stats.avg_primary_node_tests     = overall_stats.num_internal_node_tests / rays.size();
    stats.avg_primary_aabb_tests     = overall_stats.num_aabb_tests / rays.size();
    stats.avg_primary_triangle_tests = overall_stats.num_triangle_tests / rays.size();

    return stats;
}
template <uint32_t FACTOR>
inline bool Bvh<FACTOR>::IsValid() const
{
    std::queue<std::tuple<uint32_t, uint32_t, bool>> q;
    std::set<std::pair<uint32_t, bool>>              visited;
    q.push({Root(), kInvalidID, false});
    visited.emplace(Root(), false);
    while (!q.empty())
    {
        auto current = q.front();
        q.pop();
        if (!std::get<2>(current))
        {
            auto            addr   = std::get<0>(current);
            auto            parent = std::get<1>(current);
            BvhNode<FACTOR> node   = nodes[addr];
            // check topology consistency
            if (parent != node.parent)
            {
                return false;
            }
            Aabb current_aabb;
            if (parent != kInvalidID)
            {
                BvhNode<FACTOR> parent_node = nodes[parent];
                for (uint32_t i = 0; i < parent_node.children_count; i++)
                {
                    if (parent_node.children_addr[i] == addr)
                    {
                        current_aabb = parent_node.children_aabb[i];
                        break;
                    }
                }
            }
            for (uint32_t i = 0; i < node.children_count; i++)
            {
                q.push(std::make_tuple(node.children_addr[i], addr, node.children_is_prim[i]));
                auto inserted = visited.emplace(node.children_addr[i], node.children_is_prim[i]);
                if (!inserted.second)
                {
                    // already visited
                    return false;
                }
                if (parent != kInvalidID)
                {
                    // check volumes
                    if (!current_aabb.Includes(node.children_aabb[i]))
                    {
                        return false;
                    }
                }
            }
        }
    }
    return visited.size() == nodes.size() + primitives.size();
}

template <uint32_t FACTOR>
inline float Bvh<FACTOR>::CalculateSAH(float cprim, float cnode) const
{
    std::stack<std::pair<uint32_t, bool>> traversal_stack;
    traversal_stack.push({Root(), false});

    BvhNode<FACTOR> root      = nodes[Root()];
    auto            root_area = root.GetAabb().Area();

    float total_sah = 0.f;

    while (!traversal_stack.empty())
    {
        auto node_index = traversal_stack.top();
        traversal_stack.pop();
        if (!node_index.second)
        {
            BvhNode<FACTOR> node = nodes[node_index.first];

            float relative_area = node.GetAabb().Area() / root_area;
            float sah           = relative_area * cnode;
            total_sah += sah;
            for (uint32_t i = 0; i < node.children_count; i++)
            {
                traversal_stack.push({node.children_addr[i], node.children_is_prim[i]});
            }
        } else
        {
            Triangle tri = primitives[node_index.first];

            float relative_area = tri.GetAabb().Area() / root_area;
            float sah           = relative_area * cprim;
            total_sah += sah;
        }
    }

    return total_sah;
}
// this is generic solution that can be specilized for performance purposes
template <uint32_t FACTOR>
struct BvhIntersect
{
    BvhIntersect(const Bvh<FACTOR>& bvh, QueryType type) : bvh_(bvh), type_(type) {}

    Hit operator()(const Ray& r) const
    {
        Hit isect;

        float3 origin = {r.origin[0], r.origin[1], r.origin[2]};
        float3 dir    = {r.direction[0], r.direction[1], r.direction[2]};
        // Reset stats.
        stats_.Reset();

        // Get BVH nodes pointer.
        auto const& nodes     = bvh_.Nodes();
        auto const& triangles = bvh_.Primitives();

        // Precompute inverse direction and neg.
        float3 invd      = rcp(dir);
        float3 oxinvd    = -origin * invd;
        using StackEntry = std::pair<uint32_t, bool>;
        // Push root node on the stack, bool indicates is triangle
        std::stack<StackEntry> traversal_stack;
        traversal_stack.push({bvh_.Root(), false});

        float t = r.max_t;

        // Traverse.
        while (!traversal_stack.empty())
        {
            // Pop next node index from the stack.
            auto node_index = traversal_stack.top();
            traversal_stack.pop();

            // Check if it is internal or leaf.
            if (!node_index.second)
            {
                const BvhNode<FACTOR>& node = nodes[node_index.first];
                // Internal node adds 1 node test == children_count bbox tests
                stats_.num_internal_node_tests += 1;
                stats_.num_aabb_tests += node.children_count;

                struct SortedEntry
                {
                    float      dist;
                    StackEntry entry;
                    bool       operator<(SortedEntry const& rhs) const { return dist < rhs.dist; }
                    bool       operator>(SortedEntry const& rhs) const { return dist > rhs.dist; }
                };
                // dirty solution for generic purposes
                std::priority_queue<SortedEntry, std::vector<SortedEntry>, std::greater<SortedEntry>> tested;

                // test each bbox
                for (uint32_t i = 0; i < node.children_count; i++)
                {
                    const Aabb& aabb = node.children_aabb[i];
                    float2      dist = aabb.Intersect(invd, oxinvd, r.min_t, t);
                    if (dist.x <= dist.y)
                    {
                        tested.push({dist.x, StackEntry{node.children_addr[i], node.children_is_prim[i]}});
                    }
                }
                // push tested children to stack in order of their distances
                while (!tested.empty())
                {
                    traversal_stack.push(tested.top().entry);
                    tested.pop();
                }
            } else
            {
                const Triangle& triangle = triangles[node_index.first];
                // Leaf node adds one leaf node test == 1 triangle test.
                stats_.num_leaf_node_tests += 1;
                stats_.num_triangle_tests += 1;

                float2 uv;

                // Test triangle intersection.
                if (triangle.Intersect(r, uv, t))
                {
                    isect.inst_id = 0u;
                    isect.prim_id = triangle.prim_id;
                    isect.uv[0]   = uv.x;
                    isect.uv[1]   = uv.y;
                    if (type_ == QueryType::kAnyHit)
                    {
                        break;
                    }
                }
            }
        }

        return isect;
    }

    // Get last traversal stats.
    const TraversalStats& stats() const { return stats_; }

private:
    const Bvh<FACTOR>&     bvh_;
    QueryType              type_;
    mutable TraversalStats stats_;
};

}  // namespace bvh