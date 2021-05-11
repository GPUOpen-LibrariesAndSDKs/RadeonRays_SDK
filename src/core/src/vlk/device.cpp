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
#include "device.h"

#include <vector>

#include "src/utils/logger.h"
#include "src/vlk/command_stream.h"
#include "src/vlk/device_ptr.h"
#include "src/vlk/event.h"
#include "src/vlk/gpu_helper.h"

namespace
{
static constexpr uint64_t kInfiniteTime       = UINT64_MAX;
static constexpr auto     VK_VENDOR_ID_AMD    = 0x1002;
static constexpr auto     VK_VENDOR_ID_NVIDIA = 0x10de;
static constexpr auto     VK_VENDOR_ID_INTEL  = 0x8086;
}  // namespace


namespace rt::vulkan
{
std::unique_ptr<rt::DeviceBase> CreateDevice() { return std::make_unique<Device>(); }

std::unique_ptr<rt::DeviceBase> CreateDevice(VkDevice         device,
                                             VkPhysicalDevice ph_device,
                                             VkQueue          queue,
                                             uint32_t         queue_family_index)
{
    return std::make_unique<Device>(device, ph_device, queue, queue_family_index);
}

DevicePtr* CreateDevicePtr(VkBuffer buffer, size_t offset) { return new DevicePtr(buffer, offset); }

std::shared_ptr<GpuHelper> Device::Get() const { return impl_; }

void Device::CreateDeviceAndCommandQueue()
{
    Logger::Get().Debug("Initializing Vulkan internals");
    vk::ApplicationInfo app_info = {"RadeonRays Library", 1, "RadeonRays", 1, VK_API_VERSION_1_2};


    std::vector<const char*> instance_extensions = {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};
    std::vector<const char*> instance_layers;
#ifdef _DEBUG
    const char* validation_layer = {"VK_LAYER_KHRONOS_validation"};
    instance_layers.push_back(validation_layer);
#endif
    vk::InstanceCreateInfo instance_info = {{},
                                            &app_info,
                                            (uint32_t)instance_layers.size(),
                                            instance_layers.data(),
                                            (uint32_t)instance_extensions.size(),
                                            instance_extensions.data()};
    vk::Instance           instance      = vk::createInstance(instance_info);

    auto devices = instance.enumeratePhysicalDevices();
    if (devices.size() < 1)
    {
        Logger::Get().Error("Cannot enumerate physical devices");
        throw std::runtime_error("No physical devices.");
    }

    auto physical_device = devices[0];
    // get compute queue
    uint32_t queue_index = 0;

    std::vector<vk::QueueFamilyProperties> queue_props = physical_device.getQueueFamilyProperties();
    uint32_t                               queue_count = (uint32_t)queue_props.size();

    for (queue_index = 0; queue_index < queue_count; queue_index++)
    {
        if (queue_props[queue_index].queueFlags & vk::QueueFlagBits::eCompute)
        {
            break;
        }
    }
    if (queue_index >= queue_count)
    {
        Logger::Get().Error("No queue for compute");
        throw std::runtime_error("No compute queue.");
    }

    float                     queue_prio = 1.0f;
    vk::DeviceQueueCreateInfo queue_create_info;
    queue_create_info.queueFamilyIndex = queue_index;
    queue_create_info.queueCount       = 1;
    queue_create_info.pQueuePriorities = &queue_prio;

    auto features = physical_device.getFeatures();

    vk::PhysicalDeviceDescriptorIndexingFeatures physical_device_descriptor_indexing_features{};
    physical_device_descriptor_indexing_features.shaderStorageBufferArrayNonUniformIndexing = true;
    physical_device_descriptor_indexing_features.runtimeDescriptorArray                     = true;
    physical_device_descriptor_indexing_features.descriptorBindingVariableDescriptorCount   = true;
    vk::PhysicalDeviceFeatures2 features2{};
    features2.setFeatures(features);
    features2.setPNext(&physical_device_descriptor_indexing_features);

    vk::DeviceCreateInfo device_create_info({}, 1u, &queue_create_info);
    device_create_info.setPEnabledFeatures(nullptr);
    device_create_info.setPNext(&features2);
#ifdef _DEBUG
    device_create_info.enabledLayerCount   = 1;
    device_create_info.ppEnabledLayerNames = &validation_layer;
#else
    device_create_info.enabledLayerCount   = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
#endif
    std::vector<const char*>     device_extensions;
    vk::PhysicalDeviceProperties properties = physical_device.getProperties();
    if (properties.vendorID == VK_VENDOR_ID_NVIDIA)
    {
        device_extensions.push_back(VK_NV_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME);
    } else if (properties.vendorID == VK_VENDOR_ID_AMD)
    {
        device_extensions.push_back(VK_AMD_SHADER_BALLOT_EXTENSION_NAME);
    }
    device_extensions.push_back(VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME);
    device_extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
    device_create_info.enabledExtensionCount   = (uint32_t)device_extensions.size();
    device_create_info.ppEnabledExtensionNames = device_extensions.data();

    auto device = physical_device.createDevice(device_create_info);
    auto queue  = device.getQueue(queue_index, 0);
    impl_       = std::make_shared<GpuHelper>(device, physical_device, queue, queue_index, instance);

    Logger::Get().Debug("Vulkan device and queue are successfully created");
}

void Device::InitializePools()
{
    Logger::Get().Debug("Initializing resource pools");
    // Initialize event pool creation functions.
    event_pool_.SetCreateFn([device = impl_->device]() {
        EventBackend<BackendType::kVulkan>* event = new Event;
        vk::FenceCreateInfo                 fence_info(vk::FenceCreateFlagBits::eSignaled);
        event->Set(device.createFence(fence_info));
        return event;
    });

    event_pool_.SetDeleteFn([device = impl_->device](EventBackend<BackendType::kVulkan>* event) {
        device.destroyFence(event->Get());
        delete event;
    });

    // Initialize command stream pool funcs.
    command_stream_pool_.SetCreateFn([me = this, device = impl_->device, pool = impl_->command_pool]() {
        CommandStreamBackend<BackendType::kVulkan>* stream = new CommandStream(*me);
        vk::CommandBufferAllocateInfo               allocate_info;
        allocate_info.commandPool        = pool;
        allocate_info.level              = vk::CommandBufferLevel::ePrimary;
        allocate_info.commandBufferCount = 1;
        stream->Set(device.allocateCommandBuffers(allocate_info)[0]);
        return stream;
    });

    command_stream_pool_.SetDeleteFn([device = impl_->device, pool = impl_->command_pool](
                                         CommandStreamBackend<BackendType::kVulkan>* command_stream) {
        device.freeCommandBuffers(pool, command_stream->Get());
        command_stream->Set(nullptr);
        delete command_stream;
    });

    // Prepare temporary resource pool for staging buffers.
    temporary_buffer_pool_.SetCreateFn([this]() {
        constexpr size_t kMaxTempBufferSize = 256 * 1024 * 1024;
        AllocatedBuffer* buffer             = new AllocatedBuffer();
        *buffer                             = impl_->CreateStagingBuffer(kMaxTempBufferSize);
        return buffer;
    });

    temporary_buffer_pool_.SetDeleteFn([](AllocatedBuffer* buffer) {
        buffer->Destroy();
        delete buffer;
    });
}

DevicePtrBackend<BackendType::kVulkan>* Device::CreateAllocatedBuffer(size_t size)
{
    return new AllocatedDevicePtr(impl_->CreateStagingBuffer((vk::DeviceSize)size));
}

CommandStreamBase* Device::AllocateCommandStream()
{
    auto stream = dynamic_cast<CommandStreamBackend<BackendType::kVulkan>*>(command_stream_pool_.AcquireObject());
    vk::CommandBufferBeginInfo begin_info;
    stream->Get().begin(begin_info);
    return stream;
}

void Device::ReleaseCommandStream(CommandStreamBase* command_stream_base)
{
    auto command_stream = dynamic_cast<CommandStreamBackend<BackendType::kVulkan>*>(command_stream_base);

    // Release and clear temporary objects back to the pool
    command_stream->ClearTemporaryBuffers();
    command_stream->Get().reset(vk::CommandBufferResetFlagBits::eReleaseResources);

    // Release command stream back to the pool.
    command_stream_pool_.ReleaseObject(command_stream);
}

EventBase* Device::SubmitCommandStream(CommandStreamBase* command_stream_base, EventBase* wait_event_base)
{
    Logger::Get().Debug("Device::SubmitCommandStream()");

    auto command_stream = dynamic_cast<CommandStreamBackend<BackendType::kVulkan>*>(command_stream_base);

    EventBackend<BackendType::kVulkan>* event = event_pool_.AcquireObject();

    if (wait_event_base)
    {
        WaitEvent(wait_event_base);
    }

    command_stream->Get().end();

    impl_->device.resetFences({event->Get()});
    auto cmd_buffer = command_stream->Get();
    impl_->queue.submit(vk::SubmitInfo(0, nullptr, nullptr, 1, &cmd_buffer, 0, nullptr), event->Get());
    return event;
}

void Device::ReleaseEvent(EventBase* event_base)
{
    Logger::Get().Debug("Device::ReleaseEvent()");
    EventBackend<BackendType::kVulkan>* event = dynamic_cast<EventBackend<BackendType::kVulkan>*>(event_base);
    event_pool_.ReleaseObject(event);
}

void Device::WaitEvent(EventBase* event_base)
{
    Logger::Get().Debug("Device::WaitEvent()");
    EventBackend<BackendType::kVulkan>* event = dynamic_cast<EventBackend<BackendType::kVulkan>*>(event_base);
    // Loop until the fence is signalled:
    while (true)
    {
        // Wait for 10ms for the render to complete:
        auto result = impl_->device.waitForFences({{event->Get()}}, VK_TRUE, kInfiniteTime);

        // Check the result - if it's successful we are done:
        if (result == vk::Result::eSuccess)
        {
            break;
        }
        // Otherwise, we took longer than kInfiniteTime:
        Logger::Get().Warn("Wait for fence: {}", vk::to_string(result));

        // If the result wasn't a timeout (e.g. error), we fail:
        if (result != vk::Result::eTimeout)
        {
            throw std::runtime_error("Waiting fence failed.");
        }
    }
}

Device::Device()
{
    CreateDeviceAndCommandQueue();
    InitializePools();
}

Device::Device(VkDevice device, VkPhysicalDevice physical_device, VkQueue command_queue, uint32_t queue_family_index)
    : impl_(std::make_unique<GpuHelper>(device, physical_device, command_queue, queue_family_index))
{
    InitializePools();
}

Device::~Device() = default;

}  // namespace rt::vulkan