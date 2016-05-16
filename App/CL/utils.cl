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

// 2D distribution function
typedef struct __Distribution2D
{
	int w;
	int h;
	__global float const* data;
} Distribution2D;

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
float3 lerp(float3 a, float3 b, float w)
{
    return a + w*(b-a);
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
    
    if (fabs(n.z) > 0.7) {
        float k = sqrt(n.y*n.y + n.z*n.z);
        p.x = 0; p.y = -n.z/k; p.z = n.y/k;
    }
    else {
        float k = sqrt(n.x*n.x + n.y*n.y);
        p.x = n.y/k; p.y = -n.x/k; p.z = 0;
    }

    return normalize(p);
}

float2 Distribution2D_Sample(Distribution2D const* dist, float2 sample, float* pdf)
{
	return make_float2(0.f, 0.f);
}

float Distribution2D_GetPdf(Distribution2D const* dist, float2 sample)
{
	return 0.f;
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