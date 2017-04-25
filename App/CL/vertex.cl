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
#ifndef VERTEX_CL
#define VERTEX_CL

// Path vertex type
enum PathVertexType
{
    kCamera,
    kSurface,
    kVolume,
    kLight
};

// Path vertex descriptor
typedef struct _PathVertex
{
    float3 position;
    float3 shading_normal;
    float3 geometric_normal;
    float2 uv;
    float pdf_forward;
    float pdf_backward;
    float3 flow;
    float3 unused;
    int type;
    int material_index;
    int flags;
    int padding;
} PathVertex;

// Initialize path vertex
INLINE
void PathVertex_Init(
    PathVertex* v, 
    float3 p, 
    float3 n,
    float3 ng, 
    float2 uv, 
    float pdf_fwd,
    float pdf_bwd, 
    float3 flow, 
    int type,
    int matidx
)
{
    v->position = p;
    v->shading_normal = n;
    v->geometric_normal = ng;
    v->uv = uv;
    v->pdf_forward = pdf_fwd;
    v->pdf_backward = pdf_bwd;
    v->flow = flow;
    v->type = type;
    v->material_index = matidx;
    v->flags = 0;
    v->unused = 0.f;
}

#endif
