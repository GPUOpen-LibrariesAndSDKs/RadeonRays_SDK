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
#include <cfloat>
#include <ostream>

#include "float2.h"
#include "float3.h"

namespace bvh
{
struct Aabb
{
    //<! Create AABB from a single point.
    Aabb() : pmin{FLT_MAX, FLT_MAX, FLT_MAX}, pmax{-FLT_MAX, -FLT_MAX, -FLT_MAX} {}
    //<! Create AABB from a single point.
    Aabb(const float3& p) : pmin(p), pmax(p) {}
    //<! Create AABB from min and max points.
    Aabb(const float3& mi, const float3& ma) : pmin(mi), pmax(ma) {}
    //<! Create AABB from another AABB.
    Aabb(const Aabb& rhs) : pmin(rhs.pmin), pmax(rhs.pmax) {}
    //<! Grow AABB to enclose itself and another AABB.
    Aabb& Grow(const Aabb& rhs)
    {
        pmin = vmin(pmin, rhs.pmin);
        pmax = vmax(pmax, rhs.pmax);
        return *this;
    }
    //<! Grow AABB to enclose itself and another point.
    Aabb& Grow(const float3& p)
    {
        pmin = vmin(pmin, p);
        pmax = vmax(pmax, p);
        return *this;
    }
    //<! Box center.
    float3 Center() const { return (pmax + pmin) * 0.5; }

    //<! Box extens along each axe.
    float3 Extents() const { return pmax - pmin; }
    //<! Calculate AABB union.
    static Aabb Union(const Aabb& rhs, const Aabb& lhs)
    {
        Aabb result(vmin(lhs.pmin, rhs.pmin), vmax(lhs.pmax, rhs.pmax));
        return result;
    }

    //<! Box extens along each axe.
    float Area() const
    {
        float3 ext = Extents();
        return 2 * (ext.x * ext.y + ext.x * ext.z + ext.y * ext.z);
    }

    //<! Calculate AABB vs ray intersection distances.
    float2 Intersect(const float3& invD, const float3& oxInvD, float minT, float maxT) const
    {
        float3 f    = fma(pmax, invD, oxInvD);
        float3 n    = fma(pmin, invD, oxInvD);
        float3 tmax = vmax(f, n);
        float3 tmin = vmin(f, n);
        float  t1   = fminf(fminf(fminf(tmax.x, tmax.y), tmax.z), maxT);
        float  t0   = fmaxf(fmaxf(fmaxf(tmin.x, tmin.y), tmin.z), minT);
        return float2{t0, t1};
    }

    bool Includes(Aabb const& rhs) const
    {
        float3 min_diff = rhs.pmin - pmin;
        float3 max_diff = pmax - rhs.pmax;
        return !(min_diff.x < -1e-8f || min_diff.y < -1e-8f || min_diff.z < -1e-8f || max_diff.x < -1e-8f ||
                 max_diff.y < -1e-8f || max_diff.z < -1e-8f);
    }

    //<! Min point of AABB.
    float3 pmin;
    //<! Max point of AABB.
    float3 pmax;
};

inline std::ostream& operator<<(std::ostream& oss, const Aabb& aabb)
{
    oss << "{ min: [" << aabb.pmin.x << ", " << aabb.pmin.y << ", " << aabb.pmin.z << "], ";
    oss << " max: [" << aabb.pmax.x << ", " << aabb.pmax.y << ", " << aabb.pmax.z << "] }";
    return oss;
}
}  // namespace bvh