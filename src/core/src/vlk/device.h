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

#include <atomic>
#include <cstdint>
#include <memory>

#include "src/utils/pool.h"
#include "src/vlk/device_ptr.h"
#include "src/vlk/vulkan_wrappers.h"


namespace rt::vulkan
{
/**
 * @brief Vulkan raytracing device implementation.
 *
 * Provides basic functionality for resource allocation / pooling, command stream submission.
 **/
class Device : public DeviceBackend<BackendType::kVulkan>
{
public:
    // Constructor.
    Device();
    Device(VkDevice device, VkPhysicalDevice physical_device, VkQueue queue, uint32_t queue_family_index);
    // Destructor.
    ~Device();

    /*************************************************************
     * DeviceBase interface overrrides.
     *************************************************************/
    /**
     * @brief Allocate a command stream.
     *
     * This operation does not usually involve any system memory allocations and is relatively lightweight.
     *
     * @return Allocated command stream.
     **/
    CommandStreamBase* AllocateCommandStream() override;

    /**
     * @brief Release a command stream.
     *
     * If the command stream is still executing on GPU, deferred deletion happens.
     *
     * @param command_stream_base Command stream to release.
     **/
    void ReleaseCommandStream(CommandStreamBase* command_stream_base) override;

    /**
     * @brief Submit a command stream.
     *
     * Waits for wait_event and signals returned event upon completion.
     *
     * @param command_stream_base Command stream to submit.
     * @param wait_event_base Event to wait for before submitting (or in the submission queue).
     * @return Event marking a submission.
     **/
    EventBase* SubmitCommandStream(CommandStreamBase* command_stream_base, EventBase* wait_event_base) override;

    /**
     * @brief Releases an event.
     *
     * Release an event allocated by a submission. This call is relatively lightweight and do not usually trigger any
     * memory allocations / deallocations.
     *
     * @param event_base Event to release.
     **/
    void ReleaseEvent(EventBase* event_base) override;

    /**
     * @brief Wait for an event.
     *
     * Block on CPU until the event has signaled.
     **/
    void WaitEvent(EventBase* event_base) override;

    /**
     * @brief Create allocated buffer
     *
     * Create allocated buffer visible and coherent to host.
     **/
    DevicePtrBackend<BackendType::kVulkan>* CreateAllocatedBuffer(size_t size);

    // DeviceBackend<BackendType::kVulkan> implementation
    /// Allocate temporary staging buffer.
    // Get an underlying Vulkan device helpers.
    std::shared_ptr<GpuHelper> Get() const override;
    AllocatedBuffer*           AcquireTemporaryBuffer(size_t) override { return temporary_buffer_pool_.AcquireObject(); }
    void ReleaseTemporaryBuffer(AllocatedBuffer* buffer) override { temporary_buffer_pool_.ReleaseObject(buffer); }

private:
    void InitializePools();
    void CreateDeviceAndCommandQueue();

private:
    std::shared_ptr<GpuHelper> impl_;

    // Event pool for submission events.
    Pool<EventBackend<BackendType::kVulkan>> event_pool_;
    // Command stream pool for submissions pulling.
    Pool<CommandStreamBackend<BackendType::kVulkan>> command_stream_pool_;
    // Temporary buffer pool.
    Pool<AllocatedBuffer> temporary_buffer_pool_;

    friend class CommandStream;
};

/// Create device from scratch.
std::unique_ptr<rt::DeviceBase> CreateDevice();
/// Create device from existing Vulkan device and queue.
std::unique_ptr<rt::DeviceBase> CreateDevice(VkDevice         device,
                                             VkPhysicalDevice physical_device,
                                             VkQueue          command_queue,
                                             uint32_t         queue_family_index);

/// Create device pointer from Vulkan buffer.
DevicePtr* CreateDevicePtr(VkBuffer resource, size_t offset);

}  // namespace rt::vulkan