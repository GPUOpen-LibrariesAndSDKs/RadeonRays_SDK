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
#ifndef CAMERA_CL
#define CAMERA_CL

#include <../App/CL/common.cl>
#include <../App/CL/payload.cl>
#include <../App/CL/sampling.cl>
#include <../App/CL/utils.cl>
#include <../App/CL/path.cl>


/// Ray generation kernel for perspective camera.
/// Rays are generated from camera position to viewing plane
/// using random sample distribution within the pixel.
///
__kernel void PerspectiveCamera_GeneratePaths(
                             // Camera descriptor
                             __global Camera const* camera,
                             // Image resolution
                             int imgwidth,
                             int imgheight,
                             // RNG seed value
                             uint rngseed,
                             // Output rays
                             __global ray* rays,
                             __global uint* random,
                             __global uint const* sobolmat,
                             int frame
#ifndef NO_PATH_DATA
                             ,__global Path* paths
#endif
        )
{
    int2 globalid;
    globalid.x  = get_global_id(0);
    globalid.y  = get_global_id(1);

    // Check borders
    if (globalid.x < imgwidth && globalid.y < imgheight)
    {
        // Get pointer to ray to handle
        __global ray* myray = rays + globalid.y * imgwidth + globalid.x;

#ifndef NO_PATH_DATA
        __global Path* mypath = paths + globalid.y * imgwidth + globalid.x;
#endif

        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[globalid.x + imgwidth * globalid.y] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[globalid.x + imgwidth * globalid.y] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = globalid.x + imgwidth * globalid.y * rngseed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[globalid.x + imgwidth * globalid.y];
        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble);
#endif

        // Generate sample
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);

        // Calculate [0..1] image plane sample
        float2 imgsample;
        imgsample.x = (float)globalid.x / imgwidth + sample0.x / imgwidth;
        imgsample.y = (float)globalid.y / imgheight + sample0.y / imgheight;

        // Transform into [-0.5, 0.5]
        float2 hsample = imgsample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 csample = hsample * camera->dim;

        // Calculate direction to image plane
        myray->d.xyz = normalize(camera->focal_length * camera->forward + csample.x * camera->right + csample.y * camera->up);
        // Origin == camera position + nearz * d
        myray->o.xyz = camera->p + camera->zcap.x * myray->d.xyz;
        // Max T value = zfar - znear since we moved origin to znear
        myray->o.w = camera->zcap.y - camera->zcap.x;
        // Generate random time from 0 to 1
        myray->d.w = sample0.x;
        // Set ray max
        myray->extra.x = 0xFFFFFFFF;
        myray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(myray, 1.f);

#ifndef NO_PATH_DATA
        mypath->throughput = make_float3(1.f, 1.f, 1.f);
        mypath->volume = -1;
        mypath->flags = 0;
        mypath->active = 0xFF;
#endif
    }
}

/// Ray generation kernel for perspective camera.
/// Rays are generated from camera position to viewing plane
/// using random sample distribution within the pixel.
///
__kernel void PerspectiveCameraDof_GeneratePaths(
    // Camera descriptor
    __global Camera const* camera,
    // Image resolution
    int imgwidth,
    int imgheight,
    // RNG seed value
    uint rngseed,
    // Output rays
    __global ray* rays,
    __global uint* random,
    __global uint const* sobolmat,
    int frame
#ifndef NO_PATH_DATA
    , __global Path* paths
#endif
    )
{
    int2 globalid;
    globalid.x = get_global_id(0);
    globalid.y = get_global_id(1);

    // Check borders
    if (globalid.x < imgwidth && globalid.y < imgheight)
    {
        // Get pointer to ray to handle
        __global ray* myray = rays + globalid.y * imgwidth + globalid.x;

#ifndef NO_PATH_DATA
        __global Path* mypath = paths + globalid.y * imgwidth + globalid.x;
#endif

        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[globalid.x + imgwidth * globalid.y] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[globalid.x + imgwidth * globalid.y] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = globalid.x + imgwidth * globalid.y * rngseed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[globalid.x + imgwidth * globalid.y];
        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble);
#endif

        // Generate pixel and lens samples
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);
        float2 sample1 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);


        // Calculate [0..1] image plane sample
        float2 imgsample;
        imgsample.x = (float)globalid.x / imgwidth + sample0.x / imgwidth;
        imgsample.y = (float)globalid.y / imgheight + sample0.y / imgheight;

        // Transform into [-0.5, 0.5]
        float2 hsample = imgsample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 csample = hsample * camera->dim;


        float2 lsample = camera->aperture * Sample_MapToDiskConcentric(sample1);
        float2 fpsample = csample * camera->focus_distance / camera->focal_length;
        float2 cdir = fpsample - lsample;

        float3 o = camera->p + lsample.x * camera->right + lsample.y * camera->up;
        float3 d = normalize(camera->forward * camera->focus_distance + camera->right * cdir.x + camera->up * cdir.y);


        // Calculate direction to image plane
        myray->d.xyz = d;
        // Origin == camera position + nearz * d
        myray->o.xyz = o;
        // Max T value = zfar - znear since we moved origin to znear
        myray->o.w = camera->zcap.y - camera->zcap.x;
        // Generate random time from 0 to 1
        myray->d.w = sample0.x;
        // Set ray max
        myray->extra.x = 0xFFFFFFFF;
        myray->extra.y = 0xFFFFFFFF;

#ifndef NO_PATH_DATA
        mypath->throughput = make_float3(1.f, 1.f, 1.f);
        mypath->volume = -1;
        mypath->flags = 0;
        mypath->active = 0xFF;
#endif
    }
}

#endif // CAMERA_CL
