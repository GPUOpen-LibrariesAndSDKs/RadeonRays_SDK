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
#include "bvh2.h"

#include <atomic>
#include <mutex>
#include <numeric>

#define PARALLEL_BUILD

// Macro for allocating 16-byte aligned stack memory
#define STACK_ALLOC(COUNT, TYPE) static_cast<TYPE *>(Align(16u, (COUNT) * sizeof(TYPE), (COUNT) * sizeof(TYPE) + 15u, alloca((COUNT) * sizeof(TYPE) + 15u)))

namespace RadeonRays
{
    inline
    void *Align(std::size_t alignment, std::size_t size, std::size_t space, void *ptr)
    {
        return std::align(alignment, size, ptr, space);
    }

#ifdef __GNUC__
    #define clz(x) __builtin_clz(x)
    #define ctz(x) __builtin_ctz(x)
#else
    inline
    std::uint32_t popcnt(std::uint32_t x)
    {
        x -= ((x >> 1) & 0x55555555);
        x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
        x = (((x >> 4) + x) & 0x0f0f0f0f);
        x += (x >> 8);
        x += (x >> 16);
        return x & 0x0000003f;
    }

    inline
    std::uint32_t clz(std::uint32_t x)
    {
        x |= (x >> 1);
        x |= (x >> 2);
        x |= (x >> 4);
        x |= (x >> 8);
        x |= (x >> 16);
        return 32 - popcnt(x);
    }

    inline
    std::uint32_t ctz(std::uint32_t x)
    {
        return popcnt((std::uint32_t)(x & -(int)x) - 1);
    }
#endif

    inline
    __m128 aabb_surface_area(__m128 pmin, __m128 pmax)
    {
        auto ext = _mm_sub_ps(pmax, pmin);
        auto xxy = _mm_shuffle_ps(ext, ext, _MM_SHUFFLE(3, 1, 0, 0));
        auto yzz = _mm_shuffle_ps(ext, ext, _MM_SHUFFLE(3, 2, 2, 1));
        return _mm_mul_ps(_mm_dp_ps(xxy, yzz, 0xff), _mm_set_ps(2.f, 2.f, 2.f, 2.f));
    }

    inline
    __m128 aabb_extents(__m128 pmin, __m128 pmax)
    {
        return _mm_sub_ps(pmax, pmin);
    }

    inline
    std::uint32_t aabb_max_extent_axis(__m128 pmin, __m128 pmax)
    {
        auto xyz = _mm_sub_ps(pmax, pmin);
        auto yzx = _mm_shuffle_ps(xyz, xyz, _MM_SHUFFLE(3, 0, 2, 1));
        auto m0 = _mm_max_ps(xyz, yzx);
        auto m1 = _mm_shuffle_ps(m0, m0, _MM_SHUFFLE(3, 0, 2, 1));
        auto m2 = _mm_max_ps(m0, m1);
        auto cmp = _mm_cmpeq_ps(xyz, m2);
        return ctz(_mm_movemask_ps(cmp));
    }

    inline
    float mm_select(__m128 v, std::uint32_t index)
    {
        _MM_ALIGN16 float temp[4];
        _mm_store_ps(temp, v);
        return temp[index];
    }

    inline
    bool aabb_contains_point(
            float const* aabb_min,
            float const* aabb_max,
            float const* point)
    {
        return point[0] >= aabb_min[0] &&
               point[0] <= aabb_max[0] &&
               point[1] >= aabb_min[1] &&
               point[1] <= aabb_max[1] &&
               point[2] >= aabb_min[2] &&
               point[2] <= aabb_max[2];
    }

    struct Bvh2::SplitRequest
    {
        __m128 aabb_min;
        __m128 aabb_max;
        __m128 centroid_aabb_min;
        __m128 centroid_aabb_max;
        std::size_t start_index;
        std::size_t num_refs;
        std::uint32_t level;
        std::uint32_t index;
    };

    void Bvh2::Clear()
    {
        for (auto i = 0u; i < m_nodecount; ++i)
            m_nodes[i].~Node();
        Deallocate(m_nodes);
        m_nodes = nullptr;
        m_nodecount = 0;
    }

    void Bvh2::BuildImpl(
        __m128 MSVC_X86_ALIGNMENT_FIX scene_min,
        __m128 MSVC_X86_ALIGNMENT_FIX scene_max,
        __m128 MSVC_X86_ALIGNMENT_FIX centroid_scene_min,
        __m128 MSVC_X86_ALIGNMENT_FIX centroid_scene_max,
        const float3 *aabb_min,
        const float3 *aabb_max,
        const float3 *aabb_centroid,
        const MetaDataArray &metadata,
        std::size_t num_aabbs)
    {
        RefArray refs(num_aabbs);
        std::iota(refs.begin(), refs.end(), 0);

        m_nodecount = 2 * num_aabbs - 1;
        m_nodes = reinterpret_cast<Node*>(
            Allocate(sizeof(Node) * m_nodecount, 16u));

        for (auto i = 0u; i < m_nodecount; ++i)
            new (&m_nodes[i]) Node;

        auto constexpr inf = std::numeric_limits<float>::infinity();
        auto m128_plus_inf = _mm_set_ps(inf, inf, inf, inf);
        auto m128_minus_inf = _mm_set_ps(-inf, -inf, -inf, -inf);

#ifndef PARALLEL_BUILD
        _MM_ALIGN16 SplitRequest requests[kStackSize];

        auto sptr = 0u;

        requests[sptr++] = SplitRequest{
            scene_min,
            scene_max,
            centroid_scene_min,
            centroid_scene_max,
            0,
            num_aabbs,
            0u,
            0u
        };

        while (sptr > 0u)
        {
            auto request = requests[--sptr];

            auto &request_left{ requests[sptr++] };

            if (sptr == kStackSize)
            {
                throw std::runtime_error("Build stack overflow");
            }

            auto &request_right{ requests[sptr++] };

            if (sptr == kStackSize)
            {
                throw std::runtime_error("Build stack overflow");
            }

            if (HandleRequest(
                request,
                aabb_min,
                aabb_max,
                aabb_centroid,
                metadata,
                refs,
                num_aabbs,
                request_left,
                request_right) ==
                NodeType::kLeaf)
            {
                --sptr;
                --sptr;
            }
        }
#else
        // Parallel build variables
        // Global requests stack
        std::stack<SplitRequest> requests;
        // Condition to wait on the global stack
        std::condition_variable cv;
        // Mutex to guard cv
        std::mutex mutex;
        // Indicates if we need to shutdown all the threads
        std::atomic<bool> shutdown;
        // Number of primitives processed so far
        std::atomic<std::uint32_t> num_refs_processed;

        num_refs_processed.store(0);
        shutdown.store(false);

        requests.push(SplitRequest{
            scene_min,
            scene_max,
            centroid_scene_min,
            centroid_scene_max,
            0,
            num_aabbs,
            0u,
            0u
        });

        auto worker_thread = [&]()
        {
            thread_local std::stack<SplitRequest> local_requests;

            for (;;)
            {
                // Wait for signal
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    cv.wait(lock, [&]() { return !requests.empty() || shutdown; });

                    if (shutdown) return;

                    local_requests.push(requests.top());
                    requests.pop();
                }

                _MM_ALIGN16 SplitRequest request;
                _MM_ALIGN16 SplitRequest request_left;
                _MM_ALIGN16 SplitRequest request_right;

                // Process local requests
                while (!local_requests.empty())
                {
                    request = local_requests.top();
                    local_requests.pop();

                    auto node_type = HandleRequest(
                        request,
                        aabb_min,
                        aabb_max,
                        aabb_centroid,
                        metadata,
                        refs,
                        num_aabbs,
                        request_left,
                        request_right);

                    if (node_type == kLeaf)
                    {
                        num_refs_processed += static_cast<std::uint32_t>(request.num_refs);
                        continue;
                    }

                    if (request_right.num_refs > 4096u)
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        requests.push(request_right);
                        cv.notify_one();
                    }
                    else
                    {
                        local_requests.push(request_right);
                    }

                    local_requests.push(request_left);
                }
            }
        };

        auto num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads(num_threads);

        for (auto i = 0u; i < num_threads; ++i)
        {
            threads[i] = std::thread(worker_thread);
        }

        while (num_refs_processed != num_aabbs)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

         // Signal shutdown and wake up all the threads
        shutdown.store(true);
        cv.notify_all();
            
        // Wait for all the threads to finish
        for (auto i = 0u; i < num_threads; ++i)
        {
            threads[i].join();
        }
#endif
    }

    template <std::uint32_t axis>
    float Bvh2::FindSahSplit(
        const SplitRequest &request,
        const float3 *aabb_min,
        const float3 *aabb_max,
        const float3 *aabb_centroid,
        const std::uint32_t *refs)
    {
        auto sah = std::numeric_limits<float>::max();

        // Allocate stack memory
        auto bin_count = STACK_ALLOC(m_num_bins, std::uint32_t);
        auto bin_min = STACK_ALLOC(m_num_bins, __m128);
        auto bin_max = STACK_ALLOC(m_num_bins, __m128);

        auto constexpr inf = std::numeric_limits<float>::infinity();
        for (auto i = 0u; i < m_num_bins; ++i)
        {
            bin_count[i] = 0;
            bin_min[i] = _mm_set_ps(inf, inf, inf, inf);
            bin_max[i] = _mm_set_ps(-inf, -inf, -inf, -inf);
        }

        auto centroid_extent = aabb_extents(request.centroid_aabb_min,
            request.centroid_aabb_max);
        auto centroid_min = _mm_shuffle_ps(request.centroid_aabb_min,
            request.centroid_aabb_min,
            _MM_SHUFFLE(axis, axis, axis, axis));
        centroid_extent = _mm_shuffle_ps(centroid_extent,
            centroid_extent,
            _MM_SHUFFLE(axis, axis, axis, axis));
        auto centroid_extent_inv = _mm_rcp_ps(centroid_extent);
        auto area_inv = mm_select(
            _mm_rcp_ps(
                aabb_surface_area(
                    request.aabb_min,
                    request.aabb_max)
            ), 0);

        auto full4 = request.num_refs & ~0x3;
        auto num_bins = _mm_set_ps(
            static_cast<float>(m_num_bins), static_cast<float>(m_num_bins),
            static_cast<float>(m_num_bins), static_cast<float>(m_num_bins));

        for (auto i = request.start_index;
            i < request.start_index + full4;
            i += 4u)
        {
            auto idx0 = refs[i];
            auto idx1 = refs[i + 1];
            auto idx2 = refs[i + 2];
            auto idx3 = refs[i + 3];

            auto c = _mm_set_ps(
                aabb_centroid[idx3][axis],
                aabb_centroid[idx2][axis],
                aabb_centroid[idx1][axis],
                aabb_centroid[idx0][axis]);

            auto bin_idx = _mm_mul_ps(
                _mm_mul_ps(
                    _mm_sub_ps(c, centroid_min),
                    centroid_extent_inv), num_bins);

            auto bin_idx0 = std::min(static_cast<uint32_t>(mm_select(bin_idx, 0u)), m_num_bins - 1);
            auto bin_idx1 = std::min(static_cast<uint32_t>(mm_select(bin_idx, 1u)), m_num_bins - 1);
            auto bin_idx2 = std::min(static_cast<uint32_t>(mm_select(bin_idx, 2u)), m_num_bins - 1);
            auto bin_idx3 = std::min(static_cast<uint32_t>(mm_select(bin_idx, 3u)), m_num_bins - 1);

            ++bin_count[bin_idx0];
            ++bin_count[bin_idx1];
            ++bin_count[bin_idx2];
            ++bin_count[bin_idx3];

            bin_min[bin_idx0] = _mm_min_ps(
                bin_min[bin_idx0],
                _mm_load_ps(&aabb_min[idx0].x));
            bin_max[bin_idx0] = _mm_max_ps(
                bin_max[bin_idx0],
                _mm_load_ps(&aabb_max[idx0].x));
            bin_min[bin_idx1] = _mm_min_ps(
                bin_min[bin_idx1],
                _mm_load_ps(&aabb_min[idx1].x));
            bin_max[bin_idx1] = _mm_max_ps(
                bin_max[bin_idx1],
                _mm_load_ps(&aabb_max[idx1].x));
            bin_min[bin_idx2] = _mm_min_ps(
                bin_min[bin_idx2],
                _mm_load_ps(&aabb_min[idx2].x));
            bin_max[bin_idx2] = _mm_max_ps(
                bin_max[bin_idx2],
                _mm_load_ps(&aabb_max[idx2].x));
            bin_min[bin_idx3] = _mm_min_ps(
                bin_min[bin_idx3],
                _mm_load_ps(&aabb_min[idx3].x));
            bin_max[bin_idx3] = _mm_max_ps(
                bin_max[bin_idx3],
                _mm_load_ps(&aabb_max[idx3].x));
        }

        auto cm = mm_select(centroid_min, 0u);
        auto cei = mm_select(centroid_extent_inv, 0u);
        for (auto i = request.start_index + full4; i < request.start_index + request.num_refs; ++i)
        {
            auto idx = refs[i];
            auto bin_idx = std::min(static_cast<uint32_t>(
                m_num_bins *
                (aabb_centroid[idx][axis] - cm) *
                cei), m_num_bins - 1);
            ++bin_count[bin_idx];

            bin_min[bin_idx] = _mm_min_ps(
                bin_min[bin_idx],
                _mm_load_ps(&aabb_min[idx].x));
            bin_max[bin_idx] = _mm_max_ps(
                bin_max[bin_idx],
                _mm_load_ps(&aabb_max[idx].x));
        }

        auto right_min = STACK_ALLOC(m_num_bins - 1, __m128);
        auto right_max = STACK_ALLOC(m_num_bins - 1, __m128);
        auto tmp_min = _mm_set_ps(inf, inf, inf, inf);
        auto tmp_max = _mm_set_ps(-inf, -inf, -inf, -inf);

        for (auto i = m_num_bins - 1; i > 0; --i)
        {
            tmp_min = _mm_min_ps(tmp_min, bin_min[i]);
            tmp_max = _mm_max_ps(tmp_max, bin_max[i]);

            right_min[i - 1] = tmp_min;
            right_max[i - 1] = tmp_max;
        }

        tmp_min = _mm_set_ps(inf, inf, inf, inf);
        tmp_max = _mm_set_ps(-inf, -inf, -inf, -inf);
        auto  lc = 0u;
        auto  rc = request.num_refs;

        auto split_idx = -1;
        for (auto i = 0u; i < m_num_bins - 1; ++i)
        {
            tmp_min = _mm_min_ps(tmp_min, bin_min[i]);
            tmp_max = _mm_max_ps(tmp_max, bin_max[i]);
            lc += bin_count[i];
            rc -= bin_count[i];

            auto lsa = mm_select(
                aabb_surface_area(tmp_min, tmp_max), 0);
            auto rsa = mm_select(
                aabb_surface_area(right_min[i], right_max[i]), 0);

            auto s = m_traversal_cost + (lc * lsa + rc * rsa) * area_inv;

            if (s < sah)
            {
                split_idx = i;
                sah = s;
            }
        }

        return mm_select(centroid_min, 0u) + (split_idx + 1) * (mm_select(centroid_extent, 0u) / m_num_bins);
    }

    Bvh2::NodeType Bvh2::HandleRequest(
        const SplitRequest &request,
        const float3 *aabb_min,
        const float3 *aabb_max,
        const float3 *aabb_centroid,
        const MetaDataArray &metadata,
        RefArray &refs,
        std::size_t num_aabbs,
        SplitRequest &request_left,
        SplitRequest &request_right)
    {
        // Do we have enough primitives?
        if (request.num_refs <= kMaxLeafPrimitives)
        {
            EncodeLeaf(m_nodes[request.index], static_cast<std::uint32_t>(request.num_refs));
            for (auto i = 0u; i < request.num_refs; ++i)
            {
                auto face_data = metadata[refs[request.start_index + i]];
                SetPrimitive(
                    m_nodes[request.index],
                    i,
                    face_data);
            }
            return kLeaf;
        }

        // Otherwise, find split axis
        auto split_axis = aabb_max_extent_axis(
            request.centroid_aabb_min,
            request.centroid_aabb_max);

        auto split_axis_extent = mm_select(
            _mm_sub_ps(request.centroid_aabb_max,
                request.centroid_aabb_min),
            split_axis);

        auto split_value = mm_select(
            _mm_mul_ps(
                _mm_set_ps(0.5f, 0.5f, 0.5f, 0.5),
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

        // Partition the primitives
        if (split_axis_extent > 0.0f)
        {
            if (m_usesah && request.num_refs > kMinSAHPrimitives)
            {
                switch (split_axis)
                {
                case 0:
                    split_value = FindSahSplit<0>(
                        request,
                        aabb_min,
                        aabb_max,
                        aabb_centroid,
                        &refs[0]);
                    break;
                case 1:
                    split_value = FindSahSplit<1>(
                        request,
                        aabb_min,
                        aabb_max,
                        aabb_centroid,
                        &refs[0]);
                    break;
                case 2:
                    split_value = FindSahSplit<2>(
                        request,
                        aabb_min,
                        aabb_max,
                        aabb_centroid,
                        &refs[0]);
                    break;
                }
            }

            auto first = request.start_index;
            auto last = request.start_index + request.num_refs;

            for (;;)
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

        // Populate child requests
        request_left.aabb_min = lmin;
        request_left.aabb_max = lmax;
        request_left.centroid_aabb_min = lcmin;
        request_left.centroid_aabb_max = lcmax;
        request_left.start_index = request.start_index;
        request_left.num_refs = split_idx - request.start_index;
        request_left.level = request.level + 1;
        request_left.index = request.index + 1;

        request_right.aabb_min = rmin;
        request_right.aabb_max = rmax;
        request_right.centroid_aabb_min = rcmin;
        request_right.centroid_aabb_max = rcmax;
        request_right.start_index = split_idx;
        request_right.num_refs = request.num_refs - request_left.num_refs;
        request_right.level = request.level + 1;
        request_right.index = static_cast<std::uint32_t>(request.index + request_left.num_refs * 2);

        // Create internal node
        EncodeInternal(
            m_nodes[request.index],
            request.aabb_min,
            request.aabb_max,
            request_left.index,
            request_right.index);

        return kInternal;
    }
}
