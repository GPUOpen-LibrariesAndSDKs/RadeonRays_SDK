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
    \file intersector_hlbvh.h
    \author Dmitry Kozlov
    \version 1.0
    \brief Intersector implementation based on HLBVH acceleration structues

    IntersectorHlbvh implementation is based on the following paper:
    "HLBVH: Hierarchical LBVH Construction for Real-Time Ray Tracing"
    Jacopo Pantaleoni (NVIDIA), David Luebke (NVIDIA), in High Performance Graphics 2010, June 2010
    https://research.nvidia.com/sites/default/files/publications/HLBVH-final.pdf


    Intersector is using simple stack-based traversal method. Acceleration structure is built 
    on GPU.

    Pros:
        -Very fast to build and update.
    Cons:
        -Poor BVH quality, slow traversal.
 */
 
namespace RadeonRays
{
    class Hlbvh;

    /** 
    \brief Intersector implementation using HLBVH.
    */
    class IntersectorHlbvh : public Intersector
    {
    public:
        // Constructor
        IntersectorHlbvh(Calc::Device* device);

    private:
        // World processing implementation
        void Process(World const& world) override;

        // Intersection implemenation
        void Intersect(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays, 
            std::uint32_t max_rays, Calc::Buffer *hits, 
            Calc::Event const *wait_event, Calc::Event **event) const override;

        // Occlusion implemenation
        void Occluded(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays, 
            std::uint32_t max_rays, Calc::Buffer *hits, 
            Calc::Event const *wait_event, Calc::Event **event) const override;

    private:
        struct GpuData;
        struct ShapeData;

        // Implementation data
        std::unique_ptr<GpuData> m_gpudata;
        // Bvh data structure
        std::unique_ptr<Hlbvh> m_bvh;
    };
}
