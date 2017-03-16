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

#include "intersection_device.h"

#include "calc.h"
#include "device.h"

#include <memory>
#include <functional>
#include <queue>


namespace RadeonRays
{
    class Intersector;
    struct CalcEventHolder;

    ///< The class represents Calc based intersection device.
    ///< It uses Calc::Device abstraction to implement intersection algorithm.
    ///<
    class CalcIntersectionDevice : public IntersectionDevice
    {
    public:
        //
        CalcIntersectionDevice(Calc::Calc* calc, Calc::Device* device);
        ~CalcIntersectionDevice();

        void Preprocess(World const& world) override;

        Buffer* CreateBuffer(size_t size, void* initdata) const override;

        void DeleteBuffer(Buffer* const) const override;

        void DeleteEvent(Event* const) const override;

        void MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const override;

        void UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const override;

        void QueryIntersection(Buffer const* rays, int numrays, Buffer* hitinfos, Event const* waitevent, Event** event) const override;

        void QueryOcclusion(Buffer const* rays, int numrays, Buffer* hitresults, Event const* waitevent, Event** event) const override;

        void QueryIntersection(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitinfos, Event const* waitevent, Event** event) const override;

        void QueryOcclusion(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitresults, Event const* waitevent, Event** event) const override;

        Calc::Platform GetPlatform() const { return m_device->GetPlatform(); }
    protected:
        CalcEventHolder* CreateEventHolder() const;
        void      ReleaseEventHolder(CalcEventHolder* e) const;

        std::unique_ptr<Calc::Device, std::function<void(Calc::Device*)>> m_device;
        std::unique_ptr<Intersector> m_intersector;
        std::string m_intersector_string;

        // Initial number of events in the pool
        static const std::size_t EVENT_POOL_INITIAL_SIZE = 100;
        // Event pool
        mutable std::queue<CalcEventHolder*> m_event_pool;
    };
}

