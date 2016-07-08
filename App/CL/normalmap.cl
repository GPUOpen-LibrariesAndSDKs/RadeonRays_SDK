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
#ifndef NORMALMAP_CL
#define NORMALMAP_CL

#include <../App/CL/utils.cl>
#include <../App/CL/texture.cl>
#include <../App/CL/payload.cl>

void ApplyNormalMap(DifferentialGeometry* dg, TEXTURE_ARG_LIST)
{
    int nmapidx = dg->mat.nmapidx;
    if (nmapidx != -1)
    {
        // Now n, dpdu, dpdv is orthonormal basis
        float3 mappednormal = 2.f * Texture_Sample2D(dg->uv, TEXTURE_ARGS_IDX(nmapidx)) - make_float3(1.f, 1.f, 1.f);
    
        // Return mapped version
        dg->n = normalize(mappednormal.z *  dg->n * 0.5f + mappednormal.x * dg->dpdu + mappednormal.y * dg->dpdv);
    }
}

void ApplyBumpMap(DifferentialGeometry* dg, TEXTURE_ARG_LIST)
{
    int nmapidx = dg->mat.nmapidx;
    if (nmapidx != -1)
    {
        // Now n, dpdu, dpdv is orthonormal basis
        float3 mappednormal = 2.f * Texture_SampleBump(dg->uv, TEXTURE_ARGS_IDX(nmapidx)) - make_float3(1.f, 1.f, 1.f);

        // Return mapped version
        dg->n = normalize(mappednormal.z * dg->n + mappednormal.x * dg->dpdu + mappednormal.y * dg->dpdv);
    }
}

#endif // NORMALMAP_CL