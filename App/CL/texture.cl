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
#ifndef TEXTURE_CL
#define TEXTURE_CL

#include <../App/CL/utils.cl>

/// Supported formats
enum TextureFormat
{
    UNKNOWN,
    RGBA8,
    RGBA16,
    RGBA32
};

/// Texture description
typedef
    struct _Texture
    {
        // Width, height and depth
        int w;
        int h;
        int d;
        // Offset in texture data array
        int dataoffset;
        // Format
        int fmt;
        int extra;
    } Texture;

/// To simplify a bit
#define TEXTURE_ARG_LIST __global Texture const* textures, __global char const* texturedata
#define TEXTURE_ARG_LIST_IDX(x) int x, __global Texture const* textures, __global char const* texturedata
#define TEXTURE_ARGS textures, texturedata
#define TEXTURE_ARGS_IDX(x) x, textures, texturedata

/// Sample 2D texture
float3 Texture_Sample2D(float2 uv, TEXTURE_ARG_LIST_IDX(texidx))
{
    // Get width and height
    int width = textures[texidx].w;
    int height = textures[texidx].h;
    
    // Find the origin of the data in the pool
    __global char const* mydata = texturedata + textures[texidx].dataoffset;

    // Handle UV wrap
    // TODO: need UV mode support
    uv -= floor(uv);
    
    // Reverse Y:
    // it is needed as textures are loaded with Y axis going top to down
    // and our axis goes from down to top
    uv.y = 1.f - uv.y;
    
    // Calculate integer coordinates
    int x0 = clamp((int)floor(uv.x * width), 0, width - 1);
    int y0 = clamp((int)floor(uv.y * height), 0, height - 1);
    
    // Calculate samples for linear filtering
    int x1 = clamp(x0 + 1, 0,  width - 1);
    int y1 = clamp(y0 + 1, 0, height - 1);
    
    // Calculate weights for linear filtering
    float wx = uv.x * width - floor(uv.x * width);
    float wy = uv.y * height - floor(uv.y * height);
    
    switch (textures[texidx].fmt)
    {
        case RGBA32:
        {
            __global float3 const* mydataf = (__global float3 const*)mydata;
            
            // Get 4 values for linear filtering
            float3 val00 = *(mydataf + width * y0 + x0);
            float3 val01 = *(mydataf + width * y0 + x1);
            float3 val10 = *(mydataf + width * y1 + x0);
            float3 val11 = *(mydataf + width * y1 + x1);
            
            // Filter and return the result
            return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy);
        }
            
        case RGBA16:
        {
            __global half const* mydatah = (__global half const*)mydata;
            
            // Get 4 values
            float3 val00 = vload_half4(width * y0 + x0, mydatah).xyz;
            float3 val01 = vload_half4(width * y0 + x1, mydatah).xyz;
            float3 val10 = vload_half4(width * y1 + x0, mydatah).xyz;
            float3 val11 = vload_half4(width * y1 + x1, mydatah).xyz;
            
            // Filter and return the result
            return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy);
        }
            
        case RGBA8:
        {
            __global uchar4 const* mydatac = (__global uchar4 const*)mydata;
            
            // Get 4 values and convert to float
            uchar4 valu00 = *(mydatac + width * y0 + x0);
            uchar4 valu01 = *(mydatac + width * y0 + x1);
            uchar4 valu10 = *(mydatac + width * y1 + x0);
            uchar4 valu11 = *(mydatac + width * y1 + x1);
            
            float3 val00 = make_float3((float)valu00.x / 255.f, (float)valu00.y / 255.f, (float)valu00.z / 255.f);
            float3 val01 = make_float3((float)valu01.x / 255.f, (float)valu01.y / 255.f, (float)valu01.z / 255.f);
            float3 val10 = make_float3((float)valu10.x / 255.f, (float)valu10.y / 255.f, (float)valu10.z / 255.f);
            float3 val11 = make_float3((float)valu11.x / 255.f, (float)valu11.y / 255.f, (float)valu11.z / 255.f);
            
            // Filter and return the result
            return lerp(lerp(val00, val01, wx), lerp(val10, val11, wx), wy);
        }
            
        default:
        {
            return make_float3(0.f, 0.f, 0.f);
        }
    }
}

/// Sample lattitue-longitude environment map using 3d vector
float3 Texture_SampleEnvMap(float3 d, TEXTURE_ARG_LIST_IDX(texidx))
{
    // Transform to spherical coords
    float r, phi, theta;
    CartesianToSpherical(d, &r, &phi, &theta);
    
    // Map to [0,1]x[0,1] range and reverse Y axis
    float2 uv;
    uv.x = phi / (2*PI);
    uv.y = 1.f - theta / PI;
    
    // Sample the texture
    return native_powr(Texture_Sample2D(uv, TEXTURE_ARGS_IDX(texidx)), 1.f / 1.f);
}

/// Get data from parameter value or texture
float3 Texture_GetValue3f(
                // Value
                float3 v,
                // Texture coordinate
                float2 uv,
                // Texture args
                TEXTURE_ARG_LIST_IDX(texidx)
                )
{
    // If texture present sample from texture
    if (texidx != -1)
    {
        // Sample texture
        return native_powr(Texture_Sample2D(uv, TEXTURE_ARGS_IDX(texidx)), 2.2f);
    }
    
    // Return fixed color otherwise
    return v;
}

/// Get data from parameter value or texture
float Texture_GetValue1f(
                        // Value
                        float v,
                        // Texture coordinate
                        float2 uv,
                        // Texture args
                        TEXTURE_ARG_LIST_IDX(texidx)
                        )
{
    // If texture present sample from texture
    if (texidx != -1)
    {
        // Sample texture
        return Texture_Sample2D(uv, TEXTURE_ARGS_IDX(texidx)).x;
    }
    
    // Return fixed color otherwise
    return v;
}

/// Sample 2D texture
float3 Texture_SampleBump(float2 uv, TEXTURE_ARG_LIST_IDX(texidx))
{
    // Get width and height
    int width = textures[texidx].w;
    int height = textures[texidx].h;

    // Find the origin of the data in the pool
    __global char const* mydata = texturedata + textures[texidx].dataoffset;

    // Handle UV wrap
    // TODO: need UV mode support
    uv -= floor(uv);

    // Reverse Y:
    // it is needed as textures are loaded with Y axis going top to down
    // and our axis goes from down to top
    uv.y = 1.f - uv.y;

    // Calculate integer coordinates
    int s0 = clamp((int)floor(uv.x * width), 0, width - 1);
    int t0 = clamp((int)floor(uv.y * height), 0, height - 1);

    switch (textures[texidx].fmt)
    {
    case RGBA32:
    {
        __global float3 const* mydataf = (__global float3 const*)mydata;

        // Sobel filter
        const float tex00 = (*(mydataf + width * (t0 - 1) + (s0-1))).x; 
        const float tex10 = (*(mydataf + width * (t0 - 1) + (s0))).x;
        const float tex20 = (*(mydataf + width * (t0 - 1) + (s0 + 1))).x; 

        const float tex01 = (*(mydataf + width * (t0) + (s0 - 1))).x; 
        const float tex21 = (*(mydataf + width * (t0) + (s0 + 1))).x;

        const float tex02 = (*(mydataf + width * (t0 + 1) + (s0 - 1))).x;
        const float tex12 = (*(mydataf + width * (t0 + 1) + (s0))).x;
        const float tex22 = (*(mydataf + width * (t0 + 1) + (s0 + 1))).x;

        const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22;
        const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22;
        const float3 n = make_float3(Gx, Gy, 1.f);

        return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    case RGBA16:
    {
        __global half const* mydatah = (__global half const*)mydata;

        const float tex00 = vload_half4(width * (t0 - 1) + (s0 - 1), mydatah).x;
        const float tex10 = vload_half4(width * (t0 - 1) + (s0), mydatah).x;
        const float tex20 = vload_half4(width * (t0 - 1) + (s0 + 1), mydatah).x;

        const float tex01 = vload_half4(width * (t0)+(s0 - 1), mydatah).x; 
        const float tex21 = vload_half4(width * (t0)+(s0 + 1), mydatah).x; 

        const float tex02 = vload_half4(width * (t0 + 1) + (s0 - 1), mydatah).x;
        const float tex12 = vload_half4(width * (t0 + 1) + (s0), mydatah).x;
        const float tex22 = vload_half4(width * (t0 + 1) + (s0 + 1), mydatah).x;

        const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22;
        const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22;
        const float3 n = make_float3(Gx, Gy, 1.f);

        return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    case RGBA8:
    {
        __global uchar4 const* mydatac = (__global uchar4 const*)mydata;

        const uchar utex00 = (*(mydatac + width * (t0 - 1) + (s0 - 1))).x;
        const uchar utex10 = (*(mydatac + width * (t0 - 1) + (s0))).x;
        const uchar utex20 = (*(mydatac + width * (t0 - 1) + (s0 + 1))).x;

        const uchar utex01 = (*(mydatac + width * (t0)+(s0 - 1))).x;
        const uchar utex21 = (*(mydatac + width * (t0)+(s0 + 1))).x;

        const uchar utex02 = (*(mydatac + width * (t0 + 1) + (s0 - 1))).x;
        const uchar utex12 = (*(mydatac + width * (t0 + 1) + (s0))).x;
        const uchar utex22 = (*(mydatac + width * (t0 + 1) + (s0 + 1))).x;

        const float tex00 = (float)utex00 / 255.f;
        const float tex10 = (float)utex10 / 255.f;
        const float tex20 = (float)utex20 / 255.f;

        const float tex01 = (float)utex01 / 255.f;
        const float tex21 = (float)utex21 / 255.f;

        const float tex02 = (float)utex02 / 255.f;
        const float tex12 = (float)utex12 / 255.f;
        const float tex22 = (float)utex22 / 255.f;

        const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22;
        const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22;
        const float3 n = make_float3(Gx, Gy, 1.f);

        return 0.5f * normalize(n) + make_float3(0.5f, 0.5f, 0.5f);
    }

    default:
    {
        return make_float3(0.f, 0.f, 0.f);
    }
    }
}



#endif // TEXTURE_CL
