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
#include <functional>
// clang-format off
#include "utils/warning_push.h"
#include "utils/warning_ignore_general.h"
#include <vulkan/vulkan.hpp>
#include "utils/warning_pop.h"
// clang-format on

#include "vlk/buffer.h"

namespace rt::vulkan
{
namespace
{
static constexpr auto VK_VENDOR_ID_AMD    = 0x1002;
static constexpr auto VK_VENDOR_ID_NVIDIA = 0x10de;
static constexpr auto VK_VENDOR_ID_INTEL  = 0x8086;
}  // namespace

struct GpuHelper
{
    static auto constexpr kNumDescriptors = 1000000u;
    static auto constexpr kMaxSets        = 1000000u;

    GpuHelper(vk::Device         dev,
              vk::PhysicalDevice ph_dev,
              vk::Queue          com_queue,
              uint32_t           queue_family,
              vk::Instance       inst = nullptr)
        : instance(inst),
          device(dev),
          physical_device(ph_dev),
          queue(com_queue),
          queue_family_index(queue_family),
          is_external(VkInstance(inst) == VK_NULL_HANDLE)
    {
        Init();
    }
    void Init()
    {
        vk::CommandPoolCreateInfo pool_info;
        pool_info.queueFamilyIndex = queue_family_index;
        pool_info.flags            = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        command_pool               = device.createCommandPool(pool_info);
        device_memory_properties   = physical_device.getMemoryProperties();
        device_properties          = physical_device.getProperties();
        vk::DescriptorPoolSize       desc_pool_size(vk::DescriptorType::eStorageBuffer, kNumDescriptors);
        vk::DescriptorPoolCreateInfo desc_pool_info(
            {vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet}, kMaxSets, 1, &desc_pool_size);
        descriptor_pool = device.createDescriptorPool(desc_pool_info);
    }
    ~GpuHelper()
    {
        queue.waitIdle();
        device.waitIdle();
        if (command_pool)
        {
            device.destroyCommandPool(command_pool);
        }
        if (descriptor_pool)
        {
            device.destroyDescriptorPool(descriptor_pool);
        }
        if (!is_external)
        {
            device.destroy();
            instance.destroy();
        }
    }

    AllocatedBuffer CreateBuffer(const vk::BufferUsageFlags&    usageFlags,
                                 const vk::MemoryPropertyFlags& memoryPropertyFlags,
                                 vk::DeviceSize                 size) const
    {
        AllocatedBuffer result;
        result.device            = device;
        result.size              = size;
        result.descriptor.range  = VK_WHOLE_SIZE;
        result.descriptor.offset = 0;

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.usage = usageFlags;
        buffer_create_info.size  = size;

        result.descriptor.buffer = result.buffer = device.createBuffer(buffer_create_info);

        vk::MemoryRequirements memReqs = device.getBufferMemoryRequirements(result.buffer);
        vk::MemoryAllocateInfo mem_alloc;
        result.allocSize = mem_alloc.allocationSize = memReqs.size;
        mem_alloc.memoryTypeIndex                   = GetMemoryType(memReqs.memoryTypeBits, memoryPropertyFlags);
        result.memory                               = device.allocateMemory(mem_alloc);
        device.bindBufferMemory(result.buffer, result.memory, 0);
        return result;
    }

    AllocatedBuffer CreateStagingBuffer(vk::DeviceSize size, const void* data = nullptr) const
    {
        auto result = CreateBuffer(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc |
                                       vk::BufferUsageFlagBits::eTransferDst,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                   size);
        if (data != nullptr)
        {
            CopyToMemory(result.memory, data, size);
        }
        return result;
    }

    AllocatedBuffer CreateDeviceBuffer(const vk::BufferUsageFlags& usageFlags, vk::DeviceSize size) const
    {
        return CreateBuffer(usageFlags, vk::MemoryPropertyFlagBits::eDeviceLocal, size);
    }

    AllocatedBuffer CreateScratchDeviceBuffer(vk::DeviceSize size)
    {
        AllocatedBuffer result = CreateBuffer(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
                         vk::MemoryPropertyFlagBits::eDeviceLocal,
                         size);
        return result;
    }

    template <typename T>
    AllocatedBuffer CreateStagingBuffer(const std::vector<T>& data) const
    {
        return CreateStagingBuffer(data.size() * sizeof(T), (void*)data.data());
    }

    template <typename T>
    AllocatedBuffer CreateStagingBuffer(const T& data) const
    {
        return CreateStagingBuffer(sizeof(T), &data);
    }

    AllocatedBuffer CreateSizedUniformBuffer(vk::DeviceSize size) const
    {
        auto alignment          = device_properties.limits.minUniformBufferOffsetAlignment;
        auto extra              = size % alignment;
        auto count              = 1;
        auto aligned_size       = size + (alignment - extra);
        auto allocated_size     = count * aligned_size;
        auto result             = CreateBuffer(vk::BufferUsageFlagBits::eUniformBuffer,
                                   vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                   allocated_size);
        result.alignment        = aligned_size;
        result.descriptor.range = result.alignment;
        return result;
    }

    template <typename T>
    AllocatedBuffer CreateUniformBuffer(const T& data) const
    {
        auto result = CreateSizedUniformBuffer(vk::DeviceSize(sizeof(T)));
        result.Map();
        result.Copy(data);
        return result;
    }

    void CopyToMemory(const vk::DeviceMemory& memory,
                      const void*             data,
                      vk::DeviceSize          size,
                      vk::DeviceSize          offset = 0) const
    {
        void* mapped = device.mapMemory(memory, offset, size, vk::MemoryMapFlags());
        memcpy(mapped, data, size);
        device.unmapMemory(memory);
    }

    template <typename T>
    void CopyToMemory(const vk::DeviceMemory& memory, const T& data, size_t offset = 0) const
    {
        CopyToMemory(memory, &data, sizeof(T), offset);
    }

    template <typename T>
    void CopyToMemory(const vk::DeviceMemory& memory, const std::vector<T>& data, size_t offset = 0) const
    {
        CopyToMemory(memory, data.data(), data.size() * sizeof(T), offset);
    }

    void WithPrimaryCommandBuffer(const std::function<void(const vk::CommandBuffer& commandBuffer)>& f) const
    {
        vk::CommandBuffer             command_buffer;
        vk::CommandBufferAllocateInfo allocate_info;
        allocate_info.commandPool        = command_pool;
        allocate_info.level              = vk::CommandBufferLevel::ePrimary;
        allocate_info.commandBufferCount = 1;
        command_buffer                   = device.allocateCommandBuffers(allocate_info)[0];
        command_buffer.begin(vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
        f(command_buffer);
        command_buffer.end();
        queue.submit(vk::SubmitInfo{0, nullptr, nullptr, 1, &command_buffer}, vk::Fence());
        queue.waitIdle();
        device.waitIdle();
        device.freeCommandBuffers(command_pool, command_buffer);
    }

    AllocatedBuffer StageToDeviceBuffer(const vk::BufferUsageFlags& usage, size_t size, const void* data) const
    {
        AllocatedBuffer staging = CreateStagingBuffer(size, data);
        AllocatedBuffer result  = CreateDeviceBuffer(usage | vk::BufferUsageFlagBits::eTransferDst, size);
        WithPrimaryCommandBuffer([&](vk::CommandBuffer copyCmd) {
            copyCmd.copyBuffer(staging.buffer, result.buffer, vk::BufferCopy(0, 0, size));
        });
        staging.Destroy();
        return result;
    }

    template <typename T>
    AllocatedBuffer StageToDeviceBuffer(const vk::BufferUsageFlags& usage, const std::vector<T>& data) const
    {
        return StageToDeviceBuffer(usage, sizeof(T) * data.size(), data.data());
    }

    template <typename T>
    AllocatedBuffer StageToDeviceBuffer(const vk::BufferUsageFlags& usage, const T& data) const
    {
        return StageToDeviceBuffer(usage, sizeof(T), (void*)&data);
    }

    vk::Bool32 GetMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties, uint32_t* typeIndex) const
    {
        for (uint32_t i = 0; i < 32; i++)
        {
            if ((typeBits & 1) == 1)
            {
                if ((device_memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
                {
                    *typeIndex = i;
                    return VK_TRUE;
                }
            }
            typeBits >>= 1;
        }
        return VK_FALSE;
    }

    uint32_t GetMemoryType(uint32_t typeBits, const vk::MemoryPropertyFlags& properties) const
    {
        uint32_t result = 0;
        if (VK_FALSE == GetMemoryType(typeBits, properties, &result))
        {
            throw std::runtime_error("Unable to find memory type " + vk::to_string(properties));
        }
        return result;
    }

    vk::DescriptorSet AllocateDescriptorSet(vk::DescriptorSetLayout const& layout) const
    {
        vk::DescriptorSetAllocateInfo allocate_info(descriptor_pool, 1, &layout);
        auto                          descriptor_sets = device.allocateDescriptorSets(allocate_info);
        return descriptor_sets[0];
    }

    void WriteDescriptorSet(vk::DescriptorSet               desc_set,
                            vk::DescriptorBufferInfo const* buffer_infos,
                            uint32_t                        num_buffer_infos,
                            uint32_t                        dest_binding = 0) const
    {
        vk::WriteDescriptorSet write_descriptor_set(
            desc_set, dest_binding, 0, num_buffer_infos, vk::DescriptorType::eStorageBuffer, nullptr, buffer_infos);

        device.updateDescriptorSets(1, &write_descriptor_set, 0, nullptr);
    }

    void EncodeBufferBarrier(vk::Buffer             buffer,
                             vk::AccessFlags        src_access,
                             vk::AccessFlags        dst_access,
                             vk::PipelineStageFlags src_stage,
                             vk::PipelineStageFlags dst_stage,
                             vk::CommandBuffer&     command_buffer,
                             vk::DeviceSize         offset = 0u,
                             vk::DeviceSize         size   = VK_WHOLE_SIZE) const
    {
        vk::BufferMemoryBarrier barrier(
            src_access, dst_access, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, buffer, offset, size);
        command_buffer.pipelineBarrier(src_stage, dst_stage, {}, nullptr, {barrier}, nullptr);
    }

    void EncodePushConstant(vk::PipelineLayout layout,
                            uint32_t           offset,
                            uint32_t           data_size,
                            void const*        data,
                            vk::CommandBuffer& command_buffer) const
    {
        command_buffer.pushConstants(layout, {vk::ShaderStageFlagBits::eCompute}, offset, data_size, data);
    }

    void EncodeBindDescriptorSet(vk::DescriptorSet  desc_set,
                                 uint32_t           first_set,
                                 vk::PipelineLayout layout,
                                 vk::CommandBuffer& command_buffer) const
    {
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute, layout, first_set, 1u, &desc_set, 0u, nullptr);
    }

    void EncodeBindDescriptorSets(vk::DescriptorSet const* desc_sets,
                                  std::uint32_t            num_desc_sets,
                                  std::uint32_t            first_set,
                                  vk::PipelineLayout       layout,
                                  vk::CommandBuffer&       command_buffer) const
    {
        command_buffer.bindDescriptorSets(
            vk::PipelineBindPoint::eCompute, layout, first_set, num_desc_sets, desc_sets, 0u, nullptr);
    }

    bool IsAmdDevice() const { return device_properties.vendorID == VK_VENDOR_ID_AMD; }
    bool IsNvDevice() const { return device_properties.vendorID == VK_VENDOR_ID_NVIDIA; }

    vk::Instance                       instance           = nullptr;
    vk::Device                         device             = nullptr;
    vk::PhysicalDevice                 physical_device    = nullptr;
    vk::Queue                          queue              = nullptr;
    vk::CommandPool                    command_pool       = nullptr;
    vk::DescriptorPool                 descriptor_pool    = nullptr;
    uint32_t                           queue_family_index = VK_QUEUE_FAMILY_IGNORED;
    vk::PhysicalDeviceProperties       device_properties;
    vk::PhysicalDeviceMemoryProperties device_memory_properties;
    bool                               is_external = false;
};

}  // namespace rt::vulkan