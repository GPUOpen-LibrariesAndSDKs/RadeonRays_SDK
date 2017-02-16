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
#include "hlbvh.h"
#include <vector>
#include <numeric>
#include <chrono>
#include <cstring>
#include <iostream>
#include <assert.h>

namespace Baikal
{
    using namespace RadeonRays;

    static int kWorkGroupSize = 64;
    static int kInitialCapacity = 100000;
    
    Hlbvh::Hlbvh(CLWContext context)
    : m_context(context)
    , m_gpudata(new GpuData())
    {
        InitGpuData();
    }
    
    
    Hlbvh::~Hlbvh()
    {
    }
    
    void Hlbvh::AllocateBuffers(size_t numprims)
    {
        m_gpudata->positions = m_context.CreateBuffer<float3>(numprims, CL_MEM_READ_WRITE);
        m_gpudata->morton_codes = m_context.CreateBuffer<int>(numprims, CL_MEM_READ_WRITE);

        std::vector<int> iota(numprims);
        std::iota(iota.begin(), iota.end(), 0);
        
        m_gpudata->prim_indices = m_context.CreateBuffer<int>(numprims, CL_MEM_READ_WRITE, &iota[0]);
        m_gpudata->morton_codes = m_context.CreateBuffer<int>(numprims, CL_MEM_READ_WRITE);
        m_gpudata->sorted_morton_codes = m_context.CreateBuffer<int>(numprims, CL_MEM_READ_WRITE);
        m_gpudata->sorted_prim_indices = m_context.CreateBuffer<int>(numprims, CL_MEM_READ_WRITE);
        
        m_gpudata->nodes = m_context.CreateBuffer<Node>(2 * numprims, CL_MEM_READ_WRITE);
        // Bounds
        m_gpudata->bounds = m_context.CreateBuffer<RadeonRays::bbox>(numprims, CL_MEM_READ_WRITE);
        m_gpudata->sorted_bounds = m_context.CreateBuffer<RadeonRays::bbox>(2 * numprims, CL_MEM_READ_WRITE);
        // Propagation flags
        m_gpudata->flags = m_context.CreateBuffer<int>(2 * numprims, CL_MEM_READ_WRITE);
    }
    
    void Hlbvh::InitGpuData()
    {
        // Create parallel primitives
        m_gpudata->pp = CLWParallelPrimitives(m_context);

        std::string buildopts;

        buildopts.append(" -cl-mad-enable -cl-fast-relaxed-math -cl-std=CL1.2 -I . ");

        buildopts.append(
#if defined(__APPLE__)
            "-D APPLE "
#elif defined(_WIN32) || defined (WIN32)
            "-D WIN32 "
#elif defined(__linux__)
            "-D __linux__ "
#else
            ""
#endif
        );

        // Load kernels
        m_gpudata->program = CLWProgram::CreateFromFile("../App/CL/hlbvh_build.cl", buildopts.c_str(), m_context);


        m_gpudata->morton_code_func = m_gpudata->program.GetKernel("CalcRecordBoundsAndMortonCodes");
        m_gpudata->build_func = m_gpudata->program.GetKernel("BuildHierarchy");
        m_gpudata->refit_func = m_gpudata->program.GetKernel("RefitBounds");

        // Allocate GPU buffers
        AllocateBuffers(kInitialCapacity);
    }
    
    // Build function
    void Hlbvh::Build(CLWBuffer<RadianceCache::RadianceProbeDesc> records, std::size_t num_records)
    {
        auto s = std::chrono::high_resolution_clock::now();

        BuildImpl(records, num_records);

        m_context.Finish(0);
        // Note, that this is total time spent for setup and construction 
        // including the time spent waiting in the queue.
        auto d = std::chrono::high_resolution_clock::now() - s;
        std::cout << "HLBVH setup + construction CPU time: " << std::chrono::duration_cast<std::chrono::milliseconds>(d).count() << "ms\n";
    }
    
    
    // World space bounding box
    bbox const& Hlbvh::Bounds() const
    {
        // TODO: implement me
        static bbox s_tmp;
        return s_tmp;
    }
    
    // Build function
    void Hlbvh::BuildImpl(CLWBuffer<RadianceCache::RadianceProbeDesc> records, std::size_t num_records)
    {
        int size = num_records;

        std::cout << "Num probes " << num_records << "\n";

        // Make sure to allocate enough mem on GPU
        // We are trying to reuse space as reallocation takes time
        // but this call might be really frequent
        if (size > m_gpudata->positions.GetElementCount())
        {
            AllocateBuffers(size);
        }

        // Initialize flags with zero 
        {
            m_context.FillBuffer(0, m_gpudata->flags, 0, m_gpudata->flags.GetElementCount()).Wait();
        }

        // Calculate Morton codes array
        int arg = 0;
        m_gpudata->morton_code_func.SetArg(arg++, records);
        m_gpudata->morton_code_func.SetArg(arg++, size);
        m_gpudata->morton_code_func.SetArg(arg++, m_gpudata->morton_codes);
        m_gpudata->morton_code_func.SetArg(arg++, m_gpudata->bounds);

        // Calculate global size
        int globalsize = ((size + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        // Launch Morton codes kernel
        m_context.Launch1D(0, globalsize, kWorkGroupSize, m_gpudata->morton_code_func);

#ifdef _DEBUG
        std::vector<RadeonRays::bbox> bboxes(size);
        m_context.ReadBuffer(0, m_gpudata->bounds, &bboxes[0], size).Wait();

        std::vector<int> codes(size);
        m_context.ReadBuffer(0, m_gpudata->morton_codes, &codes[0], size).Wait();
#endif

        
        // Sort primitives according to their Morton codes
        m_gpudata->pp.SortRadix(0, m_gpudata->morton_codes, m_gpudata->sorted_morton_codes, m_gpudata->prim_indices, m_gpudata->sorted_prim_indices, size);
       
        // Prepare tree construction kernel
        arg = 0;
        m_gpudata->build_func.SetArg(arg++, m_gpudata->sorted_morton_codes);
        m_gpudata->build_func.SetArg(arg++, m_gpudata->bounds);
        m_gpudata->build_func.SetArg(arg++, m_gpudata->sorted_prim_indices);
        m_gpudata->build_func.SetArg(arg++, size);
        m_gpudata->build_func.SetArg(arg++, m_gpudata->nodes);
        m_gpudata->build_func.SetArg(arg++, m_gpudata->sorted_bounds);
        
        // Calculate global size
        globalsize = ((size + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;
        
        // Launch hierarchy emission kernel
        m_context.Launch1D(0, globalsize, kWorkGroupSize, m_gpudata->build_func);
        
        // Refit bounds
        arg = 0;
        m_gpudata->refit_func.SetArg(arg++, m_gpudata->sorted_bounds);
        m_gpudata->refit_func.SetArg(arg++, sizeof(size), &size);
        m_gpudata->refit_func.SetArg(arg++, m_gpudata->nodes);
        m_gpudata->refit_func.SetArg(arg++, m_gpudata->flags);

        globalsize = ((size + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        // Launch refit kernel
        m_context.Launch1D(0, globalsize, kWorkGroupSize, m_gpudata->refit_func);

        /// DEBUG CODE
#ifdef _DEBUG
        std::vector<Node> nodes(size * 2 - 1);
        m_context.ReadBuffer(0, m_gpudata->nodes, &nodes[0], 0, size * 2 - 1).Wait();
        bboxes.resize(size * 2 - 1);
        m_context.ReadBuffer(0, m_gpudata->sorted_bounds, &bboxes[0], 0, size * 2 - 1).Wait();
#endif
    }
}
