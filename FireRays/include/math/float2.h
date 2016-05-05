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

#ifndef FLOAT2_H
#define FLOAT2_H

#include <cmath>
#include <algorithm>

namespace FireRays
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

#endif // FLOAT2_H