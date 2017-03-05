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

/*************************************************************************
DEFINES
**************************************************************************/
#define PI 3.14159265358979323846f

typedef struct _bbox
{
    float4 pmin;
    float4 pmax;
} bbox;

typedef struct _ray
{
    float4 o;
    float4 d;
    int2 extra;
    int2 padding;
} ray;

typedef struct _Intersection
{
    int shapeid;
    int primid;
    int padding0;
    int padding1;

    float4 uvwt;
} Intersection;

typedef struct _ShapeData
{
    int id;
    int bvhidx;
    int mask;
    int padding1;
    float4 m0;
    float4 m1;
    float4 m2;
    float4 m3;
    float4  linearvelocity;
    float4  angularvelocity;
} ShapeData;

typedef bbox BvhNode;

typedef struct _Face
{
    // Vertex indices
    int idx[3];
    int shapeidx;
    // Primitive ID
    int id;
    // Idx count
    int cnt;
} Face;

#ifndef APPLE

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

int3 make_int3(int x, int y, int z)
{
    int3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}

#endif

float3 transform_point(float3 p, float4 m0, float4 m1, float4 m2, float4 m3)
{
    float3 res;
    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z + m0.s3;
    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z + m1.s3;
    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z + m2.s3;
    return res;
}

float3 transform_vector(float3 p, float4 m0, float4 m1, float4 m2, float4 m3)
{
    float3 res;
    res.x = m0.s0 * p.x + m0.s1 * p.y + m0.s2 * p.z;
    res.y = m1.s0 * p.x + m1.s1 * p.y + m1.s2 * p.z;
    res.z = m2.s0 * p.x + m2.s1 * p.y + m2.s2 * p.z;
    return res;
}

ray transform_ray(ray r, float4 m0, float4 m1, float4 m2, float4 m3)
{
    ray res;
    res.o.xyz = transform_point(r.o.xyz, m0, m1, m2, m3);
    res.d.xyz = transform_vector(r.d.xyz, m0, m1, m2, m3);
    res.o.w = r.o.w;
    res.d.w = r.d.w;
    return res;
}

float4 quaternion_mul(float4 q1, float4 q2)
{
    float4 res;
    res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x;
    res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y;
    res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z;
    res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
    return res;
}

float4 quaternion_conjugate(float4 q)
{
    return make_float4(-q.x, -q.y, -q.z, q.w);
}

float4 quaternion_inverse(float4 q)
{
    float sqnorm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    
    if (sqnorm != 0.f)
    {
        return quaternion_conjugate(q) / sqnorm;
    }
    else
    {
        return make_float4(0.f, 0.f, 0.f, 1.f);
    }
}

void rotate_ray(ray* r, float4 q)
{
    float4 qinv = quaternion_inverse(q);
    float4 v = make_float4(r->o.x, r->o.y, r->o.z, 0);
    v = quaternion_mul(qinv, quaternion_mul(v, q));
    r->o.xyz = v.xyz;
    v = make_float4(r->d.x, r->d.y, r->d.z, 0);
    v = quaternion_mul(qinv, quaternion_mul(v, q));
    r->d.xyz = v.xyz;
}

// Intersect Ray against triangle
int IntersectTriangle(ray const* r, float3 v1, float3 v2, float3 v3, Intersection* isect)
{
    const float3 e1 = v2 - v1;
    const float3 e2 = v3 - v1;
    const float3 s1 = cross(r->d.xyz, e2);
    const float  invd = native_recip(dot(s1, e1));
    const float3 d = r->o.xyz - v1;
    const float  b1 = dot(d, s1) * invd;
    const float3 s2 = cross(d, e1);
    const float  b2 = dot(r->d.xyz, s2) * invd;
    const float temp = dot(e2, s2) * invd;
    
    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f
        || temp < 0.f || temp > isect->uvwt.w)
    {
        return 0;
    }
    else
    {
        isect->uvwt = make_float4(b1, b2, 0.f, temp);
        return 1;
    }
}

int IntersectTriangleP(ray const* r, float3 v1, float3 v2, float3 v3)
{
    const float3 e1 = v2 - v1;
    const float3 e2 = v3 - v1;
    const float3 s1 = cross(r->d.xyz, e2);
    const float  invd = native_recip(dot(s1, e1));
    const float3 d = r->o.xyz - v1;
    const float  b1 = dot(d, s1) * invd;
    const float3 s2 = cross(d, e1);
    const float  b2 = dot(r->d.xyz, s2) * invd;
    const float temp = dot(e2, s2) * invd;
    
    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f
        || temp < 0.f || temp > r->o.w)
    {
        return 0;
    }
    
    return 1;
}

#ifdef AMD_MEDIA_OPS
#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable
#endif

// Intersect ray with the axis-aligned box
int IntersectBox(ray const* r, float3 invdir, bbox box, float maxt)
{
    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir;
    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir;

    const float3 tmax = max(f, n);
    const float3 tmin = min(f, n);

#ifndef AMD_MEDIA_OPS
    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt);
    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f);
#else
    const float t1 = min(amd_min3(tmax.x, tmax.y, tmax.z), maxt);
    const float t0 = max(amd_max3(tmin.x, tmin.y, tmin.z), 0.f);
#endif

    return (t1 >= t0) ? 1 : 0;
}

float IntersectBoxF(ray const* r, float3 invdir, bbox box, float maxt)
{
    const float3 f = (box.pmax.xyz - r->o.xyz) * invdir;
    const float3 n = (box.pmin.xyz - r->o.xyz) * invdir;

    const float3 tmax = max(f, n);
    const float3 tmin = min(f, n);


#ifndef AMD_MEDIA_OPS
    const float t1 = min(min(tmax.x, min(tmax.y, tmax.z)), maxt);
    const float t0 = max(max(tmin.x, max(tmin.y, tmin.z)), 0.f);
#else
    const float t1 = min(amd_min3(tmax.x, tmax.y, tmax.z), maxt);
    const float t0 = max(amd_max3(tmin.x, tmin.y, tmin.z), 0.f);
#endif

    return (t1 >= t0) ? (t0 > 0.f ? t0 : t1) : -1.f;
}

int Ray_GetMask(ray const* r)
{
    return r->extra.x;
}

int Ray_IsActive(ray const* r)
{
    return r->extra.y;
}

float Ray_GetMaxT(ray const* r)
{
    return r->o.w;
}

float Ray_GetTime(ray const* r)
{
    return r->d.w;
}