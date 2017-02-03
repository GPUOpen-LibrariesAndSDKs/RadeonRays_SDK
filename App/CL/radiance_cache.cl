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
#include <../App/CL/common.cl>
#include <../App/CL/ray.cl>
#include <../App/CL/isect.cl>
#include <../App/CL/utils.cl>
#include <../App/CL/payload.cl>
#include <../App/CL/texture.cl>
#include <../App/CL/sampling.cl>
#include <../App/CL/normalmap.cl>
#include <../App/CL/bxdf.cl>
#include <../App/CL/light.cl>
#include <../App/CL/camera.cl>
#include <../App/CL/scene.cl>
#include <../App/CL/material.cl>
#include <../App/CL/volumetrics.cl>
#include <../App/CL/path.cl>
#include <../App/CL/hlbvh.cl>
#include <../App/CL/radiance_probe.cl>

__kernel void AddRadianceSamples(
    __global ray const* sample_rays,
    __global int const* predicate,
    __global float3 const* samples, 
    int num_samples,
    __global HlbvhNode const* bvh,
    __global bbox const* bounds,
    __global RadianceProbeDesc* descs,
    __global RadianceProbeData* probes
    )
{
    int global_id = get_global_id(0);

    if (global_id < num_samples)
    {
        if (predicate[global_id] == 1)
        {
            float3 sample_position = sample_rays[global_id].o.xyz;
            float3 sample_direction = sample_rays[global_id].d.xyz;
            float ray_distance = sample_rays[global_id].o.w;

            int probe_idx = Hlbvh_GetClosestItemIndex(bvh, bounds, sample_position);

            RadianceProbe_RefineEstimate(descs + probe_idx, probes + probe_idx, samples[global_id], sample_direction, ray_distance);
        }
    }
}