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

/*************************************************************************
DEFINES
**************************************************************************/
#define PI 3.14159265358979323846f
#define KERNEL __kernel
#define GLOBAL __global
#define INLINE __attribute__((always_inline))
#define HIT_MARKER 1
#define MISS_MARKER -1
#define INVALID_IDX -1

/*************************************************************************
EXTENSIONS
**************************************************************************/
#ifdef AMD_MEDIA_OPS
#pragma OPENCL EXTENSION cl_amd_media_ops2 : enable
#endif

/*************************************************************************
TYPES
**************************************************************************/

// Axis aligned bounding box
typedef struct
{
    float4 pmin;
    float4 pmax;
} bbox;

// Ray definition
typedef struct
{
    float4 o;        // xyz - origin, w - max range
    float4 d;        // xyz - direction, w - time
    int2 extra;      // x = ray mask, y = activity flag
    int2 padding;    // @todo: not just padding since actually used for logic, right?
	int2 surface0;   // x = shape index, y = prim index
	int2 surface1;   // (currently just padding)
} ray;

// Intersection definition 
typedef struct
{
    int shape_id;
    int prim_id;
    int2 padding;
    float4 uvwt;
} Intersection;


/*************************************************************************
HELPER FUNCTIONS
**************************************************************************/
INLINE
int ray_get_mask(ray const* r)
{
    return r->extra.x;
}

INLINE
int ray_is_active(ray const* r)
{
    return r->extra.y;
}

INLINE
float ray_get_maxt(ray const* r)
{
    return r->o.w;
}

INLINE
float ray_get_time(ray const* r)
{
    return r->d.w;
}

/*************************************************************************
FUNCTIONS
**************************************************************************/
#ifndef APPLE
INLINE
float4 make_float4(float x, float y, float z, float w)
{
    float4 res;
    res.x = x;
    res.y = y;
    res.z = z;
    res.w = w;
    return res;
}
INLINE
float3 make_float3(float x, float y, float z)
{
    float3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}
INLINE
float2 make_float2(float x, float y)
{
    float2 res;
    res.x = x;
    res.y = y;
    return res;
}
INLINE
int2 make_int2(int x, int y)
{
    int2 res;
    res.x = x;
    res.y = y;
    return res;
}
INLINE
int3 make_int3(int x, int y, int z)
{
    int3 res;
    res.x = x;
    res.y = y;
    res.z = z;
    return res;
}
#endif

INLINE float min3(float a, float b, float c)
{
#ifdef AMD_MEDIA_OPS
    return amd_min3(a, b, c);
#else
    return min(min(a,b), c);
#endif
}

INLINE float max3(float a, float b, float c)
{
#ifdef AMD_MEDIA_OPS
    return amd_max3(a, b, c);
#else
    return max(max(a,b), c);
#endif
}


// Intersect ray against a triangle and return intersection interval value if it is in
// (0, t_max], return t_max otherwise.
INLINE
float fast_intersect_triangle(ray r, float3 v1, float3 v2, float3 v3, float t_max)
{
    float3 const e1 = v2 - v1;
    float3 const e2 = v3 - v1;
    float3 const s1 = cross(r.d.xyz, e2);

	#ifdef USE_SAFE_MATH
	float const invd = 1.0/(dot(s1, e1));
	#else
    float const invd = native_recip(dot(s1, e1));
	#endif

    float3 const d = r.o.xyz - v1;
    float const b1 = dot(d, s1) * invd;
    float3 const s2 = cross(d, e1);
    float const b2 = dot(r.d.xyz, s2) * invd;
    float const temp = dot(e2, s2) * invd;

    if (b1 < 0.f || b1 > 1.f || b2 < 0.f || b1 + b2 > 1.f || temp < 0.f || temp > t_max)
    {
        return t_max;
    }
    else
    {
        return temp;
    }
}


// Intersect ray against a 'capsule', defined as the convex hull of two spheres 
// at position v1.xyz, v2.xyz, with radii v1.w, v2.w
// Return intersection interval value if it is in (0, t_max], return t_max otherwise.
// On intersection, also returns u=[0,1] giving the location of the hit along the axis as (1-u)*v1 + u*v2
INLINE
float intersect_capsule(ray R, float4 v1, float4 v2, float t_max, float* u)
{
	// A rather crude test where we find the closest approach of the ray and capsule axis.
	// If this a) projects onto the axis between the endpoints, and b) is less distance than the interpolated radius at this point 
	// then we record a hit at the closest approach (i.e. the corresponding U value on the axis).

	float3 p1 = R.o.xyz; // line 1 = ray
	float3 d1 = R.d.xyz;
	float3 p2 = v1.xyz; // line 2 = cylinder axis
	float3 d2 = normalize(v2.xyz - v1.xyz);

	float3 r = p1 - p2;
	float a = dot(d1, d1);
	float b = dot(d1, d2);
	float c = dot(d1, r);
	float e = dot(d2, d2);
	float f = dot(d2, r);
	float d = a*e - b*b;
	const float epsilon = 1.0e-6;
	if (d<=epsilon) return t_max; // ray and capsule almost parallel
		
	float s = (b*f - c*e)/d;
	if (s<0.0) return t_max; // intersection behind ray

	float t = (a*f - b*c)/d;
	float capsuleLength = length(v2.xyz - v1.xyz);
	if (t<-v1.w || t>capsuleLength+v2.w) return t_max; // intersection beyond capsule ends
	
	float3 c1 = p1 + s*d1;
	float3 c2 = p2 + t*d2;
	float U = t/capsuleLength;
	float radius = (1.0-U)*v1.w + U*v2.w;
	if ( length(c2-c1) > radius ) return t_max; // intersection too far from axis

	*u = U;
	return s;
}


INLINE
float3 safe_invdir(ray r)
{
#ifdef USE_SAFE_MATH
    float const dirx = r.d.x;
    float const diry = r.d.y;
    float const dirz = r.d.z;
    float const ooeps = exp2(-80.0f); // Avoid div by zero.
    float3 invdir;
    invdir.x = 1.0f / (fabs(dirx) > ooeps ? dirx : copysign(ooeps, dirx));
    invdir.y = 1.0f / (fabs(diry) > ooeps ? diry : copysign(ooeps, diry));
    invdir.z = 1.0f / (fabs(dirz) > ooeps ? dirz : copysign(ooeps, dirz));
    return invdir;
#else
    return native_recip(r.d.xyz);
#endif
}

// Intersect rays vs bbox and return intersection span. 
// Intersection criteria is ret.x <= ret.y
INLINE
float2 fast_intersect_bbox1(bbox box, float3 invdir, float3 oxinvdir, float t_max)
{
    float3 const f = mad(box.pmax.xyz, invdir, oxinvdir);
    float3 const n = mad(box.pmin.xyz, invdir, oxinvdir);
    float3 const tmax = max(f, n);
    float3 const tmin = min(f, n);
    float const t1 = min(min3(tmax.x, tmax.y, tmax.z), t_max);
    float const t0 = max(max3(tmin.x, tmin.y, tmin.z), 0.f);
    return make_float2(t0, t1);
}

// Given a point in triangle plane, calculate its barycentrics
INLINE
float2 triangle_calculate_barycentrics(float3 p, float3 v1, float3 v2, float3 v3)
{
    float3 const e1 = v2 - v1;
    float3 const e2 = v3 - v1;
    float3 const e = p - v1;
    float const d00 = dot(e1, e1);
    float const d01 = dot(e1, e2);
    float const d11 = dot(e2, e2);
    float const d20 = dot(e, e1);
    float const d21 = dot(e, e2);

	#ifdef USE_SAFE_MATH
		float const invdenom = 1.0 / (d00 * d11 - d01 * d01);
	#else
		float const invdenom = native_recip(d00 * d11 - d01 * d01);
	#endif

    float const b1 = (d11 * d20 - d01 * d21) * invdenom;
    float const b2 = (d00 * d21 - d01 * d20) * invdenom;
    return make_float2(b1, b2);
}
