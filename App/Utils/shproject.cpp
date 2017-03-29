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
#include "shproject.h"
#include "sh.h"

#include <vector>
#include <cmath>

using namespace RadeonRays;

///< The function projects latitude-longitude environment map to SH basis up to lmax band
void ShProjectEnvironmentMap(float3 const* envmap, int width, int height, int lmax, float3* coeffs)
{
    // Resulting coefficients for RGB
    // std::vector<float3> coeffs(NumShTerms(lmax));
    // Temporary coefficients storage
    std::vector<float>  ylm(NumShTerms(lmax));

    // Precompute sin and cos for the sphere
    std::vector<float> sintheta(height);
    std::vector<float> costheta(height);
    std::vector<float> sinphi(width);
    std::vector<float> cosphi(width);

    float thetastep = PI / height;
    float phistep = 2.f * PI / width;
    float theta0 = PI / height / 2;
    float phi0 = 2.f * PI / width / 2;

    for (int i = 0; i < width; ++i)
    {
        sinphi[i] = std::sin(phi0 + i * phistep);
        cosphi[i] = std::cos(phi0 + i * phistep);
    }

    for (int i = 0; i < height; ++i)
    {
        sintheta[i] = std::sin(theta0 + i * thetastep);
        costheta[i] = std::cos(theta0 + i * thetastep);
    }

    // Iterate over the pixels calculating Riemann sum
    for (int phi = 0; phi < width; ++phi)
    {
        for (int theta = 0; theta < height; ++theta)
        {
            // Construct direction vector
            float3 w = normalize(float3(sintheta[theta] * cosphi[phi], costheta[theta], sintheta[theta] * sinphi[phi]));

            // Construct uv sample coordinates
            float2 uv = float2((float)phi / width, (float)theta / height);

            // Sample environment map
            int iu = (int)floor(uv.x * width);
            int iv = (int)floor(uv.y * height);

            float3 le = envmap[width * iv + iu];

            // Evaluate SH functions at w up to lmax band
            ShEvaluate(w, lmax, &ylm[0]);

            // Evaluate Riemann sum accouting for solid angle conversion (sin term)
            for (int i = 0; i < NumShTerms(lmax); ++i)
            {
                coeffs[i] += le * ylm[i] * sintheta[theta] * (PI / height) * (2.f * PI / width);
            }
        }
    }
}

///< The function evaluates SH functions and dumps values to latitude-longitude map
void ShEvaluateAndDump(int width, int height, int lmax, float3 const* coeffs, float3* envmap)
{
    // Allocate space for SH functions
    std::vector<float> ylm(NumShTerms(lmax));

    // Precalculate sin and cos terms 
    std::vector<float> sintheta(height);
    std::vector<float> costheta(height);
    std::vector<float> sinphi(width);
    std::vector<float> cosphi(width);

    float thetastep = PI / height;
    float phistep = 2.f*PI / width;
    float theta0 = PI / height / 2;
    float phi0= 2.f*PI / width / 2;

    for (int i = 0; i < width; ++i)
    {
        sinphi[i] = std::sin(phi0 + i * phistep);
        cosphi[i] = std::cos(phi0 + i * phistep);
    }

    for (int i = 0; i < height; ++i)
    {
        sintheta[i] = std::sin(theta0 + i * thetastep);
        costheta[i] = std::cos(theta0 + i * thetastep);
    }

    // Iterate thru image pixels
    for (int phi = 0; phi < width; ++phi)
    {
        for (int theta = 0; theta < height; ++theta)
        {
            // Calculate direction
            float3 w = normalize(float3(sintheta[theta] * cosphi[phi], costheta[theta], sintheta[theta] * sinphi[phi]));

            // Evaluate SH functions at w up to lmax band
            ShEvaluate(w, lmax, &ylm[0]);

            // Prepare data
            envmap[theta * width + phi] = float3();

            // Evaluate function injecting SH coeffs
            for (int i = 0; i < NumShTerms(lmax); ++i)
            {
                envmap[theta * width + phi] += ylm[i] * coeffs[i];
            }
        }
    }
}