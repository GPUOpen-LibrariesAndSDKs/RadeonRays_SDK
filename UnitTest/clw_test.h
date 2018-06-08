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
#ifndef CLW_TEST_H
#define CLW_TEST_H

#if USE_OPENCL
/// This test suite is testing CLW library functionality
///

#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <ctime>

#include "gtest/gtest.h"

#include "CLW.h"

// Api creation fixture, prepares api_ for further tests
class CLW : public ::testing::Test
{
public:
    void SetUp() override
    {
        std::vector<CLWPlatform> platforms;

        // Create all available platforms
        CLWPlatform::CreateAllPlatforms(platforms);

        // Remove platforms that don't have any associated devices
        for (auto it = platforms.begin(); it != platforms.end();)
        {
            if (it->GetDeviceCount() == 0)
            {
                it = platforms.erase(it);
            }
            else
            {
                ++it;
            }
        }

        ThrowIf(platforms.empty(), CL_DEVICE_NOT_FOUND, "CLW::SetUp failed => no available OpenCL platforms with at least 1 device!");

        // Try to find AMD platform and set a flag if found
        bool       amdctx_ = false;
        for (auto & platform : platforms)
        {
            if (platform.GetVendor().find("AMD") != std::string::npos ||
                platform.GetVendor().find("Advanced Micro Devices") != std::string::npos)
            {
                context_ = CLWContext::Create(platform.GetDevice(0));
                amdctx_ = true;
            }
        }

        // Create some context if there is no AMD platform in the system
        if (!amdctx_)
        {
            context_ = CLWContext::Create(platforms[0].GetDevice(0));
        }
        
        std::string buildopts;
        
        buildopts_.append(" -cl-mad-enable -cl-fast-relaxed-math -cl-std=CL1.2 -I . ");
        
        buildopts_.append(
#if defined(__APPLE__)
                         "-D APPLE "
#elif defined(_WIN32) || defined (WIN32)
                         "-D WIN32 "
#elif defined(__linux__)
                         "-D __linux__ "
#else
                         ""
#endif
                         );
    }
    
    void TearDown() override
    {
    }
    
    // Platform
    CLWContext context_;
    // Is that AMD platform?
    bool amdctx_;
    //
    std::string buildopts_;
};

// The test checks buffer creation functionality
TEST_F(CLW, BufferCreate)
{
    int kBufferSize = 100;
    
    CLWBuffer<cl_int> buffer;
    
    ASSERT_NO_THROW(buffer = context_.CreateBuffer<cl_int>(kBufferSize, CL_MEM_READ_WRITE));
}

// Checks if the initial data is successfully read down the road
TEST_F(CLW, BufferRead)
{
    int kBufferSize = 100;
    
    CLWBuffer<cl_int> buffer;
    // Reference data
    std::vector<cl_int> initdata(kBufferSize);
    // Data from the buffer
    std::vector<cl_int> resultdata(kBufferSize);
    // Fill with an increasing sequence
    std::iota (initdata.begin(), initdata.end(), 0);
    
    // Create the buffer passing initial data
    ASSERT_NO_THROW(buffer = context_.CreateBuffer<cl_int>(kBufferSize, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &initdata[0]));
    // Read the buffer and wait for the result
    ASSERT_NO_THROW(context_.ReadBuffer(0, buffer, &resultdata[0], kBufferSize).Wait());
 
    // Check for the mismatch
    auto mismatch = std::mismatch(initdata.cbegin(), initdata.cend(),
                              resultdata.cbegin());

    ASSERT_EQ(mismatch.first, initdata.cend());
}

// Checks if the written data is successfully read down the road
TEST_F(CLW, BufferWrite)
{
    int kBufferSize = 100;
    
    CLWBuffer<cl_int> buffer;
    // Reference data
    std::vector<cl_int> initdata(kBufferSize);
    // Data from the buffer
    std::vector<cl_int> resultdata(kBufferSize);
    // Fill with an increasing sequence
    std::iota (initdata.begin(), initdata.end(), 0);
    
    // Create the buffer passing initial data
    ASSERT_NO_THROW(buffer = context_.CreateBuffer<cl_int>(kBufferSize, CL_MEM_READ_WRITE));
    // Enqueue write buffer
    ASSERT_NO_THROW(context_.WriteBuffer(0, buffer, &initdata[0], kBufferSize));
    // Read the buffer and wait for the result
    ASSERT_NO_THROW(context_.ReadBuffer(0, buffer, &resultdata[0], kBufferSize).Wait());
    
    // Check for the mismatch
    auto mismatch = std::mismatch(initdata.cbegin(), initdata.cend(),
                                  resultdata.cbegin());

    ASSERT_EQ(mismatch.first, initdata.cend());
}

// Checks for scan correctness
TEST_F(CLW, ExclusiveScanSmall)
{
    // Init rand
    std::srand((unsigned)std::time(nullptr));
    // Small array test
    int arraysize = 111;

    // Device buffers
    auto devinput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);
    auto devoutput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);

    // Host buffers
    std::vector<int> hostarray(arraysize);
    std::vector<int> hostarray_gold(arraysize);

    // Fill host buffer with data
    std::generate(hostarray.begin(), hostarray.end(), []{ return rand() % 1000; });

    // Perform gold scan
    int sum = 0;
    for (int i = 0; i < arraysize; ++i)
    {
        hostarray_gold[i] = sum;
        sum += hostarray[i];
    }

    // Send data to device
    context_.WriteBuffer(0, devinput, &hostarray[0], arraysize).Wait();

    // Create parallel prims object
    CLWParallelPrimitives prims(context_, buildopts_.c_str());

    // Perform scan
    prims.ScanExclusiveAdd(0, devinput, devoutput, arraysize).Wait();

    // Read data back to host
    context_.ReadBuffer(0, devoutput, &hostarray[0], arraysize).Wait();

    // Check correctness
    for (int i = 0; i < arraysize; ++i)
    {
        ASSERT_EQ(hostarray[i], hostarray_gold[i]);
    }
}

// Checks for scan correctness
TEST_F(CLW, ExclusiveScanSmallSizeDifference)
{
    // Init rand
    std::srand((unsigned)std::time(nullptr));
    // Small array test
    int arraysize = 4096;

    // Device buffers
    auto devinput = context_.CreateBuffer<cl_int>(250000, CL_MEM_READ_WRITE);
    auto devoutput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);

    // Host buffers
    std::vector<int> hostarray(arraysize);
    std::vector<int> hostarray_gold(arraysize);

    // Fill host buffer with data
    std::generate(hostarray.begin(), hostarray.end(), [] { return rand() % 1000; });

    // Perform gold scan
    int sum = 0;
    for (int i = 0; i < arraysize; ++i)
    {
        hostarray_gold[i] = sum;
        sum += hostarray[i];
    }

    // Send data to device
    context_.WriteBuffer(0, devinput, &hostarray[0], arraysize).Wait();

    // Create parallel prims object
    CLWParallelPrimitives prims(context_, buildopts_.c_str());

    // Perform scan
    prims.ScanExclusiveAdd(0, devinput, devoutput, arraysize).Wait();

    // Read data back to host
    context_.ReadBuffer(0, devoutput, &hostarray[0], arraysize).Wait();

    // Check correctness
    for (int i = 0; i < arraysize; ++i)
    {
        ASSERT_EQ(hostarray[i], hostarray_gold[i]);
    }
}

// Checks for scan correctness
TEST_F(CLW, ExclusiveScanLarge)
{
    // Init rand
    std::srand((unsigned)std::time(nullptr));
    // Large array test
    int arraysize = 13820049;

    // Device buffers
    auto devinput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);
    auto devoutput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);

    // Host buffers
    std::vector<int> hostarray(arraysize);
    std::vector<int> hostarray_gold(arraysize);

    // Fill host buffer with data
    std::generate(hostarray.begin(), hostarray.end(), []{ return rand() % 1000; });

    // Perform gold scan
    int sum = 0;
    for (int i = 0; i < arraysize; ++i)
    {
        hostarray_gold[i] = sum;
        sum += hostarray[i];
    }

    // Send data to device
    context_.WriteBuffer(0, devinput, &hostarray[0], arraysize).Wait();

    // Create parallel prims object
    CLWParallelPrimitives prims(context_, buildopts_.c_str());

    // Perform scan
    prims.ScanExclusiveAdd(0, devinput, devoutput, arraysize).Wait();

    // Read data back to host
    context_.ReadBuffer(0, devoutput, &hostarray[0], arraysize).Wait();

    // Check correctness
    for (int i = 0; i < arraysize; ++i)
    {
        ASSERT_EQ(hostarray[i], hostarray_gold[i]);
    }
}

// Checks for scan correctness
TEST_F(CLW, ExclusiveScanRandom)
{
    // Init rand
    std::srand((unsigned)std::time(nullptr));

    for (int i = 0; i < 10; ++i)
    {
        int arraysize = rand() % 100000;

        // Device buffers
        auto devinput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);
        auto devoutput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);

        // Host buffers
        std::vector<int> hostarray(arraysize);
        std::vector<int> hostarray_gold(arraysize);

        // Fill host buffer with data
        std::generate(hostarray.begin(), hostarray.end(), []{ return rand() % 1000; });

        // Perform gold scan
        int sum = 0;
        for (int i = 0; i < arraysize; ++i)
        {
            hostarray_gold[i] = sum;
            sum += hostarray[i];
        }

        // Send data to device
        context_.WriteBuffer(0, devinput, &hostarray[0], arraysize).Wait();

        // Create parallel prims object
        CLWParallelPrimitives prims(context_, buildopts_.c_str());

        // Perform scan
        prims.ScanExclusiveAdd(0, devinput, devoutput, arraysize).Wait();

        // Read data back to host
        context_.ReadBuffer(0, devoutput, &hostarray[0], arraysize).Wait();

        // Check correctness
        for (int i = 0; i < arraysize; ++i)
        {
            ASSERT_EQ(hostarray[i], hostarray_gold[i]);
        }
    }
}

// Compact test
TEST_F(CLW, CompactIdentity)
{
    // Init rand
    std::srand((unsigned)std::time(nullptr));
    // Small array test
    int arraysize = 480021;

    // Device buffers
    std::vector<int> predicate(arraysize);
    std::fill(predicate.begin(), predicate.end(), 1);

    auto devinput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);
    auto devoutput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);
    auto devpred = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE, &predicate[0]);

    // Host buffers
    std::vector<int> hostarray(arraysize);

    // Fill host buffer with data
    std::iota(hostarray.begin(), hostarray.end(), 0);

    // Send data to device
    context_.WriteBuffer(0, devinput, &hostarray[0], arraysize).Wait();

    // Create parallel prims object
    CLWParallelPrimitives prims(context_, buildopts_.c_str());

    int num;
    // Perform compact
    prims.Compact(0,  devpred, devinput, devoutput, arraysize, num);


    // Read data back to host
    context_.ReadBuffer(0, devoutput, &hostarray[0], arraysize).Wait();

    //
    ASSERT_EQ(num, arraysize);

    // Check correctness
    for (int i = 0; i < arraysize; ++i)
    {
        ASSERT_EQ(hostarray[i], i);
    }
}


// Checks for sort correctness
TEST_F(CLW, RadixSortLarge)
{
    // Init rand
    std::srand((unsigned)std::time(nullptr));
    // Large array test
    int arraysize = 64237846;

    // Device buffers
    auto devinput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);
    auto devoutput = context_.CreateBuffer<cl_int>(arraysize, CL_MEM_READ_WRITE);

    // Host buffers
    std::vector<int> hostarray(arraysize);

    // Fill host buffer with data
    std::generate(hostarray.begin(), hostarray.end(), []{ return rand() % 16; });

    // Send data to device
    context_.WriteBuffer(0, devinput, &hostarray[0], arraysize).Wait();

    // Create parallel prims object
    CLWParallelPrimitives prims(context_, buildopts_.c_str());

    // Perform scan
    prims.SortRadix(0, devinput, devoutput).Wait();

    // Read data back to host
    context_.ReadBuffer(0, devoutput, &hostarray[0], arraysize).Wait();

    // Check correctness
    for (int i = 0; i < arraysize - 1; ++i)
    {
        ASSERT_LE(hostarray[i], hostarray[i + 1]);
    }
}

#endif

#endif //USE_OPENCL
