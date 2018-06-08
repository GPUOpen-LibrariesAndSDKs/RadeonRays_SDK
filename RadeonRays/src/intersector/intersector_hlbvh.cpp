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
#include "intersector_hlbvh.h"

#include "../accelerator/hlbvh.h"
#include "../primitive/mesh.h"
#include "../world/world.h"
#include "../translator/plain_bvh_translator.h"


#include "device.h"
#include "executable.h"
#include "../except/except.h"

#include <algorithm>
#include <memory>

// Preferred work group size for Radeon devices
static int const kWorkGroupSize = 64;
static int const kMaxStackSize = 48;
static int const kMaxBatchSize = 2048 * 2048;

namespace RadeonRays
{
    struct IntersectorHlbvh::GpuData
    {
        // Device
        Calc::Device* device;
        // Vertex positions
        Calc::Buffer* vertices;
        // Indices
        Calc::Buffer* faces;
        // Traversal stack
        Calc::Buffer* stack;

        Calc::Executable* executable;
        Calc::Function* isect_func;
        Calc::Function* occlude_func;

        GpuData(Calc::Device* d)
            : device(d)
            , vertices(nullptr)
            , faces(nullptr)
        {
        }

        ~GpuData()
        {
            device->DeleteBuffer(vertices);
            device->DeleteBuffer(faces);
            device->DeleteBuffer(stack);
            executable->DeleteFunction(isect_func);
            executable->DeleteFunction(occlude_func);
            device->DeleteExecutable(executable);
        }
    };

    IntersectorHlbvh::IntersectorHlbvh(Calc::Device* device)
        : Intersector(device)
        , m_gpudata(new GpuData(device))
        , m_bvh(nullptr)
    {
        std::string buildopts;
#ifdef RR_RAY_MASK
        buildopts.append("-D RR_RAY_MASK ");
#endif

#ifdef RR_BACKFACE_CULL
        buildopts.append("-D RR_BACKFACE_CULL ");
#endif // RR_BACKFACE_CULL

#ifdef USE_SAFE_MATH
        buildopts.append("-D USE_SAFE_MATH ");
#endif

#ifndef RR_EMBED_KERNELS
        if ( device->GetPlatform() == Calc::Platform::kOpenCL )
        {
            char const* headers[] = { "../RadeonRays/src/kernels/CL/common.cl" };

            int numheaders = sizeof( headers ) / sizeof( char const* );

            m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/src/kernels/CL/intersect_hlbvh_stack.cl", headers, numheaders, buildopts.c_str());
        }
        else
        {
            assert( device->GetPlatform() == Calc::Platform::kVulkan );
            m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/src/kernels/GLSL/hlbvh.comp", nullptr, 0, buildopts.c_str());
        }
#else
#if USE_OPENCL
        if (device->GetPlatform() == Calc::Platform::kOpenCL)
        {
            m_gpudata->executable = m_device->CompileExecutable(g_intersect_hlbvh_stack_opencl, std::strlen(g_intersect_hlbvh_stack_opencl), buildopts.c_str());
        }
#endif

#if USE_VULKAN
        if (m_gpudata->executable == nullptr && device->GetPlatform() == Calc::Platform::kVulkan)
        {
            m_gpudata->executable = m_device->CompileExecutable(g_hlbvh_build_vulkan, std::strlen(g_hlbvh_vulkan), buildopts.c_str());
        }
#endif

#endif

        m_gpudata->isect_func = m_gpudata->executable->CreateFunction("intersect_main");
        m_gpudata->occlude_func = m_gpudata->executable->CreateFunction("occluded_main");
    }

    void IntersectorHlbvh::Process(World const& world)
    {
        // If something has been changed we need to rebuild BVH
        if (!m_bvh || world.has_changed())
        {
            if (m_bvh)
            {
                m_device->DeleteBuffer(m_gpudata->vertices);
                m_device->DeleteBuffer(m_gpudata->faces);
                m_device->DeleteBuffer(m_gpudata->stack);
            }
            
            int numshapes = (int)world.shapes_.size();
            int numvertices = 0;
            int numfaces = 0;

            // This buffer tracks mesh start index for next stage as mesh face indices are relative to 0
            std::vector<int> mesh_vertices_start_idx(numshapes);
            std::vector<int> mesh_faces_start_idx(numshapes);

            //
            m_bvh = std::make_unique<Hlbvh>(m_device);

            // Here we now that only Meshes are present, otherwise 2level strategy would have been used
            for (int i = 0; i < numshapes; ++i)
            {
                Mesh const* mesh = static_cast<Mesh const*>(world.shapes_[i]);

                mesh_faces_start_idx[i] = numfaces;
                mesh_vertices_start_idx[i] = numvertices;

                numfaces += mesh->num_faces();
                numvertices += mesh->num_vertices();
            }

            // We can't avoid allocating it here, since bounds aren't stored anywhere
            std::vector<bbox> bounds(numfaces);

#pragma omp parallel for
            for (int i = 0; i < numshapes; ++i)
            {
                Mesh const* mesh = static_cast<Mesh const*>(world.shapes_[i]);

                for (int j = 0; j < mesh->num_faces(); ++j)
                {
                    mesh->GetFaceBounds(j, false, bounds[mesh_faces_start_idx[i] + j]);
                }
            }

            m_bvh->Build(&bounds[0], numfaces);

            // Create vertex buffer
            {
                // Vertices
                m_gpudata->vertices = m_device->CreateBuffer(numvertices * sizeof(float3), Calc::BufferType::kRead);

                // Get the pointer to mapped data
                float3* vertexdata = nullptr;
                Calc::Event* e = nullptr;

                m_device->MapBuffer(m_gpudata->vertices, 0, 0, numvertices * sizeof(float3), Calc::MapType::kMapWrite, (void**)&vertexdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Here we need to put data in world space rather than object space
                // So we need to get the transform from the mesh and multiply each vertex
                matrix m, minv;

#pragma omp parallel for
                for (int i = 0; i < numshapes; ++i)
                {
                    // Get the mesh
                    Mesh const* mesh = static_cast<Mesh const*>(world.shapes_[i]);
                    // Get vertex buffer of the current mesh
                    float3 const* myvertexdata = mesh->GetVertexData();
                    // Get mesh transform
                    mesh->GetTransform(m, minv);

                    //#pragma omp parallel for
                    // Iterate thru vertices multiply and append them to GPU buffer
                    for (int j = 0; j < mesh->num_vertices(); ++j)
                    {
                        vertexdata[mesh_vertices_start_idx[i] + j] = transform_point(myvertexdata[j], m);
                    }
                }
                m_device->UnmapBuffer(m_gpudata->vertices, 0, vertexdata, &e); 

                e->Wait();
                m_device->DeleteEvent(e);
            }


                // Create face buffer
            {
                struct Face
                {
                    // Up to 3 indices
                    int idx[3];
                    // Shape ID
                    int shape_id;
                    // Primitive ID
                    int prim_id;
                };

                // Create face buffer
                m_gpudata->faces = m_device->CreateBuffer(numfaces * sizeof(Face), Calc::BufferType::kRead);

                // Get the pointer to mapped data
                Face* facedata = nullptr;
                Calc::Event* e = nullptr;

                m_device->MapBuffer(m_gpudata->faces, 0, 0, numfaces * sizeof(Face), Calc::BufferType::kWrite, (void**)&facedata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Here the point is to add mesh starting index to actual index contained within the mesh,
                // getting absolute index in the buffer.
                // Besides that we need to permute the faces accorningly to BVH reordering, whihc
                // is contained within bvh.primids_
                for (int i = 0; i < numfaces; ++i)
                {
                    int indextolook4 = i;

                    // We need to find a shape corresponding to current face
                    auto iter = std::upper_bound(mesh_faces_start_idx.cbegin(), mesh_faces_start_idx.cend(), indextolook4);

                    // Find the index of the shape
                    int shapeidx = static_cast<int>(std::distance(mesh_faces_start_idx.cbegin(), iter) - 1);

                    // Get the mesh directly or out of instance
                    Mesh const* mesh = static_cast<Mesh const*>(world.shapes_[shapeidx]);

                    // Get vertex buffer of the current mesh
                    Mesh::Face const* myfacedata = mesh->GetFaceData();
                    // Find face idx
                    int faceidx = indextolook4 - mesh_faces_start_idx[shapeidx];
                    // Find mesh start idx
                    int mystartidx = mesh_vertices_start_idx[shapeidx];

                    // Copy face data to GPU buffer
                    facedata[i].idx[0] = myfacedata[faceidx].idx[0] + mystartidx;
                    facedata[i].idx[1] = myfacedata[faceidx].idx[1] + mystartidx;
                    facedata[i].idx[2] = myfacedata[faceidx].idx[2] + mystartidx;

                    // Optimization: we are putting faceid here
                    facedata[i].shape_id = mesh->GetId();
                    facedata[i].prim_id = faceidx;
                }

                m_device->UnmapBuffer(m_gpudata->faces, 0, facedata, &e);
                e->Wait();
                m_device->DeleteEvent(e);
            }

            // Stack
            m_gpudata->stack = m_device->CreateBuffer(kMaxBatchSize*kMaxStackSize, Calc::BufferType::kWrite);
            // Make sure everything is commited
            m_device->Finish(0);
        }
        else if (world.GetStateChange() != ShapeImpl::kStateChangeNone)
        {
            int numshapes = (int)world.shapes_.size();
            int numvertices = 0;
            int numfaces = 0;

            // This buffer tracks mesh start index for next stage as mesh face indices are relative to 0
            std::vector<int> mesh_vertices_start_idx(numshapes);
            std::vector<int> mesh_faces_start_idx(numshapes);

            //
            //bvh_.reset(new Hlbvh(context_));

            // Here we now that only Meshes are present, otherwise 2level strategy would have been used
            for (int i = 0; i < numshapes; ++i)
            {
                Mesh const* mesh = static_cast<Mesh const*>(world.shapes_[i]);

                mesh_faces_start_idx[i] = numfaces;
                mesh_vertices_start_idx[i] = numvertices;

                numfaces += mesh->num_faces();
                numvertices += mesh->num_vertices();
            }

            // We can't avoid allocating it here, since bounds aren't stored anywhere
            std::vector<bbox> bounds(numfaces);

#pragma omp parallel for
            for (int i = 0; i < numshapes; ++i)
            {
                Mesh const* mesh = static_cast<Mesh const*>(world.shapes_[i]);

                for (int j = 0; j < mesh->num_faces(); ++j)
                {
                    mesh->GetFaceBounds(j, false, bounds[mesh_faces_start_idx[i] + j]);
                }
            }

            m_bvh->Build(&bounds[0], numfaces);

            // Create vertex buffer
            {
                // Vertices
                m_gpudata->vertices = m_device->CreateBuffer(numvertices * sizeof(float3), Calc::BufferType::kRead);

                // Get the pointer to mapped data
                float3* vertexdata = nullptr;
                Calc::Event* e = nullptr;

                m_device->MapBuffer(m_gpudata->vertices, 0, 0, numvertices * sizeof(float3), Calc::MapType::kMapWrite, (void**)&vertexdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Here we need to put data in world space rather than object space
                // So we need to get the transform from the mesh and multiply each vertex
                matrix m, minv;

#pragma omp parallel for
                for (int i = 0; i < numshapes; ++i)
                {
                    // Get the mesh
                    Mesh const* mesh = static_cast<Mesh const*>(world.shapes_[i]);
                    // Get vertex buffer of the current mesh
                    float3 const* myvertexdata = mesh->GetVertexData();
                    // Get mesh transform
                    mesh->GetTransform(m, minv);

                    //#pragma omp parallel for
                    // Iterate thru vertices multiply and append them to GPU buffer
                    for (int j = 0; j < mesh->num_vertices(); ++j)
                    {
                        vertexdata[mesh_vertices_start_idx[i] + j] = transform_point(myvertexdata[j], m);
                    }
                }
                m_device->UnmapBuffer(m_gpudata->vertices, 0, vertexdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);
            }
        }
    }


    void IntersectorHlbvh::Intersect(std::uint32_t queue_idx, Calc::Buffer const* rays, Calc::Buffer const* num_rays, std::uint32_t max_rays, Calc::Buffer* hits, Calc::Event const* wait_event, Calc::Event** event) const
    {
        // Check if we can allocate enough stack memory
        if (max_rays >= kMaxBatchSize)
        {
            throw ExceptionImpl("hlbvh accelerator max batch size exceeded");
        }

        auto& func = m_gpudata->isect_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_bvh->GetGpuData().nodes);
        func->SetArg(arg++, m_bvh->GetGpuData().sorted_bounds);
        func->SetArg(arg++, m_gpudata->vertices);
        func->SetArg(arg++, m_gpudata->faces);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, num_rays);
        func->SetArg(arg++, m_gpudata->stack);
        func->SetArg(arg++, hits);

        size_t localsize = kWorkGroupSize;
        size_t globalsize = ((max_rays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queue_idx, globalsize, localsize, event);
    }

    void IntersectorHlbvh::Occluded(std::uint32_t queue_idx, Calc::Buffer const* rays, Calc::Buffer const* num_rays, std::uint32_t max_rays, Calc::Buffer* hits, Calc::Event const* waitevent, Calc::Event** event) const
    {
        // Check if we can allocate enough stack memory
        if (max_rays >= kMaxBatchSize)
        {
            throw ExceptionImpl("hlbvh accelerator max batch size exceeded");
        }

        auto& func = m_gpudata->occlude_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_bvh->GetGpuData().nodes);
        func->SetArg(arg++, m_bvh->GetGpuData().sorted_bounds);
        func->SetArg(arg++, m_gpudata->vertices);
        func->SetArg(arg++, m_gpudata->faces);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, num_rays);
        func->SetArg(arg++, m_gpudata->stack);
        func->SetArg(arg++, hits);

        size_t localsize = kWorkGroupSize;
        size_t globalsize = ((max_rays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queue_idx, globalsize, localsize, event);
    }

}
