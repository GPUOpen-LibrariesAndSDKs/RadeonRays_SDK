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
#ifndef STRATEGY_H
#define STRATEGY_H

#include "radeon_rays.h"
#include "calc.h"
#include "buffer.h"
#include "event.h"

namespace RadeonRays
{
    class World;

    class Intersector
    {
    public:
        Intersector(Calc::Device* device);

        virtual ~Intersector() = default;

        void Preprocess(World const& world);

        void QueryIntersection(std::uint32_t queue_idx, Calc::Buffer const* rays, std::uint32_t num_rays,
            Calc::Buffer* hits, Calc::Event const* wait_event, Calc::Event** event) const;

        void QueryOcclusion(std::uint32_t queue_idx, Calc::Buffer const* rays, std::uint32_t num_rays,
            Calc::Buffer* hits, Calc::Event const* wait_event, Calc::Event** event) const;

        void QueryIntersection(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays, 
            std::uint32_t max_rays, Calc::Buffer *hits, Calc::Event const *wait_event, Calc::Event **event) const;

        void QueryOcclusion(std::uint32_t queue_idx, Calc::Buffer const* rays, Calc::Buffer const* num_rays,
            std::uint32_t max_rays, Calc::Buffer* hits, Calc::Event const* wait_event, Calc::Event** event) const;

        Intersector(Intersector const&) = delete;
        Intersector& operator = (Intersector const&) = delete;

    private:
        virtual void PreprocessImpl(World const& world) = 0;

        virtual void Intersect(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays, 
            std::uint32_t max_rays, Calc::Buffer *hits, 
            Calc::Event const *wait_event, Calc::Event **event) const = 0;

        virtual void Occluded(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays, 
            std::uint32_t max_rays, Calc::Buffer *hits, 
            Calc::Event const *wait_event, Calc::Event **event) const = 0;

    protected: 
        Calc::Device* m_device;
        Calc::Buffer* m_counter; 
    };
}

#ifdef RR_EMBED_KERNELS
#if USE_OPENCL
#    include <RadeonRays/src/kernelcache/kernels_cl.h>
#endif

#if USE_VULKAN
#    include <RadeonRays/src/kernelcache/kernels_vk.h>
#endif
#endif // RR_EMBED_KERNELS


#endif // STRATEGY_H
