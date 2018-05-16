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

#include "float3.h"
#include "float2.h"
#include "quaternion.h"
#include "matrix.h"
#include "ray.h"
#include "bbox.h"

#include <cmath>
#include <ctime>
#include <cstdlib>
#include <string>
#include <vector>

#define PI 3.14159265358979323846f
#define OFFSETOF(struc,member) (&(((struc*)0)->member))

namespace RadeonRays
{
    /// Initialize RNG
    inline void rand_init() { std::srand((unsigned)std::time(nullptr)); }

    /// Generate random float value within [0,1] range
    inline float rand_float() { return (float)std::rand()/RAND_MAX; }

    /// Genarate random uint value
    inline unsigned    rand_uint() { return (unsigned)std::rand(); }

    /// Convert cartesian coordinates to spherical
    inline void    cartesian_to_spherical ( float3 const& cart, float& r, float& phi, float& theta )
    {
        float temp = std::atan2(cart.x, cart.z);
        r = std::sqrt(cart.x*cart.x + cart.y*cart.y + cart.z*cart.z);
        phi = (float)((temp >= 0)?temp:(temp + 2*PI));
        theta = std::acos(cart.y/r);
    }

    /// Convert cartesian coordinates to spherical
    inline void    cartesian_to_spherical ( float3 const& cart, float3& sph ) 
    {
        cartesian_to_spherical(cart, sph.x, sph.y, sph.z);
    }

    /// Clamp float value to [a, b) range
    inline float clamp(float x, float a, float b)
    {
        return x < a ? a : (x > b ? b : x);
    }

    inline unsigned clamp(unsigned x, unsigned a, unsigned b)
    {
        return x < a ? a : (x > b ? b : x);
    }
    
    /// Clamp each component of the vector to [a, b) range
    inline float3 clamp(float3 const& v, float3 const& v1, float3 const& v2)
    {
        float3 res;
        res.x = clamp(v.x, v1.x, v2.x);
        res.y = clamp(v.y, v1.y, v2.y);
        res.z = clamp(v.z, v1.z, v2.z);
        return res;
    }

    /// Clamp each component of the vector to [a, b) range
    inline float2 clamp(float2 const& v, float2 const& v1, float2 const& v2)
    {
        float2 res;
        res.x = clamp(v.x, v1.x, v2.x);
        res.y = clamp(v.y, v1.y, v2.y);
        return res;
    }

    /// Convert spherical coordinates to cartesian 
    inline void    spherical_to_cartesian ( float r, float phi, float theta, float3& cart )
    {
        cart.y = r * std::cos(theta);
        cart.z = r * std::sin(theta) * std::cos(phi);
        cart.x = r * std::sin(theta) * std::sin(phi);
    }

    /// Convert spherical coordinates to cartesian 
    inline void    spherical_to_cartesian ( float3 const& sph, float3& cart )
    {
        spherical_to_cartesian(sph.x, sph.y, sph.z, cart); 
    }

    /// Transform a point using a matrix
    inline float3 transform_point(float3 const& p, matrix const& m)
    {
        float3 res = m * p;
        res.x += m.m03;
        res.y += m.m13;
        res.z += m.m23;
        return res;
    }

    /// Transform a vector using a matrix
    inline float3 transform_vector(float3 const& v, matrix const& m)
    {
        return m * v;
    }

    /// Transform a normal using a matrix.
    /// Use this function carefully as the normal can be 
    /// transformed much faster using transform matrix in many cases
    /// NOTE: You need to pass inverted matrix for the transform
    inline float3 transform_normal(float3 const& n, matrix const& minv)
    {
        return minv.transpose() * n;
    }

    /// Transform a ray using a matrix
    inline ray transform_ray(ray const& r, matrix const& m)
    {
        return ray(transform_point(r.o, m), transform_vector(r.d, m), r.o.w, r.d.w);
    }
    
    /// Transform bounding box
    inline bbox transform_bbox(bbox const& b, matrix const& m)
    {
        // Get extents
        float3 extents = b.extents();
        
        // Transform the box to correct instance space
        bbox newbox(transform_point(b.pmin, m));
        newbox.grow(transform_point(b.pmin + float3(extents.x, 0, 0), m));
        newbox.grow(transform_point(b.pmin + float3(extents.x, extents.y, 0), m));
        newbox.grow(transform_point(b.pmin + float3(0, extents.y, 0), m));
        newbox.grow(transform_point(b.pmin + float3(extents.x, 0, extents.z), m));
        newbox.grow(transform_point(b.pmin + float3(extents.x, extents.y, extents.z), m));
        newbox.grow(transform_point(b.pmin + float3(0, extents.y, extents.z), m));
        newbox.grow(transform_point(b.pmin + float3(0, 0, extents.z), m));
        
        return newbox;
    }

    /// Solve quadratic equation
    /// Returns false in case of no real roots exist
    /// true otherwise
    bool    solve_quadratic( float a, float b, float c, float& x1, float& x2 );

    /// Matrix transforms
    matrix translation(float3 const& v);
    matrix rotation_x(float ang);
    matrix rotation_y(float ang);
    matrix rotation_z(float ang);
    matrix rotation(float3 const& axis, float ang);
    matrix scale(float3 const& v);

    /// This perspective projection matrix effectively maps view frustum to [-1,1]x[-1,1]x[0,1] clip space, i.e. DirectX depth
    matrix perspective_proj_lh_dx(float l, float r, float b, float t, float n, float f);
    matrix perspective_proj_lh_gl(float l, float r, float b, float t, float n, float f);

    /// This perspective projection matrix effectively maps view frustum to [-1,1]x[-1,1]x[-1,1] clip space, i.e. OpenGL depth
    matrix perspective_proj_rh_gl(float l, float r, float b, float t, float n, float f);

    //matrix perspective_proj_fovy(float fovy, float aspect, float n, float f);
    matrix perspective_proj_fovy_lh_dx(float fovy, float aspect, float n, float f);
    matrix perspective_proj_fovy_lh_gl(float fovy, float aspect, float n, float f);

    matrix lookat_lh_dx( float3 const& pos, float3 const& at, float3 const& up);

    /// Quaternion transforms
    quaternion  rotation_quaternion(float3 const& axis, float angle);
    float3      rotate_vector( float3 const& v, quaternion const& q );
    quaternion  rotate_quaternion( quaternion const& v, quaternion const& q );

    inline quaternion matrix_to_quaternion(matrix const& m)
    {
        quaternion q;
        q.w = 0.5f*sqrt(m.trace());
        q.x = (m.m[2][1] - m.m[1][2])/(4*q.w);
        q.y = (m.m[0][2] - m.m[2][0])/(4*q.w);
        q.z = (m.m[1][0] - m.m[0][1])/(4*q.w);
        return q;
    }

    inline matrix quaternion_to_matrix(quaternion const& q)
    {
        matrix m;
        float s = 2/q.norm();
        m.m[0][0] = 1 - s*(q.y*q.y + q.z*q.z); m.m[0][1] = s * (q.x*q.y - q.w*q.z);      m.m[0][2] = s * (q.x*q.z + q.w*q.y);      m.m[0][3] = 0;
        m.m[1][0] = s * (q.x*q.y + q.w*q.z);   m.m[1][1] = 1 - s * (q.x*q.x + q.z*q.z);  m.m[1][2] = s * (q.y*q.z - q.w*q.x);      m.m[1][3] = 0;
        m.m[2][0] = s * (q.x*q.z - q.w*q.y);   m.m[2][1] = s * (q.y*q.z + q.w*q.x);      m.m[2][2] = 1 - s * (q.x*q.x + q.y*q.y);  m.m[2][3] = 0;
        m.m[3][0] = 0;                         m.m[3][1] = 0;                            m.m[3][2] = 0;                            m.m[3][3] = 1;
        return m;
    }

    // Calculate vector orthogonal to a given one
    inline float3 orthovector(float3 const& n)
    {
        float3 p;
        if (fabs(n.z) > 0.707106781186547524401f) {
            float k = sqrt(n.y*n.y + n.z*n.z);
            p.x = 0; p.y = -n.z/k; p.z = n.y/k;
        }
        else {
            float k = sqrt(n.x*n.x + n.y*n.y);
            p.x = -n.y/k; p.y = n.x/k; p.z = 0;
        }
        return p;
    }

    // Map [0..1]x[0..1] value to unit hemisphere with pow e cos weighted pdf
    inline float3 map_to_hemisphere(float3 const& n, float2 const& s, float e)
    {
        float3 u = orthovector(n);

        float3 v = cross(u, n);
        u = cross(n, v);

        float sinpsi = sinf(2*PI*s.x);
        float cospsi = cosf(2*PI*s.x);
        float costheta = powf(1.f - s.y, 1.f/(e + 1.f));
        float sintheta = sqrt(1.f - costheta * costheta);

        return normalize(u * sintheta * cospsi + v * sintheta * sinpsi + n * costheta);
    }

    // Map [0..1]x[0..1] value to triangle and return barycentric coords
    inline float3 map_to_triangle(float2 const& s)
    {
        return float3(1.f - sqrtf(s.x), sqrtf(s.x) * (1.f - s.y), sqrtf(s.x) * s.y);
    }

    // Checks if the float value is IEEE FP NaN
    inline bool is_nan(float val)
    {
        return val != val;
    }

    // Checks if one of arguments components is IEEE FP NaN
    inline bool has_nans(float3 const& val)
    {
        return is_nan(val.x) || is_nan(val.y) || is_nan(val.z); 
    }

    // Linearly interpolate two float3 values
    inline float3 lerp(float3 const& v1, float3 const& v2, float s)
    {
        return (1.f - s) * v1 + s * v2;
    }

    // Linearly interpolate two float3 values
    inline void lerp(float3 const& v1, float3 const& v2, float s, float3& res)
    {
        res.x = (1.f - s) * v1.x + s * v2.x;
        res.y = (1.f - s) * v1.y + s * v2.y;
        res.z = (1.f - s) * v1.z + s * v2.z;
    }

    // Linearly interpolate two float values
    inline float lerp(float v1, float v2, float s)
    {
        return (1.f - s) * v1 + s * v2;
    }

    inline bool    solve_quadratic( float a, float b, float c, float& x1, float& x2 )
    {
        float d = b*b - 4*a*c;
        if ( d < 0 )
            return false;
        else
        {
            float den = 1/(2*a);
            x1 = (-b - std::sqrt(d))*den;
            x2 = (-b + std::sqrt(d))*den;
            return true;
        }
    }

    inline matrix translation(float3 const& v)
    {
        return matrix (1, 0, 0, v.x, 
            0, 1, 0, v.y, 
            0, 0, 1, v.z, 
            0, 0, 0, 1);
    }

    inline matrix rotation_x(float ang)
    {
        return matrix(1, 0, 0, 0, 
            0, std::cos(ang), -std::sin(ang), 0,
            0, std::sin(ang), std::cos(ang), 0,
            0, 0, 0, 1);
    }

    inline matrix rotation_y(float ang)
    {
        return matrix(std::cos(ang), 0, std::sin(ang), 0,
            0, 1, 0, 0,
            -std::sin(ang), 0, std::cos(ang), 0,
            0, 0, 0, 1);
    }

    inline matrix rotation_z(float ang)
    {
        return matrix(std::cos(ang), -std::sin(ang), 0, 0,
            std::sin(ang), std::cos(ang), 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1);
    }

    inline matrix rotation(const float3& a, float ang)
    {
        float cos = std::cos(ang);
        float sin = std::sin(ang);
        return matrix(cos + a.x*a.x*(1-cos), a.x*a.y*(1-cos)-a.z*sin, a.x*a.z*(1-cos) + a.y*sin, 0,
                    a.y*a.x*(1-cos)+a.z*sin, cos+ a.y*a.y*(1-cos), a.y*a.z*(1-cos) - a.x*sin, 0,
                    a.z*a.x*(1-cos)-a.y*sin, a.z*a.y*(1-cos)+a.x*sin, cos + a.z*a.z*(1-cos), 0,
                    0, 0, 0, 1);
    }

    inline matrix scale(float3 const& v)
    {
        return matrix(v.x, 0, 0, 0, 0, v.y, 0, 0, 0, 0, v.z, 0, 0, 0, 0, 1.f);
    }

    inline matrix perspective_proj_lh_dx(float l, float r, float b, float t, float n, float f)
    {
        return matrix(2*n/(r-l), 0, 0, 0, 
            0, 2*n/(t-b), 0, (r + l)/(r - l),
            0, -(t + b)/(t - b), f/(f - n), -f*n/(f - n),
            0, 0, 1, 0);  
    }

    inline matrix perspective_proj_lh_gl(float l, float r, float b, float t, float n, float f)
    {
        return matrix(2*n/(r-l), 0, (r+l)/(r-l), 0,
                      0, 2*n/(t-b), (t+b) / (t-b), 0,
                      0, 0, (n+f)/(n-f), 2*f*n/(n - f),
                      0, 0, -1, 0).transpose();
    }

    inline matrix perspective_proj_fovy_lh_gl(float fovy, float aspect, float n, float f)
    {
        float hH =  tan(fovy/2) * n;
        float hW  = hH * aspect;
        return perspective_proj_lh_gl( -hW, hW, -hH, hH, n, f);
    }

    inline matrix perspective_proj_rh_gl(float l, float r, float b, float t, float n, float f)
    {
        return matrix(2*n/(r-l), 0, 0, 0,
            0, 2*n/(t-b), 0, (r + l)/(r - l),
            0, (t + b)/(t - b), -(f + n)/(f - n), -2*f*n/(f - n),
            0, 0, -1, 0);
    }

    inline matrix perspective_proj_fovy_lh_dx(float fovy, float aspect, float n, float f)
    {
        float hH =  tan(fovy/2) * n;
        float hW  = hH * aspect;
        return perspective_proj_lh_dx( -hW, hW, -hH, hH, n, f);
    }

    inline matrix perspective_proj_fovy_rh_gl(float fovy, float aspect, float n, float f)
    {
        float hH = tan(fovy) * n;
        float hW  = hH * aspect;
        return perspective_proj_rh_gl( -hW, hW, -hH, hH, n, f);
    }

    inline matrix lookat_lh_dx( float3 const& pos, float3 const& at, float3 const& up)
    {
        float3 v = normalize(at - pos);
        float3 r = cross(normalize(up), v);
        float3 u = cross(v,r);
        float3 ip = float3(-dot(r,pos), -dot(u,pos), -dot(v,pos));

        return matrix(r.x, r.y, r.z, ip.x,
            u.x, u.y, u.z, ip.y,
            v.x, v.y, v.z, ip.z,
            0, 0, 0, 1);
    }

    inline quaternion rotation_quaternion(float3 const& axe, float angle)
    {
        // create (sin(a/2)*axis, cos(a/2)) quaternion
        // which rotates the point a radians around "axis"
        quaternion res;
        float3 u = axe; u.normalize();
        float sina2 = std::sin(angle/2);
        float cosa2 = std::cos(angle/2);

        res.x = sina2 * u.x;
        res.y = sina2 * u.y;
        res.z = sina2 * u.z;
        res.w = cosa2;

        return res;
    }

    inline float3 rotate_vector(float3 const& v, quaternion const& q)
    {
        quaternion p = quaternion(v.x, v.y, v.z, 0);
        quaternion tp = q * p * q.inverse();
        return float3(tp.x, tp.y, tp.z);
    }

    inline quaternion    rotate_quaternion( quaternion const& v, quaternion const& q )
    {
        return q * v * q.inverse();
    }
}