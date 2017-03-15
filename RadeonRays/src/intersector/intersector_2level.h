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
    \file intersector_2level.h
    \author Dmitry Kozlov
    \version 1.0
    \brief Intersector implementation based on 2-level BVH with skip links.

    IntersectorSkipLinks implementation is based on the modification of the following paper:
    "Efficiency Issues for Ray Tracing" Brian Smits
    http://www.cse.chalmers.se/edu/year/2016/course/course/TDA361/EfficiencyIssuesForRayTracing.pdf

    Intersector is using binary BVH with a single bounding box per node. BVH layout guarantees
    that left child of an internal node lies right next to it in memory. Each BVH node has a 
    skip link to the node traversed next. Intersector builds its own BVH for each scene object 
    and then top level BVH across all bottom level BVHs. Top level leafs keep object transforms and
    might reference other leafs making instancing possible.


    Pros:
        -Simple and efficient kernel with low VGPR pressure.
        -Can traverse trees of arbitrary depth.
        -Supports motion blur.
        -Supports instancing.
        -Fast to refit.
    Cons:
        -Travesal order is fixed, so poor algorithmic characteristics.
        -Does not benefit from BVH quality optimizations.
 */

#pragma once

#include "calc.h"
#include "device.h"
#include "intersector.h"
#include <memory>
#include <vector>


namespace RadeonRays
{
    class Bvh;

    /** 
    \brief Intersector implementation using 2-level skip links BVH
    */
    class IntersectorTwoLevel : public Intersector
    {
    public:
        // Constructor
        IntersectorTwoLevel(Calc::Device* device);

    private:
        // World processing implementation
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
        // Gpu data
        struct GpuData;
        struct CpuData;
        struct ShapeData;
        struct Face;

        std::unique_ptr<GpuData> m_gpudata;
        std::unique_ptr<CpuData> m_cpudata;
        std::vector<std::unique_ptr<Bvh> > m_bvhs;
    };
}

