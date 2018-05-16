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
#include "calc_intersection_device.h"

#include "calc.h"
#include "buffer.h"
#include "device.h"
#include "event.h"
#include "../primitive/shapeimpl.h"

#include "calc_holder.h"

#include "../intersector/intersector.h"
#include "../intersector/intersector_2level.h"
#include "../intersector/intersector_skip_links.h"
#include "../intersector/intersector_short_stack.h"
#include "../intersector/intersector_lds.h"
#include "../intersector/intersector_hlbvh.h"
#include "../intersector/intersector_bittrail.h"
#include "../world/world.h"
#include <iostream>
#include <memory>

namespace RadeonRays
{
    // TODO: handle different BVH strategies, for now hardcoded
    CalcIntersectionDevice::CalcIntersectionDevice(Calc::Calc* calc, Calc::Device* device)
        : m_device(device, [calc](Calc::Device* device) { calc->DeleteDevice(device); })
        , m_intersector(new IntersectorSkipLinks(device))
        , m_intersector_string("bvh")
    {
        // Initialize event pool
        for (auto i = 0; i < EVENT_POOL_INITIAL_SIZE; ++i)
        {
            m_event_pool.push(new CalcEventHolder());
        }
    }

    CalcIntersectionDevice::~CalcIntersectionDevice()
    {
        while (!m_event_pool.empty())
        {
            auto event = m_event_pool.front();
            m_event_pool.pop();
            delete event;
        }
    }

    void CalcIntersectionDevice::Preprocess(World const& world)
    {
        bool use2level = false;

        // First check if 2 level BVH has been forced
        auto opt2level = world.options_.GetOption("bvh.force2level");
        if (opt2level && opt2level->AsFloat() > 0.f)
        {
            use2level = true;
        }
        else
        {
            auto opt_force_flat = world.options_.GetOption("bvh.forceflat");

            if (opt_force_flat && opt_force_flat->AsFloat() > 0.f)
            {
                use2level = false;
            }
            else
            {
                // Otherwise check if there are instances in the world
                for (auto shape : world.shapes_)
                {
                    // Get implementation
                    auto shapeimpl = static_cast<ShapeImpl const*>(shape);
                    // Check if it is an instance and update flag
                    use2level = use2level | shapeimpl->is_instance();
                }
            }
        }

        if (use2level)
        {
            if (m_intersector_string != "bvh2l")
            {
                m_intersector.reset(new IntersectorTwoLevel(m_device.get()));
                m_intersector_string = "bvh2l";
            }
        }
        else
        {
            {
                auto optacctype = world.options_.GetOption("acc.type");
                std::string acctype = optacctype ? optacctype->AsString() : "bvh";

                if (acctype == "bvh")
                {
                    if (m_intersector_string != "bvh")
                    {
                        m_intersector.reset(new IntersectorSkipLinks(m_device.get()));
                        m_intersector_string = "bvh";
                    }
                }
                else if (acctype == "fatbvh")
                {
                    if (m_intersector_string != "fatbvh")
                    {
#if 0
                        m_intersector.reset(new IntersectorShortStack(m_device.get()));
                        m_intersector_string = "fatbvh";
#else
                        m_intersector.reset(new IntersectorLDS(m_device.get()));
                        m_intersector_string = "fatbvh";
#endif
                    }
                }
                else if (acctype == "hlbvh")
                {
                    if (m_intersector_string != "hlbvh")
                    {
                        m_intersector.reset(new IntersectorHlbvh(m_device.get()));
                        m_intersector_string = "hlbvh";
                    }
                }
                /*else if (acctype == "hashbvh")
                {
                    if (m_intersector_string != "hashbvh")
                    {
                        m_intersector.reset(new IntersectorBitTrail(m_device.get()));
                        m_intersector_string = "hashbvh";
                    }
                }*/
            }
        }

        try
        {
            // Let intersector to do its preprocessing job
            m_intersector->SetWorld(world);
        }
        catch (Exception& e)
        {
            std::cout << e.what();
            throw;
        }
    }

    Buffer* CalcIntersectionDevice::CreateBuffer(size_t size, void* initdata) const
    {
        // If initdata is passed in use different Calc call with init data
        if (initdata)
        {
            auto calc_buffer = m_device->CreateBuffer(size, Calc::BufferType::kWrite, initdata);
            return new CalcBufferHolder(m_device.get(), calc_buffer);
        }
        else
        {
            auto calc_buffer = m_device->CreateBuffer(size, Calc::BufferType::kWrite);
            return new CalcBufferHolder(m_device.get(), calc_buffer);
        }
    }

    void CalcIntersectionDevice::DeleteBuffer(Buffer* const buffer) const
    {
        delete buffer;
    }

    void CalcIntersectionDevice::DeleteEvent(Event* const event) const
    {
        ReleaseEventHolder(static_cast<CalcEventHolder*>(event));
    }

    static Calc::MapType CalcMapType(MapType type)
    {
        switch (type)
        {
        case kMapRead:
            return Calc::MapType::kMapRead;
        case kMapWrite:
            return Calc::MapType::kMapWrite;
        default:
            return Calc::MapType::kMapWrite;
        }
    }

    void CalcIntersectionDevice::MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const
    {
        auto calc_buffer = static_cast<CalcBufferHolder*>(buffer);

        if (event)
        {
            Calc::Event* e = nullptr;
            m_device->MapBuffer(calc_buffer->GetData(), 0, offset, size, CalcMapType(type), data, &e);

            auto holder = CreateEventHolder();
            holder->Set(m_device.get(), e);
            *event = holder;
        }
        else
        {
            m_device->MapBuffer(calc_buffer->GetData(), 0, offset, size, CalcMapType(type), data, nullptr);
        }
    }

    void CalcIntersectionDevice::UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const
    {
        auto calc_buffer = static_cast<CalcBufferHolder*>(buffer);

        if (event)
        {
            Calc::Event* e = nullptr;
            m_device->UnmapBuffer(calc_buffer->GetData(), 0, ptr, &e);

            auto holder = CreateEventHolder();
            holder->Set(m_device.get(), e);
            *event = holder;
        }
        else
        {
            m_device->UnmapBuffer(calc_buffer->GetData(), 0, ptr, nullptr);
        }
    }


    void CalcIntersectionDevice::QueryIntersection(Buffer const* rays, int numrays, Buffer* hits, Event const* waitevent, Event** event) const
    {
        // Extract Calc buffers from their holders
        auto ray_buffer = static_cast<CalcBufferHolder const*>(rays)->m_buffer.get();
        auto hit_buffer = static_cast<CalcBufferHolder const*>(hits)->m_buffer.get();
        // If waitevent is passed in we have to extract it as well
        auto e = waitevent ? static_cast<CalcEventHolder const*>(waitevent)->m_event.get() : nullptr;

        if (event)
        {
            // event pointer has been provided, so construct holder and return event to the user
            Calc::Event* calc_event = nullptr;
            m_intersector->QueryIntersection(0, ray_buffer, numrays, hit_buffer, e, &calc_event);

            auto holder = CreateEventHolder();
            holder->Set(m_device.get(), calc_event);
            *event = holder;
        }
        else
        {
            m_intersector->QueryIntersection(0, ray_buffer, numrays, hit_buffer, e, nullptr);
        }
    }

    void CalcIntersectionDevice::QueryOcclusion(Buffer const* rays, int numrays, Buffer* hits, Event const* waitevent, Event** event) const
    {
        // Extract Calc buffers from their holders
        auto ray_buffer = static_cast<CalcBufferHolder const*>(rays)->m_buffer.get();
        auto hit_buffer = static_cast<CalcBufferHolder const*>(hits)->m_buffer.get();
        // If waitevent is passed in we have to extract it as well
        auto e = waitevent ? static_cast<CalcEventHolder const*>(waitevent)->m_event.get() : nullptr;

        if (event)
        {
            // event pointer has been provided, so construct holder and return event to the user
            Calc::Event* calc_event = nullptr;
            m_intersector->QueryOcclusion(0, ray_buffer, numrays, hit_buffer, e, &calc_event);

            auto holder = CreateEventHolder();
            holder->Set(m_device.get(), calc_event);
            *event = holder;
        }
        else
        {
            m_intersector->QueryOcclusion(0, ray_buffer, numrays, hit_buffer, e, nullptr);
        }
    }

    void CalcIntersectionDevice::QueryIntersection(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hits, Event const* waitevent, Event** event) const
    {
        // Extract Calc buffers from their holders
        auto ray_buffer = static_cast<CalcBufferHolder const*>(rays)->m_buffer.get();
        auto hit_buffer = static_cast<CalcBufferHolder const*>(hits)->m_buffer.get();
        auto numrays_buffer = static_cast<CalcBufferHolder const*>(numrays)->m_buffer.get();
        // If waitevent is passed in we have to extract it as well
        auto e = waitevent ? static_cast<CalcEventHolder const*>(waitevent)->m_event.get() : nullptr;

        if (event)
        {
            // event pointer has been provided, so construct holder and return event to the user
            Calc::Event* calc_event = nullptr;
            m_intersector->QueryIntersection(0, ray_buffer, numrays_buffer, maxrays, hit_buffer, e, &calc_event);

            auto holder = CreateEventHolder();
            holder->Set(m_device.get(), calc_event);
            *event = holder;
        }
        else
        {
            m_intersector->QueryIntersection(0, ray_buffer, numrays_buffer, maxrays, hit_buffer, e, nullptr);
        }
    }

    void CalcIntersectionDevice::QueryOcclusion(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hits, Event const* waitevent, Event** event) const
    {
        // Extract Calc buffers from their holders
        auto ray_buffer = static_cast<CalcBufferHolder const*>(rays)->m_buffer.get();
        auto hit_buffer = static_cast<CalcBufferHolder const*>(hits)->m_buffer.get();
        auto numrays_buffer = static_cast<CalcBufferHolder const*>(numrays)->m_buffer.get();
        // If waitevent is passed in we have to extract it as well
        auto e = waitevent ? static_cast<CalcEventHolder const*>(waitevent)->m_event.get() : nullptr;

        if (event)
        {
            // event pointer has been provided, so construct holder and return event to the user
            Calc::Event* calc_event = nullptr;
            m_intersector->QueryOcclusion(0, ray_buffer, numrays_buffer, maxrays, hit_buffer, e, &calc_event);

            auto holder = CreateEventHolder();
            holder->Set(m_device.get(), calc_event);
            *event = holder;
        }
        else
        {
            m_intersector->QueryOcclusion(0, ray_buffer, numrays_buffer, maxrays, hit_buffer, e, nullptr);
        }

    }

    CalcEventHolder* CalcIntersectionDevice::CreateEventHolder() const
    {
        if (m_event_pool.empty())
        {
            auto event = new CalcEventHolder();
            return event;
        }
        else
        {
            auto event = m_event_pool.front();
            m_event_pool.pop();
            return event;
        }
    }

    void    CalcIntersectionDevice::ReleaseEventHolder(CalcEventHolder* e) const
    {
        m_event_pool.push(e);
    }
}
