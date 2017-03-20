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
#ifndef RAY_CL
#define RAY_CL

#include "../App/CL/common.cl"

// Ray descriptor
typedef struct
{
    // xyz - origin, w - max range
    float4 o;
    // xyz - direction, w - time
    float4 d;
    // x - ray mask, y - activity flag
    int2 extra;
    // Padding
    float2 padding;
} ray;

// Set ray activity flag
INLINE void Ray_SetInactive(GLOBAL ray* r)
{
    r->extra.y = 0;
}

// Set extra data for ray
INLINE void Ray_SetExtra(GLOBAL ray* r, float2 extra)
{
    r->padding = extra;
}

// Get extra data for ray
INLINE float2 Ray_GetExtra(GLOBAL ray const* r)
{
    return r->padding;
}

// Initialize ray structure
INLINE void Ray_Init(GLOBAL ray* r, float3 o, float3 d, float maxt, float time, int mask)
{
    r->o.xyz = o;
    r->d.xyz = d;
    r->o.w = maxt;
    r->d.w = time;
    r->extra.x = mask;
    r->extra.y = 0xFFFFFFFF;
}

#endif
