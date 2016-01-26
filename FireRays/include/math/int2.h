/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#ifndef INT2_H
#define INT2_H


#include <cmath>
#include <algorithm>

#include "float2.h"

namespace FireRays
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

#endif // FLOAT2_H