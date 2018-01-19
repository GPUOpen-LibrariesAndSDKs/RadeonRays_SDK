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
        // Device
        Calc::Device *device;
        // BVH nodes
        Calc::Buffer *bvh;
        // Traversal stack
        Calc::Buffer *stack;

        Calc::Executable *executable;
        Calc::Function *isect_func;
        Calc::Function *occlude_func;

        GpuData(Calc::Device *device)
            : device(device)
            , bvh(nullptr)
            , stack(nullptr)
            , executable(nullptr)
            , isect_func(nullptr)
            , occlude_func(nullptr)
        {
        }

        ~GpuData()
        {
            device->DeleteBuffer(bvh);
            device->DeleteBuffer(stack);
            executable->DeleteFunction(isect_func);
            executable->DeleteFunction(occlude_func);
            device->DeleteExecutable(executable);
        }
    };

    IntersectorLDS::IntersectorLDS(Calc::Device *device)
        : Intersector(device)
        , m_gpuData(new GpuData(device))
        , m_bvh(nullptr)
    {
        std::string buildopts;
#ifdef RR_RAY_MASK
        buildopts.append("-D RR_RAY_MASK ");    // TODO: what is this for?!? (gboisse)
#endif
#ifdef USE_SAFE_MATH
        buildopts.append("-D USE_SAFE_MATH ");
#endif

#ifndef RR_EMBED_KERNELS
        // TODO: add implementation (gboisse)
#else
#if USE_OPENCL
        if (device->GetPlatform() == Calc::Platform::kOpenCL)
        {
            m_gpuData->executable = m_device->CompileExecutable(g_intersect_bvh2_lds_opencl, std::strlen(g_intersect_bvh2_lds_opencl), buildopts.c_str());
        }
#endif
#if USE_VULKAN
        if (m_gpudata->executable == nullptr && device->GetPlatform() == Calc::Platform::kVulkan)
        {
            // TODO: implement (gboisse)
            m_gpuData->executable = m_device->CompileExecutable(g_bvh2_vulkan, std::strlen(g_bvh2_vulkan), buildopts.c_str());
        }
#endif
#endif

        m_gpuData->isect_func = m_gpuData->executable->CreateFunction("intersect_main");
        m_gpuData->occlude_func = m_gpuData->executable->CreateFunction("occluded_main");
    }

    void IntersectorLDS::Process(const World &world)
    {
        // If something has been changed we need to rebuild BVH
        if (!m_bvh || world.has_changed() || world.GetStateChange() != ShapeImpl::kStateChangeNone)
        {
            // Free previous data
            if (m_bvh)
            {
                m_device->DeleteBuffer(m_gpuData->bvh);
            }

            // Look up build options for world
            auto builder = world.options_.GetOption("bvh.builder");
            auto nbins = world.options_.GetOption("bvh.sah.num_bins");
            auto tcost = world.options_.GetOption("bvh.sah.traversal_cost");

            bool use_sah = false;
            int num_bins = (nbins ? static_cast<int>(nbins->AsFloat()) : 64);
            float traversal_cost = (tcost ? tcost->AsFloat() : 10.0f);

            if (builder && builder->AsString() == "sah")
            {
                use_sah = true;
            }

            // Create the bvh
            m_bvh.reset(new Bvh2(traversal_cost, num_bins, use_sah));

            // Partition the array into meshes and instances
            std::vector<const Shape *> shapes(world.shapes_);

            auto firstinst = std::partition(shapes.begin(), shapes.end(),
                [&](Shape const* shape)
                {
                    return !static_cast<ShapeImpl const*>(shape)->is_instance();
                });

            // TODO: deal with the instance stuff (gboisse)
            m_bvh->Build(shapes.begin(), firstinst);

            // TODO: what if we don't want to use fp16? (gboisse)
            QBvhTranslator translator;
            translator.Process(*m_bvh);

            // Update GPU data
            auto bvh_size_in_bytes = translator.GetSizeInBytes();
            m_gpuData->bvh = m_device->CreateBuffer(bvh_size_in_bytes, Calc::BufferType::kRead);

            // Get the pointer to mapped data
            Calc::Event *e = nullptr;
            QBvhTranslator::Node *bvhdata = nullptr;

            m_device->MapBuffer(m_gpuData->bvh, 0, 0, bvh_size_in_bytes, Calc::MapType::kMapWrite, (void **)&bvhdata, &e);

            e->Wait();
            m_device->DeleteEvent(e);

            // Copy BVH data
            std::size_t i = 0;
            for (auto it = translator.nodes_.begin(); it != translator.nodes_.end(); ++it)
                bvhdata[i++] = *it;

            // Unmap gpu data
            m_device->UnmapBuffer(m_gpuData->bvh, 0, bvhdata, &e);

            e->Wait();
            m_device->DeleteEvent(e);

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
        if (!m_gpuData->stack || stack_size > m_gpuData->stack->GetSize())
        {
            m_device->DeleteBuffer(m_gpuData->stack);
            m_gpuData->stack = m_device->CreateBuffer(stack_size, Calc::BufferType::kWrite);
        }

        auto &func = m_gpuData->isect_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpuData->bvh);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, num_rays);
        func->SetArg(arg++, m_gpuData->stack);
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
        if (!m_gpuData->stack || stack_size > m_gpuData->stack->GetSize())
        {
            m_device->DeleteBuffer(m_gpuData->stack);
            m_gpuData->stack = m_device->CreateBuffer(stack_size, Calc::BufferType::kWrite);
        }

        auto &func = m_gpuData->occlude_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpuData->bvh);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, num_rays);
        func->SetArg(arg++, m_gpuData->stack);
        func->SetArg(arg++, hits);

        std::size_t localsize = kWorkGroupSize;
        std::size_t globalsize = ((max_rays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queue_idx, globalsize, localsize, event);
    }
}