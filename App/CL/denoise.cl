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
#ifndef DENOISE_CL
#define DENOISE_CL

#include <../App/CL/common.cl>

// Similarity function
inline float C(float3 x1, float3 x2, float sigma)
{
    float a = length(x1 - x2) / sigma;
    a *= a;
    return native_exp(-0.5f * a);
}

// TODO: Optimize this kernel using local memory
KERNEL
void BilateralDenoise_main(
    // Color data
    GLOBAL float4 const* restrict colors,
    // Normal data
    GLOBAL float4 const* restrict normals,
    // Positional data
    GLOBAL float4 const* restrict positions,
    // Albedo data
    GLOBAL float4 const* restrict albedos,
    // Image resolution
    int width,
    int height,
    // Filter radius
    int radius,
    // Filter kernel width
    float sigma_color,
    float sigma_normal,
    float sigma_position,
    float sigma_albedo,
    // Resulting color
    GLOBAL float4* restrict out_colors
)
{
    int2 global_id;
    global_id.x = get_global_id(0);
    global_id.y = get_global_id(1);

    // Check borders
    if (global_id.x < width && global_id.y < height)
    {
        int idx = global_id.y * width + global_id.x;

        float3 color = colors[idx].xyz / colors[idx].w;
        float3 normal = normals[idx].xyz / normals[idx].w;
        float3 position = positions[idx].xyz / positions[idx].w;
        float3 albedo = albedos[idx].xyz / albedos[idx].w;

        float3 filtered_color = 0.f;
        float sum = 0.f;
        if (length(normal) > 0.f && radius > 0)
        {
            for (int i = -radius; i <= radius; ++i)
            {
                for (int j = -radius; j <= radius; ++j)
                {
                    int cx = clamp(global_id.x + i, 0, width - 1);
                    int cy = clamp(global_id.y + j, 0, height - 1);
                    int ci = cy * width + cx;

                    float3 c = colors[ci].xyz / colors[ci].w;
                    float3 n = normals[ci].xyz / normals[ci].w;
                    float3 p = positions[ci].xyz / positions[ci].w;
                    float3 a = albedos[ci].xyz / albedos[ci].w;

                    if (length(n) > 0.f)
                    {
                        filtered_color += c * C(p, position, sigma_position) *
                        C(c, color, sigma_color) *
                        C(n, normal, sigma_normal) *
                        C(a, albedo, sigma_albedo);
                        sum += C(p, position, sigma_position) * C(c, color, sigma_color) *
                        C(n, normal, sigma_normal) *
                        C(a, albedo, sigma_albedo);
                    }
                }
            }

            out_colors[idx].xyz = sum > 0 ? filtered_color / sum : 0.f;
            out_colors[idx].w = 1.f;
        }
        else
        {
            out_colors[idx].xyz = color;
            out_colors[idx].w = 1.f;
        }
    }
}

#endif
