#pragma once
#include <stdexcept>

#include "vlk/allocation.h"

namespace rt::vulkan
{
struct AllocatedBuffer : public Allocation
{
private:
    using Parent = Allocation;

public:
    vk::Buffer buffer;
    /** @brief Usage flags to be filled by external source at buffer creation (to query at some later point) */
    vk::BufferUsageFlags     usage_flags;
    vk::DescriptorBufferInfo descriptor;

    operator bool() const { return buffer.operator bool(); }

    /**
     * Attach the allocated memory block to the buffer
     *
     * @param offset (Optional) Byte offset (from the beginning) for the memory region to bind
     *
     * @return VkResult of the bindBufferMemory call
     */
    void Bind(vk::DeviceSize offset = 0) { return device.bindBufferMemory(buffer, memory, offset); }

    /**
     * Setup the default descriptor for this buffer
     *
     * @param size (Optional) Size of the memory range of the descriptor
     * @param offset (Optional) Byte offset from beginning
     *
     */
    void SetupDescriptor(vk::DeviceSize csize = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
    {
        descriptor.offset = offset;
        descriptor.buffer = buffer;
        descriptor.range  = csize;
    }

    /**
     * Copies the specified data to the mapped buffer
     *
     * @param data Pointer to the data to copy
     * @param size Size of the data to copy in machine units
     *
     */
    void CopyTo(void* data, vk::DeviceSize csize)
    {
        if (!mapped)
        {
            throw std::runtime_error("AllocatedBuffer was not mapped");
        }
        memcpy(mapped, data, csize);
    }

    /**
     * Release all Vulkan resources held by this buffer
     */
    void Destroy() override
    {
        if (buffer)
        {
            device.destroy(buffer);
            buffer = vk::Buffer{};
        }
        Parent::Destroy();
    }
};
}  // namespace rt::vulkan
