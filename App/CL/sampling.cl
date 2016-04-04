
/**********************************************************************
Copyright �2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

?   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
?   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
#ifndef SAMPLING_CL
#define SAMPLING_CL

#include <../App/CL/utils.cl>
#include <../App/CL/random.cl>


/// Sample hemisphere with cos weight
float3 Sample_MapToHemisphere(
                        // Sample
                        float2 sample,
                        // Hemisphere normal
                        float3 n,
                        // Cos power
                        float e
                        )
{
    // Construct basis
    float3 u = GetOrthoVector(n);
    float3 v = cross(u, n);
    u = cross(n, v);
    
    // Calculate 2D sample
	float r1 = sample.x;
	float r2 = sample.y;
    
    // Transform to spherical coordinates
    float sinpsi = sin(2*PI*r1);
    float cospsi = cos(2*PI*r1);
    float costheta = pow(1.f - r2, 1.f/(e + 1.f));
    float sintheta = sqrt(1.f - costheta * costheta);
    
    // Return the result
    return normalize(u * sintheta * cospsi + v * sintheta * sinpsi + n * costheta);
}

/// Sample hemisphere with cos weight
float3 Sample_MapToSphere(
                        // Sample
                        float2 sample
                        )
{
    float z = 1.f - 2.f * sample.x;
    float r = native_sqrt(max(0.f, 1.f - z*z));
    float phi = 2.f * PI * sample.y;
    float x = cos(phi);
    float y = sin(phi);
    
    // Return the result
    return make_float3(x,y,z);
}

/// Power heuristic for multiple importance sampling
float PowerHeuristic(int nf, float fpdf, int ng, float gpdf)
{
    float f = nf * fpdf;
    float g = ng * gpdf;
    return (f*f) / (f*f + g*g);
}

/// Balance heuristic for multiple importance sampling
float BalanceHeuristic(int nf, float fpdf, int ng, float gpdf)
{
    float f = nf * fpdf;
    float g = ng * gpdf;
    return (f) / (f + g);
}

typedef struct
{
	uint seq;
	uint s0;
	uint s1;
	uint s2;
} SobolSampler;

float2 UniformSampler_Sample2D(Rng* rng)
{
	float2 sample;
	sample.x = RandFloat(rng);
	sample.y = RandFloat(rng);
	return sample;
}

#define MATSIZE 52

// The code is taken from: http://gruenschloss.org/sobol/kuo-2d-proj-single-precision.zip
// 
float SobolSampler_Sample1D(uint index, uint dimension, uint scramble, __global uint const* mat)
{
	uint result = scramble;
	for (uint i = dimension * MATSIZE; index; index >>= 1, ++i)
	{
		if (index & 1)
			result ^= mat[i];
	}

	return result * (1.f / (1ULL << 32));
}


#endif // SAMPLING_CL