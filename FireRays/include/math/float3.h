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

#ifndef FLOAT3_H
#define FLOAT3_H

#include <cmath>
#include <algorithm>

namespace FireRays
{
    class float3
    {
    public:
        float3(float xx = 0.f, float yy = 0.f, float zz = 0.f, float ww = 0.f) : x(xx), y(yy), z(zz), w(ww) {}

        float& operator [](int i)       { return *(&x + i); }
        float  operator [](int i) const { return *(&x + i); }
        float3 operator-() const        { return float3(-x, -y, -z); }

        float  sqnorm() const           { return x*x + y*y + z*z; }
        void   normalize()              { (*this)/=(std::sqrt(sqnorm()));} 

        float3& operator += (float3 const& o) { x+=o.x; y+=o.y; z+= o.z; return *this;}
        float3& operator -= (float3 const& o) { x-=o.x; y-=o.y; z-= o.z; return *this;}
        float3& operator *= (float3 const& o) { x*=o.x; y*=o.y; z*= o.z; return *this;}
        float3& operator *= (float c) { x*=c; y*=c; z*= c; return *this;}
        float3& operator /= (float c) { float cinv = 1.f/c; x*=cinv; y*=cinv; z*=cinv; return *this;}

        float x, y, z, w;
    };

    typedef float3 float4;


    inline float3 operator+(float3 const& v1, float3 const& v2)
    {
        float3 res = v1;
        return res+=v2;
    }

    inline float3 operator-(float3 const& v1, float3 const& v2)
    {
        float3 res = v1;
        return res-=v2;
    }

    inline float3 operator*(float3 const& v1, float3 const& v2)
    {
        float3 res = v1;
        return res*=v2;
    }

    inline float3 operator*(float3 const& v1, float c)
    {
        float3 res = v1;
        return res*=c;
    }

    inline float3 operator*(float c, float3 const& v1)
    {
        return operator*(v1, c);
    }

    inline float dot(float3 const& v1, float3 const& v2)
    {
        return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
    }

    inline float3 normalize(float3 const& v)
    {
        float3 res = v;
        res.normalize();
        return res;
    }

    inline float3 cross(float3 const& v1, float3 const& v2)
    {
        /// |i	 j	 k|
        /// |v1x  v1y  v1z|
        /// |v2x  v2y  v2z|
        return float3(v1.y * v2.z - v2.y * v1.z, v2.x * v1.z - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
    }

    inline float3 vmin(float3 const& v1, float3 const& v2)
    {
        return float3(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z));
    }

    inline void vmin(float3 const& v1, float3 const& v2, float3& v)
    {
        v.x = std::min(v1.x, v2.x); 
        v.y = std::min(v1.y, v2.y);
        v.z = std::min(v1.z, v2.z);
    }

    inline float3 vmax(float3 const& v1, float3 const& v2)
    {
        return float3(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z));
    }

    inline void vmax(float3 const& v1, float3 const& v2, float3& v)
    {
        v.x = std::max(v1.x, v2.x); 
        v.y = std::max(v1.y, v2.y);
        v.z = std::max(v1.z, v2.z);
    }
}

#endif // FLOAT3_H