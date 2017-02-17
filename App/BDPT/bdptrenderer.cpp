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
#include "BDPT/bdptrenderer.h"
#include "CLW/clwoutput.h"
#include "Scene/scene1.h"
#include "Scene/Collector/collector.h"

#include <numeric>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <random>
#include <algorithm>

#include "sobol.h"

#ifdef RR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

namespace Baikal
{
    using namespace RadeonRays;

    static int const kMaxLightSamples = 1;
    static int const kMaxRandomWalkLength = 5;

    struct BdptRenderer::PathState
    {
        float4 throughput;
        int volume;
        int flags;
        int extra0;
        int extra1;
    };

    struct BdptRenderer::PathVertex
    {
        float3 p;
        float3 n;
        float3 ng;
        float2 uv;
        float pdf_fwd;
        float pdf_bwd;
        float3 flow;
        int type;
        int matidx;
        int flags;
        int padding;
    };

    struct BdptRenderer::RenderData
    {
        // OpenCL stuff
        CLWBuffer<ray> rays[2];
        CLWBuffer<int> hits;

        CLWBuffer<ray> shadowrays;
        CLWBuffer<int> shadowhits;

        CLWBuffer<Intersection> intersections;
        CLWBuffer<int> compacted_indices;
        CLWBuffer<int> pixelindices[2];
        CLWBuffer<int> iota;

        CLWBuffer<float3> lightsamples;
        CLWBuffer<PathState> paths;
        CLWBuffer<std::uint32_t> random;
        CLWBuffer<std::uint32_t> sobolmat;
        CLWBuffer<int> hitcount;

        CLWBuffer<PathVertex> camera_subpath;
        CLWBuffer<PathVertex> light_subpath;
        CLWBuffer<int> camera_subpath_len;
        CLWBuffer<int> light_subpath_len;

        CLWProgram program;
        CLWParallelPrimitives pp;

        // RadeonRays stuff
        Buffer* fr_rays[2];
        Buffer* fr_shadowrays;
        Buffer* fr_shadowhits;
        Buffer* fr_hits;
        Buffer* fr_intersections;
        Buffer* fr_hitcount;

        Collector mat_collector;
        Collector tex_collector;

        RenderData()
            : fr_shadowrays(nullptr)
            , fr_shadowhits(nullptr)
            , fr_hits(nullptr)
            , fr_intersections(nullptr)
            , fr_hitcount(nullptr)
        {
            fr_rays[0] = nullptr;
            fr_rays[1] = nullptr;
        }
    };

    // Constructor
    BdptRenderer::BdptRenderer(CLWContext context, int devidx, int num_bounces)
        : m_context(context)
        , m_output(nullptr)
        , m_render_data(new RenderData)
        , m_vidmemws(0)
        , m_scene_tracker(context, devidx)
        , m_num_bounces(num_bounces)
        , m_framecnt(0)
    {
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

        // Create parallel primitives
        m_render_data->pp = CLWParallelPrimitives(m_context, buildopts.c_str());

        // Load kernels
#ifndef RR_EMBED_KERNELS
        m_render_data->program = CLWProgram::CreateFromFile("../App/CL/integrator_bdpt.cl", buildopts.c_str(), m_context);
#else
        m_render_data->program = CLWProgram::CreateFromSource(cl_app, std::strlen(cl_integrator_pt), buildopts.c_str(), context);
#endif

        m_render_data->sobolmat = m_context.CreateBuffer<unsigned int>(1024 * 52, CL_MEM_READ_ONLY, &g_SobolMatrices[0]);
    }

    BdptRenderer::~BdptRenderer()
    {
    }

    Output* BdptRenderer::CreateOutput(std::uint32_t w, std::uint32_t h) const
    {
        return new ClwOutput(w, h, m_context);
    }

    void BdptRenderer::DeleteOutput(Output* output) const
    {
        delete output;
    }

    void BdptRenderer::Clear(RadeonRays::float3 const& val, Output& output) const
    {
        static_cast<ClwOutput&>(output).Clear(val);
        m_framecnt = 0;
    }

    void BdptRenderer::SetNumBounces(int num_bounces)
    {
        m_num_bounces = num_bounces;
    }

    void BdptRenderer::Preprocess(Scene1 const& scene)
    {
    }

    void BdptRenderer::RandomWalk(ClwScene const& scene, int num_rays,
        CLWBuffer<PathVertex> vertices, CLWBuffer<int> counts)
    {
        auto api = m_scene_tracker.GetIntersectionApi();

        // Copy compacted indices to track reverse indices
        m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[0], 0, 0, m_render_data->iota.GetElementCount());
        m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[1], 0, 0, m_render_data->iota.GetElementCount());
        m_context.FillBuffer(0, m_render_data->hitcount, num_rays, 1);

        // Initialize first pass
        for (int pass = 0; pass < m_num_bounces; ++pass)
        {
            // Clear ray hits buffer
            m_context.FillBuffer(0, m_render_data->hits, 0, m_render_data->hits.GetElementCount());

            // Intersect ray batch
            //Event* e = nullptr;
            api->QueryIntersection(m_render_data->fr_rays[pass & 0x1], m_render_data->fr_hitcount, num_rays, m_render_data->fr_intersections, nullptr, nullptr);
            //e->Wait();
            //m_api->DeleteEvent(e);

            // Convert intersections to predicates
            FilterPathStream(pass);

            // Compact batch
            m_render_data->pp.Compact(0, m_render_data->hits, m_render_data->iota, m_render_data->compacted_indices, m_render_data->hitcount);

            // Advance indices to keep pixel indices up to date
            RestorePixelIndices(pass);

            // Shade hits
            SampleSurface(scene, pass, vertices, counts);

            //
            m_context.Flush(0);
        }

    }

    void BdptRenderer::GenerateLightVertices(ClwScene const& scene)
    {
        // Fetch kernel
        std::string kernel_name = "GenerateLightVertices";

        CLWKernel genkernel = m_render_data->program.GetKernel(kernel_name);

        // Set kernel parameters
        int num_rays = m_output->width() * m_output->height();

        int argc = 0;
        genkernel.SetArg(argc++, num_rays);
        genkernel.SetArg(argc++, scene.vertices);
        genkernel.SetArg(argc++, scene.normals);
        genkernel.SetArg(argc++, scene.uvs);
        genkernel.SetArg(argc++, scene.indices);
        genkernel.SetArg(argc++, scene.shapes);
        genkernel.SetArg(argc++, scene.materialids);
        genkernel.SetArg(argc++, scene.materials);
        genkernel.SetArg(argc++, scene.textures);
        genkernel.SetArg(argc++, scene.texturedata);
        genkernel.SetArg(argc++, scene.envmapidx);
        genkernel.SetArg(argc++, scene.envmapmul);
        genkernel.SetArg(argc++, scene.lights);
        genkernel.SetArg(argc++, scene.num_lights);
        genkernel.SetArg(argc++, (int)rand_uint());
        genkernel.SetArg(argc++, m_render_data->rays[0]);
        genkernel.SetArg(argc++, m_render_data->random);
        genkernel.SetArg(argc++, m_render_data->sobolmat);
        genkernel.SetArg(argc++, m_framecnt);
        genkernel.SetArg(argc++, m_render_data->light_subpath);
        genkernel.SetArg(argc++, m_render_data->light_subpath_len);
        genkernel.SetArg(argc++, m_render_data->paths);

        // Run generation kernel
        {
            int globalsize = num_rays;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, genkernel);
        }
    }

    void BdptRenderer::Connect(ClwScene const& scene, int c, int l)
    {
        m_context.FillBuffer(0, m_render_data->shadowhits, 1, m_render_data->shadowhits.GetElementCount());


        int num_rays = m_output->width() * m_output->height();

        {
            // Fetch kernel
            std::string kernel_name = "Connect";

            CLWKernel connectkernel = m_render_data->program.GetKernel(kernel_name);

            // Set kernel parameters
            int num_rays = m_output->width() * m_output->height();

            int argc = 0;
            connectkernel.SetArg(argc++, c);
            connectkernel.SetArg(argc++, l);
            connectkernel.SetArg(argc++, m_render_data->camera_subpath_len);
            connectkernel.SetArg(argc++, m_render_data->camera_subpath);
            connectkernel.SetArg(argc++, m_render_data->light_subpath_len);
            connectkernel.SetArg(argc++, m_render_data->light_subpath);
            connectkernel.SetArg(argc++, scene.materials);
            connectkernel.SetArg(argc++, scene.textures);
            connectkernel.SetArg(argc++, scene.texturedata);
            connectkernel.SetArg(argc++, m_render_data->shadowrays);
            connectkernel.SetArg(argc++, m_render_data->lightsamples);

            // Run generation kernel
            {
                int globalsize = num_rays;
                m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, connectkernel);
            }
        }

        //std::vector<float3> samples(num_rays);
        //m_context.ReadBuffer(0, m_render_data->lightsamples, &samples[0], num_rays).Wait();

        m_scene_tracker.GetIntersectionApi()
            ->QueryOcclusion(m_render_data->fr_shadowrays, num_rays, m_render_data->fr_shadowhits, nullptr, nullptr);


        {
            // Fetch kernel
            std::string kernel_name = "GatherContributions";

            CLWKernel gatherkernel = m_render_data->program.GetKernel(kernel_name);

            // Set kernel parameters
            int num_rays = m_output->width() * m_output->height();

            int argc = 0;
            gatherkernel.SetArg(argc++, num_rays);
            gatherkernel.SetArg(argc++, m_render_data->shadowhits);
            gatherkernel.SetArg(argc++, m_render_data->lightsamples);
            gatherkernel.SetArg(argc++, m_output->data());

            // Run generation kernel
            {
                int globalsize = num_rays;
                m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gatherkernel);
            }
        }


    }

    // Render the scene into the output
    void BdptRenderer::Render(Scene1 const& scene)
    {
        auto& clwscene = m_scene_tracker.CompileScene(scene, m_render_data->mat_collector, m_render_data->tex_collector);

        // Check output
        assert(m_output);

        auto num_rays = m_output->width() *  m_output->height();

        // Generate primary
        GenerateCameraVertices(clwscene);

        RandomWalk(clwscene, num_rays, m_render_data->camera_subpath, m_render_data->camera_subpath_len);

        //std::vector<int> counts(num_rays);
        //m_context.ReadBuffer(0, m_render_data->camera_subpath_len, &counts[0], num_rays);

        //std::vector<PathVertex> vertices(num_rays * kMaxRandomWalkLength);
        //m_context.ReadBuffer(0, m_render_data->camera_subpath, &vertices[0], num_rays * kMaxRandomWalkLength).Wait();

        GenerateLightVertices(clwscene);

        RandomWalk(clwscene, num_rays, m_render_data->light_subpath, m_render_data->light_subpath_len);

        //m_context.ReadBuffer(0, m_render_data->light_subpath_len, &counts[0], num_rays);
        //m_context.ReadBuffer(0, m_render_data->light_subpath, &vertices[0], num_rays * kMaxRandomWalkLength).Wait();

        for (int c = 1; c < kMaxRandomWalkLength; ++c)
            for (int l = 1; l < kMaxRandomWalkLength; ++l)
            {
                Connect(clwscene, c, l);
            }

        ++m_framecnt;
    }

    void BdptRenderer::SetOutput(Output* output)
    {
        if (!m_output || m_output->width() < output->width() || m_output->height() < output->height())
        {
            ResizeWorkingSet(*output);
        }

        m_output = static_cast<ClwOutput*>(output);
    }

    void BdptRenderer::ResizeWorkingSet(Output const& output)
    {
        m_vidmemws = 0;

        // Create ray payloads
        m_render_data->rays[0] = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray);

        m_render_data->rays[1] = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray);

        m_render_data->hits = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);

        m_render_data->intersections = m_context.CreateBuffer<Intersection>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(Intersection);

        m_render_data->shadowrays = m_context.CreateBuffer<ray>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray)* kMaxLightSamples;

        m_render_data->shadowhits = m_context.CreateBuffer<int>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int)* kMaxLightSamples;

        m_render_data->lightsamples = m_context.CreateBuffer<float3>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(float3)* kMaxLightSamples;

        m_render_data->paths = m_context.CreateBuffer<PathState>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(PathState);

        std::vector<std::uint32_t> random_buffer(output.width() * output.height());
        std::generate(random_buffer.begin(), random_buffer.end(), std::rand);

        m_render_data->random = m_context.CreateBuffer<std::uint32_t>(output.width() * output.height(), CL_MEM_READ_WRITE, &random_buffer[0]);
        m_vidmemws += output.width() * output.height() * sizeof(std::uint32_t);

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

        m_render_data->camera_subpath = m_context.CreateBuffer<PathVertex>(output.width() * output.height() * kMaxRandomWalkLength, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * kMaxRandomWalkLength * sizeof(PathVertex);
        m_render_data->light_subpath = m_context.CreateBuffer<PathVertex>(output.width() * output.height() * kMaxRandomWalkLength, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * kMaxRandomWalkLength * sizeof(PathVertex);

        m_render_data->camera_subpath_len = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);
        m_render_data->light_subpath_len = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);

        auto api = m_scene_tracker.GetIntersectionApi();

        // Recreate FR buffers
        api->DeleteBuffer(m_render_data->fr_rays[0]);
        api->DeleteBuffer(m_render_data->fr_rays[1]);
        api->DeleteBuffer(m_render_data->fr_shadowrays);
        api->DeleteBuffer(m_render_data->fr_hits);
        api->DeleteBuffer(m_render_data->fr_shadowhits);
        api->DeleteBuffer(m_render_data->fr_intersections);
        api->DeleteBuffer(m_render_data->fr_hitcount);

        m_render_data->fr_rays[0] = CreateFromOpenClBuffer(api, m_render_data->rays[0]);
        m_render_data->fr_rays[1] = CreateFromOpenClBuffer(api, m_render_data->rays[1]);
        m_render_data->fr_shadowrays = CreateFromOpenClBuffer(api, m_render_data->shadowrays);
        m_render_data->fr_hits = CreateFromOpenClBuffer(api, m_render_data->hits);
        m_render_data->fr_shadowhits = CreateFromOpenClBuffer(api, m_render_data->shadowhits);
        m_render_data->fr_intersections = CreateFromOpenClBuffer(api, m_render_data->intersections);
        m_render_data->fr_hitcount = CreateFromOpenClBuffer(api, m_render_data->hitcount);

        std::cout << "Vidmem usage (working set): " << m_vidmemws / (1024 * 1024) << "Mb\n";
    }

    void BdptRenderer::GenerateCameraVertices(ClwScene const& scene)
    {
        // Fetch kernel
        std::string kernel_name = "PerspectiveCamera_GenerateVertices";

        CLWKernel genkernel = m_render_data->program.GetKernel(kernel_name);

        // Set kernel parameters
        genkernel.SetArg(0, scene.camera);
        genkernel.SetArg(1, m_output->width());
        genkernel.SetArg(2, m_output->height());
        genkernel.SetArg(3, (int)rand_uint());
        genkernel.SetArg(4, m_render_data->rays[0]);
        genkernel.SetArg(5, m_render_data->random);
        genkernel.SetArg(6, m_render_data->sobolmat);
        genkernel.SetArg(7, m_framecnt);
        genkernel.SetArg(8, m_render_data->camera_subpath);
        genkernel.SetArg(9, m_render_data->camera_subpath_len);
        genkernel.SetArg(10, m_render_data->paths);

        // Run generation kernel
        {
            size_t gs[] = { static_cast<size_t>((m_output->width() + 7) / 8 * 8), static_cast<size_t>((m_output->height() + 7) / 8 * 8) };
            size_t ls[] = { 8, 8 };

            m_context.Launch2D(0, gs, ls, genkernel);
        }
    }

    void BdptRenderer::SampleSurface(ClwScene const& scene, int pass,
        CLWBuffer<PathVertex> vertices, CLWBuffer<int> counts)
    {
        // Fetch kernel
        CLWKernel shadekernel = m_render_data->program.GetKernel("SampleSurface");

        // Set kernel parameters
        int argc = 0;
        shadekernel.SetArg(argc++, m_render_data->rays[pass & 0x1]);
        shadekernel.SetArg(argc++, m_render_data->intersections);
        shadekernel.SetArg(argc++, m_render_data->compacted_indices);
        shadekernel.SetArg(argc++, m_render_data->pixelindices[pass & 0x1]);
        shadekernel.SetArg(argc++, m_render_data->hitcount);
        shadekernel.SetArg(argc++, scene.vertices);
        shadekernel.SetArg(argc++, scene.normals);
        shadekernel.SetArg(argc++, scene.uvs);
        shadekernel.SetArg(argc++, scene.indices);
        shadekernel.SetArg(argc++, scene.shapes);
        shadekernel.SetArg(argc++, scene.materialids);
        shadekernel.SetArg(argc++, scene.materials);
        shadekernel.SetArg(argc++, scene.textures);
        shadekernel.SetArg(argc++, scene.texturedata);
        shadekernel.SetArg(argc++, scene.envmapidx);
        shadekernel.SetArg(argc++, scene.envmapmul);
        shadekernel.SetArg(argc++, scene.lights);
        shadekernel.SetArg(argc++, scene.num_lights);
        shadekernel.SetArg(argc++, rand_uint());
        shadekernel.SetArg(argc++, m_render_data->random);
        shadekernel.SetArg(argc++, m_render_data->sobolmat);
        shadekernel.SetArg(argc++, pass);
        shadekernel.SetArg(argc++, m_framecnt);
        shadekernel.SetArg(argc++, m_render_data->paths);
        shadekernel.SetArg(argc++, m_render_data->rays[(pass + 1) & 0x1]);
        shadekernel.SetArg(argc++, vertices);
        shadekernel.SetArg(argc++, counts);

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
        }
    }


    void BdptRenderer::RestorePixelIndices(int pass)
    {
        // Fetch kernel
        CLWKernel restorekernel = m_render_data->program.GetKernel("RestorePixelIndices");

        // Set kernel parameters
        int argc = 0;
        restorekernel.SetArg(argc++, m_render_data->compacted_indices);
        restorekernel.SetArg(argc++, m_render_data->hitcount);
        restorekernel.SetArg(argc++, m_render_data->pixelindices[(pass + 1) & 0x1]);
        restorekernel.SetArg(argc++, m_render_data->pixelindices[pass & 0x1]);

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, restorekernel);
        }
    }

    void BdptRenderer::FilterPathStream(int pass)
    {
        // Fetch kernel
        CLWKernel restorekernel = m_render_data->program.GetKernel("FilterPathStream");

        // Set kernel parameters
        int argc = 0;
        restorekernel.SetArg(argc++, m_render_data->intersections);
        restorekernel.SetArg(argc++, m_render_data->hitcount);
        restorekernel.SetArg(argc++, m_render_data->pixelindices[(pass + 1) & 0x1]);
        restorekernel.SetArg(argc++, m_render_data->paths);
        restorekernel.SetArg(argc++, m_render_data->hits);

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, restorekernel);
        }
    }

    CLWKernel BdptRenderer::GetCopyKernel()
    {
        return m_render_data->program.GetKernel("ApplyGammaAndCopyData");
    }

    CLWKernel BdptRenderer::GetAccumulateKernel()
    {
        return m_render_data->program.GetKernel("AccumulateData");
    }

    void BdptRenderer::RunBenchmark(Scene1 const& scene, std::uint32_t num_passes, BenchmarkStats& stats)
    {
    }
}
