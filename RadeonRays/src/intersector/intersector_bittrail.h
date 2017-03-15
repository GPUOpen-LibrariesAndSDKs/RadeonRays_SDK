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

#include "calc.h"
#include "device.h"
#include "intersector.h"
#include <memory>
/**
    \file intersector_bittrail.h
    \author Dmitry Kozlov
    \version 1.0
    \brief Intersector implementation based on BVH stackless traversal using bit trail and perfect hashing.

    Intersector is using binary BVH with two bounding boxes per node.
    Traversal is using bit trail and perfect hashing for backtracing and based on the following paper:

    "Efficient stackless hierarchy traversal on GPUs with backtracking in constant time""
    Nikolaus Binder, Alexander Keller
    http://dl.acm.org/citation.cfm?id=2977343

    Traversal pseudocode:

        while(addr is valid)
        {
            node <- fetch next node at addr
            index <- 1
            trail <- 0
            if (node is leaf)
                intersect leaf
            else
            {
                intersect ray vs left child
                intersect ray vs right child
                if (intersect any of children)
                {
                    index <- index << 1
                    trail <- trail << 1
                    determine closer child
                    if intersect both
                    {
                        trail <- trail ^ 1
                        addr = closer child
                    }
                    else
                    {
                        addr = intersected child
                    }
                    if addr is right
                        index <- index ^ 1
                    continue
                }
            }

            if (trail == 0)
            {
                break
            }

            num_levels = count trailing zeroes in trail
            trail <- (trail << num_levels) & 1
            index <- (index << num_levels) & 1

            addr = hash[index]
        }

    Pros:
        -Very fast traversal.
        -Benefits from BVH quality optimization.
        -Low VGPR pressure
    Cons:
        -Depth is limited.
        -Generates global memory traffic.
 */

namespace RadeonRays
{
    class Bvh;

    class IntersectorBitTrail : public Intersector
    {
    public:
        IntersectorBitTrail(Calc::Device* device);

    private:
        void Process(World const& world) override;

        void Intersect(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays, 
            std::uint32_t max_rays, Calc::Buffer *hits, 
            Calc::Event const *wait_event, Calc::Event **event) const override;

        void Occluded(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays, 
            std::uint32_t max_rays, Calc::Buffer *hits, 
            Calc::Event const *wait_event, Calc::Event **event) const override;

    private:
        struct GpuData;
        struct ShapeData;

        // Implementation data
        std::unique_ptr<GpuData> m_gpudata;
        // Bvh data structure
        std::unique_ptr<Bvh> m_bvh;
    };
}
