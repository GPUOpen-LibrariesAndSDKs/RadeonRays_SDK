//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

// clang-format off
#define INVALID_ID ~0u

struct Ray
{
    float3 origin;
    float  min_t;
    float3 direction;
    float  max_t;
};

typedef BuiltInTriangleIntersectionAttributes MyAttributes;

struct HitData
{
#ifdef ASM_QUERY_OUTPUT_FULL_HIT
    float2 uv;
    uint   prim_id;
#endif
    uint   inst_id;
};

RaytracingAccelerationStructure g_scene : register(t0, space0);
RWStructuredBuffer<Ray>         g_rays : register(u0);
RWStructuredBuffer<HitData>     g_hits : register(u1);

[shader("raygeneration")]
void FetchAndSpawn()
{
    uint ray_index = DispatchRaysIndex().x;

    RayDesc ray;
    ray.Origin    = g_rays[ray_index].origin;
    ray.Direction = g_rays[ray_index].direction;
    ray.TMin      = g_rays[ray_index].min_t;
    ray.TMax      = g_rays[ray_index].max_t;

    HitData hit;
    TraceRay(g_scene, 0, ~0, 0, 1, 0, ray, hit);

    g_hits[ray_index] = hit;
}

#ifndef ASM_QUERY_FIRST_HIT
[shader("closesthit")]
void Hit(inout HitData hit, in MyAttributes attr)
{
#ifdef ASM_QUERY_OUTPUT_FULL_HIT
    hit.uv      = attr.barycentrics.xy;
    hit.inst_id = InstanceIndex();
    hit.prim_id = PrimitiveIndex();
#else
    hit.inst_id = InstanceIndex();
#endif
}
#else
[shader("anyhit")]
void Hit(inout HitData hit, in MyAttributes attr)
{
#ifdef ASM_QUERY_OUTPUT_FULL_HIT
    hit.uv      = attr.barycentrics.xy;
    hit.inst_id = InstanceIndex();
    hit.prim_id = PrimitiveIndex();
#else
    hit.inst_id = InstanceIndex();
#endif
}
#endif

[shader("miss")]
void Miss(inout HitData hit)
{
    hit.inst_id = INVALID_ID;
}

// clang-format on