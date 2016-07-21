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
#pragma once

#include <cmath>
#include <algorithm>
#include <limits>

#include "float3.h"
#include "ray.h"

namespace RadeonRays
{
    class bbox
    {
    public:
        bbox()
            : pmin(float3(std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()))
            , pmax(float3(-std::numeric_limits<float>::max(),
            -std::numeric_limits<float>::max(),
            -std::numeric_limits<float>::max()))
        {
        }

        bbox(float3 const& p)
            : pmin(p)
            , pmax(p)
        {
        }

        bbox(float3 const& p1, float3 const& p2)
            : pmin(vmin(p1, p2))
            , pmax(vmax(p1, p2))
        {
        }

        float3 center()  const { return 0.5f * (pmax + pmin); }
        float3 extents() const { return pmax - pmin; }

        bool  contains(float3 const& p) const;

        int   maxdim() const;

        float surface_area() const
        {
            float3 ext = extents();
            return 2.f * (ext.x * ext.y + ext.x * ext.z + ext.y * ext.z);
        }

        // TODO: this is non-portable, optimization trial for fast intersection test
        float3 const& operator [] (int i) const { return *(&pmin + i); }

        // Grow the bounding box by a point
        void grow(float3 const& p)
        {
            vmin(pmin, p, pmin);
            vmax(pmax, p, pmax);
        }
        // Grow the bounding box by a box
        void grow(bbox const& b)
        {
            vmin(pmin, b.pmin, pmin);
            vmax(pmax, b.pmax, pmax);
        }


        float3 pmin;
        float3 pmax;
    };

    inline bool   bbox::contains(float3 const& p) const
    {
        float3 radius = 0.5f * extents();
        return std::abs(center().x - p.x) <= radius.x &&
            fabs(center().y - p.y) <= radius.y &&
            fabs(center().z - p.z) <= radius.z;
    }

    inline bbox bboxunion(bbox const& box1, bbox const& box2)
    {
        bbox res;
        res.pmin = vmin(box1.pmin, box2.pmin);
        res.pmax = vmax(box1.pmax, box2.pmax);
        return res;
    }

    inline bbox intersection(bbox const& box1, bbox const& box2)
    {
        return bbox(vmax(box1.pmin, box2.pmin), vmin(box1.pmax, box2.pmax));
    }

    inline void intersection(bbox const& box1, bbox const& box2, bbox& box)
    {
        vmax(box1.pmin, box2.pmin, box.pmin);
        vmin(box1.pmax, box2.pmax, box.pmax);
    }

#define BBOX_INTERSECTION_EPS 0.f

    inline bool intersects(bbox const& box1, bbox const& box2)
    {
        float3 b1c = box1.center();
        float3 b1r = 0.5f * box1.extents();
        float3 b2c = box2.center();
        float3 b2r = 0.5f * box2.extents();

        return (fabs(b2c.x - b1c.x) - (b1r.x + b2r.x)) <= BBOX_INTERSECTION_EPS &&
            (fabs(b2c.y - b1c.y) - (b1r.y + b2r.y)) <= BBOX_INTERSECTION_EPS &&
            (fabs(b2c.z - b1c.z) - (b1r.z + b2r.z)) <= BBOX_INTERSECTION_EPS;
    }

    inline bool contains(bbox const& box1, bbox const& box2)
    {
        return box1.contains(box2.pmin) && box1.contains(box2.pmax);
    }

    // Fast bbox test: PBRT book
    inline bool intersects(ray& r, float3 const& invrd, bbox const& box, int dirneg[3], float maxt) 
    {
        // Check for ray intersection against $x$ and $y$ slabs
        float tmin =  (box[  dirneg[0]].x - r.o.x) * invrd.x;
        float tmax =  (box[1-dirneg[0]].x - r.o.x) * invrd.x;
        float tymin = (box[  dirneg[1]].y - r.o.y) * invrd.y;
        float tymax = (box[1-dirneg[1]].y - r.o.y) * invrd.y;
        if ((tmin > tymax) || (tymin > tmax))
            return false;
        if (tymin > tmin) tmin = tymin;
        if (tymax < tmax) tmax = tymax;

        // Check for ray intersection against $z$ slab
        float tzmin = (box[  dirneg[2]].z - r.o.z) * invrd.z;
        float tzmax = (box[1-dirneg[2]].z - r.o.z) * invrd.z;
        if ((tmin > tzmax) || (tzmin > tmax))
            return false;
        if (tzmin > tmin)
            tmin = tzmin;
        if (tzmax < tmax)
            tmax = tzmax;

        return (tmin <= maxt) && (tmax > 0.f);
    }

    inline bool intersects(ray& r, float3 const& invrd, bbox const& box, int dirneg[3], float maxt, float& t)
    {
        // Check for ray intersection against $x$ and $y$ slabs
        float tmin = (box[dirneg[0]].x - r.o.x) * invrd.x;
        float tmax = (box[1 - dirneg[0]].x - r.o.x) * invrd.x;
        float tymin = (box[dirneg[1]].y - r.o.y) * invrd.y;
        float tymax = (box[1 - dirneg[1]].y - r.o.y) * invrd.y;
        if ((tmin > tymax) || (tymin > tmax))
            return false;
        if (tymin > tmin) tmin = tymin;
        if (tymax < tmax) tmax = tymax;

        // Check for ray intersection against $z$ slab
        float tzmin = (box[dirneg[2]].z - r.o.z) * invrd.z;
        float tzmax = (box[1 - dirneg[2]].z - r.o.z) * invrd.z;
        if ((tmin > tzmax) || (tzmin > tmax))
            return false;
        if (tzmin > tmin)
            tmin = tzmin;
        if (tzmax < tmax)
            tmax = tzmax;

        t = tmin >= 0.f ? tmin : tmax;

        return (tmin <= maxt) && (tmax > 0.f);
    }

    inline int bbox::maxdim() const
    {
        float3 ext = extents();

        if (ext.x >= ext.y && ext.x >= ext.z)
            return 0;
        if (ext.y >= ext.x && ext.y >= ext.z)
            return 1;
        if (ext.z >= ext.x && ext.z >= ext.y)
            return 2;

        return 0;
    }
}
