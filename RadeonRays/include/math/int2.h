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

#include "float2.h"

namespace RadeonRays
{
    class int2
    {
    public:
        int2(int xx = 0, int yy = 0) : x(xx), y(yy) {}

        int&  operator [](int i)       { return *(&x + i); }
        int   operator [](int i) const { return *(&x + i); }
        int2  operator-() const        { return int2(-x, -y); }

        int   sqnorm() const           { return x*x + y*y; }

        operator float2()     { return float2((float)x, (float)y); }

        int2& operator += (int2 const& o) { x+=o.x; y+=o.y; return *this; }
        int2& operator -= (int2 const& o) { x-=o.x; y-=o.y; return *this; }
        int2& operator *= (int2 const& o) { x*=o.x; y*=o.y; return *this; }
        int2& operator *= (int c) { x*=c; y*=c; return *this; }

        int x, y;
    };


    inline int2 operator+(int2 const& v1, int2 const& v2)
    {
        int2 res = v1;
        return res+=v2;
    }

    inline int2 operator-(int2 const& v1, int2 const& v2)
    {
        int2 res = v1;
        return res-=v2;
    }

    inline int2 operator*(int2 const& v1, int2 const& v2)
    {
        int2 res = v1;
        return res*=v2;
    }

    inline int2 operator*(int2 const& v1, int c)
    {
        int2 res = v1;
        return res*=c;
    }

    inline int2 operator*(int c, int2 const& v1)
    {
        return operator*(v1, c);
    }

    inline int dot(int2 const& v1, int2 const& v2)
    {
        return v1.x * v2.x + v1.y * v2.y;
    }


    inline int2 vmin(int2 const& v1, int2 const& v2)
    {
        return int2(std::min(v1.x, v2.x), std::min(v1.y, v2.y));
    }

    inline int2 vmax(int2 const& v1, int2 const& v2)
    {
        return int2(std::max(v1.x, v2.x), std::max(v1.y, v2.y));
    }
}