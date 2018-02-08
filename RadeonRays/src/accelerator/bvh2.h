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
#pragma once

#include <cassert>
#include <stack>
#include <utility>
#include <vector>
#include <thread>
#include <condition_variable>
#include <mmintrin.h>
#include <xmmintrin.h>
#include <smmintrin.h>

#include "../primitive/mesh.h"
#include "../primitive/instance.h"

#ifndef WIN32
#define _MM_ALIGN16
#endif

namespace RadeonRays
{

    class Bvh2
    {
        struct Node;
        struct SplitRequest;

    public:
        // Constructor
        Bvh2(float traversal_cost, int num_bins = 64, bool usesah = false)
            : m_num_bins(num_bins)
            , m_usesah(usesah)
            , m_traversal_cost(traversal_cost)
            , m_nodes(nullptr)
            , m_nodecount(0)
        {
        }

        // Build function
        template <typename Iter>
        void Build(Iter begin, Iter end);

        void Clear();

        inline std::size_t GetSizeInBytes() const;

    protected:
        using RefArray = std::vector<std::uint32_t>;
        using MetaDataArray = std::vector<std::pair<const Shape *, std::size_t> >;

        // Constant values
        enum Constants
        {
            // Invalid index marker
            kInvalidId = 0xffffffffu,
            // Max triangles per leaf
            kMaxLeafPrimitives = 1u,
            // Threshold number of primitives to disable SAH split
            kMinSAHPrimitives = 8u,
            // Maximum stack size for non-parallel builds
            kStackSize = 1024u
        };

        // Enum for node type
        enum NodeType
        {
            kLeaf,
            kInternal
        };

        // SAH flag
        bool m_usesah;
        // Node traversal cost
        float m_traversal_cost;
        // Number of spatial bins to use for SAH
        uint32_t m_num_bins;

        static void *Allocate(std::size_t size, std::size_t alignment)
        {
#ifdef WIN32
            return _aligned_malloc(size, alignment);
#else // WIN32
            (void)alignment;
            return malloc(size);
#endif // WIN32
        }

        static void Deallocate(void *ptr)
        {
#ifdef WIN32
            _aligned_free(ptr);
#else // WIN32
            free(ptr);
#endif // WIN32
        }

        void BuildImpl(
            __m128 scene_min,
            __m128 scene_max,
            __m128 centroid_scene_min,
            __m128 centroid_scene_max,
            const float3 *aabb_min,
            const float3 *aabb_max,
            const float3 *aabb_centroid,
            const MetaDataArray &metadata,
            std::size_t num_aabbs);

        template <std::uint32_t axis>
        float FindSahSplit(
            const SplitRequest &request,
            const float3 *aabb_min,
            const float3 *aabb_max,
            const float3 *aabb_centroid,
            const std::uint32_t *refs);

        NodeType HandleRequest(
            const SplitRequest &request,
            const float3 *aabb_min,
            const float3 *aabb_max,
            const float3 *aabb_centroid,
            const MetaDataArray &metadata,
            RefArray &refs,
            std::size_t num_aabbs,
            SplitRequest &request_left,
            SplitRequest &request_right);

        static inline void EncodeLeaf(
            Node &node,
            std::uint32_t num_refs);
        static inline void EncodeInternal(
            Node &node,
            __m128 aabb_min,
            __m128 aabb_max,
            std::uint32_t child0,
            std::uint32_t child1);
        static inline void SetPrimitive(
            Node &node,
            std::uint32_t index,
            std::pair<const Shape *, std::size_t> ref);

        static inline bool IsInternal(const Node &node);
        static inline std::uint32_t GetChildIndex(const Node &node, std::uint8_t idx);
        static inline void PropagateBounds(Bvh2 &bvh);

    private:
        Bvh2(const Bvh2 &);
        Bvh2 &operator = (const Bvh2 &);

        friend class QBvhTranslator;
        friend class IntersectorLDS;

        // Buffer of encoded nodes
        Node *m_nodes;
        // Number of encoded nodes
        std::size_t m_nodecount;
    };

    // Encoded node format
    struct Bvh2::Node
    {
        // Left AABB min or vertex 0 for a leaf node
        float aabb_left_min_or_v0[3] = { 0.0f, 0.0f, 0.0f };
        // Left child node address
        uint32_t addr_left = kInvalidId;
        // Left AABB max or vertex 1 for a leaf node
        float aabb_left_max_or_v1[3] = { 0.0f, 0.0f, 0.0f };
        // Mesh ID for a leaf node
        uint32_t mesh_id = kInvalidId;
        // Right AABB min or vertex 2 for a leaf node
        float aabb_right_min_or_v2[3] = { 0.0f, 0.0f, 0.0f };
        // Right child node address
        uint32_t addr_right = kInvalidId;
        // Left AABB max
        float aabb_right_max[3] = { 0.0f, 0.0f, 0.0f };
        // Primitive ID for a leaf node
        uint32_t prim_id = kInvalidId;
    };

    template <typename Iter>
    void Bvh2::Build(Iter begin, Iter end)
    {
        auto num_shapes = std::distance(begin, end);

        assert(num_shapes > 0);

        Clear();

        std::size_t num_items = 0;
        for (auto iter = begin; iter != end; ++iter)
        {
            auto shape = static_cast<const ShapeImpl *>(*iter);
            auto mesh = static_cast<const Mesh *>(shape->is_instance() ? static_cast<const Instance *>(shape)->GetBaseShape() : shape);

            // Quads are deprecated and no longer supported
            assert(mesh->puretriangle());

            num_items += mesh->num_faces();
        }

        auto deleter = [](void *ptr) { Deallocate(ptr); };
        using aligned_float3_ptr = std::unique_ptr<float3 [], decltype(deleter)>;

        auto aabb_min = aligned_float3_ptr(
            reinterpret_cast<float3*>(
                Allocate(sizeof(float3) * num_items, 16u)),
            deleter);

        auto aabb_max = aligned_float3_ptr(
            reinterpret_cast<float3*>(
                Allocate(sizeof(float3) * num_items, 16u)),
            deleter);

        auto aabb_centroid = aligned_float3_ptr(
            reinterpret_cast<float3*>(
                Allocate(sizeof(float3) * num_items, 16u)),
            deleter);

        MetaDataArray metadata(num_items);

        auto constexpr inf = std::numeric_limits<float>::infinity();

        auto scene_min = _mm_set_ps(inf, inf, inf, inf);
        auto scene_max = _mm_set_ps(-inf, -inf, -inf, -inf);
        auto centroid_scene_min = _mm_set_ps(inf, inf, inf, inf);
        auto centroid_scene_max = _mm_set_ps(-inf, -inf, -inf, -inf);

        std::size_t current_face = 0;
        for (auto iter = begin; iter != end; ++iter)
        {
            auto shape = static_cast<const ShapeImpl *>(*iter);
            auto isinstance = shape->is_instance();
            auto mesh = static_cast<const Mesh *>(isinstance ? static_cast<const Instance *>(shape)->GetBaseShape() : shape);

            matrix m, minv;
            shape->GetTransform(m, minv);

            for (std::size_t face_index = 0; face_index < mesh->num_faces(); ++face_index, ++current_face)
            {
                bbox bounds;
                mesh->GetFaceBounds(face_index, isinstance, bounds);

                // Instance is using its own transform for base shape geometry
                // so we need to get object space bounds and transform them manually
                if(isinstance)
                {
                    bounds = transform_bbox(bounds, m);
                }

                auto pmin = _mm_set_ps(bounds.pmin.w, bounds.pmin.z, bounds.pmin.y, bounds.pmin.x);
                auto pmax = _mm_set_ps(bounds.pmax.w, bounds.pmax.z, bounds.pmax.y, bounds.pmax.x);
                auto centroid = _mm_mul_ps(
                    _mm_add_ps(pmin, pmax),
                    _mm_set_ps(0.5f, 0.5f, 0.5f, 0.5f));

                scene_min = _mm_min_ps(scene_min, pmin);
                scene_max = _mm_max_ps(scene_max, pmax);

                centroid_scene_min = _mm_min_ps(centroid_scene_min, centroid);
                centroid_scene_max = _mm_max_ps(centroid_scene_max, centroid);

                _mm_store_ps(&aabb_min[current_face].x, pmin);
                _mm_store_ps(&aabb_max[current_face].x, pmax);
                _mm_store_ps(&aabb_centroid[current_face].x, centroid);

                metadata[current_face] = std::make_pair(shape, face_index);
            }
        }

        BuildImpl(
            scene_min,
            scene_max,
            centroid_scene_min,
            centroid_scene_max,
            aabb_min.get(),
            aabb_max.get(),
            aabb_centroid.get(),
            metadata,
            num_items);

        // We set 1 AABB for each node during BVH build process,
        // however our resulting structure keeps two AABBs for
        // left and right child nodes in the parent node. To
        // convert 1 AABB per node -> 2 AABBs for child nodes
        // we need to traverse the tree pulling child node AABBs
        // into their parent node. That's exactly what PropagateBounds
        // is doing.
        PropagateBounds(*this);
    }

    std::size_t Bvh2::GetSizeInBytes() const
    {
        return m_nodecount * sizeof(Node);
    }

    void Bvh2::EncodeLeaf(
        Node &node,
        std::uint32_t num_refs)
    {
        // This node only supports 1 triangle
        assert(num_refs == 1);
        node.addr_left = kInvalidId;
        node.addr_right = kInvalidId;
    }

    void Bvh2::EncodeInternal(
        Node &node,
        __m128 aabb_min,
        __m128 aabb_max,
        std::uint32_t child0,
        std::uint32_t child1)
    {
        _mm_store_ps(node.aabb_left_min_or_v0, aabb_min);
        _mm_store_ps(node.aabb_left_max_or_v1, aabb_max);
        node.addr_left = child0;
        node.addr_right = child1;
    }

    void Bvh2::SetPrimitive(
        Node &node,
        std::uint32_t index,
        std::pair<const Shape *, std::size_t> ref)
    {
        auto shape = ref.first;
        matrix worldmat, worldmatinv;
        shape->GetTransform(worldmat, worldmatinv);
        auto mesh = static_cast<const Mesh *>(static_cast<const ShapeImpl *>(shape)->is_instance() ? static_cast<const Instance *>(shape)->GetBaseShape() : shape);
        auto face = mesh->GetFaceData()[ref.second];
        auto v0 = transform_point(mesh->GetVertexData()[face.idx[0]], worldmat);
        auto v1 = transform_point(mesh->GetVertexData()[face.idx[1]], worldmat);
        auto v2 = transform_point(mesh->GetVertexData()[face.idx[2]], worldmat);
        node.aabb_left_min_or_v0[0] = v0.x;
        node.aabb_left_min_or_v0[1] = v0.y;
        node.aabb_left_min_or_v0[2] = v0.z;
        node.aabb_left_max_or_v1[0] = v1.x;
        node.aabb_left_max_or_v1[1] = v1.y;
        node.aabb_left_max_or_v1[2] = v1.z;
        node.aabb_right_min_or_v2[0] = v2.x;
        node.aabb_right_min_or_v2[1] = v2.y;
        node.aabb_right_min_or_v2[2] = v2.z;
        node.mesh_id = shape->GetId();
        node.prim_id = static_cast<std::uint32_t>(ref.second);
    }

    bool Bvh2::IsInternal(const Node &node)
    {
        return node.addr_left != kInvalidId;
    }

    std::uint32_t Bvh2::GetChildIndex(const Node &node, std::uint8_t idx)
    {
        return (IsInternal(node)
            ? (idx == 0 ?
                node.addr_left
                : node.addr_right)
            : kInvalidId);
    }

    void Bvh2::PropagateBounds(Bvh2 &bvh)
    {
        // Traversal stack
        std::stack<std::uint32_t> s;
        s.push(0);

        while (!s.empty())
        {
            auto idx = s.top();
            s.pop();

            // Fetch the node
            auto node = &bvh.m_nodes[idx];

            if (IsInternal(*node))
            {
                // If the node is internal we fetch child nodes
                auto idx0 = GetChildIndex(*node, 0);
                auto idx1 = GetChildIndex(*node, 1);

                auto child0 = &bvh.m_nodes[idx0];
                auto child1 = &bvh.m_nodes[idx1];

                // If the child is internal node itself we pull it
                // up the tree into its parent. If the child node is
                // a leaf, then we do not have AABB for it (we store 
                // vertices directly in the leaf), so we calculate 
                // AABB on the fly.
                if (IsInternal(*child0))
                {
                    node->aabb_left_min_or_v0[0] = child0->aabb_left_min_or_v0[0];
                    node->aabb_left_min_or_v0[1] = child0->aabb_left_min_or_v0[1];
                    node->aabb_left_min_or_v0[2] = child0->aabb_left_min_or_v0[2];
                    node->aabb_left_max_or_v1[0] = child0->aabb_left_max_or_v1[0];
                    node->aabb_left_max_or_v1[1] = child0->aabb_left_max_or_v1[1];
                    node->aabb_left_max_or_v1[2] = child0->aabb_left_max_or_v1[2];
                    s.push(idx0);
                }
                else
                {
                    node->aabb_left_min_or_v0[0] = std::min(
                        child0->aabb_left_min_or_v0[0],
                        std::min(child0->aabb_left_max_or_v1[0],
                            child0->aabb_right_min_or_v2[0]));

                    node->aabb_left_min_or_v0[1] = std::min(
                        child0->aabb_left_min_or_v0[1],
                        std::min(child0->aabb_left_max_or_v1[1],
                            child0->aabb_right_min_or_v2[1]));

                    node->aabb_left_min_or_v0[2] = std::min(
                        child0->aabb_left_min_or_v0[2],
                        std::min(child0->aabb_left_max_or_v1[2],
                            child0->aabb_right_min_or_v2[2]));

                    node->aabb_left_max_or_v1[0] = std::max(
                        child0->aabb_left_min_or_v0[0],
                        std::max(child0->aabb_left_max_or_v1[0],
                            child0->aabb_right_min_or_v2[0]));

                    node->aabb_left_max_or_v1[1] = std::max(
                        child0->aabb_left_min_or_v0[1],
                        std::max(child0->aabb_left_max_or_v1[1],
                            child0->aabb_right_min_or_v2[1]));

                    node->aabb_left_max_or_v1[2] = std::max(
                        child0->aabb_left_min_or_v0[2],
                        std::max(child0->aabb_left_max_or_v1[2],
                            child0->aabb_right_min_or_v2[2]));
                }

                // If the child is internal node itself we pull it
                // up the tree into its parent. If the child node is
                // a leaf, then we do not have AABB for it (we store 
                // vertices directly in the leaf), so we calculate 
                // AABB on the fly.
                if (IsInternal(*child1))
                {
                    node->aabb_right_min_or_v2[0] = child1->aabb_left_min_or_v0[0];
                    node->aabb_right_min_or_v2[1] = child1->aabb_left_min_or_v0[1];
                    node->aabb_right_min_or_v2[2] = child1->aabb_left_min_or_v0[2];
                    node->aabb_right_max[0] = child1->aabb_left_max_or_v1[0];
                    node->aabb_right_max[1] = child1->aabb_left_max_or_v1[1];
                    node->aabb_right_max[2] = child1->aabb_left_max_or_v1[2];
                    s.push(idx1);
                }
                else
                {
                    node->aabb_right_min_or_v2[0] = std::min(
                        child1->aabb_left_min_or_v0[0],
                        std::min(child1->aabb_left_max_or_v1[0],
                            child1->aabb_right_min_or_v2[0]));

                    node->aabb_right_min_or_v2[1] = std::min(
                        child1->aabb_left_min_or_v0[1],
                        std::min(child1->aabb_left_max_or_v1[1],
                            child1->aabb_right_min_or_v2[1]));

                    node->aabb_right_min_or_v2[2] = std::min(
                        child1->aabb_left_min_or_v0[2],
                        std::min(child1->aabb_left_max_or_v1[2],
                            child1->aabb_right_min_or_v2[2]));

                    node->aabb_right_max[0] = std::max(
                        child1->aabb_left_min_or_v0[0],
                        std::max(child1->aabb_left_max_or_v1[0],
                            child1->aabb_right_min_or_v2[0]));

                    node->aabb_right_max[1] = std::max(
                        child1->aabb_left_min_or_v0[1],
                        std::max(child1->aabb_left_max_or_v1[1],
                            child1->aabb_right_min_or_v2[1]));

                    node->aabb_right_max[2] = std::max(
                        child1->aabb_left_min_or_v0[2],
                        std::max(child1->aabb_left_max_or_v1[2],
                            child1->aabb_right_min_or_v2[2]));
                }
            }
        }
    }
}
