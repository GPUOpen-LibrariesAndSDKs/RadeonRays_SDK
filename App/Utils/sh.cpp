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
#include "sh.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include <vector>

using namespace RadeonRays;

// Taken from PBRT book
static void EvaluateLegendrePolynomial(float x, int lmax, float* out)
{
#define P(l,m) out[ShIndex(l,m)]
    // Calculate 0-strip values
    P(0,0) = 1.f;
    P(1,0) = x;
    for (int l = 2; l <= lmax; ++l)
    {
        P(l, 0) = ((2*l-1)*x*P(l-1,0) - (l-1)*P(l-2,0)) / l;
    }

    // Calculate edge values m=l
    float neg = -1.f;
    float dfact = 1.f;
    float xroot = sqrtf(std::max(0.f, 1.f - x*x));
    float xpow = xroot;
    for (int l = 1; l <= lmax; ++l) 
    {
        P(l, l) = neg * dfact * xpow;
        neg *= -1.f;
        dfact *= 2*l + 1;
        xpow *= xroot;
    }

    // Calculate pre-edge values  m=l-1
    for (int l = 2; l <= lmax; ++l)
    {
        P(l, l-1) = x * (2*l-1) * P(l-1, l-1);
    }

    // Calculate the rest
    for (int l = 3; l <= lmax; ++l)
    {
        for (int m = 1; m <= l-2; ++m)
        {
            P(l, m) = ((2 * (l-1) + 1) * x * P(l-1,m) - (l-1+m) * P(l-2,m)) / (l - m);
        }
    }
#undef P
}


// Calculates (a-|b|)! / (a + |b|)!
static float DivFact(int a, int b) 
{
    if (b == 0) 
    {
        return 1.f;
    }

    float fa = (float)a;
    float fb = std::fabs((float)b);
    float v = 1.f;

    for (float x = fa-fb+1.f; x <= fa+fb; x += 1.f)
    {
        v *= x;
    }

    return 1.f / v;
}

///< The function computes normalization constants for Y_l_m terms
static float K(int l, int m)
{
    return std::sqrt((2.f * l + 1.f) * 1.f / (4 * PI) * DivFact(l, m));
}

///< Calculates sin(i*s) and cos(i*c) for i up to n 
static void SinCosIndexed(float s, float c, int n, float *sout, float *cout) 
{
    float si = 0;
    float ci = 1;
    for (int i = 0; i < n; ++i) 
    {
        *sout++ = si;
        *cout++ = ci;
        float oldsi = si;
        si = si * c + ci * s;
        ci = ci * c - oldsi * s;
    }
}


///< The function evaluates the value of Y_l_m(p) coefficient up to lmax band. 
///< coeffs array should have at least NumShTerms(lmax) elements.
void ShEvaluate(float3 const& p, int lmax, float* coefs)
{
    // Initialize the array by the values of Legendre polynomials 
    EvaluateLegendrePolynomial(p.z, lmax, coefs);


    // Normalization coefficients
    std::vector<float> klm(NumShTerms(lmax));

    for (int l = 0; l <= lmax; ++l)
    {
        for (int m = -l; m <= l; ++m)
        {
            klm[ShIndex(l, m)] = K(l, m);
        }
    }

    // Calculate cos(mphi) and sin(mphi) values
    std::vector<float> cosmphi(lmax + 1);
    std::vector<float> sinmphi(lmax + 1);

    float xylen = std::sqrt(std::max(0.f, 1.f - p.z*p.z));

    // If degenerate set to 0 and 1
    if (xylen == 0.f) 
    {
        for (int i = 0; i <= lmax; ++i) 
        {
            sinmphi[i] = 0.f;
            cosmphi[i] = 1.f;
        }
    }
    else
    {
        SinCosIndexed(p.y / xylen, p.x / xylen, lmax+1, &sinmphi[0], &cosmphi[0]);
    }

    // Multiply in normalization coeffs and sin and cos where needed
    const float sqrt2 = sqrtf(2.f);
    for (int l = 0; l <= lmax; ++l) 
    {
        // For negative multiply by sin and Klm
        for (int m = -l; m < 0; ++m)
        {
            coefs[ShIndex(l, m)] = sqrt2 * klm[ShIndex(l, m)] * coefs[ShIndex(l, -m)] * sinmphi[-m];
        }

        // For 0 multiply by Klm
        coefs[ShIndex(l, 0)] *= klm[ShIndex(l, 0)];

        // For positive multiply by cos and Klm
        for (int m = 1; m <= l; ++m)
        {
            coefs[ShIndex(l, m)] *= sqrt2 * klm[ShIndex(l, m)] * cosmphi[m];
        }
    }
}

static inline float lambda(float l)
{
    return sqrtf((4.f * (float)M_PI) / (2.f * l + 1.f));
}

void ShConvolveCosTheta(int lmax, float3 const* cin, float3* cout)
{
    static const float s_costheta[18] = { 0.8862268925f, 1.0233267546f,
        0.4954159260f, 0.0000000000f, -0.1107783690f, 0.0000000000f,
        0.0499271341f, 0.0000000000f, -0.0285469331f, 0.0000000000f,
        0.0185080823f, 0.0000000000f, -0.0129818395f, 0.0000000000f,
        0.0096125342f, 0.0000000000f, -0.0074057109f, 0.0000000000f };
    
    for (int l = 0; l <= lmax; ++l)
    {
        for (int m = -l; m <= l; ++m)
        {
            int o = ShIndex(l, m);
            
            if (l < 18)
                cout[o] = lambda((float)l) * cin[o] * s_costheta[l];
            else
                cout[o] = 0.f;
        }
    }
}
