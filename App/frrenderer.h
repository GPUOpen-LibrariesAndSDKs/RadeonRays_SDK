//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#ifndef FRRENDERER_H
#define FRRENDERER_H

#include "CLW.h"
#include "renderer.h"
#include "firerays_cl.h"

class FrOutput;

///< Renderer implementation
class FrRenderer : public Renderer
{
public:
    // Constructor
    FrRenderer(CLWContext context, int devidx);
    // Destructor
    ~FrRenderer();

    // Create output
    Output* CreateOutput(int w, int h);

    // Delete output
    void DeleteOutput(Output* output);

    // Clear output
    void Clear(FireRays::float3 const& val, Output& output);

    // Do necessary precalculation and initialization
    void Preprocess(Scene const& scene);
    
    // Render the scene into the output
    void Render(Scene const& scene);

    // Render the scene into the output
    void RenderAmbientOcclusion(Scene const& scene, float radius);

    // Set output
    void SetOutput(Output* output);

    // Set number of bounces
    void SetNumBounces(int nbounces);
    
    // Benchmark the scene
    void RunBenchmark(Scene const& scene);

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
    void ShadeFirstHit(Scene const& scene, int pass);
    // Handle missing rays
    void ShadeMiss(Scene const& scene, int pass);
    // Gather light samples and account for visibility
    void GatherLightSamples(Scene const& scene, int pass);
    // Restore pixel indices after compaction
    void RestorePixelIndices(int pass);
    // Pack textures for GPU
    void BakeTextures(Scene const& scene);
    // Sample AO
    void SampleAmbientOcclusion(Scene const& scene, float radius);
    // Gather light samples and account for visibility
    void GatherAmbientOcclusion(Scene const& scene);
	// Convert intersection info to compaction predicate
	void ConvertToPredicate(int pass);

public:
    // Intersection API
    FireRays::IntersectionApiCL* api_;
    // CL context
    CLWContext context_;
    // Output object
    FrOutput* output_;
    // Number of bounces
    int nbounces_;
    
    
    // GPU data
    struct GpuData;
    std::unique_ptr<GpuData> gpudata_;
    
    // Intersector data
    std::vector<FireRays::Shape*> shapes_;

    // Vidmem usage
    // Data
    size_t vidmemusage_;
    // Working set
    size_t vidmemws_;
};

class FrOutput : public Output
{
public:
	FrOutput(int w, int h, CLWContext context)
		: Output(w, h)
		, context_(context)
		, data_(context.CreateBuffer<FireRays::float3>(w*h, CL_MEM_READ_WRITE))
	{
	}

	void GetData(FireRays::float3* data) const
	{
		context_.ReadBuffer(0, data_, data, data_.GetElementCount()).Wait();
	}

	void Clear(FireRays::float3 const& val)
	{
		context_.FillBuffer(0, data_, val, data_.GetElementCount()).Wait();
	}

	CLWBuffer<FireRays::float3> data() const { return data_; }

private:
	CLWBuffer<FireRays::float3> data_;
	CLWContext context_;
};


#endif // FRRENDERER_H
