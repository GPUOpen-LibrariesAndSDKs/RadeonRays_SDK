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
#include "buffer.h"
#include "primitives.h"
#include "executable.h"
#include "../except/except.h"
#include "calc.h"
#include "event.h"

#include <vector>
#include <numeric>
#include <chrono>
#include <cstring>
#include <iostream>
#include <assert.h>

#ifdef RR_EMBED_KERNELS
#if USE_OPENCL
#    include "kernels_cl.h"
#endif
#if USE_VULKAN
#    include "kernels_vk.h"
#endif
#endif // RR_EMBED_KERNELS

#define INITIAL_TRIANGLE_CAPACITY 100000

namespace RadeonRays
{
    
    static int kWorkGroupSize = 64;
    
    Hlbvh::Hlbvh(Calc::Device* device)
    : m_device(device)
    , m_gpudata(new GpuData(device))
    {
        InitGpuData();
    }
    
    
    Hlbvh::~Hlbvh()
    {
    }
    
    void Hlbvh::AllocateBuffers(size_t num_prims)
    {
        // * 3 since only triangles are supported just yet
        m_gpudata->positions = m_device->CreateBuffer(num_prims * sizeof(float3), Calc::BufferType::kWrite);
        // * 3 since 3 vertex indices + 1 material idx
        m_gpudata->morton_codes = m_device->CreateBuffer(num_prims * sizeof(int), Calc::BufferType::kWrite);
        
        
        std::vector<int> iota(num_prims);
        std::iota(iota.begin(), iota.end(), 0);
        
        m_gpudata->prim_indices = m_device->CreateBuffer(num_prims * sizeof(int), Calc::BufferType::kWrite, &iota[0]);
        m_gpudata->morton_codes = m_device->CreateBuffer(num_prims * sizeof(int), Calc::BufferType::kWrite);
        m_gpudata->sorted_morton_codes = m_device->CreateBuffer(num_prims * sizeof(int), Calc::BufferType::kWrite);
        m_gpudata->sorted_prim_indices = m_device->CreateBuffer(num_prims * sizeof(int), Calc::BufferType::kWrite);
        
        m_gpudata->nodes = m_device->CreateBuffer(2 * num_prims * sizeof(Node), Calc::BufferType::kWrite);
        // Bounds
        m_gpudata->bounds = m_device->CreateBuffer(num_prims * sizeof(bbox), Calc::BufferType::kWrite);
        m_gpudata->scene_bound = m_device->CreateBuffer(sizeof(bbox), Calc::BufferType::kRead);
        m_gpudata->sorted_bounds = m_device->CreateBuffer(num_prims * sizeof(bbox), Calc::BufferType::kWrite);
        // Propagation flags
        m_gpudata->flags = m_device->CreateBuffer(2 * num_prims * sizeof(int), Calc::BufferType::kWrite);
    }
    
    void Hlbvh::InitGpuData()
    {
        
#ifndef RR_EMBED_KERNELS
        if ( m_device->GetPlatform() == Calc::Platform::kOpenCL )
        {
            m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/src/kernels/CL/build_hlbvh.cl", nullptr, 0, nullptr );
        }

        else
        {
            assert( m_device->GetPlatform() == Calc::Platform::kVulkan );
            m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/src/kernels/GLSL/hlbvh_build.comp", nullptr, 0, nullptr );
        }
#else
        auto& device = m_device;
#if USE_OPENCL
        if (device->GetPlatform() == Calc::Platform::kOpenCL)
        {
            m_gpudata->executable = m_device->CompileExecutable(g_build_hlbvh_opencl, std::strlen(g_build_hlbvh_opencl), nullptr);
        }
#endif

#if USE_VULKAN
        if (m_gpudata->executable == nullptr && device->GetPlatform() == Calc::Platform::kVulkan)
        {
            m_gpudata->executable = m_device->CompileExecutable(g_hlbvh_build_vulkan, std::strlen(g_hlbvh_build_vulkan), nullptr);
        }
#endif

#endif
        m_gpudata->morton_code_func = m_gpudata->executable->CreateFunction("calculate_morton_code_main");
        m_gpudata->build_func = m_gpudata->executable->CreateFunction("emit_hierarchy_main");
        m_gpudata->refit_func = m_gpudata->executable->CreateFunction("refit_bounds_main");

        // Allocate GPU buffers
        AllocateBuffers(INITIAL_TRIANGLE_CAPACITY);
        
        // Initialize parallel primitives
        if (!m_device->HasBuiltinPrimitives())
        {
            throw ExceptionImpl("This device does not support HLBVH construction\n");
        }
        
        m_gpudata->pp = m_device->CreatePrimitives();
    }
    
    // Build function
    void Hlbvh::Build(bbox const* bounds, int numbounds)
    {
//#ifdef _DEBUG
        auto s = std::chrono::high_resolution_clock::now();
//#endif
        BuildImpl(bounds, numbounds);
//#ifdef _DEBUG
        m_device->Finish(0);
        // Note, that this is total time spent for setup and construction 
        // including the time spent waiting in the queue.
        auto d = std::chrono::high_resolution_clock::now() - s;
        std::cout << "HLBVH setup + construction CPU time: " << std::chrono::duration_cast<std::chrono::milliseconds>(d).count() << "ms\n";
//#endif
    }
    
    
    // World space bounding box
    bbox const& Hlbvh::Bounds() const
    {
        // TODO: implement me
        static bbox s_tmp;
        return s_tmp;
    }
    
    // Build function
    void Hlbvh::BuildImpl(bbox const* bounds, int numbounds)
    {
        int size = numbounds;
        
        // Make sure to allocate enough mem on GPU
        // We are trying to reuse space as reallocation takes time
        // but this call might be really frequent
        if (size > m_gpudata->positions->GetSize())
        {
            AllocateBuffers(size);
        }

        // Evaluate scene bouds
        bbox scene_bound = bbox();
        for (auto i = 0; i < numbounds; ++i)
            scene_bound.grow(bounds[i]);

        m_device->WriteBuffer(m_gpudata->scene_bound, 0, 0, sizeof(bbox), &scene_bound, nullptr);

        // Write bounds buffer
        {
            bbox* tmp = nullptr;
            m_device->MapBuffer(m_gpudata->bounds, 0, 0, sizeof(bbox) * numbounds, Calc::kMapWrite, (void**)&tmp, nullptr);
            m_device->Finish(0);
            std::memcpy(tmp, bounds, sizeof(bbox) * numbounds);
            m_device->UnmapBuffer(m_gpudata->bounds, 0, tmp, nullptr);
            m_device->Finish(0);
        }
        
        // Initialize flags with zero 
        {
            int* tmp = nullptr;
            m_device->MapBuffer(m_gpudata->flags, 0, 0, sizeof(int) * 2 * numbounds, Calc::kMapWrite, (void**)&tmp, nullptr);
            m_device->Finish(0);
            std::memset(tmp, 0, sizeof(int) * numbounds * 2);
            m_device->UnmapBuffer(m_gpudata->flags, 0, tmp, nullptr);
            m_device->Finish(0);
        }

        // Calculate Morton codes array
        int arg = 0;
        m_gpudata->morton_code_func->SetArg(arg++, m_gpudata->bounds);
        m_gpudata->morton_code_func->SetArg(arg++, sizeof(size), &size);
        m_gpudata->morton_code_func->SetArg(arg++, m_gpudata->scene_bound);
        m_gpudata->morton_code_func->SetArg(arg++, m_gpudata->morton_codes);

        // Calculate global size
        int globalsize = ((size + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;
        
        // Launch Morton codes kernel
        m_device->Execute(m_gpudata->morton_code_func, 0, globalsize, kWorkGroupSize, nullptr);
        m_device->Finish(0);
        
        // Sort primitives according to their Morton codes
        m_gpudata->pp->SortRadixInt32(0, m_gpudata->morton_codes, m_gpudata->sorted_morton_codes, m_gpudata->prim_indices, m_gpudata->sorted_prim_indices, size);

        std::vector<int> codes(size);
        m_device->ReadBuffer(m_gpudata->sorted_prim_indices, 0, 0, sizeof(int) * size, &codes[0], nullptr);
        m_device->Finish(0);
       
        // Prepare tree construction kernel
        arg = 0;
        m_gpudata->build_func->SetArg(arg++, m_gpudata->sorted_morton_codes);
        m_gpudata->build_func->SetArg(arg++, m_gpudata->bounds);
        m_gpudata->build_func->SetArg(arg++, m_gpudata->sorted_prim_indices);
        m_gpudata->build_func->SetArg(arg++, sizeof(size), &size);
        m_gpudata->build_func->SetArg(arg++, m_gpudata->nodes);
        m_gpudata->build_func->SetArg(arg++, m_gpudata->sorted_bounds);
        
        // Calculate global size
        globalsize = ((size + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;
        
        // Launch hierarchy emission kernel
        m_device->Execute(m_gpudata->build_func, 0, globalsize, kWorkGroupSize, nullptr);
        
        // Refit bounds
        arg = 0;
        m_gpudata->refit_func->SetArg(arg++, m_gpudata->sorted_bounds);
        m_gpudata->refit_func->SetArg(arg++, sizeof(size), &size);
        m_gpudata->refit_func->SetArg(arg++, m_gpudata->nodes);
        m_gpudata->refit_func->SetArg(arg++, m_gpudata->flags);
        
        globalsize = ((size + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;
        
        // Launch refit kernel
        m_device->Execute(m_gpudata->refit_func, 0, globalsize, kWorkGroupSize, nullptr);
    }
}
