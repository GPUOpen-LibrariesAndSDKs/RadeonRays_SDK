//#define ROOT_SIGNATURE "RootConstants(num32BitConstants=1, b0, visibility=SHADER_VISIBILITY_ALL), "\
 //               "UAV(u0, visibility=SHADER_VISIBILITY_ALL),"\
 //               "UAV(u1, visibility=SHADER_VISIBILITY_ALL),"\
 //               "UAV(u2, visibility=SHADER_VISIBILITY_ALL),"\

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

int BlockReduce(int key, uint lidx)
{
#ifndef USE_WAVE_INTRINSICS
    lds_keys[lidx] = key;

    GroupMemoryBarrierWithGroupSync();

    // Peform reduction within a block
    for (int stride = (GROUP_SIZE >> 1); stride > 0; stride >>= 1)
    {
        if (lidx < stride)
        {
            lds_keys[lidx] += lds_keys[lidx + stride];
        }

        GroupMemoryBarrierWithGroupSync();
    }
    return lds_keys[0];
#else
    lds_keys[lidx] = 0;

    GroupMemoryBarrierWithGroupSync();

    // Perform reduction within a subgroup
    int wave_reduced = WaveActiveSum(key);

    uint widx  = lidx / WaveGetLaneCount();
    uint wlidx = WaveGetLaneIndex();

    // First element of each wave puts subgroup-reduced value into LDS
    if (wlidx == 0)
    {
        lds_keys[widx] = wave_reduced;
    }

    GroupMemoryBarrierWithGroupSync();

    // First subroup reduces partial sums
    if (widx == 0)
    {
        lds_keys[lidx] = WaveActiveSum(lds_keys[lidx]);
    }

    GroupMemoryBarrierWithGroupSync();

    return lds_keys[0];
#endif
}

//[RootSignature(ROOT_SIGNATURE)]
[numthreads(GROUP_SIZE, 1, 1)]
void BlockReducePart(
    in uint gidx: SV_DispatchThreadID,
    in uint lidx: SV_GroupThreadID,
    in uint bidx: SV_GroupID)
{
    int thread_sum = 0;

    // Do coalesced loads and calculate their partial sums right away
    uint range_begin = bidx * GROUP_SIZE * KEYS_PER_THREAD;
    for (uint i = 0; i < KEYS_PER_THREAD; ++i)
    {
        uint load_index = range_begin + i * GROUP_SIZE + lidx;
        thread_sum += (load_index < g_constants.num_keys) ? g_input_keys[load_index] : 0;
    }

    // Reduce sums
    thread_sum = BlockReduce(thread_sum, lidx);

    // First thread writes the sum to partial sums array
    if (lidx == 0)
    {
        g_part_sums[bidx] = thread_sum;
    }
}