/**********************************************************************
Copyright (c) 2018-2021 Advanced Micro Devices, Inc. All rights reserved.

Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2011 Erwin Coumans http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If
you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not
required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original
software.
3. This notice may not be removed or altered from any source distribution.
********************************************************************/
#define RR_INVALID_ADDR 0xffffffffu

const float PI = 3.1415926535897932384626433832795;
const float PI_2 = 1.57079632679489661923;
const float PI_4 = 0.785398163397448309616;

const float unit_side = 1024.0f;

struct Ray
{
    vec3 origin;
    // Intersection min distance
    float min_t;
    vec3 direction;
    // Intersection distance
    float max_t;
};

struct Hit
{
    vec2 uv;
    uint shape_id;
    uint prim_id;
};

struct Transform
{
    vec4 m0;
    vec4 m1;
    vec4 m2;
};

struct InstanceDescription
{
    Transform transform;
    uint   index;
    uint   padding[3];
};

struct Aabb
{
    vec3 pmin;
    vec3 pmax;
};

//=====================================================================================================================
// Magic found here: https://github.com/kripken/bullet/blob/master/
//                   src/BulletMultiThreaded/GpuSoftBodySolvers/DX11/HLSL/ComputeBounds.hlsl
uvec3 Float3ToUint3(in vec3 v)
{
    // Reinterpret value as uint
    uvec3 value_as_uint = uvec3(floatBitsToUint(v.x), floatBitsToUint(v.y), floatBitsToUint(v.z));

    // Invert sign bit of positives and whole of  to anegativesllow comparison as unsigned ints
    value_as_uint.x ^= (1 + ~(value_as_uint.x >> 31) | 0x80000000);
    value_as_uint.y ^= (1 + ~(value_as_uint.y >> 31) | 0x80000000);
    value_as_uint.z ^= (1 + ~(value_as_uint.z >> 31) | 0x80000000);

    return value_as_uint;
}

//=====================================================================================================================
// Magic found here: https://github.com/kripken/bullet/blob/master/src/BulletMultiThreaded/GpuSoftBodySolvers/DX11/btSoftBodySolver_DX11.cpp
vec3 Uint3ToFloat3(in uvec3 v)
{
    v.x ^= (((v.x >> 31) - 1) | 0x80000000);
    v.y ^= (((v.y >> 31) - 1) | 0x80000000);
    v.z ^= (((v.z >> 31) - 1) | 0x80000000);

    return vec3(uintBitsToFloat(v.x), uintBitsToFloat(v.y), uintBitsToFloat(v.z));
}

float mymin3(float a, float b, float c)
{
    return min(c, min(a,b));
}

float mymax3(float a, float b, float c)
{
    return max(c, max(a,b));
}

// Ray-triangle test implementation.
float fast_intersect_triangle(Ray r,
                              vec3 v1,
                              vec3 v2,
                              vec3 v3,
                              float t_max)
{
    vec3 e1 = v2 - v1;
    vec3 e2 = v3 - v1;
    vec3 s1 = cross(r.direction, e2);

    float denom = dot(s1, e1);

    if (denom == 0.f)
    {
        return t_max;
    }

    float invd = 1.0 / denom;
    vec3 d = r.origin - v1;
    float b1 = dot(d, s1) * invd;
    vec3 s2 = cross(d, e1);
    float b2 = dot(r.direction, s2) * invd;
    float temp = dot(e2, s2) * invd;

    if (b1 < 0.0 || b1 > 1.0 || 
        b2 < 0.0 || b1 + b2 > 1.0 || 
        temp < r.min_t || temp > t_max)
    {
        return t_max;
    }
    else
    {
        return temp;
    }
}

struct Tris
{
    vec3 v0;
    uint mesh_id;
    vec3 v1;
    uint prim_id;
    vec3 v2;    
    uint unused;
};

// Ray-AABB test implementation.
vec2 fast_intersect_aabb(vec3 pmin,
                         vec3 pmax,
                         vec3 invdir,
                         vec3 oxinvdir,
                         float t_max,
                         float t_min)
{
    vec3 f = fma(pmax, invdir, oxinvdir);
    vec3 n = fma(pmin, invdir, oxinvdir);
    vec3 tmax = max(f, n);
    vec3 tmin = min(f, n);
    float t1 = min(mymin3(tmax.x, tmax.y, tmax.z), t_max);
    float t0 = max(mymax3(tmin.x, tmin.y, tmin.z), t_min);
    return vec2(t0, t1);
}

float mycopysign(float a, float b)
{
   return b < 0.f ? -a : a;
}

// Invert ray direction.
vec3 safe_invdir(vec3 d)
{
    float dirx = d.x;
    float diry = d.y;
    float dirz = d.z;
    float ooeps = 1e-5;
    vec3 invdir;
    invdir.x = 1.0 / (abs(dirx) > ooeps ? dirx : mycopysign(ooeps, dirx));
    invdir.y = 1.0 / (abs(diry) > ooeps ? diry : mycopysign(ooeps, diry));
    invdir.z = 1.0 / (abs(dirz) > ooeps ? dirz : mycopysign(ooeps, dirz));
    return invdir;
}

// Calculate barycentric coordinates of a point
// within a triangle.
vec2 calculate_barycentrics(vec3 p, vec3 v1, vec3 v2, vec3 v3)
{
    vec3 e1 = v2 - v1;
    vec3 e2 = v3 - v1;
    vec3 e = p - v1;
    float d00 = dot(e1, e1);
    float d01 = dot(e1, e2);
    float d11 = dot(e2, e2);
    float d20 = dot(e, e1);
    float d21 = dot(e, e2);

    float denom = (d00 * d11 - d01 * d01);

    if (denom == 0.f)
    {
        return vec2(0.0);
    }

    float invdenom = 1.0 / (d00 * d11 - d01 * d01);
    float b1 = (d11 * d20 - d01 * d21) * invdenom;
    float b2 = (d00 * d21 - d01 * d20) * invdenom;
    return vec2(b1, b2);
}

Aabb create_aabb_from_point(vec3 p)
{
    Aabb aabb;
    aabb.pmin = p;
    aabb.pmax = p;
    return aabb;
}

Aabb create_aabb_from_minmax(vec3 pmin, vec3 pmax)
{
    Aabb aabb;
    aabb.pmin = pmin;
    aabb.pmax = pmax;
    return aabb;
}

Aabb calculate_aabb_union(Aabb b0, Aabb b1)
{
    Aabb aabb;
    aabb.pmin = min(b0.pmin, b1.pmin);
    aabb.pmax = max(b0.pmax, b1.pmax);
    return aabb;
}

void grow_aabb(inout Aabb aabb, vec3 p)
{
    aabb.pmin = min(aabb.pmin, p);
    aabb.pmax = max(aabb.pmax, p);
}

Aabb calculate_aabb_for_triangle(vec3 v0, vec3 v1, vec3 v2)
{
    Aabb aabb = create_aabb_from_point(v0);
    grow_aabb(aabb, v1);
    grow_aabb(aabb, v2);

    return aabb;
}

// 3-dilate a number
uint expand_bits(uint r)
{
    r = (r * 0x00010001u) & 0xFF0000FFu;
    r = (r * 0x00000101u) & 0x0F00F00Fu;
    r = (r * 0x00000011u) & 0xC30C30C3u;
    r = (r * 0x00000005u) & 0x49249249u;
    return r;
}

// Calculate and pack Morton code for the point
uint calculate_morton_code(vec3 p)
{
    float x = clamp(p.x * unit_side, 0.0f, unit_side - 1.0f);
    float y = clamp(p.y * unit_side, 0.0f, unit_side - 1.0f);
    float z = clamp(p.z * unit_side, 0.0f, unit_side - 1.0f);

    return (expand_bits(uint(x)) << 2) | (expand_bits(uint(y)) << 1) | expand_bits(uint(z));
}

vec3 transform_point(vec3 p, Transform transform)
{
    vec3 result;
    result.x = dot(transform.m0.xyz, p) + transform.m0.w;
    result.y = dot(transform.m1.xyz, p) + transform.m1.w;
    result.z = dot(transform.m2.xyz, p) + transform.m2.w;

    return result;
}

void transform_aabb(inout Aabb aabb, Transform t)
{
    vec3 p0 = aabb.pmin;
    vec3 p1 = vec3(aabb.pmin.x, aabb.pmin.y, aabb.pmax.z);
    vec3 p2 = vec3(aabb.pmin.x, aabb.pmax.y, aabb.pmin.z);
    vec3 p3 = vec3(aabb.pmin.x, aabb.pmax.y, aabb.pmax.z);
    vec3 p4 = vec3(aabb.pmax.x, aabb.pmin.y, aabb.pmax.z);
    vec3 p5 = vec3(aabb.pmax.x, aabb.pmax.y, aabb.pmin.z);
    vec3 p6 = vec3(aabb.pmax.x, aabb.pmax.y, aabb.pmax.z);
    vec3 p7 = aabb.pmax;

    p0 = transform_point(p0, t);
    p1 = transform_point(p1, t);
    p2 = transform_point(p2, t);
    p3 = transform_point(p3, t);
    p4 = transform_point(p4, t);
    p5 = transform_point(p5, t);
    p6 = transform_point(p6, t);
    p7 = transform_point(p7, t);

    aabb = create_aabb_from_point(p0);
    grow_aabb(aabb, p1);
    grow_aabb(aabb, p2);
    grow_aabb(aabb, p3);
    grow_aabb(aabb, p4);
    grow_aabb(aabb, p5);
    grow_aabb(aabb, p6);
    grow_aabb(aabb, p7);
}

void transform_ray(Transform transform, inout Ray ray)
{
    vec3 o;
    o.x = dot(transform.m0.xyz, ray.origin.xyz) + transform.m0.w;
    o.y = dot(transform.m1.xyz, ray.origin.xyz) + transform.m1.w;
    o.z = dot(transform.m2.xyz, ray.origin.xyz) + transform.m2.w;

    vec3 d;
    d.x = dot(transform.m0.xyz, ray.direction.xyz);
    d.y = dot(transform.m1.xyz, ray.direction.xyz);
    d.z = dot(transform.m2.xyz, ray.direction.xyz);

    ray.origin.xyz = o;
    ray.direction.xyz = d;
}

Transform inverse(Transform t)
{
    mat4 m;
    m[0] = vec4(t.m0.x, t.m1.x, t.m2.x, 0);
    m[1] = vec4(t.m0.y, t.m1.y, t.m2.y, 0);
    m[2] = vec4(t.m0.z, t.m1.z, t.m2.z, 0);
    m[3] = vec4(t.m0.w, t.m1.w, t.m2.w, 1);

    m = inverse(m);

    Transform res;
    res.m0 = vec4(m[0].x, m[1].x, m[2].x, m[3].x);
    res.m1 = vec4(m[0].y, m[1].y, m[2].y, m[3].y);
    res.m2 = vec4(m[0].z, m[1].z, m[2].z, m[3].z);

    return res;
}