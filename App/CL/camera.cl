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

#include <../App/CL/payload.cl>
#include <../App/CL/random.cl>
#include <../App/CL/sampling.cl>
#include <../App/CL/utils.cl>
#include <../App/CL/path.cl>


/// Camera descriptor
///
typedef struct _Camera
    {
        // Camera coordinate frame
        float3 forward;
        float3 right;
        float3 up;
        float3 p;
        
        // Image plane width & height in current units
        float2 dim;
        
        // Near and far Z
        float2 zcap;
        // Vertical field of view in radians
        float  fovy;
        // Camera aspect ratio
        float  aspect;
    } Camera;


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
                             int randseed,
                             // Output rays
                             __global ray* rays,
							 __global SobolSampler* samplers,
							 __global uint const* sobolmat,
							 int reset
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
        
        // Prepare RNG
        Rng rng;
        InitRng(randseed +  globalid.x * 157 + 10433 * globalid.y, &rng);

#ifdef SOBOL
		__global SobolSampler* sampler = samplers + globalid.y * imgwidth + globalid.x;

		if (reset)
		{
			sampler->seq = 0;
			sampler->s0 = RandUint(&rng);
		}
		else
		{
			sampler->seq++;
		}

		float2 sample0;
		sample0.x = SobolSampler_Sample1D(sampler->seq, kPixelX, sampler->s0, sobolmat);
		sample0.y = SobolSampler_Sample1D(sampler->seq, kPixelY, sampler->s0, sobolmat);
#else
		float2 sample0 = UniformSampler_Sample2D(&rng);
#endif
        
        // Calculate [0..1] image plane sample
        float2 imgsample;
        imgsample.x = (float)globalid.x / imgwidth + sample0.x / imgwidth;
        imgsample.y = (float)globalid.y / imgheight + sample0.y / imgheight;
        
        // Transform into [-0.5, 0.5]
        float2 hsample = imgsample - make_float2(0.5f, 0.5f);
        // Transform into [-dim/2, dim/2]
        float2 csample = hsample * camera->dim;
        
        // Calculate direction to image plane
        myray->d.xyz = normalize(camera->zcap.x * camera->forward + csample.x * camera->right + csample.y * camera->up);
        // Origin == camera position + nearz * d
        myray->o.xyz = camera->p + camera->zcap.x * myray->d.xyz;
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
