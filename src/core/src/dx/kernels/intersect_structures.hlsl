#ifndef INTERSECT_STRUCTURES_HLSL
#define INTERSECT_STRUCTURES_HLSL

struct Ray
{
    float3 origin;
    float  min_t;
    float3 direction;
    float  max_t;
};

struct FullHitData
{
    float2 uv;
    uint   inst_id;
    uint   prim_id;
};

struct AnyHitData
{
    uint   inst_id;
};

#endif