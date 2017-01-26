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
#ifndef MATERIAL_CL
#define MATERIAL_CL

#define CMJ 1

#include <../App/CL/utils.cl>
#include <../App/CL/random.cl>
#include <../App/CL/texture.cl>
#include <../App/CL/payload.cl>
#include <../App/CL/bxdf.cl>

void Material_Select(
    // Scene data
    Scene const* scene,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,

#ifdef SOBOL
    // Sampler state
    __global SobolSampler* sampler,
    // Sobol matrices
    __global uint const* sobolmat,
    // Current bounce
    int bounce,
#else
    Rng* rng,
#endif

    // Geometry
    DifferentialGeometry* dg
)
{
    // Check material type
    int type = dg->mat.type;

    // If material is regular BxDF we do not have to sample it
    if (type != kFresnelBlend && type != kMix)
    {
        // If fresnel > 0 here we need to calculate Frensle factor (remove this workaround)
        if (dg->mat.fresnel > 0.f)
        {
            float etai = 1.f;
            float etat = dg->mat.ni;
            float cosi = dot(dg->n, wi);

            // Revert normal and eta if needed
            if (cosi < 0.f)
            {
                float tmp = etai;
                etai = etat;
                etat = tmp;
                cosi = -cosi;
            }

            float eta = etai / etat;
            float sini2 = 1.f - cosi * cosi;
            float sint2 = eta * eta * sini2;

            float fresnel = 1.f;

            if (sint2 < 1.f)
            {
                float cost = native_sqrt(max(0.f, 1.f - sint2));
                fresnel = FresnelDielectric(etai, etat, cosi, cost);
            }

            dg->mat.fresnel = Bxdf_IsBtdf(dg) ? (1.f - fresnel) : fresnel;
        }
        else
        {
            // Otherwise set multiplier to 1
            dg->mat.fresnel = 1.f;
        }
    }
    // Here we deal with combined material and we have to sample
    else
    {
        // Prefetch current material
        Material mat = dg->mat;
        int iter = 0;

        // Might need several passes of sampling
        while (mat.type == kFresnelBlend || mat.type == kMix)
        {
            if (mat.type == kFresnelBlend)
            {
                float etai = 1.f;
                float etat = mat.ni;
                float cosi = dot(dg->n, wi);

                // Revert normal and eta if needed
                if (cosi < 0.f)
                {
                    float tmp = etai;
                    etai = etat;
                    etat = tmp;
                    cosi = -cosi;
                }

                float eta = etai / etat;
                float sini2 = 1.f - cosi * cosi;
                float sint2 = eta * eta * sini2;

                int idx = 0;

                float fresnel = 1.f;

                if (sint2 < 1.f)
                {
                    float cost = native_sqrt(max(0.f, 1.f - sint2));
                    fresnel = FresnelDielectric(etai, etat, cosi, cost);
                }

#ifdef SOBOL
                float sample = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kMaterial + iter), sampler->s0, sobolmat);
#else
                float sample = UniformSampler_Sample2D(rng).x;
#endif

                if (sample < fresnel)
                {
                    // Sample top
                    idx = mat.brdftopidx;
                    // 
                    mat = scene->materials[idx];
                    mat.fresnel = 1.f;
                }
                else
                {
                    // Sample base
                    idx = mat.brdfbaseidx;
                    // 
                    mat = scene->materials[idx];
                    mat.fresnel = 1.f;
                }
            }
            else
            {

#ifdef SOBOL
                float sample = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kMaterial + iter), sampler->s0, sobolmat);
#else
                float sample = UniformSampler_Sample2D(rng).x;
#endif
                float weight = Texture_GetValue1f(mat.ns, dg->uv, TEXTURE_ARGS_IDX(mat.nsmapidx));

                if (sample < weight)
                {
                    // Sample top
                    int idx = mat.brdftopidx;
                    //
                    mat = scene->materials[idx];
                    mat.fresnel = 1.f;
                }
                else
                {
                    // Sample base
                    int idx = mat.brdfbaseidx;
                    //
                    mat = scene->materials[idx];
                    mat.fresnel = 1.f;
                }
            }
        }

        dg->mat = mat;
    }
}


#endif // MATERIAL_CL