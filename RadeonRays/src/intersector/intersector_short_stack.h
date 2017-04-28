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
/**
    \file intersector_short_stack.h
    \author Dmitry Kozlov
    \version 1.0
    \brief Intersector implementation based on BVH stacked travesal.

    Intersector is using binary BVH with two bounding boxes per node.
    Traversal is using a stack which is split into two parts:
        -Top part in fast LDS memory
        -Bottom part in slow global memory.
    Push operations first check for top part overflow and offload top
    part into slow global memory if necessary.
    Pop operations first check for top part emptiness and try to offload
    from bottom part if necessary. 

    Traversal pseudocode:

        while(addr is valid)
        {
            node <- fetch next node at addr

            if (node is leaf)
                intersect leaf
            else
            {
                intersect ray vs left child
                intersect ray vs right child
                if (intersect any of children)
                {
                    determine closer child
                    if intersect both
                    {
                        addr = closer child
                        check top stack and offload if necesary
                        push farther child into the stack
                    }
                    else
                    {
                        addr = intersected child
                    }
                    continue
                }
            }

            addr <- pop from top stack
            if (addr is not valid)
            {
                try loading data from bottom stack to top stack
                addr <- pop from top stack
            }
        }

    Pros:
        -Very fast traversal.
        -Benefits from BVH quality optimization.
    Cons:
        -Depth is limited.
        -Generates LDS traffic.
 */
#pragma once

#include "calc.h"
#include "device.h"
#include "intersector.h"
#include <memory>


namespace RadeonRays
{
    class Bvh;

    /** 
    \brief Intersector implementation using short stack BVH traversal
    */
    class IntersectorShortStack : public Intersector
    {
    public:
        // Constructor
        IntersectorShortStack(Calc::Device* device);

    private:
        // World preprocessing implementation
        void Process(World const& world) override;
        // Intersection implementation
        void Intersect(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays, 
            std::uint32_t max_rays, Calc::Buffer *hits, 
            Calc::Event const *wait_event, Calc::Event **event) const override;
        // Occlusion implementation
        void Occluded(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays, 
            std::uint32_t max_rays, Calc::Buffer *hits, 
            Calc::Event const *wait_event, Calc::Event **event) const override;

    private:
        struct GpuData;

        // Implementation data
        std::unique_ptr<GpuData> m_gpudata;
        // Bvh data structure
        std::unique_ptr<Bvh> m_bvh;
    };
}

