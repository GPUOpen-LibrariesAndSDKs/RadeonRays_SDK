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

#include "calc.h"
#include "buffer.h"
#include "device.h"
#include "event.h"
#include <memory>
#include <functional>

namespace RadeonRays
{
    struct CalcBufferHolder : public RadeonRays::Buffer
    {

        CalcBufferHolder(Calc::Device* device, Calc::Buffer* buffer)
            : m_buffer(buffer, [device](Calc::Buffer* buffer) { device->DeleteBuffer(buffer); })
        {
        }

        ~CalcBufferHolder() = default;

        Calc::Buffer* GetData() const
        {
            return m_buffer.get();
        }

        std::unique_ptr<Calc::Buffer, std::function<void(Calc::Buffer*)>> m_buffer;
    };

    struct CalcEventHolder : public RadeonRays::Event
    {
        CalcEventHolder()
            : m_event()
        {
        }

        CalcEventHolder(Calc::Device* device, Calc::Event* event)
            : m_event(event, [device](Calc::Event* event) { device->DeleteEvent(event); })
        {
        }

        ~CalcEventHolder() = default;

        void Set(Calc::Device* device, Calc::Event* event)
        {
            m_event = decltype(m_event)(event, [device](Calc::Event* event) { device->DeleteEvent(event); });
        }

        bool Complete() const override
        {
            return m_event->IsComplete();
        }

        void Wait() override
        {
            return m_event->Wait();
        }

        Calc::Event* GetData()
        {
            return m_event.get();
        }

        std::unique_ptr<Calc::Event, std::function<void(Calc::Event*)>> m_event;
    };
}

