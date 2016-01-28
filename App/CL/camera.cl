//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#ifndef CAMERA_CL
#define CAMERA_CL

#include <../App/CL/payload.cl>
#include <../App/CL/random.cl>


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
__kernel void PerspectiveCamera_GenerateRays(
                             // Camera descriptor
                             __global Camera const* camera,
                             // Image resolution
                             int imgwidth,
                             int imgheight,
                             // RNG seed value
                             int randseed,
                             // Output rays
                             __global ray* rays)
{
    int2 globalid;
    globalid.x  = get_global_id(0);
    globalid.y  = get_global_id(1);
    
    // Check borders
    if (globalid.x < imgwidth && globalid.y < imgheight)
    {
        // Get pointer to ray to handle
        __global ray* myray = rays + globalid.y * imgwidth + globalid.x;
        
        // Prepare RNG
        Rng rng;
        InitRng(randseed +  globalid.x * 157 - 33 * globalid.y, &rng);
        
        // Calculate [0..1] image plane sample
        float2 imgsample;
        imgsample.x = (float)globalid.x / imgwidth + RandFloat(&rng) / imgwidth;
        imgsample.y = (float)globalid.y / imgheight + RandFloat(&rng) / imgheight;
        
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
        myray->d.w = RandFloat(&rng);
		// Set ray max
		myray->extra.x = 0xFFFFFFFF;
		myray->extra.y = 0xFFFFFFFF;
    }
}

#endif // CAMERA_CL
