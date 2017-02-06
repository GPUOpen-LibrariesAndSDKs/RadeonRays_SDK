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
#include "icrenderer.h"
#include "CLW/clwoutput.h"
#include "Scene/scene1.h"
#include "Scene/iterator.h"
#include "Scene/Collector/collector.h"
#include "Scene/shape.h"

#include "radiance_cache.h"
#include "hlbvh.h"

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
    static int const kMaxRadianceRecords = 512000;

    struct IcRenderer::PathState
    {
        float4 throughput;
        int volume;
        int flags;
        int extra0;
        int extra1;
    };

    struct IcRenderer::RenderData
    {
        // OpenCL stuff
        CLWBuffer<ray> rays[2];
        CLWBuffer<int> hits;
        CLWBuffer<float3> indirect;
        CLWBuffer<float3> indirect_througput;
        CLWBuffer<ray> indirect_rays;
        CLWBuffer<int> indirect_predicate;

        CLWBuffer<ray> shadowrays;
        CLWBuffer<int> shadowhits;

        CLWBuffer<Intersection> intersections;
        CLWBuffer<int> compacted_indices;
        CLWBuffer<int> pixelindices[2];
        CLWBuffer<int> iota;

        CLWBuffer<float3> lightsamples;
        CLWBuffer<float3> indirect_light_samples;
#ifdef RADIANCE_PROBE_DIRECT
        CLWBuffer<float3> direct_light_samples;
#endif
        CLWBuffer<PathState> paths;
        CLWBuffer<std::uint32_t> random;
        CLWBuffer<std::uint32_t> sobolmat;
        CLWBuffer<int> hitcount;

        CLWBuffer<RadianceCache::RadianceProbeDesc> temp_descs;
        CLWBuffer<RadianceCache::RadianceProbeData> temp_probes;

        CLWProgram program;
        CLWProgram program_ic;
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
    IcRenderer::IcRenderer(CLWContext context, int devidx, int num_bounces)
        : m_context(context)
        , m_output(nullptr)
        , m_render_data(new RenderData)
        , m_radiance_cache(new RadianceCache(context, kMaxRadianceRecords))
        , m_vidmemws(0)
        , m_scene_tracker(context, devidx)
        , m_num_bounces(num_bounces)
        , m_framecnt(0)
        , m_cache_ready(false)
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
        m_render_data->program = CLWProgram::CreateFromFile("../App/CL/integrator_pt.cl", buildopts.c_str(), m_context);
        m_render_data->program_ic = CLWProgram::CreateFromFile("../App/CL/integrator_ic.cl", buildopts.c_str(), m_context);
#else
        m_render_data->program = CLWProgram::CreateFromSource(cl_app, std::strlen(cl_integrator_pt), buildopts.c_str(), context);
#endif

        m_render_data->sobolmat = m_context.CreateBuffer<unsigned int>(1024 * 52, CL_MEM_READ_ONLY, &g_SobolMatrices[0]);
    }

    IcRenderer::~IcRenderer()
    {
    }

    Output* IcRenderer::CreateOutput(std::uint32_t w, std::uint32_t h) const
    {
        return new ClwOutput(w, h, m_context);
    }

    void IcRenderer::DeleteOutput(Output* output) const
    {
        delete output;
    }

    void IcRenderer::Clear(RadeonRays::float3 const& val, Output& output) const
    {
        static_cast<ClwOutput&>(output).Clear(val);
        m_framecnt = 0;
    }

    void IcRenderer::SetNumBounces(int num_bounces)
    {
        m_num_bounces = num_bounces;
    }

    void IcRenderer::Preprocess(Scene1 const& scene)
    {
    }

    // Render the scene into the output
    void IcRenderer::Render(Scene1 const& scene)
    {
        auto api = m_scene_tracker.GetIntersectionApi();
        auto& clwscene = m_scene_tracker.CompileScene(scene, m_render_data->mat_collector, m_render_data->tex_collector);

        // Check output
        assert(m_output);

        static bool once = false;

        if (!once)
        {
            std::size_t num_probes;
            GenerateProbeRequests(scene, m_render_data->temp_descs, m_render_data->temp_probes, num_probes);

            m_radiance_cache->AttachProbes(m_render_data->temp_descs, m_render_data->temp_probes, num_probes);

            once = true;
        }

        // Number of rays to generate
        int maxrays = m_output->width() * m_output->height();

        // Generate primary
        GeneratePrimaryRays(clwscene);

        // Copy compacted indices to track reverse indices
        m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[0], 0, 0, m_render_data->iota.GetElementCount());
        m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[1], 0, 0, m_render_data->iota.GetElementCount());
        m_context.FillBuffer(0, m_render_data->hitcount, maxrays, 1);
        m_context.FillBuffer(0, m_render_data->indirect, float3(), m_render_data->indirect.GetElementCount());
        m_context.FillBuffer(0, m_render_data->indirect_througput, float3(1.f, 1.f, 1.f), m_render_data->indirect_througput.GetElementCount());
        m_context.FillBuffer(0, m_render_data->indirect_predicate, -1, m_render_data->indirect_predicate.GetElementCount());

        int pass_to_cache = (rand_uint() % (m_num_bounces - 2)) + 1;

        // Initialize first pass
        for (int pass = 0; pass < m_num_bounces; ++pass)
        {
            // Clear ray hits buffer
            m_context.FillBuffer(0, m_render_data->hits, 0, m_render_data->hits.GetElementCount());

            // Intersect ray batch
            //Event* e = nullptr;
            api->QueryIntersection(m_render_data->fr_rays[pass & 0x1], m_render_data->fr_hitcount, maxrays, m_render_data->fr_intersections, nullptr, nullptr);
            //e->Wait();
            //m_api->DeleteEvent(e);

            // Apply scattering
            EvaluateVolume(clwscene, pass);

            if (pass > 0 && clwscene.envmapidx > -1)
            {
                ShadeBackground(clwscene, pass);
            }

            // Convert intersections to predicates
            FilterPathStream(pass);

            // Compact batch
            m_render_data->pp.Compact(0, m_render_data->hits, m_render_data->iota, m_render_data->compacted_indices, m_render_data->hitcount);

            /*int cnt = 0;
            m_context.ReadBuffer(0, m_render_data->hitcount[0], &cnt, 1).Wait();
            std::cout << "Pass " << pass << " Alive " << cnt << "\n";*/

            // Advance indices to keep pixel indices up to date
            RestorePixelIndices(pass);

            // Shade hits
            ShadeVolume(clwscene, pass);

            if (!m_cache_ready)
                // Shade hits
                ShadeSurface(clwscene, pass, pass_to_cache);
            else
                ShadeSurfaceCached(clwscene, pass);

            // Shade missing rays
            if (pass == 0)
                ShadeMiss(clwscene, pass);

            // Intersect shadow rays
            api->QueryOcclusion(m_render_data->fr_shadowrays, m_render_data->fr_hitcount, maxrays, m_render_data->fr_shadowhits, nullptr, nullptr);
            //e->Wait();
            //m_api->DeleteEvent(e);

            // Gather light samples and account for visibility
            GatherLightSamples(clwscene, pass);

#ifdef RADIANCE_PROBE_DIRECT
            UpdateDirectRadianceCache(clwscene, pass, maxrays);
#endif

            //
            m_context.Flush(0);
        }

        if (m_framecnt < 128 && !m_cache_ready)
        {
            UpdateRadianceCache(clwscene, 0, maxrays);
        }
        else
        {
            m_cache_ready = true;
        }

        ++m_framecnt;
    }

    void IcRenderer::SetOutput(Output* output)
    {
        if (!m_output || m_output->width() < output->width() || m_output->height() < output->height())
        {
            ResizeWorkingSet(*output);
        }

        m_output = static_cast<ClwOutput*>(output);
    }

    void IcRenderer::ResizeWorkingSet(Output const& output)
    {
        m_vidmemws = 0;

        // Create ray payloads
        m_render_data->rays[0] = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray);

        m_render_data->rays[1] = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray);

        m_render_data->indirect_rays = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray);

        m_render_data->indirect_predicate = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);

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

        m_render_data->indirect_light_samples = m_context.CreateBuffer<float3>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(float3)* kMaxLightSamples;

#ifdef RADIANCE_PROBE_DIRECT
        m_render_data->direct_light_samples = m_context.CreateBuffer<float3>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(float3)* kMaxLightSamples;
#endif

        m_render_data->indirect = m_context.CreateBuffer<float3>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(float3);

        m_render_data->indirect_througput = m_context.CreateBuffer<float3>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(float3);

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

    void IcRenderer::GeneratePrimaryRays(ClwScene const& scene)
    {
        // Fetch kernel
        std::string kernel_name = (scene.camera_type == CameraType::kDefault) ? "PerspectiveCamera_GeneratePaths" : "PerspectiveCameraDof_GeneratePaths";

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
        genkernel.SetArg(8, m_render_data->paths);

        // Run generation kernel
        {
            size_t gs[] = { static_cast<size_t>((m_output->width() + 7) / 8 * 8), static_cast<size_t>((m_output->height() + 7) / 8 * 8) };
            size_t ls[] = { 8, 8 };

            m_context.Launch2D(0, gs, ls, genkernel);
        }
    }

    void IcRenderer::ShadeSurface(ClwScene const& scene, int pass, int pass_cached)
    {
        // Fetch kernel
        CLWKernel shadekernel = m_render_data->program_ic.GetKernel("ShadeSurfaceAndCache");

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
        shadekernel.SetArg(argc++, pass_cached);
        shadekernel.SetArg(argc++, m_framecnt);
        shadekernel.SetArg(argc++, scene.volumes);
        shadekernel.SetArg(argc++, m_render_data->shadowrays);
        shadekernel.SetArg(argc++, m_render_data->lightsamples);
        shadekernel.SetArg(argc++, m_render_data->indirect_light_samples);
        shadekernel.SetArg(argc++, m_render_data->paths);
        shadekernel.SetArg(argc++, m_render_data->rays[(pass + 1) & 0x1]);
        shadekernel.SetArg(argc++, m_output->data());
        shadekernel.SetArg(argc++, m_render_data->indirect);
        shadekernel.SetArg(argc++, m_render_data->indirect_througput);
        shadekernel.SetArg(argc++, m_render_data->indirect_rays);
        shadekernel.SetArg(argc++, m_render_data->indirect_predicate);
#ifdef RADIANCE_PROBE_DIRECT
        shadekernel.SetArg(argc++, m_render_data->direct_light_samples);
#endif

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
        }
    }

    void IcRenderer::ShadeVolume(ClwScene const& scene, int pass)
    {
        // Fetch kernel
        CLWKernel shadekernel = m_render_data->program.GetKernel("ShadeVolume");

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
        shadekernel.SetArg(argc++, scene.volumes);
        shadekernel.SetArg(argc++, m_render_data->shadowrays);
        shadekernel.SetArg(argc++, m_render_data->lightsamples);
        shadekernel.SetArg(argc++, m_render_data->paths);
        shadekernel.SetArg(argc++, m_render_data->rays[(pass + 1) & 0x1]);
        shadekernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
        }

    }

    void IcRenderer::EvaluateVolume(ClwScene const& scene, int pass)
    {
        // Fetch kernel
        CLWKernel evalkernel = m_render_data->program.GetKernel("EvaluateVolume");

        // Set kernel parameters
        int argc = 0;
        evalkernel.SetArg(argc++, m_render_data->rays[pass & 0x1]);
        evalkernel.SetArg(argc++, m_render_data->pixelindices[(pass + 1) & 0x1]);
        evalkernel.SetArg(argc++, m_render_data->hitcount);
        evalkernel.SetArg(argc++, scene.volumes);
        evalkernel.SetArg(argc++, scene.textures);
        evalkernel.SetArg(argc++, scene.texturedata);
        evalkernel.SetArg(argc++, rand_uint());
        evalkernel.SetArg(argc++, m_render_data->random);
        evalkernel.SetArg(argc++, m_render_data->sobolmat);
        evalkernel.SetArg(argc++, pass);
        evalkernel.SetArg(argc++, m_framecnt);
        evalkernel.SetArg(argc++, m_render_data->intersections);
        evalkernel.SetArg(argc++, m_render_data->paths);
        evalkernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, evalkernel);
        }
    }

    void IcRenderer::ShadeMiss(ClwScene const& scene, int pass)
    {
        // Fetch kernel
        CLWKernel misskernel = m_render_data->program.GetKernel("ShadeMiss");

        int numrays = m_output->width() * m_output->height();

        // Set kernel parameters
        int argc = 0;
        misskernel.SetArg(argc++, m_render_data->rays[pass & 0x1]);
        misskernel.SetArg(argc++, m_render_data->intersections);
        misskernel.SetArg(argc++, m_render_data->pixelindices[(pass + 1) & 0x1]);
        misskernel.SetArg(argc++, numrays);
        misskernel.SetArg(argc++, scene.textures);
        misskernel.SetArg(argc++, scene.texturedata);
        misskernel.SetArg(argc++, scene.envmapidx);
        misskernel.SetArg(argc++, m_render_data->paths);
        misskernel.SetArg(argc++, scene.volumes);
        misskernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, misskernel);
        }
    }

    void IcRenderer::GatherLightSamples(ClwScene const& scene, int pass)
    {
        // Fetch kernel
        CLWKernel gatherkernel = m_render_data->program_ic.GetKernel("GatherLightSamplesIndirect");

        // Set kernel parameters
        int argc = 0;
        gatherkernel.SetArg(argc++, m_render_data->pixelindices[pass & 0x1]);
        gatherkernel.SetArg(argc++, m_render_data->hitcount);
        gatherkernel.SetArg(argc++, m_render_data->shadowhits);
        gatherkernel.SetArg(argc++, m_render_data->lightsamples);
        gatherkernel.SetArg(argc++, m_render_data->indirect_light_samples);
        gatherkernel.SetArg(argc++, m_render_data->paths);
        gatherkernel.SetArg(argc++, m_output->data());
        gatherkernel.SetArg(argc++, m_render_data->indirect);

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gatherkernel);
        }
    }

    void IcRenderer::RestorePixelIndices(int pass)
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

    void IcRenderer::FilterPathStream(int pass)
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

        //int cnt = 0;
        //m_context.ReadBuffer(0, m_render_data->debug, &cnt, 1).Wait();
        //std::cout << "Pass " << pass << " killed " << cnt << "\n";
    }

    CLWKernel IcRenderer::GetCopyKernel()
    {
        return m_render_data->program.GetKernel("ApplyGammaAndCopyData");
    }

    CLWKernel IcRenderer::GetAccumulateKernel()
    {
        return m_render_data->program.GetKernel("AccumulateData");
    }

    // Shade background
    void IcRenderer::ShadeBackground(ClwScene const& scene, int pass)
    {
        // Fetch kernel
        CLWKernel misskernel = m_render_data->program.GetKernel("ShadeBackground");

        //int numrays = m_output->width() * m_output->height();

        // Set kernel parameters
        int argc = 0;
        misskernel.SetArg(argc++, m_render_data->rays[pass & 0x1]);
        misskernel.SetArg(argc++, m_render_data->intersections);
        misskernel.SetArg(argc++, m_render_data->pixelindices[(pass + 1) & 0x1]);
        misskernel.SetArg(argc++, m_render_data->hitcount);
        misskernel.SetArg(argc++, scene.textures);
        misskernel.SetArg(argc++, scene.texturedata);
        misskernel.SetArg(argc++, scene.envmapidx);
        misskernel.SetArg(argc++, scene.envmapmul);
        misskernel.SetArg(argc++, scene.num_lights);
        misskernel.SetArg(argc++, m_render_data->paths);
        misskernel.SetArg(argc++, scene.volumes);
        misskernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, misskernel);
        }

    }

    void IcRenderer::RunBenchmark(Scene1 const& scene, std::uint32_t num_passes, BenchmarkStats& stats)
    {
        stats.num_passes = num_passes;
        stats.resolution = int2(m_output->width(), m_output->height());

        auto api = m_scene_tracker.GetIntersectionApi();
        auto& clwscene = m_scene_tracker.CompileScene(scene, m_render_data->mat_collector, m_render_data->tex_collector);

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


        // Clear ray hits buffer
        m_context.FillBuffer(0, m_render_data->hits, 0, m_render_data->hits.GetElementCount());

        // Intersect ray batch
        auto start = std::chrono::high_resolution_clock::now();

        for (auto i = 0U; i < num_passes; ++i)
        {
            api->QueryIntersection(m_render_data->fr_rays[0], m_render_data->fr_hitcount, maxrays, m_render_data->fr_intersections, nullptr, nullptr);
        }

        m_context.Finish(0);

        auto delta = std::chrono::high_resolution_clock::now() - start;

        stats.primary_rays_time_in_ms = (float)std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() / num_passes;

        // Convert intersections to predicates
        FilterPathStream(0);

        // Compact batch
        m_render_data->pp.Compact(0, m_render_data->hits, m_render_data->iota, m_render_data->compacted_indices, m_render_data->hitcount);

        // Advance indices to keep pixel indices up to date
        RestorePixelIndices(0);

        // Shade hits
        ShadeSurface(clwscene, 0, 0);

        // Shade missing rays
        ShadeMiss(clwscene, 0);

        // Intersect ray batch
        start = std::chrono::high_resolution_clock::now();

        for (auto i = 0U; i < num_passes; ++i)
        {
            api->QueryOcclusion(m_render_data->fr_shadowrays, m_render_data->fr_hitcount, maxrays, m_render_data->fr_shadowhits, nullptr, nullptr);
        }

        m_context.Finish(0);

        delta = std::chrono::high_resolution_clock::now() - start;

        stats.shadow_rays_time_in_ms = (float)std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() / num_passes;

        // Gather light samples and account for visibility
        GatherLightSamples(clwscene, 0);

        //
        m_context.Flush(0);

        // Clear ray hits buffer
        m_context.FillBuffer(0, m_render_data->hits, 0, m_render_data->hits.GetElementCount());

        // Intersect ray batch
        start = std::chrono::high_resolution_clock::now();

        for (auto i = 0U; i < num_passes; ++i)
        {
            api->QueryIntersection(m_render_data->fr_rays[1], m_render_data->fr_hitcount, maxrays, m_render_data->fr_intersections, nullptr, nullptr);
        }

        m_context.Finish(0);

        delta = std::chrono::high_resolution_clock::now() - start;

        stats.secondary_rays_time_in_ms = (float)std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() / num_passes;

        // Convert intersections to predicates
        FilterPathStream(1);

        // Compact batch
        m_render_data->pp.Compact(0, m_render_data->hits, m_render_data->iota, m_render_data->compacted_indices, m_render_data->hitcount);

        // Advance indices to keep pixel indices up to date
        RestorePixelIndices(1);

        // Shade hits
        ShadeSurface(clwscene, 1, 0);

        // Shade missing rays
        ShadeMiss(clwscene, 1);

        api->QueryOcclusion(m_render_data->fr_shadowrays, m_render_data->fr_hitcount, maxrays, m_render_data->fr_shadowhits, nullptr, nullptr);

        // Gather light samples and account for visibility
        GatherLightSamples(clwscene, 0);

        //
        m_context.Flush(0);
    }

    void IcRenderer::GenerateProbeRequests(Scene1 const& scene,
        CLWBuffer<RadianceCache::RadianceProbeDesc>& desc,
        CLWBuffer<RadianceCache::RadianceProbeData>& probes,
        std::size_t& num_probes)
    {
        std::vector<RadianceCache::RadianceProbeDesc> temp_desc;

        auto shape_iter = scene.CreateShapeIterator();

        for (shape_iter; shape_iter->IsValid(); shape_iter->Next())
        {
            auto mesh = shape_iter->ItemAs<Mesh const>();

            // Get pointers data
            auto mesh_vertex_array = mesh->GetVertices();
            auto mesh_num_vertices = mesh->GetNumVertices();

            auto mesh_normal_array = mesh->GetNormals();
            auto mesh_num_normals = mesh->GetNumNormals();

            auto mesh_uv_array = mesh->GetUVs();
            auto mesh_num_uvs = mesh->GetNumUVs();

            auto mesh_index_array = mesh->GetIndices();
            auto mesh_num_indices = mesh->GetNumIndices();

            float density_per_sq_cm = 200.f;

            for (auto idx = 0U; idx < mesh_num_indices / 3; ++idx)
            {
                float3 v1 = mesh_vertex_array[mesh_index_array[3 * idx]];
                float3 v2 = mesh_vertex_array[mesh_index_array[3 * idx + 1]];
                float3 v3 = mesh_vertex_array[mesh_index_array[3 * idx + 2]];

                float3 n1 = mesh_normal_array[mesh_index_array[3 * idx]];
                float3 n2 = mesh_normal_array[mesh_index_array[3 * idx + 1]];
                float3 n3 = mesh_normal_array[mesh_index_array[3 * idx + 2]];

                float area = 0.5f * sqrtf(cross(v3 - v1, v2 - v1).sqnorm());
                int num_samples = (int)(area * density_per_sq_cm);

                for (int ss = 0; ss < num_samples; ++ss)
                {
                    float r0 = rand_float();
                    float r1 = rand_float();

                    // Convert random to barycentric coords
                    float2 uv;
                    uv.x = sqrtf(r0) * (1.f - r1);
                    uv.y = sqrtf(r0) * r1;

                    float3 n = ((1.f - uv.x - uv.y) * n1 + uv.x * n2 + uv.y * n3);
                    n.normalize();

                    float3 p = (1.f - uv.x - uv.y) * v1 + uv.x * v2 + uv.y * v3;

                    float3 t = normalize(cross(n, v3 - v1));
                    float3 b = cross(t, n);

                    matrix world_to_tangent;
                    world_to_tangent.m00 = t.x; world_to_tangent.m01 = t.y; world_to_tangent.m02 = t.z;
                    world_to_tangent.m03 = -dot(t, p);
                    world_to_tangent.m10 = n.x; world_to_tangent.m11 = n.y; world_to_tangent.m12 = n.z;
                    world_to_tangent.m13 = -dot(n, p);
                    world_to_tangent.m20 = b.x; world_to_tangent.m21 = b.y; world_to_tangent.m22 = b.z;
                    world_to_tangent.m23 = -dot(b, p);

                    matrix tangent_to_world = world_to_tangent.transpose();
                    tangent_to_world.m03 = p.x;
                    tangent_to_world.m13 = p.y;
                    tangent_to_world.m23 = p.z;
                    tangent_to_world.m30 = tangent_to_world.m31 = tangent_to_world.m32 = 0;
                    tangent_to_world.m33 = 1.f;

                    temp_desc.push_back(RadianceCache::RadianceProbeDesc{ world_to_tangent , tangent_to_world, p, 0.5f, 1, 0, 1 });
                }
            }
        }

        desc = m_context.CreateBuffer<RadianceCache::RadianceProbeDesc>(temp_desc.size(), CL_MEM_READ_WRITE, &temp_desc[0]);

        /// Just to make it faster
        std::vector<RadianceCache::RadianceProbeData> temp_probes(temp_desc.size());
        std::cout << "Radiance probes count " << temp_desc.size() << "\n";

        probes = m_context.CreateBuffer<RadianceCache::RadianceProbeData>(temp_desc.size(), CL_MEM_READ_WRITE, &temp_probes[0]);

        num_probes = temp_desc.size();
    }

    void IcRenderer::RenderCachedData(Scene1 const& scene)
    {
        auto api = m_scene_tracker.GetIntersectionApi();
        auto& clwscene = m_scene_tracker.CompileScene(scene, m_render_data->mat_collector, m_render_data->tex_collector);

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

        // Clear ray hits buffer
        m_context.FillBuffer(0, m_render_data->hits, 0, m_render_data->hits.GetElementCount());

        // Intersect ray batch
        // Event* e = nullptr;
        api->QueryIntersection(m_render_data->fr_rays[0], m_render_data->fr_hitcount, maxrays, m_render_data->fr_intersections, nullptr, nullptr);
        // e->Wait();
        // m_api->DeleteEvent(e);

        // Convert intersections to predicates
        FilterPathStream(0);

        // Compact batch
        m_render_data->pp.Compact(0, m_render_data->hits, m_render_data->iota, m_render_data->compacted_indices, m_render_data->hitcount);

        /* int cnt = 0;
        m_context.ReadBuffer(0, m_render_data->hitcount[0], &cnt, 1).Wait();
        std::cout << "Pass " << pass << " Alive " << cnt << "\n"; */

        // Advance indices to keep pixel indices up to date
        RestorePixelIndices(0);

        // Shade hits
        VisualizeCache(clwscene);

        //
        m_context.Flush(0);

        ++m_framecnt;

    }

    void IcRenderer::VisualizeCache(ClwScene& scene)
    {
        // Fetch kernel
        CLWKernel shadekernel = m_render_data->program_ic.GetKernel("VisualizeCache");

        // Set kernel parameters
        int argc = 0;
        shadekernel.SetArg(argc++, m_render_data->rays[0]);
        shadekernel.SetArg(argc++, m_render_data->intersections);
        shadekernel.SetArg(argc++, m_render_data->compacted_indices);
        shadekernel.SetArg(argc++, m_render_data->pixelindices[0]);
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
        shadekernel.SetArg(argc++, m_radiance_cache->GetAccel().GetGpuData().nodes);
        shadekernel.SetArg(argc++, m_radiance_cache->GetAccel().GetGpuData().sorted_bounds);
        shadekernel.SetArg(argc++, m_radiance_cache->GetProbeDescs());
        shadekernel.SetArg(argc++, m_radiance_cache->GetProbes());
        shadekernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
        } 

    }

    void IcRenderer::UpdateRadianceCache(ClwScene const& scene, int pass, std::size_t num_rays)
    {
        m_radiance_cache->AddRadianceSamples(m_render_data->indirect_rays, m_render_data->indirect_predicate, m_render_data->indirect, num_rays);
    }

#ifdef RADIANCE_PROBE_DIRECT
    void IcRenderer::UpdateDirectRadianceCache(ClwScene const& scene, int pass, std::size_t max_rays)
    {
        m_radiance_cache->AddDirectRadianceSamples(m_render_data->shadowrays, m_render_data->shadowhits, m_render_data->direct_light_samples, m_render_data->hitcount, max_rays);
    }
#endif

    void IcRenderer::ShadeSurfaceCached(ClwScene const& scene, int pass)
    {
        // Fetch kernel
        CLWKernel shadekernel = m_render_data->program_ic.GetKernel("ShadeSurface");

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
        shadekernel.SetArg(argc++, scene.volumes);

        shadekernel.SetArg(argc++, m_radiance_cache->GetAccel().GetGpuData().nodes);
        shadekernel.SetArg(argc++, m_radiance_cache->GetAccel().GetGpuData().sorted_bounds);
        shadekernel.SetArg(argc++, m_radiance_cache->GetProbeDescs());
        shadekernel.SetArg(argc++, m_radiance_cache->GetProbes());

        shadekernel.SetArg(argc++, m_render_data->shadowrays);
        shadekernel.SetArg(argc++, m_render_data->lightsamples);
        shadekernel.SetArg(argc++, m_render_data->paths);
        shadekernel.SetArg(argc++, m_render_data->rays[(pass + 1) & 0x1]);
        shadekernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
        }
    }
}
