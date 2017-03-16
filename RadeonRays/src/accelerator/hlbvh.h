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

#include "calc.h"
#include "device.h"
#include "executable.h"
#include "math/bbox.h"
#include "../accelerator/bvh.h"

#include <memory>

namespace RadeonRays
{
    ///< The class represents hierarchical LBVH constructed fully on GPU
    ///< https://research.nvidia.com/sites/default/files/publications/HLBVH-final.pdf
    ///
    class Hlbvh
    {
    public:
        Hlbvh(Calc::Device* device);
        
        virtual ~Hlbvh();
        
        // World space bounding box
        bbox const& Bounds() const;
        
        // Build function
        void Build(bbox const* bounds, int numbounds);
        
        // This class has its own  GPU data,
        // and it provides it as an interface in GPU memory
        struct GpuData;
        GpuData const& GetGpuData() const { return *m_gpudata; }
        
        // Get reordered indices
        int const* GetIndices() const { return &m_prim_indices[0]; }

    
    protected:
        // Build function
        virtual void BuildImpl(bbox const* bounds, int numbounds);
        
    private:
        void InitGpuData();
        void AllocateBuffers(size_t numprims);
        
        Hlbvh(Hlbvh const&);
        Hlbvh& operator = (Hlbvh const&);
        
        // Context for GPU work submision
        Calc::Device* m_device;
        
        // Device data types
        struct Box;
        struct SplitRequest;
        struct Node;
        
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
        // Device
        Calc::Device* device;

        // Parallel primitives
        Calc::Primitives* pp;

        // GPU program
        Calc::Executable* executable;
        Calc::Function* morton_code_func;
        Calc::Function* build_func;
        Calc::Function* refit_func;
        
        // Parallel primitives instance
        //CLWParallelPrimitives pp_;
        
        // Bbox centers
        Calc::Buffer* positions;
        
        // Morton codes of the primitive centroids
        Calc::Buffer* morton_codes;
        // Reorder indices
        Calc::Buffer* prim_indices;
        
        Calc::Buffer* sorted_morton_codes;
        Calc::Buffer* sorted_prim_indices;
        
        // Nodes: first N-1 - internal nodes, last N - leafs
        Calc::Buffer* nodes;
        
        // Bounds
        Calc::Buffer* bounds;
        Calc::Buffer* sorted_bounds;
        Calc::Buffer* scene_bound;
        
        // Atomic flags
        Calc::Buffer*  flags;

        GpuData(Calc::Device* dev)
            : device(dev)
        {
        }

        ~GpuData()
        {
            executable->DeleteFunction(morton_code_func);
            executable->DeleteFunction(build_func);
            executable->DeleteFunction(refit_func);
            device->DeleteExecutable(executable);
            device->DeletePrimitives(pp);
            device->DeleteBuffer(positions);
            device->DeleteBuffer(morton_codes);
            device->DeleteBuffer(prim_indices);
            device->DeleteBuffer(sorted_morton_codes);
            device->DeleteBuffer(sorted_prim_indices);
            device->DeleteBuffer(nodes);
            device->DeleteBuffer(bounds);
            device->DeleteBuffer(sorted_bounds);
            device->DeleteBuffer(scene_bound);
            device->DeleteBuffer(flags);
        }
    };
}

#endif // HLBVH_H
