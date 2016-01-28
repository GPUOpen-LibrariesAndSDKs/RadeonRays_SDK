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