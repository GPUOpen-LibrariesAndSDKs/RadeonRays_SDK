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

#include "src/vlk/buffer.h"
#include "src/vlk/vulkan_wrappers.h"

namespace rt::vulkan
{
/**
 * @brief Vulkan implementation of a device pointer.
 **/
class DevicePtr : public DevicePtrBackend<BackendType::kVulkan>
{
public:
    /// Constructor.
    DevicePtr() = default;
    DevicePtr(VkBuffer buf, size_t off) : buffer(buf), offset(off) {}
    /// Destructor.
    ~DevicePtr() = default;
    vk::Buffer Get() const override { return buffer; }
    size_t   Offset() const override { return offset; }
    void*      Map() override { return nullptr; }
    void       Unmap() override {}

private:
    // Vulkan buffer.
    vk::Buffer buffer;
    // Offset in the buffer.
    size_t offset = 0;
};

class AllocatedDevicePtr : public DevicePtrBackend<BackendType::kVulkan>
{
public:
    /// Constructor.
    explicit AllocatedDevicePtr(AllocatedBuffer buf) : buffer(buf) {}
    /// Destructor.
    ~AllocatedDevicePtr() { buffer.Destroy(); }
    vk::Buffer Get() const override { return buffer.buffer; }
    size_t     Offset() const override { return 0u; }
    void*      Map() override { return buffer.Map(); }
    void       Unmap() override { buffer.Unmap(); }

private:
    // Vulkan buffer.
    AllocatedBuffer buffer;
};
}  // namespace rt::vulkan