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
 * @brief Vulkan command stream implementation.
 *
 * Vulkan commands stream implementation is based on Vulkan command buffer.
 **/
class CommandStream : public CommandStreamBackend<BackendType::kVulkan>
{
public:
    CommandStream(DeviceBackend<BackendType::kVulkan>& dev) : device_(dev), external_(false) {}
    CommandStream(DeviceBackend<BackendType::kVulkan>& dev, VkCommandBuffer cmd_buffer)
        : device_(dev), command_buffer_(cmd_buffer), external_(true)
    {
    }
    ~CommandStream();

    // Get temporary allocated buffer and attach it to this command stream.
    AllocatedBuffer*                GetAllocatedTemporaryBuffer(size_t size) override;
    void                            ClearTemporaryBuffers() override;
    vk::CommandBuffer               Get() const override;
    void                            Set(vk::CommandBuffer command_buffer) override;

public:
    // vulkan::Device device.
    DeviceBackend<BackendType::kVulkan>& device_;

    /// Vulkan things.
    vk::CommandBuffer command_buffer_ = nullptr;

    /// Temporary resources attached by the device.
    std::vector<AllocatedBuffer*> temp_buffers_;

    bool external_ = false;
};
}  // namespace rt::vulkan