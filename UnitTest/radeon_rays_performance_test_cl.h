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
#pragma once

#if USE_OPENCL
/// This test suite is testing RadeonRays performance
///
#include "gtest/gtest.h"
#include "radeon_rays_cl.h"

using namespace RadeonRays;

#include "tiny_obj_loader.h"

using namespace tinyobj;

#include <vector>
#include <cstdio>
#include <chrono>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

// Api creation fixture, prepares api_ for further tests
class ApiPerformance : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        cl_int status = CL_SUCCESS;
        cl_platform_id platform;
        cl_device_id device;
        
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

        ASSERT_NO_THROW(api_ = IntersectionApiCL::CreateFromOpenClContext(rawcontext_, device, queue_));

        // Load obj file 
        std::string res = LoadObj(shapes_, materials_, "../Resources/bmw/i8.obj");

        // Create meshes within IntersectionApi
        for  (int i=0; i<(int)shapes_.size(); ++i)
        {
            Shape* shape = nullptr;

            ASSERT_NO_THROW(shape = api_->CreateMesh(&shapes_[i].mesh.positions[0], (int)shapes_[i].mesh.positions.size(), 3*sizeof(float),
                &shapes_[i].mesh.indices[0], 0, nullptr, (int)shapes_[i].mesh.indices.size() / 3));

            ASSERT_NO_THROW(api_->AttachShape(shape));

            apishapes_.push_back(shape);
        }
    }

    virtual void TearDown()
    {
        // Delete meshes
        for (int i=0; i<(int)apishapes_.size(); ++i)
        {
            ASSERT_NO_THROW(api_->DeleteShape(apishapes_[i]));
        }

        IntersectionApi::Delete(api_);
        clReleaseCommandQueue(queue_);
        clReleaseContext(rawcontext_);
    }

    // Platform
    cl_context rawcontext_;
    cl_command_queue queue_;

    // Api
    IntersectionApi* api_;

    std::vector<Shape*> apishapes_;

    // Tinyobj data
    std::vector<shape_t> shapes_;
    std::vector<material_t> materials_;
};

TEST_F(ApiPerformance, BvhBuild)
{
    api_->SetOption("acc.type", "bvh");
    api_->SetOption("bvh.builder", "sah");
    
    auto start = std::chrono::high_resolution_clock::now();
    api_->Commit();
    auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();

    std::cout << "Bvh build time: " << delta << " ms\n";
}

#endif // USE_OPENCL
