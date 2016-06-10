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
#include "PT/ptrenderer.h"
#include "CLW/clwoutput.h"
#include "scene.h"

#include <numeric>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstdint>

#include "sobol.h"

#ifdef FR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

namespace Baikal
{
    using namespace FireRays;

    static int const kMaxLightSamples = 1;

    struct PtRenderer::QmcSampler
    {
        std::uint32_t seq;
        std::uint32_t s0;
        std::uint32_t s1;
        std::uint32_t s2;
    };

    struct PtRenderer::PathState
    {
        float4 throughput;
        int volume;
        int flags;
        int extra0;
        int extra1;
    };

    struct PtRenderer::Volume
    {
        int type;
        int phase_func;
        int data;
        int extra;

        float3 sigma_a;
        float3 sigma_s;
        float3 sigma_e;
    };

    struct PtRenderer::RenderData
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
        CLWBuffer<QmcSampler> samplers;
        CLWBuffer<unsigned int> sobolmat;
        CLWBuffer<int> hitcount;

        CLWProgram program;
        CLWParallelPrimitives pp;

        // FireRays stuff
        Buffer* fr_rays[2];
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
        {
            fr_rays[0] = nullptr;
            fr_rays[1] = nullptr;
        }
    };

    struct PtRenderer::SceneData
    {
        CLWBuffer<Volume> volumes;
        CLWBuffer<int> materialids;
        CLWBuffer<Scene::Emissive> emissives;
        CLWBuffer<Scene::Material> materials;
        CLWBuffer<float3> vertices;
        CLWBuffer<float3> normals;
        CLWBuffer<float2> uvs;
        CLWBuffer<int> indices;
        CLWBuffer<Scene::Shape> shapes;
        CLWBuffer<Scene::Texture> textures;
        CLWBuffer<char> texturedata;
        CLWBuffer<PerspectiveCamera> camera;

        int numemissive;
    };

    // Constructor
    PtRenderer::PtRenderer(CLWContext context, int devidx)
    : m_context(context)
    , m_output(nullptr)
    , m_render_data(new RenderData)
    , m_scene_data(new SceneData)
    , m_vidmemusage(0)
    , m_vidmemws(0)
    , m_resetsampler(true)
    {
        // Get raw CL data out of CLW context
        cl_device_id id = m_context.GetDevice(devidx).GetID();
        cl_command_queue queue = m_context.GetCommandQueue(devidx);

        // Create intersection API
        m_api = IntersectionApiCL::CreateFromOpenClContext(m_context, id, queue);

        // Do app specific settings
        #ifdef __APPLE__
        // Apple runtime has known issue with stacked traversal
        m_api->SetOption("acc.type", "bvh");
        m_api->SetOption("bvh.builder", "sah");
        #else
        m_api->SetOption("acc.type", "fatbvh");
        m_api->SetOption("bvh.builder", "sah");
        #endif

        // Create parallel primitives
        m_render_data->pp = CLWParallelPrimitives(m_context);

        // Load kernels
        #ifndef FR_EMBED_KERNELS
        m_render_data->program = CLWProgram::CreateFromFile("../App/CL/integrator.cl", m_context);
        #else
        m_render_data->program = CLWProgram::CreateFromSource(cl_app, std::strlen(cl_integrator), context);
        #endif

        // Create static buffers
        m_scene_data->camera = m_context.CreateBuffer<PerspectiveCamera>(1, CL_MEM_READ_ONLY);
        m_render_data->hitcount = m_context.CreateBuffer<int>(1, CL_MEM_READ_ONLY);
        m_render_data->fr_hitcount = m_api->CreateFromOpenClBuffer(m_render_data->hitcount);

        m_render_data->sobolmat = m_context.CreateBuffer<unsigned int>(1024 * 52, CL_MEM_READ_ONLY, &g_SobolMatrices[0]);

        //Volume vol = {1, 0, 0, 0, {0.9f, 0.6f, 0.9f}, {5.1f, 1.8f, 5.1f}, {0.0f, 0.0f, 0.0f}};
        Volume vol = { 1, 0, 0, 0,{	1.2f, 0.4f, 1.2f },{ 5.1f, 4.8f, 5.1f },{ 0.0f, 0.0f, 0.0f } };

        m_scene_data->volumes = m_context.CreateBuffer<Volume>(1, CL_MEM_READ_ONLY, &vol);
    }

    PtRenderer::~PtRenderer()
    {
        // Delete all shapes
        for (int i = 0; i < (int)m_shapes.size(); ++i)
        {
            m_api->DeleteShape(m_shapes[i]);
        }

        // Delete API
        IntersectionApi::Delete(m_api);
    }

    Output* PtRenderer::CreateOutput(std::uint32_t w, std::uint32_t h) const
    {
        return new ClwOutput(w, h, m_context);
    }

    void PtRenderer::DeleteOutput(Output* output) const
    {
        delete output;
    }

    void PtRenderer::Clear(FireRays::float3 const& val, Output& output) const
    {
        static_cast<ClwOutput&>(output).Clear(val);
        m_resetsampler = true;
    }

    void PtRenderer::Preprocess(Scene const& scene)
    {
        // Release old data if any
        // TODO: implement me
        rand_init();

        // Enumerate all shapes in the scene
        for (int i = 0; i < (int)scene.shapes_.size(); ++i)
        {
            Shape* shape = nullptr;

            shape = m_api->CreateMesh(
            // Vertices starting from the first one
            (float*)&scene.vertices_[scene.shapes_[i].startvtx],
            // Number of vertices
            scene.shapes_[i].numvertices,
            // Stride
            sizeof(float3),
            // TODO: make API signature const
            const_cast<int*>(&scene.indices_[scene.shapes_[i].startidx]),
            // Index stride
            0,
            // All triangles
            nullptr,
            // Number of primitives
            (int)scene.shapes_[i].numprims
            );

            int midx = scene.materialids_[scene.shapes_[i].startidx / 3];
            if (scene.material_names_[midx] == "glass" || scene.material_names_[midx] == "glasstranslucent")
            {
                //std::cout << "Setting glass mask\n";
                shape->SetMask(0xFFFF0000);
            }

            shape->SetLinearVelocity(scene.shapes_[i].linearvelocity);

            shape->SetAngularVelocity(scene.shapes_[i].angularvelocity);

            shape->SetTransform(scene.shapes_[i].m, inverse(scene.shapes_[i].m));

            m_api->AttachShape(shape);

            m_shapes.push_back(shape);
        }

        // Commit to intersector
        auto startime = std::chrono::high_resolution_clock::now();

        m_api->Commit();

        auto delta = std::chrono::high_resolution_clock::now() - startime;

        std::cout << "Commited in " << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() / 1000.f << "s\n";

        // Duplicate geometry in OpenCL buffers
        CompileScene(scene);
    }

    // Render the scene into the output
    void PtRenderer::Render(Scene const& scene)
    {
        // Check output
        assert(m_output);

        // Update camera data
        m_context.WriteBuffer(0, m_scene_data->camera, scene.camera_.get(), 1);

        // Number of rays to generate
        int maxrays = m_output->width() * m_output->height();

        // Generate primary
        GeneratePrimaryRays();

        // Copy compacted indices to track reverse indices
        m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[0], 0, 0, m_render_data->iota.GetElementCount());
        m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[1], 0, 0, m_render_data->iota.GetElementCount());
        m_context.FillBuffer(0, m_render_data->hitcount, maxrays, 1);

        //float3 clearval;
        //clearval.w = 1.f;
        //m_context.FillBuffer(0, m_render_data->radiance, clearval, m_render_data->radiance.GetElementCount());

        // Initialize first pass
        for (int pass = 0; pass < 5; ++pass)
        {
            // Clear ray hits buffer
            m_context.FillBuffer(0, m_render_data->hits, 0, m_render_data->hits.GetElementCount());

            // Intersect ray batch
            //Event* e = nullptr;
            m_api->QueryIntersection(m_render_data->fr_rays[pass & 0x1], m_render_data->fr_hitcount, maxrays, m_render_data->fr_intersections, nullptr, nullptr);
            //e->Wait();
            //m_api->DeleteEvent(e);

            // Apply scattering
            EvaluateVolume(scene, pass);

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
            ShadeVolume(scene, pass);

            // Shade hits
            ShadeSurface(scene, pass);

            // Shade missing rays
            if (pass == 0) ShadeMiss(scene, pass);

            // Intersect shadow rays
            m_api->QueryOcclusion(m_render_data->fr_shadowrays, m_render_data->fr_hitcount, maxrays, m_render_data->fr_shadowhits, nullptr, nullptr);
            //e->Wait();
            //m_api->DeleteEvent(e);

            // Gather light samples and account for visibility
            GatherLightSamples(scene, pass);

            //
            m_context.Flush(0);
        }
    }

    void PtRenderer::SetOutput(Output* output)
    {
        if (!m_output || m_output->width() < output->width() || m_output->height() < output->height())
        {
            ResizeWorkingSet(*output);
        }

        m_output = static_cast<ClwOutput*>(output);
    }

    void PtRenderer::ResizeWorkingSet(Output const& output)
    {
        m_vidmemws = 0;

        // Create ray payloads
        m_render_data->rays[0] = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray);

        m_render_data->rays[1] = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray);

        m_render_data->rays[2] = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(ray);

        m_render_data->rays[3] = m_context.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
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

        // Recreate FR buffers
        m_api->DeleteBuffer(m_render_data->fr_rays[0]);
        m_api->DeleteBuffer(m_render_data->fr_rays[1]);
        m_api->DeleteBuffer(m_render_data->fr_shadowrays);
        m_api->DeleteBuffer(m_render_data->fr_hits);
        m_api->DeleteBuffer(m_render_data->fr_shadowhits);
        m_api->DeleteBuffer(m_render_data->fr_intersections);

        m_render_data->fr_rays[0] = m_api->CreateFromOpenClBuffer(m_render_data->rays[0]);
        m_render_data->fr_rays[1] = m_api->CreateFromOpenClBuffer(m_render_data->rays[1]);
        m_render_data->fr_shadowrays = m_api->CreateFromOpenClBuffer(m_render_data->shadowrays);
        m_render_data->fr_hits = m_api->CreateFromOpenClBuffer(m_render_data->hits);
        m_render_data->fr_shadowhits = m_api->CreateFromOpenClBuffer(m_render_data->shadowhits);
        m_render_data->fr_intersections = m_api->CreateFromOpenClBuffer(m_render_data->intersections);

        std::cout << "Vidmem usage (working set): " << m_vidmemws / (1024 * 1024) << "Mb\n";
    }

    void PtRenderer::GeneratePrimaryRays()
    {
        // Fetch kernel
        CLWKernel genkernel = m_render_data->program.GetKernel("PerspectiveCamera_GeneratePaths");

        // Set kernel parameters
        genkernel.SetArg(0, m_scene_data->camera);
        genkernel.SetArg(1, m_output->width());
        genkernel.SetArg(2, m_output->height());
        genkernel.SetArg(3, (int)rand_uint());
        genkernel.SetArg(4, m_render_data->rays[0]);
        genkernel.SetArg(5, m_render_data->samplers);
        genkernel.SetArg(6, m_render_data->sobolmat);
        genkernel.SetArg(7, m_resetsampler);
        genkernel.SetArg(8, m_render_data->paths);
        m_resetsampler = 0;

        // Run generation kernel
        {
            size_t gs[] = { static_cast<size_t>((m_output->width() + 7) / 8 * 8), static_cast<size_t>((m_output->height() + 7) / 8 * 8) };
            size_t ls[] = { 8, 8 };

            m_context.Launch2D(0, gs, ls, genkernel);
        }
    }

    void PtRenderer::ShadeSurface(Scene const& scene, int pass)
    {
        // Fetch kernel
        CLWKernel shadekernel = m_render_data->program.GetKernel("ShadeSurface");

        // Set kernel parameters
        int argc = 0;
        shadekernel.SetArg(argc++, m_render_data->rays[pass & 0x1]);
        shadekernel.SetArg(argc++, m_render_data->intersections);
        shadekernel.SetArg(argc++, m_render_data->compacted_indices);
        shadekernel.SetArg(argc++, m_render_data->pixelindices[pass & 0x1]);
        shadekernel.SetArg(argc++, m_render_data->hitcount);
        shadekernel.SetArg(argc++, m_scene_data->vertices);
        shadekernel.SetArg(argc++, m_scene_data->normals);
        shadekernel.SetArg(argc++, m_scene_data->uvs);
        shadekernel.SetArg(argc++, m_scene_data->indices);
        shadekernel.SetArg(argc++, m_scene_data->shapes);
        shadekernel.SetArg(argc++, m_scene_data->materialids);
        shadekernel.SetArg(argc++, m_scene_data->materials);
        shadekernel.SetArg(argc++, m_scene_data->textures);
        shadekernel.SetArg(argc++, m_scene_data->texturedata);
        shadekernel.SetArg(argc++, scene.envidx_);
        shadekernel.SetArg(argc++, scene.envmapmul_);
        shadekernel.SetArg(argc++, m_scene_data->emissives);
        shadekernel.SetArg(argc++, m_scene_data->numemissive);
        shadekernel.SetArg(argc++, rand_uint());
        shadekernel.SetArg(argc++, m_render_data->samplers);
        shadekernel.SetArg(argc++, m_render_data->sobolmat);
        shadekernel.SetArg(argc++, pass);
        shadekernel.SetArg(argc++, m_scene_data->volumes);
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

    void PtRenderer::ShadeVolume(Scene const& scene, int pass)
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
        shadekernel.SetArg(argc++, m_scene_data->vertices);
        shadekernel.SetArg(argc++, m_scene_data->normals);
        shadekernel.SetArg(argc++, m_scene_data->uvs);
        shadekernel.SetArg(argc++, m_scene_data->indices);
        shadekernel.SetArg(argc++, m_scene_data->shapes);
        shadekernel.SetArg(argc++, m_scene_data->materialids);
        shadekernel.SetArg(argc++, m_scene_data->materials);
        shadekernel.SetArg(argc++, m_scene_data->textures);
        shadekernel.SetArg(argc++, m_scene_data->texturedata);
        shadekernel.SetArg(argc++, scene.envidx_);
        shadekernel.SetArg(argc++, scene.envmapmul_);
        shadekernel.SetArg(argc++, m_scene_data->emissives);
        shadekernel.SetArg(argc++, m_scene_data->numemissive);
        shadekernel.SetArg(argc++, rand_uint());
        shadekernel.SetArg(argc++, m_render_data->samplers);
        shadekernel.SetArg(argc++, m_render_data->sobolmat);
        shadekernel.SetArg(argc++, pass);
        shadekernel.SetArg(argc++, m_scene_data->volumes);
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

    void PtRenderer::EvaluateVolume(Scene const& scene, int pass)
    {
        // Fetch kernel
        CLWKernel evalkernel = m_render_data->program.GetKernel("EvaluateVolume");

        // Set kernel parameters
        int argc = 0;
        evalkernel.SetArg(argc++, m_render_data->rays[pass & 0x1]);
        evalkernel.SetArg(argc++, m_render_data->pixelindices[(pass + 1) & 0x1]);
        evalkernel.SetArg(argc++, m_render_data->hitcount);
        evalkernel.SetArg(argc++, m_scene_data->volumes);
        evalkernel.SetArg(argc++, m_scene_data->textures);
        evalkernel.SetArg(argc++, m_scene_data->texturedata);
        evalkernel.SetArg(argc++, rand_uint());
        evalkernel.SetArg(argc++, m_render_data->samplers);
        evalkernel.SetArg(argc++, m_render_data->sobolmat);
        evalkernel.SetArg(argc++, pass);
        evalkernel.SetArg(argc++, m_render_data->intersections);
        evalkernel.SetArg(argc++, m_render_data->paths);
        evalkernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, evalkernel);
        }
    }

    void PtRenderer::ShadeMiss(Scene const& scene, int pass)
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
        misskernel.SetArg(argc++, m_scene_data->textures);
        misskernel.SetArg(argc++, m_scene_data->texturedata);
        misskernel.SetArg(argc++, scene.envidx_);
        misskernel.SetArg(argc++, m_render_data->paths);
        misskernel.SetArg(argc++, m_scene_data->volumes);
        misskernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, misskernel);
        }
    }

    void PtRenderer::GatherLightSamples(Scene const& scene, int pass)
    {
        // Fetch kernel
        CLWKernel gatherkernel = m_render_data->program.GetKernel("GatherLightSamples");

        // Set kernel parameters
        int argc = 0;
        gatherkernel.SetArg(argc++, m_render_data->pixelindices[pass & 0x1]);
        gatherkernel.SetArg(argc++, m_render_data->hitcount);
        gatherkernel.SetArg(argc++, m_render_data->shadowhits);
        gatherkernel.SetArg(argc++, m_render_data->lightsamples);
        gatherkernel.SetArg(argc++, m_render_data->paths);
        gatherkernel.SetArg(argc++, m_output->data());

        // Run shading kernel
        {
            int globalsize = m_output->width() * m_output->height();
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gatherkernel);
        }
    }

    void PtRenderer::RestorePixelIndices(int pass)
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

    void PtRenderer::FilterPathStream(int pass)
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

    void PtRenderer::CompileScene(Scene const& scene)
    {
        m_vidmemusage = 0;

        // Vertex, normal and uv data
        m_scene_data->vertices = m_context.CreateBuffer<float3>(scene.vertices_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.vertices_[0]);
        m_vidmemusage += scene.vertices_.size() * sizeof(float3);

        m_scene_data->normals = m_context.CreateBuffer<float3>(scene.normals_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.normals_[0]);
        m_vidmemusage += scene.normals_.size() * sizeof(float3);

        m_scene_data->uvs = m_context.CreateBuffer<float2>(scene.uvs_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.uvs_[0]);
        m_vidmemusage += scene.uvs_.size() * sizeof(float2);

        // Index data
        m_scene_data->indices = m_context.CreateBuffer<int>(scene.indices_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.indices_[0]);
        m_vidmemusage += scene.indices_.size() * sizeof(int);

        // Shapes
        m_scene_data->shapes = m_context.CreateBuffer<Scene::Shape>(scene.shapes_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.shapes_[0]);
        m_vidmemusage += scene.shapes_.size() * sizeof(Scene::Shape);

        // Material IDs
        m_scene_data->materialids = m_context.CreateBuffer<int>(scene.materialids_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.materialids_[0]);
        m_vidmemusage += scene.materialids_.size() * sizeof(int);

        // Material descriptions
        m_scene_data->materials = m_context.CreateBuffer<Scene::Material>(scene.materials_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.materials_[0]);
        m_vidmemusage += scene.materials_.size() * sizeof(Scene::Material);

        // Bake textures
        BakeTextures(scene);

        // Emissives
        if (scene.emissives_.size() > 0)
        {
            m_scene_data->emissives = m_context.CreateBuffer<Scene::Emissive>(scene.emissives_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.emissives_[0]);
            m_scene_data->numemissive = scene.emissives_.size();
            m_vidmemusage += scene.emissives_.size() * sizeof(Scene::Emissive);
        }
        else
        {
            m_scene_data->emissives = m_context.CreateBuffer<Scene::Emissive>(1, CL_MEM_READ_ONLY);
            m_scene_data->numemissive = 0;
            m_vidmemusage += sizeof(Scene::Emissive);
        }

        std::cout << "Vidmem usage (data): " << m_vidmemusage / (1024 * 1024) << "Mb\n";
        std::cout << "Polygon count " << scene.indices_.size() / 3 << "\n";
    }

    void PtRenderer::BakeTextures(Scene const& scene)
    {
        if (scene.textures_.size() > 0)
        {
            // Evaluate data size
            size_t datasize = 0;
            for (auto iter = scene.textures_.cbegin(); iter != scene.textures_.cend(); ++iter)
            {
                datasize += iter->size;
            }

            // Texture descriptors
            m_scene_data->textures = m_context.CreateBuffer<Scene::Texture>(scene.textures_.size(), CL_MEM_READ_ONLY);
            m_vidmemusage += scene.textures_.size() * sizeof(Scene::Texture);

            // Texture data
            m_scene_data->texturedata = m_context.CreateBuffer<char>(datasize, CL_MEM_READ_ONLY);

            // Map both buffers
            Scene::Texture* mappeddesc = nullptr;
            char* mappeddata = nullptr;
            Scene::Texture* mappeddesc_orig = nullptr;
            char* mappeddata_orig = nullptr;

            m_context.MapBuffer(0, m_scene_data->textures, CL_MAP_WRITE, &mappeddesc).Wait();
            m_context.MapBuffer(0, m_scene_data->texturedata, CL_MAP_WRITE, &mappeddata).Wait();

            // Save them for unmap
            mappeddesc_orig = mappeddesc;
            mappeddata_orig = mappeddata;

            // Write data into both buffers
            int current_offset = 0;
            for (auto iter = scene.textures_.cbegin(); iter != scene.textures_.cend(); ++iter)
            {
                // Copy texture desc
                Scene::Texture texture = *iter;

                // Write data into the buffer
                memcpy(mappeddata, scene.texturedata_[texture.dataoffset].get(), texture.size);
                m_vidmemusage += texture.size;

                // Adjust offset in the texture
                texture.dataoffset = current_offset;

                // Copy desc into the buffer
                *mappeddesc = texture;

                // Adjust offset
                current_offset += texture.size;

                // Adjust data pointer
                mappeddata += texture.size;

                // Adjust descriptor pointer
                ++mappeddesc;
            }

            m_context.UnmapBuffer(0, m_scene_data->textures, mappeddesc_orig).Wait();
            m_context.UnmapBuffer(0, m_scene_data->texturedata, mappeddata_orig).Wait();
        }
        else
        {
            // Create stub
            m_scene_data->textures = m_context.CreateBuffer<Scene::Texture>(1, CL_MEM_READ_ONLY);
            m_vidmemusage += sizeof(Scene::Texture);

            // Texture data
            m_scene_data->texturedata = m_context.CreateBuffer<char>(1, CL_MEM_READ_ONLY);
            m_vidmemusage += 1;
        }
    }

    CLWKernel PtRenderer::GetCopyKernel()
    {
        return m_render_data->program.GetKernel("ApplyGammaAndCopyData");
    }

    CLWKernel PtRenderer::GetAccumulateKernel()
    {
        return m_render_data->program.GetKernel("AccumulateData");
    }
}
