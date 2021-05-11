struct Constants
{
    uint num_keys;
    uint num_blocks;
    uint bit_shift;
    uint padding;
};

ConstantBuffer<Constants> g_constants : register(b0);
RWStructuredBuffer<uint> g_input_keys : register(u0);
RWStructuredBuffer<uint> g_group_histograms: register(u1);
RWStructuredBuffer<uint> g_output_keys : register(u2);
#ifdef SORT_VALUES
RWStructuredBuffer<uint> g_input_values : register(u3);
RWStructuredBuffer<uint> g_output_values : register(u4);
#endif

#define NUM_BINS 16u
#define GROUP_SIZE 256u
#define KEYS_PER_THREAD 4u
groupshared int lds_histograms[NUM_BINS];

//[RootSignature(ROOT_SIGNATURE)]
[numthreads(GROUP_SIZE, 1, 1)]
void BitHistogram(
    in uint gidx: SV_DispatchThreadID,
    in uint lidx: SV_GroupThreadID,
    in uint bidx: SV_GroupID)
{
    if (lidx < NUM_BINS)
    {
        lds_histograms[lidx] = 0;
    }

    GroupMemoryBarrierWithGroupSync();

    for (int i = 0; i < KEYS_PER_THREAD; ++i)
    {
        // Calculate next input element index
        uint key_index = gidx * KEYS_PER_THREAD + i;

        // Handle out of bounds
        if (key_index >= g_constants.num_keys)
        {
            break;
        }

        // Determine bin index for next element
        int bin_index = (g_input_keys[key_index] >> g_constants.bit_shift) & 0xf;

        // Increment LDS histogram counter (no atomic required, histogram is
        // private)
        InterlockedAdd(lds_histograms[bin_index], 1);
    }

    GroupMemoryBarrierWithGroupSync();

    // Write reduced bins into global memory
    if (lidx < NUM_BINS)
    {
        g_group_histograms[g_constants.num_blocks * lidx + bidx] = lds_histograms[lidx];
    }
}