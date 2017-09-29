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
#include <cstring>

#include "float3.h"

namespace RadeonRays
{
    class matrix
    {
    public:
        matrix(float mm00 = 1.f, float mm01 = 0.f, float mm02 = 0.f, float mm03 = 0.f,
            float mm10 = 0.f, float mm11 = 1.f, float mm12 = 0.f, float mm13 = 0.f,
            float mm20 = 0.f, float mm21 = 0.f, float mm22 = 1.f, float mm23 = 0.f,
            float mm30 = 0.f, float mm31 = 0.f, float mm32 = 0.f, float mm33 = 1.f)
            : m00(mm00), m01(mm01), m02(mm02), m03(mm03)
            , m10(mm10), m11(mm11), m12(mm12), m13(mm13)
            , m20(mm20), m21(mm21), m22(mm22), m23(mm23)
            , m30(mm30), m31(mm31), m32(mm32), m33(mm33)
        {
        }

        matrix(matrix const& o)
            : m00(o.m00), m01(o.m01), m02(o.m02), m03(o.m03)
            , m10(o.m10), m11(o.m11), m12(o.m12), m13(o.m13)
            , m20(o.m20), m21(o.m21), m22(o.m22), m23(o.m23)
            , m30(o.m30), m31(o.m31), m32(o.m32), m33(o.m33)
        {
        }

        matrix& operator = (matrix const& o)
        {
            for (int i = 0; i < 4; ++i)
                for (int j = 0; j < 4; ++j)
                    m[i][j] = o.m[i][j];
            return *this;
        }

        matrix operator-() const;
        matrix transpose() const;
        float  trace() const { return m00 + m11 + m22 + m33; }

        matrix& operator += (matrix const& o);
        matrix& operator -= (matrix const& o);
        matrix& operator *= (matrix const& o);
        matrix& operator *= (float c);

        union
        {
            float m[4][4];
            struct 
            {
                float m00, m01, m02, m03;
                float m10, m11, m12, m13;
                float m20, m21, m22, m23;
                float m30, m31, m32, m33;
            };
        };
    };


    inline matrix matrix::operator -() const
    {
        matrix res = *this;
        for(int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                res.m[i][j] = -m[i][j]; 
        return res;
    }

    inline matrix matrix::transpose() const
    {
        matrix res;
        for (int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                res.m[j][i] = m[i][j];
        return res;
    }

    inline matrix& matrix::operator += (matrix const& o)
    {
        for(int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                m[i][j] += o.m[i][j]; 
        return *this;
    }

    inline matrix& matrix::operator -= (matrix const& o)
    {
        for(int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                m[i][j] -= o.m[i][j]; 
        return *this;
    }

    inline matrix& matrix::operator *= (matrix const& o)
    {
        matrix temp;
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                temp.m[i][j] = 0.f;
                for (int k=0;k<4;++k)
                    temp.m[i][j] += m[i][k] * o.m[k][j];
            }
        }
        *this = temp;
        return *this;
    }

    inline matrix& matrix::operator *= (float c)
    {
        for(int i=0;i<4;++i)
            for (int j=0;j<4;++j)
                m[i][j] *= c; 
        return *this;
    }

    inline matrix operator+(matrix const& m1, matrix const& m2)
    {
        matrix res = m1;
        return res+=m2;
    }

    inline matrix operator-(matrix const& m1, matrix const& m2)
    {
        matrix res = m1;
        return res-=m2;
    }

    inline matrix operator*(matrix const& m1, matrix const& m2)
    {
        matrix res;
        for (int i=0;i<4;++i)
        {
            for (int j=0;j<4;++j)
            {
                res.m[i][j] = 0.f;
                for (int k=0;k<4;++k)
                    res.m[i][j] += m1.m[i][k]*m2.m[k][j];
            }
        }
        return res;
    }

    inline matrix operator*(matrix const& m, float c)
    {
        matrix res = m;
        return res*=c;
    }

    inline matrix operator*(float c, matrix const& m)
    {
        matrix res = m;
        return res*=c;
    }

    inline float4 operator * (matrix const& m, float4 const& v)
    {
        float4 res(0, 0, 0, 0);

        for (int i = 0; i < 4; ++i) {
            //res[i] = 0.f;
            for (int j = 0; j < 4; ++j)
                res[i] += m.m[i][j] * v[j];
        }

        return res;
    }

    inline matrix inverse(matrix const& m)
    {
        int indxc[4], indxr[4];
        int ipiv[4] = { 0, 0, 0, 0 };
        float minv[4][4];
        matrix temp = m;  
        memcpy(minv,  &temp.m[0][0], 4*4*sizeof(float));
        for (int i = 0; i < 4; i++) {
            int irow = -1, icol = -1;
            float big = 0.;
            // Choose pivot
            for (int j = 0; j < 4; j++) {
                if (ipiv[j] != 1) {
                    for (int k = 0; k < 4; k++) {
                        if (ipiv[k] == 0) {
                            if (fabsf(minv[j][k]) >= big) {
                                big = float(fabsf(minv[j][k]));
                                irow = j;
                                icol = k;
                            }
                        }
                        else if (ipiv[k] > 1)
                            return matrix();
                    }
                }
            }
            ++ipiv[icol];
            // Swap rows _irow_ and _icol_ for pivot
            if (irow != icol) {
                for (int k = 0; k < 4; ++k)
                    std::swap(minv[irow][k], minv[icol][k]);
            }
            indxr[i] = irow;
            indxc[i] = icol;
            if (minv[icol][icol] == 0.)
                return matrix();

            // Set $m[icol][icol]$ to one by scaling row _icol_ appropriately
            float pivinv = 1.f / minv[icol][icol];
            minv[icol][icol] = 1.f;
            for (int j = 0; j < 4; j++)
                minv[icol][j] *= pivinv;

            // Subtract this row from others to zero out their columns
            for (int j = 0; j < 4; j++) {
                if (j != icol) {
                    float save = minv[j][icol];
                    minv[j][icol] = 0;
                    for (int k = 0; k < 4; k++)
                        minv[j][k] -= minv[icol][k]*save;
                }
            }
        }
        // Swap columns to reflect permutation
        for (int j = 3; j >= 0; j--) {
            if (indxr[j] != indxc[j]) {
                for (int k = 0; k < 4; k++)
                    std::swap(minv[k][indxr[j]], minv[k][indxc[j]]);
            }
        }

        matrix result;
        std::memcpy(&result.m[0][0], minv, 4*4*sizeof(float));
        return result;
    }
}