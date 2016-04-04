/**********************************************************************
Copyright �2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

?   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
?   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
							 int reset,
							 __global Path* paths
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
		__global Path* mypath = paths + globalid.y * imgwidth + globalid.x;
        
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

		mypath->throughput = make_float3(1.f, 1.f, 1.f);
		mypath->volume = -1;
		mypath->flags = 0;
		mypath->active = 0xFF;
    }
}

#endif // CAMERA_CL
