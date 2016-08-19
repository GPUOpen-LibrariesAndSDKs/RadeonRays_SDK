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

#define MAX_BAND 2
#define PI 3.14159265358979323846f

enum TextureFormat
{
    UNKNOWN,
    RGBA8,
    RGBA16,
    RGBA32
};

// Texture description
typedef struct _Texture
{
    // Texture width, height and depth
    int w;
    int h;
    int d;
    // Offset in texture data array
    int dataoffset;
    int fmt;
    int extra;
} Texture;


float3 make_float3(float x, float y, float z)
{
    float3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}


void CartesianToSpherical ( float3 cart, float* r, float* phi, float* theta )
{
    float temp = atan2(cart.x, cart.z);
    *r = sqrt(cart.x*cart.x + cart.y*cart.y + cart.z*cart.z);
    *phi = (float)((temp >= 0)?temp:(temp + 2*PI));
    *theta = acos(cart.y/ *r);
}


/// Sample 2D texture described by texture in texturedata pool
float3 Sample2D(Texture const* texture, __global char const* texturedata, float2 uv)
{
    // Get width and height
    int width = texture->w;
    int height = texture->h;

    // Find the origin of the data in the pool
    __global char const* mydata = texturedata + texture->dataoffset;

    // Handle wrap
    uv -= floor(uv);

    // Reverse Y
    // it is needed as textures are loaded with Y axis going top to down
    // and our axis goes from down to top
    uv.y = 1.f - uv.y;

    // Figure out integer coordinates
    int x = floor(uv.x * width);
    int y = floor(uv.y * height);

    // Calculate samples for linear filtering
    int x1 = min(x + 1, width - 1);
    int y1 = min(y + 1, height - 1);

    // Calculate weights for linear filtering
    float wx = uv.x * width - floor(uv.x * width);
    float wy = uv.y * height - floor(uv.y * height);

    if (texture->fmt == RGBA32)
    {
        __global float3 const* mydataf = (__global float3 const*)mydata;

        // Get 4 values
        float3 valx = *(mydataf + width * y + x);

        // Filter and return the result
        return valx;
    }
    else if (texture->fmt == RGBA16)
    {
        __global half const* mydatah = (__global half const*)mydata;

        float valx = vload_half(0, mydatah + 4*(width * y + x));
        float valy = vload_half(0, mydatah + 4*(width * y + x) + 1);
        float valz = vload_half(0, mydatah + 4*(width * y + x) + 2);

        return make_float3(valx, valy, valz);
    }
    else
    {
        __global uchar4 const* mydatac = (__global uchar4 const*)mydata;

        // Get 4 values
        uchar4 valx = *(mydatac + width * y + x);

        float3 valxf = make_float3((float)valx.x / 255.f, (float)valx.y / 255.f, (float)valx.z / 255.f);

        // Filter and return the result
        return valxf;
    }
}


void ShEvaluate(float3 p, float* coeffs) 
{
                     float fC0, fC1, fS0, fS1, fTmpA, fTmpB, fTmpC;
                     float pz2 = p.z * p.z;
                     coeffs[0] = 0.2820947917738781f;
                     coeffs[2] = 0.4886025119029199f * p.z;
                     coeffs[6] = 0.9461746957575601f * pz2 + -0.3153915652525201f;
                     fC0 = p.x;
                     fS0 = p.y;
                     fTmpA = -0.48860251190292f;
                     coeffs[3] = fTmpA * fC0;
                     coeffs[1] = fTmpA * fS0;
                     fTmpB = -1.092548430592079f * p.z;
                     coeffs[7] = fTmpB * fC0;
                     coeffs[5] = fTmpB * fS0;
                     fC1 = p.x*fC0 - p.y*fS0;
                     fS1 = p.x*fS0 + p.y*fC0;
                     fTmpC = 0.5462742152960395f;
                     coeffs[8] = fTmpC * fC1;
                     coeffs[4] = fTmpC * fS1;
}

__attribute__((reqd_work_group_size(8, 8, 1)))
///< Project the function represented by lat-long map lmmap to Sh up to lmax band
__kernel void ShProject(
    __global Texture const* textures,
    // Texture data
    __global char const* texturedata,
    // Environment texture index
    int envmapidx,
    // Harmonic coefficients flattened: NumShTerms(lmax) * num_groups
    __global float3* coeffs
    )
{
    // Temporary work area for trigonom results
    __local float3 cx[64];

    int x = get_global_id(0);
    int y = get_global_id(1);

    int xl = get_local_id(0);
    int yl = get_local_id(1);
    int wl = get_local_size(0);

    int ngx = get_num_groups(0);
    int gx = get_group_id(0);
    int gy = get_group_id(1);
    int g = gy * ngx + gx;

    int lid = yl * wl + xl;

    Texture envmap = textures[envmapidx];
    int w = envmap.w;
    int h = envmap.h;

    if (x < w && y < h)
    {
        // Calculate spherical angles
        float thetastep = PI / h;
        float phistep = 2.f*PI / w;
        float theta0 = 0;//PI / h / 2.f;
        float phi0 = 0;//2.f*PI / w / 2.f;

        float phi = phi0 + x * phistep;
        float theta = theta0 + y * thetastep;

        float2 uv;
        uv.x = (float)x/w;
        uv.y = 1.f - (float)y/h;
        float3 le = 3.f * Sample2D(&envmap, texturedata, uv);

        float sinphi = sin(phi);
        float cosphi = cos(phi);
        float costheta = cos(theta);
        float sintheta = sin(theta);

        // Construct point on unit sphere
        float3 p = normalize(make_float3(sintheta * cosphi, costheta, sintheta * sinphi));

        // Evaluate SH functions at w up to lmax band
        float ylm[9];
        ShEvaluate(p, ylm);

        // Evaluate Riemann sum
        for (int i = 0; i < 9; ++i)
        {
            // Calculate the coefficient into local memory
             cx[lid] = le * ylm[i] * sintheta * (PI / h) * (2.f * PI / w);

             barrier(CLK_LOCAL_MEM_FENCE);

             // Reduce the coefficient to get the resulting one
             for (int stride = 1; stride <= (64 >> 1); stride <<= 1)
             {
                 if (lid < 64/(2*stride))
                 {
                     cx[2*(lid + 1)*stride-1] = cx[2*(lid + 1)*stride-1] + cx[(2*lid + 1)*stride-1];
                 }

                 barrier(CLK_LOCAL_MEM_FENCE);
             }

             // Put the coefficient into global memory
             if (lid == 0)
             {
                coeffs[g * 9 + i] = cx[63];
             }

             barrier(CLK_LOCAL_MEM_FENCE);
        }
    }
}

__attribute__((reqd_work_group_size(8, 8, 1)))
///< Project the function represented by lat-long map lmmap to Sh up to lmax band
__kernel void ShProjectHemisphericalProbe(
    __global Texture const* textures,
    // Texture data
    __global char const* texturedata,
    // Environment texture index
    int envmapidx,
    // Harmonic coefficients flattened: NumShTerms(lmax) * num_groups
    __global float3* coeffs
    )
{
    // Temporary work area for trigonom results
    __local float3 cx[64];

    int x = get_global_id(0);
    int y = get_global_id(1);

    int xl = get_local_id(0);
    int yl = get_local_id(1);
    int wl = get_local_size(0);

    int ngx = get_num_groups(0);
    int gx = get_group_id(0);
    int gy = get_group_id(1);
    int g = gy * ngx + gx;

    int lid = yl * wl + xl;

    Texture envmap = textures[envmapidx];
    int w = envmap.w;
    int h = envmap.h;

    if (x < w && y < h)
    {
        // Calculate spherical angles
        float thetastep = PI / h;
        float phistep = 2.f*PI / w;
        float theta0 = 0;//PI / h / 2.f;
        float phi0 = 0;//2.f*PI / w / 2.f;

        float phi = phi0 + x * phistep;
        float theta = theta0 + y * thetastep;

        float sinphi = sin(phi);
        float cosphi = cos(phi);
        float costheta = cos(theta);
        float sintheta = sin(theta);

        // Construct point on unit sphere
        float3 p = normalize(make_float3(sintheta * cosphi, costheta, sintheta * sinphi));

        float envmapaspect = (float)envmap.h / envmap.w;
        float2 uv = p.xz;
        uv.y = 0.5f*uv.y + 0.5f;
        uv.x = 0.5f*uv.x + 0.5f;
        uv.x = (1.f - envmapaspect) * 0.5f + uv.x * envmapaspect;

        //uv.x = (float)x/w;
        //uv.y = 1.f - (float)y/h;
        float3 le = Sample2D(&envmap, texturedata, uv);

        // Evaluate SH functions at w up to lmax band
        float ylm[9];
        ShEvaluate(p, ylm);

        // Evaluate Riemann sum
        for (int i = 0; i < 9; ++i)
        {
            // Calculate the coefficient into local memory
             cx[lid] = le * ylm[i] * sintheta * (PI / h) * (2.f * PI / w);

             barrier(CLK_LOCAL_MEM_FENCE);

             // Reduce the coefficient to get the resulting one
             for (int stride = 1; stride <= (64 >> 1); stride <<= 1)
             {
                 if (lid < 64/(2*stride))
                 {
                     cx[2*(lid + 1)*stride-1] = cx[2*(lid + 1)*stride-1] + cx[(2*lid + 1)*stride-1];
                 }

                 barrier(CLK_LOCAL_MEM_FENCE);
             }

             // Put the coefficient into global memory
             if (lid == 0)
             {
                coeffs[g * 9 + i] = cx[63];
             }

             barrier(CLK_LOCAL_MEM_FENCE);
        }
    }
}


#define GROUP_SIZE 256
__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1)))
__kernel void ShReduce(
    // Harmonic coefficients flattened: NumShTerms(lmax) * num_groups
    const __global float3* coeffs,
    // Number of sets
    int numsets,
    // Resulting coeffs
    __global float3* result
    )
{
    __local float3 lds[GROUP_SIZE];

    int gid = get_global_id(0);
    int lid = get_global_id(0);

    // How many items to reduce for a single work item
    int numprivate = numsets / GROUP_SIZE;

    for (int i=0;i<9;++i)
    {
        float3 res = {0,0,0};

        // Private reduction
        for (int j=0;j<numprivate;++j)
        {
            res += coeffs[gid * numprivate * 9 + j * 9 + i];
        }

        // LDS reduction
        lds[lid] = res;

        barrier (CLK_LOCAL_MEM_FENCE);

        // Work group reduction
        for (int stride = 1; stride <= (256 >> 1); stride <<= 1)
        {
            if (lid < GROUP_SIZE/(2*stride))
            {
                lds[2*(lid + 1)*stride-1] = lds[2*(lid + 1)*stride-1] + lds[(2*lid + 1)*stride-1];
            }

            barrier (CLK_LOCAL_MEM_FENCE);
        }

        // Write final result
        if (lid == 0)
        {
            result[i] = lds[GROUP_SIZE-1];
        }

        barrier (CLK_LOCAL_MEM_FENCE);
    }
}
#undef GROUP_SIZE


__attribute__((reqd_work_group_size(8, 8, 1)))
__kernel void ShReconstructLmmap(
    // SH coefficients: NumShTerms(lmax) items
    const __global float3* coeffs,
    // Resulting image width
    int w,
    // Resulting image height
    int h,
    // Resulting image
    __global float3* lmmap
    )
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    int xl = get_local_id(0);
    int yl = get_local_id(1);
    int wl = get_local_size(0);

    int ngx = get_num_groups(0);
    int gx = get_group_id(0);
    int gy = get_group_id(1);

    if (x < w && y < h)
    {
        // Calculate spherical angles
        float thetastep = M_PI / h;
        float phistep = 2.f*M_PI / w;
        float theta0 = 0;//M_PI / h / 2.f;
        float phi0 = 0;//2.f*M_PI / w / 2.f;

        float phi = phi0 + x * phistep;
        float theta = theta0 + y * thetastep;

        float2 uv;
        uv.x = (float)x/w;
        uv.y = (float)y/h;

        float sinphi = sin(phi);
        float cosphi = cos(phi);
        float costheta = cos(theta);
        float sintheta = sin(theta);

        // Construct point on unit sphere
        float3 p = normalize(make_float3(sintheta * cosphi, costheta, sintheta * sinphi));

        // Evaluate SH functions at w up to lmax band
        float ylm[9];
        ShEvaluate(p, ylm);

        // Evaluate Riemann sum
        float3 val = {0, 0, 0};
        for (int i = 0; i < 9; ++i)
        {
            // Calculate the coefficient into local memory
             val += ylm[i] * coeffs[i];
        }

        int x = floor(uv.x * w);
        int y = floor(uv.y * h);

        lmmap[w * y + x] = val;
    }
}