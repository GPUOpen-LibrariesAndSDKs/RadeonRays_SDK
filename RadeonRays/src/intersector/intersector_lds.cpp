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
#include "intersector_lds.h"

#include "calc.h"
#include "executable.h"
#include "../accelerator/bvh2.h"
#include "../primitive/mesh.h"
#include "../primitive/instance.h"
#include "../translator/q_bvh_translator.h"
#include "../world/world.h"

namespace RadeonRays
{
    // Preferred work group size for Radeon devices
    static int const kMaxStackSize  = 48;
    static int const kWorkGroupSize = 64;

    struct IntersectorLDS::GpuData
    {
        struct Program
        {
            Program(Calc::Device *device)
                : device(device)
                , executable(nullptr)
                , isect_func(nullptr)
                , occlude_func(nullptr)
            {
            }

            ~Program()
            {
                if (executable)
                {
                    executable->DeleteFunction(isect_func);
                    executable->DeleteFunction(occlude_func);
                    device->DeleteExecutable(executable);
                }
            }

            Calc::Device *device;

            Calc::Executable *executable;
            Calc::Function *isect_func;
            Calc::Function *occlude_func;
        };

        // Device
        Calc::Device *device;
        // BVH nodes
        Calc::Buffer *bvh;
        // Traversal stack
        Calc::Buffer *stack;

        Program *prog;
        Program bvh_prog;
        Program qbvh_prog;

        GpuData(Calc::Device *device)
            : device(device)
            , bvh(nullptr)
            , stack(nullptr)
            , prog(nullptr)
            , bvh_prog(device)
            , qbvh_prog(device)
        {
        }

        ~GpuData()
        {
            device->DeleteBuffer(bvh);
            device->DeleteBuffer(stack);
        }
    };

    IntersectorLDS::IntersectorLDS(Calc::Device *device)
        : Intersector(device)
        , m_gpudata(new GpuData(device))
    {
        std::string buildopts;
#ifdef RR_RAY_MASK
        buildopts.append("-D RR_RAY_MASK ");
#endif
#ifdef USE_SAFE_MATH
        buildopts.append("-D USE_SAFE_MATH ");
#endif

        Calc::DeviceSpec spec;
        m_device->GetSpec(spec);

#ifndef RR_EMBED_KERNELS
        if (device->GetPlatform() == Calc::Platform::kOpenCL)
        {
            const char *headers[] = { "../RadeonRays/src/kernels/CL/common.cl" };

            int numheaders = sizeof(headers) / sizeof(const char *);

            m_gpudata->bvh_prog.executable = m_device->CompileExecutable("../RadeonRays/src/kernels/CL/intersect_bvh2_lds.cl", headers, numheaders, buildopts.c_str());
            if (spec.has_fp16)
                m_gpudata->qbvh_prog.executable = m_device->CompileExecutable("../RadeonRays/src/kernels/CL/intersect_bvh2_lds_fp16.cl", headers, numheaders, buildopts.c_str());
        }
        else
        {
            assert(device->GetPlatform() == Calc::Platform::kVulkan);
            m_gpudata->bvh_prog.executable = m_device->CompileExecutable("../RadeonRays/src/kernels/GLSL/bvh2.comp", nullptr, 0, buildopts.c_str());
            if (spec.has_fp16)
                m_gpudata->qbvh_prog.executable = m_device->CompileExecutable("../RadeonRays/src/kernels/GLSL/bvh2_fp16.comp", nullptr, 0, buildopts.c_str());
        }
#else
#if USE_OPENCL
        if (device->GetPlatform() == Calc::Platform::kOpenCL)
        {
            m_gpudata->bvh_prog.executable = m_device->CompileExecutable(g_intersect_bvh2_lds_opencl, std::strlen(g_intersect_bvh2_lds_opencl), buildopts.c_str());
            if (spec.has_fp16)
                m_gpudata->qbvh_prog.executable = m_device->CompileExecutable(g_intersect_bvh2_lds_fp16_opencl, std::strlen(g_intersect_bvh2_lds_fp16_opencl), buildopts.c_str());
        }
#endif
#if USE_VULKAN
        if (device->GetPlatform() == Calc::Platform::kVulkan)
        {
            if (m_gpudata->bvh_prog.executable == nullptr)
                m_gpudata->bvh_prog.executable = m_device->CompileExecutable(g_bvh2_vulkan, std::strlen(g_bvh2_vulkan), buildopts.c_str());
            if (m_gpudata->qbvh_prog.executable == nullptr && spec.has_fp16)
                m_gpudata->qbvh_prog.executable = m_device->CompileExecutable(g_bvh2_fp16_vulkan, std::strlen(g_bvh2_fp16_vulkan), buildopts.c_str());
        }
#endif
#endif

        m_gpudata->bvh_prog.isect_func = m_gpudata->bvh_prog.executable->CreateFunction("intersect_main");
        m_gpudata->bvh_prog.occlude_func = m_gpudata->bvh_prog.executable->CreateFunction("occluded_main");

        if (m_gpudata->qbvh_prog.executable)
        {
            m_gpudata->qbvh_prog.isect_func = m_gpudata->qbvh_prog.executable->CreateFunction("intersect_main");
            m_gpudata->qbvh_prog.occlude_func = m_gpudata->qbvh_prog.executable->CreateFunction("occluded_main");
        }
    }

    void IntersectorLDS::Process(const World &world)
    {
        // If something has been changed we need to rebuild BVH
        if (!m_gpudata->bvh || world.has_changed() || world.GetStateChange() != ShapeImpl::kStateChangeNone)
        {
            // Free previous data
            if (m_gpudata->bvh)
            {
                m_device->DeleteBuffer(m_gpudata->bvh);
            }

            // Look up build options for world
            auto type = world.options_.GetOption("bvh.type");
            auto builder = world.options_.GetOption("bvh.builder");
            auto nbins = world.options_.GetOption("bvh.sah.num_bins");
            auto tcost = world.options_.GetOption("bvh.sah.traversal_cost");

            bool use_qbvh = false, use_sah = false;
            int num_bins = (nbins ? static_cast<int>(nbins->AsFloat()) : 64);
            float traversal_cost = (tcost ? tcost->AsFloat() : 10.0f);

#if 0
            if (type && type->AsString() == "qbvh")
            {
                use_qbvh = (m_gpudata->qbvh_prog.executable != nullptr);
            }
#endif

            if (builder && builder->AsString() == "sah")
            {
                use_sah = true;
            }

            // Create the bvh
            Bvh2 bvh(traversal_cost, num_bins, use_sah);
            bvh.Build(world.shapes_.begin(), world.shapes_.end());

            // Upload BVH data to GPU memory
            if (!use_qbvh)
            {
                auto bvh_size_in_bytes = bvh.GetSizeInBytes();
                m_gpudata->bvh = m_device->CreateBuffer(bvh_size_in_bytes, Calc::BufferType::kRead);

                // Get the pointer to mapped data
                Calc::Event *e = nullptr;
                Bvh2::Node *bvhdata = nullptr;

                m_device->MapBuffer(m_gpudata->bvh, 0, 0, bvh_size_in_bytes, Calc::MapType::kMapWrite, (void **)&bvhdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Copy BVH data
                for (std::size_t i = 0; i < bvh.m_nodecount; ++i)
                    bvhdata[i] = bvh.m_nodes[i];

                // Unmap gpu data
                m_device->UnmapBuffer(m_gpudata->bvh, 0, bvhdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Select intersection program
                m_gpudata->prog = &m_gpudata->bvh_prog;
            }
            else
            {
                QBvhTranslator translator;
                translator.Process(bvh);

                // Update GPU data
                auto bvh_size_in_bytes = translator.GetSizeInBytes();
                m_gpudata->bvh = m_device->CreateBuffer(bvh_size_in_bytes, Calc::BufferType::kRead);

                // Get the pointer to mapped data
                Calc::Event *e = nullptr;
                QBvhTranslator::Node *bvhdata = nullptr;

                m_device->MapBuffer(m_gpudata->bvh, 0, 0, bvh_size_in_bytes, Calc::MapType::kMapWrite, (void **)&bvhdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Copy BVH data
                std::size_t i = 0;
                for (auto it = translator.nodes_.begin(); it != translator.nodes_.end(); ++it)
                    bvhdata[i++] = *it;

                // Unmap gpu data
                m_device->UnmapBuffer(m_gpudata->bvh, 0, bvhdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Select intersection program
                m_gpudata->prog = &m_gpudata->qbvh_prog;
            }

            // Make sure everything is committed
            m_device->Finish(0);
        }
    }

    void IntersectorLDS::Intersect(std::uint32_t queue_idx, const Calc::Buffer *rays, const Calc::Buffer *num_rays,
        std::uint32_t max_rays, Calc::Buffer *hits,
        const Calc::Event *wait_event, Calc::Event **event) const
    {
        std::size_t stack_size = 4 * max_rays * kMaxStackSize;

        // Check if we need to reallocate memory
        if (!m_gpudata->stack || stack_size > m_gpudata->stack->GetSize())
        {
            m_device->DeleteBuffer(m_gpudata->stack);
            m_gpudata->stack = m_device->CreateBuffer(stack_size, Calc::BufferType::kWrite);
        }

        assert(m_gpudata->prog);
        auto &func = m_gpudata->prog->isect_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpudata->bvh);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, num_rays);
        func->SetArg(arg++, m_gpudata->stack);
        func->SetArg(arg++, hits);

        std::size_t localsize = kWorkGroupSize;
        std::size_t globalsize = ((max_rays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queue_idx, globalsize, localsize, event);
    }

    void IntersectorLDS::Occluded(std::uint32_t queue_idx, const Calc::Buffer *rays, const Calc::Buffer *num_rays,
        std::uint32_t max_rays, Calc::Buffer *hits,
        const Calc::Event *wait_event, Calc::Event **event) const
    {
        std::size_t stack_size = 4 * max_rays * kMaxStackSize;

        // Check if we need to reallocate memory
        if (!m_gpudata->stack || stack_size > m_gpudata->stack->GetSize())
        {
            m_device->DeleteBuffer(m_gpudata->stack);
            m_gpudata->stack = m_device->CreateBuffer(stack_size, Calc::BufferType::kWrite);
        }

        assert(m_gpudata->prog);
        auto &func = m_gpudata->prog->occlude_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpudata->bvh);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, num_rays);
        func->SetArg(arg++, m_gpudata->stack);
        func->SetArg(arg++, hits);

        std::size_t localsize = kWorkGroupSize;
        std::size_t globalsize = ((max_rays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queue_idx, globalsize, localsize, event);
    }
}
