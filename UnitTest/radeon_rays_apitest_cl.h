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
/// This test suite is testing RadeonRays library functionality
///

#include "gtest/gtest.h"
#include "radeon_rays.h"
#include "math/quaternion.h"
#include "tiny_obj_loader.h"
#include "utils.h"

using namespace RadeonRays;


// Api creation fixture, prepares api_ for further tests
class ApiBackendOpenCL : public ::testing::Test
{
public:
    void SetUp() override
    {
        api_ = nullptr;
        int nativeidx = -1;

        // Always use OpenCL
        IntersectionApi::SetPlatform(DeviceInfo::kOpenCL);

        for (auto idx=0U; idx < IntersectionApi::GetDeviceCount(); ++idx)
        {
            DeviceInfo devinfo;
            IntersectionApi::GetDeviceInfo(idx, devinfo);

            if (devinfo.type == DeviceInfo::kGpu && nativeidx == -1)
            {
                nativeidx = idx;
            }
        }

        ASSERT_NE(nativeidx, -1);

        api_ = IntersectionApi::Create(nativeidx);
    }

    void TearDown() override
    {
        if(api_ != nullptr) IntersectionApi::Delete(api_);
    }

    void Wait()
    {
        e_->Wait();
        api_->DeleteEvent(e_);
    }

    void Perform_1Ray_Masked_Test();

    IntersectionApi* api_;
    Event* e_;

    static float const * vertices() {
        static float const vertices[] = {
            -1.f,-1.f,0.f,
            0.f,1.f,0.f,
            1.f,-1.f,0.f,

        };
        return vertices;
    }
    static int const * indices() {
        static int const indices[] = { 0, 1, 2 };
        return indices;
    }

    static int const * numfaceverts() {
        static const int numfaceverts[] = { 3 };
        return numfaceverts;
    }
};


// The test checks whether the api has been successfully created
TEST_F(ApiBackendOpenCL, DeviceEnum)
{
    int numdevices = 0;
    ASSERT_NO_THROW(numdevices = IntersectionApi::GetDeviceCount());
    ASSERT_GT(numdevices, 0);

    for (int i=0; i<numdevices; ++i)
    {
        DeviceInfo devinfo;
        IntersectionApi::GetDeviceInfo(i, devinfo);

        ASSERT_NE(devinfo.name, nullptr);
        ASSERT_NE(devinfo.vendor, nullptr);
    }
}

// The test checks whether the api has been successfully created
TEST_F(ApiBackendOpenCL, SingleDevice)
{
    ASSERT_TRUE(api_ != nullptr);
}

// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, Mesh)
{
    Shape* shape = nullptr;

    ASSERT_NO_THROW(shape = api_->CreateMesh(vertices(), 3, 3*sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(shape != nullptr);

    ASSERT_NO_THROW(api_->AttachShape(shape));
    ASSERT_NO_THROW(api_->DetachShape(shape));
    ASSERT_NO_THROW(api_->DeleteShape(shape));
}

// The test creates an empty scene
TEST_F(ApiBackendOpenCL, EmptyScene)
{
    ASSERT_THROW(api_->Commit(), Exception);
}

// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, MeshStrided)
{
    struct Vertex
    {
        float position[3];
        float normal[3];
        float uv[2];
    };

    // Mesh vertices
    Vertex vertices[] = {
        { 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f },
        { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f },
        { 0.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f },
        { 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f },
        { 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f }
    };

    // Indices
    int mindices[] = { 0, 1, 2, 0, 0, 1, 2, 0};

    Shape* shape = nullptr;

    ASSERT_NO_THROW(shape = api_->CreateMesh((float*)vertices, 6, sizeof(Vertex), mindices, 4*sizeof(int), nullptr, 2));

    ASSERT_TRUE(shape != nullptr);

    ASSERT_NO_THROW(api_->AttachShape(shape));
    ASSERT_NO_THROW(api_->DetachShape(shape));
    ASSERT_NO_THROW(api_->DeleteShape(shape));
}



//The test creates a single triangle mesh and then tries to create an instance of the mesh
TEST_F(ApiBackendOpenCL, Instance)
{    
    Shape* shape = nullptr;
    
    ASSERT_NO_THROW(shape = api_->CreateMesh(vertices(), 3, 3*sizeof(float), indices(),0, numfaceverts(), 1));
    
    ASSERT_TRUE(shape != nullptr);
    
    ASSERT_NO_THROW(api_->AttachShape(shape));
    ASSERT_NO_THROW(api_->DetachShape(shape));
    
    Shape* instance = nullptr;
    
    ASSERT_NO_THROW(instance = api_->CreateInstance(shape));
    
    ASSERT_TRUE(instance != nullptr);
    
    ASSERT_NO_THROW(api_->DeleteShape(shape));
}

// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, Intersection_1Ray)
{

    Shape* mesh = nullptr;

    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices(), 3, 3*sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(mesh != nullptr);

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));

    // Prepare the ray
    ray r(float3(0.f, 0.f, -10.f), float3(0.f, 0.f, 1.f), 10000.f);

    // Intersection and hit data
    Intersection isect;

    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);
    
    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));

    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();

    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_)); 
    Wait();
    
    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());

    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}


#ifdef RR_RAY_MASK
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, Intersection_1Ray_Masked_2level)
#else
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, DISABLED_Intersection_1Ray_Masked_2level)
#endif
{

    api_->SetOption("acc.type", "bvh");
    api_->SetOption("bvh.force2level", 1.f);

    Perform_1Ray_Masked_Test();

}


#ifdef RR_RAY_MASK
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, Intersection_1Ray_Masked_bvh)
#else
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, DISABLED_Intersection_1Ray_Masked_bvh)
#endif
{

    api_->SetOption("acc.type", "bvh");

    Perform_1Ray_Masked_Test();

}


#ifdef RR_RAY_MASK
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, Intersection_1Ray_Masked_fatbvh)
#else
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, DISABLED_Intersection_1Ray_Masked_fatbvh)
#endif
{

    api_->SetOption("acc.type", "fatbvh");

    Perform_1Ray_Masked_Test();

}


#ifdef RR_RAY_MASK
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, DISABLED_Intersection_1Ray_Masked_hlbvh)
#else
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, DISABLED_Intersection_1Ray_Masked_hlbvh)
#endif
{

    api_->SetOption("acc.type", "hlbvh");

    Perform_1Ray_Masked_Test();

}


#ifdef RR_RAY_MASK
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, DISABLED_Intersection_1Ray_Masked_hashbvh)
#else
// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, DISABLED_Intersection_1Ray_Masked_hashbvh)
#endif
{

    api_->SetOption("acc.type", "hashbvh");

    Perform_1Ray_Masked_Test();

}

#ifdef RR_BACKFACE_CULL
// The test creates a single triangle mesh and tests backface culling functionality
TEST_F(ApiBackendOpenCL, Intersection_1Ray_Backface_Culling)
#else
// The test creates a single triangle mesh and tests backface culling functionality
TEST_F(ApiBackendOpenCL, DISABLED_Intersection_1Ray_Backface_Culling)
#endif
{

    Shape* mesh = nullptr;

    api_->SetOption("acc.type", "bvh");

    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices(), 3, 3 * sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(mesh != nullptr);

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));

    // Prepare the ray
    ray r(float3(0.f, 0.f, -10.f), float3(0.f, 0.f, 1.f), 10000.f);
    r.SetDoBackfaceCulling(true);

    // Intersection and hit data
    Intersection isect;

    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);
    auto isect_flag_buffer = api_->CreateBuffer(sizeof(int), nullptr);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());

    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr));

    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();

    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());

    r = ray(float3(0.f, 0.f, 10.f), float3(0.f, 0.f, -1.f), 10000.f);
    r.SetDoBackfaceCulling(true);

    // Update ray buffer
    ray* rr = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(ray_buffer, kMapWrite, 0, sizeof(ray), (void**)&rr, &e_));
    Wait();
    *rr = r;
    ASSERT_NO_THROW(api_->UnmapBuffer(ray_buffer, rr, &e_));
    Wait();

    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr));

    tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();

    // Check results
    ASSERT_EQ(isect.shapeid, kNullId);


    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_flag_buffer));

}


// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, Intersection_1Ray_Active)
{
    Shape* mesh = nullptr;

    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices(), 3, 3 * sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(mesh != nullptr);

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));

    // Prepare the ray
    ray r(float3(0.f, 0.f, -10.f), float3(0.f, 0.f, 1.f), 10000.f);

    // Intersection and hit data
    Intersection isect;
    
    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());

    isect.primid = kNullId;
    isect.shapeid = kNullId;

    r.SetActive(false);
    
    ray* rr = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(ray_buffer, kMapWrite, 0, sizeof(ray), (void**)&rr, &e_));
    Wait();
    *rr = r;
    ASSERT_NO_THROW(api_->UnmapBuffer(ray_buffer, rr, &e_));
    Wait();
    
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapWrite, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    *tmp = isect;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();

    
    // Check results
    ASSERT_EQ(isect.shapeid, kNullId);


    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}

// The test creates a single triangle mesh and tests attach/detach functionality
TEST_F(ApiBackendOpenCL, Intersection_3Rays)
{

    Shape* mesh = nullptr;

    // 
    ASSERT_NO_THROW(api_->SetOption("acc.type", "grid"));

    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices(), 3, 3*sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(mesh != nullptr);

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));

    // Rays
    ray rays[3];

    // Prepare the ray
    rays[0].o = float4(0.f,0.f,-10.f, 1000.f);
    rays[0].d = float3(0.f,0.f,1.f);

    rays[1].o = float4(0.f,0.5f,-10.f, 1000.f);
    rays[1].d = float3(0.f,0.f,1.f);

    rays[2].o = float4(0.5f,0.f,-10.f, 1000.f);
    rays[2].d = float3(0.f,0.f,1.f);

    // Intersection and hit data
    Intersection isect[3];
    
    auto ray_buffer = api_->CreateBuffer(3*sizeof(ray), rays);
    auto isect_buffer = api_->CreateBuffer(3*sizeof(Intersection), nullptr);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 3, isect_buffer, nullptr, nullptr ));
    
    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, 3*sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect[0] = tmp[0];
    isect[1] = tmp[1];
    isect[2] = tmp[2];
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Check results
    for (int i=0; i<3; ++i)
    {
        ASSERT_EQ(isect[i].shapeid, mesh->GetId());
    }

    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}


// Test is checking if mesh transform is working as expected
TEST_F(ApiBackendOpenCL, Intersection_1Ray_Transformed)
{

    Shape* mesh = nullptr;

    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices(), 3, 3*sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(mesh != nullptr);

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));

    // Prepare the ray
    ray r;
    r.o = float4(0.f,0.f,-10.f, 1000.f);
    r.d = float3(0.f,0.f,1.f);

    // Intersection and hit data
    Intersection isect;
    
    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());

    matrix m = translation(float3(0,2,0));
    matrix minv = inverse(m);
    // Move the mesh
    ASSERT_NO_THROW(mesh->SetTransform(m, minv));
    // Reset ray

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Check results
    ASSERT_EQ(isect.shapeid, -1);

    // Set transform to identity
    m = matrix();
    ASSERT_NO_THROW(mesh->SetTransform(m, m));

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());

    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}

// The test checks intersection after geometry addition
TEST_F(ApiBackendOpenCL, Intersection_1Ray_DynamicGeo)
{
    // Mesh vertices
    float vertices[] = {
        -1.f,-1.f,0.f,
        0.f,1.f,0.f,
        1.f,-1.f,0.f,
        
    };
    
    float vertices1[] = {
        -1.f,-1.f,-1.f,
        0.f,1.f,-1.f,
        1.f,-1.f,-1.f,
        
    };

    Shape* closemesh = nullptr;
    Shape* farmesh = nullptr;

    // Create two meshes
    ASSERT_NO_THROW(farmesh = api_->CreateMesh(vertices, 3, 3*sizeof(float), indices(), 0, numfaceverts(), 1));
    ASSERT_NO_THROW(closemesh = api_->CreateMesh(vertices1, 3, 3*sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(farmesh != nullptr);
    ASSERT_TRUE(closemesh != nullptr);

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(farmesh));

    // Prepare the ray
    ray r;
    r.o = float4(0.f,0.f,-10.f, 1000.f);
    r.d = float3(0.f,0.f,1.f);


    // Intersection and hit data
    Intersection isect;
    
    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Check results
    ASSERT_EQ(isect.shapeid, farmesh->GetId());

    // Attach closer mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(closemesh));

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Check results
    ASSERT_EQ(isect.shapeid, closemesh->GetId());

    // Attach closer mesh to the scene
    ASSERT_NO_THROW(api_->DetachShape(closemesh));

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Check results
    ASSERT_EQ(isect.shapeid, farmesh->GetId());

    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(farmesh));
    ASSERT_NO_THROW(api_->DeleteShape(farmesh));
    ASSERT_NO_THROW(api_->DeleteShape(closemesh));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}

TEST_F(ApiBackendOpenCL, CornellBoxLoad)
{
    using namespace tinyobj;
    std::vector<shape_t> shapes;
    std::vector<material_t> materials;
    std::vector<Shape*> apishapes;

    // Load obj file 
    std::string res = LoadObj(shapes, materials, "../Resources/CornellBox/orig.objm");

    ASSERT_NO_THROW(api_->SetOption("acc.type", "grid"));

    // Create meshes within IntersectionApi
    for  (auto & tObjShape : shapes)
    {
        Shape* shape = nullptr;
        ASSERT_NO_THROW(shape = api_->CreateMesh(&tObjShape.mesh.positions[0], (int)tObjShape.mesh.positions.size() / 3, 3*sizeof(float),
            &tObjShape.mesh.indices[0], 0, nullptr, (int)tObjShape.mesh.indices.size() / 3));

        ASSERT_NO_THROW(api_->AttachShape(shape));
        apishapes.push_back(shape);
    }

    // Commit update
    ASSERT_NO_THROW(api_->Commit());

    // Delete meshes
    for (auto & apishape : apishapes)
    {
        ASSERT_NO_THROW(api_->DeleteShape(apishape));
    }
}

TEST_F(ApiBackendOpenCL, CornellBox_1Ray)
{
    using namespace tinyobj;
    std::vector<shape_t> shapes;
    std::vector<material_t> materials;
    std::vector<Shape*> apishapes;

    // Load obj file 
    std::string res = LoadObj(shapes, materials, "../Resources/CornellBox/orig.objm");

    //ASSERT_NO_THROW(api_->SetOption("acc.type", "grid"));

    // Create meshes within IntersectionApi
    for (auto & tObjShape : shapes)
    {
        Shape* shape = nullptr;
        ASSERT_NO_THROW(shape = api_->CreateMesh(&tObjShape.mesh.positions[0], (int)tObjShape.mesh.positions.size() / 3, 3 * sizeof(float),
            &tObjShape.mesh.indices[0], 0, nullptr, (int)tObjShape.mesh.indices.size() / 3));

        ASSERT_NO_THROW(api_->AttachShape(shape));
        apishapes.push_back(shape);
    }

    // Prepare the ray
    ray r;
    r.o = float4(0.f, 0.5f, -10.f, 1000.f);
    r.d = float3(0.f, 0.f, 1.f);


    // Intersection and hit data
    Intersection isect;
    
    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();


    // Delete meshes
    for (auto & apishape : apishapes)
    {
        ASSERT_NO_THROW(api_->DeleteShape(apishape));
    }

    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}


// Test is checking if mesh transform is working as expected
TEST_F(ApiBackendOpenCL, Intersection_1Ray_TransformedInstance1)
{
    // this test uses a single mesh, it added into the world as itself
    // at <0,-1,1000> AND as an instance at <0,0,2>
    // ray from <0,0,-10> along the pos z should hit the uninstanced mesh

    std::vector<TestShape> shapes = { TestShape(vertices(), 3, indices(), 3, numfaceverts(), 1),
                            TestShape(vertices(), 3, indices(), 3, numfaceverts(), 1), 
                            TestShape(vertices(), 3, indices(), 3, numfaceverts(), 1)};
    TestShape& mesh0 = shapes[0];
    TestShape& mesh1 = shapes[1];
    TestShape& instance = shapes[2];

    // Create meshes
    // NOTE mesh in world and as a instance upsets the simple TestIntersection API call 
    ASSERT_NO_THROW(mesh0.shape = api_->CreateMesh(vertices(), 3, 3 * sizeof(float), indices(), 0, numfaceverts(), 1));
    ASSERT_TRUE(mesh0.shape != nullptr);
    ASSERT_NO_THROW(mesh1.shape = api_->CreateMesh(vertices(), 3, 3 * sizeof(float), indices(), 0, numfaceverts(), 1));
    ASSERT_TRUE(mesh1.shape != nullptr);

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh0.shape));
    // Create instance of a triangle
    ASSERT_NO_THROW(instance.shape = api_->CreateInstance(mesh1.shape));

    matrix m = translation(float3(0, 0, 2));
    const matrix minv = inverse(m);
    ASSERT_NO_THROW(instance.shape->SetTransform(m, minv));

    ASSERT_NO_THROW(api_->AttachShape(instance.shape));

    ray r(float3(0.f, 0.f, -10.f), float3(0.f, 0.f, 1.f), 10000.f);

    // Intersection and hit data
    Intersection isect;

    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);


    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());

    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr));

    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();

    // Check results for 1st ray
    Intersection isect_brute;
    TestIntersections(shapes.data(), (int)shapes.size(), &r, 1, &isect_brute);
    // check the test gets the mesh we expect
    EXPECT_EQ(isect_brute.shapeid, mesh0.shape->GetId());
    // does the accelerated radeon rays match the test
    EXPECT_EQ(isect.shapeid, isect_brute.shapeid);
    EXPECT_LE(std::fabs(isect.uvwt.w - 10.f), 0.01f);

    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(instance.shape));
    ASSERT_NO_THROW(api_->DetachShape(mesh0.shape));
    ASSERT_NO_THROW(api_->DetachShape(mesh1.shape));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));

}
TEST_F(ApiBackendOpenCL, Intersection_1Ray_TransformedInstance2)
{
    // this test uses a single mesh, it added into the world as itself
    // at <0,-1,1000> AND as an instance at <0,0,-2>
    // ray from <0,0,-10> along the pos z should hit the instanced mesh

    std::vector<TestShape> shapes = { TestShape(vertices(), 3, indices(), 3, numfaceverts(), 1),
        TestShape(vertices(), 3, indices(), 3, numfaceverts(), 1),
        TestShape(vertices(), 3, indices(), 3, numfaceverts(), 1) };
    TestShape& mesh0 = shapes[0];
    TestShape& mesh1 = shapes[1];
    TestShape& instance = shapes[2];

    // Create meshes
    // NOTE mesh in world and as a instance upsets the simple TestIntersection API call 
    ASSERT_NO_THROW(mesh0.shape = api_->CreateMesh(vertices(), 3, 3 * sizeof(float), indices(), 0, numfaceverts(), 1));
    ASSERT_TRUE(mesh0.shape != nullptr);
    ASSERT_NO_THROW(mesh1.shape = api_->CreateMesh(vertices(), 3, 3 * sizeof(float), indices(), 0, numfaceverts(), 1));
    ASSERT_TRUE(mesh1.shape != nullptr);

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh0.shape));
    // Create instance of a triangle
    ASSERT_NO_THROW(instance.shape = api_->CreateInstance(mesh1.shape));

    //
    const matrix m = translation(float3(0, 0, -2));
    const matrix minv = inverse(m);
    ASSERT_NO_THROW(instance.shape->SetTransform(m, minv));

    // Prepare the ray
    ray r(float3(0.f, 0.f, -10.f), float3(0.f, 0.f, 1.f), 10000.f);


    // Commit geometry update
    EXPECT_NO_THROW(api_->Commit());

    ASSERT_NO_THROW(api_->AttachShape(instance.shape));

    // Intersection and hit data
    Intersection isect;

    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());

    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr));

    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();

    // Check results for 1st ray
    Intersection isect_brute;
    TestIntersections(shapes.data(), (int)shapes.size(), &r, 1, &isect_brute);
    // check the test gets the mesh we expect
    EXPECT_EQ(isect_brute.shapeid, instance.shape->GetId());
    // does the accelerated radeon rays match the test
    EXPECT_EQ(isect.shapeid, isect_brute.shapeid);
    EXPECT_LE(std::fabs(isect.uvwt.w - 8.f), 0.01f);

    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(instance.shape));
    ASSERT_NO_THROW(api_->DetachShape(mesh0.shape));
    ASSERT_NO_THROW(api_->DetachShape(mesh1.shape));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}


TEST_F(ApiBackendOpenCL, Intersection_1Ray_TransformedInstanceFlat)
{

    // Set flattening
    api_->SetOption("bvh.forceflat", 1.f);
    
    Shape* mesh = nullptr;

    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices(), 3, 3*sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(mesh != nullptr);

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));

    // Prepare the ray
    ray r;
    r.o = float3(0.f,0.f,-10.f, 1000.f);
    r.d = float3(0.f,0.f,1.f);

    // Intersection and hit data
    Intersection isect;
    
    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);
    
    // Create instance of a triangle
    Shape* instance = nullptr;
    ASSERT_NO_THROW(instance = api_->CreateInstance(mesh));
    
    matrix m = translation(float3(0,0,-2));
    matrix minv = inverse(m);
    ASSERT_NO_THROW(instance->SetTransform(m, minv));
    
    ASSERT_NO_THROW(api_->AttachShape(instance));
    
    // Prepare the ray
    r.o = float3(0.f,0.f,-10.f, 1000.f);
    r.d = float3(0.f,0.f,1.f);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();
    
    // Check results
    ASSERT_EQ(isect.shapeid, instance->GetId());
    ASSERT_LE(std::fabs(isect.uvwt.w - 8.f), 0.01f);
    
    //
    m = translation(float3(0,0,2));
    minv = inverse(m);
    ASSERT_NO_THROW(instance->SetTransform(m, minv));

    // Prepare the ray
    r.o = float3(0.f,0.f,-10.f, 1000.f);
    r.d = float3(0.f,0.f,1.f);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    
    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr ));
    
    tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();

    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());
    ASSERT_LE(std::fabs(isect.uvwt.w - 10.f), 0.01f);

    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(instance));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}
// Test is checking if mesh transform is working as expected
// DK: #22 repro case : Commit throws if base shape has not been attached
TEST_F(ApiBackendOpenCL, Intersection_1Ray_InstanceNoShape)
{

    Shape* mesh = nullptr;

    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices(), 3, 3 * sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(mesh != nullptr);

    // Attach the mesh to the scene
    //ASSERT_NO_THROW(api_->AttachShape(mesh));

    // Prepare the ray
    ray r;
    r.o = float3(0.f, 0.f, -10.f, 1000.f);
    r.d = float3(0.f, 0.f, 1.f);

    // Intersection and hit data
    Intersection isect;

    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);

    // Create instance of a triangle
    Shape* instance = nullptr;
    ASSERT_NO_THROW(instance = api_->CreateInstance(mesh));

    matrix m = translation(float3(0, 0, 2));
    matrix minv = inverse(m);
    ASSERT_NO_THROW(instance->SetTransform(m, minv));

    ASSERT_NO_THROW(api_->AttachShape(instance));

    // Prepare the ray
    r.o = float3(0.f, 0.f, -10.f, 1000.f);
    r.d = float3(0.f, 0.f, 1.f);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());

    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr));

    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();

    // Check results
    ASSERT_EQ(isect.shapeid, instance->GetId());
    ASSERT_LE(std::fabs(isect.uvwt.w - 12.f), 0.01f);

    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(instance));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
}

void ApiBackendOpenCL::Perform_1Ray_Masked_Test()
{
    Shape* mesh = nullptr;
    Shape* mesh2 = nullptr;

    // Create mesh
    ASSERT_NO_THROW(mesh = api_->CreateMesh(vertices(), 3, 3 * sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(mesh != nullptr);

    // Set mesh Id
    ASSERT_NO_THROW(mesh->SetId(0));

    // Attach the mesh to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh));

    float const vertices2[] = {
        -1.f,-1.f,1.f,
        0.f,1.f,1.f,
        1.f,-1.f,1.f,
    };

    ASSERT_NO_THROW(mesh2 = api_->CreateMesh(vertices2, 3, 3 * sizeof(float), indices(), 0, numfaceverts(), 1));

    ASSERT_TRUE(mesh2 != nullptr);

    // Set mesh Id
    ASSERT_NO_THROW(mesh2->SetId(10));

    // Attach mesh2 to the scene
    ASSERT_NO_THROW(api_->AttachShape(mesh2));

    // Prepare the ray
    ray r(float3(0.f, 0.f, -10.f), float3(0.f, 0.f, 1.f), 10000.f);
    r.SetMask(-1);

    // Intersection and hit data
    Intersection isect;

    auto ray_buffer = api_->CreateBuffer(sizeof(ray), &r);
    auto isect_buffer = api_->CreateBuffer(sizeof(Intersection), nullptr);
    auto isect_flag_buffer = api_->CreateBuffer(sizeof(int), nullptr);

    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());

    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr));

    Intersection* tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();

    // Check results
    ASSERT_EQ(isect.shapeid, mesh->GetId());

    r.SetMask(0);

    // Update ray buffer
    ray* rr = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(ray_buffer, kMapWrite, 0, sizeof(ray), (void**)&rr, &e_));
    Wait();
    *rr = r;
    ASSERT_NO_THROW(api_->UnmapBuffer(ray_buffer, rr, &e_));
    Wait();

    // Intersect
    ASSERT_NO_THROW(api_->QueryIntersection(ray_buffer, 1, isect_buffer, nullptr, nullptr));

    tmp = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_buffer, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
    Wait();
    isect = *tmp;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_buffer, tmp, &e_));
    Wait();

    // Check results
    ASSERT_EQ(isect.shapeid, mesh2->GetId());

    mesh->SetId(1);

    // Detach mesh2 from the scene
    ASSERT_NO_THROW(api_->DetachShape(mesh2));

    int result = kNullId;
    // Commit geometry update
    ASSERT_NO_THROW(api_->Commit());
    // Intersect
    ASSERT_NO_THROW(api_->QueryOcclusion(ray_buffer, 1, isect_flag_buffer, nullptr, nullptr));

    int* isect_flag = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_flag_buffer, kMapRead, 0, sizeof(int), (void**)&isect_flag, &e_));
    Wait();
    result = *isect_flag;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_flag_buffer, isect_flag, &e_));
    Wait();

    // Check results
    ASSERT_GT(result, 0);

    r.SetMask(1);

    // Update ray buffer
    rr = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(ray_buffer, kMapWrite, 0, sizeof(ray), (void**)&rr, &e_));
    Wait();
    *rr = r;
    ASSERT_NO_THROW(api_->UnmapBuffer(ray_buffer, rr, &e_));
    Wait();

    // Intersect
    ASSERT_NO_THROW(api_->QueryOcclusion(ray_buffer, 1, isect_flag_buffer, nullptr, nullptr));

    isect_flag = nullptr;
    ASSERT_NO_THROW(api_->MapBuffer(isect_flag_buffer, kMapRead, 0, sizeof(int), (void**)&isect_flag, &e_));
    Wait();
    result = *isect_flag;
    ASSERT_NO_THROW(api_->UnmapBuffer(isect_flag_buffer, isect_flag, &e_));
    Wait();
    // Check results
    ASSERT_EQ(result, kNullId);


    // Bail out
    ASSERT_NO_THROW(api_->DetachShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh));
    ASSERT_NO_THROW(api_->DeleteShape(mesh2));
    ASSERT_NO_THROW(api_->DeleteBuffer(ray_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_buffer));
    ASSERT_NO_THROW(api_->DeleteBuffer(isect_flag_buffer));
}

#endif // USE_OPENCL