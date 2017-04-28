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
#include "bdptrenderer.h"
#include "CLW/clwoutput.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/Collector/collector.h"

#include <numeric>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <random>
#include <algorithm>

#include "Utils/sobol.h"

#ifdef RR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

namespace Baikal
{
    using namespace RadeonRays;

    int constexpr kMaxLightSamples = 1;
    int constexpr kTileSizeX = 1024;
    int constexpr kTileSizeY = 1024;

    int constexpr kMaxRandomWalkLength = 3;

    // Path state description
    struct BdptRenderer::PathState
    {
        float4 throughput;
        int volume;
        int flags;
        int extra0;
        int extra1;
    };

    // BDPT integrator keeps path vertices around,
    // so it needs a structure to describe the vertex.
    struct BdptRenderer::PathVertex
    {
        // Vertex position
        float3 position;
        // Shading normal
        float3 shading_normal;
        // True normal
        float3 geometric_normal;
        // UV coordinates
        float2 uv;
        // Pdf of sampling this vertex 
        // using forward transfer
        float pdf_forward;
        // Pdf of sampling this vertex
        // using backward transfer
        float pdf_backward;
        // Value flow 
        //(importance or radiance)
        float3 flow;
        float3 unused;
        int type;
        int material_index;
        int flags;
        int padding;
    };

    struct BdptRenderer::RenderData
    {
        // OpenCL stuff
        // Ray buffers for an intersector
        CLWBuffer<ray> rays[2];
        // Hit predicates
        CLWBuffer<int> hits;
        // Path data
        CLWBuffer<PathState> paths;

        // Shadow rays buffer
        CLWBuffer<ray> shadowrays;
        // Shadow hits (occluison)
        CLWBuffer<int> shadowhits;

        // Intersection data
        CLWBuffer<Intersection> intersections;
        // Compacted ray stream indices
        CLWBuffer<int> compacted_indices;
        // Pixel indices
        CLWBuffer<int> pixelindices[2];
        // Constant range buffer
        CLWBuffer<int> iota;

        // Light samples collected 
        // after surface shading
        CLWBuffer<float3> lightsamples;
        // Image plane coordinates for splatting
        CLWBuffer<float2> image_plane_positions;
        // RNG data
        CLWBuffer<std::uint32_t> random;
        CLWBuffer<std::uint32_t> sobolmat;
        // Hit counter
        CLWBuffer<int> hitcount;

        // Vertices stored during
        // random walk from the eye
        CLWBuffer<PathVertex> eye_subpath;
        // Vertices stored during
        // random walk from the light
        CLWBuffer<PathVertex> light_subpath;
        // Number of vertices in
        // the corresponding subpaths
        CLWBuffer<int> eye_subpath_length;
        CLWBuffer<int> light_subpath_length;

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
        , m_render_data(new RenderData)
        , m_vidmemws(0)
        , m_scene_controller(context, devidx)
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
        m_render_data->program = CLWProgram::CreateFromSource(cl_app, std::strlen(cl_integrator_bdpt), buildopts.c_str(), context);
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

    void BdptRenderer::GenerateCameraVertices(ClwScene const& scene, Output const& output, int2 const& tile_size)
    {
        // Fetch kernel
        std::string kernel_name = (scene.camera_type == CameraType::kDefault) ? "PerspectiveCamera_GenerateVertices" : "PerspectiveCameraDof_GenerateVertices";

        CLWKernel genkernel = m_render_data->program.GetKernel(kernel_name);

        // Set kernel parameters
        int argc = 0;
        genkernel.SetArg(argc++, scene.camera);
        genkernel.SetArg(argc++, output.width());
        genkernel.SetArg(argc++, output.height());
        genkernel.SetArg(argc++, m_render_data->pixelindices[0]);
        genkernel.SetArg(argc++, m_render_data->hitcount);
        genkernel.SetArg(argc++, (int)rand_uint());
        genkernel.SetArg(argc++, m_framecnt);
        genkernel.SetArg(argc++, m_render_data->random);
        genkernel.SetArg(argc++, m_render_data->sobolmat);
        genkernel.SetArg(argc++, m_render_data->rays[0]);
        genkernel.SetArg(argc++, m_render_data->eye_subpath);
        genkernel.SetArg(argc++, m_render_data->eye_subpath_length);
        genkernel.SetArg(argc++, m_render_data->paths);

        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, genkernel);
        }
    }

    void BdptRenderer::GenerateLightVertices(ClwScene const& scene, Output const& output, int2 const& tile_size)
    {
        // Fetch kernel
        CLWKernel genkernel = m_render_data->program.GetKernel("GenerateLightVertices");

        // Set kernel parameters
        int num_rays = tile_size.x * tile_size.y;

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
        genkernel.SetArg(argc++, scene.lights);
        genkernel.SetArg(argc++, scene.num_lights);
        genkernel.SetArg(argc++, (int)rand_uint());
        genkernel.SetArg(argc++, m_framecnt);
        genkernel.SetArg(argc++, m_render_data->random);
        genkernel.SetArg(argc++, m_render_data->sobolmat);
        genkernel.SetArg(argc++, m_render_data->rays[0]);
        genkernel.SetArg(argc++, m_render_data->light_subpath);
        genkernel.SetArg(argc++, m_render_data->light_subpath_length);
        genkernel.SetArg(argc++, m_render_data->paths);

        // Run generation kernel
        {
            int globalsize = num_rays;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, genkernel);
        }
    }

    void BdptRenderer::Render(Scene1 const& scene)
    {
        auto output = FindFirstNonZeroOutput();
        auto output_size = int2(output->width(), output->height());

        if (output_size.x > kTileSizeX || output_size.y > kTileSizeY)
        {
            auto num_tiles_x = (output_size.x + kTileSizeX - 1) / kTileSizeX;
            auto num_tiles_y = (output_size.y + kTileSizeY - 1) / kTileSizeY;

            for (auto x = 0; x < num_tiles_x; ++x)
                for (auto y = 0; y < num_tiles_y; ++y)
                {
                    auto tile_offset = int2(x * kTileSizeX, y * kTileSizeY);
                    auto tile_size = int2(std::min(kTileSizeX, output_size.x - tile_offset.x),
                        std::min(kTileSizeY, output_size.y - tile_offset.y));

                    RenderTile(scene, tile_offset, tile_size);
                }
        }
        else
        {
            RenderTile(scene, int2(), output_size);
        }
    }

    // Render the scene into the output
    void BdptRenderer::RenderTile(Scene1 const& scene, int2 const& tile_origin, int2 const& tile_size)
    {
        auto api = m_scene_controller.GetIntersectionApi();
        auto& clwscene = m_scene_controller.CompileScene(scene, m_render_data->mat_collector, m_render_data->tex_collector);

        // Number of rays to generate
        auto output = GetOutput(OutputType::kColor);

        if (output)
        {
            auto num_rays = tile_size.x * tile_size.y;
            auto output_size = int2(output->width(), output->height());

            // Generate tile domain
            GenerateTileDomain(output_size, tile_origin, tile_size, tile_size);
            // Generate camera vertices into eye_subpath array
            GenerateCameraVertices(clwscene, *output, tile_size);

            //std::vector<ray> rays(num_rays);
            //m_context.ReadBuffer(0, m_render_data->rays[0], &rays[0], num_rays).Wait();

            // Perform random walk from camera
            RandomWalk(clwscene, num_rays, m_render_data->eye_subpath, m_render_data->eye_subpath_length, 0, tile_size);

            //std::vector<PathVertex> eye_vertices(num_rays);
            //std::vector<int> eye_length(num_rays);
            //m_context.ReadBuffer(0, m_render_data->eye_subpath, &eye_vertices[0], num_rays).Wait();
            //m_context.ReadBuffer(0, m_render_data->eye_subpath_length, &eye_length[0], num_rays).Wait();

            // Generate light vertices into light_subpath array
            GenerateLightVertices(clwscene, *output, tile_size);

            // Need to update pixel indices 
            m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[0], 0, 0, num_rays);
            m_context.CopyBuffer(0, m_render_data->iota, m_render_data->pixelindices[1], 0, 0, num_rays);
            m_context.FillBuffer(0, m_render_data->hitcount, num_rays, 1);

            // Perform random walk from camera
            RandomWalk(clwscene, num_rays, m_render_data->light_subpath, m_render_data->light_subpath_length, 0, tile_size);


            // Generate tile domain
            GenerateTileDomain(output_size, tile_origin, tile_size, tile_size);
            std::vector<PathVertex> light_vertices(num_rays);
            std::vector<int> light_length(num_rays);
            m_context.ReadBuffer(0, m_render_data->light_subpath, &light_vertices[0], num_rays).Wait();
            m_context.ReadBuffer(0, m_render_data->light_subpath_length, &light_length[0], num_rays).Wait();

            //for (int c = 1; c < kMaxRandomWalkLength; ++c)
            {
                //ConnectDirect(clwscene, c, tile_size);
            }

            for (int c = 1; c < kMaxRandomWalkLength; ++c)
            {
                for (int l = 1; l < kMaxRandomWalkLength; ++l)
                {
                    Connect(clwscene, c, l, tile_size);
                }
            }

            //for (int l = 1; l < 2; ++l)
            //{
            //    ConnectCaustic(clwscene, l, tile_size);
            //}

            IncrementSampleCounter(clwscene, tile_size);

            m_context.Finish(0);
        }

        // Check if we have other outputs, than color
        bool aov_pass_needed = (FindFirstNonZeroOutput(false) != nullptr);
        if (aov_pass_needed)
        {
            FillAOVs(clwscene, tile_origin, tile_size);
            m_context.Flush(0);
        }

        ++m_framecnt;
    }

    void BdptRenderer::IncrementSampleCounter(ClwScene const& scene, int2 const& tile_size)
    {
        int num_rays = tile_size.x * tile_size.y;
        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

        {
            // Fetch kernel
            CLWKernel increment_kernel = m_render_data->program.GetKernel("IncrementSampleCounter");

            int argc = 0;
            increment_kernel.SetArg(argc++, num_rays);
            increment_kernel.SetArg(argc++, m_render_data->pixelindices[0]);
            increment_kernel.SetArg(argc++, output->data());

            // Run generation kernel
            {
                int globalsize = num_rays;
                m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, increment_kernel);
            }
        }
    }

    void BdptRenderer::Connect(ClwScene const& scene, int eye_vertex_index, int light_vertex_index, int2 const& tile_size)
    {
        m_context.FillBuffer(0, m_render_data->shadowhits, 1, m_render_data->shadowhits.GetElementCount()).Wait();


        int num_rays = tile_size.x * tile_size.y;
        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

        {
            // Fetch kernel
            CLWKernel connect_kernel = m_render_data->program.GetKernel("Connect");

            int argc = 0;
            connect_kernel.SetArg(argc++, num_rays);
            connect_kernel.SetArg(argc++, eye_vertex_index);
            connect_kernel.SetArg(argc++, light_vertex_index);
            connect_kernel.SetArg(argc++, m_render_data->pixelindices[0]);
            connect_kernel.SetArg(argc++, m_render_data->eye_subpath);
            connect_kernel.SetArg(argc++, m_render_data->eye_subpath_length);
            connect_kernel.SetArg(argc++, m_render_data->light_subpath);
            connect_kernel.SetArg(argc++, m_render_data->light_subpath_length);
            connect_kernel.SetArg(argc++, scene.materials);
            connect_kernel.SetArg(argc++, scene.textures);
            connect_kernel.SetArg(argc++, scene.texturedata);
            connect_kernel.SetArg(argc++, m_render_data->shadowrays);
            connect_kernel.SetArg(argc++, m_render_data->lightsamples);

            // Run generation kernel
            {
                int globalsize = num_rays;
                m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, connect_kernel);
            }
        }

        m_scene_controller.GetIntersectionApi()
            ->QueryOcclusion(m_render_data->fr_shadowrays, num_rays, m_render_data->fr_shadowhits, nullptr, nullptr);

        {
            CLWKernel gather_kernel = m_render_data->program.GetKernel("GatherContributions");

            int argc = 0;
            gather_kernel.SetArg(argc++, num_rays);
            gather_kernel.SetArg(argc++, m_render_data->pixelindices[0]);
            gather_kernel.SetArg(argc++, m_render_data->shadowhits);
            gather_kernel.SetArg(argc++, m_render_data->lightsamples);
            gather_kernel.SetArg(argc++, output->data());

            // Run generation kernel
            {
                int globalsize = num_rays;
                m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gather_kernel);
            }
        }
    }

    void BdptRenderer::ConnectDirect(ClwScene const& scene, int eye_vertex_index, int2 const& tile_size)
    {
        m_context.FillBuffer(0, m_render_data->shadowhits, 1, m_render_data->shadowhits.GetElementCount());


        int num_rays = tile_size.x * tile_size.y;
        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

        {
            // Fetch kernel
            CLWKernel connect_kernel = m_render_data->program.GetKernel("ConnectDirect");

            int argc = 0;
            connect_kernel.SetArg(argc++, eye_vertex_index);
            connect_kernel.SetArg(argc++, num_rays);
            connect_kernel.SetArg(argc++, m_render_data->pixelindices[0]);
            connect_kernel.SetArg(argc++, m_render_data->eye_subpath);
            connect_kernel.SetArg(argc++, m_render_data->eye_subpath_length);
            connect_kernel.SetArg(argc++, scene.vertices);
            connect_kernel.SetArg(argc++, scene.normals);
            connect_kernel.SetArg(argc++, scene.uvs);
            connect_kernel.SetArg(argc++, scene.indices);
            connect_kernel.SetArg(argc++, scene.shapes);
            connect_kernel.SetArg(argc++, scene.materialids);
            connect_kernel.SetArg(argc++, scene.materials);
            connect_kernel.SetArg(argc++, scene.textures);
            connect_kernel.SetArg(argc++, scene.texturedata);
            connect_kernel.SetArg(argc++, scene.envmapidx);
            connect_kernel.SetArg(argc++, scene.lights);
            connect_kernel.SetArg(argc++, scene.num_lights);
            connect_kernel.SetArg(argc++, rand_uint());
            connect_kernel.SetArg(argc++, m_render_data->random);
            connect_kernel.SetArg(argc++, m_render_data->sobolmat);
            connect_kernel.SetArg(argc++, m_framecnt);
            connect_kernel.SetArg(argc++, m_render_data->shadowrays);
            connect_kernel.SetArg(argc++, m_render_data->lightsamples);

            // Run generation kernel
            {
                int globalsize = num_rays;
                m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, connect_kernel);
            }
        }

        m_scene_controller.GetIntersectionApi()
            ->QueryOcclusion(m_render_data->fr_shadowrays, num_rays, m_render_data->fr_shadowhits, nullptr, nullptr);


        {
            CLWKernel gather_kernel = m_render_data->program.GetKernel("GatherContributions");

            int argc = 0;
            gather_kernel.SetArg(argc++, num_rays);
            gather_kernel.SetArg(argc++, m_render_data->pixelindices[0]);
            gather_kernel.SetArg(argc++, m_render_data->shadowhits);
            gather_kernel.SetArg(argc++, m_render_data->lightsamples);
            gather_kernel.SetArg(argc++, output->data());

            // Run generation kernel
            {
                int globalsize = num_rays;
                m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gather_kernel);
            }
        }
    }

    void BdptRenderer::ConnectCaustic(ClwScene const& scene, int light_vertex_index, int2 const& tile_size)
    {
        m_context.FillBuffer(0, m_render_data->shadowhits, 1, m_render_data->shadowhits.GetElementCount());


        int num_rays = tile_size.x * tile_size.y;
        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

        {
            // Fetch kernel
            CLWKernel connect_kernel = m_render_data->program.GetKernel("ConnectCaustics");

            int argc = 0;
            connect_kernel.SetArg(argc++, num_rays);
            connect_kernel.SetArg(argc++, light_vertex_index);
            connect_kernel.SetArg(argc++, m_render_data->light_subpath);
            connect_kernel.SetArg(argc++, m_render_data->light_subpath_length);
            connect_kernel.SetArg(argc++, scene.camera);
            connect_kernel.SetArg(argc++, scene.materials);
            connect_kernel.SetArg(argc++, scene.textures);
            connect_kernel.SetArg(argc++, scene.texturedata);
            connect_kernel.SetArg(argc++, m_render_data->shadowrays);
            connect_kernel.SetArg(argc++, m_render_data->lightsamples);
            connect_kernel.SetArg(argc++, m_render_data->image_plane_positions);

            // Run generation kernel
            {
                int globalsize = num_rays;
                m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, connect_kernel);
            }
        }

        m_scene_controller.GetIntersectionApi()
            ->QueryOcclusion(m_render_data->fr_shadowrays, num_rays, m_render_data->fr_shadowhits, nullptr, nullptr);


        {
            CLWKernel gather_kernel = m_render_data->program.GetKernel("GatherCausticContributions");

            int argc = 0;
            gather_kernel.SetArg(argc++, num_rays);
            gather_kernel.SetArg(argc++, output->width());
            gather_kernel.SetArg(argc++, output->height());
            gather_kernel.SetArg(argc++, m_render_data->shadowhits);
            gather_kernel.SetArg(argc++, m_render_data->lightsamples);
            gather_kernel.SetArg(argc++, m_render_data->image_plane_positions);
            gather_kernel.SetArg(argc++, output->data());

            // Run generation kernel
            {
                int globalsize = num_rays;
                m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gather_kernel);
            }
        }
    }

    void BdptRenderer::GenerateTileDomain(int2 const& output_size, int2 const& tile_origin,
        int2 const& tile_size, int2 const& subtile_size)
    {
        // Fetch kernel
        CLWKernel generate_kernel = m_render_data->program.GetKernel("GenerateTileDomain");

        // Set kernel parameters
        int argc = 0;
        generate_kernel.SetArg(argc++, output_size.x);
        generate_kernel.SetArg(argc++, output_size.y);
        generate_kernel.SetArg(argc++, tile_origin.x);
        generate_kernel.SetArg(argc++, tile_origin.y);
        generate_kernel.SetArg(argc++, tile_size.x);
        generate_kernel.SetArg(argc++, tile_size.y);
        generate_kernel.SetArg(argc++, subtile_size.x);
        generate_kernel.SetArg(argc++, subtile_size.y);
        generate_kernel.SetArg(argc++, m_render_data->pixelindices[0]);
        generate_kernel.SetArg(argc++, m_render_data->pixelindices[1]);
        generate_kernel.SetArg(argc++, m_render_data->hitcount);

        // Run shading kernel
        {
            size_t gs[] = { static_cast<size_t>((tile_size.x + 7) / 8 * 8), static_cast<size_t>((tile_size.y + 7) / 8 * 8) };
            size_t ls[] = { 8, 8 };

            m_context.Launch2D(0, gs, ls, generate_kernel);
        }
    }

    void BdptRenderer::RandomWalk(ClwScene const& scene, int num_rays, CLWBuffer<PathVertex> subpath, CLWBuffer<int> subpath_length, int mode, int2 const& tile_size)
    {
        auto api = m_scene_controller.GetIntersectionApi();

        // Initialize first pass
        for (int pass = 0; pass < kMaxRandomWalkLength; ++pass)
        {
            // Clear ray hits buffer
            m_context.FillBuffer(0, m_render_data->hits, 0, m_render_data->hits.GetElementCount());

            // Intersect ray batch
            api->QueryIntersection(m_render_data->fr_rays[pass & 0x1], m_render_data->fr_hitcount, num_rays, m_render_data->fr_intersections, nullptr, nullptr);

            // Convert intersections to predicates
            FilterPathStream(pass, tile_size);

            // Compact batch
            m_render_data->pp.Compact(0, m_render_data->hits, m_render_data->iota, m_render_data->compacted_indices, num_rays, m_render_data->hitcount);

            //int hitcount = 0;
            //m_context.ReadBuffer(0, m_render_data->hitcount, &hitcount, 1).Wait();

            //std::vector<int> indices(num_rays);
            //m_context.ReadBuffer(0, m_render_data->pixelindices[0], &indices[0], num_rays).Wait();


            // Advance indices to keep pixel indices up to date
            RestorePixelIndices(pass, tile_size);

            //m_context.ReadBuffer(0, m_render_data->pixelindices[0], &indices[0], num_rays).Wait();

            // Shade hits
            SampleSurface(scene, pass, subpath, subpath_length, mode, tile_size);

            //
            m_context.Flush(0);
        }
    }

    void BdptRenderer::SampleSurface(ClwScene const& scene, int pass, CLWBuffer<PathVertex> subpath, CLWBuffer<int> subpath_length, int mode, int2 const& tile_size)
    {
        // Fetch kernel
        CLWKernel sample_kernel = m_render_data->program.GetKernel("SampleSurface");

        // Set kernel parameters
        int argc = 0;
        sample_kernel.SetArg(argc++, m_render_data->rays[pass & 0x1]);
        sample_kernel.SetArg(argc++, m_render_data->intersections);
        sample_kernel.SetArg(argc++, m_render_data->compacted_indices);
        sample_kernel.SetArg(argc++, m_render_data->pixelindices[pass & 0x1]);
        sample_kernel.SetArg(argc++, m_render_data->hitcount);
        sample_kernel.SetArg(argc++, scene.vertices);
        sample_kernel.SetArg(argc++, scene.normals);
        sample_kernel.SetArg(argc++, scene.uvs);
        sample_kernel.SetArg(argc++, scene.indices);
        sample_kernel.SetArg(argc++, scene.shapes);
        sample_kernel.SetArg(argc++, scene.materialids);
        sample_kernel.SetArg(argc++, scene.materials);
        sample_kernel.SetArg(argc++, scene.textures);
        sample_kernel.SetArg(argc++, scene.texturedata);
        sample_kernel.SetArg(argc++, scene.envmapidx);
        sample_kernel.SetArg(argc++, scene.lights);
        sample_kernel.SetArg(argc++, scene.num_lights);
        sample_kernel.SetArg(argc++, rand_uint());
        sample_kernel.SetArg(argc++, m_render_data->random);
        sample_kernel.SetArg(argc++, m_render_data->sobolmat);
        sample_kernel.SetArg(argc++, pass);
        sample_kernel.SetArg(argc++, m_framecnt);
        sample_kernel.SetArg(argc++, mode);
        sample_kernel.SetArg(argc++, m_render_data->paths);
        sample_kernel.SetArg(argc++, m_render_data->rays[(pass + 1) & 0x1]);
        sample_kernel.SetArg(argc++, subpath);
        sample_kernel.SetArg(argc++, subpath_length);

        // Run shading kernel
        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, sample_kernel);
        }
    }

    Output* BdptRenderer::FindFirstNonZeroOutput(bool include_color) const
    {
        // Find first non-zero output
        auto current_output = include_color ? GetOutput(Renderer::OutputType::kColor) : nullptr;
        if (!current_output)
        {
            for (auto i = 1U; i < static_cast<std::uint32_t>(Renderer::OutputType::kMax); ++i)
            {
                current_output = GetOutput(static_cast<Renderer::OutputType>(i));

                if (current_output)
                {
                    break;
                }
            }
        }

        return current_output;
    }

    void BdptRenderer::SetOutput(OutputType type, Output* output)
    {
        if (output)
        {
            auto required_size = output->width() * output->height();

            if (required_size > m_render_data->rays[0].GetElementCount())
            {
                ResizeWorkingSet(*output);
            }
        }

        Renderer::SetOutput(type, output);
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

        m_render_data->image_plane_positions = m_context.CreateBuffer<float2>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(float2)* kMaxLightSamples;

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

        m_render_data->eye_subpath = m_context.CreateBuffer<PathVertex>(output.width() * output.height() * kMaxRandomWalkLength, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * kMaxRandomWalkLength * sizeof(PathVertex);
        m_render_data->light_subpath = m_context.CreateBuffer<PathVertex>(output.width() * output.height() * kMaxRandomWalkLength, CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * kMaxRandomWalkLength * sizeof(PathVertex);

        m_render_data->eye_subpath_length = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);
        m_render_data->light_subpath_length = m_context.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
        m_vidmemws += output.width() * output.height() * sizeof(int);

        auto api = m_scene_controller.GetIntersectionApi();

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

    void BdptRenderer::FillAOVs(ClwScene const& scene, int2 const& tile_origin, int2 const& tile_size)
    {
        auto api = m_scene_controller.GetIntersectionApi();

        // Find first non-zero AOV to get buffer dimensions
        auto output = FindFirstNonZeroOutput();
        auto output_size = int2(output->width(), output->height());

        // Generate tile domain
        GenerateTileDomain(output_size, tile_origin, tile_size, tile_size);

        // Generate primary
        GeneratePrimaryRays(scene, *output, tile_size);

        auto num_rays = tile_size.x * tile_size.y;

        // Intersect ray batch
        api->QueryIntersection(m_render_data->fr_rays[0], m_render_data->fr_hitcount, num_rays, m_render_data->fr_intersections, nullptr, nullptr);

        CLWKernel fill_kernel = m_render_data->program.GetKernel("FillAOVs");

        auto argc = 0U;
        fill_kernel.SetArg(argc++, m_render_data->rays[0]);
        fill_kernel.SetArg(argc++, m_render_data->intersections);
        fill_kernel.SetArg(argc++, m_render_data->pixelindices[0]);
        fill_kernel.SetArg(argc++, m_render_data->hitcount);
        fill_kernel.SetArg(argc++, scene.vertices);
        fill_kernel.SetArg(argc++, scene.normals);
        fill_kernel.SetArg(argc++, scene.uvs);
        fill_kernel.SetArg(argc++, scene.indices);
        fill_kernel.SetArg(argc++, scene.shapes);
        fill_kernel.SetArg(argc++, scene.materialids);
        fill_kernel.SetArg(argc++, scene.materials);
        fill_kernel.SetArg(argc++, scene.textures);
        fill_kernel.SetArg(argc++, scene.texturedata);
        fill_kernel.SetArg(argc++, scene.envmapidx);
        fill_kernel.SetArg(argc++, scene.lights);
        fill_kernel.SetArg(argc++, scene.num_lights);
        fill_kernel.SetArg(argc++, rand_uint());
        fill_kernel.SetArg(argc++, m_render_data->random);
        fill_kernel.SetArg(argc++, m_render_data->sobolmat);
        fill_kernel.SetArg(argc++, m_framecnt);
        for (auto i = 1U; i < static_cast<std::uint32_t>(Renderer::OutputType::kMax); ++i)
        {
            if (auto aov = static_cast<ClwOutput*>(GetOutput(static_cast<Renderer::OutputType>(i))))
            {
                fill_kernel.SetArg(argc++, 1);
                fill_kernel.SetArg(argc++, aov->data());
            }
            else
            {
                fill_kernel.SetArg(argc++, 0);
                // This is simply a dummy buffer
                fill_kernel.SetArg(argc++, m_render_data->hitcount);
            }
        }

        // Run AOV kernel
        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, fill_kernel);
        }
    }


    void BdptRenderer::GeneratePrimaryRays(ClwScene const& scene, Output const& output, int2 const& tile_size)
    {
        // Fetch kernel
        std::string kernel_name = (scene.camera_type == CameraType::kDefault) ? "PerspectiveCamera_GeneratePaths" : "PerspectiveCameraDof_GeneratePaths";

        CLWKernel genkernel = m_render_data->program.GetKernel(kernel_name);

        // Set kernel parameters
        int argc = 0;
        genkernel.SetArg(argc++, scene.camera);
        genkernel.SetArg(argc++, output.width());
        genkernel.SetArg(argc++, output.height());
        genkernel.SetArg(argc++, m_render_data->pixelindices[0]);
        genkernel.SetArg(argc++, m_render_data->hitcount);
        genkernel.SetArg(argc++, (int)rand_uint());
        genkernel.SetArg(argc++, m_framecnt);
        genkernel.SetArg(argc++, m_render_data->rays[0]);
        genkernel.SetArg(argc++, m_render_data->random);
        genkernel.SetArg(argc++, m_render_data->sobolmat);

        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, genkernel);
        }
    }

    void BdptRenderer::ShadeSurface(ClwScene const& scene, int pass, int2 const& tile_size)
    {
        // Fetch kernel
        CLWKernel shadekernel = m_render_data->program.GetKernel("ShadeSurface");

        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

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
        shadekernel.SetArg(argc++, m_render_data->rays[(pass + 1) & 0x1]);
        shadekernel.SetArg(argc++, output->data());

        // Run shading kernel
        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
        }
    }


    void BdptRenderer::ShadeVolume(ClwScene const& scene, int pass, int2 const& tile_size)
    {
        // Fetch kernel
        CLWKernel shadekernel = m_render_data->program.GetKernel("ShadeVolume");

        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

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
        shadekernel.SetArg(argc++, m_render_data->rays[(pass + 1) & 0x1]);
        shadekernel.SetArg(argc++, output->data());

        // Run shading kernel
        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
        }
    }

    void BdptRenderer::EvaluateVolume(ClwScene const& scene, int pass, int2 const& tile_size)
    {
        // Fetch kernel
        CLWKernel evalkernel = m_render_data->program.GetKernel("EvaluateVolume");

        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

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
        evalkernel.SetArg(argc++, output->data());

        // Run shading kernel
        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, evalkernel);
        }
    }


    void BdptRenderer::ShadeBackground(ClwScene const& scene, int pass, int2 const& tile_size)
    {
        // Fetch kernel
        CLWKernel misskernel = m_render_data->program.GetKernel("ShadeBackgroundEnvMap");

        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

        // Set kernel parameters
        int argc = 0;
        misskernel.SetArg(argc++, m_render_data->rays[pass & 0x1]);
        misskernel.SetArg(argc++, m_render_data->intersections);
        misskernel.SetArg(argc++, m_render_data->pixelindices[(pass + 1) & 0x1]);
        misskernel.SetArg(argc++, tile_size.x * tile_size.y);
        misskernel.SetArg(argc++, scene.lights);
        misskernel.SetArg(argc++, scene.envmapidx);
        misskernel.SetArg(argc++, scene.textures);
        misskernel.SetArg(argc++, scene.texturedata);
        misskernel.SetArg(argc++, scene.volumes);
        misskernel.SetArg(argc++, output->data());

        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, misskernel);
        }
    }

    void BdptRenderer::GatherLightSamples(ClwScene const& scene, int pass, int2 const& tile_size)
    {
        // Fetch kernel
        CLWKernel gatherkernel = m_render_data->program.GetKernel("GatherLightSamples");

        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

        // Set kernel parameters
        int argc = 0;
        gatherkernel.SetArg(argc++, m_render_data->pixelindices[pass & 0x1]);
        gatherkernel.SetArg(argc++, m_render_data->hitcount);
        gatherkernel.SetArg(argc++, m_render_data->shadowhits);
        gatherkernel.SetArg(argc++, m_render_data->lightsamples);
        gatherkernel.SetArg(argc++, output->data());

        // Run shading kernel
        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gatherkernel);
        }
    }


    void BdptRenderer::RestorePixelIndices(int pass, int2 const& tile_size)
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
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, restorekernel);
        }
    }

    void BdptRenderer::FilterPathStream(int pass, int2 const& tile_size)
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
            int globalsize = tile_size.x * tile_size.y;
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

    // Shade background
    void BdptRenderer::ShadeMiss(ClwScene const& scene, int pass, int2 const& tile_size)
    {
        // Fetch kernel
        CLWKernel misskernel = m_render_data->program.GetKernel("ShadeMiss");

        auto output = static_cast<ClwOutput*>(GetOutput(OutputType::kColor));

        // Set kernel parameters
        int argc = 0;
        misskernel.SetArg(argc++, m_render_data->rays[pass & 0x1]);
        misskernel.SetArg(argc++, m_render_data->intersections);
        misskernel.SetArg(argc++, m_render_data->pixelindices[(pass + 1) & 0x1]);
        misskernel.SetArg(argc++, m_render_data->hitcount);
        misskernel.SetArg(argc++, scene.lights);
        misskernel.SetArg(argc++, scene.envmapidx);
        misskernel.SetArg(argc++, scene.textures);
        misskernel.SetArg(argc++, scene.texturedata);
        misskernel.SetArg(argc++, scene.volumes);
        misskernel.SetArg(argc++, output->data());

        // Run shading kernel
        {
            int globalsize = tile_size.x * tile_size.y;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, misskernel);
        }
    }

    void BdptRenderer::RunBenchmark(Scene1 const& scene, std::uint32_t num_passes, BenchmarkStats& stats)
    {
    }
}
