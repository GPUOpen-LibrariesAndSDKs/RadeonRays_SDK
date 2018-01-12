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
#include "bvh.h"

#include <algorithm>
#include <thread>
#include <stack>
#include <numeric>
#include <cassert>
#include <vector>
#include <future>

// TODO: remove me (gboisse)
#include "../util/cputimer.h"

namespace RadeonRays
{
    // TODO: clean up (gboisse)
#ifdef __GNUC__
#define clz(x) __builtin_clz(x)
#define ctz(x) __builtin_ctz(x)
#else
    inline std::uint32_t popcnt(std::uint32_t x) {
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return x & 0x0000003f;
    }
    inline std::uint32_t clz(std::uint32_t x) {
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return 32 - popcnt(x);
    }
    inline std::uint32_t ctz(std::uint32_t x) {
        return popcnt((std::uint32_t)(x & -(int)x) - 1);
    }
#endif
    inline
    std::uint32_t aabb_max_extent_axis(__m128 pmin, __m128 pmax) {
        auto xyz = _mm_sub_ps(pmax, pmin);
        auto yzx = _mm_shuffle_ps(xyz, xyz, _MM_SHUFFLE(3, 0, 2, 1));
        auto m0 = _mm_max_ps(xyz, yzx);
        auto m1 = _mm_shuffle_ps(m0, m0, _MM_SHUFFLE(3, 0, 2, 1));
        auto m2 = _mm_max_ps(m0, m1);
        auto cmp = _mm_cmpeq_ps(xyz, m2);
        return ctz(_mm_movemask_ps(cmp));
    }
    inline
    float mm_select(__m128 v, std::uint32_t index) {
        _MM_ALIGN16 float temp[4];
        _mm_store_ps(temp, v);
        return temp[index];
    }

    static int constexpr kMaxPrimitivesPerLeaf = 1;

    static bool is_nan(float v)
    {
        return v != v;
    }

    void Bvh::Build(bbox const* bounds, int numbounds)
    {
        // TODO: that's not very useful... should refactore to do more ops in that loop to be worth it (gboisse)
#if 0
        {
            CPU_TIMED_SECTION(AggregateBounds);

            auto constexpr inf = std::numeric_limits<float>::infinity();

            auto scene_min = _mm_set_ps(inf, inf, inf, inf);
            auto scene_max = _mm_set_ps(-inf, -inf, -inf, -inf);

            for (int i = 0; i < numbounds; ++i)
            {
                auto pmin = _mm_load_ps(&bounds[i].pmin.x);
                auto pmax = _mm_load_ps(&bounds[i].pmax.x);

                scene_min = _mm_min_ps(scene_min, pmin);
                scene_max = _mm_max_ps(scene_max, pmax);
            }

            _mm_store_ps(&m_bounds.pmin.x, scene_min);
            _mm_store_ps(&m_bounds.pmax.x, scene_max);
        }
#else
        {
            CPU_TIMED_SECTION(AggregateBounds);

            for (int i = 0; i < numbounds; ++i)
            {
                // Calc bbox
                m_bounds.grow(bounds[i]);
            }
        }
#endif

        BuildImpl(bounds, numbounds);
    }

    bbox const& Bvh::Bounds() const
    {
        return m_bounds;
    }

    void  Bvh::InitNodeAllocator(size_t maxnum)
    {
        m_nodecnt = 0;
        m_nodes.resize(maxnum);
    }

    Bvh::Node* Bvh::AllocateNode()
    {
        return &m_nodes[m_nodecnt++];
    }

    void Bvh::BuildNode(SplitRequest const& req, bbox const* bounds, float3 const* centroids, int* primindices)
    {
        m_height = std::max(m_height, req.level);

        Node* node = AllocateNode();
        node->bounds = req.bounds;
        node->index = req.index;

        // Create leaf node if we have enough prims
        if (req.numprims < 2)
        {
#ifdef USE_TBB
            primitive_mutex_.lock();
#endif
            node->type = kLeaf;
            node->startidx = static_cast<int>(m_packed_indices.size());
            node->numprims = req.numprims;

            for (auto i = 0U; i < req.numprims; ++i)
            {
                m_packed_indices.push_back(primindices[req.startidx + i]);
            }
#ifdef USE_TBB
            primitive_mutex_.unlock();
#endif
        }
        else
        {
            // Choose the maximum extent
            int axis = req.centroid_bounds.maxdim();
            float border = req.centroid_bounds.center()[axis];

            if (m_usesah)
            {
                SahSplit ss = FindSahSplit(req, bounds, centroids, primindices);

                if (!is_nan(ss.split))
                {
                    axis = ss.dim;
                    border = ss.split;

                    if (req.numprims < ss.sah && req.numprims < kMaxPrimitivesPerLeaf)
                    {
                        node->type = kLeaf;
                        node->startidx = static_cast<int>(m_packed_indices.size());
                        node->numprims = req.numprims;

                        for (auto i = 0U; i < req.numprims; ++i)
                        {
                            m_packed_indices.push_back(primindices[req.startidx + i]);
                        }

                        if (req.ptr) *req.ptr = node;
                        return;
                    }
                }
            }

            node->type = kInternal;

            // Start partitioning and updating extents for children at the same time
            bbox leftbounds, rightbounds, leftcentroid_bounds, rightcentroid_bounds;
            int splitidx = req.startidx;

            bool near2far = (req.numprims + req.startidx) & 0x1;

            if (req.centroid_bounds.extents()[axis] > 0.f)
            {
                auto first = req.startidx;
                auto last = req.startidx + req.numprims;

                if (near2far)
                {
                    while (1)
                    {
                        while ((first != last) &&
                            centroids[primindices[first]][axis] < border)
                        {
                            leftbounds.grow(bounds[primindices[first]]);
                            leftcentroid_bounds.grow(centroids[primindices[first]]);
                            ++first;
                        }

                        if (first == last--) break;

                        rightbounds.grow(bounds[primindices[first]]);
                        rightcentroid_bounds.grow(centroids[primindices[first]]);

                        while ((first != last) &&
                            centroids[primindices[last]][axis] >= border)
                        {
                            rightbounds.grow(bounds[primindices[last]]);
                            rightcentroid_bounds.grow(centroids[primindices[last]]);
                            --last;
                        }

                        if (first == last) break;

                        leftbounds.grow(bounds[primindices[last]]);
                        leftcentroid_bounds.grow(centroids[primindices[last]]);

                        std::swap(primindices[first++], primindices[last]);
                    }
                }
                else
                {
                    while (1)
                    {
                        while ((first != last) &&
                            centroids[primindices[first]][axis] >= border)
                        {
                            leftbounds.grow(bounds[primindices[first]]);
                            leftcentroid_bounds.grow(centroids[primindices[first]]);
                            ++first;
                        }

                        if (first == last--) break;

                        rightbounds.grow(bounds[primindices[first]]);
                        rightcentroid_bounds.grow(centroids[primindices[first]]);

                        while ((first != last) &&
                            centroids[primindices[last]][axis] < border)
                        {
                            rightbounds.grow(bounds[primindices[last]]);
                            rightcentroid_bounds.grow(centroids[primindices[last]]);
                            --last;
                        }

                        if (first == last) break;

                        leftbounds.grow(bounds[primindices[last]]);
                        leftcentroid_bounds.grow(centroids[primindices[last]]);

                        std::swap(primindices[first++], primindices[last]);
                    }
                }

                splitidx = first;
            }

            if (splitidx == req.startidx || splitidx == req.startidx + req.numprims)
            {
                splitidx = req.startidx + (req.numprims >> 1);

                for (int i = req.startidx; i < splitidx; ++i)
                {
                    leftbounds.grow(bounds[primindices[i]]);
                    leftcentroid_bounds.grow(centroids[primindices[i]]);
                }

                for (int i = splitidx; i < req.startidx + req.numprims; ++i)
                {
                    rightbounds.grow(bounds[primindices[i]]);
                    rightcentroid_bounds.grow(centroids[primindices[i]]);
                }
            }

            // Left request
            SplitRequest leftrequest = { req.startidx, splitidx - req.startidx, &node->lc, leftbounds, leftcentroid_bounds, req.level + 1, (req.index << 1) };
            // Right request
            SplitRequest rightrequest = { splitidx, req.numprims - (splitidx - req.startidx), &node->rc, rightbounds, rightcentroid_bounds, req.level + 1, (req.index << 1) + 1 };

            {
                // Put those to stack
                BuildNode(leftrequest, bounds, centroids, primindices);
            }

            {
                BuildNode(rightrequest, bounds, centroids, primindices);
            }
        }

        // Set parent ptr if any
        if (req.ptr) *req.ptr = node;
    }

    Bvh::SahSplit Bvh::FindSahSplit(SplitRequest const& req, bbox const* bounds, float3 const* centroids, int* primindices) const
    {
        // SAH implementation
        // calc centroids histogram
        // int const kNumBins = 128;
        // moving split bin index
        int splitidx = -1;
        // Set SAH to maximum float value as a start
        float sah = std::numeric_limits<float>::max();
        SahSplit split;
        split.dim = 0;
        split.split = std::numeric_limits<float>::quiet_NaN();

        // if we cannot apply histogram algorithm
        // put NAN sentinel as split border
        // PerformObjectSplit simply splits in half
        // in this case
        float3 centroid_extents = req.centroid_bounds.extents();
        if (centroid_extents.sqnorm() == 0.f)
        {
            return split;
        }

        // Bin has bbox and occurence count
        struct Bin
        {
            bbox bounds;
            int count;
        };

        // Keep bins for each dimension
        std::vector<Bin> bins[3];
        bins[0].resize(m_num_bins);
        bins[1].resize(m_num_bins);
        bins[2].resize(m_num_bins);

        // Precompute inverse parent area
        float invarea = 1.f / req.bounds.surface_area();
        // Precompute min point
        float3 rootmin = req.centroid_bounds.pmin;

        // Evaluate all dimensions
        for (int axis = 0; axis < 3; ++axis)
        {
            float rootminc = rootmin[axis];
            // Range for histogram
            float centroid_rng = centroid_extents[axis];
            float invcentroid_rng = 1.f / centroid_rng;

            // If the box is degenerate in that dimension skip it
            if (centroid_rng == 0.f) continue;

            // Initialize bins
            for (unsigned i = 0; i < m_num_bins; ++i)
            {
                bins[axis][i].count = 0;
                bins[axis][i].bounds = bbox();
            }

            // Calc primitive refs histogram
            for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
            {
                int idx = primindices[i];
                int binidx = (int)std::min<float>(m_num_bins * ((centroids[idx][axis] - rootminc) * invcentroid_rng), m_num_bins - 1);

                ++bins[axis][binidx].count;
                bins[axis][binidx].bounds.grow(bounds[idx]);
            }

            std::vector<bbox> rightbounds(m_num_bins - 1);

            // Start with 1-bin right box
            bbox rightbox = bbox();
            for (int i = m_num_bins - 1; i > 0; --i)
            {
                rightbox.grow(bins[axis][i].bounds);
                rightbounds[i - 1] = rightbox;
            }

            bbox leftbox = bbox();
            int  leftcount = 0;
            int  rightcount = req.numprims;

            // Start best SAH search
            // i is current split candidate (split between i and i + 1)
            float sahtmp = 0.f;
            for (int i = 0; i < m_num_bins - 1; ++i)
            {
                leftbox.grow(bins[axis][i].bounds);
                leftcount += bins[axis][i].count;
                rightcount -= bins[axis][i].count;

                // Compute SAH
                sahtmp = m_traversal_cost + (leftcount * leftbox.surface_area() + rightcount * rightbounds[i].surface_area()) * invarea;

                // Check if it is better than what we found so far
                if (sahtmp < sah)
                {
                    split.dim = axis;
                    splitidx = i;
                    split.sah = sah = sahtmp;
                }
            }
        }

        // Choose split plane
        if (splitidx != -1)
        {
            split.split = rootmin[split.dim] + (splitidx + 1) * (centroid_extents[split.dim] / m_num_bins);
        }

        return split;
    }

    void Bvh::BuildImpl(bbox const* bounds, int numbounds)
    {
        // Structure describing split request
        InitNodeAllocator(2 * numbounds - 1);

        // Cache some stuff to have faster partitioning
        std::vector<float3> centroids(numbounds);
        m_indices.resize(numbounds);
        std::iota(m_indices.begin(), m_indices.end(), 0);

        // Calc bbox
        bbox centroid_bounds;
        for (size_t i = 0; i < numbounds; ++i)
        {
            float3 c = bounds[i].center();
            centroid_bounds.grow(c);
            centroids[i] = c;
        }

        SplitRequest init = { 0, numbounds, nullptr, m_bounds, centroid_bounds, 0, 1 };

#if 1
        {
            // TODO: move this out of here somehow (gboisse)
            auto deleter = [](void *ptr) { _aligned_free(ptr); };
            using aligned_float3_ptr = std::unique_ptr<float3[], decltype(deleter)>;
            auto aabb_min = aligned_float3_ptr(reinterpret_cast<float3 *>(_aligned_malloc(sizeof(float3) * numbounds, 16)), deleter);
            auto aabb_max = aligned_float3_ptr(reinterpret_cast<float3 *>(_aligned_malloc(sizeof(float3) * numbounds, 16)), deleter);
            auto aabb_centroid = aligned_float3_ptr(reinterpret_cast<float3 *>(_aligned_malloc(sizeof(float3) * numbounds, 16)), deleter);

            for (std::int32_t i = 0; i < numbounds; ++i) {
                aabb_min[i] = bounds[i].pmin;
                aabb_max[i] = bounds[i].pmax;
                aabb_centroid[i] = centroid_bounds[i];
            }

            struct SplitRequest2
            {
                inline SplitRequest2() {}

                // TODO: why do I need this?!? (gboisse)
                inline SplitRequest2(__m128 aabb_min, __m128 aabb_max, __m128 centroid_aabb_min, __m128 centroid_aabb_max, std::size_t start_index, std::size_t num_refs, std::uint32_t level, std::uint32_t index, Node **ptr)
                    : aabb_min(aabb_min)
                    , aabb_max(aabb_max)
                    , centroid_aabb_min(centroid_aabb_min)
                    , centroid_aabb_max(centroid_aabb_max)
                    , start_index(start_index)
                    , num_refs(num_refs)
                    , level(level)
                    , index(index)
                    , ptr(ptr)
                {
#if 1
                    _MM_ALIGN16 float3 pmin, pmax;
                    _mm_store_ps(&pmin.x, aabb_min);
                    _mm_store_ps(&pmax.x, aabb_max);
                    const bool res = true;
#endif
                }

                __m128 aabb_min;
                __m128 aabb_max;
                __m128 centroid_aabb_min;
                __m128 centroid_aabb_max;
                std::size_t start_index;
                std::size_t num_refs;
                std::uint32_t level;
                std::uint32_t index;
                Node **ptr;
            };

            auto constexpr inf = std::numeric_limits<float>::infinity();
            auto m128_plus_inf = _mm_set_ps(inf, inf, inf, inf);
            auto m128_minus_inf = _mm_set_ps(-inf, -inf, -inf, -inf);

            std::stack<SplitRequest2> requests;

            std::mutex mutex;
            std::condition_variable cv;
            std::atomic_bool shutdown = false;
            std::atomic_uint32_t num_refs_processed = 0;

            requests.push(SplitRequest2(
                _mm_set_ps(m_bounds.pmin.w, m_bounds.pmin.z, m_bounds.pmin.y, m_bounds.pmin.x),
                _mm_set_ps(m_bounds.pmax.w, m_bounds.pmax.z, m_bounds.pmax.y, m_bounds.pmax.x),
                _mm_set_ps(centroid_bounds.pmin.w, centroid_bounds.pmin.z, centroid_bounds.pmin.y, centroid_bounds.pmin.x),
                _mm_set_ps(centroid_bounds.pmax.w, centroid_bounds.pmax.z, centroid_bounds.pmax.y, centroid_bounds.pmax.x),
                0,
                numbounds,
                0u,
                1u,
                nullptr
            ));

            auto HandleRequest = [&](
                const SplitRequest2 &request,
                const float3 *aabb_min,
                const float3 *aabb_max,
                const float3 *aabb_centroid,
                void *, // TODO: what is this for?!? (gboisse)
                std::vector<int> &refs, // TODO: rename back/remove? (gboisse)
                std::size_t num_aabbs,
                SplitRequest2 &request_left,
                SplitRequest2 &request_right
                ) -> NodeType
            {
                // TODO: create node (gboisse)
                Node *node;
                static std::mutex removeMe; // TODO: remove me (gboisse)
                {
                    const std::unique_lock<std::mutex> lock(mutex);
                    node = AllocateNode();
                }
                node->index = request.index;
                _MM_ALIGN16 float3 pmin, pmax; // TODO: node buffer should be aligned (gboisse)
                _mm_store_ps(&pmin.x, request.aabb_min);
                _mm_store_ps(&pmax.x, request.aabb_max);
                node->bounds.pmin = pmin;
                node->bounds.pmax = pmax;
                if (request.ptr) *request.ptr = node;

                // Create leaf node if we have enough prims
                if (request.num_refs <= kMaxPrimitivesPerLeaf)
                {
                    // TODO: create leaf node (gboisse)
                    const std::unique_lock<std::mutex> lock(mutex);
                    node->type = kLeaf;
                    node->startidx = static_cast<int>(m_packed_indices.size());
                    node->numprims = request.num_refs;
                    for (auto i = 0; i < request.num_refs; ++i)
                    {
                        m_packed_indices.push_back(refs[request.start_index + i]);
                    }
                    return kLeaf;
                }

                //
                node->type = kInternal;

                auto split_axis = aabb_max_extent_axis(
                    request.centroid_aabb_min,
                    request.centroid_aabb_max);

                auto split_axis_extent = mm_select(
                    _mm_sub_ps(request.centroid_aabb_max,
                               request.centroid_aabb_min),
                    split_axis);

                auto split_value = mm_select(
                    _mm_mul_ps(
                        _mm_set_ps(0.5f, 0.5f, 0.5f, 0.5f),
                        _mm_add_ps(request.centroid_aabb_max,
                                   request.centroid_aabb_min)),
                    split_axis);

                auto split_idx = request.start_index;

                auto constexpr inf = std::numeric_limits<float>::infinity();
                auto m128_plus_inf = _mm_set_ps(inf, inf, inf, inf);
                auto m128_minus_inf = _mm_set_ps(-inf, -inf, -inf, -inf);

                auto lmin = m128_plus_inf;
                auto lmax = m128_minus_inf;
                auto rmin = m128_plus_inf;
                auto rmax = m128_minus_inf;

                auto lcmin = m128_plus_inf;
                auto lcmax = m128_minus_inf;
                auto rcmin = m128_plus_inf;
                auto rcmax = m128_minus_inf;

                if (split_axis_extent > 0.0f)
                {
                    // TODO: SAH split evaluation in here (gboisse)

                    auto first = request.start_index;
                    auto last = request.start_index + request.num_refs;

                    while (1)
                    {
                        while ((first != last) &&
                            aabb_centroid[refs[first]][split_axis] < split_value)
                        {
                            auto idx = refs[first];
                            lmin = _mm_min_ps(lmin, _mm_load_ps(&aabb_min[idx].x));
                            lmax = _mm_max_ps(lmax, _mm_load_ps(&aabb_max[idx].x));

                            auto c = _mm_load_ps(&aabb_centroid[idx].x);
                            lcmin = _mm_min_ps(lcmin, c);
                            lcmax = _mm_max_ps(lcmax, c);

                            ++first;
                        }

                        if (first == last--) break;

                        auto idx = refs[first];
                        rmin = _mm_min_ps(rmin, _mm_load_ps(&aabb_min[idx].x));
                        rmax = _mm_max_ps(rmax, _mm_load_ps(&aabb_max[idx].x));

                        auto c = _mm_load_ps(&aabb_centroid[idx].x);
                        rcmin = _mm_min_ps(rcmin, c);
                        rcmax = _mm_max_ps(rcmax, c);

                        while ((first != last) &&
                            aabb_centroid[refs[last]][split_axis] >= split_value)
                        {
                            auto idx = refs[last];
                            rmin = _mm_min_ps(rmin, _mm_load_ps(&aabb_min[idx].x));
                            rmax = _mm_max_ps(rmax, _mm_load_ps(&aabb_max[idx].x));

                            auto c = _mm_load_ps(&aabb_centroid[idx].x);
                            rcmin = _mm_min_ps(rcmin, c);
                            rcmax = _mm_max_ps(rcmax, c);

                            --last;
                        }

                        if (first == last) break;

                        idx = refs[last];
                        lmin = _mm_min_ps(lmin, _mm_load_ps(&aabb_min[idx].x));
                        lmax = _mm_max_ps(lmax, _mm_load_ps(&aabb_max[idx].x));

                        c = _mm_load_ps(&aabb_centroid[idx].x);
                        lcmin = _mm_min_ps(lcmin, c);
                        lcmax = _mm_max_ps(lcmax, c);

                        std::swap(refs[first++], refs[last]);
                    }

                    split_idx = first;
                }

                // Edge case?
                if (split_idx == request.start_index ||
                    split_idx == request.start_index + request.num_refs)
                {
                    split_idx = request.start_index + (request.num_refs >> 1);

                    lmin = m128_plus_inf;
                    lmax = m128_minus_inf;
                    rmin = m128_plus_inf;
                    rmax = m128_minus_inf;

                    lcmin = m128_plus_inf;
                    lcmax = m128_minus_inf;
                    rcmin = m128_plus_inf;
                    rcmax = m128_minus_inf;

                    for (auto i = request.start_index; i < split_idx; ++i)
                    {
                        auto idx = refs[i];
                        lmin = _mm_min_ps(lmin, _mm_load_ps(&aabb_min[idx].x));
                        lmax = _mm_max_ps(lmax, _mm_load_ps(&aabb_max[idx].x));

                        auto c = _mm_load_ps(&aabb_centroid[idx].x);
                        lcmin = _mm_min_ps(lcmin, c);
                        lcmax = _mm_max_ps(lcmax, c);
                    }

                    for (auto i = split_idx; i < request.start_index + request.num_refs; ++i)
                    {
                        auto idx = refs[i];
                        rmin = _mm_min_ps(rmin, _mm_load_ps(&aabb_min[idx].x));
                        rmax = _mm_max_ps(rmax, _mm_load_ps(&aabb_max[idx].x));

                        auto c = _mm_load_ps(&aabb_centroid[idx].x);
                        rcmin = _mm_min_ps(rcmin, c);
                        rcmax = _mm_max_ps(rcmax, c);
                    }
                }

                // Create left request
                request_left.aabb_min = lmin;
                request_left.aabb_max = lmax;
                request_left.centroid_aabb_min = lcmin;
                request_left.centroid_aabb_max = lcmax;
                request_left.start_index = request.start_index;
                request_left.num_refs = split_idx - request.start_index;
                request_left.level = request.level + 1;
                //request_left.index = request.index + 1;
                request_left.index = request.index << 1;
                request_left.ptr = &node->lc;

                // Create right request
                request_right.aabb_min = rmin;
                request_right.aabb_max = rmax;
                request_right.centroid_aabb_min = rcmin;
                request_right.centroid_aabb_max = rcmax;
                request_right.start_index = split_idx;
                request_right.num_refs = request.num_refs - request_left.num_refs;
                request_right.level = request.level + 1;
                //request_right.index = static_cast<std::uint32_t>(request.index + request_left.num_refs * 2);
                request_right.index = (request.index << 1) + 1;
                request_right.ptr = &node->rc;

                return kInternal;
            };

            auto worker_thread = [&]() {
                thread_local std::stack<SplitRequest2> local_requests;

                for (;;) {
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        cv.wait(lock, [&]() { return !requests.empty() || shutdown; });

                        if (shutdown) return;

                        local_requests.push(requests.top());
                        requests.pop();
                    }

                    _MM_ALIGN16 SplitRequest2 request_left;
                    _MM_ALIGN16 SplitRequest2 request_right;
                    _MM_ALIGN16 SplitRequest2 request;

                    while (!local_requests.empty()) {
                        request = local_requests.top();
                        local_requests.pop();

                        auto node_type = HandleRequest(
                            request,
                            aabb_min.get(),
                            aabb_max.get(),
                            aabb_centroid.get(),
                            NULL,//metadata,
                            m_indices,
                            numbounds,
                            request_left,
                            request_right);

                        if (node_type == kLeaf) {
                            num_refs_processed += static_cast<std::uint32_t>(request.num_refs);
                            continue;
                        }

                        if (request_right.num_refs > 4096u) {
                            std::unique_lock<std::mutex> lock(mutex);
                            requests.push(request_right);
                            cv.notify_one();
                        }
                        else {
                            local_requests.push(request_right);
                        }

                        // TODO: so each thread always processes all the left children?!? (gboisse)
                        local_requests.push(request_left);
                    }
                }
            };

            auto num_threads = std::thread::hardware_concurrency();
            std::vector<std::thread> threads(num_threads);

            for (auto i = 0u; i < num_threads; ++i) {
                threads[i] = std::move(std::thread(worker_thread));
                threads[i].detach();
            }

            while (num_refs_processed != numbounds) {
                // TODO: this should be a signal of sort... (gboisse)
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }

            shutdown = true;
        }
#elif 0
        std::stack<SplitRequest> stack;
        // Put initial request into the stack
        stack.push(init);

        while (!stack.empty())
        {
            // Fetch new request
            SplitRequest req = stack.top();
            stack.pop();

            Node* node = AllocateNode();
            node->bounds = req.bounds;

            // Create leaf node if we have enough prims
            if (req.numprims < 2)
            {
                node->type = kLeaf;
                node->startidx = (int)primitives_.size();
                node->numprims = req.numprims;
                for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
                {
                    primitives_.push_back(prims[primindices[i]]);
                }
            }
            else
            {
                node->type = kInternal;

                // Choose the maximum extent
                int axis = req.centroid_bounds.maxdim();
                float border = req.centroid_bounds.center()[axis];

                // Start partitioning and updating extents for children at the same time
                bbox leftbounds, rightbounds, leftcentroid_bounds, rightcentroid_bounds;
                int splitidx = 0;

                if (req.centroid_bounds.extents()[axis] > 0.f)
                {

                    auto first = req.startidx;
                    auto last = req.startidx + req.numprims;

                    while (1)
                    {
                        while ((first != last) &&
                            centroids[primindices[first]][axis] < border)
                        {
                            leftbounds.grow(bounds[primindices[first]]);
                            leftcentroid_bounds.grow(centroids[primindices[first]]);
                            ++first;
                        }

                        if (first == last--) break;

                        rightbounds.grow(bounds[primindices[first]]);
                        rightcentroid_bounds.grow(centroids[primindices[first]]);

                        while ((first != last) &&
                            centroids[primindices[last]][axis] >= border)
                        {
                            rightbounds.grow(bounds[primindices[last]]);
                            rightcentroid_bounds.grow(centroids[primindices[last]]);
                            --last;
                        }

                        if (first == last) break;

                        leftbounds.grow(bounds[primindices[last]]);
                        leftcentroid_bounds.grow(centroids[primindices[last]]);

                        std::swap(primindices[first++], primindices[last]);
                    }

                    splitidx = first;
                }
                else
                {
                    splitidx = req.startidx + (req.numprims >> 1);

                    for (int i = req.startidx; i < splitidx; ++i)
                    {
                        leftbounds.grow(bounds[primindices[i]]);
                        leftcentroid_bounds.grow(centroids[primindices[i]]);
                    }

                    for (int i = splitidx; i < req.startidx + req.numprims; ++i)
                    {
                        rightbounds.grow(bounds[primindices[i]]);
                        rightcentroid_bounds.grow(centroids[primindices[i]]);
                    }
                }

                // Left request
                SplitRequest leftrequest = { req.startidx, splitidx - req.startidx, &node->lc, leftbounds, leftcentroid_bounds };
                // Right request
                SplitRequest rightrequest = { splitidx, req.numprims - (splitidx - req.startidx), &node->rc, rightbounds, rightcentroid_bounds };

                // Put those to stack
                stack.push(leftrequest);
                stack.push(rightrequest);
            }

            // Set parent ptr if any
            if (req.ptr) *req.ptr = node;
        }
#else
        BuildNode(init, bounds, &centroids[0], &m_indices[0]);
#endif

        // Set root_ pointer
        m_root = &m_nodes[0];
    }

    void Bvh::PrintStatistics(std::ostream& os) const
    {
        os << "Class name: " << "Bvh\n";
        os << "SAH: " << (m_usesah ? "enabled\n" : "disabled\n");
        os << "SAH bins: " << m_num_bins << "\n";
        os << "Number of triangles: " << m_indices.size() << "\n";
        os << "Number of nodes: " << m_nodecnt << "\n";
        os << "Tree height: " << GetHeight() << "\n";
    }

}
