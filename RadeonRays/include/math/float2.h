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

namespace RadeonRays
{
    class float2
    {
    public:
        float2(float xx = 0.f, float yy = 0.f) : x(xx), y(yy) {}

        float& operator [](int i)       { return *(&x + i); }
        float  operator [](int i) const { return *(&x + i); }
        float2 operator-() const        { return float2(-x, -y); }

        float  sqnorm() const           { return x*x + y*y; }
        void   normalize()              { (*this)/=(std::sqrt(sqnorm()));} 

        float2& operator += (float2 const& o) { x+=o.x; y+=o.y; return *this; }
        float2& operator -= (float2 const& o) { x-=o.x; y-=o.y; return *this; }
        float2& operator *= (float2 const& o) { x*=o.x; y*=o.y; return *this; }
        float2& operator *= (float c) { x*=c; y*=c; return *this; }
        float2& operator /= (float c) { float cinv = 1.f/c; x*=cinv; y*=cinv; return *this; }

        float x, y;
    };


    inline float2 operator+(float2 const& v1, float2 const& v2)
    {
        float2 res = v1;
        return res+=v2;
    }

    inline float2 operator-(float2 const& v1, float2 const& v2)
    {
        float2 res = v1;
        return res-=v2;
    }

    inline float2 operator*(float2 const& v1, float2 const& v2)
    {
        float2 res = v1;
        return res*=v2;
    }

    inline float2 operator*(float2 const& v1, float c)
    {
        float2 res = v1;
        return res*=c;
    }

    inline float2 operator*(float c, float2 const& v1)
    {
        return operator*(v1, c);
    }

    inline float dot(float2 const& v1, float2 const& v2)
    {
        return v1.x * v2.x + v1.y * v2.y;
    }

    inline float2 normalize(float2 const& v)
    {
        float2 res = v;
        res.normalize();
        return res;
    }

    inline float2 vmin(float2 const& v1, float2 const& v2)
    {
        return float2(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
    }


    inline float2 vmax(float2 const& v1, float2 const& v2)
    {
        return float2(std::max(v1.x, v2.x), std::max(v1.y, v2.y));
    }
}
