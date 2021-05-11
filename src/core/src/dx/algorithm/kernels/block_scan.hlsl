// #define ROOT_SIGNATURE "RootConstants(num32BitConstants=1, b0, visibility=SHADER_VISIBILITY_ALL), "\
//                "UAV(u0, visibility=SHADER_VISIBILITY_ALL),"\
//                "UAV(u1, visibility=SHADER_VISIBILITY_ALL),"\
//                "UAV(u2, visibility=SHADER_VISIBILITY_ALL),"\

struct Constants
{
    uint num_keys;
};

ConstantBuffer<Constants> g_constants: register(b0);
RWStructuredBuffer<int> g_input_keys: register(u0);
RWStructuredBuffer<int> g_part_sums: register(u1);
RWStructuredBuffer<int> g_output_keys: register(u2);

#define GROUP_SIZE 256u
#define KEYS_PER_THREAD 4u
groupshared int lds_keys[GROUP_SIZE];
// This is to transform uncoalesced loads into coalesced loads and 
// then scattered load from LDS
groupshared int lds_loads[KEYS_PER_THREAD][GROUP_SIZE];

int BlockScan(int key, uint lidx)
{
#ifndef USE_WAVE_INTRINSICS
    // Load the key into LDS
    lds_keys[lidx] = key;

    GroupMemoryBarrierWithGroupSync();

    // Calculate reduction
    int stride = 0;
    for (stride = 1; stride < GROUP_SIZE; stride <<= 1)
    {
        if (lidx < GROUP_SIZE / (2 * stride))
        {
            lds_keys[2 * (lidx + 1) * stride - 1] =
                lds_keys[2 * (lidx + 1) * stride - 1] + lds_keys[(2 * lidx + 1) * stride - 1];
        }

        GroupMemoryBarrierWithGroupSync();
    }

    // Then put 0 into the root for downsweep
    if (lidx == 0)
        lds_keys[GROUP_SIZE - 1] = 0;

    GroupMemoryBarrierWithGroupSync();

    // Perform downsweep
    for (stride = GROUP_SIZE >> 1; stride > 0; stride >>= 1)
    {
        if (lidx < GROUP_SIZE / (2 * stride))
        {
            int temp                              = lds_keys[(2 * lidx + 1) * stride - 1];
            lds_keys[(2 * lidx + 1) * stride - 1] = lds_keys[2 * (lidx + 1) * stride - 1];
            lds_keys[2 * (lidx + 1) * stride - 1] = lds_keys[2 * (lidx + 1) * stride - 1] + temp;
        }

        GroupMemoryBarrierWithGroupSync();
    }

    return lds_keys[lidx];
#else
    lds_keys[lidx] = 0;

    GroupMemoryBarrierWithGroupSync();

    // Perform scan within a subgroup
    int wave_scanned = WavePrefixSum(key);

    uint widx  = lidx / WaveGetLaneCount();
    uint wlidx = WaveGetLaneIndex();

    // Last element in each subgroup writes partial sum into LDS
    if (wlidx == WaveGetLaneCount() - 1)
    {
        lds_keys[widx] = wave_scanned + key;
    }

    GroupMemoryBarrierWithGroupSync();

    // Then first subgroup scannes partial sums
    if (widx == 0)
    {
        lds_keys[lidx] = WavePrefixSum(lds_keys[lidx]);
    }

    GroupMemoryBarrierWithGroupSync();

    // And we add partial sums back to each subgroup-scanned element
    wave_scanned += lds_keys[widx];

    return wave_scanned;
#endif
}

//[RootSignature(ROOT_SIGNATURE)]
[numthreads(GROUP_SIZE, 1, 1)]
void BlockScanAdd(
    in uint gidx: SV_DispatchThreadID,
    in uint lidx: SV_GroupThreadID,
    in uint bidx: SV_GroupID)
{
    uint i = 0; // Loop counter.

    // Perform coalesced load into LDS
    uint range_begin = bidx * GROUP_SIZE * KEYS_PER_THREAD;
    for (i = 0; i < KEYS_PER_THREAD; ++i)
    {
        uint load_index = range_begin + i * GROUP_SIZE + lidx;

        uint col = (i * GROUP_SIZE + lidx) / KEYS_PER_THREAD;
        uint row = (i * GROUP_SIZE + lidx) % KEYS_PER_THREAD;

        lds_loads[row][col] = (load_index < g_constants.num_keys) ? g_input_keys[load_index] : 0;
    }

    GroupMemoryBarrierWithGroupSync();

    int thread_sum = 0;

    // Calculate scan on this thread's elements
    for (i = 0; i < KEYS_PER_THREAD; ++i)
    {
        int tmp = lds_loads[i][lidx];
        lds_loads[i][lidx] = thread_sum;
        thread_sum += tmp;
    }

    // Scan partial sums
    thread_sum = BlockScan(thread_sum, lidx);

    // Add global partial sums if required
    int part_sum = 0;
#ifdef ADD_PART_SUM
    part_sum = g_part_sums[bidx];
#endif

    // Add partial sums back
    for (i = 0; i < KEYS_PER_THREAD; ++i)
    {
        lds_loads[i][lidx] += thread_sum;
    }

    GroupMemoryBarrierWithGroupSync();

    // Perform coalesced writes back to global memory
    for (i = 0; i < KEYS_PER_THREAD; ++i)
    {
        uint store_index = range_begin + i * GROUP_SIZE + lidx;
        uint col = (i * GROUP_SIZE + lidx) / KEYS_PER_THREAD;
        uint row = (i * GROUP_SIZE + lidx) % KEYS_PER_THREAD;

        if (store_index < g_constants.num_keys)
        {
            g_output_keys[store_index] = lds_loads[row][col] + part_sum;
        }
    }
}