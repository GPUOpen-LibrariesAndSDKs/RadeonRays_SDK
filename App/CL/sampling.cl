
/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

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

/// Uniformly sample a triangle
/*void SampleTriangleUniform(
                           // RNG to use
                           Rng* rng,
                           // Triangle vertices
                           float3 v0,
                           float3 v1,
                           float3 v2,
                           // Triangle normals
                           float3 n0,
                           float3 n1,
                           float3 n2,
                           // Point and normal
                           float3* p,
                           float3* n,
                           // PDF
                           float* pdf
                           )
{
    // Sample 2D value
    float a = RandFloat(rng);
    float b = RandFloat(rng);
    
    // Calculate point
    *p = (1.f - sqrt(a)) * v0.xyz + sqrt(a) * (1.f - b) * v1.xyz + sqrt(a) * b * v2.xyz;
    // Calculate normal
    *n = normalize((1.f - sqrt(a)) * n0.xyz + sqrt(a) * (1.f - b) * n1.xyz + sqrt(a) * b * n2.xyz);
    // PDF
    *pdf = 1.f / (length(cross((v2-v0), (v1-v0))) * 0.5f);
}*/

/// Sample hemisphere with cos weight
float3 SampleHemisphere(
                        // RNG to use
                        Rng* rng,
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
    float r1 = RandFloat(rng);
    float r2 = RandFloat(rng);
    
    // Transform to spherical coordinates
    float sinpsi = sin(2*PI*r1);
    float cospsi = cos(2*PI*r1);
    float costheta = pow(1.f - r2, 1.f/(e + 1.f));
    float sintheta = sqrt(1.f - costheta * costheta);
    
    // Return the result
    return normalize(u * sintheta * cospsi + v * sintheta * sinpsi + n * costheta);
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


#endif // SAMPLING_CL