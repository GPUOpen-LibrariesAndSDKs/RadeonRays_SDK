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
#include "embree_intersection_device.h"

#include <iostream>
#include <future>
#include <thread>
#include "../world/world.h"
#include "../primitive/mesh.h"
#include "../primitive/instance.h"
#include "buffer.h"
#include "device.h"
#include "event.h"
#include "../except/except.h"
#include "embree2/rtcore.h"
#include "embree2/rtcore_ray.h"
#include "../async/thread_pool.h"

#include <xmmintrin.h>
#include <pmmintrin.h>

//count of elements for one thread pool task
#define TASK_SIZE 256

//switch between rtcIntersect4 and rtcIntercetN
//#define INTERSECTN


namespace RadeonRays
{
    //simple RadeonRays::Buffer implementation
    class EmbreeBuffer : public Buffer
    {
    public:
        EmbreeBuffer(size_t size, void* init)
            : m_data(nullptr)
        {
            m_data = new char[size];
            if (init)
                memcpy(m_data, init, size);
        }
        virtual ~EmbreeBuffer()
        {
            delete[] m_data;
            m_data = nullptr;
        }

        void* GetData()
        {
            return m_data;
        }

        const void* GetData() const
        {
            return m_data;
        }

    private:
        void* m_data;
    };

    //simple RadeonRays::Event implementation
    class EmbreeEvent : public Event
    {
    public:
        EmbreeEvent(std::function<void()>&& f)
            : m_ftr()
        {
            std::packaged_task<void()> task(std::move(f));
            m_ftr = task.get_future();
            std::thread(std::move(task)).detach();
        }

        virtual ~EmbreeEvent()
        {
            Wait();
        }

        virtual bool Complete() const
        {
            return m_ftr.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
        }

        virtual void Wait()
        {
            m_ftr.wait();
        }
    private:
        std::future<void> m_ftr;
    };

    EmbreeIntersectionDevice::EmbreeIntersectionDevice()
        : m_pool(1)
    {
        m_device = rtcNewDevice(nullptr);
        RTCError result = rtcDeviceGetError(m_device);
        if (result != RTC_NO_ERROR)
            std::cout << "Failed to create embree rtcDevice: " << result << std::endl;

        m_scene = rtcDeviceNewScene(m_device, RTC_SCENE_STATIC, RTC_INTERSECT1 | RTC_INTERSECT4 | RTC_INTERSECT8 | RTC_INTERSECT16 );
        result = rtcDeviceGetError(m_device);
        if (result != RTC_NO_ERROR)
            std::cout << "Failed to create embree scene: " << result << std::endl;
    }
    
    EmbreeIntersectionDevice::~EmbreeIntersectionDevice()
    {
        if (m_device)
        {
            rtcDeleteDevice(m_device);
            m_device = nullptr;
        }

    }

    void EmbreeIntersectionDevice::Preprocess(World const& world)
    {
        for (auto& it : m_instances)
            it.second.updated = false;

        //checking removed shapes
        for (auto it = world.shapes_.begin(); it != world.shapes_.end(); ++it)
        {
            auto& i = *it;
            const ShapeImpl* shape = dynamic_cast<const ShapeImpl*>(i);
            if (m_instances.count(shape))
                m_instances[shape].updated = true;
        }

        //cleanup not updated instances
        auto itr = m_instances.begin();
        while (itr != m_instances.end())
        {
            if (!itr->second.updated)
            {
                //if no instances left => clear stored mesh
                auto& mesh = m_meshes[itr->first];
                ThrowIf(mesh.instance_count <= 0, "Invalid embree mesh");
                --mesh.instance_count;
                if (mesh.instance_count == 0)
                {
                    rtcDeleteScene(itr->second.scene);
                    CheckEmbreeError();
                    m_meshes.erase(itr->first);
                }
                itr = m_instances.erase(itr);
            }
            else
            {
                ++itr;
            }
        }
        m_instances.clear();
        rtcDeleteScene(m_scene); CheckEmbreeError();
        m_scene = rtcDeviceNewScene(m_device, RTC_SCENE_STATIC, RTC_INTERSECT1 | RTC_INTERSECT4 | RTC_INTERSECT8 | RTC_INTERSECT16 ); CheckEmbreeError();

        for (auto i : world.shapes_)
        {
            const ShapeImpl* shape = dynamic_cast<const ShapeImpl*>(i);
            ThrowIf(!shape, "Invalid shape.");

            //update only if shape already precessed before
            EmbreeSceneData& data = m_instances[shape];
            data.mesh_id = shape->GetId();
            data.updated = true;

            //for each new Shape creating new embree scene with geometry
            //and adding its instance to m_scene
            const Mesh* mesh = dynamic_cast<const Mesh*> (shape);
            if (mesh)
            {
                data.scene = GetEmbreeMesh(mesh);
            }
            else // instance
            {
                const Instance* inst = dynamic_cast<const Instance*> (shape);
                ThrowIf(!inst, "Invalid shape.");
                mesh = dynamic_cast<const Mesh*> (inst->GetBaseShape());
                ThrowIf(!mesh, "Invalid mesh.");
                //adding mesh of instance if it's not being processed before
                data.scene = GetEmbreeMesh(mesh);
                ++m_meshes[mesh].instance_count;
            }

            unsigned geom = rtcNewInstance(m_scene, data.scene);
            CheckEmbreeError();
            matrix trans, transInv;
            shape->GetTransform(trans, transInv);
            rtcSetTransform(m_scene, geom, RTC_MATRIX_ROW_MAJOR, &trans.m00);
            CheckEmbreeError();
            rtcSetMask(m_scene, geom, shape->GetMask());
            CheckEmbreeError();
            rtcSetUserData(m_scene, geom, &data);
            CheckEmbreeError();

            data.geom = geom;

        }

        rtcCommit(m_scene);
        CheckEmbreeError();
    }

    Buffer* EmbreeIntersectionDevice::CreateBuffer(size_t size, void* initdata) const
    {
        return new EmbreeBuffer(size, initdata);
    }

    void EmbreeIntersectionDevice::DeleteBuffer(Buffer* const buffer) const
    {
        delete buffer;
    }

    void EmbreeIntersectionDevice::DeleteEvent(Event* const event) const
    {
        delete event;
    }

    void EmbreeIntersectionDevice::MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const
    {
        EmbreeEvent* ev = new EmbreeEvent([]() {});
        if (data)
        {
            EmbreeBuffer* buf = dynamic_cast<EmbreeBuffer*>(buffer);
            ThrowIf(!buf, "Invalid embree buffer.");
            *data = buf->GetData();
        }

        if (event)
        {
            *event = ev;
        }
        else
        {
            ev->Wait();
            DeleteEvent(ev);
        }
    }

    void EmbreeIntersectionDevice::UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const
    {
        EmbreeEvent* ev = new EmbreeEvent([]() {});

        if (event)
        {
            *event = ev;
        }
        else
        {
            ev->Wait();
            DeleteEvent(ev);
        }
    }
    

    void EmbreeIntersectionDevice::QueryIntersection(Buffer const* rays, int numrays, Buffer* hits, Event const* waitevent, Event** event) const
    {
        const EmbreeBuffer* fireRays = dynamic_cast<const EmbreeBuffer*>(rays); ThrowIf(!fireRays, "Invalid embree buffer.");
        EmbreeBuffer* fireHits = dynamic_cast<EmbreeBuffer*>(hits); ThrowIf(!fireHits, "Invalid embree buffer.");

        EmbreeEvent* ev = new EmbreeEvent([this, fireRays, fireHits, numrays]() 
        {
            m_pool.setSleepTime(0);
            //processing buffers workflow:
            //1. convert RadeonRays::ray to RTCRay
            //2. rtcIntersect
            //3. convert RTCRay hit result to RadeonRays::Intersection
            std::vector<std::future<void> > jobs;
            jobs.reserve(numrays / TASK_SIZE);
#ifndef INTERSECTN
            for (int i = 0; i < numrays; i += TASK_SIZE)
            {
                const ray* src_ray = &static_cast<const ray*>(fireRays->GetData())[i];
                Intersection* hit = &static_cast<Intersection*>(fireHits->GetData())[i];
                int count = (i + TASK_SIZE) < numrays ? TASK_SIZE : numrays - i;

                jobs.push_back(std::move(m_pool.submit(([this, src_ray, hit, count]()
                {
                    RTCRay4 data;
                    for (int i = 0; i < count; i+=4)
                    {
                        int rays_count = (i + 4) < count ? 4 : count - i; // count of valid rays
                        RTCORE_ALIGN(16) int valid[4] = { 0, 0, 0, 0,}; //disable all rays
                        for (int j = 0; j < rays_count; ++j)
                        {
                            valid[j] = src_ray[i + j].IsActive() ? -1 : 0;
                            FillRTCRay(data, j, src_ray[i+j]);
                        }
                        rtcIntersect4(valid, m_scene, data); CheckEmbreeError();
                        for (int j = 0; j < rays_count; ++j)
                            FillIntersection(hit[i+j], data, j);
                    }
                }))));
            }
#else
            std::vector<RTCRay> data(numrays);
            for (int i = 0; i < numrays; i += TASK_SIZE)
            {
                const ray* src_ray = &static_cast<const ray*>(fireRays->GetData())[i];
                RTCRay* dst_ray = &data[i];
                Intersection* hit = &static_cast<Intersection*>(fireHits->GetData())[i];
                int count = (i + TASK_SIZE) < numrays ? TASK_SIZE : numrays - i;
                jobs.push_back(std::move(m_pool.submit([this, dst_ray, src_ray, count]()
                {
                    for (int j = 0; j < count; ++j)
                        FillRTCRay(dst_ray[j], src_ray[j]);
                })));
            }
            std::for_each(jobs.begin(), jobs.end(), [](std::future<void>& j) {j.wait(); });
            jobs.clear();
            rtcIntersectN(m_scene, &data[0], numrays, sizeof(RTCRay));
            CheckEmbreeError();
            for (int i = 0; i < numrays; i += TASK_SIZE)
            {
                const ray* src_ray = &static_cast<const ray*>(fireRays->GetData())[i];
                RTCRay* src_hit = &data[i];
                Intersection* hit = &static_cast<Intersection*>(fireHits->GetData())[i];
                int count = (i + TASK_SIZE) < numrays ? TASK_SIZE : numrays - i;

                jobs.push_back(std::move(m_pool.submit([this, hit, src_hit, src_ray, count]()
                {
                    for (int i = 0; i < count; ++i)
                        if (src_ray[i].IsActive())
                        {
                            FillIntersection(hit[i], src_hit[i]);
                        }
                })));
            }
            
#endif // INTERSECTN

            std::for_each(jobs.begin(), jobs.end(), [](std::future<void>& j) {j.wait(); });
            m_pool.setSleepTime(1);
        });

        if (event)
        {
            *event = ev;
        }
        else
        {
            ev->Wait();
            DeleteEvent(ev);
        }
    }

    void EmbreeIntersectionDevice::QueryOcclusion(Buffer const* rays, int numrays, Buffer* hits, Event const* waitevent, Event** event) const
    {
        const EmbreeBuffer* fireRays = dynamic_cast<const EmbreeBuffer*>(rays); ThrowIf(!fireRays, "Invalid embree buffer.");
        EmbreeBuffer* fireHits = dynamic_cast<EmbreeBuffer*>(hits); ThrowIf(!fireHits, "Invalid embree buffer.");

        EmbreeEvent* ev = new EmbreeEvent([this, fireRays, fireHits, numrays]()
        {
            m_pool.setSleepTime(0);
            //processing buffers workflow:
            //1. convert RadeonRays::ray to RTCRay
            //2. rtcOccluded
            //3. convert RTCRay hit result
            std::vector<std::future<void> > jobs;
            jobs.reserve(numrays / TASK_SIZE);
#ifndef INTERSECTN
            for (int i = 0; i < numrays; i += TASK_SIZE)
            {

                const ray* src_ray = &static_cast<const ray*>(fireRays->GetData())[i];
                int* hit = &static_cast<int*>(fireHits->GetData())[i];
                int count = (i + TASK_SIZE) < numrays ? TASK_SIZE : numrays - i;

                jobs.push_back(std::move(m_pool.submit(([this, src_ray, hit, count]()
                {
                    RTCRay4 data;
                    for (int i = 0; i < count; i += 4)
                    {
                        int rays_count = (i + 4) < count ? 4 : count - i; // count of valid rays
                        RTCORE_ALIGN(16) int valid[4] = { 0, 0, 0, 0, }; //disable all rays
                        for (int j = 0; j < 4; ++j)
                        {
                            data.orgx[j] = 0;
                            data.orgy[j] = 0;
                            data.orgz[j] = 0;

                            data.dirx[j] = 0;
                            data.diry[j] = 0;
                            data.dirz[j] = 0;

                            data.tnear[j] = 0;
                            data.tfar[j] = 0;
                            data.geomID[j] = RTC_INVALID_GEOMETRY_ID;
                            data.primID[j] = RTC_INVALID_GEOMETRY_ID;
                            data.instID[j] = RTC_INVALID_GEOMETRY_ID;
                            data.time[j] = 0;
                            data.mask[j] = 0xFFFFFF;
                        }
                        for (int j = 0; j < rays_count; ++j)
                        {
                            valid[j] = src_ray[i + j].IsActive() ? -1 : 0;
                            FillRTCRay(data, j, src_ray[i + j]);
                        }
                        rtcOccluded4(valid, m_scene, data); CheckEmbreeError();
                        for (int j = 0; j < rays_count; ++j)
                        {
                            if (data.instID[j] == RTC_INVALID_GEOMETRY_ID || data.geomID[j] == RTC_INVALID_GEOMETRY_ID)
                            {
                                hit[i + j] = RTC_INVALID_GEOMETRY_ID;
                                continue;
                            }
                            hit[i + j] = data.instID[j];
                            EmbreeSceneData* data = static_cast<EmbreeSceneData*>(rtcGetUserData(m_scene, hit[i + j]));
                            hit[i + j] = data->mesh_id;
                        }
                    }
                }))));
            }
#else
            std::vector<RTCRay> data(numrays);
            for (int i = 0; i < numrays; i += TASK_SIZE)
            {
                const ray* src_ray = &static_cast<const ray*>(fireRays->GetData())[i];
                RTCRay* dst_ray = &data[i];
                Intersection* hit = &static_cast<Intersection*>(fireHits->GetData())[i];
                int count = (i + TASK_SIZE) < numrays ? TASK_SIZE : numrays - i;
                jobs.push_back(std::move(m_pool.submit([this, dst_ray, src_ray, count]()
                {
                    for (int j = 0; j < count; ++j)
                        FillRTCRay(dst_ray[j], src_ray[j]);
                })));
            }
            std::for_each(jobs.begin(), jobs.end(), [](std::future<void>& j) {j.wait(); });
            jobs.clear();
            rtcOccludedN(m_scene, &data[0], numrays, sizeof(RTCRay));
            CheckEmbreeError();
            for (int i = 0; i < numrays; i += TASK_SIZE)
            {
                const ray* src_ray = &static_cast<const ray*>(fireRays->GetData())[i];
                int* hit = &static_cast<int*>(fireHits->GetData())[i];
                int count = (i + TASK_SIZE) < numrays ? TASK_SIZE : numrays - i;
                RTCRay* hit_src = &data[i];
                jobs.push_back(std::move(m_pool.submit([this, hit, hit_src, src_ray, count]()
                {
                    for (int i = 0; i < count; ++i)
                    {
                        if (hit_src[i].instID == RTC_INVALID_GEOMETRY_ID || hit_src[i].geomID == RTC_INVALID_GEOMETRY_ID)
                        {
                            hit[i] = RTC_INVALID_GEOMETRY_ID;
                            continue;
                        }
                        hit[i] = hit_src[i].instID;
                        EmbreeSceneData* data = static_cast<EmbreeSceneData*>(rtcGetUserData(m_scene, hit[i]));
                        hit[i] = data->mesh_id;
                    }
                })));
            }
#endif // INTERSECTN

            std::for_each(jobs.begin(), jobs.end(), [](std::future<void>& j) {j.wait(); });
            m_pool.setSleepTime(1);
        });

        if (event)
        {
            *event = ev;
        }
        else
        {
            ev->Wait();
            DeleteEvent(ev);
        }
    }

    void EmbreeIntersectionDevice::QueryIntersection(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hits, Event const* waitevent, Event** event) const
    {
        Throw("Not implemented for embree device.");
    }

    void EmbreeIntersectionDevice::QueryOcclusion(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hits, Event const* waitevent, Event** event) const
    {
        Throw("Not implemented for embree device.");
    }

    RTCScene EmbreeIntersectionDevice::GetEmbreeMesh(const RadeonRays::Mesh* mesh)
    {
        if (m_meshes.count(mesh))
            return m_meshes[mesh].scene;
        RTCScene result = rtcDeviceNewScene(m_device, RTC_SCENE_STATIC, RTC_INTERSECT1 | RTC_INTERSECT4 | RTC_INTERSECT8 | RTC_INTERSECT16 );
        CheckEmbreeError();
        ThrowIf(!mesh->puretriangle(), "Only triangle meshes supported by now.");

        unsigned id = rtcNewTriangleMesh(result, RTC_GEOMETRY_STATIC, mesh->num_faces(), mesh->num_vertices());
        CheckEmbreeError();
        
        const float3* kMeshVerts = mesh->GetVertexData();
        float* verts = static_cast<float*>(rtcMapBuffer(result, id, RTC_VERTEX_BUFFER));
        CheckEmbreeError();
        ThrowIf(!verts, "Failed to map embree buffer.");
        for (int i = 0; i < mesh->num_vertices(); ++i)
        {
            verts[4 * i] = kMeshVerts[i].x;
            verts[4 * i + 1] = kMeshVerts[i].y;
            verts[4 * i + 2] = kMeshVerts[i].z;
            verts[4 * i + 3] = kMeshVerts[i].w;
        }
        rtcUnmapBuffer(result, id, RTC_VERTEX_BUFFER);

        int* indices = static_cast<int*>(rtcMapBuffer(result, id, RTC_INDEX_BUFFER));
        CheckEmbreeError();
        ThrowIf(!indices, "Failed to map embree buffer.");
        const Mesh::Face* kFaces = mesh->GetFaceData();
        for (int i = 0; i < mesh->num_faces(); ++i)
        {
            indices[3 * i] = kFaces[i].i0;
            indices[3 * i + 1] = kFaces[i].i1;
            indices[3 * i + 2] = kFaces[i].i2;
        }
        rtcUnmapBuffer(result, id, RTC_INDEX_BUFFER);
        CheckEmbreeError();
        rtcCommit(result);

        m_meshes[mesh].scene = result;
        m_meshes[mesh].instance_count = 1;

        return result;
    }

    void EmbreeIntersectionDevice::UpdateShape(const RadeonRays::ShapeImpl* shape)
    {
        const EmbreeSceneData& data = m_instances[shape];
        int state = shape->GetStateChange();
        if (state == ShapeImpl::kStateChangeNone)
            return;

        if (state & ShapeImpl::kStateChangeMask)
        {
            rtcSetMask(m_scene, data.geom, shape->GetMask());
            CheckEmbreeError();
        }
        if (state & ShapeImpl::kStateChangeTransform)
        {
            matrix trans, transInv;
            shape->GetTransform(trans, transInv);
            rtcSetTransform(m_scene, data.geom, RTC_MATRIX_ROW_MAJOR, &trans.m00);
            CheckEmbreeError();
        }
        if (state & ShapeImpl::kStateChangeId)
        {
            m_instances[shape].mesh_id = shape->GetId();
        }

        ThrowIf((state & ShapeImpl::kStateChangeMotion) ? true : false, "Not implemented for embree device");
    }

    void EmbreeIntersectionDevice::FillRTCRay(RTCRay& dst, const ray& src) const
    {
        dst.org[0] = src.o.x;
        dst.org[1] = src.o.y;
        dst.org[2] = src.o.z;

        dst.dir[0] = src.d.x;
        dst.dir[1] = src.d.y;
        dst.dir[2] = src.d.z;

        dst.tnear = 0;
        dst.tfar = src.GetMaxT();
        dst.geomID = RTC_INVALID_GEOMETRY_ID;
        dst.primID = RTC_INVALID_GEOMETRY_ID;
        dst.instID = RTC_INVALID_GEOMETRY_ID;
        dst.time = src.GetTime();
        dst.mask = src.GetMask();
    }

    void EmbreeIntersectionDevice::FillRTCRay(RTCRay4& dst, int i, const ray& src) const
    {
        dst.orgx[i] = src.o.x;
        dst.orgy[i] = src.o.y;
        dst.orgz[i] = src.o.z;

        dst.dirx[i] = src.d.x;
        dst.diry[i] = src.d.y;
        dst.dirz[i] = src.d.z;

        dst.tnear[i] = 0;
        dst.tfar[i] = src.GetMaxT();
        dst.geomID[i] = RTC_INVALID_GEOMETRY_ID;
        dst.primID[i] = RTC_INVALID_GEOMETRY_ID;
        dst.instID[i] = RTC_INVALID_GEOMETRY_ID;
        dst.time[i] = src.GetTime();
        dst.mask[i] = src.GetMask();
    }


    void EmbreeIntersectionDevice::FillIntersection(Intersection& dst, const RTCRay& src) const
    {
        dst.shapeid = src.instID;
        if (dst.shapeid != RTC_INVALID_GEOMETRY_ID)
        {
            const EmbreeSceneData* kData = static_cast<const EmbreeSceneData*>(rtcGetUserData(m_scene, dst.shapeid));
            dst.shapeid = kData->mesh_id;
        }
        dst.primid = src.primID;

        dst.uvwt.x = src.u;
        dst.uvwt.y = src.v;
        dst.uvwt.z = 0;
        dst.uvwt.w = src.tfar;
    }
    void EmbreeIntersectionDevice::FillIntersection(Intersection& dst, const RTCRay4& src, int i) const
    {
        dst.shapeid = src.instID[i];
        if (dst.shapeid != RTC_INVALID_GEOMETRY_ID)
        {
            const EmbreeSceneData* kData = static_cast<const EmbreeSceneData*>(rtcGetUserData(m_scene, dst.shapeid));
            dst.shapeid = kData->mesh_id;
        }
        dst.primid = src.primID[i];

        dst.uvwt.x = src.u[i];
        dst.uvwt.y = src.v[i];
        dst.uvwt.z = 0;
        dst.uvwt.w = src.tfar[i];
    }


    void EmbreeIntersectionDevice::CheckEmbreeError() const
    {
        RTCError err = rtcDeviceGetError(m_device);
        if (err != RTC_NO_ERROR)
        {
            std::cout << "Embree error: " << err << std::endl;
            Throw("Embree error");
        }
    }
} //RadeonRays