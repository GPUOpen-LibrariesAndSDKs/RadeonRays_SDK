// clang-format off
#include "intersector_constants.hlsl"
#include "intersect_structures.hlsl"
#include "intersector_common.hlsl"
#include "transform.hlsl"


ConstantBuffer<Constants> g_constants: register(b0);
RWStructuredBuffer<BvhNode> g_bvh: register(u0);
RWStructuredBuffer<Transform> g_transforms: register(u1);
RWStructuredBuffer<uint> g_ray_count: register(u2);
RWStructuredBuffer<Ray> g_rays: register(u3);
#ifdef INSTANCE_ONLY
RWStructuredBuffer<AnyHitData> g_hits: register(u4);
#else
RWStructuredBuffer<FullHitData> g_hits: register(u4);
#endif
RWStructuredBuffer<uint> g_stack: register(u5);
StructuredBuffer<BvhNode> g_bottom_bvhs[]: register(t0);

#define GROUP_SIZE 128

#define LDS_STACK_SIZE 16
#define STACK_SIZE 64
#define TOP_LEVEL_SENTINEL 0xFFFFFFFE
groupshared uint lds_stack[LDS_STACK_SIZE * GROUP_SIZE];

void PushStack(in uint addr, inout uint lds_sptr, inout uint lds_sbegin, inout uint sptr, inout uint sbegin)
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
    lds_stack[lds_sptr++] = addr;
}

uint PopStack(inout uint lds_sptr, inout uint lds_sbegin, inout uint sptr, inout uint sbegin)
{
    uint addr = lds_stack[--lds_sptr];
    if (addr == INVALID_NODE && sptr > sbegin)
    {
        sptr -= LDS_STACK_SIZE;
        for (int i = 1; i < LDS_STACK_SIZE; ++i)
        {
            lds_stack[lds_sbegin + i] = g_stack[sptr + i];
        }
        lds_sptr = lds_sbegin + LDS_STACK_SIZE;
        addr = lds_stack[--lds_sptr];
    }
    return addr;
}
 
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
    // define stack start
    uint sbegin = STACK_SIZE * gidx;
    uint sptr = sbegin;
    uint lds_sbegin = lidx * LDS_STACK_SIZE;
    uint lds_sptr = lds_sbegin;
    lds_stack[lds_sptr++] = INVALID_NODE;
    // prepare ray info for trace
    float3 ray_o = g_rays[gidx].origin;
    float3 ray_d = g_rays[gidx].direction;
    float  ray_mint = g_rays[gidx].min_t;
    float  ray_maxt = g_rays[gidx].max_t;
    float3 ray_invd = safe_invdir(ray_d);
    float2 uv = float2(0.f, 0.f);

    // instance index for scene
    uint current_instance = INVALID_NODE;
    uint closest_instance = INVALID_NODE;
    uint closest_prim = INVALID_NODE;

    // entry point is 0
    uint node_index = 0;
    BvhNode node = g_bvh[node_index];

    while (node_index != INVALID_NODE)
    {
        if (current_instance == INVALID_NODE)
        {
            node = g_bvh[node_index];
        }
        else
        {
            node = g_bottom_bvhs[NonUniformResourceIndex(current_instance)][node_index];
        }
        bool is_leaf = (node.left_child == INVALID_NODE);

        if (!is_leaf)
        {               
            uint2 result = IntersectInternalNode(node, ray_invd, ray_o, ray_mint, ray_maxt);
            if (result.y != INVALID_NODE)
            {
                PushStack(result.y, lds_sptr, lds_sbegin, sptr, sbegin);
            }

            if (result.x != INVALID_NODE)
            {
                node_index = result.x;
                continue;
            }
        }
        // top-level leaf: adjust ray respecively to transforms
        else if (current_instance == INVALID_NODE)
        {
            current_instance = node.right_child;
            // push sentinel
            PushStack(TOP_LEVEL_SENTINEL, lds_sptr, lds_sbegin, sptr, sbegin);
            node_index = 0;
            Transform transform = g_transforms[2 * current_instance];
            ray_o = TransformPoint(ray_o, transform);
            ray_d = TransformDirection(ray_d, transform);
            ray_invd = safe_invdir(ray_d);
            continue;
        }
        // bottom-level leaf
        else
        {
            if (IntersectLeafNode(node, ray_d, ray_o, ray_mint, ray_maxt, uv))
            {
#ifdef ANY_HIT
                g_hits[gidx].inst_id = current_instance;
    #ifdef FULL_HIT
                g_hits[gidx].uv = uv;
                g_hits[gidx].prim_id = node.right_child;
    #endif
                return;
#else
                closest_instance = current_instance;
                closest_prim = node.right_child;
#endif
            }
        }

        node_index = PopStack(lds_sptr, lds_sbegin, sptr, sbegin);
        // check if need to go back to the top-level
        if (node_index == TOP_LEVEL_SENTINEL)
        {
            node_index = PopStack(lds_sptr, lds_sbegin, sptr, sbegin);
            current_instance = INVALID_NODE;
            // restore ray
            ray_o = g_rays[gidx].origin;
            ray_d = g_rays[gidx].direction;
            ray_invd = safe_invdir(ray_d);
        }
    }
    // fill-in hit output
#ifndef ANY_HIT
    if (closest_instance != INVALID_NODE)
    {
    #ifdef FULL_HIT
        g_hits[gidx].uv = uv;
        g_hits[gidx].prim_id = closest_prim;
    #endif
        g_hits[gidx].inst_id = closest_instance;
    }
    else
#endif
    {
        g_hits[gidx].inst_id = INVALID_NODE;
    }

}
// clang-format on