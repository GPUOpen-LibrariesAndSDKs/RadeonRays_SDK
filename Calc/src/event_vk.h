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

#include "device_vkw.h"
#include "wrappers/event.h"

namespace Calc {
    class DeviceVulkanw;

    // Vulkan version of Event. It does not used any OS primitives but ensures using existing functionality whether an execution of the CommandBuffer has been completed or not.
    class EventVulkan : public Event
    {
    public:
        EventVulkan( const DeviceVulkanw* in_device ) :
                m_device( in_device )
        {
            m_event_fence = m_device->GetFenceId();
        }

        virtual ~EventVulkan()
        {
            m_device = nullptr;
        }

        void Wait() override
        {
            Assert( nullptr != m_device );
            m_device->WaitForFence(m_event_fence);
        }

        bool IsComplete() const override
        {
            Assert( nullptr != m_device );
            return m_device->HasFenceBeenPassed(m_event_fence);
        }
    private:
        const DeviceVulkanw* m_device;
        std::atomic<uint64_t>           m_event_fence;
    };
}
