#include "bvh.hlsl"

// clang-format off
struct Constants
{
    uint width;
    uint height;
    uint triangle_count;
    uint padding;
};

ConstantBuffer<Constants> g_constants : register(b0);
RWStructuredBuffer<float> g_vertices : register(u0);
RWStructuredBuffer<uint> g_indices : register(u1);
RWStructuredBuffer<BvhNode> g_bvh : register(u2);
RWStructuredBuffer<int> g_output : register(u3);

#define TILE_SIZE 8
#define LDS_STACK_SIZE 24

groupshared uint stack[LDS_STACK_SIZE * TILE_SIZE * TILE_SIZE];

[numthreads(TILE_SIZE, TILE_SIZE, 1)]
void TraceRays(
    in uint2 gidx: SV_DispatchThreadID,
    in uint2 lidx: SV_GroupThreadID,
    in uint2 bidx: SV_GroupID,
    in uint  flat_lidx: SV_GroupIndex)
{
    uint sbegin = flat_lidx * LDS_STACK_SIZE;
    uint sptr = sbegin;
    stack[sptr++] = INVALID_NODE;

    bool found = false;
    float2 w = (float2)gidx.xy / float2(g_constants.width, g_constants.height);


    float znear = 0.01f;
    /// sponza
    float3 ray_o = float3(0, 15, 0);
    float3 ray_d = normalize(float3(-znear, lerp(-0.01f, 0.01f, 1.f - w.y), lerp(-0.01f, 0.01f, w.x)));
    /// cornell
    // float3 ray_o = float3(0, 1, 1);
    // float3 ray_d = normalize(float3(lerp(-0.01f, 0.01f, w.x), lerp(-0.01f, 0.01f, 1.f - w.y), -znear));
    float ray_mint = 0.001;
    float ray_maxt = 1000000000.f;
    float3 ray_invd = safe_invdir(ray_d);
    float2 uv = float2(0.f, 0.f);
    float t = ray_maxt;

    if (gidx.x < g_constants.width && gidx.y < g_constants.height)
    {
        uint node_index = 0;
        while (node_index != INVALID_NODE)
        {
            bool is_tri = node_index >= g_constants.triangle_count;

            if (is_tri)
            {
                if (IntersectLeafNode(g_bvh[node_index], ray_d, ray_o, ray_mint, ray_maxt, uv))
                {
                    found = true;
                }
                node_index = INVALID_NODE;
            }
            else
            {
                uint2 result = IntersectInternalNode(g_bvh[node_index], ray_invd, ray_o, ray_mint, ray_maxt);
                stack[sptr] = result.y;
                if (result.y != INVALID_NODE)
                {
                    ++sptr;
                }
                node_index = result.x;
                if (result.x != INVALID_NODE)
                {
                    continue;
                }
            }

            if (node_index == INVALID_NODE && sptr > sbegin)
            {
                node_index = stack[--sptr];
            }
        }

        if (found)
        {
            g_output[gidx.x + g_constants.width * gidx.y] = 0xff000000 | (uint(255 * uv.x) << 8)| (uint(255 * uv.y) << 16);
        }
        else
        {
            g_output[gidx.x + g_constants.width * gidx.y] = 0xff101010;
        }
   }
}

// clang-format on
