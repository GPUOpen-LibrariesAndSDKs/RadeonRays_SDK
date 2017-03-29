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
#ifndef SH_H
#define SH_H

#include "math/mathutils.h"

///< In mathematics, spherical harmonics are the angular portion of a set of solutions to Laplace's equation. 
///< Represented in a system of spherical coordinates, Laplace's spherical harmonics 
///<  Y_l_m are a specific set of spherical harmonics that forms an orthogonal system, first introduced by Pierre Simon de Laplace in 1782.
///< Spherical harmonics are important in many theoretical and practical applications, 
///< particularly in the computation of atomic orbital electron configurations, 
///< representation of gravitational fields, geoids, and the magnetic fields of planetary bodies and stars, 
///< and characterization of the cosmic microwave background radiation. 
///< In 3D computer graphics, spherical harmonics play a special role in a wide variety of topics including 
///< indirect lighting (ambient occlusion, global illumination, precomputed radiance transfer, etc.) and modelling of 3D shapes.
///<

///< The function gives the total number of coefficients required for the term up to specified band
inline int NumShTerms(int l)
{
    return (l + 1)*(l + 1);
}

///< The function flattens 2D (l,m) index into 1D to be used in array indexing
inline int ShIndex(int l, int m)
{
    return l*l + l + m;
}


///< The function evaluates the value of Y_l_m(p) coefficient up to lmax band. 
///< coeffs array should have at least NumShTerms(lmax) elements.
void ShEvaluate(RadeonRays::float3 const&p, int lmax, float* out);


///< Apply convolution with dot(n,wi) term
void ShConvolveCosTheta(int lmax, RadeonRays::float3 const* cin, RadeonRays::float3* cout);



#endif // SH_H