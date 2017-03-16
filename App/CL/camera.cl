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

// Pinhole camera implementation.
// This kernel is being used if aperture value = 0.
KERNEL 
void PerspectiveCamera_GeneratePaths(
    // Camera
    GLOBAL Camera const* restrict camera,
    // Image resolution
    int img_width,
    int img_height,
    // RNG seed value
    uint rng_seed,
    // Current frame
    uint frame,
    // Rays to generate
    GLOBAL ray* rays,
    // RNG data
    GLOBAL uint* random,
    GLOBAL uint const* restrict sobolmat,
    // Path buffer
    GLOBAL Path* paths
)
{
    int2 global_id;
    global_id.x  = get_global_id(0);
    global_id.y  = get_global_id(1);

    // Check borders
    if (global_id.x < img_width && global_id.y < img_height)
    {
        // Get pointer to ray & path handles
        GLOBAL ray* my_ray = rays + global_id.y * img_width + global_id.x;
        GLOBAL Path* my_path = paths + global_id.y * img_width + global_id.x;

        // Initialize sampler
        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[global_id.x + img_width * global_id.y] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[global_id.x + img_width * global_id.y] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = global_id.x + img_width * global_id.y * rng_seed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[global_id.x + img_width * global_id.y];
        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble);
#endif

        // Generate sample
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);

        // Calculate [0..1] image plane sample
        float2 img_sample;
        img_sample.x = (float)global_id.x / img_width + sample0.x / img_width;
        img_sample.y = (float)global_id.y / img_height + sample0.y / img_height;

        // Transform into [-0.5, 0.5]
        float2 h_sample = img_sample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 c_sample = h_sample * camera->dim;

        // Calculate direction to image plane
        my_ray->d.xyz = normalize(camera->focal_length * camera->forward + c_sample.x * camera->right + c_sample.y * camera->up);
        // Origin == camera position + nearz * d
        my_ray->o.xyz = camera->p + camera->zcap.x * my_ray->d.xyz;
        // Max T value = zfar - znear since we moved origin to znear
        my_ray->o.w = camera->zcap.y - camera->zcap.x;
        // Generate random time from 0 to 1
        my_ray->d.w = sample0.x;
        // Set ray max
        my_ray->extra.x = 0xFFFFFFFF;
        my_ray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(my_ray, 1.f);

        // Initalize path data
        my_path->throughput = make_float3(1.f, 1.f, 1.f);
        my_path->volume = INVALID_IDX;
        my_path->flags = 0;
        my_path->active = 0xFF;
    }
}

// Physical camera implemenation.
// This kernel is being used if aperture > 0.
KERNEL void PerspectiveCameraDof_GeneratePaths(
    // Camera
    GLOBAL Camera const* camera,
    // Image resolution
    int img_width,
    int img_height,
    // RNG seed value
    uint rng_seed,
    // Current frame
    uint frame,
    // Rays to generate
    GLOBAL ray* rays,
    // RNG data
    GLOBAL uint* random,
    GLOBAL uint const* restrict sobolmat,
    GLOBAL Path* paths
    )
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);

    // Check borders
    if (global_id.x < img_width && global_id.y < img_height)
    {
        // Get pointer to ray & path handles
        GLOBAL ray* my_ray = rays + global_id.y * img_width + global_id.x;
        GLOBAL Path* my_path = paths + global_id.y * img_width + global_id.x;

        // Initialize sampler
        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[global_id.x + img_width * global_id.y] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[global_id.x + img_width * global_id.y] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = global_id.x + img_width * global_id.y * rng_seed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[global_id.x + img_width * global_id.y];
        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble);
#endif

        // Generate pixel and lens samples
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);
        float2 sample1 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);

        // Calculate [0..1] image plane sample
        float2 img_sample;
        img_sample.x = (float)global_id.x / img_width + sample0.x / img_width;
        img_sample.y = (float)global_id.y / img_height + sample0.y / img_height;

        // Transform into [-0.5, 0.5]
        float2 h_sample = img_sample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 c_sample = h_sample * camera->dim;

        // Generate sample on the lens
        float2 lens_sample = camera->aperture * Sample_MapToDiskConcentric(sample1);
        // Calculate position on focal plane
        float2 focal_plane_sample = c_sample * camera->focus_distance / camera->focal_length;
        // Calculate ray direction
        float2 camera_dir = focal_plane_sample - lens_sample;

        // Calculate direction to image plane
        my_ray->d.xyz = normalize(camera->forward * camera->focus_distance + camera->right * camera_dir.x + camera->up * camera_dir.y);
        // Origin == camera position + nearz * d
        my_ray->o.xyz = camera->p + lens_sample.x * camera->right + lens_sample.y * camera->up;
        // Max T value = zfar - znear since we moved origin to znear
        my_ray->o.w = camera->zcap.y - camera->zcap.x;
        // Generate random time from 0 to 1
        my_ray->d.w = sample0.x;
        // Set ray max
        my_ray->extra.x = 0xFFFFFFFF;
        my_ray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(my_ray, 1.f);

        // Initlize path data
        my_path->throughput = make_float3(1.f, 1.f, 1.f);
        my_path->volume = -1;
        my_path->flags = 0;
        my_path->active = 0xFF;
    }
}

#endif // CAMERA_CL
