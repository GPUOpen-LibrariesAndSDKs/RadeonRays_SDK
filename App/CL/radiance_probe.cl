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
#pragma once

#include <../App/CL/utils.cl>

typedef struct _RadianceProbeDesc
{
    matrix world_to_tangent;
    matrix tangent_to_world;
    float3 p;
    float radius;
    int num_samples;
    int atomic_update;
    int padding1;
} RadianceProbeDesc;

typedef struct _RadianceProbeData
{
    float3 r0;
    float3 r1;
    float3 r2;

    float3 g0;
    float3 g1;
    float3 g2;

    float3 b0;
    float3 b1;
    float3 b2;
} RadianceProbeData;

inline
void SH_Get2ndOrderCoeffs(float3 d, float3* sh0, float3* sh1, float3* sh2)
{
    d = normalize(d);

    float fC0, fC1, fS0, fS1, fTmpA, fTmpB, fTmpC;
    float pz2 = d.z * d.z;

    // 
    sh0->x = 0.2820947917738781f;
    sh0->z = 0.4886025119029199f * d.z;
    sh2->x = 0.9461746957575601f * pz2 + -0.3153915652525201f;
    fC0 = d.x;
    fS0 = d.y;
    fTmpA = -0.48860251190292f;
    sh1->x = fTmpA * fC0;
    sh0->y = fTmpA * fS0;
    fTmpB = -1.092548430592079f * d.z;
    sh2->y = fTmpB * fC0;
    sh1->z = fTmpB * fS0;
    fC1 = d.x*fC0 - d.y*fS0;
    fS1 = d.x*fS0 + d.y*fC0;
    fTmpC = 0.5462742152960395f;
    sh2->z = fTmpC * fC1;
    sh1->y = fTmpC * fS1;
}


inline
void RadianceProbe_RefineEstimate(
    __global RadianceProbeDesc* restrict desc,
    __global RadianceProbeData* restrict probe, 
    float3 sample, 
    float3 direction, 
    float ray_distance
)
{
    float3 normal = desc->world_to_tangent.m1.xyz;

    if (dot(normal, direction) > 0.f)
    {
        float3 direction_ts = matrix_mul_vector3(desc->world_to_tangent, direction);

        float3 sh0, sh1, sh2;
        SH_Get2ndOrderCoeffs(direction_ts, &sh0, &sh1, &sh2);

        // Lock here
        //while (atomic_cmpxchg(&desc->atomic_update, 0, 1) == 1);

        probe->r0.xyz += sh0 * sample.x;
        probe->r1.xyz += sh1 * sample.x;
        probe->r2.xyz += sh2 * sample.x;

        probe->g0.xyz += sh0 * sample.y;
        probe->g1.xyz += sh1 * sample.y;
        probe->g2.xyz += sh2 * sample.y;

        probe->b0.xyz += sh0 * sample.z;
        probe->b1.xyz += sh1 * sample.z;
        probe->b2.xyz += sh2 * sample.z;

        desc->radius += 1.f / ray_distance;

        atomic_inc(&desc->num_samples);

        // Unlock here
        //atomic_xchg(&desc->atomic_update, 0);
    }
}

inline
bool
RadianceProbe_AddContribution(
    __global RadianceProbeDesc* restrict desc,
    __global RadianceProbeData* restrict probe, 
    float3 p, 
    float3 n, 
    RadianceProbeData* out_probe,
    float* weight
)
{
    float radius = desc->num_samples / desc->radius;
    float3 normal = desc->world_to_tangent.m1.xyz;
    float perr = length(p - desc->p) / radius;
    float nerr = 9.0124*native_sqrt(1.f - dot(n, normal));
    float err = max(perr, nerr);
    if (err < 1.0f)
    {
        float invs = 1.f / desc->num_samples;
        float wt = (1.0f - err);
        *weight += wt;

        out_probe->r0 += wt * probe->r0 * invs;
        out_probe->r1 += wt * probe->r1 * invs;
        out_probe->r2 += wt * probe->r2 * invs;

        out_probe->g0 += wt * probe->g0 * invs;
        out_probe->g1 += wt * probe->g1 * invs;
        out_probe->g2 += wt * probe->g2 * invs;

        out_probe->b0 += wt * probe->b0 * invs;
        out_probe->b1 += wt * probe->b1 * invs;
        out_probe->b2 += wt * probe->b2 * invs;
        return true;
    }

    return false;
}

