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

float matrix_get(matrix m, int i, int j)
{
    return m.m[i * 4 + j];
}

matrix matrix_from_cols(float4 c0, float4 c1, float4 c2, float4 c3)
{
    matrix m;
    m.rows.m0 = make_float4(c0.x, c1.x, c2.x, c3.x);
    m.rows.m1 = make_float4(c0.y, c1.y, c2.y, c3.y);
    m.rows.m2 = make_float4(c0.z, c1.z, c2.z, c3.z);
    m.rows.m3 = make_float4(c0.w, c1.w, c2.w, c3.w);
    return m;
}

matrix matrix_from_rows(float4 c0, float4 c1, float4 c2, float4 c3)
{
    matrix m;
    m.rows.m0 = c0;
    m.rows.m1 = c1;
    m.rows.m2 = c2;
    m.rows.m3 = c3;
    return m;
}

matrix matrix_from_rows3(float3 c0, float3 c1, float3 c2)
{
    matrix m;
    m.rows.m0.xyz = c0; m.rows.m0.w = 0;
    m.rows.m1.xyz = c1; m.rows.m1.w = 0;
    m.rows.m2.xyz = c2; m.rows.m2.w = 0;
    m.rows.m3 = make_float4(0.f, 0.f, 0.f, 1.f);
    return m;
}

matrix matrix_from_cols3(float3 c0, float3 c1, float3 c2)
{
    matrix m;
    m.rows.m0 = make_float4(c0.x, c1.x, c2.x, 0.f);
    m.rows.m1 = make_float4(c0.y, c1.y, c2.y, 0.f);
    m.rows.m2 = make_float4(c0.z, c1.z, c2.z, 0.f);
    m.rows.m3 = make_float4(0.f, 0.f, 0.f, 1.f);
    return m;
}

matrix matrix_transpose(matrix m)
{
    return matrix_from_cols(m.rows.m0, m.rows.m1, m.rows.m2, m.rows.m3);
}

float4 matrix_mul_vector4(matrix m, float4 v)
{
    float4 res;
    res.x = dot(m.rows.m0, v);
    res.y = dot(m.rows.m1, v);
    res.z = dot(m.rows.m2, v);
    res.w = dot(m.rows.m3, v);
    return res;
}

float3 matrix_mul_vector3(matrix m, float3 v)
{
    float3 res;
    res.x = dot(m.rows.m0.xyz, v);
    res.y = dot(m.rows.m1.xyz, v);
    res.z = dot(m.rows.m2.xyz, v);
    return res;
}

float3 matrix_mul_point3(matrix m, float3 v)
{
    float3 res;
    res.x = dot(m.rows.m0.xyz, v) + m.rows.m0.w;
    res.y = dot(m.rows.m1.xyz, v) + m.rows.m1.w;
    res.z = dot(m.rows.m2.xyz, v) + m.rows.m2.w;
    return res;
}





/// Transform point with transformation matrix.
/// m0...m3 are matrix rows
float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3)
{
    float3 res;
    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3;
    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3;
    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3;
    return res;
}

/// Transform vector with transformation matrix (no translation involved)
/// m0...m3 are matrix rows
float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3)
{
    float3 res;
    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z;
    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z;
    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z;
    return res;
}

/// Multiply two quaternions
float4 quaternion_mul(float4 q1, float4 q2)
{
    float4 res;
    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x;
    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y;
    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z;
    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
    return res;
}

/// Calculate conjugate quaternion
float4 quaternion_conjugate(float4 q)
{
    return make_float4(-q.x, -q.y, -q.z, q.w);
}


/// Inverse quaternion
float4 quaternion_inverse(float4 q)
{
    float sqnorm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;

    /// Check if it is singular
    if (sqnorm != 0.f)
    {
        return quaternion_conjugate(q) / sqnorm;
    }
    else
    {
        return make_float4(0.f, 0.f, 0.f, 1.f);
    }
}

/// Rotate a vector using quaternion
float3 rotate_vector(float3 v, float4 q)
{
    // The formula is v' = q * v * q_inv;
    float4 qinv = quaternion_inverse(q);
    float4 vv = make_float4(v.x, v.y, v.z, 0);
    return quaternion_mul(q, quaternion_mul(vv, qinv)).xyz;
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
