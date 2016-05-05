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

#ifndef QUATERNION_H
#define QUATERNION_H

#include <cmath>
#include "matrix.h"

namespace FireRays
{
    class quaternion
    {
    public:
        quaternion (float xx = 0.f, float yy = 0.f, float zz = 0.f, float ww = 1.f) : x(xx), y(yy), z(zz), w(ww) {}

        //explicit			   quaternion( const vector<T,4>& v );
        /// create quaternion from a orthogonal(!) matrix
        /// make sure the matrix is ORTHOGONAL
        explicit			   quaternion( const matrix& m );

        /// convert quaternion to matrix
        //void				   to_matrix( matrix<T,4,4>& pM ) const;
        void to_matrix(matrix& m) const;

        quaternion      operator -() const { return quaternion(-x, -y, -z, -w); }
        quaternion&     operator +=( quaternion const& o ) { x+=o.x; y+=o.y; z+=o.z; w+=o.w; return *this; }
        quaternion&     operator -=( quaternion const& o ) { x-=o.x; y-=o.y; z-=o.z; w-=o.w; return *this; }
        quaternion&     operator *=( quaternion const& o ) { x*=o.x; y*=o.y; z*=o.z; w*=o.w; return *this; }
        quaternion&     operator *=( float a ) { x*=a; y*=a; z*=a; w*=a; return *this; }
        quaternion&     operator /=( float a ) { float inva = 1.f/a; x*=inva; y*=inva; z*=inva; w*=inva; return *this; }

        quaternion      conjugate() const { return quaternion(-x, -y, -z, w); }
        quaternion      inverse()   const; 

        float  sqnorm() const { return x * x + y * y + z * z + w * w; }
        float  norm()   const { return std::sqrt(sqnorm()); }

        float x, y, z, w;
    };

    inline quaternion::quaternion (const matrix& m)
    {
        float tr = m.m00 + m.m11 + m.m22;

        if (tr > 0) 
        { 
            float S = sqrt(tr+1.f) * 2;
            w = 0.25f * S;
            x = (m.m21 - m.m12) / S;
            y = (m.m02 - m.m20) / S; 
            z = (m.m10 - m.m01) / S; 
        } 
        else if ((m.m00 > m.m11)&(m.m00 > m.m22)) 
        { 
            float S = sqrt(1.f + m.m00 - m.m11 - m.m22) * 2;
            w = (m.m21 - m.m12) / S;
            x = 0.25f * S;
            y = (m.m01 + m.m10) / S; 
            z = (m.m02 + m.m20) / S; 
        } 
        else if (m.m11 > m.m22) 
        { 
            float S = sqrt(1.f + m.m11 - m.m00 - m.m22) * 2; // S=4*qy
            w = (m.m02 - m.m20) / S;
            x = (m.m01 + m.m10) / S; 
            y = 0.25f * S;
            z = (m.m12 + m.m21) / S; 
        } 
        else 
        { 
            float S = sqrt(1.f + m.m22 - m.m00 - m.m11) * 2; // S=4*qz
            w = (m.m10 - m.m01) / S;
            x = (m.m02 + m.m20) / S;
            y = (m.m12 + m.m21) / S;
            z = 0.25f * S;
        }
    }
    
    inline quaternion      quaternion::inverse()   const
    {
        if (sqnorm() == 0)
        {
            return quaternion();
        }
        else
        {
            quaternion q = conjugate();
            q /= sqnorm();
            return q;
        }
    }

    inline quaternion operator * (quaternion const& q1,  quaternion const& q2)
    {
        quaternion res;
        res.x = q1.y*q2.z - q1.z*q2.y + q2.w*q1.x + q1.w*q2.x;
        res.y = q1.z*q2.x - q1.x*q2.z + q2.w*q1.y + q1.w*q2.y;
        res.z = q1.x*q2.y - q2.x*q1.y + q2.w*q1.z + q1.w*q2.z;
        res.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
        return res;
    }

    inline quaternion operator / ( const quaternion& q,  float a )
    {
        quaternion res = q;
        return res /= a;
    }

    inline quaternion operator + ( const quaternion& q1,  const quaternion& q2 )
    {
        quaternion res = q1;
        return res += q2;
    }

    inline quaternion operator - ( const quaternion& q1,  const quaternion& q2 )
    {
        quaternion res = q1;
        return res -= q2;
    }

    inline quaternion operator * ( const quaternion& q,  float a )
    {
        quaternion res = q;
        return res *= a;
    }

    inline quaternion operator * ( float a, quaternion const& q )
    {
        quaternion res = q;
        return res *= a;
    }

    inline quaternion normalize( quaternion const& q )
    {
        float norm = q.norm();
        return q / norm;
    }
    
    inline void quaternion::to_matrix( matrix& m ) const
    {
        float s = 2.f/norm();
        m.m00 = 1 - s*(y*y + z*z); m.m01 = s * (x*y - w*z);	    m.m02 = s * (x*z + w*y);     m.m03 = 0;
        m.m10 = s * (x*y + w*z);   m.m11 = 1 - s * (x*x + z*z); m.m12 = s * (y*z - w*x);     m.m13 = 0;
        m.m20 = s * (x*z - w*y);   m.m21 = s * (y*z + w*x);	    m.m22 = 1 - s * (x*x + y*y); m.m23 = 0;
        m.m30 =                    m.m31 =                      m.m32;                       m.m33 = 1;
    }
}

#endif // QUATERNION_H