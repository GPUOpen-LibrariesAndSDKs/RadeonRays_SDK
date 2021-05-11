// clang-format off
#include "intersector_constants.hlsl"
#include "intersect_structures.hlsl"
#include "intersector_common.hlsl"

ConstantBuffer<Constants> g_constants: register(b0);
RWStructuredBuffer<BvhNode> g_bvh: register(u0);
RWStructuredBuffer<uint> g_ray_count: register(u1);
RWStructuredBuffer<Ray> g_rays: register(u2);
#ifdef INSTANCE_ONLY
RWStructuredBuffer<AnyHitData> g_hits: register(u3);
#else
RWStructuredBuffer<FullHitData> g_hits: register(u3);
#endif
RWStructuredBuffer<uint> g_stack: register(u4);

#define GROUP_SIZE 128
#define LDS_STACK_SIZE 16
#define STACK_SIZE 64
groupshared uint lds_stack[LDS_STACK_SIZE * GROUP_SIZE];

/**
 * @brief Trace rays againsts given acceleration structure
 * 
 **/

[numthreads(GROUP_SIZE, 1, 1)]
void TraceRays(
    in uint gidx: SV_DispatchThreadID,
    in uint lidx: SV_GroupThreadID)
{
#ifdef INDIRECT_TRACE
    const uint ray_count = g_ray_count[0];
#else
    const uint ray_count = g_constants.ray_count;
#endif

    if (gidx >= ray_count) 
    {
        return;
    }

    uint sbegin = STACK_SIZE * gidx;
    uint sptr = sbegin;
    uint lds_sbegin = lidx * LDS_STACK_SIZE;
    uint lds_sptr = lds_sbegin;
    lds_stack[lds_sptr++] = INVALID_NODE;
    // prepare ray info for trace
    float3 ray_o = g_rays[gidx].origin;
    float3 ray_d = g_rays[gidx].direction;
    float  ray_maxt = g_rays[gidx].max_t;
    float  ray_mint = g_rays[gidx].min_t;
    float3 ray_invd = safe_invdir(ray_d);
    float2 uv = float2(0.f, 0.f);

    // primitive index for geometry
    uint closest_prim = INVALID_NODE;
    // entry point is 0
    uint node_index = 0;
    while (node_index != INVALID_NODE)
    {
        bool is_tri = (g_bvh[node_index].left_child == INVALID_NODE);
        if (is_tri)
        {
            if (IntersectLeafNode(g_bvh[node_index], ray_d, ray_o, ray_mint, ray_maxt, uv))
            {
#ifdef ANY_HIT
    #ifdef INSTANCE_ONLY
                g_hits[gidx].inst_id = g_bvh[node_index].right_child;
    #else
                g_hits[gidx].uv = uv;
                g_hits[gidx].prim_id = g_bvh[node_index].right_child;
                g_hits[gidx].inst_id = 0;
    #endif
                return;
#else
                closest_prim = g_bvh[node_index].right_child;
            #endif
            }
        }
        else
        {
            uint2 result = IntersectInternalNode(g_bvh[node_index], ray_invd, ray_o, ray_mint, ray_maxt);

            if (result.y != INVALID_NODE)
            {
                if (lds_sptr - lds_sbegin >= LDS_STACK_SIZE)
                {
                    for (int i = 1; i < LDS_STACK_SIZE; ++i)
                    {
                        g_stack[sptr + i] = lds_stack[lds_sbegin + i];
                    }

                    sptr += LDS_STACK_SIZE;
                    lds_sptr = lds_sbegin + 1;
                }
                lds_stack[lds_sptr++] = result.y;
            }
            if (result.x != INVALID_NODE)
            {
                node_index = result.x;
                continue;
            }
        }
        
        node_index = lds_stack[--lds_sptr];
        if (node_index == INVALID_NODE)
        {
            if (lds_sptr == lds_sbegin && sptr > sbegin)
            {
                sptr -= LDS_STACK_SIZE;
                for (int i = 1; i < LDS_STACK_SIZE; ++i)
                {
                    lds_stack[lds_sbegin + i] = g_stack[sptr + i];
                }

                lds_sptr = lds_sbegin + LDS_STACK_SIZE;
            }
            if (lds_sptr > lds_sbegin)
            {
                node_index = lds_stack[--lds_sptr];
            }
        }
    }
    // fill-in hit output
#ifndef ANY_HIT
    if (closest_prim != INVALID_NODE)
    {
    #ifdef FULL_HIT
        g_hits[gidx].uv = uv;
        g_hits[gidx].prim_id = closest_prim;
        g_hits[gidx].inst_id = 0;
    #else
        g_hits[gidx].inst_id = closest_prim;
    #endif
    }
    else
#endif
    {
        g_hits[gidx].inst_id = INVALID_NODE;
    }
}
// clang-format on