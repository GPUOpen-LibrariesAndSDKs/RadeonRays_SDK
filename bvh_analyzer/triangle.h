/**********************************************************************
Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
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
#pragma once

#include "aabb.h"
#include "intersection_primitives.h"

namespace bvh
{
struct Triangle
{
    Triangle() = default;
    Triangle(const float3& p0, const float3& p1, const float3& p2, uint32_t primitive)
        : v0(p0), v1(p1), v2(p2), prim_id(primitive)
    {
    }

    bool Intersect(const Ray& ray, float2& uv, float& t) const
    {
        float3 dir    = float3(ray.direction[0], ray.direction[1], ray.direction[2]);
        float3 origin = float3(ray.origin[0], ray.origin[1], ray.origin[2]);

        float3 e1 = v1 - v0;
        float3 e2 = v2 - v0;
        float3 s1 = cross(dir, e2);

        float denom = dot(s1, e1);

        if (denom == 0.f)
        {
            return false;
        }

        float  invd = 1.0f / denom;
        float3 d    = origin - v0;
        float  b1   = dot(d, s1) * invd;

        float3 s2 = cross(d, e1);
        float  b2 = dot(dir, s2) * invd;

        float temp = dot(e2, s2) * invd;

        if ((b1 < 0.f) || (b1 > 1.f) || (b2 < 0.f) || (b1 + b2 > 1.f) || (temp < ray.min_t) || (temp > ray.max_t))
        {
            return false;
        } else
        {
            uv.x = b1;
            uv.y = b2;
            t    = temp;
            return true;
        }
    }

    Aabb GetAabb() const
    {
        return Aabb().Grow(v0).Grow(v1).Grow(v2);
    }

    float3 Center() const { return Aabb().Center(); }

    float Area() const
    {
        float3 c = cross(v2 - v0, v1 - v0);
        return sqrt(dot(c, c)) * 0.5f;
    }

    float3   v0, v1, v2;
    uint32_t prim_id = kInvalidID;
};
}  // namespace bvh