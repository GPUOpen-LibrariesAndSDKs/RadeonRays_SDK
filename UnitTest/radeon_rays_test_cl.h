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
#ifndef RADEONRAYS_CL_TEST_H
#define RADEONRAYS_CL_TEST_H

#if USE_OPENCL

/// This test suite is testing RadeonRays OpenCL interoperability
///

#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <ctime>

#include "gtest/gtest.h"
#include "radeon_rays_cl.h"

using namespace RadeonRays;

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

// Api creation fixture, prepares api_ for further tests
class ApiCl : public ::testing::Test
{
public:
    void SetUp() override
    {
        cl_int status = CL_SUCCESS;
        cl_platform_id platform[2];
        cl_device_id device;
       
        api_ = nullptr;
        queue_ = nullptr;
        rawcontext_ = nullptr;

        // Create OpenCL stuff

        status = clGetPlatformIDs(1, platform, nullptr);
        ASSERT_EQ(status, CL_SUCCESS);
        
        cl_device_type type = CL_DEVICE_TYPE_ALL;
        
        // TODO: this is a workaround for nasty Apple's OpenCL runtime
        // which doesn't allow to have work group sizes > 1 on CPU devices
        // so disable useless CPU
#ifdef __APPLE__
        type = CL_DEVICE_TYPE_GPU;
#endif
        
        status = clGetDeviceIDs(platform[0], type, 1, &device, nullptr);
        ASSERT_EQ(status, CL_SUCCESS);
        
        rawcontext_ = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
        ASSERT_EQ(status, CL_SUCCESS);
        
        queue_ = clCreateCommandQueue(rawcontext_, device, 0, &status);
        ASSERT_EQ(status, CL_SUCCESS);
        
        //try
        //{
        api_ = nullptr;
        ASSERT_NO_THROW(api_ = RadeonRays::CreateFromOpenClContext(rawcontext_, device, queue_));
        //}
        //catch (Exception& e)
        //{
            //std::cout << e.what();
        //}
    }
    
    void TearDown() override
    {
        IntersectionApi::Delete(api_);
        clReleaseCommandQueue(queue_);
        clReleaseContext(rawcontext_);
    }
    
    // Platform
    cl_context rawcontext_;
    cl_command_queue queue_;
    // Api
    IntersectionApi* api_;
};

// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiCl, Intersection_1Ray)
{
    // Mesh vertices
    float vertices[] = {
        -1.f,-1.f,0.f,
        0.f,1.f,0.f,
        1.f,-1.f,0.f,
        
    };
    
    // Indices
    int indices[] = {0, 1, 2};
    // Number of vertices for the face
    int numfaceverts[] = { 3 };
    
    Shape* mesh = nullptr;
    
    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices, 3, 3*sizeof(float), indices, 0, numfaceverts, 1));
    
    ASSERT_TRUE(mesh != nullptr);
    
    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));
    
    // Prepare the ray
    ray r;
    r.o = float3(0.f,0.f,-10.f, 10000.f);
    r.d = float3(0.f,0.f,1.f, 0.f);
    
    // Intersection and hit data
    Intersection isect;
    
    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    Intersection* tmp = nullptr;
    Event* e = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e));

    e->Wait();
    api_->DeleteEvent(e);

    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e));

    e->Wait();
    api_->DeleteEvent(e);
    
    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());
    
    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}


// OpenCL buffer creation
TEST_F(ApiCl, ClBuffer)
{
    cl_int status = CL_SUCCESS;
    int kBufferSize = 100;
    
    // Create OpenCL buffer
    cl_mem buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE, kBufferSize, nullptr, &status);
    
    ASSERT_EQ(status, CL_SUCCESS);
    
    Buffer* apibuffer = nullptr;
    
    ASSERT_NO_THROW(apibuffer = RadeonRays::CreateFromOpenClBuffer(api_,buffer));
    
    ASSERT_NO_THROW(api_->DeleteBuffer(apibuffer));

    status = clReleaseMemObject(buffer);

    ASSERT_EQ(status, CL_SUCCESS);
}

// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiCl, Intersection_1Ray_Buffer)
{
    // Mesh vertices
    float vertices[] = {
        -1.f,-1.f,0.f,
        0.f,1.f,0.f,
        1.f,-1.f,0.f,
        
    };
    
    // Indices
    int indices[] = {0, 1, 2};
    // Number of vertices for the face
    int numfaceverts[] = { 3 };
    
    Shape* mesh = nullptr;
    
    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices, 3, 3*sizeof(float), indices, 0, numfaceverts, 1));
    
    ASSERT_TRUE(mesh != nullptr);
    
    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));
    
    // Prepare the ray
    ray r;
    r.o = float3(0.f,0.f,-10.f, 10000.f);
    r.d = float3(0.f,0.f,1.f, 0.f);
    
    // Intersection and hit data
    Intersection isect;

    cl_int status = CL_SUCCESS;
    
    // Create OpenCL buffer
    cl_mem rays_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(ray), &r, &status);
    ASSERT_EQ(status, CL_SUCCESS);

    cl_mem hitinfos_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE, sizeof(Intersection), nullptr, &status);
    ASSERT_EQ(status, CL_SUCCESS);
    
    Buffer* rays = nullptr;
    Buffer* hitinfos = nullptr;
    
    // Create API objects
    ASSERT_NO_THROW(rays = CreateFromOpenClBuffer(api_,rays_buffer));
    ASSERT_NO_THROW(hitinfos = CreateFromOpenClBuffer(api_,hitinfos_buffer));
    
    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());

    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(rays, 1, hitinfos, nullptr, nullptr ));
    
    // Read data back to memory
    status = clEnqueueReadBuffer(queue_, hitinfos_buffer, CL_TRUE, 0, sizeof(Intersection), &isect, 0, nullptr, nullptr);
    ASSERT_EQ(status, CL_SUCCESS);
    
    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());
    
    // Bail out
    ASSERT_NO_THROW(api_->DeleteBuffer(rays));
    ASSERT_NO_THROW(api_->DeleteBuffer(hitinfos));
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));

    status = clReleaseMemObject(rays_buffer);
    ASSERT_EQ(status, CL_SUCCESS);

    status = clReleaseMemObject(hitinfos_buffer);
    ASSERT_EQ(status, CL_SUCCESS);
}


// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiCl, Intersection_3Rays_Buffer)
{
    // Mesh vertices
    float vertices[] = {
        -1.f,-1.f,0.f,
        0.f,1.f,0.f,
        1.f,-1.f,0.f,
    };
    
    // Indices
    int indices[] = {0, 1, 2};
    // Number of vertices for the face
    int numfaceverts[] = { 3 };
    
    Shape* mesh = nullptr;
    
    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices, 3, 3*sizeof(float), indices, 0, numfaceverts, 1));
    
    ASSERT_TRUE(mesh != nullptr);
    
    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));
    
    // Prepare the ray
    ray r[3];
    r[0].o = float3(0.f,0.0f,-10.f, 10000.f);
    r[0].d = float3(0.f,0.f,1.f, 0.f);
    
    r[1].o = float3(0.f,0.01f,-10.f, 10000.f);
    r[1].d = float3(0.f,0.f,1.f, 0.f);
    
    r[2].o = float3(0.01f,0.0f,-10.f, 10000.f);
    r[2].d = float3(0.f,0.f,1.f, 0.f);
    
    // Intersection and hit data
    Intersection isect[3];
    
    cl_int status = CL_SUCCESS;
    
    // Create OpenCL buffer
    cl_mem rays_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 3*sizeof(ray), r, &status);
    ASSERT_EQ(status, CL_SUCCESS);

    cl_mem hitinfos_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE, 3*sizeof(Intersection), nullptr, &status);
    ASSERT_EQ(status, CL_SUCCESS);
    
    Buffer* rays = nullptr;
    Buffer* hitinfos = nullptr;
    
    // Create API objects
    ASSERT_NO_THROW(rays = CreateFromOpenClBuffer(api_,rays_buffer));
    ASSERT_NO_THROW(hitinfos = CreateFromOpenClBuffer(api_, hitinfos_buffer));
    
    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(rays, 3, hitinfos, nullptr, nullptr));
    
    // Read data back to memory
    status = clEnqueueReadBuffer(queue_, hitinfos_buffer, CL_TRUE, 0, 3*sizeof(Intersection), isect, 0, nullptr, nullptr);
    ASSERT_EQ(status, CL_SUCCESS);
    
    // Check results
    for (int i=0; i<3; ++i)
    {
        ASSERT_EQ(isect[i].shapeid, mesh->GetId());
    }
    
    // Bail out
    ASSERT_NO_THROW(api_->DeleteBuffer(rays));
    ASSERT_NO_THROW(api_->DeleteBuffer(hitinfos));
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));

    status = clReleaseMemObject(rays_buffer);
    ASSERT_EQ(status, CL_SUCCESS);

    status = clReleaseMemObject(hitinfos_buffer);
    ASSERT_EQ(status, CL_SUCCESS);
}

// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiCl, Intersection_3Rays_Buffer_Indirect)
{
    // Mesh vertices
    float vertices[] = {
        -1.f,-1.f,0.f,
        0.f,1.f,0.f,
        1.f,-1.f,0.f,
        
    };
    
    // Indices
    int indices[] = {0, 1, 2};
    // Number of vertices for the face
    int numfaceverts[] = { 3 };
    
    Shape* mesh = nullptr;
    
    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices, 3, 3*sizeof(float), indices, 0, numfaceverts, 1));
    
    ASSERT_TRUE(mesh != nullptr);
    
    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));
    
    // Prepare the ray
    ray r[3];
    r[0].o = float3(0.f,0.0f,-10.f, 10000.f);
    r[0].d = float3(0.f,0.f,1.f);
    
    r[1].o = float3(0.f,0.01f,-10.f, 10000.f);
    r[1].d = float3(0.f,0.f,1.f);
    
    r[2].o = float3(0.01f,0.0f,-10.f, 10000.f);
    r[2].d = float3(0.f,0.f,1.f);

    // Intersection and hit data
    Intersection isect[3];

    cl_int status = CL_SUCCESS;

    // Create OpenCL buffer
    cl_mem rays_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 3*sizeof(ray), r, &status);
    ASSERT_EQ(status, CL_SUCCESS);

    cl_mem hitinfos_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE, 3*sizeof(Intersection), nullptr, &status);
    ASSERT_EQ(status, CL_SUCCESS);

    int nr = 3;
    cl_mem numrays_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(int), &nr, &status);
    ASSERT_EQ(status, CL_SUCCESS);

    Buffer* rays = nullptr;
    Buffer* hitinfos = nullptr;
    Buffer* numrays = nullptr;

    // Create API objects
    ASSERT_NO_THROW(rays = CreateFromOpenClBuffer(api_, rays_buffer));
    ASSERT_NO_THROW(hitinfos = CreateFromOpenClBuffer(api_, hitinfos_buffer));
    ASSERT_NO_THROW(numrays = CreateFromOpenClBuffer(api_, numrays_buffer));

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(rays, numrays, 3, hitinfos, nullptr, nullptr));
    
    // Read data back to memory
    status = clEnqueueReadBuffer(queue_, hitinfos_buffer, CL_TRUE, 0, 3*sizeof(Intersection), isect, 0, nullptr, nullptr);
    ASSERT_EQ(status, CL_SUCCESS);
    
    // Check results
    for (int i=0; i<3; ++i)
    {
        ASSERT_EQ(isect[i].shapeid, mesh->GetId());
    }
    
    // Bail out
    ASSERT_NO_THROW(api_->DeleteBuffer(rays));
    ASSERT_NO_THROW(api_->DeleteBuffer(hitinfos));
    ASSERT_NO_THROW(api_->DeleteBuffer(numrays));
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));

    status = clReleaseMemObject(rays_buffer);
    ASSERT_EQ(status, CL_SUCCESS);

    status = clReleaseMemObject(hitinfos_buffer);
    ASSERT_EQ(status, CL_SUCCESS);

    status = clReleaseMemObject(numrays_buffer);
    ASSERT_EQ(status, CL_SUCCESS);
}


// Test if events are working correctly
TEST_F(ApiCl, Intersection_Events)
{
    // Mesh vertices
    float vertices[] = {
        -1.f,-1.f,0.f,
        0.f,1.f,0.f,
        1.f,-1.f,0.f,
        
    };
    
    // Indices
    int indices[] = {0, 1, 2};
    // Number of vertices for the face
    int numfaceverts[] = { 3 };
    
    Shape* mesh = nullptr;
    
    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices, 3, 3*sizeof(float), indices, 0, numfaceverts, 1));
    
    ASSERT_TRUE(mesh != nullptr);
    
    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));
    
    // Prepare the ray
    ray r;
    r.o = float3(0.f,0.f,-10.f, 10000.f);
    r.d = float3(0.f,0.f,1.f);

    // Intersection and hit data
    Intersection isect;

    cl_int status = CL_SUCCESS;

    // Create OpenCL buffer
    cl_mem rays_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(ray), &r, &status);
    ASSERT_EQ(status, CL_SUCCESS);

    cl_mem hitinfos_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE, sizeof(Intersection), nullptr, &status);
    ASSERT_EQ(status, CL_SUCCESS);
    
    Buffer* rays = nullptr;
    Buffer* hitinfos = nullptr;
    
    // Create API objects
    ASSERT_NO_THROW(rays = CreateFromOpenClBuffer(api_, rays_buffer));
    ASSERT_NO_THROW(hitinfos = CreateFromOpenClBuffer(api_, hitinfos_buffer));
    
    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());

    Event* event = nullptr;
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(rays, 1, hitinfos, nullptr, &event));

    // Check if event is returned
    ASSERT_NE(event, nullptr);

    // Check if calls are possible
    ASSERT_NO_THROW(event->Complete());
    ASSERT_NO_THROW(event->Wait());
    
    // Read data back to memory
    status = clEnqueueReadBuffer(queue_, hitinfos_buffer, CL_TRUE, 0, sizeof(Intersection), &isect, 0, nullptr, nullptr);
    ASSERT_EQ(status, CL_SUCCESS);
    
    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());
    
    // Bail out
    ASSERT_NO_THROW(api_->DeleteEvent(event));
    ASSERT_NO_THROW(api_->DeleteBuffer(rays));
    ASSERT_NO_THROW(api_->DeleteBuffer(hitinfos));
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));

    status = clReleaseMemObject(rays_buffer);
    ASSERT_EQ(status, CL_SUCCESS);

    status = clReleaseMemObject(hitinfos_buffer);
    ASSERT_EQ(status, CL_SUCCESS);
}


// Test dependency by means of events
TEST_F(ApiCl, Intersection_DependencyEvents)
{
    // Mesh vertices
    float vertices[] = {
        -1.f,-1.f,0.f,
        0.f,1.f,0.f,
        1.f,-1.f,0.f,
        
    };
    
    // Indices
    int indices[] = {0, 1, 2};
    // Number of vertices for the face
    int numfaceverts[] = { 3 };
    
    Shape* mesh = nullptr;
    
    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices, 3, 3*sizeof(float), indices, 0, numfaceverts, 1));
    
    ASSERT_TRUE(mesh != nullptr);
    
    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));
    
    // Prepare the ray
    ray r;
    r.o = float3(0.f,0.f,-10.f,10000.f);
    r.d = float3(0.f,0.f,1.f);

    // Intersection and hit data
    Intersection isect;
    cl_int status = CL_SUCCESS;

    // Create OpenCL buffer
    cl_mem rays_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(ray), &r, &status);
    ASSERT_EQ(status, CL_SUCCESS);

    cl_mem hitinfos_buffer = clCreateBuffer(rawcontext_, CL_MEM_READ_WRITE, sizeof(Intersection), nullptr, &status);
    ASSERT_EQ(status, CL_SUCCESS);
    
    Buffer* rays = nullptr;
    Buffer* hitinfos = nullptr;
    
    // Create API objects
    ASSERT_NO_THROW(rays = CreateFromOpenClBuffer(api_, rays_buffer));
    ASSERT_NO_THROW(hitinfos = CreateFromOpenClBuffer(api_, hitinfos_buffer));
    
    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());

    Event* event = nullptr;
    Event* event1 = nullptr;
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(rays, 1, hitinfos, nullptr, &event));

    // Check if event is returned
    ASSERT_NE(event, nullptr);

    // Check if calls are possible
    ASSERT_NO_THROW(event->Complete());

    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(rays, 1, hitinfos, event, &event1));

    // Check if calls are possible
    ASSERT_NO_THROW(event1->Complete());
    ASSERT_NO_THROW(event1->Wait());

    // Read data back to memory
    status = clEnqueueReadBuffer(queue_, hitinfos_buffer, CL_TRUE, 0, sizeof(Intersection), &isect, 0, nullptr, nullptr);
    ASSERT_EQ(status, CL_SUCCESS);
    
    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());
    
    // Bail out
    ASSERT_NO_THROW(api_->DeleteEvent(event));
    ASSERT_NO_THROW(api_->DeleteEvent(event1));
    ASSERT_NO_THROW(api_->DeleteBuffer(rays));
    ASSERT_NO_THROW(api_->DeleteBuffer(hitinfos));
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));

    status = clReleaseMemObject(rays_buffer);
    ASSERT_EQ(status, CL_SUCCESS);

    status = clReleaseMemObject(hitinfos_buffer);
    ASSERT_EQ(status, CL_SUCCESS);
}

#endif // USE_OPENCL

#endif // RadeonRays_CL_TEST_H
