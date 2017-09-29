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
#include "intersector_2level.h"
#include "../accelerator/bvh.h"
#include "../translator/plain_bvh_translator.h"
#include "../world/world.h"
#include "../primitive/mesh.h"
#include "../primitive/instance.h"
#include "../except/except.h"

#include "device.h"
#include "executable.h"

#include <set>

static int const kWorkGroupSize = 64;

namespace RadeonRays
{
    struct IntersectorTwoLevel::ShapeData
    {
        // Shape ID
        Id id;
        // Index of root bvh node
        int bvhidx;
        int mask;
        int padding1;
        // Transform
        matrix minv;
        // Motion blur data
        float3 linearvelocity;
        // Angular veocity (quaternion)
        quaternion angularvelocity;
    };

    struct IntersectorTwoLevel::Face
    {
        // Up to 3 indices
        int idx[3];
        // Shape maks
        int shape_mask;
        // Shape ID
        int shape_id;
        // Primitive ID
        int prim_id;
    };

    struct IntersectorTwoLevel::GpuData
    {
        // Device
        Calc::Device* device;
        // BVH nodes
        Calc::Buffer* bvh;
        // Vertex positions
        Calc::Buffer* vertices;
        // Indices
        Calc::Buffer* faces;
        // Shape IDs
        Calc::Buffer* shapes;

        int bvhrootidx;

        Calc::Executable* executable;
        Calc::Function* isect_func;
        Calc::Function* occlude_func;

        GpuData(Calc::Device* d)
            : device(d)
            , bvh(nullptr)
            , vertices(nullptr)
            , faces(nullptr)
            , shapes(nullptr)
            , bvhrootidx(-1)
            , executable(nullptr)
            , isect_func(nullptr)
            , occlude_func(nullptr)
        {
        }

        ~GpuData()
        {
            device->DeleteBuffer(bvh);
            device->DeleteBuffer(vertices);
            device->DeleteBuffer(faces);
            device->DeleteBuffer(shapes);
            if(executable != nullptr)
            {
                executable->DeleteFunction(isect_func);
                executable->DeleteFunction(occlude_func);
                device->DeleteExecutable(executable);
            }
        }
    };


    struct IntersectorTwoLevel::CpuData
    {
        std::vector<int> mesh_vertices_start_idx;
        std::vector<int> mesh_faces_start_idx;
        std::vector<Bvh const*> bvhptrs;
        std::vector<ShapeData> shapedata;
        std::vector<bbox> bounds;

        PlainBvhTranslator translator;
    };

    IntersectorTwoLevel::IntersectorTwoLevel(Calc::Device* device)
        : Intersector(device)
        , m_gpudata(new GpuData(device))
        , m_cpudata(new CpuData)
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

            int numheaders = sizeof(headers) / sizeof(char const*);

            m_gpudata->executable = m_device->CompileExecutable("../RadeonRays/src/kernels/CL/intersect_bvh2level_skiplinks.cl", headers, numheaders, buildopts.c_str());
        }
        else
        {
            assert( device->GetPlatform() == Calc::Platform::kVulkan );
            m_gpudata->executable = m_device->CompileExecutable( "../RadeonRays/src/kernels/GLSL/bvh2l.comp", nullptr, 0, buildopts.c_str());
        }

#else
#if USE_OPENCL
        if (device->GetPlatform() == Calc::Platform::kOpenCL)
        {
            m_gpudata->executable = m_device->CompileExecutable(g_intersect_bvh2level_skiplinks_opencl, std::strlen(g_intersect_bvh2level_skiplinks_opencl), buildopts.c_str());
        }
#endif

#if USE_VULKAN
        if (m_gpudata->executable == nullptr && device->GetPlatform() == Calc::Platform::kVulkan)
        {
            m_gpudata->executable = m_device->CompileExecutable(g_bvh2l_vulkan, std::strlen(g_bvh2l_vulkan), buildopts.c_str());
        }
#endif
#endif

        m_gpudata->isect_func = m_gpudata->executable->CreateFunction("intersect_main");
        m_gpudata->occlude_func = m_gpudata->executable->CreateFunction("occluded_main");
    }

    void IntersectorTwoLevel::Process(World const& world)
    {
        // If something has been changed we need to rebuild BVH
        int statechange = world.GetStateChange();

        // Full rebuild in case number of objects changes
        if (m_bvhs.size() == 0 || world.has_changed())
        {
            if (m_bvhs.size() != 0)
            {
                m_device->DeleteBuffer(m_gpudata->bvh);
                m_device->DeleteBuffer(m_gpudata->vertices);
                m_device->DeleteBuffer(m_gpudata->faces);
                m_device->DeleteBuffer(m_gpudata->shapes);
            }


            auto builder = world.options_.GetOption("bvh.builder");
            auto tcost = world.options_.GetOption("bvh.sah.traversal_cost");
            auto nbins = world.options_.GetOption("bvh.sah.num_bins");

            bool use_sah = false;
            float traversal_cost = tcost ? tcost->AsFloat() : 10.f;
            int num_bins = nbins ? (int)nbins->AsFloat() : 64;


            if (builder && builder->AsString() == "sah")
            {
                use_sah = true;
            }

            // Copy the shapes here to be able to partition them and handle more efficiently
            // #22: we need to be able to handle instances whos base shapes are not present 
            // in the scene, so we have to add them manually here.
            std::vector<Shape const*> shapes;
            std::set<Shape const*> shapes_disabled;

            for (auto s : world.shapes_)
            {
                auto shapeimpl = static_cast<ShapeImpl const*>(s);

                if (shapeimpl->is_instance())
                {
                    // Here we know this is an instance, need to check if its base shape has been added as well
                    auto instance = static_cast<Instance const*>(shapeimpl);
                    auto base_shape = instance->GetBaseShape();

                    if (std::find(world.shapes_.cbegin(), world.shapes_.cend(), base_shape) == world.shapes_.cend())
                    {
                        // Need to add the shape to the list
                        shapes.push_back(base_shape);
                        // And mark it disabled
                        shapes_disabled.insert(base_shape);
                    }
                }

                shapes.push_back(s);
            }

            // Now partition the range into meshes and instances
            auto firstinst = std::partition(shapes.begin(), shapes.end(), [&](Shape const* shape)
            {
                return !static_cast<ShapeImpl const*>(shape)->is_instance();
            });

            // Count the number of meshes
            int nummeshes = (int)std::distance(shapes.begin(), firstinst);
            // Count the number of instances
            int numinstances = (int)std::distance(firstinst, shapes.end());

            int numvertices = 0;
            int numfaces = 0;

            // This buffer tracks mesh start index for next stage as mesh face indices are relative to 0
            m_cpudata->mesh_vertices_start_idx.resize(nummeshes);
            m_cpudata->mesh_faces_start_idx.resize(nummeshes);
            m_cpudata->bvhptrs.resize(nummeshes + 1);
            m_cpudata->shapedata.resize(nummeshes + numinstances);

            // [0...numshapes-1] contain bottom level BVHs
            // [numshapes] is the top level one
            m_bvhs.resize(nummeshes + 1);
            // Create actual BVH objects
            for (int i = 0; i < nummeshes + 1; ++i)
            {
                m_bvhs[i].reset(new Bvh(traversal_cost, num_bins, use_sah));
                m_cpudata->bvhptrs[i] = m_bvhs[i].get();
            }

            // Prepare necessary offsets in the arrays
            // in order to be able to parallelize
            for (int i = 0; i < nummeshes; ++i)
            {
                Mesh const* mesh = static_cast<Mesh const*>(shapes[i]);

                m_cpudata->mesh_faces_start_idx[i] = numfaces;
                m_cpudata->mesh_vertices_start_idx[i] = numvertices;

                numfaces += mesh->num_faces();
                numvertices += mesh->num_vertices();
            }

            // We can't avoild allocating it here, since bounds aren't stored anywhere
            m_cpudata->bounds.resize(numfaces);

            // We are storing individual object bounds here to build top level BVH
            std::vector<bbox> object_bounds(nummeshes + numinstances);

            matrix m, minv;

            // Handle simple shapes
#pragma omp parallel for
            for (int i = 0; i < nummeshes; ++i)
            {

                Mesh const* mesh = static_cast<Mesh const*>(shapes[i]);
                // Get transform to apply to object bounds
                mesh->GetTransform(m, minv);

                for (int j = 0; j < mesh->num_faces(); ++j)
                {
                    // Request bounds in object space since we build BVHs for objects locally
                    mesh->GetFaceBounds(j, true, m_cpudata->bounds[m_cpudata->mesh_faces_start_idx[i] + j]);
                }

                // Build BVH for current mesh
                m_bvhs[i]->Build(&m_cpudata->bounds[m_cpudata->mesh_faces_start_idx[i]], mesh->num_faces());

                // Extract and store bounds. Note they are in object space and we need to translate them to world space
                object_bounds[i] = transform_bbox(m_bvhs[i]->Bounds(), m);

                // Collect BVH pointers for toip level build
                m_cpudata->bvhptrs[i] = m_bvhs[i].get();
            }

            // Handle instances
#pragma omp parallel for
            for (int i = nummeshes; i < nummeshes + numinstances; ++i)
            {

                Instance const* instance = static_cast<Instance const*>(shapes[i]);
                // Get transform to apply to object bounds
                instance->GetTransform(m, minv);

                // Find BVH for the instance
                Mesh const* basemesh = static_cast<Mesh const*>(instance->GetBaseShape());

                // It should be there
                auto iter = std::find(shapes.cbegin(), shapes.cbegin() + nummeshes, basemesh);

                // TODO: should be assert
                ThrowIf(iter == shapes.cbegin() + nummeshes, "Internal error");

                int bvhidx = (int)std::distance(shapes.cbegin(), iter);

                // Extract and store bounds. Note they are in object space and we need to translate them to world space
                object_bounds[i] = transform_bbox(m_bvhs[bvhidx]->Bounds(), m);
            }

            // Calculate top level BVH
            m_bvhs[nummeshes]->Build(&object_bounds[0], nummeshes + numinstances);
            m_cpudata->bvhptrs[nummeshes] = m_bvhs[nummeshes].get();

            m_cpudata->translator.Flush();
            // TODO: parallelize this
            m_cpudata->translator.Process(&m_cpudata->bvhptrs[0], &m_cpudata->mesh_faces_start_idx[0], nummeshes);

            // Update GPU data
            // Copy translated nodes first
            m_gpudata->bvh = m_device->CreateBuffer(m_cpudata->translator.nodes_.size() * sizeof(PlainBvhTranslator::Node), Calc::kRead, &m_cpudata->translator.nodes_[0]);
            m_gpudata->bvhrootidx = m_cpudata->translator.root_;

            // Create vertex buffer
            {
                // Vertices
                m_gpudata->vertices = m_device->CreateBuffer(numvertices * sizeof(float3), Calc::kRead);

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
                for (int i = 0; i < nummeshes; ++i)
                {
                    // Get the mesh
                    Mesh const* mesh = static_cast<Mesh const*>(shapes[i]);
                    // Get vertex buffer of the current mesh
                    float3 const* myvertexdata = mesh->GetVertexData();

                    // Iterate thru vertices multiply and append them to GPU buffer
                    for (int j = 0; j < mesh->num_vertices(); ++j)
                    {
                        vertexdata[m_cpudata->mesh_vertices_start_idx[i] + j] = myvertexdata[j];
                    }
                }

                m_device->UnmapBuffer(m_gpudata->vertices, 0, vertexdata, &e);

                e->Wait();
                m_device->DeleteEvent(e);
            }

            // Create face buffer
            {
                // Create face buffer
                m_gpudata->faces = m_device->CreateBuffer(numfaces * sizeof(Face), Calc::kRead);

                // Get the pointer to mapped data
                Face* facedata = nullptr;
                Calc::Event* e = nullptr;

                m_device->MapBuffer(m_gpudata->faces, 0, 0, numfaces * sizeof(Face), Calc::MapType::kMapWrite, (void**)&facedata, &e);

                e->Wait();
                m_device->DeleteEvent(e);

                // Here the point is to add mesh starting index to actual index contained within the mesh,
                // getting absolute index in the buffer.
                // Besides that we need to permute the faces accorningly to BVH reordering, whihc
                // is contained within bvh.primids_

#pragma omp parallel for
                for (int i = 0; i < nummeshes; ++i)
                {
                    // Reordering indices for a given mesh
                    int const* reordering = m_bvhs[i]->GetIndices();

                    // Get the mesh
                    Mesh const* mesh = static_cast<Mesh const*>(shapes[i]);

                    Mesh::Face const* myfaces = mesh->GetFaceData();

                    int startidx = m_cpudata->mesh_vertices_start_idx[i];

                    for (int j = 0; j < mesh->num_faces(); ++j)
                    {
                        // Copy face data to GPU buffer
                        int myidx = m_cpudata->mesh_faces_start_idx[i] + j;
                        int faceidx = reordering[j];

                        facedata[myidx].idx[0] = myfaces[faceidx].idx[0] + startidx;
                        facedata[myidx].idx[1] = myfaces[faceidx].idx[1] + startidx;
                        facedata[myidx].idx[2] = myfaces[faceidx].idx[2] + startidx;

                        facedata[myidx].shape_id = mesh->GetId();
                        facedata[myidx].prim_id = faceidx;
                    }
                }

                m_device->UnmapBuffer(m_gpudata->faces, 0, facedata, &e);

                e->Wait();
                m_device->DeleteEvent(e);
            }


            // Now we need to collect shapdata
            int const* topindices = m_bvhs[nummeshes]->GetIndices();

#pragma omp parallel for
            for (int i = 0; i < nummeshes + numinstances; ++i)
            {
                // Get the mesh
                ShapeImpl const* shapeimpl = static_cast<ShapeImpl const*>(shapes[topindices[i]]);

                m_cpudata->shapedata[i].id = shapeimpl->GetId();

                // For disabled shapes force mask to zero since these shapes 
                // present only virtually (they have not been added to the scene)
                // and we need to skip them while doing traversal.
                if (shapes_disabled.find(shapeimpl) == shapes_disabled.cend())
                {
                    m_cpudata->shapedata[i].mask = shapeimpl->GetMask();
                }
                else
                {
                    m_cpudata->shapedata[i].mask = 0x0;
                }

                shapeimpl->GetTransform(m, m_cpudata->shapedata[i].minv);

                if (!shapeimpl->is_instance())
                {
                    m_cpudata->shapedata[i].bvhidx = m_cpudata->translator.roots_[topindices[i]];
                }
                else
                {
                    Instance const* instance = static_cast<Instance const*>(shapes[topindices[i]]);

                    // Find corresponding mesh
                    Mesh const* basemesh = static_cast<Mesh const*>(instance->GetBaseShape());

                    // It should be there
                    auto iter = std::find(shapes.cbegin(), shapes.cbegin() + nummeshes, basemesh);

                    // TODO: should be assert
                    ThrowIf(iter == shapes.cbegin() + nummeshes, "Internal error");

                    int bvhidx = (int)std::distance(shapes.cbegin(), iter);

                    m_cpudata->shapedata[i].bvhidx = m_cpudata->translator.roots_[bvhidx];
                }
            }

            // Create face ID buffer
            m_gpudata->shapes = m_device->CreateBuffer((nummeshes + numinstances) * sizeof(ShapeData), Calc::kRead, &m_cpudata->shapedata[0]);
        }
        // Refit
        else if (statechange != ShapeImpl::kStateChangeNone)
        {
            //std::cout << "Refit\n";

            // Copy the shapes here to be able to partition them and handle more efficiently
            // #22: we need to be able to handle instances whos base shapes are not present 
            // in the scene, so we have to add them manually here.
            std::vector<Shape const*> shapes;
            std::set<Shape const*> shapes_disabled;

            for (auto s : world.shapes_)
            {
                auto shapeimpl = static_cast<ShapeImpl const*>(s);

                if (shapeimpl->is_instance())
                {
                    // Here we know this is an instance, need to check if its base shape has been added as well
                    auto instance = static_cast<Instance const*>(shapeimpl);
                    auto base_shape = instance->GetBaseShape();

                    if (std::find(world.shapes_.cbegin(), world.shapes_.cend(), base_shape) == world.shapes_.cend())
                    {
                        // Need to add the shape to the list
                        shapes.push_back(base_shape);
                        // And mark it disabled
                        shapes_disabled.insert(base_shape);
                    }
                }

                shapes.push_back(s);
            }

            // Now partition the range into meshes and instances
            auto firstinst = std::partition(shapes.begin(), shapes.end(), [&](Shape const* shape)
            {
                return !static_cast<ShapeImpl const*>(shape)->is_instance();
            });

            // Count the number of meshes
            int nummeshes = (int)std::distance(shapes.begin(), firstinst);
            // Count the number of instances
            int numinstances = (int)std::distance(firstinst, shapes.end());

            std::vector<bbox> object_bounds(nummeshes + numinstances);

            matrix m, minv;

            // Go over meshes and rebuild BVH bounds
#pragma omp parallel for
            for (int i = 0; i < nummeshes; ++i)
            {
                Mesh const* mesh = static_cast<Mesh const*>(shapes[i]);
                // Get transform to apply to object bounds
                mesh->GetTransform(m, minv);
                // Extract and store bounds. Note they are in object space and we need to translate them to world space
                object_bounds[i] = transform_bbox(m_bvhs[i]->Bounds(), m);
            }

#pragma omp parallel for
            for (int i = nummeshes; i < nummeshes + numinstances; ++i)
            {

                Instance const* instance = static_cast<Instance const*>(shapes[i]);
                // Get transform to apply to object bounds
                instance->GetTransform(m, minv);

                // Find BVH for the instance
                Mesh const* basemesh = static_cast<Mesh const*>(instance->GetBaseShape());

                // It should be there
                auto iter = std::find(shapes.cbegin(), shapes.cbegin() + nummeshes, basemesh);

                // TODO: should be assert
                ThrowIf(iter == shapes.cbegin() + nummeshes, "Internal error");

                int bvhidx = (int)std::distance(shapes.cbegin(), iter);

                // Extract and store bounds. Note they are in object space and we need to translate them to world space
                object_bounds[i] = transform_bbox(m_bvhs[bvhidx]->Bounds(), m);
            }

            // Calculate top level BVH
            auto builder = world.options_.GetOption("bvh.builder");
            auto tcost = world.options_.GetOption("bvh.sah.traversal_cost");
            auto nbins = world.options_.GetOption("bvh.sah.num_bins");

            bool use_sah = false;
            float traversal_cost = tcost ? tcost->AsFloat() : 10.f;
            int num_bins = nbins ? (int)nbins->AsFloat() : 64;


            if (builder && builder->AsString() == "sah")
            {
                use_sah = true;
            }

            m_bvhs[nummeshes].reset(new Bvh(traversal_cost, num_bins, use_sah));
            m_bvhs[nummeshes]->Build(&object_bounds[0], nummeshes + numinstances);
            m_cpudata->bvhptrs[nummeshes] = m_bvhs[nummeshes].get();


            // TODO: parallelize this
            m_cpudata->translator.UpdateTopLevel(*m_bvhs[nummeshes]);

            // Update GPU data
            // Copy only top BVH data
            Calc::Event* e = nullptr;
            m_device->WriteBuffer(m_gpudata->bvh, 0, m_cpudata->translator.root_ * sizeof(PlainBvhTranslator::Node), (2 * (nummeshes + numinstances) - 1) * sizeof(PlainBvhTranslator::Node), (char*)&m_cpudata->translator.nodes_[m_cpudata->translator.root_], &e);

            e->Wait();
            m_device->DeleteEvent(e);

            // Now we need to collect shapdata
            int const* topindices = m_bvhs[nummeshes]->GetIndices();

#pragma omp parallel for
            for (int i = 0; i < nummeshes + numinstances; ++i)
            {
                // Get the mesh
                ShapeImpl const* shapeimpl = static_cast<ShapeImpl const*>(shapes[topindices[i]]);

                m_cpudata->shapedata[i].id = shapeimpl->GetId();
                // For disabled shapes force mask to zero since these shapes 
                // present only virtually (they have not been added to the scene)
                // and we need to skip them while doing traversal.
                if (shapes_disabled.find(shapeimpl) == shapes_disabled.cend())
                {
                    m_cpudata->shapedata[i].mask = shapeimpl->GetMask();
                }
                else
                {
                    m_cpudata->shapedata[i].mask = 0x0;
                }

                shapeimpl->GetTransform(m, m_cpudata->shapedata[i].minv);

                if (!shapeimpl->is_instance())
                {
                    m_cpudata->shapedata[i].bvhidx = m_cpudata->translator.roots_[topindices[i]];
                }
                else
                {
                    Instance const* instance = static_cast<Instance const*>(shapes[topindices[i]]);

                    // Find corresponding mesh
                    Mesh const* basemesh = static_cast<Mesh const*>(instance->GetBaseShape());

                    // It should be there
                    auto iter = std::find(shapes.cbegin(), shapes.cbegin() + nummeshes, basemesh);

                    // TODO: should be assert
                    ThrowIf(iter == shapes.cbegin() + nummeshes, "Internal error");

                    int bvhidx = (int)std::distance(shapes.cbegin(), iter);

                    m_cpudata->shapedata[i].bvhidx = m_cpudata->translator.roots_[bvhidx];
                }
            }

            // Create face ID buffer
            m_device->WriteBuffer(m_gpudata->shapes, 0, 0, (nummeshes + numinstances) * sizeof(ShapeData), (char*)&m_cpudata->shapedata[0], &e);

            e->Wait();
            m_device->DeleteEvent(e);

            m_device->Finish(0);
        }
    }

    void IntersectorTwoLevel::Intersect(std::uint32_t queueidx, Calc::Buffer const* rays, Calc::Buffer const* numrays, std::uint32_t maxrays, Calc::Buffer* hits, Calc::Event const* waitevent, Calc::Event** event) const
    {
        auto& func = m_gpudata->isect_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpudata->bvh);
        func->SetArg(arg++, m_gpudata->vertices);
        func->SetArg(arg++, m_gpudata->faces);
        func->SetArg(arg++, m_gpudata->shapes);
        func->SetArg(arg++, sizeof(int), &m_gpudata->bvhrootidx);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, numrays);
        func->SetArg(arg++, hits);

        size_t localsize = kWorkGroupSize;
        size_t globalsize = ((maxrays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queueidx, globalsize, localsize, event);
    }

    void IntersectorTwoLevel::Occluded(std::uint32_t queueidx, Calc::Buffer const* rays, Calc::Buffer const* numrays, std::uint32_t maxrays, Calc::Buffer* hits, Calc::Event const* waitevent, Calc::Event** event) const
    {
        auto& func = m_gpudata->occlude_func;

        // Set args
        int arg = 0;

        func->SetArg(arg++, m_gpudata->bvh);
        func->SetArg(arg++, m_gpudata->vertices);
        func->SetArg(arg++, m_gpudata->faces);
        func->SetArg(arg++, m_gpudata->shapes);
        func->SetArg(arg++, sizeof(int), &m_gpudata->bvhrootidx);
        func->SetArg(arg++, rays);
        func->SetArg(arg++, numrays);
        func->SetArg(arg++, hits);

        size_t localsize = kWorkGroupSize;
        size_t globalsize = ((maxrays + kWorkGroupSize - 1) / kWorkGroupSize) * kWorkGroupSize;

        m_device->Execute(func, queueidx, globalsize, localsize, event);
    }
}
