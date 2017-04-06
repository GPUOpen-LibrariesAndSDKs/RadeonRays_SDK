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
#pragma once

#include "Core/renderer.h"
#include "Core/output.h"
#include "gl_scene_controller.h"
#include "Utils/shader_manager.h"

#include <memory>
#include <vector>

namespace Baikal
{
    class Output;
    class PtRenderer;

    ///< Renderer implementation
    class RsRenderer : public Renderer
    {
    public:
        // Constructor
        RsRenderer();
        // Destructor
        ~RsRenderer();

        // Renderer overrides
        // Create output
        Output* CreateOutput(std::uint32_t w, std::uint32_t h) const override;
        // Delete output
        void DeleteOutput(Output* output) const override;
        // Clear output
        void Clear(RadeonRays::float3 const& val, Output& output) const override;
        // Do necessary precalculation and initialization
        void Preprocess(Scene1 const& scene) override;
        // Render the scene into the output
        void Render(Scene1 const& scene) override;
        // Set output
        void SetOutput(Output* output) override;
        // Run render benchmark
        void RunBenchmark(Scene1 const& scene, std::uint32_t num_passes, BenchmarkStats& stats) override;

    private:
        void RenderBackground(GlScene const& scene);
        void ResolveGI(GlScene const& scene, int radius);

    public:
        // Scene tracker
        GlSceneController m_scene_controller;

        struct RenderData;
        std::unique_ptr<RenderData> m_render_data;

        Output* m_output;
        ShaderManager m_shader_manager;

    private:
        mutable std::uint32_t m_framecnt;
    };

    class GlOutput : public Output
    {
    public:
        GlOutput(std::uint32_t w, std::uint32_t h);
        ~GlOutput();

        void GetData(RadeonRays::float3* data) const;

        void Clear(RadeonRays::float3 const& val);

        GLuint GetGlFramebuffer() const;

    private:
        GLuint m_frame_buffer;
        GLuint m_depth_buffer;
        GLuint m_color_buffer;

        std::uint32_t m_width;
        std::uint32_t m_height;
    };

}
