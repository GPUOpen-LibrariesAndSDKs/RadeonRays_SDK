/* Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.

Bullet Continuous Collision Detection and Physics Library
Copyright (c) 2003-2011 Erwin Coumans http://bulletphysics.org

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution. */

#define USE_30BIT_MORTON_CODE
#include "bvh2il.hlsl"
#include "build_hlbvh_constants.hlsl"
#include "transform.hlsl"


ConstantBuffer<Constants> g_constants: register(b0);
#ifndef TOP_LEVEL
RWStructuredBuffer<float> g_vertices: register(u0);
RWStructuredBuffer<uint> g_indices: register(u1);
#else
RWStructuredBuffer<InstanceDescription> g_instances: register(u0);
RWStructuredBuffer<uint> g_indices: register(u1);
#endif
RWByteAddressBuffer g_aabb: register(u2);
RWStructuredBuffer<uint> g_morton_codes: register(u3);
RWStructuredBuffer<uint> g_primitive_refs: register(u4);
globallycoherent RWStructuredBuffer<BvhNode> g_bvh: register(u5);
#ifdef TOP_LEVEL
RWStructuredBuffer<Transform> g_transforms: register(u6);
StructuredBuffer<BvhNode> g_bottom_bvh[]: register(t0);
#endif
#ifdef UPDATE_KERNEL
RWStructuredBuffer<uint> g_update_counters: register(u6);
#endif

#include "triangle_mesh.hlsl"
#define PRIMITIVES_PER_THREAD 8

#define GROUP_SIZE 128
#define INVALID_IDX 0xffffffff

#define unit_side 1024.0f

//=====================================================================================================================
// 3-dilate a number
uint ExpandBits(in uint r)
{
    r = (r * 0x00010001u) & 0xFF0000FFu;
    r = (r * 0x00000101u) & 0x0F00F00Fu;
    r = (r * 0x00000011u) & 0xC30C30C3u;
    r = (r * 0x00000005u) & 0x49249249u;
    return r;
}

//=====================================================================================================================
// Calculate and pack Morton code for the point
uint CalculateMortonCode(in float3 p)
{
    float x = clamp(p.x * unit_side, 0.0f, unit_side - 1.0f);
    float y = clamp(p.y * unit_side, 0.0f, unit_side - 1.0f);
    float z = clamp(p.z * unit_side, 0.0f, unit_side - 1.0f);

    return ((ExpandBits(uint(x)) << 2) | (ExpandBits(uint(y)) << 1) | ExpandBits(uint(z)));
}

//=====================================================================================================================
// HLSL implementation of OpenCL clz. This function counts the number of leading 0's from MSB
int clz(int value) { return (31 - firstbithigh(value)); }

//=====================================================================================================================
int clz64(uint64_t value) { return (63 - firstbithigh(value)); }

//=====================================================================================================================
// Calculates longest common prefix length of bit representations
// if  representations are equal we consider sucessive indices
int delta(in int i1, in int i2, int num_prims)
{
    // Select left end
    int left = min(i1, i2);
    // Select right end
    int right = max(i1, i2);
    // This is to ensure the node breaks if the index is out of bounds
    if (left < 0 || right >= num_prims)
    {
        return -1;
    }

    // Fetch Morton codes for both ends
#ifdef USE_30BIT_MORTON_CODE
    int leftcode  = g_morton_codes[left];
    int rightcode = g_morton_codes[right];
#else
    uint64_t leftcode  = g_morton_codes[left];
    uint64_t rightcode = g_morton_codes[right];
#endif

    // Special handling of duplicated codes: use their indices as a fallback
#ifdef USE_30BIT_MORTON_CODE
    return leftcode != rightcode ? clz(leftcode ^ rightcode) : (32 + clz(left ^ right));
#else
    return leftcode != rightcode ? clz64(leftcode ^ rightcode) : (64 + clz64(left ^ right));
#endif
}

//=====================================================================================================================
// Find span occupied by internal node with index idx
int2 FindSpan(int idx, int num_prims)
{
    // Find the direction of the range
    int d = sign((float)(delta(idx, idx + 1, num_prims) - delta(idx, idx - 1, num_prims)));

    // Find minimum number of bits for the break on the other side
    int deltamin = delta(idx, idx - d, num_prims);

    // Search conservative far end
    int lmax = 2;
    while (delta(idx, idx + lmax * d, num_prims) > deltamin)
    {
        lmax *= 2;
    }

    // Search back to find exact bound
    // with binary search
    int l = 0;
    int t = lmax;
    do
    {
        t /= 2;
        if (delta(idx, idx + (l + t) * d, num_prims) > deltamin)
        {
            l = l + t;
        }
    } while (t > 1);

    // Pack span
    int2 span;
    span.x = min(idx, idx + l * d);
    span.y = max(idx, idx + l * d);
    return span;
}

//=====================================================================================================================
// Find split idx within the span
int FindSplit(int2 span, int num_prims)
{
    // Fetch codes for both ends
    int left  = span.x;
    int right = span.y;

    // Calculate the number of identical bits from higher end
    int numidentical = delta(left, right, num_prims);

    do
    {
        // Proposed split
        int newsplit = (right + left) / 2;

        // If it has more equal leading bits than left and right accept it
        if (delta(left, newsplit, num_prims) > numidentical)
        {
            left = newsplit;
        } else
        {
            right = newsplit;
        }
    } while (right > left + 1);

    return left;
}

//=====================================================================================================================
// Magic found here: https://github.com/kripken/bullet/blob/master/
//                   src/BulletMultiThreaded/GpuSoftBodySolvers/DX11/HLSL/ComputeBounds.hlsl
uint3 Float3ToUint3(in float3 v)
{
    // Reinterpret value as uint
    uint3 value_as_uint = uint3(asuint(v.x), asuint(v.y), asuint(v.z));

    // Invert sign bit of positives and whole of  to anegativesllow comparison as unsigned ints
    value_as_uint.x ^= (1 + ~(value_as_uint.x >> 31) | 0x80000000);
    value_as_uint.y ^= (1 + ~(value_as_uint.y >> 31) | 0x80000000);
    value_as_uint.z ^= (1 + ~(value_as_uint.z >> 31) | 0x80000000);

    return value_as_uint;
}

//=====================================================================================================================
// Magic found here: https://github.com/kripken/bullet/blob/master/src/BulletMultiThreaded/GpuSoftBodySolvers/DX11/btSoftBodySolver_DX11.cpp
float3 Uint3ToFloat3(in uint3 v)
{
    v.x ^= (((v.x >> 31) - 1) | 0x80000000);
    v.y ^= (((v.y >> 31) - 1) | 0x80000000);
    v.z ^= (((v.z >> 31) - 1) | 0x80000000);

    return asfloat(v);
}

//=====================================================================================================================
void UpdateGlobalAabb(in Aabb aabb, inout RWByteAddressBuffer global_aabb)
{
    // Calculate the combined AABB for the entire wave.
    const float3 wave_bounds_min = WaveActiveMin(aabb.pmin);
    const float3 wave_bound_max = WaveActiveMax(aabb.pmax);
    GroupMemoryBarrierWithGroupSync();

    // Calculate the AABB for the entire scene using memory atomics.
    // Scalarize the atomic min/max writes by only using the first lane.
    if (WaveIsFirstLane())
    {
        // Convert the wave bounds to uints so we can atomically min/max them against the scene bounds in memory.
        const uint3 wave_min_uint = Float3ToUint3(wave_bounds_min);
        const uint3 wave_max_uint = Float3ToUint3(wave_bound_max);

        uint temp;
        global_aabb.InterlockedMin(0, wave_min_uint.x, temp);
        global_aabb.InterlockedMin(4, wave_min_uint.y, temp);
        global_aabb.InterlockedMin(8, wave_min_uint.z, temp);

        global_aabb.InterlockedMax(12, wave_max_uint.x, temp);
        global_aabb.InterlockedMax(16, wave_max_uint.y, temp);
        global_aabb.InterlockedMax(20, wave_max_uint.z, temp);
    }
}

#ifdef TOP_LEVEL
Aabb GetInstanceAabb(in InstanceDescription instance_desc)
{
    BvhNode node = g_bottom_bvh[NonUniformResourceIndex(instance_desc.index)][0];
    Aabb local_aabb = GetNodeAabb(node, true);
    return TransformAabb(local_aabb, instance_desc.transform);
}
#endif

[numthreads(GROUP_SIZE, 1, 1)]
void Init(in uint gidx: SV_DispatchThreadID,
          in uint lidx: SV_GroupThreadID,
          in uint bidx: SV_GroupID)
{
    if (gidx == 0)
    {
        Aabb aabb = CreateEmptyAabb();
        g_aabb.Store3(0, Float3ToUint3(aabb.pmin));
        g_aabb.Store3(12, Float3ToUint3(aabb.pmax));
    }
}

[numthreads(GROUP_SIZE, 1, 1)]
void CalculateAabb(in uint gidx: SV_DispatchThreadID,
                   in uint lidx: SV_GroupThreadID,
                   in uint bidx: SV_GroupID)

{
    Aabb local_aabb = CreateEmptyAabb();
    // Each thread handles PRIMITIVES_PER_THREAD triangles.
    for (int i = 0; i < PRIMITIVES_PER_THREAD; ++i)
    {
        //  Calculate linear triangle index.
        uint prim_index = gidx * PRIMITIVES_PER_THREAD + i;

        // Check out of bounds for this triangle.
        if (prim_index < g_constants.triangle_count)
        {
#ifndef TOP_LEVEL
                // Fetch triangle indices & vertices.
                uint3 indices = GetFaceIndices(prim_index);
                Triangle tri = FetchTriangle(indices);

                // Update local AABB for the thread.
                GrowAabb(tri.v0, local_aabb);
                GrowAabb(tri.v1, local_aabb);
                GrowAabb(tri.v2, local_aabb);
#else
                InstanceDescription instance_desc = g_instances[prim_index];
                Aabb instance_aabb = GetInstanceAabb(instance_desc);
                GrowAabb(instance_aabb.pmin, local_aabb);
                GrowAabb(instance_aabb.pmax, local_aabb);
#endif
        }
    }

    // Update global AABB for 
    UpdateGlobalAabb(local_aabb, g_aabb);
}

[numthreads(GROUP_SIZE, 1, 1)]
void CalculateMortonCodes(in uint gidx: SV_DispatchThreadID,
                          in uint lidx: SV_GroupThreadID,
                          in uint bidx: SV_GroupID)
{
    Aabb mesh_aabb;
    mesh_aabb.pmin = Uint3ToFloat3(g_aabb.Load3(0));
    mesh_aabb.pmax = Uint3ToFloat3(g_aabb.Load3(12));

    // Each thread handles PRIMITIVES_PER_THREAD triangles.
    for (int i = 0; i < PRIMITIVES_PER_THREAD; ++i)
    {
        //  Calculate linear triangle index.
        uint prim_index = gidx * PRIMITIVES_PER_THREAD + i;

        // Check out of bounds for this triangle.
        if (prim_index < g_constants.triangle_count)
        {
            // Calculate primitive centroid and map it to [0, 1].
#ifndef TOP_LEVEL
            uint3 indices = GetFaceIndices(prim_index);
            Triangle tri = FetchTriangle(indices);
            Aabb triangle_aabb = CreateEmptyAabb();
            GrowAabb(tri.v0, triangle_aabb);
            GrowAabb(tri.v1, triangle_aabb);
            GrowAabb(tri.v2, triangle_aabb);
            float3 center = 0.5f * (triangle_aabb.pmin + triangle_aabb.pmax);
#else
            InstanceDescription instance_desc = g_instances[prim_index];
            Aabb instance_aabb = GetInstanceAabb(instance_desc);
            float3 center = 0.5f * (instance_aabb.pmin + instance_aabb.pmax);
#endif
            center -= mesh_aabb.pmin;
            center /= (mesh_aabb.pmax - mesh_aabb.pmin);

            // Calculate Morton code and save triangle index for further sorting.
            g_morton_codes[prim_index] = CalculateMortonCode(center);
            g_primitive_refs[prim_index] = prim_index;
        }
    }
}

[numthreads(GROUP_SIZE, 1, 1)]
void EmitTopology(in uint gidx: SV_DispatchThreadID,
                  in uint lidx: SV_GroupThreadID,
                  in uint bidx: SV_GroupID)
{
    const uint N = g_constants.triangle_count;

    // Each thread handles PRIMITIVES_PER_THREAD triangles.
    for (int i = 0; i < PRIMITIVES_PER_THREAD; ++i)
    {
        //  Calculate linear primitive index.
        uint index = gidx * PRIMITIVES_PER_THREAD + i;

        // Check out of bounds for this triangle.
        if (index < N)
        {
            uint tri_index = g_primitive_refs[index];

            g_bvh[LEAF_NODE_INDEX(index, N)].child0 = INVALID_IDX;
            g_bvh[LEAF_NODE_INDEX(index, N)].child1 = tri_index;
        }
        // Internal nodes
        if (index < N - 1)
        {
            // Find span of the current tree node.
            int2 span = FindSpan(index, g_constants.triangle_count);

            // Find split within the tree node.
            int split = FindSplit(span, g_constants.triangle_count);

            // Set internal tree node data.
            uint child0 =
                (split == span.x) ? LEAF_NODE_INDEX(split , N) : split;
            uint child1 =
                (split + 1 == span.y) ? LEAF_NODE_INDEX(split + 1, N) : (split + 1);

            // Store internal node 
            g_bvh[INTERNAL_NODE_INDEX(index, N)].child0 = child0;
            g_bvh[INTERNAL_NODE_INDEX(index, N)].child1 = child1;
            g_bvh[INTERNAL_NODE_INDEX(index, N)].update = 0;
            g_bvh[child0].parent = index;
            g_bvh[child1].parent = index;
        }

        if (index == 0)
        {
            g_bvh[0].parent = INVALID_IDX;
        }
    }
}

[numthreads(GROUP_SIZE, 1, 1)]
void Refit(in uint gidx: SV_DispatchThreadID,
           in uint lidx: SV_GroupThreadID,
           in uint bidx: SV_GroupID)
{
    const uint N = g_constants.triangle_count;

    // Each thread handles PRIMITIVES_PER_THREAD triangles.
    for (int i = 0; i < PRIMITIVES_PER_THREAD; ++i)
    {
        //  Calculate linear primitive index.
        uint prim_index = gidx * PRIMITIVES_PER_THREAD + i;
        if (prim_index >= N)
        {
            return;
        }

        // Leaf nodes.
    #ifdef TOP_LEVEL
        uint instance_index = g_bvh[LEAF_NODE_INDEX(prim_index, N)].child1;
        g_transforms[2 * instance_index] = Inverse(g_instances[instance_index].transform);
        g_transforms[2 * instance_index + 1] = g_instances[instance_index].transform;
        Aabb instance_aabb = GetInstanceAabb(g_instances[instance_index]);
        g_bvh[LEAF_NODE_INDEX(prim_index, N)].aabb0_min_or_v0 = instance_aabb.pmin;
        g_bvh[LEAF_NODE_INDEX(prim_index, N)].aabb0_max_or_v1 = instance_aabb.pmax;
        g_bvh[LEAF_NODE_INDEX(prim_index, N)].aabb1_min_or_v2 = instance_aabb.pmin;
        g_bvh[LEAF_NODE_INDEX(prim_index, N)].aabb1_max_or_v3 = instance_aabb.pmax;
        
    #else
        // Fetch triangle indices & vertices.
        uint tri_index = g_bvh[LEAF_NODE_INDEX(prim_index, N)].child1;
        uint3 indices = GetFaceIndices(tri_index);
        Triangle tri = FetchTriangle(indices);

        // Store triangle BVH node.
        g_bvh[LEAF_NODE_INDEX(prim_index, N)].aabb0_min_or_v0 = tri.v0;
        g_bvh[LEAF_NODE_INDEX(prim_index, N)].aabb0_max_or_v1 = tri.v1;
        g_bvh[LEAF_NODE_INDEX(prim_index, N)].aabb1_min_or_v2 = tri.v2;
    #endif

        uint index = g_bvh[LEAF_NODE_INDEX(prim_index, N)].parent;

        [allow_uav_condition] 
        while (index != INVALID_IDX)
        {
            uint old_value = 0;
    #ifndef UPDATE_KERNEL        
            InterlockedExchange(g_bvh[index].update, 1, old_value);
    #else
            InterlockedExchange(g_update_counters[index], 1, old_value);
    #endif
            if (old_value == 1)
            {
                uint child0 = g_bvh[index].child0;
                uint child1 = g_bvh[index].child1;

                Aabb aabb0 = GetNodeAabb(g_bvh[child0], IS_INTERNAL_NODE(child0, N));
                Aabb aabb1 = GetNodeAabb(g_bvh[child1], IS_INTERNAL_NODE(child1, N));

                g_bvh[index].aabb0_min_or_v0 = aabb0.pmin;
                g_bvh[index].aabb0_max_or_v1 = aabb0.pmax;
                g_bvh[index].aabb1_min_or_v2 = aabb1.pmin;
                g_bvh[index].aabb1_max_or_v3 = aabb1.pmax;
            }
            else
            {
                // This is first thread, bail out.
                break;
            }

            index = g_bvh[index].parent;
        }
    }
}

#ifdef UPDATE_KERNEL

[numthreads(GROUP_SIZE, 1, 1)]
void InitUpdateFlags(in uint gidx: SV_DispatchThreadID)
{
    const uint triangle_count = g_constants.triangle_count;

    // Init internal nodes (conservative count)
    if (gidx < triangle_count - 1)
    {
        g_update_counters[gidx] = 0;
    }

    // Init leaf nodes.
    if (gidx < triangle_count)
    {
        g_update_counters[triangle_count - 1 + gidx] = 0;
    }
}
#endif
// clang-format on