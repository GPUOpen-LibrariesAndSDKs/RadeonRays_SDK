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
#include <stack>

#define PARALLEL_BUILD

namespace RadeonRays
{
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

    void Bvh2::BuildImpl(
        __m128 scene_min,
        __m128 scene_max,
        __m128 centroid_scene_min,
        __m128 centroid_scene_max,
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

        auto constexpr inf = std::numeric_limits<float>::infinity();
        auto m128_plus_inf = _mm_set_ps(inf, inf, inf, inf);
        auto m128_minus_inf = _mm_set_ps(-inf, -inf, -inf, -inf);

#ifndef PARALLEL_BUILD
        #error TODO: implement (gboisse)
#else
        std::mutex mutex;
        std::condition_variable cv;
        std::atomic_bool shutdown = false;
        std::atomic_uint32_t num_refs_processed = 0;

        std::stack<SplitRequest> requests;

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
            threads[i] = std::move(std::thread(worker_thread));
            threads[i].detach();
        }

        while (num_refs_processed != num_aabbs)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        shutdown = true;
#endif
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
        return kLeaf;
    }
}
