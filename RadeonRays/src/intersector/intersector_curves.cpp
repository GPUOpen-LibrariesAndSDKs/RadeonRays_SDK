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
#include "intersector_curves.h"

#include "../accelerator/bvh.h"
#include "../accelerator/split_bvh.h"
#include "../primitive/curves.h"
#include "../world/world.h"

#include "../translator/plain_bvh_translator.h"

#include "device.h"
#include "executable.h"
#include <algorithm>

// Preferred work group size for Radeon devices
static int const kWorkGroupSize = 64;

namespace RadeonRays
{
    struct IntersectorCurves::GpuData
    {
        // Device
        Calc::Device* device;

        // BVH nodes
        Calc::Buffer* bvh;

        // Vertex positions
        Calc::Buffer* vertices;

        // Indices
        Calc::Buffer* segments;

        Calc::Executable* executable;
        Calc::Function* isect_func;
        Calc::Function* occlude_func;

        GpuData(Calc::Device* d)
            : device(d)
            , bvh(nullptr)
            , vertices(nullptr)
            , segments(nullptr)
            , executable(nullptr)
        {
        }

        ~GpuData()
        {
            device->DeleteBuffer(bvh);
            device->DeleteBuffer(vertices);
            device->DeleteBuffer(segments);
            if (executable)
            {
                executable->DeleteFunction(isect_func);
                executable->DeleteFunction(occlude_func);
                device->DeleteExecutable(executable);
            }
        }
    };

	IntersectorCurves::IntersectorCurves(Calc::Device* device)
        : Intersector(device)
        , m_gpudata(new GpuData(device))
        , m_bvh(nullptr)
    {
        std::string buildopts =
#ifdef RR_RAY_MASK
            "-D RR_RAY_MASK ";
#else
            "";
#endif

#ifdef USE_SAFE_MATH
        buildopts.append("-D USE_SAFE_MATH ");
#endif
        
#ifndef RR_EMBED_KERNELS
        if ( device->GetPlatform() == Calc::Platform::kOpenCL )
        {
            char const* headers[] = { "../RadeonRays/src/kernels/CL/common.cl" };

            int numheaders = sizeof( headers ) / sizeof( char const* );

            m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/src/kernels/CL/intersect_bvh2_curves.cl", headers, numheaders, buildopts.c_str());
        }
        else
        {
            assert( device->GetPlatform() == Calc::Platform::kVulkan );
            m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/src/kernels/GLSL/bvh.comp", nullptr, 0, buildopts.c_str());
        }
#else
#if USE_OPENCL
        if (device->GetPlatform() == Calc::Platform::kOpenCL)
        {
            m_gpudata->executable = m_device->CompileExecutable(g_intersect_bvh2_skiplinks_opencl, std::strlen(g_intersect_bvh2_curves_opencl), buildopts.c_str());
        }
#endif

#if USE_VULKAN
        if (m_gpudata->executable == nullptr && device->GetPlatform() == Calc::Platform::kVulkan)
        {
            m_gpudata->executable = m_device->CompileExecutable(g_bvh_vulkan, std::strlen(g_bvh_vulkan), buildopts.c_str());
        }
#endif
#endif

        assert(m_gpudata->executable);

        m_gpudata->isect_func = m_gpudata->executable->CreateFunction("intersect_main");
        m_gpudata->occlude_func = m_gpudata->executable->CreateFunction("occluded_main");
    }

    void IntersectorCurves::Process(World const& world)
    {
        // If something has been changed we need to rebuild BVH
        if (!m_bvh || world.has_changed() || world.GetStateChange() != ShapeImpl::kStateChangeNone)
        {
            if (m_bvh)
            {
                m_device->DeleteBuffer(m_gpudata->bvh);
                m_device->DeleteBuffer(m_gpudata->vertices);
                m_device->DeleteBuffer(m_gpudata->segments);
            }

			// Setup BVH options
			{
				auto builder = world.options_.GetOption("bvh.builder");
				auto splits = world.options_.GetOption("bvh.sah.use_splits");
				auto maxdepth = world.options_.GetOption("bvh.sah.max_split_depth");
				auto overlap = world.options_.GetOption("bvh.sah.min_overlap");
				auto tcost = world.options_.GetOption("bvh.sah.traversal_cost");
				auto node_budget = world.options_.GetOption("bvh.sah.extra_node_budget");
				auto nbins = world.options_.GetOption("bvh.sah.num_bins");

				bool use_sah = false;
				bool use_splits = false;
				int max_split_depth = maxdepth ? (int)maxdepth->AsFloat() : 10;
				int num_bins = nbins ? (int)nbins->AsFloat() : 64;
				float min_overlap = overlap ? overlap->AsFloat() : 0.05f;
				float traversal_cost = tcost ? tcost->AsFloat() : 10.f;
				float extra_node_budget = node_budget ? node_budget->AsFloat() : 0.5f;

				if (builder && builder->AsString() == "sah")
				{
					use_sah = true;
				}

				if (splits && splits->AsFloat() > 0.f)
				{
					use_splits = true;
				}

				m_bvh.reset(use_splits ?
					new SplitBvh(traversal_cost, num_bins, max_split_depth, min_overlap, extra_node_budget) :
					new Bvh(traversal_cost, num_bins, use_sah)
				);
			}

			// Identify all the curve shapes in the world (non-instanced only for now)
            int numshapes = (int)world.shapes_.size();
			std::vector<const Shape*> curveShapes;
			for (int n=0; n<numshapes; ++n)
			{
				const Shape* shape = world.shapes_[n];
				if (shape->getType() == Shape::SHAPE_CURVES)
				{
					curveShapes.push_back(shape);
				}
			}
			int numcurves = curveShapes.size();

            // This buffer tracks curve start index for next stage as curve segment indices are relative to 0
            std::vector<int> curve_vertices_start_idx(numcurves);
            std::vector<int> curve_segments_start_idx(numcurves);
			int vertexIndex = 0;
			int segmentIndex = 0;
            for (int i=0; i<numcurves; ++i)
            {
				const Curves* curves = static_cast<const Curves*>(curveShapes[i]);
				curve_vertices_start_idx[i] = vertexIndex;
				curve_segments_start_idx[i] = segmentIndex;
				vertexIndex  += curves->num_vertices();
				segmentIndex += curves->num_segments();
            }

			// total number of vertices and segments (summed over all curves in scene)
			int numsegments = segmentIndex;
			int numvertices = vertexIndex;

            // We can't avoid allocating it here, since bounds aren't stored anywhere
            std::vector<bbox> bounds(numsegments);

            // Collect world space bounds of the segments of all curves
#pragma omp parallel for
            for (int i=0; i<numcurves; ++i)
            {
				const Curves* curves = static_cast<const Curves*>(curveShapes[i]);
				for (int j=0; j<curves->num_segments(); ++j)
				{
					// Here we directly get world space bounds
					curves->GetSegmentBounds(j, false, bounds[curve_segments_start_idx[i] + j]);
				}
            }

            m_bvh->Build(&bounds[0], numcurves);

#ifdef RR_PROFILE
            m_bvh->PrintStatistics(std::cout);
#endif
            PlainBvhTranslator translator;
            translator.Process(*m_bvh);

            // Update GPU data
            // Copy translated nodes first
            m_gpudata->bvh = m_device->CreateBuffer(translator.nodes_.size() * sizeof(PlainBvhTranslator::Node), Calc::BufferType::kRead, &translator.nodes_[0]);

            // Create vertex buffer
            {
                // Vertices
                m_gpudata->vertices = m_device->CreateBuffer(numvertices*sizeof(float4), Calc::BufferType::kRead);

                // Get the pointer to mapped data
				float4* vertexdata = nullptr;
                Calc::Event* e = nullptr;
                m_device->MapBuffer(m_gpudata->vertices, 0, 0, numvertices*sizeof(float4), Calc::MapType::kMapWrite, (void**)&vertexdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Here we need to put data in world space rather than object space
                // So we need to get the transform from the shape and multiply each vertex
                matrix m, minv;

#pragma omp parallel for
				for (int i = 0; i<numcurves; ++i)
                {
                    // Get the mesh
					const Curves* curves = static_cast<const Curves*>(curveShapes[i]);

                    // Get vertex buffer of the current curves
                    const float3* vertexData = curves->GetVertexData();

					// Get curves transform
					curves->GetTransform(m, minv);

                    //#pragma omp parallel for
                    // Iterate thru vertices, multiply, and append them to GPU buffer
                    for (int j=0; j<curves->num_vertices(); ++j)
                    {
						// transform curve vertex into world space
						float4 vLocal = vertexData[j];
						float4 vWorld = transform_point(vLocal, m);

						// scale curve width into world space
						float widthL = vLocal.w;
						float worldScale = std::max(std::max(m.m00, m.m11), m.m22);
						float widthW = widthL * worldScale;
						vWorld.w = widthW;
						
                        vertexdata[curve_vertices_start_idx[i] + j] = vWorld;
                    }
                }

                m_device->UnmapBuffer(m_gpudata->vertices, 0, vertexdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);
            }

            // Create segment buffer
            {
				// This struct must match exactly the corresponding struct in
				// intersect_bvh2_curves.cl
                struct Segment
                {
                    int idx[2];
                    int shape_mask;
                    int shape_id;
                    int prim_id;
                };

                // This number is different from the number of segments for some BVHs
                auto numindices = m_bvh->GetNumIndices();

                // Create segment buffer
                m_gpudata->segments = m_device->CreateBuffer(numindices*sizeof(Segment), Calc::BufferType::kRead);

                // Get the pointer to mapped data
				Segment* segmentdata = nullptr;
                Calc::Event* e = nullptr;

                m_device->MapBuffer(m_gpudata->segments, 0, 0, numindices*sizeof(Segment), Calc::BufferType::kWrite, (void**)&segmentdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Here the point is to add curves starting index to actual index contained within the curves,
                // getting absolute index in the buffer.
                // Besides that we need to permute the segments according to BVH reordering, which
                // is contained within bvh.primids_
                int const* reordering = m_bvh->GetIndices();
                for (int i=0; i<numindices; ++i)
                {
                    int indextolook4 = reordering[i];

                    // Find the curves shape corresponding to the current face
                    auto iter = std::upper_bound(curve_segments_start_idx.cbegin(), curve_segments_start_idx.cend(), indextolook4);
                    int shapeidx = static_cast<int>(std::distance(curve_segments_start_idx.cbegin(), iter) - 1);
					const Curves* curves = static_cast<const Curves*>(curveShapes[shapeidx]);

                    // Get segment indices of this curves shape
					const int* segmentIndices = curves->GetSegmentData();

                    // Find segment idx
                    int segmentidx = indextolook4 - curve_segments_start_idx[shapeidx];

                    // Find segment start index for this curves shape
                    int segment_start_index = curve_segments_start_idx[shapeidx];

                    // Copy segment data to GPU buffer
					segmentdata[i].idx[0] = segmentIndices[2*segmentidx+0] + segment_start_index;
					segmentdata[i].idx[1] = segmentIndices[2*segmentidx+1] + segment_start_index;

                    // Optimization: we are putting faceid here
					segmentdata[i].shape_id   = curves->GetId();
					segmentdata[i].shape_mask = curves->GetMask();
					segmentdata[i].prim_id = segmentidx;
                }

                m_device->UnmapBuffer(m_gpudata->segments, 0, segmentdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);
            }

            // Make sure everything is commited
            m_device->Finish(0);
        }
    }


    void IntersectorCurves::Intersect(std::uint32_t queueidx, Calc::Buffer const* rays, Calc::Buffer const* numrays, std::uint32_t maxrays, Calc::Buffer* hits, Calc::Event const* waitevent, Calc::Event** event) const
    {
        auto& func = m_gpudata->isect_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpudata->bvh);
        func->SetArg(arg++, m_gpudata->vertices);
        func->SetArg(arg++, m_gpudata->segments);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, numrays);
        func->SetArg(arg++, hits);

        size_t localsize = kWorkGroupSize;
        size_t globalsize = ((maxrays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queueidx, globalsize, localsize, event);
    }


    void IntersectorCurves::Occluded(std::uint32_t queueidx, Calc::Buffer const* rays, Calc::Buffer const* numrays, std::uint32_t maxrays, Calc::Buffer* hits, Calc::Event const* waitevent, Calc::Event** event) const
    {
        auto& func = m_gpudata->occlude_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpudata->bvh);
        func->SetArg(arg++, m_gpudata->vertices);
        func->SetArg(arg++, m_gpudata->segments);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, numrays);
        func->SetArg(arg++, hits);

        size_t localsize = kWorkGroupSize;
        size_t globalsize = ((maxrays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queueidx, globalsize, localsize, event);
    }

}
