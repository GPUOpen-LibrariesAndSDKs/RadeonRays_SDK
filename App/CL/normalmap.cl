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

void DifferentialGeometry_ApplyNormalMap(DifferentialGeometry* diffgeo, TEXTURE_ARG_LIST)
{
    int nmapidx = diffgeo->mat.nmapidx;
    if (nmapidx != -1)
    {
        // Now n, dpdu, dpdv is orthonormal basis
        float3 mappednormal = 2.f * Texture_Sample2D(diffgeo->uv, TEXTURE_ARGS_IDX(nmapidx)).xyz - make_float3(1.f, 1.f, 1.f);

        // Return mapped version
        diffgeo->n = normalize(mappednormal.z *  diffgeo->n + mappednormal.x * diffgeo->dpdu - mappednormal.y * diffgeo->dpdv);
        diffgeo->dpdv = normalize(cross(diffgeo->n, diffgeo->dpdu));
        diffgeo->dpdu = normalize(cross(diffgeo->dpdv, diffgeo->n));
    }
}

void DifferentialGeometry_ApplyBumpMap(DifferentialGeometry* diffgeo, TEXTURE_ARG_LIST)
{
    int nmapidx = diffgeo->mat.nmapidx;
    if (nmapidx != -1)
    {
        // Now n, dpdu, dpdv is orthonormal basis
        float3 mappednormal = 2.f * Texture_SampleBump(diffgeo->uv, TEXTURE_ARGS_IDX(nmapidx)) - make_float3(1.f, 1.f, 1.f);

        // Return mapped version
        diffgeo->n = normalize(mappednormal.z * diffgeo->n + mappednormal.x * diffgeo->dpdu + mappednormal.y * diffgeo->dpdv);
        diffgeo->dpdv = normalize(cross(diffgeo->n, diffgeo->dpdu));
        diffgeo->dpdu = normalize(cross(diffgeo->dpdv, diffgeo->n));
    }
}

void DifferentialGeometry_ApplyBumpNormalMap(DifferentialGeometry* diffgeo, TEXTURE_ARG_LIST)
{
    int bump_flag = diffgeo->mat.bump_flag;
    if (bump_flag)
    {
        DifferentialGeometry_ApplyBumpMap(diffgeo, TEXTURE_ARGS);
    }
    else
    {
        DifferentialGeometry_ApplyNormalMap(diffgeo, TEXTURE_ARGS);
    }
}



#endif // NORMALMAP_CL
