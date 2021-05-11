/**********************************************************************
Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
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
#include "base/command_stream_base.h"
#include "base/device_base.h"
#include "base/device_ptr_base.h"
#include "base/event_base.h"
#include "base/intersector_base.h"
#include "vlk/gpu_helper.h"

namespace rt
{
template <>
class CommandStreamBackend<BackendType::kVulkan> : public CommandStreamBase
{
public:
    virtual vulkan::AllocatedBuffer* GetAllocatedTemporaryBuffer(size_t size) = 0;
    virtual void                     ClearTemporaryBuffers()                  = 0;
    virtual vk::CommandBuffer        Get() const                              = 0;
    virtual void                     Set(vk::CommandBuffer command_buffer)    = 0;
};

template <>
class EventBackend<BackendType::kVulkan> : public EventBase
{
public:
    virtual vk::Fence Get() const      = 0;
    virtual void      Set(vk::Fence f) = 0;
};

template <>
class DevicePtrBackend<BackendType::kVulkan> : public DevicePtrBase
{
public:
    virtual vk::Buffer Get() const    = 0;
    virtual size_t     Offset() const = 0;
    virtual void*      Map()          = 0;
    virtual void       Unmap()        = 0;
};

template <>
class DeviceBackend<BackendType::kVulkan> : public DeviceBase
{
public:
    virtual std::shared_ptr<vulkan::GpuHelper>      Get() const                                                 = 0;
    virtual DevicePtrBackend<BackendType::kVulkan>* CreateAllocatedBuffer(size_t size)                          = 0;
    virtual vulkan::AllocatedBuffer*                AcquireTemporaryBuffer(size_t size)                         = 0;
    virtual void                                    ReleaseTemporaryBuffer(vulkan::AllocatedBuffer* tmp_buffer) = 0;
};

}  // namespace rt