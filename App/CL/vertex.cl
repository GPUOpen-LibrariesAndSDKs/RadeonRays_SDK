#pragma once

enum PathVertexType
{
    kCamera,
    kSurface,
    kVolume,
    kLight
};

typedef struct _PathVertex
{
    float3 p;
    float3 n;
    float3 ng;
    float2 uv;
    float pdf_fwd;
    float pdf_bwd;
    float3 flow;
    float3 val;
    int type;
    int matidx;
    int flags;
    int padding;
} PathVertex;

inline
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
    v->p = p;
    v->n = n;
    v->ng = ng;
    v->uv = uv;
    v->pdf_fwd = pdf_fwd;
    v->pdf_bwd = pdf_bwd;
    v->flow = flow;
    v->type = type;
    v->matidx = matidx;
    v->flags = 0;
    v->val = 0.f;
}
