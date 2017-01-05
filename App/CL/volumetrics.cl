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
#ifndef VOLUMETRICS_CL
#define VOLUMETRICS_CL

#include <../App/CL/payload.cl>
#include <../App/CL/path.cl>

#define FAKE_SHAPE_SENTINEL 0xFFFFFF




// The following functions are taken from PBRT
float PhaseFunction_Uniform(float3 wi, float3 wo)
{
    return 1.f / (4.f * PI);
}

float PhaseFunction_Rayleigh(float3 wi, float3 wo)
{
    float costheta = dot(wi, wo);
    return  3.f / (16.f*PI) * (1 + costheta * costheta);
}

float PhaseFunction_MieHazy(float3 wi, float3 wo)
{
    float costheta = dot(wi, wo);
    return (0.5f + 4.5f * native_powr(0.5f * (1.f + costheta), 8.f)) / (4.f*PI);
}

float PhaseFunction_MieMurky(float3 wi, float3 wo)
{
    float costheta = dot(wi, wo);
    return (0.5f + 16.5f * native_powr(0.5f * (1.f + costheta), 32.f)) / (4.f*PI);
}

float PhaseFunction_HG(float3 wi, float3 wo, float g)
{
    float costheta = dot(wi, wo);
    return 1.f / (4.f * PI) *
        (1.f - g*g) / native_powr(1.f + g*g - 2.f * g * costheta, 1.5f);
}


// Evaluate volume transmittance along the ray [0, dist] segment
float3 Volume_Transmittance(__global Volume const* volume, __global ray const* ray, float dist)
{
    switch (volume->type)
    {
        case kHomogeneous:
        {
            // For homogeneous it is e(-sigma * dist)
            float3 sigma_t = volume->sigma_a + volume->sigma_s;
            return native_exp(-sigma_t * dist);
        }
    }
    
    return 1.f;
}

// Evaluate volume selfemission along the ray [0, dist] segment
float3 Volume_Emission(__global Volume const* volume, __global ray const* ray, float dist)
{
    switch (volume->type)
    {
        case kHomogeneous:
        {
            // For homogeneous it is simply Tr * Ev (since sigma_e is constant)
            return Volume_Transmittance(volume, ray, dist) * volume->sigma_e;
        }
    }
    
    return 0.f;
}

// Sample volume in order to find next scattering event
float Volume_SampleDistance(__global Volume const* volume, __global ray const* ray, float maxdist, float sample, float* pdf)
{
    switch (volume->type)
    {
        case kHomogeneous:
        {
            // The PDF = sigma * e(-sigma * x), so the larger sigma the closer we scatter
            float sigma = (volume->sigma_s.x + volume->sigma_s.y + volume->sigma_s.z) / 3;
            float d = sigma > 0.f ? (-native_log(sample) / sigma) : -1.f;
            *pdf = sigma > 0.f ? (sigma * native_exp(-sigma * d)) : 0.f;
            return d;
        }
    }
    
    return -1.f;
}

// Apply volume effects (absorbtion and emission) and scatter if needed.
// The rays we handling here might intersect something or miss, 
// since scattering can happen even for missed rays.
// That's why this function is called prior to ray compaction.
// In case ray has missed geometry (has shapeid < 0) and has been scattered,
// we put FAKE_SHAPE_SENTINEL into shapeid to prevent ray from being compacted away.
//
__kernel void EvaluateVolume(
    // Ray batch
    __global ray const* rays,
    // Pixel indices
    __global int const* pixelindices,
    // Number of rays
    __global int const* numrays,
    // Volumes
    __global Volume const* volumes,
    // Textures
    TEXTURE_ARG_LIST,
    // RNG seed
    int rngseed,
    // Sampler state
    __global SobolSampler* samplers,
    // Sobol matrices
    __global uint const* sobolmat,
    // Current bounce 
    int bounce,
    // Intersection data
    __global Intersection* isects,
    // Current paths
    __global Path* paths,
    // Output
    __global float3* output
    )
{
    int globalid = get_global_id(0);
    
    // Only handle active rays
    if (globalid < *numrays)
    {
        int pixelidx = pixelindices[globalid];
        
        __global Path* path = paths + pixelidx;

        // Path can be dead here since compaction step has not 
        // yet been applied
        if (!Path_IsAlive(path))
            return;

        int volidx = Path_GetVolumeIdx(path);

        // Check if we are inside some volume
        if (volidx != -1)
        {
#ifdef SOBOL
            __global SobolSampler* sampler = samplers + pixelidx;
            float sample = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kVolume), sampler->s0, sobolmat);
#else
            Rng rng;
            InitRng(rngseed + (globalid << 2) * 157 + 13, &rng);
            float sample = UniformSampler_Sample2D(&rng).x;
#endif
            // Try sampling volume for a next scattering event
            float pdf = 0.f;
            float maxdist = Intersection_GetDistance(isects + globalid);
            float d = Volume_SampleDistance(&volumes[volidx], &rays[globalid], maxdist, sample, &pdf);
            
            // Check if we shall skip the event (it is either outside of a volume or not happened at all)
            bool skip = d < 0 || d > maxdist || pdf <= 0.f;

            if (skip)
            {
                // In case we skip we just need to apply volume absorbtion and emission for the segment we went through
                // and clear scatter flag
                Path_ClearScatterFlag(path);
                // Emission contribution accounting for a throughput we have so far
                Path_AddContribution(path, output, pixelidx, Volume_Emission(&volumes[volidx], &rays[globalid], maxdist));
                // And finally update the throughput
                Path_MulThroughput(path, Volume_Transmittance(&volumes[volidx], &rays[globalid], maxdist));
            }
            else
            {
                // Set scattering flag to notify ShadeVolume kernel to handle this path
                Path_SetScatterFlag(path);
                // Emission contribution accounting for a throughput we have so far
                Path_AddContribution(path, output, pixelidx, Volume_Emission(&volumes[volidx], &rays[globalid], d) / pdf);
                // Update the throughput
                Path_MulThroughput(path, (Volume_Transmittance(&volumes[volidx], &rays[globalid], d) / pdf));
                // Put fake shape to prevent from being compacted away
                isects[globalid].shapeid = FAKE_SHAPE_SENTINEL;
                // And keep scattering distance around as well
                isects[globalid].uvwt.w = d;
            }
        }
    }
}

#endif // VOLUMETRICS_CL
