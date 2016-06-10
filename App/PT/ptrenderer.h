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

#include "CLW.h"
#include "firerays_cl.h"

namespace Baikal
{
    class ClwOutput;

    ///< Renderer implementation
    class PtRenderer : public Renderer
    {
    public:
        // Constructor
        PtRenderer(CLWContext context, int devidx);
        // Destructor
        ~PtRenderer();

        // Renderer overrides
        // Create output
        Output* CreateOutput(std::uint32_t w, std::uint32_t h) const override;
        // Delete output
        void DeleteOutput(Output* output) const override;
        // Clear output
        void Clear(FireRays::float3 const& val, Output& output) const override;
        // Do necessary precalculation and initialization
        void Preprocess(Scene const& scene) override;
        // Render the scene into the output
        void Render(Scene const& scene) override;
        // Set output
        void SetOutput(Output* output) override;


        // Interop function
        CLWKernel GetCopyKernel();
        // Add function
        CLWKernel GetAccumulateKernel();

    protected:
        // Resize output-dependent buffers
        void ResizeWorkingSet(Output const& output);
        // Create buffers for shading part
        void CompileScene(Scene const& scene);
        // Generate rays
        void GeneratePrimaryRays();
        // Shade first hit
        void ShadeSurface(Scene const& scene, int pass);
        // Evaluate volume
        void EvaluateVolume(Scene const& scene, int pass);
        // Handle missing rays
        void ShadeMiss(Scene const& scene, int pass);
        // Gather light samples and account for visibility
        void GatherLightSamples(Scene const& scene, int pass);
        // Restore pixel indices after compaction
        void RestorePixelIndices(int pass);
        // Pack textures for GPU
        void BakeTextures(Scene const& scene);
        // Convert intersection info to compaction predicate
        void FilterPathStream(int pass);
        // Integrate volume
        void ShadeVolume(Scene const& scene, int pass);


    public:
        // Intersection API
        FireRays::IntersectionApiCL* m_api;
        // CL context
        CLWContext m_context;
        // Output object
        ClwOutput* m_output;
        // Flag to reset the sampler
        mutable bool m_resetsampler;

        // GPU data
        struct QmcSampler;
        struct PathState;

        struct Volume;
        struct RenderData;
        struct SceneData;
        std::unique_ptr<RenderData> m_render_data;
        std::unique_ptr<SceneData> m_scene_data;

        // Intersector data
        std::vector<FireRays::Shape*> m_shapes;

        // Vidmem usage
        // Data
        size_t m_vidmemusage;
        // Working set
        size_t m_vidmemws;
    };

}
