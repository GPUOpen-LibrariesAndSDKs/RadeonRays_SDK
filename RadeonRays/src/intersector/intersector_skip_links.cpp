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
#include "intersector_skip_links.h"

#include "../accelerator/bvh.h"
#include "../accelerator/split_bvh.h"
#include "../primitive/mesh.h"
#include "../primitive/instance.h"
#include "../primitive/curves.h"
#include "../world/world.h"

#include "../translator/plain_bvh_translator.h"

#include "device.h"
#include "executable.h"
#include <algorithm>
#include <unordered_map>

// Preferred work group size for Radeon devices
static int const kWorkGroupSize = 64;

namespace RadeonRays
{
    struct IntersectorSkipLinks::GpuData
    {
        // Device
        Calc::Device* device;
        // BVH nodes
        Calc::Buffer* bvh;

        // Vertex positions
        Calc::Buffer* mesh_vertices;
		Calc::Buffer* curve_vertices;
        
		// Primitive indices
        Calc::Buffer* primitives;

        Calc::Executable* executable;
        Calc::Function* isect_func;
        Calc::Function* occlude_func;

        GpuData(Calc::Device* d)
            : device(d)
            , bvh(nullptr)
            , mesh_vertices(nullptr)
			, curve_vertices(nullptr)
            , primitives(nullptr)
            , executable(nullptr)
        {
        }

        ~GpuData()
        {
            device->DeleteBuffer(bvh);
            device->DeleteBuffer(mesh_vertices);
			device->DeleteBuffer(curve_vertices);
            device->DeleteBuffer(primitives);
            if (executable)
            {
                executable->DeleteFunction(isect_func);
                executable->DeleteFunction(occlude_func);
                device->DeleteExecutable(executable);
            }
        }
    };

    IntersectorSkipLinks::IntersectorSkipLinks(Calc::Device* device)
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

            m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/src/kernels/CL/intersect_bvh2_skiplinks.cl", headers, numheaders, buildopts.c_str());
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
            m_gpudata->executable = m_device->CompileExecutable(g_intersect_bvh2_skiplinks_opencl, std::strlen(g_intersect_bvh2_skiplinks_opencl), buildopts.c_str());
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

    void IntersectorSkipLinks::Process(World const& world)
    {
        // If something has been changed we need to rebuild BVH
        if (!m_bvh || world.has_changed() || world.GetStateChange() != ShapeImpl::kStateChangeNone)
        {
            if (m_bvh)
            {
                m_device->DeleteBuffer(m_gpudata->bvh);
                m_device->DeleteBuffer(m_gpudata->mesh_vertices);
				m_device->DeleteBuffer(m_gpudata->curve_vertices);
                m_device->DeleteBuffer(m_gpudata->primitives);
            }

            // Check options
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

            m_bvh.reset( use_splits ?
                new SplitBvh(traversal_cost, num_bins, max_split_depth, min_overlap, extra_node_budget) :
                new Bvh(traversal_cost, num_bins, use_sah)
            );

			std::vector<Shape const*> shapes(world.shapes_);

			// Compute total number of shapes of each type
			int num_meshes = 0;
			int num_curves = 0;
			for (size_t n=0; n<shapes.size(); ++n)
			{
				const Shape* shape = shapes[n];
				switch (shape->getType())
				{	
					case Shape::SHAPE_MESH:             num_meshes++; break;
					case Shape::SHAPE_INSTANCED_MESH:   num_meshes++; break;
					case Shape::SHAPE_CURVES:           num_curves++; break;
					case Shape::SHAPE_INSTANCED_CURVES: assert(0);    break; // Not supported yet..
				}
			}

			// Build various arrays of indices:

			// array which tracks the start index of each shape in the bounds array 
			// which is required for the next stage as we need to be able to map each bbox index to a shape.
			std::vector<int> shape_bounds_start_idx(shapes.size());

			// arrays which keep track of the start index of each mesh or curve in the GPU vertex and index buffers
			std::vector<int> mesh_vertices_start_idx(num_meshes);
			std::vector<int> mesh_faces_start_idx(num_meshes);
			std::vector<int> curve_vertices_start_idx(num_curves);
			std::vector<int> curve_segments_start_idx(num_curves);

			int bounds_start_idx = 0;
			int mesh_vertices_idx = 0;
			int mesh_faces_idx = 0;
			int curve_vertices_idx = 0;
			int curve_segments_idx = 0;
			std::unordered_map<int, int> shapeToMesh;   // shapeIdx -> meshIdx
			std::unordered_map<int, int> shapeToCurves; // shapeIdx -> curvesIdx
			{
				int meshIdx = 0;
				int curvesIdx = 0;
				for (size_t shapeIdx=0; shapeIdx<shapes.size(); ++shapeIdx)
				{
					const Shape* shape = shapes[shapeIdx];
					switch (shape->getType())
					{	
						case Shape::SHAPE_MESH:
						case Shape::SHAPE_INSTANCED_MESH:
						{							
							const Shape* baseShape = (shape->getType()==Shape::SHAPE_MESH) ? shape : static_cast<Instance const*>(shape)->GetBaseShape();
							Mesh const* mesh = static_cast<Mesh const*>(baseShape);
							const int num_faces = mesh->num_faces();
							shape_bounds_start_idx[shapeIdx] = bounds_start_idx;
							bounds_start_idx += num_faces;
							mesh_vertices_start_idx[meshIdx] = mesh_vertices_idx;
							mesh_faces_start_idx[meshIdx]    = mesh_faces_idx;
							mesh_vertices_idx += mesh->num_vertices();
							mesh_faces_idx += num_faces;
							shapeToMesh[shapeIdx] = meshIdx;
							meshIdx++;
							break;
						}

						case Shape::SHAPE_CURVES:
						case Shape::SHAPE_INSTANCED_CURVES: // Not supported yet..
						{
							const Curves* curves = static_cast<const Curves*>(shape);
							const int num_segments = curves->num_segments();
							shape_bounds_start_idx[shapeIdx] = bounds_start_idx;
							bounds_start_idx += num_segments;
							curve_vertices_start_idx[curvesIdx] = curve_vertices_idx;
							curve_segments_start_idx[curvesIdx] = curve_segments_idx;
							curve_vertices_idx += curves->num_vertices();
							curve_segments_idx += num_segments;
							shapeToCurves[shapeIdx] = curvesIdx;
							curvesIdx++;
							break;
						}
					}
				}
			}

			int num_mesh_vertices = mesh_vertices_idx;
			int num_mesh_faces = mesh_faces_idx;
			int num_curve_vertices = curve_vertices_idx;
			int num_curve_segments = curve_segments_idx;

			// Compute bbox of all shape primitives
			int num_primitives = num_mesh_faces + num_curve_segments;
			std::vector<bbox> bounds(num_primitives);

			for (size_t shapeIdx=0; shapeIdx<shapes.size(); ++shapeIdx)
			{
				const Shape* shape = shapes[shapeIdx];
				bounds_start_idx = shape_bounds_start_idx[shapeIdx];

				switch (shape->getType())
				{	
					case Shape::SHAPE_MESH:
					{
						Mesh const* mesh = static_cast<Mesh const*>(shape);
						const int num_faces = mesh->num_faces();
	#pragma omp parallel for
						for (int j = 0; j<num_faces; ++j)
						{
							mesh->GetFaceBounds(j, false, bounds[bounds_start_idx + j]);
						}
						break;
					}

					case Shape::SHAPE_INSTANCED_MESH:
					{
						Instance const* instance = static_cast<Instance const*>(shape);
						Mesh const* mesh = static_cast<Mesh const*>(instance->GetBaseShape());
						matrix m, minv;
						instance->GetTransform(m, minv);
						const int num_faces = mesh->num_faces();
	#pragma omp parallel for
						for (int j = 0; j<num_faces; ++j)
						{
							bbox faceBounds;
							mesh->GetFaceBounds(j, false, faceBounds);
							bounds[bounds_start_idx + j] = transform_bbox(faceBounds, m);
						}
						break;
					}

					case Shape::SHAPE_CURVES:
					{
						const Curves* curves = static_cast<const Curves*>(shape);
						const int num_segments = curves->num_segments();
	#pragma omp parallel for
						for (int j=0; j<num_segments; ++j)
						{
							curves->GetSegmentBounds(j, false, bounds[bounds_start_idx + j]);
						}
						break;
					}

					case Shape::SHAPE_INSTANCED_CURVES:
					{
						// Not supported yet..
						assert(0);
						break;
					}
				}
			}

			// Build the BVH from the supplied primitive bounds
            m_bvh->Build(&bounds[0], num_primitives);

#ifdef RR_PROFILE
            m_bvh->PrintStatistics(std::cout);
#endif
            PlainBvhTranslator translator;
            translator.Process(*m_bvh);

            // Update GPU data
            // Copy translated nodes first
            m_gpudata->bvh = m_device->CreateBuffer(translator.nodes_.size() * sizeof(PlainBvhTranslator::Node), Calc::BufferType::kRead, &translator.nodes_[0]);

            // Create vertex buffers
            {
				Calc::Event* e = nullptr;

				// Create GPU vertex buffers
				m_gpudata->mesh_vertices  = m_device->CreateBuffer(num_mesh_vertices * sizeof(float3), Calc::BufferType::kRead);
				m_gpudata->curve_vertices = m_device->CreateBuffer(num_curve_vertices * sizeof(float4), Calc::BufferType::kRead);;

				// Get the pointers to mapped data
				float3* mesh_vertexdata = nullptr;
				m_device->MapBuffer(m_gpudata->mesh_vertices, 0, 0, num_mesh_vertices*sizeof(float3), Calc::MapType::kMapWrite, (void**)&mesh_vertexdata, &e);
				e->Wait();
				m_device->DeleteEvent(e);

				float4* curve_vertexdata = nullptr;
				m_device->MapBuffer(m_gpudata->curve_vertices, 0, 0, num_curve_vertices*sizeof(float4), Calc::MapType::kMapWrite, (void**)&curve_vertexdata, &e);
				e->Wait();
				m_device->DeleteEvent(e);

				int meshIdx = 0;
				int curvesIdx = 0;
				for (size_t shapeIdx=0; shapeIdx<shapes.size(); ++shapeIdx)
				{
					const Shape* shape = shapes[shapeIdx];
					switch (shape->getType())
					{	
						case Shape::SHAPE_MESH:
						{
							Mesh const* mesh = static_cast<Mesh const*>(shape);
							float3 const* vertexData = mesh->GetVertexData();
							matrix m, minv;
							mesh->GetTransform(m, minv);
#pragma omp parallel for
							for (int j = 0; j < mesh->num_vertices(); ++j)
							{
								mesh_vertexdata[mesh_vertices_start_idx[meshIdx] + j] = transform_point(vertexData[j], m);
							}
							meshIdx++;
							break;
						}

						case Shape::SHAPE_INSTANCED_MESH:
						{
							Instance const* instance = static_cast<Instance const*>(shape);
							Mesh const* mesh = static_cast<Mesh const*>(instance->GetBaseShape());
							float3 const* vertexData = mesh->GetVertexData();
							matrix m, minv;
							instance->GetTransform(m, minv);
#pragma omp parallel for
							for (int j = 0; j < mesh->num_vertices(); ++j)
							{
								mesh_vertexdata[mesh_vertices_start_idx[meshIdx] + j] = transform_point(vertexData[j], m);
							}
							meshIdx++;
							break;
						}

						case Shape::SHAPE_CURVES:
						{
							const Curves* curves = static_cast<const Curves*>(shape);
							const float3* vertexData = curves->GetVertexData();
							matrix m, minv;
							curves->GetTransform(m, minv);
#pragma omp parallel for
							for (int j=0; j<curves->num_vertices(); ++j)
							{
								float4 vLocal = vertexData[j];
								float4 vWorld = transform_point(vLocal, m);
								float widthL = vLocal.w;
								float worldScale = std::max(std::max(m.m00, m.m11), m.m22);
								float widthW = widthL * worldScale;
								vWorld.w = widthW;
								curve_vertexdata[curve_vertices_start_idx[curvesIdx] + j] = vWorld;
							}
							curvesIdx++;
							break;
						}

						case Shape::SHAPE_INSTANCED_CURVES:
						{
							// Not supported yet..
							assert(0);
							break;
						}
					}
				}

                m_device->UnmapBuffer(m_gpudata->mesh_vertices, 0, mesh_vertexdata, &e);
                e->Wait();
                m_device->DeleteEvent(e);

				m_device->UnmapBuffer(m_gpudata->curve_vertices, 0, curve_vertexdata, &e);
				e->Wait();
				m_device->DeleteEvent(e);
            }

            // Create primitives buffer
            {
                struct Primitive
                {
                    int idx[3];
                    int shape_mask;
                    int shape_id;
                    int prim_id;
					int type_id; // Type ID (0=mesh triangle, 1=curve segment)
                };

                // This number is different from the number of faces for some BVHs
                auto numindices = m_bvh->GetNumIndices();

                // Create primitive buffer
                m_gpudata->primitives = m_device->CreateBuffer(numindices*sizeof(Primitive), Calc::BufferType::kRead);

                // Get the pointer to mapped data
				Primitive* primitivedata = nullptr;
                Calc::Event* e = nullptr;
                m_device->MapBuffer(m_gpudata->primitives, 0, 0, numindices*sizeof(Primitive), Calc::BufferType::kWrite, (void**)&primitivedata, &e);
                e->Wait();
                m_device->DeleteEvent(e);

				// Upload index data for primitives (i.e. mesh triangles and/or curve segments)
                int const* reordering = m_bvh->GetIndices();
                for (int i = 0; i<numindices; ++i)
                {
					// Find the shape corresponding to the reordered BVH index
                    int indextolook4 = reordering[i];
                    auto iter = std::upper_bound(shape_bounds_start_idx.cbegin(), shape_bounds_start_idx.cend(), indextolook4);
                    int shapeIdx = static_cast<int>(std::distance(shape_bounds_start_idx.cbegin(), iter) - 1);
					assert(shapeIdx>=0 && shapeIdx<shapes.size());
					const Shape* shape = shapes[shapeIdx];

					int bounds_start_idx = shape_bounds_start_idx[shapeIdx];

					switch (shape->getType() )
					{	
						case Shape::SHAPE_MESH:
						case Shape::SHAPE_INSTANCED_MESH:
						{							
							const Shape* baseShape = (shape->getType()==Shape::SHAPE_MESH) ? shape : static_cast<Instance const*>(shape)->GetBaseShape();
							Mesh const* mesh = static_cast<Mesh const*>(baseShape);
							int meshIdx = shapeToMesh[shapeIdx];
							Mesh::Face const* faceData = mesh->GetFaceData();
							int faceidx = indextolook4 - bounds_start_idx;
							int vertex_start_index = mesh_vertices_start_idx[meshIdx];

							// Copy face data to GPU buffer
							primitivedata[i].idx[0] = faceData[faceidx].idx[0] + vertex_start_index;
							primitivedata[i].idx[1] = faceData[faceidx].idx[1] + vertex_start_index;
							primitivedata[i].idx[2] = faceData[faceidx].idx[2] + vertex_start_index;
							primitivedata[i].shape_id = mesh->GetId();
							primitivedata[i].shape_mask = mesh->GetMask();
							primitivedata[i].prim_id = faceidx;
							primitivedata[i].type_id = 0;
							break;
						}

						case Shape::SHAPE_CURVES:
						{
							const Curves* curves = static_cast<const Curves*>(shape);
							int curvesIdx = shapeToCurves[shapeIdx];
							const int* segmentIndices = curves->GetSegmentData();
							int segmentidx = indextolook4 - bounds_start_idx;
							int vertex_start_index = curve_vertices_start_idx[curvesIdx];

							// Copy segment data to GPU buffer
							primitivedata[i].idx[0] = segmentIndices[2*segmentidx+0] + vertex_start_index;
							primitivedata[i].idx[1] = segmentIndices[2*segmentidx+1] + vertex_start_index;
							primitivedata[i].shape_id = curves->GetId();
							primitivedata[i].shape_mask = curves->GetMask();
							primitivedata[i].prim_id = segmentidx;
							primitivedata[i].type_id = 1;
							break;
						}

						// Not supported yet..
						case Shape::SHAPE_INSTANCED_CURVES: assert(0); break; 
					}
                }

                m_device->UnmapBuffer(m_gpudata->primitives, 0, primitivedata, &e);
                e->Wait();
                m_device->DeleteEvent(e);
            }

            // Make sure everything is commited
            m_device->Finish(0);
        }
    }

    void IntersectorSkipLinks::Intersect(std::uint32_t queueidx, Calc::Buffer const* rays, Calc::Buffer const* numrays, std::uint32_t maxrays, Calc::Buffer* hits, Calc::Event const* waitevent, Calc::Event** event) const
    {
        auto& func = m_gpudata->isect_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpudata->bvh);
        func->SetArg(arg++, m_gpudata->mesh_vertices);
		func->SetArg(arg++, m_gpudata->curve_vertices);
        func->SetArg(arg++, m_gpudata->primitives);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, numrays);
        func->SetArg(arg++, hits);

        size_t localsize = kWorkGroupSize;
        size_t globalsize = ((maxrays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queueidx, globalsize, localsize, event);
    }

    void IntersectorSkipLinks::Occluded(std::uint32_t queueidx, Calc::Buffer const* rays, Calc::Buffer const* numrays, std::uint32_t maxrays, Calc::Buffer* hits, Calc::Event const* waitevent, Calc::Event** event) const
    {
        auto& func = m_gpudata->occlude_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpudata->bvh);
		func->SetArg(arg++, m_gpudata->mesh_vertices);
		func->SetArg(arg++, m_gpudata->curve_vertices);
		func->SetArg(arg++, m_gpudata->primitives);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, numrays);
        func->SetArg(arg++, hits);

        size_t localsize = kWorkGroupSize;
        size_t globalsize = ((maxrays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queueidx, globalsize, localsize, event);
    }

}
