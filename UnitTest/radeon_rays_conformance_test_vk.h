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

/// This test suite is testing RadeonRays GPU results to conform to CPU
///

#include "gtest/gtest.h"
#include "radeon_rays.h"

#if USE_VULKAN

using namespace RadeonRays;
using namespace tinyobj;

#include "tiny_obj_loader.h"
#include "utils.h"

#include <vector>
#include <cstdio>

// Api creation fixture, prepares api_ for further tests
class ApiConformanceVK : public ::testing::Test
{
public:
    static const int kMaxRaysTests = 10000;

    void SetUp() override;
    void TearDown() override;

    void Wait(IntersectionApi* api)
    {
        e_->Wait();
        api->DeleteEvent(e_);
    }

    void ExpectClosestIntersectionOk(const Intersection& expected, const Intersection& test) const;

    template< int kNumRays> void ExpectClosestRaysOk(RadeonRays::IntersectionApi* api) const;

    template< int kNumRays> void ExpectAnyRaysOk(RadeonRays::IntersectionApi* api) const;

    // GPU api
    IntersectionApi* apigpu_;

    std::vector<Shape*> apishapes_gpu_;
    std::vector<TestShape> test_shapes_;

    Event* e_;

    // Tinyobj data
    std::vector<shape_t> shapes_;
    std::vector<material_t> materials_;

};

inline void ApiConformanceVK::SetUp()
{
    apigpu_ = nullptr;

    // TODO make conformance tests across multiple backends and devices
    IntersectionApi::SetPlatform(DeviceInfo::kVulkan);

    //Search for native CPU
    int gpuidx = -1;
    for (auto idx = 0U; idx < IntersectionApi::GetDeviceCount(); ++idx)
    {
        DeviceInfo devinfo;
        IntersectionApi::GetDeviceInfo(idx, devinfo);

        if (devinfo.type == DeviceInfo::kGpu && gpuidx == -1)
        {
            gpuidx = idx;
        }
    }

    EXPECT_NE(gpuidx, -1);

    apigpu_ = IntersectionApi::Create(gpuidx);
    EXPECT_NE(apigpu_, nullptr);

    // Load obj file 
    std::string res = LoadObj(shapes_, materials_, "../Resources/CornellBox/orig.objm");

    // Create meshes within IntersectionApi
    for (int i = 0; i<(int)shapes_.size(); ++i)
    {
        Shape* shape = nullptr;

        EXPECT_NO_THROW(shape = apigpu_->CreateMesh(&shapes_[i].mesh.positions[0], (int)shapes_[i].mesh.positions.size() / 3, 3 * sizeof(float),
            &shapes_[i].mesh.indices[0], 0, nullptr, (int)shapes_[i].mesh.indices.size() / 3));

        EXPECT_NO_THROW(apigpu_->AttachShape(shape));

        test_shapes_.push_back({ &shapes_[i].mesh.positions[0], (int)shapes_[i].mesh.positions.size() / 3,
            &shapes_[i].mesh.indices[0], (int)shapes_[i].mesh.indices.size(), nullptr, (int)shapes_[i].mesh.indices.size() / 3 });
        test_shapes_.back().shape = shape;

        apishapes_gpu_.push_back(shape);
    }

    apigpu_->SetOption("acc.type", "bvh");
    apigpu_->SetOption("bvh.builder", "sah");

    srand(0xABCDEF12);

}

inline void ApiConformanceVK::TearDown()
{
    // TearDown needs to be safe for no OpenCL cpu or GPU hence
    // all the if( apiXpu_)

    // Commit update
    if (apigpu_) { EXPECT_NO_THROW(apigpu_->Commit()); }

    // Delete meshes
    for (int i = 0; i<(int)apishapes_gpu_.size(); ++i)
    {
        if (apigpu_) { EXPECT_NO_THROW(apigpu_->DeleteShape(apishapes_gpu_[i])); }
    }

    if (apigpu_) { IntersectionApi::Delete(apigpu_); }
}

/*
BEGIN GPU TESTS
*/
TEST_F(ApiConformanceVK, GPU_CornellBox_1RandomRay_ClosestHit_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectClosestRaysOk<1>(api);
}

TEST_F(ApiConformanceVK, GPU_CornellBox_100RayRandom_ClosestHit_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectClosestRaysOk<100>(api);
}

TEST_F(ApiConformanceVK, GPU_CornellBox_1000RaysRandom_ClosestHit_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectClosestRaysOk<1000>(api);
}

TEST_F(ApiConformanceVK, GPU_CornellBox_10000RaysRandom_ClosestHit_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectClosestRaysOk<10000>(api);
}

TEST_F(ApiConformanceVK, GPU_CornellBox_10RaysRandom_ClosestHit_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectClosestRaysOk<10>(api);
}

TEST_F(ApiConformanceVK, GPU_CornellBox_10000RaysRandom_ClosestHit_Force2level_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 1.f);

    ExpectClosestRaysOk<10000>(api);

}

TEST_F(ApiConformanceVK, GPU_CornellBox_1000RandomRays_ClosestHit_Bruteforce_FatBvh)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "fatbvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectClosestRaysOk<1000>(api);

}

TEST_F(ApiConformanceVK, DISABLED_GPU_CornellBox_1000Rays_Brutforce_HlBvh)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "hlbvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectClosestRaysOk<1000>(api);

}


TEST_F(ApiConformanceVK, GPU_CornellBox_1RandomRays_AnyHit_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectAnyRaysOk<1>(api);
}
TEST_F(ApiConformanceVK, GPU_CornellBox_100RandomRays_AnyHit_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectAnyRaysOk<100>(api);
}

TEST_F(ApiConformanceVK, GPU_CornellBox_1000RandomRays_AnyHit_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectAnyRaysOk<1000>(api);
}

TEST_F(ApiConformanceVK, GPU_CornellBox_10000RandomRays_AnyHit_Bruteforce)
{
    auto api = apigpu_;
    api->SetOption("acc.type", "bvh");
    api->SetOption("bvh.builder", "sah");
    api->SetOption("bvh.force2level", 0.f);

    ExpectAnyRaysOk<10000>(api);
}

TEST_F(ApiConformanceVK, CornellBox_10000RaysRandom_ClosestHit_Events_Bruteforce)
{
    int const kNumRays = 10000;

    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this
    Intersection isect_brute[kNumRays];
    ray r_brute[kNumRays];

    // generate some random vectors
    for (int i = 0; i < kNumRays; ++i)
    {
        r_brute[i].o = float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
        r_brute[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));
    }

    EXPECT_NO_THROW(apigpu_->Commit());

    TestIntersections(test_shapes_.data(), test_shapes_.size(), r_brute, kNumRays, isect_brute);

    auto ray_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
    auto isect_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

    ray* r_gpu = nullptr;

    Event* egpu;
    EXPECT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_gpu, &egpu));
    egpu->Wait(); apigpu_->DeleteEvent(egpu);

    for (int i = 0; i<kNumRays; ++i)
    {
        r_gpu[i].o = r_brute[i].o;
        r_gpu[i].d = r_brute[i].d;
        r_gpu[i].SetActive(true);
        r_gpu[i].SetMask(0xFFFFFFFF);
    }

    EXPECT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, r_gpu, &egpu));
    egpu->Wait(); apigpu_->DeleteEvent(egpu);

    // Intersect
    Event* gpu_event = nullptr;
    EXPECT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, kNumRays, isect_buffer_gpu, nullptr, &gpu_event));

    EXPECT_NE(gpu_event, nullptr);

    EXPECT_NO_THROW(gpu_event->Complete());
    EXPECT_NO_THROW(gpu_event->Wait());

    Intersection* isect_gpu = nullptr;

    EXPECT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_gpu, &egpu));
    egpu->Wait(); apigpu_->DeleteEvent(egpu);

    for (int i = 0; i<kNumRays; ++i)
    {
        ExpectClosestIntersectionOk(isect_brute[i] , isect_gpu[i]);
    }


    EXPECT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, isect_gpu, &egpu));
    egpu->Wait(); apigpu_->DeleteEvent(egpu);

    EXPECT_NO_THROW(apigpu_->DeleteEvent(gpu_event));
    EXPECT_NO_THROW(apigpu_->DeleteBuffer(ray_buffer_gpu));
    EXPECT_NO_THROW(apigpu_->DeleteBuffer(isect_buffer_gpu));
}


inline void ApiConformanceVK::ExpectClosestIntersectionOk(const Intersection& expected, const Intersection& test) const
{
    ASSERT_EQ(test.shapeid, expected.shapeid);

    if (test.shapeid != kNullId)
    {
        // Check if the distance is the same
        const double dist = (test.uvwt.w - expected.uvwt.w) * (test.uvwt.w - expected.uvwt.w);
        ASSERT_NEAR(0, dist, 1e-5);
    }
}

template<int kNumRays>
inline void ApiConformanceVK::ExpectClosestRaysOk(RadeonRays::IntersectionApi* api)const
{
    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this
    Intersection isect_brute[kNumRays];
    ray r_brute[kNumRays];

    // generate some random vectors
    for (int i = 0; i < kNumRays; ++i)
    {
        r_brute[i].o = float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
        r_brute[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));
    }

    EXPECT_NO_THROW(api->Commit());

    // generate the golden test results
    TestIntersections(test_shapes_.data(), test_shapes_.size(), r_brute, kNumRays, isect_brute);

    auto ray_buffer = api->CreateBuffer(kNumRays * sizeof(ray), nullptr);
    auto isect_buffer = api->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

    ray* rays = nullptr;
    Event* ev;

    EXPECT_NO_THROW(api->MapBuffer(ray_buffer, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&rays, &ev));
    ev->Wait(); api->DeleteEvent(ev);

    for (auto i = 0; i<kNumRays; ++i)
    {
        rays[i].o = r_brute[i].o;
        rays[i].d = r_brute[i].d;

        rays[i].SetActive(true);
        rays[i].SetMask(0xFFFFFFFF);
    }

    EXPECT_NO_THROW(api->UnmapBuffer(ray_buffer, rays, &ev));
    ev->Wait(); api->DeleteEvent(ev);

    // Intersect
    EXPECT_NO_THROW(api->QueryIntersection(ray_buffer, kNumRays, isect_buffer, nullptr, nullptr));

    Intersection* isect = nullptr;
    EXPECT_NO_THROW(api->MapBuffer(isect_buffer, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect, &ev));
    ev->Wait(); api->DeleteEvent(ev);

    for (auto i = 0; i<kNumRays; ++i)
    {
        ExpectClosestIntersectionOk(isect_brute[i], isect[i]);
    }

    EXPECT_NO_THROW(api->UnmapBuffer(isect_buffer, isect, &ev));
    ev->Wait(); api->DeleteEvent(ev);


    EXPECT_NO_THROW(api->DeleteBuffer(ray_buffer));
    EXPECT_NO_THROW(api->DeleteBuffer(isect_buffer));
}

template<int kNumRays>
inline void ApiConformanceVK::ExpectAnyRaysOk(RadeonRays::IntersectionApi* api) const
{
    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this

    bool any_brute[kNumRays];
    ray r_brute[kNumRays];

    // generate some random vectors
    for (int i = 0; i < kNumRays; ++i)
    {
        r_brute[i].o = float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
        r_brute[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));
    }

    EXPECT_NO_THROW(api->Commit());

    // generate the golden test results
    TestOcclusions(test_shapes_.data(), test_shapes_.size(), r_brute, kNumRays, any_brute);

    auto ray_buffer = api->CreateBuffer(kNumRays * sizeof(ray), nullptr);
    auto result_buffer = api->CreateBuffer(kNumRays * sizeof(int), nullptr);

    ray* rays = nullptr;
    Event* ev;

    EXPECT_NO_THROW(api->MapBuffer(ray_buffer, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&rays, &ev));
    ev->Wait(); api->DeleteEvent(ev);

    for (auto i = 0; i<kNumRays; ++i)
    {
        rays[i].o = r_brute[i].o;
        rays[i].d = r_brute[i].d;

        rays[i].SetActive(true);
        rays[i].SetMask(0xFFFFFFFF);
    }

    EXPECT_NO_THROW(api->UnmapBuffer(ray_buffer, rays, &ev));
    ev->Wait(); api->DeleteEvent(ev);

    // Intersect
    EXPECT_NO_THROW(api->QueryOcclusion(ray_buffer, kNumRays, result_buffer, nullptr, nullptr));


    int* results = nullptr;
    EXPECT_NO_THROW(api->MapBuffer(result_buffer, kMapRead, 0, kNumRays * sizeof(int), (void**)&results, &ev));
    ev->Wait(); api->DeleteEvent(ev);

    for (auto i = 0; i<kNumRays; ++i)
    {
        ASSERT_EQ(any_brute[i], (results[i] > 0) ? true : false);
    }

    EXPECT_NO_THROW(api->UnmapBuffer(result_buffer, results, &ev));
    ev->Wait(); api->DeleteEvent(ev);


    EXPECT_NO_THROW(api->DeleteBuffer(ray_buffer));
    EXPECT_NO_THROW(api->DeleteBuffer(result_buffer));
}

#endif // USE_VULKAN

