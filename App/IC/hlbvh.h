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
#ifndef HLBVH_H
#define HLBVH_H

#include "radeon_rays.h"
#include "CLW.h"
#include "math/bbox.h"
#include "radiance_cache.h"

#include <memory>

namespace Baikal
{
    ///< The class represents hierarchical LBVH constructed fully on GPU
    ///< https://research.nvidia.com/sites/default/files/publications/HLBVH-final.pdf
    ///
    class Hlbvh
    {
    public:
        struct Node;

        Hlbvh(CLWContext context);
        
        virtual ~Hlbvh();
        
        // World space bounding box
        RadeonRays::bbox const& Bounds() const;
        
        // Build function
        void Build(CLWBuffer<RadianceCache::RadianceProbeDesc> records, std::size_t num_records);

        // This class has its own  GPU data,
        // and it provides it as an interface in GPU memory
        struct GpuData;
        GpuData const& GetGpuData() const { return *m_gpudata; }

    protected:
        // Build function
        virtual void BuildImpl(CLWBuffer<RadianceCache::RadianceProbeDesc> records, std::size_t num_records);
        
    private:
        void InitGpuData();
        void AllocateBuffers(size_t numprims);
        
        Hlbvh(Hlbvh const&);
        Hlbvh& operator = (Hlbvh const&);
        
        // Context for GPU work submision
        CLWContext m_context;
        
        // Device data types
        struct Box;
        struct SplitRequest;
        
        // GPU data
        std::unique_ptr<GpuData> m_gpudata;
        
        // Primitive indices
        std::vector<int> m_prim_indices;
    };
    
    // BVH node
    struct Hlbvh::Node
    {
        int parent;
        int left;
        int right;
        int next;
    };
    
    struct Hlbvh::GpuData
    {
        // Parallel primitives
        CLWParallelPrimitives pp;

        // GPU program
        CLWProgram program;
        CLWKernel morton_code_func;
        CLWKernel build_func;
        CLWKernel refit_func;
        
        // Bbox centers
        CLWBuffer<RadeonRays::float3> positions;
        
        // Morton codes of the primitive centroids
        CLWBuffer<int> morton_codes;
        // Reorder indices
        CLWBuffer<int> prim_indices;
        
        CLWBuffer<int> sorted_morton_codes;
        CLWBuffer<int> sorted_prim_indices;
        
        // Nodes: first N-1 - internal nodes, last N - leafs
        CLWBuffer<Node> nodes;
        
        // Bounds
        CLWBuffer<RadeonRays::bbox> bounds;
        CLWBuffer<RadeonRays::bbox> sorted_bounds;
        
        // Atomic flags
        CLWBuffer<int>  flags;

        GpuData()
        {
        }

        ~GpuData()
        {
        }
    };
}

#endif // HLBVH_H
