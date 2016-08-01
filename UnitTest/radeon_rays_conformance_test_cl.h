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

#ifndef RADEONRAYS_CONFORMANCE_TEST_CL_H
#define RADEONRAYS_CONFORMANCE_TEST_CL_H

#if USE_OPENCL
/// This test suite is testing RadeonRays GPU results to conform to CPU
///

#include "gtest/gtest.h"
#include "radeon_rays.h"
#include "tiny_obj_loader.h"

using namespace RadeonRays;
using namespace tinyobj;

#include "tiny_obj_loader.h"

#include <vector>
#include <cstdio>

// Api creation fixture, prepares api_ for further tests
class ApiConformanceCL : public ::testing::Test
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

    void BruteforceTraceClosest(const ray& r, Intersection& isect) const;
	bool BruteforceTraceAny(const ray& r) const;

	void ExpectClosestIntersectionOk(const Intersection& expected, const Intersection& test) const;

	template< int kNumRays> void ExpectClosestRaysOk(RadeonRays::IntersectionApi* api) const;

	template< int kNumRays> void ExpectAnyRaysOk(RadeonRays::IntersectionApi* api) const;

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

	ray r_brute[kMaxRaysTests];
	Intersection isect_brute[kMaxRaysTests];
	int any_brute[kMaxRaysTests];
};

inline void ApiConformanceCL::SetUp()
{
	apicpu_ = nullptr;
	apigpu_ = nullptr;

	// TODO make conformance tests across multiple backends and devices
	IntersectionApi::SetPlatform(DeviceInfo::kOpenCL);

	//Search for native CPU
	int cpuidx = -1;
	int gpuidx = -1;
	for (auto idx = 0U; idx < IntersectionApi::GetDeviceCount(); ++idx)
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

	apicpu_ = IntersectionApi::Create(cpuidx);
	ASSERT_NE(apicpu_, nullptr);
	apigpu_ = IntersectionApi::Create(gpuidx);
	ASSERT_NE(apigpu_, nullptr);

	// Load obj file 
	std::string res = LoadObj(shapes_, materials_, "CornellBox/orig.objm");

	// Create meshes within IntersectionApi
	for (int i = 0; i<(int)shapes_.size(); ++i)
	{
		Shape* shape = nullptr;

		EXPECT_NO_THROW(shape = apicpu_->CreateMesh(&shapes_[i].mesh.positions[0], (int)shapes_[i].mesh.positions.size() / 3, 3 * sizeof(float),
			&shapes_[i].mesh.indices[0], 0, nullptr, (int)shapes_[i].mesh.indices.size() / 3));

		EXPECT_NO_THROW(apicpu_->AttachShape(shape));

		apishapes_cpu_.push_back(shape);

		EXPECT_NO_THROW(shape = apigpu_->CreateMesh(&shapes_[i].mesh.positions[0], (int)shapes_[i].mesh.positions.size() / 3, 3 * sizeof(float),
			&shapes_[i].mesh.indices[0], 0, nullptr, (int)shapes_[i].mesh.indices.size() / 3));

		EXPECT_NO_THROW(apigpu_->AttachShape(shape));

		apishapes_gpu_.push_back(shape);
	}

	apigpu_->SetOption("acc.type", "bvh");
	apigpu_->SetOption("bvh.builder", "sah");

	srand((unsigned)time(0));

	// generate some random vectors
	for (int i = 0; i < kMaxRaysTests; ++i)
	{
		r_brute[i].o = float3(rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, rand_float() * 3.f - 1.5f, 1000.f);
		r_brute[i].d = normalize(float3(rand_float(), rand_float(), rand_float()));
	}

	// pre generate brute closest intersections and any results
	for (int i = 0; i < kMaxRaysTests; ++i)
	{
		BruteforceTraceClosest(r_brute[i], isect_brute[i]);
		any_brute[i] = BruteforceTraceAny(r_brute[i]) ? 1 : -1;
	}

}

inline void ApiConformanceCL::TearDown()
{
	// TearDown needs to be safe for no OpenCL cpu or GPU hence
	// all the if( apiXpu_)

	// Commit update
	if (apicpu_) { EXPECT_NO_THROW(apicpu_->Commit()); }
	if (apigpu_) { EXPECT_NO_THROW(apigpu_->Commit()); }

	// Delete meshes
	for (int i = 0; i<(int)apishapes_cpu_.size(); ++i)
	{
		if (apicpu_) { EXPECT_NO_THROW(apicpu_->DeleteShape(apishapes_cpu_[i])); }
		if (apigpu_) { EXPECT_NO_THROW(apigpu_->DeleteShape(apishapes_gpu_[i])); }
	}

	if (apicpu_) { IntersectionApi::Delete(apicpu_); }
	if (apigpu_) { IntersectionApi::Delete(apigpu_); }
}

/*
BEGIN CPU TESTS
*/

TEST_F(ApiConformanceCL, CPU_CornellBox_1Ray_ClosestHit_Bruteforce)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<1>(api);
}

TEST_F(ApiConformanceCL, CPU_CornellBox_100RayRandom_ClosestHit_Bruteforce)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<100>(api);
}

TEST_F(ApiConformanceCL, CPU_CornellBox_1000RaysRandom_ClosestHit_Bruteforce)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<1000>(api);
}

TEST_F(ApiConformanceCL, CPU_CornellBox_10000RaysRandom_ClosestHit_Bruteforce)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<10000>(api);
}


TEST_F(ApiConformanceCL, CPU_CornellBox_10000RaysRandom_ClosestHit_Force2level_Bruteforce)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 1.f);

	ExpectClosestRaysOk<10000>(api);
}

TEST_F(ApiConformanceCL, CPU_CornellBox_1000RandomRays_ClosestHit_Bruteforce_FatBvh)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "fatbvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<1000>(api);

}

TEST_F(ApiConformanceCL, CPU_CornellBox_1000Rays_Brutforce_HlBvh)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "hlbvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<1000>(api);
}

TEST_F(ApiConformanceCL, CPU_CornellBox_1RandomRays_AnyHit_Bruteforce)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectAnyRaysOk<1>(api);
}

TEST_F(ApiConformanceCL, CPU_CornellBox_1000RandomRays_AnyHit_Bruteforce)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectAnyRaysOk<1000>(api);
}

TEST_F(ApiConformanceCL, CPU_CornellBox_10000RandomRays_AnyHit_Bruteforce)
{
	auto api = apicpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectAnyRaysOk<10000>(api);
}


/*
	BEGIN GPU TESTS
*/
TEST_F(ApiConformanceCL, GPU_CornellBox_1RandomRay_ClosestHit_Bruteforce)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<1>(api);
}

TEST_F(ApiConformanceCL, GPU_CornellBox_100RayRandom_ClosestHit_Bruteforce)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<100>(api);
}

TEST_F(ApiConformanceCL, GPU_CornellBox_1000RaysRandom_ClosestHit_Bruteforce)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<1000>(api);
}

TEST_F(ApiConformanceCL, GPU_CornellBox_10000RaysRandom_ClosestHit_Bruteforce)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<10000>(api);
}


TEST_F(ApiConformanceCL, GPU_CornellBox_10000RaysRandom_ClosestHit_Force2level_Bruteforce)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 1.f);

	ExpectClosestRaysOk<10000>(api);

}

TEST_F(ApiConformanceCL, GPU_CornellBox_1000RandomRays_ClosestHit_Bruteforce_FatBvh)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "fatbvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<1000>(api);

}

TEST_F(ApiConformanceCL, GPU_CornellBox_1000Rays_Brutforce_HlBvh)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "hlbvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectClosestRaysOk<1000>(api);

}


TEST_F(ApiConformanceCL, GPU_CornellBox_1RandomRays_AnyHit_Bruteforce)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectAnyRaysOk<1>(api);
}

TEST_F(ApiConformanceCL, GPU_CornellBox_1000RandomRays_AnyHit_Bruteforce)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectAnyRaysOk<1000>(api);
}

TEST_F(ApiConformanceCL, GPU_CornellBox_10000RandomRays_AnyHit_Bruteforce)
{
	auto api = apigpu_;
	api->SetOption("acc.type", "bvh");
	api->SetOption("bvh.builder", "sah");
	api->SetOption("bvh.force2level", 0.f);

	ExpectAnyRaysOk<10000>(api);
}

TEST_F(ApiConformanceCL, CornellBox_10000RaysRandom_ClosestHit_Events_Bruteforce)
{
	int const kNumRays = 10000;

	// Make sure the ray is not on BB boundary
	// in this case results may differ due to 
	// different NaNs propagation in BB test
	// TODO: fix this

	EXPECT_NO_THROW(apicpu_->Commit());
	EXPECT_NO_THROW(apigpu_->Commit());

	auto ray_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_cpu = apicpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	auto ray_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(ray), nullptr);
	auto isect_buffer_gpu = apigpu_->CreateBuffer(kNumRays * sizeof(Intersection), nullptr);

	ray* r_cpu = nullptr;
	ray* r_gpu = nullptr;

	Event* egpu, *ecpu;
	EXPECT_NO_THROW(apicpu_->MapBuffer(ray_buffer_cpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_cpu, &ecpu));
	EXPECT_NO_THROW(apigpu_->MapBuffer(ray_buffer_gpu, kMapWrite, 0, kNumRays * sizeof(ray), (void**)&r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apigpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		r_gpu[i].o = r_cpu[i].o = r_brute[i].o;
		r_gpu[i].d = r_cpu[i].d = r_brute[i].d;
		r_gpu[i].SetActive(true);
		r_cpu[i].SetActive(true);
		r_gpu[i].SetMask(0xFFFFFFFF);
		r_cpu[i].SetMask(0xFFFFFFFF);
	}

	EXPECT_NO_THROW(apicpu_->UnmapBuffer(ray_buffer_cpu, r_cpu, &ecpu));
	EXPECT_NO_THROW(apigpu_->UnmapBuffer(ray_buffer_gpu, r_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apigpu_->DeleteEvent(egpu);

	// Intersect
	Event* cpu_event = nullptr;
	Event* gpu_event = nullptr;
	EXPECT_NO_THROW(apicpu_->QueryIntersection(ray_buffer_cpu, kNumRays, isect_buffer_cpu, nullptr, &cpu_event));
	EXPECT_NO_THROW(apigpu_->QueryIntersection(ray_buffer_gpu, kNumRays, isect_buffer_gpu, nullptr, &gpu_event));

	EXPECT_NE(cpu_event, nullptr);
	EXPECT_NE(gpu_event, nullptr);

	EXPECT_NO_THROW(cpu_event->Complete());
	EXPECT_NO_THROW(cpu_event->Wait());

	EXPECT_NO_THROW(gpu_event->Complete());
	EXPECT_NO_THROW(gpu_event->Wait());

	Intersection* isect_cpu = nullptr;
	Intersection* isect_gpu = nullptr;

	EXPECT_NO_THROW(apicpu_->MapBuffer(isect_buffer_cpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_cpu, &ecpu));
	EXPECT_NO_THROW(apigpu_->MapBuffer(isect_buffer_gpu, kMapRead, 0, kNumRays * sizeof(Intersection), (void**)&isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apigpu_->DeleteEvent(egpu);

	for (int i = 0; i<kNumRays; ++i)
	{
		Intersection isect_brute;
		BruteforceTraceClosest(r_brute[i], isect_brute);

		ExpectClosestIntersectionOk(isect_brute, isect_cpu[i]);
		ExpectClosestIntersectionOk(isect_brute, isect_gpu[i]);
	}


	EXPECT_NO_THROW(apicpu_->UnmapBuffer(isect_buffer_cpu, isect_cpu, &ecpu));
	EXPECT_NO_THROW(apigpu_->UnmapBuffer(isect_buffer_gpu, isect_gpu, &egpu));
	ecpu->Wait(); apicpu_->DeleteEvent(ecpu);
	egpu->Wait(); apigpu_->DeleteEvent(egpu);

	EXPECT_NO_THROW(apicpu_->DeleteEvent(cpu_event));
	EXPECT_NO_THROW(apigpu_->DeleteEvent(gpu_event));
	EXPECT_NO_THROW(apicpu_->DeleteBuffer(ray_buffer_cpu));
	EXPECT_NO_THROW(apigpu_->DeleteBuffer(ray_buffer_gpu));
	EXPECT_NO_THROW(apicpu_->DeleteBuffer(isect_buffer_cpu));
	EXPECT_NO_THROW(apigpu_->DeleteBuffer(isect_buffer_gpu));
}


inline void ApiConformanceCL::BruteforceTraceClosest(const ray& r, Intersection& isect) const
{
    isect.uvwt = float4(0, 0, 0, r.o.w);

    Intersection tmpisect;

    for (int s=0; s<(int)shapes_.size(); ++s)
    {
        float const* vertices = &shapes_[s].mesh.positions[0];
        int const* indices = &shapes_[s].mesh.indices[0];

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

inline bool ApiConformanceCL::BruteforceTraceAny(const ray& r) const
{
	Intersection tmpisect;

	for (int s = 0; s<(int)shapes_.size(); ++s)
	{
		float const* vertices = &shapes_[s].mesh.positions[0];
		int const* indices = &shapes_[s].mesh.indices[0];

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

			if (temp > 0.f)
			{
				return true;
			}
		}
	}
	return false;
}

inline void ApiConformanceCL::ExpectClosestIntersectionOk(const Intersection& expected, const Intersection& test) const
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
inline void ApiConformanceCL::ExpectClosestRaysOk(RadeonRays::IntersectionApi* api)const 
{
	// Make sure the ray is not on BB boundary
	// in this case results may differ due to 
	// different NaNs propagation in BB test
	// TODO: fix this

	EXPECT_NO_THROW(api->Commit());

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
inline void ApiConformanceCL::ExpectAnyRaysOk(RadeonRays::IntersectionApi* api) const 
{
	// Make sure the ray is not on BB boundary
	// in this case results may differ due to 
	// different NaNs propagation in BB test
	// TODO: fix this

	EXPECT_NO_THROW(api->Commit());

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
		ASSERT_EQ(any_brute[i], results[i]);
	}

	EXPECT_NO_THROW(api->UnmapBuffer(result_buffer, results, &ev));
	ev->Wait(); api->DeleteEvent(ev);


	EXPECT_NO_THROW(api->DeleteBuffer(ray_buffer));
	EXPECT_NO_THROW(api->DeleteBuffer(result_buffer));
}

#endif // USE_OPENCL

#endif // RadeonRays_CONFORMANCE_TEST_CL_H

