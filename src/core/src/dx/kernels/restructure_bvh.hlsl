// clang-format off
#define USE_30BIT_MORTON_CODE
#include "bvh2il.hlsl"
#include "restructure_bvh_constants.hlsl"

ConstantBuffer<Constants> g_constants: register(b0);
globallycoherent RWStructuredBuffer<uint> g_treelet_count: register(u0);
globallycoherent RWStructuredBuffer<uint> g_treelet_roots: register(u1);
globallycoherent RWStructuredBuffer<uint> g_primitive_counts: register(u2);
globallycoherent RWStructuredBuffer<BvhNode> g_bvh: register(u3);

#define GROUP_SIZE 64
#define INVALID_IDX 0xffffffff
#define MAX_TREELET_SIZE 7
#define MAX_TREELET_INTERNAL_NODES (MAX_TREELET_SIZE - 1)
#define NUM_TREE_SPLIT_PERMUTATIONS (1 << MAX_TREELET_SIZE)
#define C_INT 1.2f
#define NUM_TREE_SPLIT_PERMUTATIONS_PER_THREAD (1 << (MAX_TREELET_SIZE - 6))

#define USE_BITWISE_OPT  1
#define USE_PRECOMPUTED_PERMUTATIONS 1

/**
 * @brief Initialize treelet counter.
 **/
[numthreads(GROUP_SIZE, 1, 1)]
void InitPrimitiveCounts(in uint gidx: SV_DispatchThreadID)
{
    // Internal nodes.
    if (gidx < g_constants.triangle_count - 1)
    {
        g_primitive_counts[gidx] = 0;
    }

    // Leafs.
    if (gidx < g_constants.triangle_count)
    {
        g_primitive_counts[g_constants.triangle_count - 1 + gidx] = 1;
    }

    if (gidx == 0)
    {
        g_treelet_count[0] = 0;
    }
}

/**
 * @brief Find treelet roots.
 **/
[numthreads(GROUP_SIZE, 1, 1)]
void FindTreeletRoots(in uint gidx: SV_DispatchThreadID)
{
    const uint N = g_constants.triangle_count;

    if (gidx >= N)
    {
        return;
    }

    uint prim_count = 1;
    uint index = g_bvh[LEAF_NODE_INDEX(gidx, N)].parent;

    [allow_uav_condition] 
    while (index != INVALID_IDX)
    {
        uint old_value = 0;
        InterlockedExchange(g_primitive_counts[index], prim_count, old_value);

        prim_count += old_value;

        if (old_value)
        {
            if (prim_count >= g_constants.min_prims_per_treelet)
            {
                // Write treelet.
                uint idx;
                InterlockedAdd(g_treelet_count[0], 1, idx);
                g_treelet_roots[idx] = index;
                break;
            }
        }
        else
        {
            // This is first thread, bail out.
            break;
        }

        index = g_bvh[index].parent;
    }
}

//=====================================================================================================================
float CalculateCost(in Aabb node_aabb, in float parent_aabb_surface_area)
{
    // TODO consider caching rcp(parent_aabb_surface_area)
    return C_INT * GetAabbSurfaceArea(node_aabb) / parent_aabb_surface_area;
}

float CalculateCost(in float surface_area, in float parent_aabb_surface_area)
{
    // TODO consider caching rcp(parent_aabb_surface_area)
    return C_INT * surface_area / parent_aabb_surface_area;
}
#define BIT(x) (1 << (x))

groupshared uint lds_nodes_to_reorder[MAX_TREELET_SIZE];
groupshared uint lds_internal_nodes[MAX_TREELET_INTERNAL_NODES];
groupshared float lds_area[NUM_TREE_SPLIT_PERMUTATIONS];
groupshared float lds_optimal_cost[NUM_TREE_SPLIT_PERMUTATIONS];
groupshared uint lds_optimal_partition[NUM_TREE_SPLIT_PERMUTATIONS];
groupshared uint lds_node_index;
groupshared bool lds_is_done;
groupshared float lds_partiton_cost[64];
groupshared uint lds_partition_mask[64];

struct PartitionEntry
{
    uint mask;
    uint node_index;
};

#if USE_PRECOMPUTED_PERMUTATIONS

// All results of s_full_treelet_size choose i
static const uint s_full_treelet_size_choose[MAX_TREELET_SIZE + 1] = { 1, 7, 21, 35, 35, 21, /*7*/56 /*special case*/, 64 /*special case*/ };

// Precalculated bit permutations with k bits set, for k = 2 to s_full_treelet_size - 1.
// Permutations with k = 0, 1, and s_full_treelet_size are calculated in GetBitPermutation.
static const uint s_bit_permutations[MAX_TREELET_SIZE - 2][35] = {
    { 0x03, 0x05, 0x06, 0x09, 0x0a, 0x0c, 0x11, 0x12, 0x14, 0x18, 0x21, 0x22, 0x24, 0x28, 0x30, 0x41, 0x42, 0x44, 0x48, 0x50, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x07, 0x0b, 0x0d, 0x0e, 0x13, 0x15, 0x16, 0x19, 0x1a, 0x1c, 0x23, 0x25, 0x26, 0x29, 0x2a, 0x2c, 0x31, 0x32, 0x34, 0x38, 0x43, 0x45, 0x46, 0x49, 0x4a, 0x4c, 0x51, 0x52, 0x54, 0x58, 0x61, 0x62, 0x64, 0x68, 0x70 },
    { 0x0f, 0x17, 0x1b, 0x1d, 0x1e, 0x27, 0x2b, 0x2d, 0x2e, 0x33, 0x35, 0x36, 0x39, 0x3a, 0x3c, 0x47, 0x4b, 0x4d, 0x4e, 0x53, 0x55, 0x56, 0x59, 0x5a, 0x5c, 0x63, 0x65, 0x66, 0x69, 0x6a, 0x6c, 0x71, 0x72, 0x74, 0x78 },
    { 0x1f, 0x2f, 0x37, 0x3b, 0x3d, 0x3e, 0x4f, 0x57, 0x5b, 0x5d, 0x5e, 0x67, 0x6b, 0x6d, 0x6e, 0x73, 0x75, 0x76, 0x79, 0x7a, 0x7c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 0x3f, 0x5f, 0x6f, 0x77, 0x7b, 0x7d, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

uint GetBitPermutation(uint num_bits_set, uint n)
{
    // GetBitPermutation is always called with num_bits_set == [2, s_full_treelet_size]
    if (num_bits_set == MAX_TREELET_SIZE)
    {
        return  BIT(MAX_TREELET_SIZE) - 1;
    }
    return s_bit_permutations[num_bits_set - 2][n];
}
//************************************************************
#endif

/**
 * @brief Restructure.
 * 
 * Each group works on a separate treelet here and then goes up the tree.
 **/
[numthreads(GROUP_SIZE, 1, 1)]
void Restructure(in uint gidx : SV_DispatchThreadID,
                 in uint lidx : SV_GroupThreadID,
                 in uint bidx : SV_GroupID)
{
    const uint N = g_constants.triangle_count;

    // Make sure we have a treelet to work on.
    if (bidx >= g_treelet_count[0])
    {
        return;
    }

    if (lidx == 0)
    {
        lds_node_index = g_treelet_roots[bidx];
        lds_is_done = false;
    }

    GroupMemoryBarrierWithGroupSync();

    [allow_uav_condition]
    while (true)
    {
        // Form a treelet
        if (lidx == 0)
        {
            lds_internal_nodes[0] = lds_node_index;

            lds_nodes_to_reorder[0] = g_bvh[lds_node_index].child0;
            lds_nodes_to_reorder[1] = g_bvh[lds_node_index].child1;

            float surface_areas[MAX_TREELET_SIZE];

            surface_areas[0] = GetAabbSurfaceArea(GetNodeAabb(g_bvh[lds_nodes_to_reorder[0]], IS_INTERNAL_NODE(lds_nodes_to_reorder[0], N)));
            surface_areas[1] = GetAabbSurfaceArea(GetNodeAabb(g_bvh[lds_nodes_to_reorder[1]], IS_INTERNAL_NODE(lds_nodes_to_reorder[1], N)));

            uint treelet_size = 2;
            while (treelet_size < MAX_TREELET_SIZE)
            {
                float largest_surface_area = 0.0;
                uint node_index_to_traverse = 0;
                uint index_of_node_index_to_traverse = 0;
                for (uint i = 0; i < treelet_size; i++)
                {
                    uint treelet_node_index = lds_nodes_to_reorder[i];
                    // Leaf nodes can't be split so skip these
                    if (IS_INTERNAL_NODE(treelet_node_index, N))
                    {
                        float surface_area = surface_areas[i];

                        if (largest_surface_area == 0.0 || surface_area > largest_surface_area)
                        {
                            largest_surface_area = surface_area;
                            node_index_to_traverse = treelet_node_index;
                            index_of_node_index_to_traverse = i;
                        }
                    }
                }

                // Replace the original node with its left child and add the right child to the end
                lds_internal_nodes[treelet_size - 1] = node_index_to_traverse;

                BvhNode node_to_traverse = g_bvh[node_index_to_traverse];

                lds_nodes_to_reorder[index_of_node_index_to_traverse] = node_to_traverse.child0;
                lds_nodes_to_reorder[treelet_size] = node_to_traverse.child1;

                surface_areas[index_of_node_index_to_traverse] = GetAabbSurfaceArea(GetNodeAabb(g_bvh[node_to_traverse.child0], IS_INTERNAL_NODE(node_to_traverse.child0, N)));
                surface_areas[treelet_size] = GetAabbSurfaceArea(GetNodeAabb(g_bvh[node_to_traverse.child1], IS_INTERNAL_NODE(node_to_traverse.child1, N)));

                treelet_size++;
            }
        }

        // Now that a treelet has been formed, try to reorder

        // Make sure thread0 is done before we procceed
        GroupMemoryBarrierWithGroupSync();

        for (uint b = 0; b < NUM_TREE_SPLIT_PERMUTATIONS_PER_THREAD; b++)
        {
            uint treelet_bitmask = NUM_TREE_SPLIT_PERMUTATIONS_PER_THREAD * lidx + b;

            Aabb aabb = CreateEmptyAabb();
            for (uint i = 0; i < MAX_TREELET_SIZE; i++)
            {
                if (BIT(i) & treelet_bitmask)
                {
                    GrowAabb(GetNodeAabb(g_bvh[lds_nodes_to_reorder[i]], IS_INTERNAL_NODE(lds_nodes_to_reorder[i], N)), aabb);
                }
            }

            lds_area[treelet_bitmask] = GetAabbSurfaceArea(aabb);
        }

        //**can be removed and just use lds_area as theres no leaf intersection in cost**
        float root_aabb_surface_area = 1;

        //calc cost per bit in bitmask (per leaf)
        //cost for ONE bit
        //**can be removed and just use lds_area as theres no leaf intersection in cost**

        if (lidx < MAX_TREELET_SIZE)
        {
            lds_optimal_cost[BIT(lidx)] = CalculateCost(lds_area[BIT(lidx)], root_aabb_surface_area);
        }

        //calc cost for >ONE bit
        //iterate through all bitmasks with "N bits" to calculate the costs
        //the costs depened on lower bits so start from bits 2
        for (uint bits = 2; bits <= MAX_TREELET_SIZE; bits++)
        {
            // Sync before move on to calculate cost for more bits
            GroupMemoryBarrierWithGroupSync();

            uint num_treelet_bitmasks = s_full_treelet_size_choose[bits];
            if (lidx < num_treelet_bitmasks)
            {
                if (bits == MAX_TREELET_SIZE - 1) //special case (optimization, saves 0.5 ms for Crytek Sponza)
                {
                    //7 different masks - 31 partitions per mask
                    //8 threads per mask - 4 partitions per thread
                    //56 = 7x8 threads to calculate cost for each partition

                    uint mask_index = lidx / 8; //8 threads
                    uint thread_index = lidx % 8;

                    uint treelet_bitmask = GetBitPermutation(bits, mask_index);

                    //8 threads working on same treelet_bitmask
                    //divide work by 8 and find best partition in parallel
                    float lowest_cost = FLT_MAX;
                    uint best_partition = 0;

                    uint delta = (treelet_bitmask - 1) & treelet_bitmask;
                    uint partition_bitmask = (-delta) & treelet_bitmask;

                    // starting partition depends on which thread
                    // 4 partitions per thread
                    partition_bitmask = (partition_bitmask - delta * thread_index * 4) & treelet_bitmask;

                    int counter = 0;

                    do
                    {
                        const float cost = lds_optimal_cost[partition_bitmask] + lds_optimal_cost[treelet_bitmask ^ partition_bitmask];
                        if ((best_partition == 0) || (cost < lowest_cost))
                        {
                            lowest_cost = cost;
                            best_partition = partition_bitmask;
                        }

                        partition_bitmask = (partition_bitmask - delta) & treelet_bitmask;
                        counter++;
                    } while (partition_bitmask != 0 && counter < 4);

                    lds_partiton_cost[lidx] = lowest_cost;
                    lds_partition_mask[lidx] = best_partition;

                    GroupMemoryBarrierWithGroupSync();

                    uint num_unsorted_numbers = 8;
                    for (uint l = 0; l < ceil(log2(num_unsorted_numbers)); l++)
                    {
                        //[][][][]
                        //|/  | /
                        //[]  []
                        //|   /
                        //|  /
                        //[]
                        if ((lidx & ((1 << (l + 1)) - 1)) == 0) //gather and compare at each level
                        {
                            if (lds_partiton_cost[lidx] > lds_partiton_cost[lidx + (1 << l)]) //find min cost
                            {
                                lds_partiton_cost[lidx] = lds_partiton_cost[lidx + (1 << l)];
                                lds_partition_mask[lidx] = lds_partition_mask[lidx + (1 << l)];
                            }
                        }

                        GroupMemoryBarrierWithGroupSync();
                    }

                    if (thread_index == 0)
                    {
                        lds_optimal_cost[treelet_bitmask] = C_INT * lds_area[treelet_bitmask] + lds_partiton_cost[lidx];
                        lds_optimal_partition[treelet_bitmask] = lds_partition_mask[lidx];
                    }
                }
                else if (bits == MAX_TREELET_SIZE)  //special case (optimization, saves 2 ms on Crytek Sponza)
                {
                    uint treelet_bitmask = GetBitPermutation(bits, lidx);

                    //1 mask
                    //63 threads to calculate cost for each partition
                    //64 threads to sort to find lowest cost partition

                    if (lidx < ((1 << (MAX_TREELET_SIZE - 1)) - 1)) //63 possible partitions (1-63)
                    {
                        uint partition_bitmask = ((lidx + 1) << 1); //<<1 because the last bit/leaf will be part of the other side of the partition

                        const float cost = lds_optimal_cost[partition_bitmask] + lds_optimal_cost[treelet_bitmask ^ partition_bitmask];

                        lds_partiton_cost[lidx] = cost;
                        lds_partition_mask[lidx] = partition_bitmask;
                    }

                    GroupMemoryBarrierWithGroupSync();

                    uint num_unsorted_numbers = 63;
                    for (uint l = 0; l < ceil(log2(num_unsorted_numbers)); l++)
                    {
                        //[][][][]
                        //|/  | /
                        //[]  []
                        //|   /
                        //|  /
                        //[]
                        if ((lidx & ((1 << (l + 1)) - 1)) == 0 && ((lidx + (1 << l)) < num_unsorted_numbers - 1)) //gather and compare at each level
                        {
                            if (lds_partiton_cost[lidx] > lds_partiton_cost[lidx + (1 << l)]) //find min cost
                            {
                                lds_partiton_cost[lidx] = lds_partiton_cost[lidx + (1 << l)];
                                lds_partition_mask[lidx] = lds_partition_mask[lidx + (1 << l)];
                            }
                        }

                        GroupMemoryBarrierWithGroupSync();
                    }

                    if (lidx == 0)
                    {
                        lds_optimal_cost[treelet_bitmask] = C_INT * lds_area[treelet_bitmask] + lds_partiton_cost[0];
                        lds_optimal_partition[treelet_bitmask] = lds_partition_mask[0];
                    }
                }
                else
                {
                    //NOTE: since everything in s_full_treelet_size_choose < 64 (wave size), we can simply assign one bitmask per thread
                    uint treelet_bitmask = GetBitPermutation(bits, lidx);

                    float lowest_cost = FLT_MAX;
                    uint best_partition = 0;

                    //https://research.nvidia.com/sites/default/files/pubs/2013-07_Fast-Parallel-Construction/karras2013hpg_paper.pdf
                    //Algorithm 3

                    uint delta = (treelet_bitmask - 1) & treelet_bitmask;
                    uint partition_bitmask = (-delta) & treelet_bitmask;

                    do
                    {
                        const float cost = lds_optimal_cost[partition_bitmask] + lds_optimal_cost[treelet_bitmask ^ partition_bitmask];
                        if ((best_partition == 0) || (cost < lowest_cost))
                        {
                            lowest_cost = cost;
                            best_partition = partition_bitmask;
                        }

                        partition_bitmask = (partition_bitmask - delta) & treelet_bitmask;
                    } while (partition_bitmask != 0);
                    lds_optimal_cost[treelet_bitmask] = C_INT * lds_area[treelet_bitmask] + lowest_cost;

                    //pseusorandomize the partitioning to left vs right as this can impact performance
                    lds_optimal_partition[treelet_bitmask] = (countbits(treelet_bitmask) & 0x1) ? best_partition : (treelet_bitmask ^ best_partition);
                }
            }
        }

        GroupMemoryBarrierWithGroupSync();

        //now have optimal partition for each type of bitmask (ie where to split for the active bits)
        if (lidx == 0)
        {
            const uint full_partition_mask = NUM_TREE_SPLIT_PERMUTATIONS - 1;

            // Now that a reordering has been calculated, reform the tree
            uint nodes_allocated = 1;
            uint partition_stack_size = 1;

            PartitionEntry partition_stack[MAX_TREELET_SIZE];

            partition_stack[0].mask = full_partition_mask;
            partition_stack[0].node_index = lds_internal_nodes[0];

            while (partition_stack_size > 0)
            {
                PartitionEntry partition = partition_stack[partition_stack_size - 1];
                partition_stack_size--;

                PartitionEntry left_entry;
                left_entry.mask = lds_optimal_partition[partition.mask];

                if (countbits(left_entry.mask) > 1) //internal treelet node
                {
                    left_entry.node_index = lds_internal_nodes[nodes_allocated++];
                    partition_stack[partition_stack_size++] = left_entry;
                }
                else //"leaf"..end of treeLet
                {
                    left_entry.node_index = lds_nodes_to_reorder[log2(left_entry.mask)];
                }

                PartitionEntry right_entry;
                right_entry.mask = partition.mask ^ left_entry.mask;
                if (countbits(right_entry.mask) > 1) //internal treelet node
                {
                    right_entry.node_index = lds_internal_nodes[nodes_allocated++];
                    partition_stack[partition_stack_size++] = right_entry;
                }
                else //"leaf"..end of treeLet
                {
                    right_entry.node_index = lds_nodes_to_reorder[log2(right_entry.mask)];
                }

                g_bvh[partition.node_index].child0 = left_entry.node_index;
                g_bvh[partition.node_index].child1 = right_entry.node_index;
                g_bvh[left_entry.node_index].parent = partition.node_index;
                g_bvh[right_entry.node_index].parent = partition.node_index;
            }

            ////recalc AABB for each internal node since they change topology

            // Start from the back. This is optimizing since the previous traversal went from
            // top-down, the reverse order is guaranteed to be bottom-up
            for (int j = MAX_TREELET_INTERNAL_NODES - 1; j >= 0; j--)
            {
                uint internal_node_index = lds_internal_nodes[j];
                Aabb left_aabb = GetNodeAabb(g_bvh[g_bvh[internal_node_index].child0], IS_INTERNAL_NODE(g_bvh[internal_node_index].child0, N));
                Aabb right_aabb = GetNodeAabb(g_bvh[g_bvh[internal_node_index].child1], IS_INTERNAL_NODE(g_bvh[internal_node_index].child1, N));

                g_bvh[internal_node_index].aabb0_min_or_v0 = left_aabb.pmin;
                g_bvh[internal_node_index].aabb0_max_or_v1 = left_aabb.pmax;

                g_bvh[internal_node_index].aabb1_min_or_v2 = right_aabb.pmin;
                g_bvh[internal_node_index].aabb1_max_or_v3 = right_aabb.pmax;
            }
        }

        if (lidx == 0)
        {
            lds_node_index = g_bvh[lds_node_index].parent;

            if (lds_node_index == INVALID_IDX)
            {
                lds_is_done = true;
            }
            else
            {
                uint old = 0;

                InterlockedAdd(g_primitive_counts[lds_node_index], 1, old);
                if (old != 0)
                {
                    Aabb left_aabb = GetNodeAabb(g_bvh[g_bvh[lds_node_index].child0], IS_INTERNAL_NODE(g_bvh[lds_node_index].child0, N));
                    Aabb right_aabb = GetNodeAabb(g_bvh[g_bvh[lds_node_index].child1], IS_INTERNAL_NODE(g_bvh[lds_node_index].child1, N));

                    g_bvh[lds_node_index].aabb0_min_or_v0 = left_aabb.pmin;
                    g_bvh[lds_node_index].aabb0_max_or_v1 = left_aabb.pmax;
                    g_bvh[lds_node_index].aabb1_min_or_v2 = right_aabb.pmin;
                    g_bvh[lds_node_index].aabb1_max_or_v3 = right_aabb.pmax;
                }
                else
                {
                    lds_is_done = true;
                }
            }
        }

        DeviceMemoryBarrierWithGroupSync();

        if (lds_is_done)
        {
            return;
        }
    }
}


