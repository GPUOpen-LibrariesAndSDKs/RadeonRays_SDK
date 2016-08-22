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
#ifndef CLW_CL_TEST_H
#define CLW_CL_TEST_H


#if USE_OPENCL
/// This test suite is testing CLW library interop with pure OpenCL
///

#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <ctime>

#include "gtest/gtest.h"
#include "CLW.h"

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

// Api creation fixture, prepares api_ for further tests
class CLWCL : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        cl_int status = CL_SUCCESS;
        cl_platform_id platform;
        cl_device_id device;
        queue_ = nullptr;
        rawcontext_ = nullptr;

        // Create OpenCL stuff
        status = clGetPlatformIDs(1, &platform, nullptr);
        ASSERT_EQ(status, CL_SUCCESS);
        
        cl_device_type type = CL_DEVICE_TYPE_ALL;
        
        // TODO: this is a workaround for nasty Apple's OpenCL runtime
        // which doesn't allow to have work group sizes > 1 on CPU devices
        // so disable useless CPU
#ifdef __APPLE__
        type = CL_DEVICE_TYPE_GPU;
#endif
        
        status = clGetDeviceIDs(platform, type, 1, &device, nullptr);
        ASSERT_EQ(status, CL_SUCCESS);
        
        rawcontext_ = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
        ASSERT_EQ(status, CL_SUCCESS);
        
        queue_ = clCreateCommandQueue(rawcontext_, device, 0, &status);
        ASSERT_EQ(status, CL_SUCCESS);
        
        // Create CLW context out of CL
        ASSERT_NO_THROW(context_ = CLWContext::Create(rawcontext_, &device, &queue_, 1));
    }
    
    virtual void TearDown()
    {
        clReleaseCommandQueue(queue_);
        clReleaseContext(rawcontext_);
        queue_ = nullptr;
        rawcontext_ = nullptr;

    }
    
    // Platform
    CLWContext context_;
    cl_context rawcontext_;
    cl_command_queue queue_;
};

// Checks if the initial data is successfully read down the road
TEST_F(CLWCL, BufferRead)
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
TEST_F(CLWCL, BufferWrite)
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

#endif // use opengl

#endif // CLW_CL_TEST
