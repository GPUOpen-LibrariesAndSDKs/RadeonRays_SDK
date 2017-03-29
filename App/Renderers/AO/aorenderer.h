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
#include "CLW/clwscene.h"
#include "CLW/clw_scene_controller.h"

#include "CLW.h"

namespace Baikal
{
    class ClwOutput;
    struct ClwScene;

    ///< Renderer implementation
    class AoRenderer : public Renderer
    {
    public:
        // Constructor
        AoRenderer(CLWContext context, int devidx);
        // Destructor
        ~AoRenderer();

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
        // Interop function
        CLWKernel GetCopyKernel();
        // Add function
        CLWKernel GetAccumulateKernel();

    protected:
        // Resize output-dependent buffers
        void ResizeWorkingSet(Output const& output);
        // Generate rays
        void GeneratePrimaryRays(ClwScene const& scene);
        // Shade first hit
        void SampleOcclusion(ClwScene const& scene);
        // Gather light samples and account for visibility
        void GatherOcclusion(ClwScene const& scene);
        // Restore pixel indices after compaction
        void RestorePixelIndices();
        // Convert intersection info to compaction predicate
        void FilterPathStream();

    public:
        // CL context
        CLWContext m_context;
        // Output object
        ClwOutput* m_output;
        // Flag to reset the sampler
        mutable bool m_resetsampler;
        // Scene tracker
        ClwSceneController m_scene_controller;

        // GPU data
        struct QmcSampler;
        struct RenderData;

        std::unique_ptr<RenderData> m_render_data;

        // Intersector data
        std::vector<RadeonRays::Shape*> m_shapes;

        // Vidmem usage
        // Working set
        size_t m_vidmemws;
    };

}
