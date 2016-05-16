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
#ifndef FIRERAYS_CONFORMANCE_TEST_H
#define FIRERAYS_CONFORMANCE_TEST_H

/// This test suite is testing Firerays GPU results to conform to CPU
///

#include "gtest/gtest.h"
#include "firerays.h"

using namespace FireRays;
using namespace tinyobj;

#include "tiny_obj_loader.h"

#include <vector>
#include <cstdio>

// Api creation fixture, prepares api_ for further tests
class ApiConformance : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        //Search for native CPU
        int cpuidx = -1;
        int gpuidx = -1;
        for (int idx=0; idx < IntersectionApi::GetDeviceCount(); ++idx)
        {
			DeviceInfo devinfo;
			IntersectionApi::GetDeviceInfo(idx, devinfo);

            if (devinfo.type == DeviceInfo::kCpu && cpuidx == -1)
            {
                cpuidx = idx;
            }

            if (devinfo.type == DeviceInfo::kGpu && gpuidx == -1)
            {
                gpuidx = idx;
            }
        }
        
        //ASSERT_NE(nativeidx, -1);
        
        apicpu_ = IntersectionApi::Create(cpuidx);
        apigpu_ = IntersectionApi::Create(gpuidx);

        // Load obj file 
        std::string res = LoadObj(shapes_, materials_, "../Resources/CornellBox/orig.objm");

        // Create meshes within IntersectionApi
        for  (int i=0; i<(int)shapes_.size(); ++i)
        {
            Shape* shape = nullptr;

            ASSERT_NO_THROW(shape = apicpu_->CreateMesh(&shapes_[i].mesh.positions[0], (int)shapes_[i].mesh.positions.size() / 3, 3*sizeof(float),
                &shapes_[i].mesh.indices[0], 0, nullptr, (int)shapes_[i].mesh.indices.size() / 3));

            ASSERT_NO_THROW(apicpu_->AttachShape(shape));

            apishapes_cpu_.push_back(shape);

            ASSERT_NO_THROW(shape = apigpu_->CreateMesh(&shapes_[i].mesh.positions[0], (int)shapes_[i].mesh.positions.size() / 3, 3*sizeof(float),
                &shapes_[i].mesh.indices[0], 0, nullptr, (int)shapes_[i].mesh.indices.size() / 3));

            ASSERT_NO_THROW(apigpu_->AttachShape(shape));

            apishapes_gpu_.push_back(shape);
        }

		apigpu_->SetOption("acc.type", "bvh");
		apigpu_->SetOption("bvh.builder", "sah");
    }

    virtual void TearDown()
    {
        // Commit update
        ASSERT_NO_THROW(apicpu_->Commit());
        ASSERT_NO_THROW(apigpu_->Commit());

        // Delete meshes
        for (int i=0; i<(int)apishapes_cpu_.size(); ++i)
        {
            ASSERT_NO_THROW(apicpu_->DeleteShape(apishapes_cpu_[i]));
            ASSERT_NO_THROW(apigpu_->DeleteShape(apishapes_gpu_[i]));
        }

        IntersectionApi::Delete(apicpu_);
        IntersectionApi::Delete(apigpu_);
    }

	void Wait(IntersectionApi* api)
	{
		e_->Wait();
		api->DeleteEvent(e_);
	}

    void BrutforceTrace(ray& r, Intersection& isect);

    // CPU api
    IntersectionApi* apicpu_;
    // GPU api
    IntersectionApi* apigpu_;

    std::vector<Shape*> apishapes_cpu_;
    std::vector<Shape*> apishapes_gpu_;
	
	Event* e_;

    // Tinyobj data
    std::vector<shape_t> shapes_;
    std::vector<material_t> materials_;
};


TEST_F(ApiConformance, CornellBox_1Ray_ClosestHit)
{
    ASSERT_NO_THROW(apicpu_->Commit());
    ASSERT_NO_THROW(apigpu_->Commit());

    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this
    ray r;
    r.o = float4(0.1f, 0.6f, -3.f, 1000.f);
    r.d = float4(0.0f, 0.0f, 1.f, 0.f);

    Intersection isect_cpu;
    Intersection isect_gpu;

	auto ray_buffer_cpu = apicpu_->CreateBuffer(sizeof(ray), &r);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(sizeof(Intersection), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(sizeof(ray), &r);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(sizeof(Intersection), nullptr);

	// Intersect
	ASSERT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, 1, isect_buffer_cpu, nullptr, nullptr));
	ASSERT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, 1, isect_buffer_gpu, nullptr, nullptr));

	Intersection* tmp = nullptr;
	ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
	Wait(apicpu_);
	isect_cpu = *tmp;
	ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, tmp, &e_));
	Wait(apicpu_);

	
	tmp = nullptr;
	ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
	Wait(apigpu_);

	isect_gpu = *tmp;
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, tmp, &e_));
	Wait(apigpu_);

    // Check if an intersection is the same
    ASSERT_EQ(isect_cpu.shapeid, isect_gpu.shapeid);
    ASSERT_EQ(isect_cpu.primid, isect_gpu.primid);
    ASSERT_LE((isect_cpu.uvwt - isect_gpu.uvwt).sqnorm(), 0.001f);

	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}

TEST_F(ApiConformance, CornellBox_1Ray_AnyHit)
{
    ASSERT_NO_THROW(apicpu_->Commit());
    ASSERT_NO_THROW(apigpu_->Commit());

    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this
    ray r;
    r.o = float3(0.5, 0.5, -10, 1000.f);
    r.d = float3(0, 0, 1);

    int hitresult_cpu = 0;
    int hitresult_gpu = 0;

	auto ray_buffer_cpu = apicpu_->CreateBuffer(sizeof(ray), &r);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(sizeof(int), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(sizeof(ray), &r);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(sizeof(int), nullptr);

	// Intersect
	ASSERT_NO_THROW(apicpu_->QueryOcclusion(ray_buffer_cpu, 1, isect_buffer_cpu, nullptr, nullptr));
	ASSERT_NO_THROW(apigpu_->QueryOcclusion(ray_buffer_gpu, 1, isect_buffer_gpu, nullptr, nullptr));

	int* tmp = nullptr;
	ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, sizeof(int), (void**)&tmp, &e_));
	Wait(apicpu_);
	hitresult_cpu = *tmp;
	ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, tmp, &e_));
	Wait(apicpu_);

	tmp = nullptr;
	ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, sizeof(int), (void**)&tmp, &e_));
	Wait(apigpu_);
	hitresult_gpu = *tmp;
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, tmp, &e_));
	Wait(apigpu_);


    bool cpu_hit = (hitresult_cpu == kNullId);
    bool gpu_hit = (hitresult_gpu == kNullId);
    ASSERT_NE(hitresult_cpu, gpu_hit);

	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}

TEST_F(ApiConformance, CornellBox_1RayRandom_ClosestHit)
{
    int kNumTest = 100;
    srand((unsigned)time(0));
    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this

     ASSERT_NO_THROW(apicpu_->Commit());
     ASSERT_NO_THROW(apigpu_->Commit());

	 auto ray_buffer_cpu = apicpu_->CreateBuffer(sizeof(ray), nullptr);
	 auto isect_buffer_cpu = apicpu_->CreateBuffer(sizeof(Intersection), nullptr);

	 auto ray_buffer_gpu = apigpu_->CreateBuffer(sizeof(ray), nullptr);
	 auto isect_buffer_gpu = apigpu_->CreateBuffer(sizeof(Intersection), nullptr);

    for (int i=0; i<kNumTest; ++i)
    {
        ray r_cpu, r_gpu;
        r_gpu.o = r_cpu.o = float4(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
        r_gpu.d = r_cpu.d = normalize(float3(rand_float(), rand_float(), rand_float()));

        Intersection isect_cpu;
        Intersection isect_gpu;

		ray* pray = nullptr;
		ASSERT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, sizeof(ray), (void**)&pray, &e_));
		Wait(apicpu_);

		*pray = r_cpu;
		ASSERT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, pray, &e_));
		Wait(apicpu_);


		ASSERT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, sizeof(ray), (void**)&pray, &e_));
		Wait(apigpu_);

		*pray = r_gpu;
		ASSERT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, pray, &e_));
		Wait(apigpu_);


		// Intersect
		ASSERT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, 1, isect_buffer_cpu, nullptr, nullptr));
		ASSERT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, 1, isect_buffer_gpu, nullptr, nullptr));


		Intersection* tmp = nullptr;
		ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
		Wait(apicpu_);

		isect_cpu = *tmp;
		ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, tmp, &e_));
		Wait(apicpu_);

		tmp = nullptr;
		ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, sizeof(Intersection), (void**)&tmp, &e_));
		Wait(apigpu_);

		isect_gpu = *tmp;
		ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, tmp, &e_));
		Wait(apigpu_);

       

        if (isect_cpu.shapeid >= 0 && isect_gpu.shapeid >= 0)
        {
            // Check if the distance is the same
            ASSERT_LE((isect_cpu.uvwt.w - isect_gpu.uvwt.w)*(isect_cpu.uvwt.w - isect_gpu.uvwt.w), 0.0001f);
        }

    }


	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}

TEST_F(ApiConformance, CornellBox_1RayRandom_AnyHit)
{
    int kNumTest = 1000;
    srand((unsigned)time(0));
    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this

     ASSERT_NO_THROW(apicpu_->Commit());
     ASSERT_NO_THROW(apigpu_->Commit());

	 auto ray_buffer_cpu = apicpu_->CreateBuffer(sizeof(ray), nullptr);
	 auto isect_buffer_cpu = apicpu_->CreateBuffer(sizeof(int), nullptr);

	 auto ray_buffer_gpu = apigpu_->CreateBuffer(sizeof(ray), nullptr);
	 auto isect_buffer_gpu = apigpu_->CreateBuffer(sizeof(int), nullptr);

    for (int i=0; i<kNumTest; ++i)
    {
        ray r_cpu, r_gpu;
        r_gpu.o = r_cpu.o = float4(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
        r_gpu.d = r_cpu.d = normalize(float3(rand_float(), rand_float(), rand_float()));

        int hitresult_cpu = 0;
        int hitresult_gpu = 0;

		ray* pray = nullptr;
		ASSERT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, sizeof(ray), (void**)&pray, &e_));
		Wait(apicpu_);

		*pray = r_cpu;
		ASSERT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, pray, &e_));
		Wait(apicpu_);


		ASSERT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, sizeof(ray), (void**)&pray, &e_));
		Wait(apigpu_);

		*pray = r_gpu;
		ASSERT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, pray, &e_));
		Wait(apigpu_);


		// Intersect
		ASSERT_NO_THROW(apicpu_->QueryOcclusion(ray_buffer_cpu, 1, isect_buffer_cpu, nullptr, nullptr));
		ASSERT_NO_THROW(apigpu_->QueryOcclusion(ray_buffer_gpu, 1, isect_buffer_gpu, nullptr, nullptr));

		int* tmp = nullptr;
		ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, sizeof(int), (void**)&tmp, &e_));
		Wait(apicpu_);

		hitresult_cpu = *tmp;
		ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, tmp, &e_));
		Wait(apicpu_);


		tmp = nullptr;
		ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, sizeof(int), (void**)&tmp, &e_));
		Wait(apigpu_);

		hitresult_gpu = *tmp;
		ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, tmp, &e_));
		Wait(apigpu_);


        bool cpu_hit = (hitresult_cpu == kNullId);
        bool gpu_hit = (hitresult_gpu == kNullId);
        ASSERT_NE(hitresult_cpu, gpu_hit);
    }

	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}


TEST_F(ApiConformance, CornellBox_10000RaysRandom_ClosestHit)
{
    int const kNumRays = 10000;
    srand((unsigned)time(0));
    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this

    ASSERT_NO_THROW(apicpu_->Commit());
    ASSERT_NO_THROW(apigpu_->Commit());

	auto ray_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

    ray* r_cpu = nullptr;
	ray* r_gpu = nullptr;

	Event* egpu, *ecpu;
	ASSERT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

    for (int i=0; i<kNumRays; ++i)
    {
        r_gpu[i].o = r_cpu[i].o = float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
        r_gpu[i].d = r_cpu[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));
		r_gpu[i].SetActive(true);
		r_cpu[i].SetActive(true);
		r_gpu[i].SetMask(0xFFFFFFFF);
		r_cpu[i].SetMask(0xFFFFFFFF);
    }

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

 
	// Intersect
	ASSERT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, kNumRays, isect_buffer_cpu, nullptr, nullptr));
	ASSERT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, kNumRays, isect_buffer_gpu, nullptr, nullptr));


	Intersection* isect_cpu = nullptr;
	Intersection* isect_gpu = nullptr;
	ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

    for (int i = 0; i<kNumRays;++i)
    {
        if (isect_cpu[i].shapeid >= 0 && isect_gpu[i].shapeid >= 0)
        {
            // Check if the distance is the same
            float dist = (isect_cpu[i].uvwt.w - isect_gpu[i].uvwt.w) * (isect_cpu[i].uvwt.w - isect_gpu[i].uvwt.w);
            ASSERT_LE(dist, 0.0001f);
        }
    }

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);


	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}


TEST_F(ApiConformance, CornellBox_10000RaysRandom_ClosestHit_Force2level)
{
    apigpu_->SetOption("bvh.force2level", 1.f);

	int const kNumRays = 10000;
	srand((unsigned)time(0));
	// Make sure the ray is not on BB boundary
	// in this case results may differ due to 
	// different NaNs propagation in BB test
	// TODO: fix this

	ASSERT_NO_THROW(apicpu_->Commit());
	ASSERT_NO_THROW(apigpu_->Commit());

	auto ray_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	ray* r_cpu = nullptr;
	ray* r_gpu = nullptr;

	Event* egpu, *ecpu;
	ASSERT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);


	for (int i = 0; i<kNumRays; ++i)
	{
		r_gpu[i].o = r_cpu[i].o = float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
		r_gpu[i].d = r_cpu[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));
		r_gpu[i].SetActive(true);
		r_cpu[i].SetActive(true);
		r_gpu[i].SetMask(0xFFFFFFFF);
		r_cpu[i].SetMask(0xFFFFFFFF);
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);


	// Intersect
	ASSERT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, kNumRays, isect_buffer_cpu, nullptr, nullptr));
	ASSERT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, kNumRays, isect_buffer_gpu, nullptr, nullptr));

	Intersection* isect_cpu = nullptr;
	Intersection* isect_gpu = nullptr;
	ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		if (isect_cpu[i].shapeid >= 0 && isect_gpu[i].shapeid >= 0)
		{
			// Check if the distance is the same
			float dist = (isect_cpu[i].uvwt.w - isect_gpu[i].uvwt.w) * (isect_cpu[i].uvwt.w - isect_gpu[i].uvwt.w);
			ASSERT_LE(dist, 0.0001f);
		}
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);


	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}


TEST_F(ApiConformance, CornellBox_10000RaysRandom_ClosestHit_Events)
{
    int const kNumRays = 10000;
    srand((unsigned)time(0));
    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this

	ASSERT_NO_THROW(apicpu_->Commit());
	ASSERT_NO_THROW(apigpu_->Commit());


	auto ray_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	ray* r_cpu = nullptr;
	ray* r_gpu = nullptr;

	Event* egpu, *ecpu;
	ASSERT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);


	for (int i = 0; i<kNumRays; ++i)
	{
		r_gpu[i].o = r_cpu[i].o = float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
		r_gpu[i].d = r_cpu[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));
		r_gpu[i].SetActive(true);
		r_cpu[i].SetActive(true);
		r_gpu[i].SetMask(0xFFFFFFFF);
		r_cpu[i].SetMask(0xFFFFFFFF);
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	// Intersect
	Event* cpu_event = nullptr;
	Event* gpu_event = nullptr;
	ASSERT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, kNumRays, isect_buffer_cpu, nullptr, &cpu_event));
	ASSERT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, kNumRays, isect_buffer_gpu, nullptr, &gpu_event));

	ASSERT_NE(cpu_event, nullptr);
	ASSERT_NE(gpu_event, nullptr);

	ASSERT_NO_THROW(cpu_event->Complete());
	ASSERT_NO_THROW(cpu_event->Wait());

	ASSERT_NO_THROW(gpu_event->Complete());
	ASSERT_NO_THROW(gpu_event->Wait());

	Intersection* isect_cpu = nullptr;
	Intersection* isect_gpu = nullptr;

	ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		if (isect_cpu[i].shapeid >= 0 && isect_gpu[i].shapeid >= 0)
		{
			// Check if the distance is the same
			float dist = (isect_cpu[i].uvwt.w - isect_gpu[i].uvwt.w) * (isect_cpu[i].uvwt.w - isect_gpu[i].uvwt.w);
			ASSERT_LE(dist, 0.0001f);
		}
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

    for (int i = 0; i<kNumRays;++i)
    {
        if (isect_cpu[i].shapeid >= 0 && isect_gpu[i].shapeid >= 0)
        {
            // Check if the distance is the same
            float dist = (isect_cpu[i].uvwt.w - isect_gpu[i].uvwt.w) * (isect_cpu[i].uvwt.w - isect_gpu[i].uvwt.w);
            ASSERT_LE(dist, 0.05f);
        }
    }

	ASSERT_NO_THROW(apicpu_->DeleteEvent(cpu_event));
	ASSERT_NO_THROW(apicpu_->DeleteEvent(gpu_event));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}

TEST_F(ApiConformance, CornellBox_1000Rays_Brutforce)
{
    int const kNumRays = 1000;
    srand((unsigned)time(0));

    // Make sure the ray is not on BB boundary
    // in this case results may differ due to 
    // different NaNs propagation in BB test
    // TODO: fix this
    ASSERT_NO_THROW(apicpu_->Commit());
    ASSERT_NO_THROW(apigpu_->Commit());

	auto ray_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

    std::vector<ray> r_gold(kNumRays);

	ray* r_cpu = nullptr;
	ray* r_gpu = nullptr;

	std::vector<Intersection> isect_gold(kNumRays);

	Event* egpu, *ecpu;
	ASSERT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

    for (int i=0; i<kNumRays; ++i)
    {
        r_gold[i].o = r_gpu[i].o = r_cpu[i].o = float4(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
        r_gold[i].d = r_gpu[i].d = r_cpu[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));

		r_gpu[i].SetActive(true);
		r_cpu[i].SetActive(true);
		r_gold[i].SetActive(true);

		r_gpu[i].SetMask(0xFFFFFFFF);
		r_cpu[i].SetMask(0xFFFFFFFF);
		r_gold[i].SetMask(0xFFFFFFFF);

        BrutforceTrace(r_gold[i], isect_gold[i]);
    }

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	// Intersect
	ASSERT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, kNumRays, isect_buffer_cpu, nullptr, nullptr));
	ASSERT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, kNumRays, isect_buffer_gpu, nullptr, nullptr));

	Intersection* isect_cpu = nullptr;
	Intersection* isect_gpu = nullptr;
	ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		if (isect_gold[i].shapeid >= 0)
		{
			float dist1 = (isect_cpu[i].uvwt.w - isect_gold[i].uvwt.w) * (isect_cpu[i].uvwt.w - isect_gold[i].uvwt.w);
			float dist2 = (isect_gpu[i].uvwt.w - isect_gold[i].uvwt.w) * (isect_gpu[i].uvwt.w - isect_gold[i].uvwt.w);

			ASSERT_LE(dist1, 0.0001f);
			ASSERT_LE(dist2, 0.0001f);
		}
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}

TEST_F(ApiConformance, CornellBox_1000Rays_Brutforce_FatBvh)
{
	apicpu_->SetOption("acc.type", "fatbvh");
	apigpu_->SetOption("bvh.builder", "sah");

	int const kNumRays = 1000;
	srand((unsigned)time(0));

	// Make sure the ray is not on BB boundary
	// in this case results may differ due to 
	// different NaNs propagation in BB test
	// TODO: fix this
	ASSERT_NO_THROW(apicpu_->Commit());
	ASSERT_NO_THROW(apigpu_->Commit());

	auto ray_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	std::vector<ray> r_gold(kNumRays);

	ray* r_cpu = nullptr;
	ray* r_gpu = nullptr;

	std::vector<Intersection> isect_gold(kNumRays);

	Event* ecpu, *egpu;
	ASSERT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		r_gold[i].o = r_gpu[i].o = r_cpu[i].o = float4(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
		r_gold[i].d = r_gpu[i].d = r_cpu[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));

		r_gpu[i].SetActive(true);
		r_cpu[i].SetActive(true);
		r_gold[i].SetActive(true);

		r_gpu[i].SetMask(0xFFFFFFFF);
		r_cpu[i].SetMask(0xFFFFFFFF);
		r_gold[i].SetMask(0xFFFFFFFF);

		BrutforceTrace(r_gold[i], isect_gold[i]);
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	// Intersect
	ASSERT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, kNumRays, isect_buffer_cpu, nullptr, nullptr));
	ASSERT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, kNumRays, isect_buffer_gpu, nullptr, nullptr));

	Intersection* isect_cpu = nullptr;
	Intersection* isect_gpu = nullptr;
	ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		if (isect_gold[i].shapeid >= 0)
		{
			float dist1 = (isect_cpu[i].uvwt.w - isect_gold[i].uvwt.w) * (isect_cpu[i].uvwt.w - isect_gold[i].uvwt.w);
			float dist2 = (isect_gpu[i].uvwt.w - isect_gold[i].uvwt.w) * (isect_gpu[i].uvwt.w - isect_gold[i].uvwt.w);

			ASSERT_LE(dist1, 0.0001f);
			ASSERT_LE(dist2, 0.0001f);
		}
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}

TEST_F(ApiConformance, CornellBox_1000Rays_Brutforce_HlBvh)
{
	apicpu_->SetOption("acc.type", "hlbvh");

	int const kNumRays = 1000;
	srand((unsigned)time(0));

	// Make sure the ray is not on BB boundary
	// in this case results may differ due to 
	// different NaNs propagation in BB test
	// TODO: fix this
	ASSERT_NO_THROW(apicpu_->Commit());
	ASSERT_NO_THROW(apigpu_->Commit());

	auto ray_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	std::vector<ray> r_gold(kNumRays);

	ray* r_cpu = nullptr;
	ray* r_gpu = nullptr;

	std::vector<Intersection> isect_gold(kNumRays);

	Event* ecpu, *egpu;
	ASSERT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		r_gold[i].o = r_gpu[i].o = r_cpu[i].o = float4(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
		r_gold[i].d = r_gpu[i].d = r_cpu[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));

		r_gpu[i].SetActive(true);
		r_cpu[i].SetActive(true);
		r_gold[i].SetActive(true);

		r_gpu[i].SetMask(0xFFFFFFFF);
		r_cpu[i].SetMask(0xFFFFFFFF);
		r_gold[i].SetMask(0xFFFFFFFF);

		BrutforceTrace(r_gold[i], isect_gold[i]);
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	// Intersect
	ASSERT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, kNumRays, isect_buffer_cpu, nullptr, nullptr));
	ASSERT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, kNumRays, isect_buffer_gpu, nullptr, nullptr));

	Intersection* isect_cpu = nullptr;
	Intersection* isect_gpu = nullptr;
	ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		if (isect_gold[i].shapeid >= 0)
		{
			float dist1 = (isect_cpu[i].uvwt.w - isect_gold[i].uvwt.w) * (isect_cpu[i].uvwt.w - isect_gold[i].uvwt.w);
			float dist2 = (isect_gpu[i].uvwt.w - isect_gold[i].uvwt.w) * (isect_gpu[i].uvwt.w - isect_gold[i].uvwt.w);

			ASSERT_LE(dist1, 0.0001f);
			ASSERT_LE(dist2, 0.0001f);
		}
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}

TEST_F(ApiConformance, CornellBox_1000Rays_Brutforce_2level)
{
	apigpu_->SetOption("bvh.force2level", 1.f);

	int const kNumRays = 1000;
	srand((unsigned)time(0));

	// Make sure the ray is not on BB boundary
	// in this case results may differ due to 
	// different NaNs propagation in BB test
	// TODO: fix this
	ASSERT_NO_THROW(apicpu_->Commit());
	ASSERT_NO_THROW(apigpu_->Commit());

	auto ray_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	std::vector<ray> r_gold(kNumRays);

	ray* r_cpu = nullptr;
	ray* r_gpu = nullptr;

	std::vector<Intersection> isect_gold(kNumRays);

	Event* ecpu, *egpu;
	ASSERT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		r_gold[i].o = r_gpu[i].o = r_cpu[i].o = float4(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
		r_gold[i].d = r_gpu[i].d = r_cpu[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));

		r_gpu[i].SetActive(true);
		r_cpu[i].SetActive(true);
		r_gold[i].SetActive(true);

		r_gpu[i].SetMask(0xFFFFFFFF);
		r_cpu[i].SetMask(0xFFFFFFFF);
		r_gold[i].SetMask(0xFFFFFFFF);

		BrutforceTrace(r_gold[i], isect_gold[i]);
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, r_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	// Intersect
	ASSERT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, kNumRays, isect_buffer_cpu, nullptr, nullptr));
	ASSERT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, kNumRays, isect_buffer_gpu, nullptr, nullptr));

	Intersection* isect_cpu = nullptr;
	Intersection* isect_gpu = nullptr;
	ASSERT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		if (isect_gold[i].shapeid >= 0)
		{
			float dist1 = (isect_cpu[i].uvwt.w - isect_gold[i].uvwt.w) * (isect_cpu[i].uvwt.w - isect_gold[i].uvwt.w);
			float dist2 = (isect_gpu[i].uvwt.w - isect_gold[i].uvwt.w) * (isect_gpu[i].uvwt.w - isect_gold[i].uvwt.w);

			ASSERT_LE(dist1, 0.0001f);
			ASSERT_LE(dist2, 0.0001f);
		}
	}

	ASSERT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, isect_cpu, &ecpu));
	ASSERT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apicpu_->DeleteEvent(egpu);

	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_gpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	ASSERT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_gpu));
}


inline void ApiConformance::BrutforceTrace(ray& r, Intersection& isect)
{
    isect.uvwt = float4(0, 0, 0, r.o.w);

    Intersection tmpisect;

    for (int s=0; s<(int)shapes_.size(); ++s)
    {
        float* vertices = &shapes_[s].mesh.positions[0];
        int*   indices = &shapes_[s].mesh.indices[0];

        for (int t = 0; t < (int)shapes_[s].mesh.indices.size() / 3; ++t)
        {
            int i0 = indices[t * 3];
            int i1 = indices[t * 3 + 1];
            int i2 = indices[t * 3 + 2];

            float3 vv0 = float3(vertices[i0 * 3], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
            float3 vv1 = float3(vertices[i1 * 3], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
            float3 vv2 = float3(vertices[i2 * 3], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);

            float3 e1 = vv1 - vv0;
            float3 e2 = vv2 - vv0;

            float3 s1 = cross(r.d, e2);
            float det = dot(s1, e1);

            float  invdet = 1.f / det;

            float3 d = r.o - vv0;
            float  b1 = dot(d, s1) * invdet;

            if (b1 < 0.f || b1 > 1.f)
            {
                   continue;
            }

            float3 s2 = cross(d, e1);
            float  b2 = dot(r.d, s2) * invdet;

            if (b2 < 0.f || b1 + b2 > 1.f)
            {
                continue;
            }

            float temp = dot(e2, s2) * invdet;

            if (temp > 0.f && temp < isect.uvwt.w)
            {
                isect.uvwt = float4(b1, b2, 0, temp);
                isect.shapeid = s + 1;
                isect.primid = t;
            }
        }
    }
}


#endif // FIRERAYS_CONFORMANCE_TEST_H
