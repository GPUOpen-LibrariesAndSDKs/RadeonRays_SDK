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
#ifndef UTILS_CL
#define UTILS_CL

#define PI 3.14159265358979323846f

#include <../App/CL/payload.cl>

#ifndef APPLE
/// These functions are defined on OSX already
float4 make_float4(float x, float y, float z, float w)
{
    float4 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    return res;
}

float3 make_float3(float x, float y, float z)
{
    float3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}

float2 make_float2(float x, float y)
{
    float2 res;
    res.x = x;
    res.y = y;
    return res;
}

int2 make_int2(int x, int y)
{
    int2 res;
    res.x = x;
    res.y = y;
    return res;
}
#endif

float4x4 matrix_from_cols(float4 c0, float4 c1, float4 c2, float4 c3)
{
    float4x4 m;
    m.m0 = make_float4(c0.x, c1.x, c2.x, c3.x);
    m.m1 = make_float4(c0.y, c1.y, c2.y, c3.y);
    m.m2 = make_float4(c0.z, c1.z, c2.z, c3.z);
    m.m3 = make_float4(c0.w, c1.w, c2.w, c3.w);
    return m;
}

float4x4 matrix_from_rows(float4 c0, float4 c1, float4 c2, float4 c3)
{
    float4x4 m;
    m.m0 = c0;
    m.m1 = c1;
    m.m2 = c2;
    m.m3 = c3;
    return m;
}

float4x4 matrix_from_rows3(float3 c0, float3 c1, float3 c2)
{
    float4x4 m;
    m.m0.xyz = c0; m.m0.w = 0;
    m.m1.xyz = c1; m.m1.w = 0;
    m.m2.xyz = c2; m.m2.w = 0;
    m.m3 = make_float4(0.f, 0.f, 0.f, 1.f);
    return m;
}

float4x4 matrix_from_cols3(float3 c0, float3 c1, float3 c2)
{
    float4x4 m;
    m.m0 = make_float4(c0.x, c1.x, c2.x, 0.f);
    m.m1 = make_float4(c0.y, c1.y, c2.y, 0.f);
    m.m2 = make_float4(c0.z, c1.z, c2.z, 0.f);
    m.m3 = make_float4(0.f, 0.f, 0.f, 1.f);
    return m;
}

float4x4 matrix_transpose(float4x4 m)
{
    return matrix_from_cols(m.m0, m.m1, m.m2, m.m3);
}

float4 matrix_mul_vector4(float4x4 m, float4 v)
{
    float4 res;
    res.x = dot(m.m0, v);
    res.y = dot(m.m1, v);
    res.z = dot(m.m2, v);
    res.w = dot(m.m3, v);
    return res;
}

float3 matrix_mul_vector3(float4x4 m, float3 v)
{
    float3 res;
    res.x = dot(m.m0.xyz, v);
    res.y = dot(m.m1.xyz, v);
    res.z = dot(m.m2.xyz, v);
    return res;
}

float3 matrix_mul_point3(float4x4 m, float3 v)
{
    float3 res;
    res.x = dot(m.m0.xyz, v) + m.m0.w;
    res.y = dot(m.m1.xyz, v) + m.m1.w;
    res.z = dot(m.m2.xyz, v) + m.m2.w;
    return res;
}

/// Linearly interpolate between two values
float4 lerp(float4 a, float4 b, float w)
{
    return a + w*(b-a);
}

/// Linearly interpolate between two values
float3 lerp3(float3 a, float3 b, float w)
{
	return a + w*(b - a);
}

/// Translate cartesian coordinates to spherical system
void CartesianToSpherical ( float3 cart, float* r, float* phi, float* theta )
{
    float temp = atan2(cart.x, cart.z);
    *r = sqrt(cart.x*cart.x + cart.y*cart.y + cart.z*cart.z);
    // Account for discontinuity
    *phi = (float)((temp >= 0)?temp:(temp + 2*PI));
    *theta = acos(cart.y/ *r);
}

/// Get vector orthogonal to a given one
float3 GetOrthoVector(float3 n)
{
    float3 p;

    if (fabs(n.z) > 0.f) {
        float k = sqrt(n.y*n.y + n.z*n.z);
        p.x = 0; p.y = -n.z/k; p.z = n.y/k;
    }
    else {
        float k = sqrt(n.x*n.x + n.y*n.y);
        p.x = n.y/k; p.y = -n.x/k; p.z = 0;
    }

    return normalize(p);
}

uint upper_power_of_two(uint v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}


#endif // UTILS_CL
