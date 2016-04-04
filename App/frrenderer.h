/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
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
    // Sample AO
    void SampleAmbientOcclusion(Scene const& scene, float radius);
    // Gather light samples and account for visibility
    void GatherAmbientOcclusion(Scene const& scene);
	// Convert intersection info to compaction predicate
	void FilterPathStream(int pass);
	// Integrate volume
	void ShadeVolume(Scene const& scene, int pass);


public:
    // Intersection API
    FireRays::IntersectionApiCL* api_;
    // CL context
    CLWContext context_;
    // Output object
    FrOutput* output_;
    // Number of bounces
    int nbounces_;
	int resetsampler_;
    
    
    // GPU data
	struct QmcSampler;
	struct PathState;
    struct Volume;
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
