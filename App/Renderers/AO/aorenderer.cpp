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
#include "Renderers/AO/aorenderer.h"
#include "CLW/clwoutput.h"
#include "SceneGraph/scene1.h"

#include <numeric>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstdint>

//#include "sobol.h"
extern unsigned g_SobolMatrices[];

#ifdef RR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

namespace Baikal
{
    using namespace RadeonRays;

    static int const kMaxLightSamples = 1;

    struct AoRenderer::QmcSampler
    {
        std::uint32_t seq;
        std::uint32_t s0;
        std::uint32_t s1;
        std::uint32_t s2;
    };

    struct AoRenderer::RenderData
    {
        // OpenCL stuff
        CLWBuffer<ray> rays;
        CLWBuffer<int> hits;

        CLWBuffer<ray> shadowrays;
        CLWBuffer<int> shadowhits;

        CLWBuffer<Intersection> intersections;
        CLWBuffer<int> compacted_indices;
        CLWBuffer<int> pixelindices[2];
        CLWBuffer<int> iota;

        CLWBuffer<QmcSampler> samplers;
        CLWBuffer<unsigned int> sobolmat;
        CLWBuffer<int> hitcount;

        CLWProgram program;
        CLWParallelPrimitives pp;

        // RadeonRays stuff
        Buffer* fr_rays;
        Buffer* fr_shadowrays;
        Buffer* fr_shadowhits;
        Buffer* fr_hits;
        Buffer* fr_intersections;
        Buffer* fr_hitcount;

        RenderData()
            : fr_shadowrays(nullptr)
            , fr_shadowhits(nullptr)
            , fr_hits(nullptr)
            , fr_intersections(nullptr)
            , fr_hitcount(nullptr)
        {
            fr_rays = nullptr;
        }
    };

    // Constructor
    AoRenderer::AoRenderer(CLWContext context, int devidx)
        : m_context(context)
        , m_output(nullptr)
        , m_render_data(new RenderData)
        , m_vidmemws(0)
        , m_resetsampler(true)
        , m_scene_controller(context, devidx)
    {

        // Create parallel primitives
        m_render_data->pp = CLWParallelPrimitives(m_context);

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
#ifndef RR_EMBED_KERNELS
        m_render_data->program = CLWProgram::CreateFromFile("../App/CL/integrator_ao.cl", buildopts.c_str(), m_context);
#else
        m_render_data->program = CLWProgram::CreateFromSource(cl_app, std::strlen(cl_integrator_ao), buildopts.c_str(), context);
#endif

        m_render_data->sobolmat = m_context.CreateBuffer<unsigned int>(1024 * 52, CL_MEM_READ_ONLY, &g_SobolMatrices[0]);
    }

    AoRenderer::~AoRenderer()
    {
    }

    Output* AoRenderer::CreateOutput(std::uint32_t w, std::uint32_t h) const
    {
        return new ClwOutput(w, h, m_context);
    }

    void AoRenderer::DeleteOutput(Output* output) const
    {
        delete output;
    }

    void AoRenderer::Clear(RadeonRays::float3 const& val, Output& output) const
    {
        static_cast<ClwOutput&>(output).Clear(val);
        m_resetsampler = true;
    }

    void AoRenderer::Preprocess(Scene1 const& scene)
    {
    }

    // Render the scene into the output
    void AoRenderer::Render(Scene1 const& scene)
    {
        auto api = m_scene_controller.GetIntersectionApi();
        
        Collector mat_collector;
        Collector tex_collector;
        auto& clwscene = m_scene_controller.CompileScene(scene, mat_collector, tex_collector);

        // Check output
        assert(m_output);

        // Number of rays to generate
        int maxrays = m_output->width() * m_output->height();

        // Generate primary
        GeneratePrimaryRays(clwscene);

        // Copy compacted indices to track reverse indices
        m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[0], 0, 0, m_render_data->iota.GetElementCount());
        m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[1], 0, 0, m_render_data->iota.GetElementCount());
        m_context.FillBuffer(0, m_render_data->hitcount, maxrays, 1);

        {
            // Clear ray hits buffer
            m_context.FillBuffer(0, m_render_data->hits, 0, m_render_data->hits.GetElementCount());

            // Intersect ray batch
            //Event* e = nullptr;
            api->QueryIntersection(m_render_data->fr_rays, m_render_data->fr_hitcount, maxrays, m_render_data->fr_intersections, nullptr, nullptr);
            //e->Wait();
            //m_api->DeleteEvent(e);

            // Convert intersections to predicates
            FilterPathStream();

            // Compact batch
            m_render_data->pp.Compact(0, m_render_data->hits, m_render_data->iota, m_render_data->compacted_indices, maxrays, m_render_data->hitcount);

            /*int cnt = 0;
            m_context.ReadBuffer(0, m_render_data->hitcount[0], &cnt, 1).Wait();
            std::cout << "Pass " << pass << " Alive " << cnt << "\n";*/

            // Advance indices to keep pixel indices up to date
            RestorePixelIndices();

            // Shade hits
            SampleOcclusion(clwscene);

            // Intersect shadow rays
            api->QueryOcclusion(m_render_data->fr_shadowrays, m_render_data->fr_hitcount, maxrays, m_render_data->fr_shadowhits, nullptr, nullptr);
            //e->Wait();
            //m_api->DeleteEvent(e);

            // Gather light samples and account for visibility
            GatherOcclusion(clwscene);

            //
            m_context.Flush(0);
        }
    }

    void AoRenderer::SetOutput(Output* output)
    {
        if (!m_output || m_output->width() < output->width() || m_output->height() < output->height())
        {
            ResizeWorkingSet(*output);
        }

        m_output = static_cast<ClwOutput*>(output);
    }

    void AoRenderer::ResizeWorkingSet(Output const& output)
    {
        m_vidmemws = 0;

        // Create ray payloads
        m_render_data->rays = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray);

        m_render_data->hits = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);

        m_render_data->intersections = m_context.CreateBuffer<Intersection>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(Intersection);

        m_render_data->shadowrays = m_context.CreateBuffer<ray>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray)* kMaxLightSamples;

        m_render_data->shadowhits = m_context.CreateBuffer<int>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int)* kMaxLightSamples;

        m_render_data->samplers = m_context.CreateBuffer<QmcSampler>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(QmcSampler);

        std::vector<int> initdata(output.width() * output.height());
        std::iota(initdata.begin(), initdata.end(), 0);
        m_render_data->iota = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &initdata[0]);
        m_vidmemws += output.width() * output.height() * sizeof(int);

        m_render_data->compacted_indices = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);

        m_render_data->pixelindices[0] = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);

        m_render_data->pixelindices[1] = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);

        m_render_data->hitcount = m_context.CreateBuffer<int>(1, CL_MEM_READ_WRITE);

        auto api = m_scene_controller.GetIntersectionApi();

        // Recreate FR buffers
        api->DeleteBuffer(m_render_data->fr_rays);
        api->DeleteBuffer(m_render_data->fr_shadowrays);
        api->DeleteBuffer(m_render_data->fr_hits);
        api->DeleteBuffer(m_render_data->fr_shadowhits);
        api->DeleteBuffer(m_render_data->fr_intersections);
        api->DeleteBuffer(m_render_data->fr_hitcount);

        m_render_data->fr_rays = CreateFromOpenClBuffer(api, m_render_data->rays);
        m_render_data->fr_shadowrays = CreateFromOpenClBuffer(api, m_render_data->shadowrays);
        m_render_data->fr_hits = CreateFromOpenClBuffer(api, m_render_data->hits);
        m_render_data->fr_shadowhits = CreateFromOpenClBuffer(api, m_render_data->shadowhits);
        m_render_data->fr_intersections = CreateFromOpenClBuffer(api, m_render_data->intersections);
        m_render_data->fr_hitcount = CreateFromOpenClBuffer(api, m_render_data->hitcount);

        std::cout << "Vidmem usage (working set): " << m_vidmemws / (1024 * 1024) << "Mb\n";
    }

    void AoRenderer::GeneratePrimaryRays(ClwScene const& scene)
    {
        // Fetch kernel
        CLWKernel genkernel = m_render_data->program.GetKernel("PerspectiveCamera_GeneratePaths");

        // Set kernel parameters
        genkernel.SetArg(0, scene.camera);
        genkernel.SetArg(1, m_output->width());
        genkernel.SetArg(2, m_output->height());
        genkernel.SetArg(3, (int)rand_uint());
        genkernel.SetArg(4, m_render_data->rays);
        genkernel.SetArg(5, m_render_data->samplers);
        genkernel.SetArg(6, m_render_data->sobolmat);
        genkernel.SetArg(7, m_resetsampler);
        m_resetsampler = 0;

        // Run generation kernel
        {
            size_t gs[] = { static_cast<size_t>((m_output->width() + 7) / 8 * 8), static_cast<size_t>((m_output->height() + 7) / 8 * 8) };
            size_t ls[] = { 8, 8 };

            m_context.Launch2D(0, gs, ls, genkernel);
        }
    }

    void AoRenderer::SampleOcclusion(ClwScene const& scene)
    {
        // Fetch kernel
        CLWKernel samplekernel = m_render_data->program.GetKernel("SampleOcclusion");

        // Set kernel parameters
        int argc = 0;
        samplekernel.SetArg(argc++, m_render_data->rays);
        samplekernel.SetArg(argc++, m_render_data->intersections);
        samplekernel.SetArg(argc++, m_render_data->compacted_indices);
        samplekernel.SetArg(argc++, m_render_data->pixelindices[0]);
        samplekernel.SetArg(argc++, m_render_data->hitcount);
        samplekernel.SetArg(argc++, scene.vertices);
        samplekernel.SetArg(argc++, scene.normals);
        samplekernel.SetArg(argc++, scene.uvs);
        samplekernel.SetArg(argc++, scene.indices);
        samplekernel.SetArg(argc++, scene.shapes);
        samplekernel.SetArg(argc++, rand_uint());
        samplekernel.SetArg(argc++, 1000000.f);
        samplekernel.SetArg(argc++, m_render_data->samplers);
        samplekernel.SetArg(argc++, m_render_data->sobolmat);
        samplekernel.SetArg(argc++, m_render_data->shadowrays);

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, samplekernel);
        }
    }

    void AoRenderer::GatherOcclusion(ClwScene const& scene)
    {
        // Fetch kernel
        CLWKernel gatherkernel = m_render_data->program.GetKernel("GatherOcclusion");

        // Set kernel parameters
        int argc = 0;
        gatherkernel.SetArg(argc++, m_render_data->pixelindices[0]);
        gatherkernel.SetArg(argc++, m_render_data->hitcount);
        gatherkernel.SetArg(argc++, m_render_data->shadowhits);
        gatherkernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gatherkernel);
        }
    }

    void AoRenderer::RestorePixelIndices()
    {
        // Fetch kernel
        CLWKernel restorekernel = m_render_data->program.GetKernel("RestorePixelIndices");

        // Set kernel parameters
        int argc = 0;
        restorekernel.SetArg(argc++, m_render_data->compacted_indices);
        restorekernel.SetArg(argc++, m_render_data->hitcount);
        restorekernel.SetArg(argc++, m_render_data->pixelindices[1]);
        restorekernel.SetArg(argc++, m_render_data->pixelindices[0]);

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, restorekernel);
        }
    }

    void AoRenderer::FilterPathStream()
    {
        // Fetch kernel
        CLWKernel restorekernel = m_render_data->program.GetKernel("FilterPathStream");

        // Set kernel parameters
        int argc = 0;
        restorekernel.SetArg(argc++, m_render_data->intersections);
        restorekernel.SetArg(argc++, m_render_data->hitcount);
        restorekernel.SetArg(argc++, m_render_data->pixelindices[1]);
        restorekernel.SetArg(argc++, m_render_data->hits);

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, restorekernel);
        }

        //int cnt = 0;
        //m_context.ReadBuffer(0, m_render_data->debug, &cnt, 1).Wait();
        //std::cout << "Pass " << pass << " killed " << cnt << "\n";
    }

    CLWKernel AoRenderer::GetCopyKernel()
    {
        return m_render_data->program.GetKernel("ApplyGammaAndCopyData");
    }

    CLWKernel AoRenderer::GetAccumulateKernel()
    {
        return m_render_data->program.GetKernel("AccumulateData");
    }
}
