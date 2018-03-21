/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/
#include "CLWParallelPrimitives.h"

#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <cstring>

#ifdef RR_EMBED_KERNELS
#if USE_OPENCL
#include "clwkernels_cl.h"
#endif
#endif // RR_EMBED_KERNELS

#define WG_SIZE 64
#define NUM_SCAN_ELEMS_PER_WI 8
#define NUM_SEG_SCAN_ELEMS_PER_WI 1
#define NUM_SCAN_ELEMS_PER_WG (WG_SIZE * NUM_SCAN_ELEMS_PER_WI)
#define NUM_SEG_SCAN_ELEMS_PER_WG (WG_SIZE * NUM_SEG_SCAN_ELEMS_PER_WI)

CLWParallelPrimitives::CLWParallelPrimitives(CLWContext context, char const* buildopts)
    : context_(context)
{
#ifndef RR_EMBED_KERNELS
    program_ = CLWProgram::CreateFromFile("../CLW/CL/CLW.cl", buildopts, context_);
#else
    program_ = CLWProgram::CreateFromSource(g_CLW_opencl, std::strlen(g_CLW_opencl), buildopts, context_);
#endif
}

CLWParallelPrimitives::~CLWParallelPrimitives()
{
    ReclaimDeviceMemory();
}

CLWEvent CLWParallelPrimitives::ScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems)
{
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_int4");

    topLevelScan.SetArg(0, input);
    topLevelScan.SetArg(1, output);
    topLevelScan.SetArg(2, (cl_uint)numElems);
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));

    return context_.Launch1D(0, WG_SIZE, WG_SIZE, topLevelScan);
}


CLWEvent CLWParallelPrimitives::ScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems)
{
    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE << 3);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE << 2);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSums = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
        //context_.CreateBuffer<cl_int>(NUM_GROUPS_BOTTOM_LEVEL);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_int4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_int4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_int4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSums);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSums);
    topLevelScan.SetArg(1, devicePartSums);
    topLevelScan.SetArg(2, (cl_uint)devicePartSums.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSums);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempIntBuffer(devicePartSums);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
}




CLWEvent CLWParallelPrimitives::SegmentedScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();
    
    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE);
    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    
    auto devicePartSums = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartFlags = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    //context_.CreateBuffer<cl_int>(NUM_GROUPS_BOTTOM_LEVEL);

    CLWKernel bottomLevelScan = program_.GetKernel("segmented_scan_exclusive_int_part");
    CLWKernel topLevelScan = program_.GetKernel("segmented_scan_exclusive_int_nocut");
    CLWKernel distributeSums = program_.GetKernel("segmented_distribute_part_sum_int");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, inputHeads);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, output);
    bottomLevelScan.SetArg(4, devicePartSums);
    bottomLevelScan.SetArg(5, devicePartFlags);
    bottomLevelScan.SetArg(6, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    //std::vector<cl_int> hostPartSums(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    //std::vector<cl_int> hostPartFlags(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    //std::vector<cl_int> hostResult(numElems);

    //context_.ReadBuffer(0,  output, &hostResult[0], numElems).Wait();

    //context_.ReadBuffer(0,  devicePartSums, &hostPartSums[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();
    //context_.ReadBuffer(0,  devicePartFlags, &hostPartFlags[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();
    
    topLevelScan.SetArg(0, devicePartSums);
    topLevelScan.SetArg(1, devicePartFlags);
    topLevelScan.SetArg(2, (cl_uint)devicePartSums.GetElementCount());
    topLevelScan.SetArg(3, devicePartSums);
    topLevelScan.SetArg(4, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    //context_.ReadBuffer(0,  devicePartSums, &hostPartSums[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();
    
    distributeSums.SetArg(0, output);
    distributeSums.SetArg(1, inputHeads);
    distributeSums.SetArg(2, numElems);
    distributeSums.SetArg(3, devicePartSums);
    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, distributeSums);
    
    //context_.ReadBuffer(0,  output, &hostResult[0], numElems).Wait();
    
    //return CLWEvent();
    //distributeSums.SetArg(0, devicePartSums);
    //distributeSums.SetArg(1, output);
    //distributeSums.SetArg(2, (cl_uint)numElems);
    
    //ReclaimTempIntBuffer(devicePartSums);
    //ReclaimTempIntBuffer(devicePartFlags);
    
    //return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
}

CLWEvent CLWParallelPrimitives::SegmentedScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();
    
    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_MID_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_MID_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;
    int NUM_GROUPS_MID_LEVEL_DISTRIBUTE = (NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSumsBottomLevel = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartFlagsBottomLevel = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartSumsMidLevel = GetTempIntBuffer(NUM_GROUPS_MID_LEVEL_SCAN);
    auto devicePartFlagsMidLevel = GetTempIntBuffer(NUM_GROUPS_MID_LEVEL_SCAN);

    CLWKernel bottomLevelScan = program_.GetKernel("segmented_scan_exclusive_int_part");
    CLWKernel midLevelScan = program_.GetKernel("segmented_scan_exclusive_int_nocut_part");
    CLWKernel topLevelScan = program_.GetKernel("segmented_scan_exclusive_int_nocut");
    CLWKernel distributeSumsBottomLevel = program_.GetKernel("segmented_distribute_part_sum_int");
    CLWKernel distributeSumsMidLevel = program_.GetKernel("segmented_distribute_part_sum_int_nocut");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, inputHeads);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, output);
    bottomLevelScan.SetArg(4, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(5, devicePartFlagsBottomLevel);
    bottomLevelScan.SetArg(6, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    //std::vector<cl_int> hostPartSumsBL(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    //std::vector<cl_int> hostPartFlagsBL(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    //std::vector<cl_int> hostPartSumsML(NUM_GROUPS_MID_LEVEL_SCAN);
    //std::vector<cl_int> hostPartFlagsML(NUM_GROUPS_MID_LEVEL_SCAN);

    //context_.ReadBuffer(0,  output, &hostResult[0], numElems).Wait();

    //context_.ReadBuffer(0,  devicePartSumsBottomLevel, &hostPartSumsBL[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();
    //context_.ReadBuffer(0,  devicePartFlagsBottomLevel, &hostPartFlagsBL[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();

    midLevelScan.SetArg(0, devicePartSumsBottomLevel);
    midLevelScan.SetArg(1, devicePartFlagsBottomLevel);
    midLevelScan.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    midLevelScan.SetArg(3, devicePartSumsBottomLevel);
    midLevelScan.SetArg(4, devicePartSumsMidLevel);
    midLevelScan.SetArg(5, devicePartFlagsMidLevel);

    midLevelScan.SetArg(6, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_SCAN * WG_SIZE, WG_SIZE, midLevelScan);

    //context_.ReadBuffer(0,  devicePartSumsMidLevel, &hostPartSumsML[0], NUM_GROUPS_MID_LEVEL_SCAN).Wait();
    //context_.ReadBuffer(0,  devicePartFlagsMidLevel, &hostPartFlagsML[0], NUM_GROUPS_MID_LEVEL_SCAN).Wait();

    topLevelScan.SetArg(0, devicePartSumsMidLevel);
    topLevelScan.SetArg(1, devicePartFlagsMidLevel);
    topLevelScan.SetArg(2, (cl_uint)devicePartSumsMidLevel.GetElementCount());
    topLevelScan.SetArg(3, devicePartSumsMidLevel);
    topLevelScan.SetArg(4, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    //context_.ReadBuffer(0,  devicePartSumsMidLevel, &hostPartSumsML[0], NUM_GROUPS_MID_LEVEL_SCAN).Wait();
    //context_.ReadBuffer(0,  devicePartFlagsMidLevel, &hostPartFlagsML[0], NUM_GROUPS_MID_LEVEL_SCAN).Wait();

    distributeSumsMidLevel.SetArg(0, devicePartSumsBottomLevel);
    distributeSumsMidLevel.SetArg(1, devicePartFlagsBottomLevel);
    distributeSumsMidLevel.SetArg(2, NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    distributeSumsMidLevel.SetArg(3, devicePartSumsMidLevel);

    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSumsMidLevel);

    //context_.ReadBuffer(0,  devicePartSumsBottomLevel, &hostPartSumsBL[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();
    //context_.ReadBuffer(0,  devicePartFlagsBottomLevel, &hostPartFlagsBL[0], NUM_GROUPS_BOTTOM_LEVEL_SCAN).Wait();

    distributeSumsBottomLevel.SetArg(0, output);
    distributeSumsBottomLevel.SetArg(1, inputHeads);
    distributeSumsBottomLevel.SetArg(2, numElems);
    distributeSumsBottomLevel.SetArg(3, devicePartSumsBottomLevel);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSumsBottomLevel);
}


CLWEvent CLWParallelPrimitives::SegmentedScanExclusiveAddFourLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();
    
    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_MID_LEVEL_SCAN_1 = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_MID_LEVEL_SCAN_2 = (NUM_GROUPS_MID_LEVEL_SCAN_1 + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_MID_LEVEL_SCAN_2 + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;
    int NUM_GROUPS_MID_LEVEL_DISTRIBUTE_1 = (NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;
    int NUM_GROUPS_MID_LEVEL_DISTRIBUTE_2 = (NUM_GROUPS_MID_LEVEL_DISTRIBUTE_1 + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSumsBottomLevel = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartFlagsBottomLevel = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartSumsMidLevel1 = GetTempIntBuffer(NUM_GROUPS_MID_LEVEL_SCAN_1);
    auto devicePartFlagsMidLevel1 = GetTempIntBuffer(NUM_GROUPS_MID_LEVEL_SCAN_1);
    auto devicePartSumsMidLevel2 = GetTempIntBuffer(NUM_GROUPS_MID_LEVEL_SCAN_2);
    auto devicePartFlagsMidLevel2 = GetTempIntBuffer(NUM_GROUPS_MID_LEVEL_SCAN_2);

    CLWKernel bottomLevelScan = program_.GetKernel("segmented_scan_exclusive_int_part");
    CLWKernel midLevelScan = program_.GetKernel("segmented_scan_exclusive_int_nocut_part");
    CLWKernel topLevelScan = program_.GetKernel("segmented_scan_exclusive_int_nocut");
    CLWKernel distributeSumsBottomLevel = program_.GetKernel("segmented_distribute_part_sum_int");
    CLWKernel distributeSumsMidLevel = program_.GetKernel("segmented_distribute_part_sum_int_nocut");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, inputHeads);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, output);
    bottomLevelScan.SetArg(4, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(5, devicePartFlagsBottomLevel);
    bottomLevelScan.SetArg(6, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    midLevelScan.SetArg(0, devicePartSumsBottomLevel);
    midLevelScan.SetArg(1, devicePartFlagsBottomLevel);
    midLevelScan.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    midLevelScan.SetArg(3, devicePartSumsBottomLevel);
    midLevelScan.SetArg(4, devicePartSumsMidLevel1);
    midLevelScan.SetArg(5, devicePartFlagsMidLevel1);

    midLevelScan.SetArg(6, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_SCAN_1 * WG_SIZE, WG_SIZE, midLevelScan);

    midLevelScan.SetArg(0, devicePartSumsMidLevel1);
    midLevelScan.SetArg(1, devicePartFlagsMidLevel1);
    midLevelScan.SetArg(2, (cl_uint)devicePartSumsMidLevel1.GetElementCount());
    midLevelScan.SetArg(3, devicePartSumsMidLevel1);
    midLevelScan.SetArg(4, devicePartSumsMidLevel2);
    midLevelScan.SetArg(5, devicePartFlagsMidLevel2);

    midLevelScan.SetArg(6, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_SCAN_2 * WG_SIZE, WG_SIZE, midLevelScan);

    topLevelScan.SetArg(0, devicePartSumsMidLevel2);
    topLevelScan.SetArg(1, devicePartFlagsMidLevel2);
    topLevelScan.SetArg(2, (cl_uint)devicePartSumsMidLevel2.GetElementCount());
    topLevelScan.SetArg(3, devicePartSumsMidLevel2);
    topLevelScan.SetArg(4, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSumsMidLevel.SetArg(0, devicePartSumsMidLevel1);
    distributeSumsMidLevel.SetArg(1, devicePartFlagsMidLevel1);
    distributeSumsMidLevel.SetArg(2, NUM_GROUPS_MID_LEVEL_SCAN_1);
    distributeSumsMidLevel.SetArg(3, devicePartSumsMidLevel2);

    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_DISTRIBUTE_2 * WG_SIZE, WG_SIZE, distributeSumsMidLevel);

    distributeSumsMidLevel.SetArg(0, devicePartSumsBottomLevel);
    distributeSumsMidLevel.SetArg(1, devicePartFlagsBottomLevel);
    distributeSumsMidLevel.SetArg(2, NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    distributeSumsMidLevel.SetArg(3, devicePartSumsMidLevel1);

    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_DISTRIBUTE_1 * WG_SIZE, WG_SIZE, distributeSumsMidLevel);

    distributeSumsBottomLevel.SetArg(0, output);
    distributeSumsBottomLevel.SetArg(1, inputHeads);
    distributeSumsBottomLevel.SetArg(2, numElems);
    distributeSumsBottomLevel.SetArg(3, devicePartSumsBottomLevel);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSumsBottomLevel);
}




CLWEvent CLWParallelPrimitives::ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems)
{
    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE << 3);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE << 2);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_MID_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_MID_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;
    int NUM_GROUPS_MID_LEVEL_DISTRIBUTE = (NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSumsBottomLevel = GetTempIntBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartSumsMidLevel = GetTempIntBuffer(NUM_GROUPS_MID_LEVEL_SCAN);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_int4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_int4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_int4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    bottomLevelScan.SetArg(0, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(1, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    bottomLevelScan.SetArg(3, devicePartSumsMidLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSumsMidLevel);
    topLevelScan.SetArg(1, devicePartSumsMidLevel);
    topLevelScan.SetArg(2, (cl_uint)devicePartSumsMidLevel.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSumsMidLevel);
    distributeSums.SetArg(1, devicePartSumsBottomLevel);
    distributeSums.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);

    distributeSums.SetArg(0, devicePartSumsBottomLevel);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempIntBuffer(devicePartSumsMidLevel);
    ReclaimTempIntBuffer(devicePartSumsBottomLevel);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
}



CLWEvent CLWParallelPrimitives::ScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output, int numElems)
{
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_float4");

    topLevelScan.SetArg(0, input);
    topLevelScan.SetArg(1, output);
    topLevelScan.SetArg(2, (cl_uint)numElems);
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));

    return context_.Launch1D(0, WG_SIZE, WG_SIZE, topLevelScan);
}

CLWEvent CLWParallelPrimitives::SegmentedScanExclusiveAddWG(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output)
{
    cl_uint numElems = (cl_uint)input.GetElementCount();
    
    CLWKernel topLevelScan = program_.GetKernel("segmented_scan_exclusive_int");
    
    topLevelScan.SetArg(0, input);
    topLevelScan.SetArg(1, inputHeads);
    topLevelScan.SetArg(2, (cl_uint)numElems);
    topLevelScan.SetArg(3, output);
    topLevelScan.SetArg(4, SharedMemory(WG_SIZE * (sizeof(cl_int) + sizeof(cl_char))));
    
    return context_.Launch1D(0, WG_SIZE, WG_SIZE, topLevelScan);
}


CLWEvent CLWParallelPrimitives::ScanExclusiveAddTwoLevel(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output, int numElems)
{
    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE << 3);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE << 2);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSums = GetTempFloatBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
        //context_.CreateBuffer<cl_int>(NUM_GROUPS_BOTTOM_LEVEL);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_float4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_float4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_float4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSums);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSums);
    topLevelScan.SetArg(1, devicePartSums);
    topLevelScan.SetArg(2, (cl_uint)devicePartSums.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSums);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempFloatBuffer(devicePartSums);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
}

CLWEvent CLWParallelPrimitives::ScanExclusiveAddThreeLevel(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output, int numElems)
{
    int GROUP_BLOCK_SIZE_SCAN = (WG_SIZE << 3);
    int GROUP_BLOCK_SIZE_DISTRIBUTE = (WG_SIZE << 2);

    int NUM_GROUPS_BOTTOM_LEVEL_SCAN = (numElems + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_MID_LEVEL_SCAN = (NUM_GROUPS_BOTTOM_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;
    int NUM_GROUPS_TOP_LEVEL_SCAN = (NUM_GROUPS_MID_LEVEL_SCAN + GROUP_BLOCK_SIZE_SCAN - 1) / GROUP_BLOCK_SIZE_SCAN;

    int NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE = (numElems + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;
    int NUM_GROUPS_MID_LEVEL_DISTRIBUTE = (NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE + GROUP_BLOCK_SIZE_DISTRIBUTE - 1) / GROUP_BLOCK_SIZE_DISTRIBUTE;

    auto devicePartSumsBottomLevel = GetTempFloatBuffer(NUM_GROUPS_BOTTOM_LEVEL_SCAN);
    auto devicePartSumsMidLevel = GetTempFloatBuffer(NUM_GROUPS_MID_LEVEL_SCAN);

    CLWKernel bottomLevelScan = program_.GetKernel("scan_exclusive_part_float4");
    CLWKernel topLevelScan = program_.GetKernel("scan_exclusive_float4");
    CLWKernel distributeSums = program_.GetKernel("distribute_part_sum_float4");

    bottomLevelScan.SetArg(0, input);
    bottomLevelScan.SetArg(1, output);
    bottomLevelScan.SetArg(2, numElems);
    bottomLevelScan.SetArg(3, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    bottomLevelScan.SetArg(0, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(1, devicePartSumsBottomLevel);
    bottomLevelScan.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    bottomLevelScan.SetArg(3, devicePartSumsMidLevel);
    bottomLevelScan.SetArg(4, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_SCAN * WG_SIZE, WG_SIZE, bottomLevelScan);

    topLevelScan.SetArg(0, devicePartSumsMidLevel);
    topLevelScan.SetArg(1, devicePartSumsMidLevel);
    topLevelScan.SetArg(2, (cl_uint)devicePartSumsMidLevel.GetElementCount());
    topLevelScan.SetArg(3, SharedMemory(WG_SIZE * sizeof(cl_int)));
    context_.Launch1D(0, NUM_GROUPS_TOP_LEVEL_SCAN * WG_SIZE, WG_SIZE, topLevelScan);

    distributeSums.SetArg(0, devicePartSumsMidLevel);
    distributeSums.SetArg(1, devicePartSumsBottomLevel);
    distributeSums.SetArg(2, (cl_uint)devicePartSumsBottomLevel.GetElementCount());
    context_.Launch1D(0, NUM_GROUPS_MID_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);

    distributeSums.SetArg(0, devicePartSumsBottomLevel);
    distributeSums.SetArg(1, output);
    distributeSums.SetArg(2, (cl_uint)numElems);

    ReclaimTempFloatBuffer(devicePartSumsMidLevel);
    ReclaimTempFloatBuffer(devicePartSumsBottomLevel);

    return context_.Launch1D(0, NUM_GROUPS_BOTTOM_LEVEL_DISTRIBUTE * WG_SIZE, WG_SIZE, distributeSums);
}

CLWEvent CLWParallelPrimitives::ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems)
{
    if (numElems < NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddWG(deviceIdx, input, output, numElems);
    }
    else if (numElems < NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddTwoLevel(deviceIdx, input, output, numElems);
    }
    else if (numElems < NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddThreeLevel(deviceIdx, input, output, numElems);
    }
    else
    {
        throw std::runtime_error("The maximum number of elements for scan exceeded\n");
    }

    return CLWEvent::Create(nullptr);
}

CLWEvent CLWParallelPrimitives::ScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_float> input, CLWBuffer<cl_float> output, int numElems)
{
    if (numElems <= NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddWG(deviceIdx, input, output, numElems);
    }
    else if (numElems <= NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddTwoLevel(deviceIdx, input, output, numElems);
    }
    else if (numElems <= NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG * NUM_SCAN_ELEMS_PER_WG)
    {
        return ScanExclusiveAddThreeLevel(deviceIdx, input, output, numElems);
    }
    else
    {
        throw std::runtime_error("The maximum number of elements for scan exceeded\n");
    }

    return CLWEvent::Create(nullptr);
}


CLWEvent CLWParallelPrimitives::SegmentedScanExclusiveAdd(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> inputHeads, CLWBuffer<cl_int> output)
{
    assert(input.GetElementCount() == output.GetElementCount());
    
    cl_uint numElems = (cl_uint)input.GetElementCount();
    
    if (numElems <= NUM_SEG_SCAN_ELEMS_PER_WG)
    {
        return SegmentedScanExclusiveAddWG(deviceIdx, input, inputHeads, output);
    }
    else if (numElems <= NUM_SEG_SCAN_ELEMS_PER_WG * NUM_SEG_SCAN_ELEMS_PER_WG)
    {
        return SegmentedScanExclusiveAddTwoLevel(deviceIdx, input, inputHeads, output);
    }
    else if (numElems <= NUM_SEG_SCAN_ELEMS_PER_WG * NUM_SEG_SCAN_ELEMS_PER_WG * NUM_SEG_SCAN_ELEMS_PER_WG)
    {
        return SegmentedScanExclusiveAddThreeLevel(deviceIdx, input, inputHeads, output);
    }
    else if  (numElems <= NUM_SEG_SCAN_ELEMS_PER_WG * NUM_SEG_SCAN_ELEMS_PER_WG * NUM_SEG_SCAN_ELEMS_PER_WG * NUM_SEG_SCAN_ELEMS_PER_WG)
    {
        return SegmentedScanExclusiveAddFourLevel(deviceIdx, input, inputHeads, output);
    }
    else
    {
        throw std::runtime_error("The maximum number of elements for scan exceeded\n");
    }
    
    return CLWEvent::Create(nullptr);
}

CLWBuffer<cl_int> CLWParallelPrimitives::GetTempIntBuffer(size_t size)
{
    auto iter = intBufferCache_.find(size);

    if (iter != intBufferCache_.end())
    {
        CLWBuffer<int> tmp = iter->second;
        intBufferCache_.erase(iter);
        return tmp;
    }

    return context_.CreateBuffer<cl_int>(size, CL_MEM_READ_WRITE);
}

CLWBuffer<char> CLWParallelPrimitives::GetTempCharBuffer(size_t size)
{
    auto iter = charBufferCache_.find(size);

    if (iter != charBufferCache_.end())
    {
        CLWBuffer<char> tmp = iter->second;
        charBufferCache_.erase(iter);
        return tmp;
    }

    return context_.CreateBuffer<char>(size, CL_MEM_READ_WRITE);
}

CLWBuffer<cl_float> CLWParallelPrimitives::GetTempFloatBuffer(size_t size)
{
    auto iter = floatBufferCache_.find(size);

    if (iter != floatBufferCache_.end())
    {
        CLWBuffer<cl_float> tmp = iter->second;
        floatBufferCache_.erase(iter);
        return tmp;
    }

    return context_.CreateBuffer<cl_float>(size, CL_MEM_READ_WRITE);
}

CLWEvent CLWParallelPrimitives::SortRadix(unsigned int deviceIdx, CLWBuffer<cl_int> inputKeys, CLWBuffer<cl_int> outputKeys,
    CLWBuffer<cl_int> inputValues, CLWBuffer<cl_int> outputValues, int numElems)
{
    assert(inputKeys.GetElementCount() == outputKeys.GetElementCount());
    assert(inputValues.GetElementCount() == inputValues.GetElementCount());

    //cl_uint numElems = (cl_uint)inputKeys.GetElementCount();

    int GROUP_BLOCK_SIZE = (WG_SIZE * 4 * 8);
    int NUM_BLOCKS = (numElems + GROUP_BLOCK_SIZE - 1) / GROUP_BLOCK_SIZE;

    auto deviceHistograms = GetTempIntBuffer(NUM_BLOCKS * 16);
    auto deviceTempKeysBuffer = GetTempIntBuffer(numElems);
    auto deviceTempValsBuffer = GetTempIntBuffer(numElems);

    auto fromKeys = &inputKeys;
    auto fromVals = &inputValues;
    auto toKeys = &deviceTempKeysBuffer;
    auto toVals = &deviceTempValsBuffer;

    CLWKernel histogramKernel = program_.GetKernel("BitHistogram");
    CLWKernel scatterKeysAndVals = program_.GetKernel("ScatterKeysAndValues");

    CLWEvent event;

    for (int offset = 0; offset < 32; offset += 4)
    {
        // Split
        histogramKernel.SetArg(0, offset);
        histogramKernel.SetArg(1, *fromKeys);
        histogramKernel.SetArg(2, numElems);
        histogramKernel.SetArg(3, deviceHistograms);

        context_.Launch1D(0, NUM_BLOCKS*WG_SIZE, WG_SIZE, histogramKernel);

        // Scan histograms
        ScanExclusiveAdd(0, deviceHistograms, deviceHistograms, numElems);

        //context_.ReadBuffer(0, deviceHistograms, &hist[0], 16).Wait();

        // Scatter keys
        scatterKeysAndVals.SetArg(0, offset);
        scatterKeysAndVals.SetArg(1, *fromKeys);
        scatterKeysAndVals.SetArg(2, *fromVals);
        scatterKeysAndVals.SetArg(3, numElems);
        scatterKeysAndVals.SetArg(4, deviceHistograms);
        scatterKeysAndVals.SetArg(5, *toKeys);
        scatterKeysAndVals.SetArg(6, *toVals);

        event = context_.Launch1D(0, NUM_BLOCKS*WG_SIZE, WG_SIZE, scatterKeysAndVals);

        //context_.ReadBuffer(0, *toKeys, &keys[0], 64).Wait();

        if (offset == 0)
        {
            fromKeys = &outputKeys;
            fromVals = &outputValues;
        }

        // Swap pointers
        std::swap(fromKeys, toKeys);
        std::swap(fromVals, toVals);
    }

    // Return buffers to memory manager
    ReclaimTempIntBuffer(deviceHistograms);
    ReclaimTempIntBuffer(deviceTempKeysBuffer);
    ReclaimTempIntBuffer(deviceTempValsBuffer);

    return event;
}

CLWEvent CLWParallelPrimitives::SortRadix(unsigned int deviceIdx, CLWBuffer<char> inputKeys, CLWBuffer<char> outputKeys,
    CLWBuffer<char> inputValues, CLWBuffer<char> outputValues, int numElems)
{
    int GROUP_BLOCK_SIZE = (WG_SIZE * 4 * 8);
    int NUM_BLOCKS = (numElems + GROUP_BLOCK_SIZE - 1) / GROUP_BLOCK_SIZE;

    auto deviceHistograms = GetTempIntBuffer(NUM_BLOCKS * 16);
    auto deviceTempKeysBuffer = GetTempCharBuffer(numElems * 4);
    auto deviceTempValsBuffer = GetTempCharBuffer(numElems * 4);

    auto fromKeys = &inputKeys;
    auto fromVals = &inputValues;
    auto toKeys = &deviceTempKeysBuffer;
    auto toVals = &deviceTempValsBuffer;

    CLWKernel histogramKernel = program_.GetKernel("BitHistogram");
    CLWKernel scatterKeysAndVals = program_.GetKernel("ScatterKeysAndValues");

    CLWEvent event;

    for (int offset = 0; offset < 32; offset += 4)
    {
        // Split
        histogramKernel.SetArg(0, offset);
        histogramKernel.SetArg(1, *fromKeys);
        histogramKernel.SetArg(2, numElems);
        histogramKernel.SetArg(3, deviceHistograms);

        context_.Launch1D(0, NUM_BLOCKS*WG_SIZE, WG_SIZE, histogramKernel);

        // Scan histograms
        ScanExclusiveAdd(0, deviceHistograms, deviceHistograms, numElems);

        //context_.ReadBuffer(0, deviceHistograms, &hist[0], 16).Wait();

        // Scatter keys
        scatterKeysAndVals.SetArg(0, offset);
        scatterKeysAndVals.SetArg(1, *fromKeys);
        scatterKeysAndVals.SetArg(2, *fromVals);
        scatterKeysAndVals.SetArg(3, numElems);
        scatterKeysAndVals.SetArg(4, deviceHistograms);
        scatterKeysAndVals.SetArg(5, *toKeys);
        scatterKeysAndVals.SetArg(6, *toVals);

        event = context_.Launch1D(0, NUM_BLOCKS*WG_SIZE, WG_SIZE, scatterKeysAndVals);

        //context_.ReadBuffer(0, *toKeys, &keys[0], 64).Wait();

        if (offset == 0)
        {
            fromKeys = &outputKeys;
            fromVals = &outputValues;
        }

        // Swap pointers
        std::swap(fromKeys, toKeys);
        std::swap(fromVals, toVals);
    }

    // Return buffers to memory manager
    ReclaimTempIntBuffer(deviceHistograms);
    ReclaimTempCharBuffer(deviceTempKeysBuffer);
    ReclaimTempCharBuffer(deviceTempValsBuffer);

    return event;
}


CLWEvent CLWParallelPrimitives::SortRadix(unsigned int deviceIdx, CLWBuffer<cl_int> inputKeys, CLWBuffer<cl_int> outputKeys)
{
    assert(inputKeys.GetElementCount() == outputKeys.GetElementCount());
    
    cl_uint numElems = (cl_uint)inputKeys.GetElementCount();
    
    int GROUP_BLOCK_SIZE = (WG_SIZE * 4 * 8);
    int NUM_BLOCKS =  (numElems + GROUP_BLOCK_SIZE - 1) / GROUP_BLOCK_SIZE;
    
    auto deviceHistograms = GetTempIntBuffer(NUM_BLOCKS * 16);
    auto deviceTempKeys = GetTempIntBuffer(numElems);
    
    auto fromKeys = &inputKeys;
    auto toKeys = &deviceTempKeys;
    
    CLWKernel histogramKernel = program_.GetKernel("BitHistogram");
    CLWKernel scatterKeys = program_.GetKernel("ScatterKeys");

    CLWEvent event;

    for (int offset = 0; offset < 32; offset += 4)
    {
        // Split
        histogramKernel.SetArg(0, offset);
        histogramKernel.SetArg(1, *fromKeys);
        histogramKernel.SetArg(2, numElems);
        histogramKernel.SetArg(3, deviceHistograms);

        context_.Launch1D(0, NUM_BLOCKS*WG_SIZE, WG_SIZE, histogramKernel);

        // Scan histograms
        ScanExclusiveAdd(0, deviceHistograms, deviceHistograms, deviceHistograms.GetElementCount());

        // Scatter keys
        scatterKeys.SetArg(0, offset);
        scatterKeys.SetArg(1, *fromKeys);
        scatterKeys.SetArg(2, (cl_uint)toKeys->GetElementCount());
        scatterKeys.SetArg(3, deviceHistograms);
        scatterKeys.SetArg(4, *toKeys);

        event = context_.Launch1D(0, NUM_BLOCKS*WG_SIZE, WG_SIZE, scatterKeys);

        if (offset == 0)
        {
            fromKeys = &outputKeys;
        }

        // Swap pointers
        std::swap(fromKeys, toKeys);
    }

    // Return buffers to memory manager
    ReclaimTempIntBuffer(deviceHistograms);
    ReclaimTempIntBuffer(deviceTempKeys);

    // Return last copy event back to the user
    return event;
}

void CLWParallelPrimitives::ReclaimDeviceMemory()
{
    intBufferCache_.clear();
    floatBufferCache_.clear();
}

void CLWParallelPrimitives::ReclaimTempIntBuffer(CLWBuffer<cl_int> buffer)
{
    auto iter = intBufferCache_.find(buffer.GetElementCount());

    if (iter != intBufferCache_.end())
    {
        return;
    }

    intBufferCache_[buffer.GetElementCount()] = buffer;
}

void CLWParallelPrimitives::ReclaimTempCharBuffer(CLWBuffer<char> buffer)
{
    auto iter = charBufferCache_.find(buffer.GetElementCount());

    if (iter != charBufferCache_.end())
    {
        return;
    }

    charBufferCache_[buffer.GetElementCount()] = buffer;
}

void CLWParallelPrimitives::ReclaimTempFloatBuffer(CLWBuffer<cl_float> buffer)
{
    auto iter = floatBufferCache_.find(buffer.GetElementCount());

    if (iter != floatBufferCache_.end())
    {
        return;
    }

    floatBufferCache_[buffer.GetElementCount()] = buffer;
}

CLWEvent CLWParallelPrimitives::Compact(unsigned int deviceIdx, CLWBuffer<cl_int> predicate, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems, cl_int& newSize)
{
    /// Scan predicate array first to temp buffer
    CLWBuffer<cl_int> addresses = GetTempIntBuffer(numElems);

    ScanExclusiveAdd(deviceIdx, predicate, addresses, numElems).Wait();

    /// New size = scanned[last] + perdicate[last]
    /// TODO: remove this sync
    context_.ReadBuffer(deviceIdx, addresses, &newSize, numElems - 1, 1).Wait();

    cl_int lastPredicate = 0;
    context_.ReadBuffer(deviceIdx, predicate, &lastPredicate, numElems - 1, 1).Wait();
    newSize += lastPredicate;

    int NUM_BLOCKS =  (int)((numElems + WG_SIZE - 1)/ WG_SIZE);

    CLWKernel compactKernel = program_.GetKernel("compact_int");

    compactKernel.SetArg(0, predicate);
    compactKernel.SetArg(1, addresses);
    compactKernel.SetArg(2, input);
    compactKernel.SetArg(3, (cl_uint)numElems);
    compactKernel.SetArg(4, output);

    /// TODO: unsafe as it is used in the kernel, may have problems on DMA devices
    ReclaimTempIntBuffer(addresses);

    return context_.Launch1D(deviceIdx, NUM_BLOCKS * WG_SIZE, WG_SIZE, compactKernel);
}

CLWEvent CLWParallelPrimitives::Compact(unsigned int deviceIdx, CLWBuffer<cl_int> predicate, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems, CLWBuffer<cl_int> newSize)
{
    /// Scan predicate array first to temp buffer
    CLWBuffer<cl_int> addresses = GetTempIntBuffer(numElems);

    ScanExclusiveAdd(deviceIdx, predicate, addresses, numElems);

    int NUM_BLOCKS = (int)((numElems + WG_SIZE - 1) / WG_SIZE);

    CLWKernel compactKernel = program_.GetKernel("compact_int_1");

    compactKernel.SetArg(0, predicate);
    compactKernel.SetArg(1, addresses);
    compactKernel.SetArg(2, input);
    compactKernel.SetArg(3, (cl_uint)numElems);
    compactKernel.SetArg(4, output);
    compactKernel.SetArg(5, newSize);

    /// TODO: unsafe as it is used in the kernel, may have problems on DMA devices
    ReclaimTempIntBuffer(addresses);

    return context_.Launch1D(deviceIdx, NUM_BLOCKS * WG_SIZE, WG_SIZE, compactKernel);
}


CLWEvent CLWParallelPrimitives::Copy(unsigned int deviceIdx, CLWBuffer<cl_int> input, CLWBuffer<cl_int> output, int numElems)
{
    int ELEMS_PER_WI = 4;
    int GROUP_BLOCK_SIZE = (WG_SIZE * ELEMS_PER_WI);
    int NUM_BLOCKS =  (numElems + GROUP_BLOCK_SIZE - 1)/ GROUP_BLOCK_SIZE;

    CLWKernel copyKernel = program_.GetKernel("copy");

    copyKernel.SetArg(0, input);
    copyKernel.SetArg(1, numElems);
    copyKernel.SetArg(2, output);

    return context_.Launch1D(0, NUM_BLOCKS * WG_SIZE, WG_SIZE, copyKernel);
}
