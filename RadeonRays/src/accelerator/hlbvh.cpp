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




#ifdef FR_EMBED_KERNELS
#include "../kernel/CL/cache/kernels.h"
#endif

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
    
    void Hlbvh::AllocateBuffers(size_t numprims)
    {
        // * 3 since only triangles are supported just yet
        m_gpudata->positions = m_device->CreateBuffer(numprims * sizeof(float3), Calc::BufferType::kWrite);
        // * 3 since 3 vertex indices + 1 material idx
		m_gpudata->morton_codes = m_device->CreateBuffer(numprims * sizeof(int), Calc::BufferType::kWrite);
        
        
        std::vector<int> iota(numprims);
        std::iota(iota.begin(), iota.end(), 0);
        
        m_gpudata->prim_indices = m_device->CreateBuffer(numprims * sizeof(int), Calc::BufferType::kWrite, &iota[0]);
		m_gpudata->morton_codes = m_device->CreateBuffer(numprims * sizeof(int), Calc::BufferType::kWrite);
		m_gpudata->sorted_morton_codes = m_device->CreateBuffer(numprims * sizeof(int), Calc::BufferType::kWrite);
		m_gpudata->sorted_prim_indices = m_device->CreateBuffer(numprims * sizeof(int), Calc::BufferType::kWrite);
        
        m_gpudata->nodes = m_device->CreateBuffer(2 * numprims * sizeof(Node), Calc::BufferType::kWrite);
        // Bounds
		m_gpudata->bounds = m_device->CreateBuffer(numprims * sizeof(bbox), Calc::BufferType::kWrite);
		m_gpudata->sorted_bounds = m_device->CreateBuffer(numprims * sizeof(bbox), Calc::BufferType::kWrite);
        // Propagation flags
		m_gpudata->flags = m_device->CreateBuffer(2 * numprims * sizeof(int), Calc::BufferType::kWrite);
    }
    
    void Hlbvh::InitGpuData()
    {
#ifndef FR_EMBED_KERNELS
		if ( m_device->GetPlatform() == Calc::Platform::kOpenCL )
		{
			m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/kernel/CL/hlbvh_build.cl", nullptr, 0 );
		}

		else
		{
			assert( m_device->GetPlatform() == Calc::Platform::kVulkan );
			m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/kernel/GLSL/hlbvh_build", nullptr, 0 );
		}
#else
		m_gpudata->executable = m_device->CompileExecutable(cl_hlbvh_build, std::strlen(cl_hlbvh_build), "");
#endif
		m_gpudata->morton_code_func = m_gpudata->executable->CreateFunction("CalcMortonCode");
		m_gpudata->build_func = m_gpudata->executable->CreateFunction("BuildHierarchy");
		m_gpudata->refit_func = m_gpudata->executable->CreateFunction("RefitBounds");

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
#ifdef _DEBUG
        auto s = std::chrono::high_resolution_clock::now();
#endif
        BuildImpl(bounds, numbounds);
#ifdef _DEBUG
        m_device->Finish(0);
		// Note, that this is total time spent for setup and construction 
		// including the time spent waiting in the queue.
        auto d = std::chrono::high_resolution_clock::now() - s;
        std::cout << "HLBVH setup + construction CPU time: " << std::chrono::duration_cast<std::chrono::milliseconds>(d).count() << "ms\n";
#endif
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
        

		// Write bounds buffer
		{
			bbox* tmp = nullptr;
			Calc::Event* e = nullptr;
			m_device->MapBuffer(m_gpudata->bounds, 0, 0, sizeof(bbox) * numbounds, Calc::kMapWrite, (void**)&tmp, &e);
			e->Wait();
			m_device->DeleteEvent(e);


			std::memcpy(tmp, bounds, sizeof(bbox) * numbounds);
			m_device->UnmapBuffer(m_gpudata->bounds, 0, tmp, &e);
			e->Wait();
			m_device->DeleteEvent(e);
		}
		
		// Initialize flags with zero 
		{
			int* tmp = nullptr;
			Calc::Event* e = nullptr;
			m_device->MapBuffer(m_gpudata->flags, 0, 0, sizeof(int) * 2 * numbounds, Calc::kMapWrite, (void**)&tmp, &e);
			e->Wait();
			m_device->DeleteEvent(e);

			std::memset(tmp, 0, sizeof(int) * numbounds * 2);

			m_device->UnmapBuffer(m_gpudata->flags, 0, tmp, &e);

			e->Wait();
			m_device->DeleteEvent(e);
		}
        
        // Calculate Morton codes array
        int arg = 0;
        m_gpudata->morton_code_func->SetArg(arg++, m_gpudata->bounds);
		m_gpudata->morton_code_func->SetArg(arg++, sizeof(size), &size);
		m_gpudata->morton_code_func->SetArg(arg++, m_gpudata->morton_codes);
        
        // Calculate global size
        int globalsize = ((size + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;
        
        // Launch Morton codes kernel
        m_device->Execute(m_gpudata->morton_code_func, 0, globalsize, kWorkGroupSize, nullptr);
        
        // Sort primitives according to their Morton codes
        m_gpudata->pp->SortRadixInt32(0, m_gpudata->morton_codes, m_gpudata->sorted_morton_codes, m_gpudata->prim_indices, m_gpudata->sorted_prim_indices, size);
       
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

        

		/// DEBUG CODE
        /*std::vector<Node> nodes(size * 2 - 1);
        context_.ReadBuffer(0, gpudata_->nodes_, &nodes[0], 0, size * 2 - 1).Wait();
        
        std::vector<bbox> bnds(size * 2 - 1);
        context_.ReadBuffer(0, gpudata_->sortedbounds_, &bnds[0], 0, size * 2 - 1).Wait();
        std::vector<int> flg(size * 2 - 1);
        context_.ReadBuffer(0, gpudata_->flags_, &flg[0], 0, size * 2 - 1).Wait();*/
        //primindices_.resize(size);
        //context_.ReadBuffer(0, gpudata_->sorted_primindices_, &primindices_[0], 0, size).Wait();
    }
}
