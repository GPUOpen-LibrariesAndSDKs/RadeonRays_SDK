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

static const char cl_sh[]= \
" \n"\
"#define MAX_BAND 2 \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"enum TextureFormat \n"\
"{ \n"\
"    UNKNOWN, \n"\
"    RGBA8, \n"\
"    RGBA16, \n"\
"    RGBA32 \n"\
"}; \n"\
" \n"\
"// Texture description \n"\
"typedef struct _Texture \n"\
"{ \n"\
"    // Texture width, height and depth \n"\
"    int w; \n"\
"    int h; \n"\
"    int d; \n"\
"    // Offset in texture data array \n"\
"    int dataoffset; \n"\
"    int fmt; \n"\
"    int extra; \n"\
"} Texture; \n"\
" \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
" \n"\
"void CartesianToSpherical ( float3 cart, float* r, float* phi, float* theta ) \n"\
"{ \n"\
"    float temp = atan2(cart.x, cart.z); \n"\
"    *r = sqrt(cart.x*cart.x + cart.y*cart.y + cart.z*cart.z); \n"\
"    *phi = (float)((temp >= 0)?temp:(temp + 2*PI)); \n"\
"    *theta = acos(cart.y/ *r); \n"\
"} \n"\
" \n"\
" \n"\
"/// Sample 2D texture described by texture in texturedata pool \n"\
"float3 Sample2D(Texture const* texture, __global char const* texturedata, float2 uv) \n"\
"{ \n"\
"    // Get width and height \n"\
"    int width = texture->w; \n"\
"    int height = texture->h; \n"\
" \n"\
"    // Find the origin of the data in the pool \n"\
"    __global char const* mydata = texturedata + texture->dataoffset; \n"\
" \n"\
"    // Handle wrap \n"\
"    uv -= floor(uv); \n"\
" \n"\
"    // Reverse Y \n"\
"    // it is needed as textures are loaded with Y axis going top to down \n"\
"    // and our axis goes from down to top \n"\
"    uv.y = 1.f - uv.y; \n"\
" \n"\
"    // Figure out integer coordinates \n"\
"    int x = floor(uv.x * width); \n"\
"    int y = floor(uv.y * height); \n"\
" \n"\
"    // Calculate samples for linear filtering \n"\
"    int x1 = min(x + 1, width - 1); \n"\
"    int y1 = min(y + 1, height - 1); \n"\
" \n"\
"    // Calculate weights for linear filtering \n"\
"    float wx = uv.x * width - floor(uv.x * width); \n"\
"    float wy = uv.y * height - floor(uv.y * height); \n"\
" \n"\
"    if (texture->fmt == RGBA32) \n"\
"    { \n"\
"        __global float3 const* mydataf = (__global float3 const*)mydata; \n"\
" \n"\
"        // Get 4 values \n"\
"        float3 valx = *(mydataf + width * y + x); \n"\
" \n"\
"        // Filter and return the result \n"\
"        return valx; \n"\
"    } \n"\
"    else if (texture->fmt == RGBA16) \n"\
"    { \n"\
"        __global half const* mydatah = (__global half const*)mydata; \n"\
" \n"\
"        float valx = vload_half(0, mydatah + 4*(width * y + x)); \n"\
"        float valy = vload_half(0, mydatah + 4*(width * y + x) + 1); \n"\
"        float valz = vload_half(0, mydatah + 4*(width * y + x) + 2); \n"\
" \n"\
"        return make_float3(valx, valy, valz); \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        __global uchar4 const* mydatac = (__global uchar4 const*)mydata; \n"\
" \n"\
"        // Get 4 values \n"\
"        uchar4 valx = *(mydatac + width * y + x); \n"\
" \n"\
"        float3 valxf = make_float3((float)valx.x / 255.f, (float)valx.y / 255.f, (float)valx.z / 255.f); \n"\
" \n"\
"        // Filter and return the result \n"\
"        return valxf; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"void ShEvaluate(float3 p, float* coeffs)  \n"\
"{ \n"\
"                     float fC0, fC1, fS0, fS1, fTmpA, fTmpB, fTmpC; \n"\
"                     float pz2 = p.z * p.z; \n"\
"                     coeffs[0] = 0.2820947917738781f; \n"\
"                     coeffs[2] = 0.4886025119029199f * p.z; \n"\
"                     coeffs[6] = 0.9461746957575601f * pz2 + -0.3153915652525201f; \n"\
"                     fC0 = p.x; \n"\
"                     fS0 = p.y; \n"\
"                     fTmpA = -0.48860251190292f; \n"\
"                     coeffs[3] = fTmpA * fC0; \n"\
"                     coeffs[1] = fTmpA * fS0; \n"\
"                     fTmpB = -1.092548430592079f * p.z; \n"\
"                     coeffs[7] = fTmpB * fC0; \n"\
"                     coeffs[5] = fTmpB * fS0; \n"\
"                     fC1 = p.x*fC0 - p.y*fS0; \n"\
"                     fS1 = p.x*fS0 + p.y*fC0; \n"\
"                     fTmpC = 0.5462742152960395f; \n"\
"                     coeffs[8] = fTmpC * fC1; \n"\
"                     coeffs[4] = fTmpC * fS1; \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(8, 8, 1))) \n"\
"///< Project the function represented by lat-long map lmmap to Sh up to lmax band \n"\
"__kernel void ShProject( \n"\
"    __global Texture const* textures, \n"\
"    // Texture data \n"\
"    __global char const* texturedata, \n"\
"    // Environment texture index \n"\
"    int envmapidx, \n"\
"    // Harmonic coefficients flattened: NumShTerms(lmax) * num_groups \n"\
"    __global float3* coeffs \n"\
"    ) \n"\
"{ \n"\
"    // Temporary work area for trigonom results \n"\
"    __local float3 cx[64]; \n"\
" \n"\
"    int x = get_global_id(0); \n"\
"    int y = get_global_id(1); \n"\
" \n"\
"    int xl = get_local_id(0); \n"\
"    int yl = get_local_id(1); \n"\
"    int wl = get_local_size(0); \n"\
" \n"\
"    int ngx = get_num_groups(0); \n"\
"    int gx = get_group_id(0); \n"\
"    int gy = get_group_id(1); \n"\
"    int g = gy * ngx + gx; \n"\
" \n"\
"    int lid = yl * wl + xl; \n"\
" \n"\
"    Texture envmap = textures[envmapidx]; \n"\
"    int w = envmap.w; \n"\
"    int h = envmap.h; \n"\
" \n"\
"    if (x < w && y < h) \n"\
"    { \n"\
"        // Calculate spherical angles \n"\
"        float thetastep = PI / h; \n"\
"        float phistep = 2.f*PI / w; \n"\
"        float theta0 = 0;//PI / h / 2.f; \n"\
"        float phi0 = 0;//2.f*PI / w / 2.f; \n"\
" \n"\
"        float phi = phi0 + x * phistep; \n"\
"        float theta = theta0 + y * thetastep; \n"\
" \n"\
"        float2 uv; \n"\
"        uv.x = (float)x/w; \n"\
"        uv.y = 1.f - (float)y/h; \n"\
"        float3 le = 3.f * Sample2D(&envmap, texturedata, uv); \n"\
" \n"\
"        float sinphi = sin(phi); \n"\
"        float cosphi = cos(phi); \n"\
"        float costheta = cos(theta); \n"\
"        float sintheta = sin(theta); \n"\
" \n"\
"        // Construct point on unit sphere \n"\
"        float3 p = normalize(make_float3(sintheta * cosphi, costheta, sintheta * sinphi)); \n"\
" \n"\
"        // Evaluate SH functions at w up to lmax band \n"\
"        float ylm[9]; \n"\
"        ShEvaluate(p, ylm); \n"\
" \n"\
"        // Evaluate Riemann sum \n"\
"        for (int i = 0; i < 9; ++i) \n"\
"        { \n"\
"            // Calculate the coefficient into local memory \n"\
"             cx[lid] = le * ylm[i] * sintheta * (PI / h) * (2.f * PI / w); \n"\
" \n"\
"             barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"             // Reduce the coefficient to get the resulting one \n"\
"             for (int stride = 1; stride <= (64 >> 1); stride <<= 1) \n"\
"             { \n"\
"                 if (lid < 64/(2*stride)) \n"\
"                 { \n"\
"                     cx[2*(lid + 1)*stride-1] = cx[2*(lid + 1)*stride-1] + cx[(2*lid + 1)*stride-1]; \n"\
"                 } \n"\
" \n"\
"                 barrier(CLK_LOCAL_MEM_FENCE); \n"\
"             } \n"\
" \n"\
"             // Put the coefficient into global memory \n"\
"             if (lid == 0) \n"\
"             { \n"\
"                coeffs[g * 9 + i] = cx[63]; \n"\
"             } \n"\
" \n"\
"             barrier(CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__attribute__((reqd_work_group_size(8, 8, 1))) \n"\
"///< Project the function represented by lat-long map lmmap to Sh up to lmax band \n"\
"__kernel void ShProjectHemisphericalProbe( \n"\
"    __global Texture const* textures, \n"\
"    // Texture data \n"\
"    __global char const* texturedata, \n"\
"    // Environment texture index \n"\
"    int envmapidx, \n"\
"    // Harmonic coefficients flattened: NumShTerms(lmax) * num_groups \n"\
"    __global float3* coeffs \n"\
"    ) \n"\
"{ \n"\
"    // Temporary work area for trigonom results \n"\
"    __local float3 cx[64]; \n"\
" \n"\
"    int x = get_global_id(0); \n"\
"    int y = get_global_id(1); \n"\
" \n"\
"    int xl = get_local_id(0); \n"\
"    int yl = get_local_id(1); \n"\
"    int wl = get_local_size(0); \n"\
" \n"\
"    int ngx = get_num_groups(0); \n"\
"    int gx = get_group_id(0); \n"\
"    int gy = get_group_id(1); \n"\
"    int g = gy * ngx + gx; \n"\
" \n"\
"    int lid = yl * wl + xl; \n"\
" \n"\
"    Texture envmap = textures[envmapidx]; \n"\
"    int w = envmap.w; \n"\
"    int h = envmap.h; \n"\
" \n"\
"    if (x < w && y < h) \n"\
"    { \n"\
"        // Calculate spherical angles \n"\
"        float thetastep = PI / h; \n"\
"        float phistep = 2.f*PI / w; \n"\
"        float theta0 = 0;//PI / h / 2.f; \n"\
"        float phi0 = 0;//2.f*PI / w / 2.f; \n"\
" \n"\
"        float phi = phi0 + x * phistep; \n"\
"        float theta = theta0 + y * thetastep; \n"\
" \n"\
"        float sinphi = sin(phi); \n"\
"        float cosphi = cos(phi); \n"\
"        float costheta = cos(theta); \n"\
"        float sintheta = sin(theta); \n"\
" \n"\
"        // Construct point on unit sphere \n"\
"        float3 p = normalize(make_float3(sintheta * cosphi, costheta, sintheta * sinphi)); \n"\
" \n"\
"        float envmapaspect = (float)envmap.h / envmap.w; \n"\
"        float2 uv = p.xz; \n"\
"        uv.y = 0.5f*uv.y + 0.5f; \n"\
"        uv.x = 0.5f*uv.x + 0.5f; \n"\
"        uv.x = (1.f - envmapaspect) * 0.5f + uv.x * envmapaspect; \n"\
" \n"\
"        //uv.x = (float)x/w; \n"\
"        //uv.y = 1.f - (float)y/h; \n"\
"        float3 le = Sample2D(&envmap, texturedata, uv); \n"\
" \n"\
"        // Evaluate SH functions at w up to lmax band \n"\
"        float ylm[9]; \n"\
"        ShEvaluate(p, ylm); \n"\
" \n"\
"        // Evaluate Riemann sum \n"\
"        for (int i = 0; i < 9; ++i) \n"\
"        { \n"\
"            // Calculate the coefficient into local memory \n"\
"             cx[lid] = le * ylm[i] * sintheta * (PI / h) * (2.f * PI / w); \n"\
" \n"\
"             barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"             // Reduce the coefficient to get the resulting one \n"\
"             for (int stride = 1; stride <= (64 >> 1); stride <<= 1) \n"\
"             { \n"\
"                 if (lid < 64/(2*stride)) \n"\
"                 { \n"\
"                     cx[2*(lid + 1)*stride-1] = cx[2*(lid + 1)*stride-1] + cx[(2*lid + 1)*stride-1]; \n"\
"                 } \n"\
" \n"\
"                 barrier(CLK_LOCAL_MEM_FENCE); \n"\
"             } \n"\
" \n"\
"             // Put the coefficient into global memory \n"\
"             if (lid == 0) \n"\
"             { \n"\
"                coeffs[g * 9 + i] = cx[63]; \n"\
"             } \n"\
" \n"\
"             barrier(CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"#define GROUP_SIZE 256 \n"\
"__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1))) \n"\
"__kernel void ShReduce( \n"\
"    // Harmonic coefficients flattened: NumShTerms(lmax) * num_groups \n"\
"    const __global float3* coeffs, \n"\
"    // Number of sets \n"\
"    int numsets, \n"\
"    // Resulting coeffs \n"\
"    __global float3* result \n"\
"    ) \n"\
"{ \n"\
"    __local float3 lds[GROUP_SIZE]; \n"\
" \n"\
"    int gid = get_global_id(0); \n"\
"    int lid = get_global_id(0); \n"\
" \n"\
"    // How many items to reduce for a single work item \n"\
"    int numprivate = numsets / GROUP_SIZE; \n"\
" \n"\
"    for (int i=0;i<9;++i) \n"\
"    { \n"\
"        float3 res = {0,0,0}; \n"\
" \n"\
"        // Private reduction \n"\
"        for (int j=0;j<numprivate;++j) \n"\
"        { \n"\
"            res += coeffs[gid * numprivate * 9 + j * 9 + i]; \n"\
"        } \n"\
" \n"\
"        // LDS reduction \n"\
"        lds[lid] = res; \n"\
" \n"\
"        barrier (CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Work group reduction \n"\
"        for (int stride = 1; stride <= (256 >> 1); stride <<= 1) \n"\
"        { \n"\
"            if (lid < GROUP_SIZE/(2*stride)) \n"\
"            { \n"\
"                lds[2*(lid + 1)*stride-1] = lds[2*(lid + 1)*stride-1] + lds[(2*lid + 1)*stride-1]; \n"\
"            } \n"\
" \n"\
"            barrier (CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
" \n"\
"        // Write final result \n"\
"        if (lid == 0) \n"\
"        { \n"\
"            result[i] = lds[GROUP_SIZE-1]; \n"\
"        } \n"\
" \n"\
"        barrier (CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
"#undef GROUP_SIZE \n"\
" \n"\
" \n"\
"__attribute__((reqd_work_group_size(8, 8, 1))) \n"\
"__kernel void ShReconstructLmmap( \n"\
"    // SH coefficients: NumShTerms(lmax) items \n"\
"    const __global float3* coeffs, \n"\
"    // Resulting image width \n"\
"    int w, \n"\
"    // Resulting image height \n"\
"    int h, \n"\
"    // Resulting image \n"\
"    __global float3* lmmap \n"\
"    ) \n"\
"{ \n"\
"    int x = get_global_id(0); \n"\
"    int y = get_global_id(1); \n"\
" \n"\
"    int xl = get_local_id(0); \n"\
"    int yl = get_local_id(1); \n"\
"    int wl = get_local_size(0); \n"\
" \n"\
"    int ngx = get_num_groups(0); \n"\
"    int gx = get_group_id(0); \n"\
"    int gy = get_group_id(1); \n"\
" \n"\
"    if (x < w && y < h) \n"\
"    { \n"\
"        // Calculate spherical angles \n"\
"        float thetastep = M_PI / h; \n"\
"        float phistep = 2.f*M_PI / w; \n"\
"        float theta0 = 0;//M_PI / h / 2.f; \n"\
"        float phi0 = 0;//2.f*M_PI / w / 2.f; \n"\
" \n"\
"        float phi = phi0 + x * phistep; \n"\
"        float theta = theta0 + y * thetastep; \n"\
" \n"\
"        float2 uv; \n"\
"        uv.x = (float)x/w; \n"\
"        uv.y = (float)y/h; \n"\
" \n"\
"        float sinphi = sin(phi); \n"\
"        float cosphi = cos(phi); \n"\
"        float costheta = cos(theta); \n"\
"        float sintheta = sin(theta); \n"\
" \n"\
"        // Construct point on unit sphere \n"\
"        float3 p = normalize(make_float3(sintheta * cosphi, costheta, sintheta * sinphi)); \n"\
" \n"\
"        // Evaluate SH functions at w up to lmax band \n"\
"        float ylm[9]; \n"\
"        ShEvaluate(p, ylm); \n"\
" \n"\
"        // Evaluate Riemann sum \n"\
"        float3 val = {0, 0, 0}; \n"\
"        for (int i = 0; i < 9; ++i) \n"\
"        { \n"\
"            // Calculate the coefficient into local memory \n"\
"             val += ylm[i] * coeffs[i]; \n"\
"        } \n"\
" \n"\
"        int x = floor(uv.x * w); \n"\
"        int y = floor(uv.y * h); \n"\
" \n"\
"        lmmap[w * y + x] = val; \n"\
"    } \n"\
"} \n"\
;
static const char cl_app[]= \
" \n"\
"#define PI 3.14159265358979323846f \n"\
" \n"\
"typedef struct _ray \n"\
"{ \n"\
"    float4 o; \n"\
"    float4 d; \n"\
"} ray; \n"\
" \n"\
"typedef struct __attribute__ ((packed)) _Intersection \n"\
"{ \n"\
"    float4 uvwt; \n"\
"    int shapeid; \n"\
"    int primid; \n"\
"} Intersection; \n"\
" \n"\
"typedef struct _Camera \n"\
"{ \n"\
"    // Camera coordinate frame \n"\
"    float3 forward; \n"\
"    float3 right; \n"\
"    float3 up; \n"\
"    float3 p; \n"\
" \n"\
"    // Image plane width & height in scene units \n"\
"    float2 dim; \n"\
" \n"\
"    // Near and far Z \n"\
"    float2 zcap; \n"\
"    float  fovy; \n"\
"    float  aspect; \n"\
"} Camera; \n"\
" \n"\
"typedef struct _Rng \n"\
"{ \n"\
"    uint val; \n"\
"} Rng; \n"\
" \n"\
"typedef struct _Material \n"\
"{ \n"\
"    float4 kd; \n"\
"    float4 ks; \n"\
"    float3 ke; \n"\
"    int kdmapidx; \n"\
"    int nmapidx; \n"\
"    float metallic; \n"\
"    float subsurface; \n"\
"    float specular; \n"\
"    float roughness; \n"\
"    float speculartint; \n"\
"    float anisotropic; \n"\
"    float sheen; \n"\
"    float sheentint; \n"\
"    float clearcoat; \n"\
"    float clearcoatgloss; \n"\
"} Material; \n"\
" \n"\
"enum TextureFormat \n"\
"{ \n"\
"    UNKNOWN, \n"\
"    RGBA8, \n"\
"    RGBA16, \n"\
"    RGBA32 \n"\
"}; \n"\
" \n"\
"// Texture description \n"\
"typedef struct _Texture \n"\
"{ \n"\
"    // Texture width, height and depth \n"\
"    int w; \n"\
"    int h; \n"\
"    int d; \n"\
"    // Offset in texture data array \n"\
"    int dataoffset; \n"\
"    int fmt; \n"\
"    int extra; \n"\
"} Texture; \n"\
" \n"\
"// Shape description \n"\
"typedef struct _Shape \n"\
"{ \n"\
"    // Shape starting index in indices_ array \n"\
"    int startidx; \n"\
"    // Number of primitives in the shape \n"\
"    int numprims; \n"\
"    // Start vertex \n"\
"    int startvtx; \n"\
"    // Number of vertices \n"\
"    int numvertices; \n"\
"    // Linear motion vector \n"\
"    float3 linearvelocity; \n"\
"    // Angular velocity \n"\
"    float4 angularvelocity; \n"\
"    // Transform \n"\
"    float4 m0; \n"\
"    float4 m1; \n"\
"    float4 m2; \n"\
"    float4 m3; \n"\
"} Shape; \n"\
" \n"\
"// Emissive object \n"\
"typedef struct _Emissive \n"\
"{ \n"\
"    // Shape index \n"\
"    int shapeidx; \n"\
"    // Polygon index \n"\
"    int primidx; \n"\
"} Emissive; \n"\
" \n"\
" \n"\
"// Unitility functions \n"\
"#ifndef APPLE \n"\
" \n"\
"float4 make_float4(float x, float y, float z, float w) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    res.w = w; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 make_float3(float x, float y, float z) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    res.z = z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float2 make_float2(float x, float y) \n"\
"{ \n"\
"    float2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"int2 make_int2(int x, int y) \n"\
"{ \n"\
"    int2 res; \n"\
"    res.x = x; \n"\
"    res.y = y; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"#endif \n"\
" \n"\
"float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3) \n"\
"{ \n"\
"    float3 res; \n"\
"    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z; \n"\
"    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z; \n"\
"    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_mul(float4 q1, float4 q2) \n"\
"{ \n"\
"    float4 res; \n"\
"    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x; \n"\
"    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y; \n"\
"    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z; \n"\
"    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"float4 quaternion_conjugate(float4 q) \n"\
"{ \n"\
"    return make_float4(-q.x, -q.y, -q.z, q.w); \n"\
"} \n"\
" \n"\
"float4 quaternion_inverse(float4 q) \n"\
"{ \n"\
"    float sqnorm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w; \n"\
" \n"\
"    if (sqnorm != 0.f) \n"\
"    { \n"\
"        return quaternion_conjugate(q) / sqnorm; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        return make_float4(0.f, 0.f, 0.f, 1.f); \n"\
"    } \n"\
"} \n"\
" \n"\
"float3 rotate_vector(float3 v, float4 q) \n"\
"{ \n"\
"    float4 qinv = quaternion_inverse(q); \n"\
"    float4 vv = make_float4(v.x, v.y, v.z, 0); \n"\
"    return quaternion_mul(q, quaternion_mul(vv, qinv)).xyz; \n"\
"} \n"\
" \n"\
"uint WangHash(uint seed) \n"\
"{ \n"\
"    seed = (seed ^ 61) ^ (seed >> 16); \n"\
"    seed *= 9; \n"\
"    seed = seed ^ (seed >> 4); \n"\
"    seed *= 0x27d4eb2d; \n"\
"    seed = seed ^ (seed >> 15); \n"\
"    return seed; \n"\
"} \n"\
" \n"\
"uint RandUint(Rng* rng) \n"\
"{ \n"\
"    rng->val = WangHash(1664525U * rng->val + 1013904223U); \n"\
"    return rng->val; \n"\
"} \n"\
" \n"\
"float RandFloat(Rng* rng) \n"\
"{ \n"\
"    return ((float)RandUint(rng)) / 0xffffffffU; \n"\
"} \n"\
" \n"\
"void InitRng(uint seed, Rng* rng) \n"\
"{ \n"\
"    rng->val = WangHash(seed); \n"\
"    for (int i=0;i<100;++i) \n"\
"        RandFloat(rng); \n"\
"} \n"\
" \n"\
"float3 lerp(float3 a, float3 b, float w) \n"\
"{ \n"\
"    return a + w*(b-a); \n"\
"} \n"\
" \n"\
"void CartesianToSpherical ( float3 cart, float* r, float* phi, float* theta ) \n"\
"{ \n"\
"    float temp = atan2(cart.x, cart.z); \n"\
"    *r = sqrt(cart.x*cart.x + cart.y*cart.y + cart.z*cart.z); \n"\
"    *phi = (float)((temp >= 0)?temp:(temp + 2*PI)); \n"\
"    *theta = acos(cart.y/ *r); \n"\
"} \n"\
" \n"\
"/// Uniformly sample a triangle \n"\
"void SampleTriangleUniform( \n"\
"    // RNG to use \n"\
"    Rng* rng, \n"\
"    // Triangle vertices \n"\
"    float3 v0, \n"\
"    float3 v1, \n"\
"    float3 v2, \n"\
"    // Triangle normals \n"\
"    float3 n0, \n"\
"    float3 n1, \n"\
"    float3 n2, \n"\
"    // Point and normal \n"\
"    float3* p, \n"\
"    float3* n, \n"\
"    // PDF \n"\
"    float* pdf \n"\
"    ) \n"\
"{ \n"\
"    // Sample 2D value \n"\
"    float a = RandFloat(rng); \n"\
"    float b = RandFloat(rng); \n"\
" \n"\
"    // Calculate point \n"\
"    *p = (1.f - sqrt(a)) * v0.xyz + sqrt(a) * (1.f - b) * v1.xyz + sqrt(a) * b * v2.xyz; \n"\
"    // Calculate normal \n"\
"    *n = normalize((1.f - sqrt(a)) * n0.xyz + sqrt(a) * (1.f - b) * n1.xyz + sqrt(a) * b * n2.xyz); \n"\
"    // PDF \n"\
"    *pdf = 1.f / (length(cross((v2-v0), (v1-v0))) * 0.5f); \n"\
"} \n"\
" \n"\
"/// Get vector orthogonal to a given one \n"\
"float3 GetOrthoVector(float3 n) \n"\
"{ \n"\
"    float3 p; \n"\
"    if (fabs(n.z) > 0.707106781186547524401f) { \n"\
"        float k = sqrt(n.y*n.y + n.z*n.z); \n"\
"        p.x = 0; p.y = -n.z/k; p.z = n.y/k; \n"\
"    } \n"\
"    else { \n"\
"        float k = sqrt(n.x*n.x + n.y*n.y); \n"\
"        p.x = -n.y/k; p.y = n.x/k; p.z = 0; \n"\
"    } \n"\
" \n"\
"    return p; \n"\
"} \n"\
" \n"\
"///< Power heuristic for multiple importance sampling \n"\
"float PowerHeuristic(int nf, float fpdf, int ng, float gpdf) \n"\
"{ \n"\
"    float f = nf * fpdf; \n"\
"    float g = ng * gpdf; \n"\
"    return (f*f) / (f*f + g*g); \n"\
"} \n"\
" \n"\
"///< Sample hemisphere with cos weight \n"\
"float3 SampleHemisphere( \n"\
"    // RNG to use \n"\
"    Rng* rng, \n"\
"    // Hemisphere normal \n"\
"    float3 n, \n"\
"    // Cos power \n"\
"    float e) \n"\
"{ \n"\
"    // Construct basis \n"\
"    float3 u = GetOrthoVector(n); \n"\
"    float3 v = cross(u, n); \n"\
"    u = cross(n, v); \n"\
" \n"\
"    // Calculate 2D sample \n"\
"    float r1 = RandFloat(rng); \n"\
"    float r2 = RandFloat(rng); \n"\
" \n"\
"    // Transform to spherical coordinates \n"\
"    float sinpsi = sin(2*PI*r1); \n"\
"    float cospsi = cos(2*PI*r1); \n"\
"    float costheta = pow(1.f - r2, 1.f/(e + 1.f)); \n"\
"    float sintheta = sqrt(1.f - costheta * costheta); \n"\
" \n"\
"    // Return the result \n"\
"    return normalize(u * sintheta * cospsi + v * sintheta * sinpsi + n * costheta); \n"\
"} \n"\
" \n"\
"/// Sample 2D texture described by texture in texturedata pool \n"\
"float3 Sample2D(Texture const* texture, __global char const* texturedata, float2 uv) \n"\
"{ \n"\
"    // Get width and height \n"\
"    int width = texture->w; \n"\
"    int height = texture->h; \n"\
" \n"\
"    // Find the origin of the data in the pool \n"\
"    __global char const* mydata = texturedata + texture->dataoffset; \n"\
" \n"\
"    // Handle wrap \n"\
"    uv -= floor(uv); \n"\
" \n"\
"    // Reverse Y \n"\
"    // it is needed as textures are loaded with Y axis going top to down \n"\
"    // and our axis goes from down to top \n"\
"    uv.y = 1.f - uv.y; \n"\
" \n"\
"    // Figure out integer coordinates \n"\
"    int x = floor(uv.x * width); \n"\
"    int y = floor(uv.y * height); \n"\
" \n"\
"    // Calculate samples for linear filtering \n"\
"    int x1 = min(x + 1, width - 1); \n"\
"    int y1 = min(y + 1, height - 1); \n"\
" \n"\
"    // Calculate weights for linear filtering \n"\
"    float wx = uv.x * width - floor(uv.x * width); \n"\
"    float wy = uv.y * height - floor(uv.y * height); \n"\
" \n"\
"    if (texture->fmt == RGBA32) \n"\
"    { \n"\
"        __global float3 const* mydataf = (__global float3 const*)mydata; \n"\
" \n"\
"        // Get 4 values \n"\
"        float3 valx = *(mydataf + width * y + x); \n"\
"        float3 valx1 = *(mydataf + width * y + x1); \n"\
"        float3 valy1 = *(mydataf + width * y1 + x); \n"\
"        float3 valx1y1 = *(mydataf + width * y1 + x1); \n"\
" \n"\
"        // Filter and return the result \n"\
"        return lerp(lerp(valx, valx1, wx), lerp(valy1, valx1y1, wx), wy); \n"\
"    } \n"\
"    else if (texture->fmt == RGBA16) \n"\
"    { \n"\
"        __global half const* mydatah = (__global half const*)mydata; \n"\
" \n"\
"        // Get 4 values \n"\
"        float3 valx = vload_half4(width * y + x, mydatah).xyz; \n"\
"        float3 valx1 = vload_half4(width * y + x1, mydatah).xyz; \n"\
"        float3 valy1 = vload_half4(width * y1 + x, mydatah).xyz; \n"\
"        float3 valx1y1 = vload_half4(width * y1 + x1, mydatah).xyz; \n"\
" \n"\
"        // Filter and return the result \n"\
"        return lerp(lerp(valx, valx1, wx), lerp(valy1, valx1y1, wx), wy); \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        __global uchar4 const* mydatac = (__global uchar4 const*)mydata; \n"\
" \n"\
"        // Get 4 values \n"\
"        uchar4 valx = *(mydatac + width * y + x); \n"\
"        uchar4 valx1 = *(mydatac + width * y + x1); \n"\
"        uchar4 valy1 = *(mydatac + width * y1 + x); \n"\
"        uchar4 valx1y1 = *(mydatac + width * y1 + x1); \n"\
" \n"\
"        float3 valxf = make_float3((float)valx.x / 255.f, (float)valx.y / 255.f, (float)valx.z / 255.f); \n"\
"        float3 valx1f = make_float3((float)valx1.x / 255.f, (float)valx1.y / 255.f, (float)valx1.z / 255.f); \n"\
"        float3 valy1f = make_float3((float)valy1.x / 255.f, (float)valy1.y / 255.f, (float)valy1.z / 255.f); \n"\
"        float3 valx1y1f = make_float3((float)valx1y1.x / 255.f, (float)valx1y1.y / 255.f, (float)valx1y1.z / 255.f); \n"\
" \n"\
"        // Filter and return the result \n"\
"        return lerp(lerp(valxf, valx1f, wx), lerp(valy1f, valx1y1f, wx), wy); \n"\
"    } \n"\
"} \n"\
" \n"\
"/// Sample 2D lat-long IBL map \n"\
"float3 SampleEnvMap(Texture const* texture, __global char const* texturedata, float3 d) \n"\
"{ \n"\
"    // Transform to spherical coords \n"\
"    float r, phi, theta; \n"\
"    CartesianToSpherical(d, &r, &phi, &theta); \n"\
" \n"\
"    // Map to [0,1]x[0,1] range and reverse Y axis \n"\
"    float2 uv; \n"\
"    uv.x = phi / (2*PI); \n"\
"    uv.y = 1.f - theta / PI; \n"\
" \n"\
"    // Sample the texture \n"\
"    return Sample2D(texture, texturedata, uv); \n"\
"} \n"\
" \n"\
"/// Get material data from hardcoded value or texture \n"\
"float3 GetValue( \n"\
"    // Value \n"\
"    float3 v, \n"\
"    // Texture index (can be -1 whihc means no texture) \n"\
"    int texidx,  \n"\
"    // Texture coordinate \n"\
"    float2 uv, \n"\
"    // Texture data \n"\
"    __global Texture const* textures, \n"\
"    __global char const* texturedata) \n"\
"{ \n"\
"    // If texture present sample from texture \n"\
"    if (texidx != -1) \n"\
"    { \n"\
"        // Get texture desc \n"\
"        Texture texture = textures[texidx]; \n"\
"        // Sample diffuse texture \n"\
"        return Sample2D(&texture, texturedata, uv); \n"\
"    } \n"\
" \n"\
"    // Return fixed color otherwise \n"\
"    return v; \n"\
"} \n"\
" \n"\
" \n"\
"/// Construct tangent basis on the fly and apply normal map \n"\
"float3 ApplyNormalMap(Texture* normalmap, __global char const* texturedata, float2 uv, float3 n, float3 v0, float3 v1, float3 v2, float2 uv0, float2 uv1, float2 uv2) \n"\
"{ \n"\
"    /// From PBRT book \n"\
"    float du1 = uv0.x - uv2.x; \n"\
"    float du2 = uv1.x - uv2.x; \n"\
"    float dv1 = uv0.y - uv2.y; \n"\
"    float dv2 = uv1.y - uv2.y; \n"\
" \n"\
"    float3 dp1 = v0 - v2; \n"\
"    float3 dp2 = v1 - v2; \n"\
" \n"\
"    float det = du1 * dv2 - dv1 * du2; \n"\
" \n"\
" \n"\
"    float3 dpdu, dpdv; \n"\
" \n"\
"    if (det != 0.f) \n"\
"    { \n"\
"        float invdet = 1.f / det; \n"\
"        dpdu = normalize(( dv2 * dp1 - dv1 * dp2) * invdet); \n"\
"        dpdv = normalize((-du2 * dp1 + du1 * dp2) * invdet); \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        dpdu = GetOrthoVector(n); \n"\
"        dpdv = cross(n, dpdu); \n"\
"    } \n"\
" \n"\
"    float3 newnormal = cross(dpdv, dpdu); \n"\
" \n"\
"    // Now n, dpdu, dpdv is orthonormal basis \n"\
"    float3 mappednormal = 2.f * Sample2D(normalmap, texturedata, uv) - make_float3(1.f, 1.f, 1.f); \n"\
" \n"\
"    // Return mapped version \n"\
"    return  normalize(mappednormal.z *  n + mappednormal.x * dpdu + mappednormal.y * dpdv);   \n"\
"} \n"\
" \n"\
"///< Shlick Fresnel term approx for Disney BRDF \n"\
"float SchlickFresnel(const float u) \n"\
"{ \n"\
"    const float m = clamp(1.f - u, 0.f, 1.f); \n"\
"    const float m2 = m * m; \n"\
"    return m2 * m2 * m; \n"\
"} \n"\
" \n"\
"float SchlickFresnelEta(float eta, float cosi) \n"\
"{ \n"\
"    const float f = ((1.f - eta) / (1.f + eta)) * ((1.f - eta) / (1.f + eta)); \n"\
"    const float m = 1.f - fabs(cosi); \n"\
"    const float m2 = m*m; \n"\
"    return f + (1.f - f) * m2 * m2 * m; \n"\
"} \n"\
" \n"\
"float Gtr1(const float ndoth, const float a)  \n"\
"{ \n"\
"    if (a >= 1.f) \n"\
"        return 1.f / PI; \n"\
" \n"\
"    const float a2 = a * a; \n"\
"    const float t = 1.f + (a2 - 1.f) * ndoth * ndoth; \n"\
" \n"\
"    return (a2 - 1.f) / (PI * log(a2) * t); \n"\
"} \n"\
" \n"\
"float Gtr2Aniso( \n"\
"    const float ndoth,  \n"\
"    const float hdotx,  \n"\
"    const float hdoty, \n"\
"    const float ax,  \n"\
"    const float ay \n"\
"    )  \n"\
"{ \n"\
"    return clamp( \n"\
"        1.f / (PI * ax * ay *  \n"\
"        ((hdotx / ax)*(hdotx / ax) + (hdoty / ay)*(hdoty / ay) + ndoth * ndoth) *  \n"\
"        ((hdotx / ax)*(hdotx / ax) + (hdoty / ay)*(hdoty / ay) + ndoth * ndoth)), \n"\
"        0.f, 10.f); // A trick to avoid fireflies \n"\
"} \n"\
" \n"\
"float SmithGggx(const float ndotv, const float alphag)  \n"\
"{ \n"\
"    float a = alphag * alphag; \n"\
"    float b = ndotv * ndotv; \n"\
"    return 1.f / (ndotv + sqrt(a + b - a * b)); \n"\
"} \n"\
" \n"\
"///< Evaluate actual layers \n"\
"float3 DisneyEvaluateLayers( \n"\
"    Material *material, \n"\
"    const float3 n,  \n"\
"    const float3 v,  \n"\
"    const float3 l,  \n"\
"    const float3 h, \n"\
"    const float ds,  \n"\
"    const float2 uv, \n"\
"    __global Texture const* textures, \n"\
"    __global char const* texturedata \n"\
"    ) \n"\
"{ \n"\
"    // Read material parameters \n"\
"    const float3 basecolor = GetValue(material->kd.xyz, material->kdmapidx, uv, textures, texturedata); \n"\
"    const float metallic = material->metallic; \n"\
"    const float subsurface = material->subsurface; \n"\
"    const float specular = material->specular; \n"\
"    const float roughness = material->roughness; \n"\
"    const float speculartint = material->speculartint; \n"\
"    const float sheen = material->sheen; \n"\
"    const float sheentint = material->sheentint; \n"\
"    const float clearcoat = material->clearcoat; \n"\
"    const float clearcoatgloss = material->clearcoatgloss; \n"\
" \n"\
"    const float ndotv = dot(n, v); \n"\
"    const float ndotl = dot(n, l); \n"\
" \n"\
"    const float ndoth = dot(n, h); \n"\
"    const float ldoth = dot(l, h); \n"\
" \n"\
"    const float3 cdlin = basecolor; \n"\
"    const float cdlum = .3f * cdlin.x + .6f * cdlin.y + .1f * cdlin.z; // luminance approx. \n"\
" \n"\
"    const float3 ctint = (cdlum > 0.f) ? (cdlin / cdlum) : make_float3(1.f,1.f,1.f); // normalize lum. to isolate hue+sat \n"\
"    const float3 cspec0 = mix(specular * .08f * mix(1.f, ctint, speculartint), cdlin, metallic); \n"\
"    const float3 csheen = mix(1.f, ctint, sheentint); \n"\
" \n"\
"    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing \n"\
"    // and mix in diffuse retro-reflection based on roughness \n"\
"    const float fl = SchlickFresnel(ndotl); \n"\
"    const float fv = SchlickFresnel(ndotv); \n"\
"    const float fd90 = .5f + 2.f * ldoth * ldoth * roughness; \n"\
"    const float fd = mix(1.f, fd90, fl) * mix(1.f, fd90, fv); \n"\
" \n"\
"    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf \n"\
"    // 1.25 scale is used to (roughly) preserve albedo \n"\
"    // fss90 used to flatten retroreflection based on roughness \n"\
"    const float fss90 = ldoth * ldoth * roughness; \n"\
"    const float fss = mix(1.f, fss90, fl) * mix(1.f, fss90, fv); \n"\
"    const float ss = 1.25f * (fss * (1.f / (ndotl + ndotv) - .5f) + .5f); \n"\
" \n"\
"    // Specular \n"\
"    // The original implementation uses ldoth (i.e. the micro-facets normal) but \n"\
"    // the effect of many parameters is hardly noticeable in this case. \n"\
"    const float fh = SchlickFresnel(ndotl); \n"\
"    const float3 fs = mix(cspec0, 1.f, fh); \n"\
"    const float roughg = (roughness * .5f + .5f) * (roughness * .5f + .5f); \n"\
"    const float gs = SmithGggx(ndotl, roughg) * SmithGggx(ndotv, roughg); \n"\
" \n"\
"    // Sheen \n"\
"    const float3 fsheen = fh * sheen * csheen; \n"\
" \n"\
"    // Clearcoat (ior = 1.5 -> f0 = 0.04) \n"\
"    const float dr = Gtr1(ndoth, mix(.1f, .001f, clearcoatgloss)); \n"\
"    const float fr = mix(.04f, 1.f, fh); \n"\
"    const float gr = SmithGggx(ndotl, .25f) * SmithGggx(ndotv, .25f); \n"\
" \n"\
"    float f = 1.f; \n"\
"    const float et = material->kd.w; \n"\
"    if (et != 0.f)  \n"\
"    { \n"\
"        const float3 wo = -v; \n"\
"        const float cosi = dot(n, wo); \n"\
"        f = 1.f - SchlickFresnelEta(et, cosi); \n"\
"    } \n"\
" \n"\
"    return f * ( \n"\
"        // Diffuse layer \n"\
"        ((1.f / PI) * mix(fd, ss, subsurface) * cdlin + fsheen) * (1.f - metallic) \n"\
"        + \n"\
"        // 1st Specular lobe \n"\
"        gs * fs * ds \n"\
"        + \n"\
"        // 2nd Specular lobe \n"\
"        // \n"\
"        // The original paper us a .25 scale but is not well visible in \n"\
"        // my opinion. I'm also clamping to reduce fireflies. \n"\
"        clamp(clearcoat * gr * fr * dr, 0.f, .5f) \n"\
"        ); \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"///< Evaluate Disney principled BRDF \n"\
"float3 DisneyEvaluate(Material *mat,  \n"\
"    float3 n,  \n"\
"    float3 wi, \n"\
"    float3 wo,  \n"\
"    float2 uv,  \n"\
"    __global Texture const* textures,  \n"\
"    __global char const* texturedata \n"\
")  \n"\
"{ \n"\
"    const float3 v = wi; \n"\
"    const float3 l = wo; \n"\
" \n"\
"    if (dot(v, n) < 0.f) \n"\
"        return 0.f; \n"\
"    if (dot(l, n) <= 0.f) \n"\
"        return 0.f; \n"\
" \n"\
"    const float anisotropic = mat->anisotropic; \n"\
"    const float roughness = mat->roughness; \n"\
" \n"\
"    const float aspect = sqrt(1.f - anisotropic * .9f); \n"\
"    const float ax = max(.001f, roughness * roughness / aspect); \n"\
"    const float ay = max(.001f, roughness * roughness * aspect); \n"\
" \n"\
"    float3 x, y; \n"\
"    x = GetOrthoVector(n); \n"\
"    y = normalize(cross(n, x)); \n"\
" \n"\
"    const float3 h = normalize(l + v); \n"\
" \n"\
"    const float ndoth = dot(n, h); \n"\
"    const float hdotx = dot(h, x); \n"\
"    const float hdoty = dot(h, y); \n"\
"    const float ds = Gtr2Aniso(ndoth, hdotx, hdoty, ax, ay); \n"\
" \n"\
"    return DisneyEvaluateLayers(mat, n, v, l, h, ds, uv, textures, texturedata); \n"\
"} \n"\
" \n"\
" \n"\
"///< Sample Disney BRDF \n"\
"float3 DisneySample( \n"\
"    Rng* rng, \n"\
"    Material *mat,  \n"\
"    float3 n,  \n"\
"    float3 wi, \n"\
"    float2 uv,  \n"\
"    __global Texture const* textures,  \n"\
"    __global char const* texturedata, \n"\
"    float3* wo, \n"\
"    float* pdf \n"\
")  \n"\
"{ \n"\
"    const float weight2 = mat->metallic; \n"\
"    const float weight1 = 1.f - weight2; \n"\
" \n"\
"    const float3 v = wi; \n"\
"    if (dot(v, n) < 0.f) \n"\
"        n = -n; \n"\
" \n"\
"    const float anisotropic = mat->anisotropic; \n"\
"    const float roughness = mat->roughness; \n"\
" \n"\
"    const float aspect = sqrt(1.f - anisotropic * .9f); \n"\
"    const float ax = max(.001f, roughness * roughness / aspect); \n"\
"    const float ay = max(.001f, roughness * roughness * aspect); \n"\
" \n"\
"    float3 x, y; \n"\
"    x = GetOrthoVector(n); \n"\
"    y = normalize(cross(n, x)); \n"\
" \n"\
"    const float u = RandFloat(rng); \n"\
"    const bool samplediffuse = (u < weight1); \n"\
" \n"\
"    float ds; \n"\
"    float3 h, l; \n"\
"    float diffusepdf, specularpdf; \n"\
"    if (samplediffuse)  \n"\
"    { \n"\
"        // Sample the diffuse layer \n"\
"        l = SampleHemisphere(rng, n, 0.f); \n"\
" \n"\
"        h = normalize(l + v); \n"\
" \n"\
"        const float ndoth = dot(n, h); \n"\
"        const float hdotx = dot(h, x); \n"\
"        const float hdoty = dot(h, y); \n"\
"        ds = Gtr2Aniso(ndoth, hdotx, hdoty, ax, ay); \n"\
" \n"\
"        const float ldoth = dot(l, h); \n"\
"        diffusepdf = dot(l, n) / PI; \n"\
"        specularpdf = (ds * ndoth) / (4.f * ldoth); \n"\
"    } \n"\
"    else { \n"\
"        // Sample the specular layer \n"\
"        const float u1 = RandFloat(rng); \n"\
"        const float u2 = RandFloat(rng); \n"\
" \n"\
"        h = normalize(sqrt(u2 / (1.f - u2)) * (ax * cos(2.f * PI * u1) + ay * sin(2.f * PI * u1)) + n); \n"\
"        l = 2.f * dot(v, h) * h - v; \n"\
" \n"\
"        const float ndoth = dot(n, h); \n"\
"        const float hdotx = dot(h, x); \n"\
"        const float hdoty = dot(h, y); \n"\
"        ds = Gtr2Aniso(ndoth, hdotx, hdoty, ax, ay); \n"\
" \n"\
"        const float ldoth = dot(l, h); \n"\
"        if (ldoth < .01f)  \n"\
"        { \n"\
"            *pdf = 0.f; \n"\
"            return make_float3(0.f, 0.f, 0.f); \n"\
"        } \n"\
" \n"\
"        diffusepdf = dot(l, n) / PI; \n"\
"        specularpdf = (ds * ndoth) / (4.f * ldoth); \n"\
" \n"\
"        // A trick to avoid fireflies \n"\
"        if (specularpdf < 1.f)  \n"\
"        { \n"\
"            *pdf = 0.f; \n"\
"            return make_float3(0.f, 0.f, 0.f); \n"\
"        } \n"\
"    } \n"\
" \n"\
"    *wo = l; \n"\
"    *pdf = weight1 * diffusepdf + weight2 * specularpdf; \n"\
" \n"\
"    return DisneyEvaluateLayers(mat, n, v, l, h, ds, uv, textures, texturedata); \n"\
"} \n"\
" \n"\
"float DisneyGetPdf( \n"\
"    Material *mat,  \n"\
"    float3 n,  \n"\
"    float3 wi,  \n"\
"    float3 wo, \n"\
"    float2 uv, \n"\
"    __global Texture const* textures, \n"\
"    __global char const* texturedata \n"\
"    )  \n"\
"{ \n"\
"    const float3 v = wi; \n"\
"    const float3 l = wo; \n"\
" \n"\
"    if (dot(v, n) < 0.f) \n"\
"        return 0.f; \n"\
"    if (dot(l, n) <= 0.f) \n"\
"        return 0.f; \n"\
" \n"\
"    const float weight2 = mat->metallic; \n"\
"    //const float weight2 = .25f * .5f * material->m_disney_principled.m_metallic; \n"\
"    //const float weight2 = .5f; \n"\
"    const float weight1 = 1.f - weight2; \n"\
" \n"\
"    // Diffuse pdf \n"\
"    const float diffusepdf = dot(l, n) / PI; \n"\
" \n"\
"    // Specular pdf \n"\
"    const float anisotropic = mat->anisotropic; \n"\
"    const float roughness = mat->roughness; \n"\
" \n"\
"    const float aspect = sqrt(1.f - anisotropic * .9f); \n"\
"    const float ax = max(.001f, roughness * roughness / aspect); \n"\
"    const float ay = max(.001f, roughness * roughness * aspect); \n"\
" \n"\
"    float3 x, y; \n"\
"    x = GetOrthoVector(n); \n"\
"    y = normalize(cross(n, x)); \n"\
" \n"\
"    const float3 h = normalize(l + v); \n"\
" \n"\
"    const float ndoth = dot(n, h); \n"\
"    const float hdotx = dot(h, x); \n"\
"    const float hdoty = dot(h, y); \n"\
"    const float ldoth = dot(l, h); \n"\
"    const float ds = Gtr2Aniso(ndoth, hdotx, hdoty, ax, ay); \n"\
" \n"\
"    const float specularpdf = (ds * ndoth) / (4.f * ldoth); \n"\
" \n"\
"    return weight1 * diffusepdf + weight2 * specularpdf; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
" \n"\
"///< Evaluate material at shading point \n"\
"float3 EvaluateBxdf(Material* mat, float3 wi, float3 wo, float3 n, float2 uv, __global Texture const* textures, __global char const* texturedata) \n"\
"{ \n"\
"    return DisneyEvaluate(mat, n, wi, wo, uv, textures, texturedata); \n"\
"} \n"\
" \n"\
"float3 SampleBxdf(Rng* rng, Material* mat, float3 wi, float3 n, float2 uv, __global Texture const* textures, __global char const* texturedata, float3* wo, float* pdf) \n"\
"{ \n"\
"    return DisneySample(rng, mat, n, wi, uv, textures, texturedata, wo, pdf); \n"\
"} \n"\
" \n"\
"float GetPdfBxfd(Material* mat, float3 wi, float3 wo, float3 n, float2 uv, __global Texture const* textures, __global char const* texturedata) \n"\
"{ \n"\
"    return DisneyGetPdf(mat, n, wi, wo, uv, textures, texturedata); \n"\
"} \n"\
" \n"\
"///< Light sampling  \n"\
" \n"\
"float3 EvaluateLight(float3 p, float3 n, float3 wo, int envmapidx, float envmapmul, __global Texture const* textures, __global char const* texturedata) \n"\
"{ \n"\
"    // Sample envmap \n"\
"    Texture envtexture = textures[envmapidx]; \n"\
"    return envmapmul * SampleEnvMap(&envtexture, texturedata, wo); \n"\
"} \n"\
" \n"\
"float3 SampleLight(Rng* rng, float3 p, float3 n, int envmapidx, float envmapmul, __global Texture const* textures, __global char const* texturedata, float3* wo, float* pdf) \n"\
"{ \n"\
"    // Generate direction \n"\
"    *wo = SampleHemisphere(rng, n, 1.f); \n"\
" \n"\
"    // Envmap PDF  \n"\
"    *pdf = dot(n, *wo) / PI; \n"\
" \n"\
"    // Sample envmap \n"\
"    Texture envtexture = textures[envmapidx]; \n"\
"    return envmapmul * SampleEnvMap(&envtexture, texturedata, *wo); \n"\
"} \n"\
" \n"\
"float GetPdfLight(float3 p, float3 n, float3 wo) \n"\
"{ \n"\
"    return dot(n, wo) > 0 ? (dot(n, wo) / PI) : 0.f; \n"\
"} \n"\
" \n"\
" \n"\
"/// Ray generation kernel \n"\
"/// Rays are generated from camera position to viewing plane \n"\
"/// using random sample distribution within the pixel \n"\
"/// \n"\
"__kernel void GenerateRays2D( \n"\
"    // Camera buffer \n"\
"    __global Camera const* camera, \n"\
"    // Image resolution \n"\
"    int imgwidth, \n"\
"    int imgheight, \n"\
"    // RNG seed value \n"\
"    int randseed, \n"\
"    // Output rays \n"\
"    __global ray* rays) \n"\
"{ \n"\
"    int2 global_id; \n"\
"    global_id.x  = get_global_id(0); \n"\
"    global_id.y  = get_global_id(1); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (global_id.x < imgwidth && global_id.y < imgheight) \n"\
"    { \n"\
"        __global ray* myray = rays + global_id.y * imgwidth + global_id.x; \n"\
" \n"\
"        Rng rng; \n"\
"        InitRng(randseed +  global_id.x * 157 - 33 * global_id.y, &rng); \n"\
" \n"\
"        // Calculate [0..1] image plane sample \n"\
"        float2 imgsample; \n"\
"        imgsample.x = (float)global_id.x / imgwidth + RandFloat(&rng) / imgwidth; \n"\
"        imgsample.y = (float)global_id.y / imgheight + RandFloat(&rng) / imgheight; \n"\
" \n"\
"        // Transform into [-0.5,0.5] \n"\
"        float2 hsample = imgsample - make_float2(0.5f, 0.5f); \n"\
"        // Transform into [-dim/2, dim/2] \n"\
"        float2 csample = hsample * camera->dim; \n"\
" \n"\
"        // Direction to image plane \n"\
"        myray->d.xyz = normalize(camera->zcap.x * camera->forward + csample.x * camera->right + csample.y * camera->up); \n"\
"        // Origin == camera position \n"\
"        myray->o.xyz = camera->p + camera->zcap.x * myray->d.xyz; \n"\
"        // Zcap == (znear, zfar) \n"\
"        myray->o.w = camera->zcap.y; \n"\
"        // Generate random time from 0 to 1 \n"\
"        myray->d.w = RandFloat(&rng); \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Sample environemnt light and produce light samples and shadow rays into separate buffer \n"\
"__kernel void SampleEnvironment( \n"\
"    // Ray batch \n"\
"    __global ray const* rays, \n"\
"    // Intersection data \n"\
"    __global Intersection const* isects, \n"\
"    // Hit indices \n"\
"    __global int const* hitindices, \n"\
"    // Pixel indices \n"\
"    __global int const* pixelindices, \n"\
"    // Number of rays \n"\
"    int numhits, \n"\
"    // Vertices \n"\
"    __global float3 const* vertices, \n"\
"    // Normals \n"\
"    __global float3 const* normals, \n"\
"    // UVs \n"\
"    __global float2 const* uvs, \n"\
"    // Indices \n"\
"    __global int const* indices, \n"\
"    // Shapes \n"\
"    __global Shape const* shapes, \n"\
"    // Material IDs \n"\
"    __global int const* materialids, \n"\
"    // Materials \n"\
"    __global Material const* materials, \n"\
"    // Textures \n"\
"    __global Texture const* textures, \n"\
"    // Texture data \n"\
"    __global char const* texturedata, \n"\
"    // Environment texture index \n"\
"    int envmapidx, \n"\
"    // Envmap multiplier \n"\
"    float envmapmul, \n"\
"    // Emissives \n"\
"    __global Emissive const* emissives, \n"\
"    // Number of emissive objects \n"\
"    int numemissives, \n"\
"    // RNG seed \n"\
"    int rngseed, \n"\
"    // Shadow rays \n"\
"    __global ray* shadowrays, \n"\
"    // Light samples \n"\
"    __global float3* lightsamples, \n"\
"    // Number of shadow rays to spawn \n"\
"    int numlightsamples, \n"\
"    // Path throughput \n"\
"    __global float3* throughput, \n"\
"    // Indirect rays \n"\
"    __global ray* indirectrays \n"\
"    ) \n"\
"{ \n"\
"    int globalid = get_global_id(0); \n"\
" \n"\
"    if (globalid < numhits) \n"\
"    { \n"\
"        // Fetch index \n"\
"        int hitidx = hitindices[globalid]; \n"\
"        int pixelidx = pixelindices[globalid]; \n"\
" \n"\
"        // Fetch incoming ray \n"\
"        float3 wi = -normalize(rays[hitidx].d.xyz); \n"\
"        float time = rays[hitidx].d.w; \n"\
" \n"\
"        // Determine shape and polygon \n"\
"        int shapeid = isects[hitidx].shapeid - 1; \n"\
"        int primid = isects[hitidx].primid; \n"\
"        float2 uv = isects[hitidx].uvwt.xy; \n"\
" \n"\
"        // Extract shape data \n"\
"        Shape shape = shapes[shapeid]; \n"\
"        float3 linearvelocity = shape.linearvelocity; \n"\
"        float4 angularvelocity = shape.angularvelocity; \n"\
" \n"\
"        // Fetch indices starting from startidx and offset by primid \n"\
"        int i0 = indices[shape.startidx + 3*primid]; \n"\
"        int i1 = indices[shape.startidx + 3*primid + 1]; \n"\
"        int i2 = indices[shape.startidx + 3*primid + 2]; \n"\
" \n"\
"        // Fetch normals \n"\
"        float3 n0 = normals[shape.startvtx + i0]; \n"\
"        float3 n1 = normals[shape.startvtx + i1]; \n"\
"        float3 n2 = normals[shape.startvtx + i2]; \n"\
" \n"\
"        // Fetch positions \n"\
"        float3 v0 = vertices[shape.startvtx + i0]; \n"\
"        float3 v1 = vertices[shape.startvtx + i1]; \n"\
"        float3 v2 = vertices[shape.startvtx + i2]; \n"\
" \n"\
"        // Fetch UVs \n"\
"        float2 uv0 = uvs[shape.startvtx + i0]; \n"\
"        float2 uv1 = uvs[shape.startvtx + i1]; \n"\
"        float2 uv2 = uvs[shape.startvtx + i2]; \n"\
" \n"\
"        // Calculate barycentric position and normal \n"\
"        float3 n = normalize((1.f - uv.x - uv.y) * n0 + uv.x * n1 + uv.y * n2); \n"\
"        float3 v = (1.f - uv.x - uv.y) * v0 + uv.x * v1 + uv.y * v2; \n"\
"        float2 t = (1.f - uv.x - uv.y) * uv0 + uv.x * uv1 + uv.y * uv2; \n"\
" \n"\
"        // Bail out if opposite facing normal \n"\
"        if (dot(wi, n) < 0.f) \n"\
"        { \n"\
"            //throughput[pixelidx] = 0; \n"\
"            //return; \n"\
"        } \n"\
" \n"\
"        // Get material at shading point \n"\
"        int matidx = materialids[shape.startidx / 3 + primid]; \n"\
"        Material mat = materials[matidx]; \n"\
" \n"\
" \n"\
"        // Apply transform & linear motion blur \n"\
"        v = rotate_vector(v, angularvelocity); \n"\
"        v = transform_point(v, shape.m0, shape.m1, shape.m2, shape.m3); \n"\
"        v += (linearvelocity * time); \n"\
"        // MT^-1 should be used if scale is present \n"\
"        n = rotate_vector(n, angularvelocity); \n"\
"        n = transform_vector(n, shape.m0, shape.m1, shape.m2, shape.m3); \n"\
" \n"\
"        // Apply normal map if needed \n"\
"        if (mat.nmapidx != -1) \n"\
"        { \n"\
"            Texture normalmap = textures[mat.nmapidx]; \n"\
"            n = ApplyNormalMap(&normalmap, texturedata, t, n, v0, v1, v2, uv0, uv1, uv2); \n"\
"        } \n"\
" \n"\
"        if (dot(wi, n) < 0.f) \n"\
"        { \n"\
"            n = -n; \n"\
"        } \n"\
" \n"\
"        // Prepare RNG for light sampling \n"\
"        Rng rng; \n"\
"        InitRng(rngseed + globalid * 157, &rng); \n"\
" \n"\
"        float lightpdf = 0.f; \n"\
"        float bxdfpdf = 0.f; \n"\
"        float3 wo; \n"\
"        float3 le; \n"\
" \n"\
"        // Generate shadow rays and light samples \n"\
"        for (int i=0; i<numlightsamples/2; ++i) \n"\
"        { \n"\
"            // STEP1: Sample light and apply MIS \n"\
" \n"\
"            // Generate direction \n"\
"            le = SampleLight(&rng, v, n, envmapidx, envmapmul, textures, texturedata, &wo, &lightpdf); \n"\
"            bxdfpdf = GetPdfBxfd(&mat, wi, wo, n, uv, textures, texturedata); \n"\
"            float lightsampling_weight = PowerHeuristic(1, lightpdf, 1, bxdfpdf); \n"\
" \n"\
"            float3 radiance = 0.f; \n"\
" \n"\
"            if (lightpdf > 0.f) \n"\
"            { \n"\
"                // Generate light sample \n"\
"                radiance = lightsampling_weight * le * EvaluateBxdf(&mat, wi, wo, n, t, textures, texturedata) * throughput[pixelidx] * max(0.f, dot(n, wo)) / lightpdf; \n"\
"            } \n"\
" \n"\
"            // Generate ray \n"\
"            shadowrays[globalid * numlightsamples + 2*i].d.xyz = wo; \n"\
"            shadowrays[globalid * numlightsamples + 2*i].o.xyz = v + 0.001f * wo; \n"\
"            shadowrays[globalid * numlightsamples + 2*i].o.w = 100000000.f; \n"\
"            shadowrays[globalid * numlightsamples + 2*i].d.w = rays[hitidx].d.w; \n"\
"            lightsamples[globalid * numlightsamples + 2*i] = clamp(radiance, 0.f, 10.f); \n"\
" \n"\
"            radiance = 0.f; \n"\
"            { \n"\
"                // Sample BxDF \n"\
"                float3 bxdf = SampleBxdf(&rng, &mat, wi, n, t, textures, texturedata, &wo, &bxdfpdf); \n"\
"                lightpdf = GetPdfLight(v, n, wo); \n"\
"                float bxdfsampling_weight = PowerHeuristic(1, bxdfpdf, 1, lightpdf); \n"\
"                le = EvaluateLight(v, n, wo, envmapidx, envmapmul, textures, texturedata);  \n"\
" \n"\
"                if (bxdfpdf > 0.f) \n"\
"                { \n"\
"                    radiance = bxdfsampling_weight * le * bxdf * throughput[pixelidx] * max(0.f, dot(n, wo)) / bxdfpdf; \n"\
"                } \n"\
"            } \n"\
" \n"\
"            // Generate ray \n"\
"            shadowrays[globalid * numlightsamples + 2 * i + 1].d.xyz = wo; \n"\
"            shadowrays[globalid * numlightsamples + 2 * i + 1].o.xyz = v + 0.001f * wo; \n"\
"            shadowrays[globalid * numlightsamples + 2 * i + 1].o.w = 100000000.f; \n"\
"            shadowrays[globalid * numlightsamples + 2 * i + 1].d.w = rays[hitidx].d.w; \n"\
"            lightsamples[globalid * numlightsamples + 2 * i + 1] = clamp(radiance, 0.f, 10.f); \n"\
"        } \n"\
" \n"\
"        // Generate indirect ray and update throughput \n"\
" \n"\
"        float pdf; \n"\
"        float3 bxdf = SampleBxdf(&rng, &mat, wi, n, t, textures, texturedata, &wo, &pdf); \n"\
" \n"\
"        if (pdf > 0.001f && dot(n, wo) > 0.001f) \n"\
"            throughput[pixelidx] *= (bxdf * max(0.f, dot(n, wo)) / pdf); \n"\
"        else \n"\
"            throughput[pixelidx] = 0.f; \n"\
" \n"\
"        indirectrays[globalid].d.xyz = normalize(wo); \n"\
"        indirectrays[globalid].o.xyz = v + 0.001f * normalize(wo); \n"\
"        indirectrays[globalid].o.w = 100000000.f; \n"\
"        indirectrays[globalid].d.w = rays[hitidx].d.w; \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Illuminate missing rays \n"\
"__kernel void ShadeMiss( \n"\
"    // Ray batch \n"\
"    __global ray const* rays, \n"\
"    // Intersection data \n"\
"    __global Intersection const* isects, \n"\
"    // Pixel indices \n"\
"    __global int const* pixelindices, \n"\
"    // Number of rays \n"\
"    int numrays, \n"\
"    // Textures \n"\
"    __global Texture const* textures, \n"\
"    // Texture data \n"\
"    __global char const* texturedata, \n"\
"    // Environment texture index \n"\
"    int envmapidx, \n"\
"    // Througput \n"\
"    __global float3 const* througput, \n"\
"    // Output values \n"\
"    __global float4* output \n"\
"    ) \n"\
"{ \n"\
"    int globalid = get_global_id(0); \n"\
" \n"\
"    if (globalid < numrays) \n"\
"    { \n"\
"        int pixelidx = pixelindices[globalid]; \n"\
" \n"\
"        // In case of a miss \n"\
"        if (isects[globalid].shapeid < 0) \n"\
"        { \n"\
"            // Sample IBL for that ray \n"\
"            Texture texture = textures[envmapidx]; \n"\
"            // Multiply by througput \n"\
"            output[pixelidx].xyz += SampleEnvMap(&texture, texturedata, rays[globalid].d.xyz) * througput[pixelidx]; \n"\
"        } \n"\
" \n"\
"        output[pixelidx].w += 1.f; \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Handle light samples and visibility info and add contribution to final buffer \n"\
"__kernel void GatherLightSamples( \n"\
"    // Pixel indices \n"\
"    __global int const* pixelindices, \n"\
"    // Number of rays \n"\
"    int numrays, \n"\
"    // Number of light samples per ray \n"\
"    int numlightsamples, \n"\
"    // Shadow rays hits \n"\
"    __global int const* shadowhits, \n"\
"    // Light samples \n"\
"    __global float3 const* lightsamples, \n"\
"    // Througput \n"\
"    __global float3 const* throughput, \n"\
"    // Radiance sample buffer \n"\
"    __global float4* output \n"\
"    ) \n"\
"{ \n"\
"    int globalid = get_global_id(0); \n"\
" \n"\
"    if (globalid < numrays) \n"\
"    { \n"\
"        // Prepare accumulator variable \n"\
"        float3 radiance = make_float3(0.f, 0.f, 0.f); \n"\
" \n"\
"        // Get pixel id for this sample set \n"\
"        int pixelidx = pixelindices[globalid]; \n"\
" \n"\
"        // Start collecting samples \n"\
"        for (int i=0; i<numlightsamples; ++i) \n"\
"        { \n"\
"            // If shadow ray didn't hit anything and reached skydome \n"\
"            if (shadowhits[globalid * numlightsamples + i] == -1) \n"\
"            { \n"\
"                // Add its contribution to radiance accumulator \n"\
"                radiance += lightsamples[globalid * numlightsamples + i]; \n"\
"            } \n"\
"        } \n"\
" \n"\
"        // Divide by number of light samples (samples already have built-in throughput \n"\
"        output[pixelidx].xyz += (radiance / numlightsamples); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"///< Restore pixel indices after compaction \n"\
"__kernel void RestorePixelIndices( \n"\
"    // Compacted indices \n"\
"    __global int const* compacted_indices, \n"\
"    // Number of compacted indices \n"\
"    int numitems, \n"\
"    // Previous pixel indices \n"\
"    __global int const* previndices, \n"\
"    // New pixel indices \n"\
"    __global int* newindices \n"\
"    ) \n"\
"{ \n"\
"    int globalid = get_global_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (globalid < numitems) \n"\
"    { \n"\
"        newindices[globalid] = previndices[compacted_indices[globalid]]; \n"\
"    } \n"\
"} \n"\
" \n"\
"///< Restore pixel indices after compaction \n"\
"__kernel void ConvertToPredicate( \n"\
"    // Intersections \n"\
"    __global Intersection const* isects, \n"\
"    // Number of compacted indices \n"\
"    int numitems, \n"\
"    // Previous pixel indices \n"\
"    __global int* predicate \n"\
"    ) \n"\
"{ \n"\
"    int globalid = get_global_id(0); \n"\
" \n"\
"    // Handle only working subset \n"\
"    if (globalid < numitems) \n"\
"    { \n"\
"        predicate[globalid] = isects[globalid].shapeid >= 0 ? 1 : 0; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
;
