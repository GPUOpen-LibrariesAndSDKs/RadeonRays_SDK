//#define ROOT_SIGNATURE "RootConstants(num32BitConstants=3, b0, visibility=SHADER_VISIBILITY_ALL), "\
//               "UAV(u0, visibility=SHADER_VISIBILITY_ALL),"\
//                "UAV(u1, visibility=SHADER_VISIBILITY_ALL),"\

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

#define NUM_BITS_PER_PASS 4
#define NUM_BINS (1 << NUM_BITS_PER_PASS)
#define GROUP_SIZE 256u
#define KEYS_PER_THREAD 4u
//#define HISTOGRAM_TYPE short
#define INT_MAX 0x7fffffff

// Scratch memory for group operations
groupshared int lds_scratch[GROUP_SIZE];
// Temporary storage for the keys
groupshared int lds_keys[GROUP_SIZE];
// Cache for scanned histogram for faster indexing
groupshared int lds_scanned_histogram[NUM_BINS];
// Block histogram
groupshared int lds_histogram[NUM_BINS];

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
void Scatter(
    in uint gidx: SV_DispatchThreadID,
    in uint lidx: SV_GroupThreadID,
    in uint bidx: SV_GroupID)
{
    // Cache scanned histogram in LDS and clear block histogram
    if (lidx < NUM_BINS)
    {
        lds_scanned_histogram[lidx] = g_group_histograms[g_constants.num_blocks * lidx + bidx];
    }

    // Starting point of our block in global memory
    uint block_start_index = bidx * GROUP_SIZE * KEYS_PER_THREAD;
    // Each thread handles PP_KEYS_PER_THREAD elements
    for (int i = 0; i < KEYS_PER_THREAD; ++i)
    {
        // Clear block histogram
        if (lidx < NUM_BINS)
        {
            lds_histogram[lidx] = 0;
        }

        // Calculate next input element index
        uint key_index = block_start_index + i * GROUP_SIZE + lidx;

        // Fetch next element and put it in LDS
        int key = (key_index < g_constants.num_keys) ? g_input_keys[key_index] : INT_MAX;
#ifdef SORT_VALUES
        int value = (key_index < g_constants.num_keys) ? g_input_values[key_index] : 0;
#endif

        // Sort keys locally in LDS
        for (uint shift = 0; shift < NUM_BITS_PER_PASS; shift += 2)
        {
            // Detemine bin index for the key
            int bin_index = ((key >> g_constants.bit_shift) >> shift) & 0x3;
            // Create local packed histogram (0th in 1st byte, 1th in 2nd, etc)
            int local_histogram = 1 << (bin_index * 8);
            // Scan local histograms
            int local_histogram_scanned = BlockScan(local_histogram, lidx);
            // Last thread in a block broadcasts total block histogram
            if (lidx == (GROUP_SIZE - 1))
            {
                lds_scratch[0] = local_histogram_scanned + local_histogram;
            }
            // Make sure broadcast happened
            GroupMemoryBarrierWithGroupSync();
            // Load broadcast value
            local_histogram = lds_scratch[0];
            // Scan block histogram in order to add scanned values as offsets
            local_histogram = (local_histogram << 8) +
                              (local_histogram << 16) +
                              (local_histogram << 24);
            // Add offsets from previos bins
            local_histogram_scanned += local_histogram;
            // Calculate target offset
            int offset = (local_histogram_scanned >> (bin_index * 8)) & 0xff;
            // Distribute the key
            lds_keys[offset] = key;
            //
            GroupMemoryBarrierWithGroupSync();
            // Load key
            key = lds_keys[lidx];

#ifdef SORT_VALUES
            // Perform value exchange
            GroupMemoryBarrierWithGroupSync();
            lds_keys[offset] = value;
            GroupMemoryBarrierWithGroupSync();
            value = lds_keys[lidx];
            GroupMemoryBarrierWithGroupSync();
#endif
        }

        // Reconstruct original 16-bins histogram
        int bin_index = (key >> g_constants.bit_shift) & 0xf;

        InterlockedAdd(lds_histogram[bin_index], 1);

        GroupMemoryBarrierWithGroupSync();

        // Scan original histogram
        int histogram_value = BlockScan(lidx < NUM_BINS ? lds_histogram[lidx] : 0, lidx);

        // Broadcast scanned histogram via LDS
        if (lidx < NUM_BINS)
        {
            lds_scratch[lidx] = histogram_value;
        }

        // Fetch scanned block histogram index
        int global_offset = lds_scanned_histogram[bin_index];

        GroupMemoryBarrierWithGroupSync();

        // Fetch scanned histogram within a block
        int local_offset = int(lidx) - lds_scratch[bin_index];

        // Write the element back to global memory
        if (global_offset + local_offset < g_constants.num_keys)
        {
            g_output_keys[global_offset + local_offset] = key;
#ifdef SORT_VALUES
            g_output_values[global_offset + local_offset] = value;
#endif
        }

        GroupMemoryBarrierWithGroupSync();

        // Update scanned histogram
        if (lidx < NUM_BINS)
        {
            lds_scanned_histogram[lidx] += lds_histogram[lidx];
        }
    }
}