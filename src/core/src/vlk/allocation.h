#pragma once
// clang-format off
#include "utils/warning_push.h"
#include "utils/warning_ignore_general.h"
#include <vulkan/vulkan.hpp>
#include "utils/warning_pop.h"
// clang-format on

namespace rt::vulkan
{
struct Allocation
{
    vk::Device       device;
    vk::DeviceMemory memory;
    vk::DeviceSize   size{0};
    vk::DeviceSize   alignment{0};
    vk::DeviceSize   allocSize{0};
    void*            mapped{nullptr};
    /** @brief Memory propertys flags to be filled by external source at buffer creation (to query at some later point)
     */
    vk::MemoryPropertyFlags memoryPropertyFlags;

    template <typename T = void>
    inline T* Map(size_t offset = 0, VkDeviceSize size = VK_WHOLE_SIZE)
    {
        mapped = device.mapMemory(memory, offset, size, vk::MemoryMapFlags());
        return (T*)mapped;
    }

    inline void Unmap()
    {
        device.unmapMemory(memory);
        mapped = nullptr;
    }

    inline void Copy(size_t csize, const void* data, VkDeviceSize offset = 0) const
    {
        memcpy(static_cast<uint8_t*>(mapped) + offset, data, csize);
    }

    template <typename T>
    inline void Copy(const T& data, VkDeviceSize offset = 0) const
    {
        Copy(sizeof(T), &data, offset);
    }

    template <typename T>
    inline void Copy(const std::vector<T>& data, VkDeviceSize offset = 0) const
    {
        Copy(sizeof(T) * data.size(), data.data(), offset);
    }

    /**
     * Flush a memory range of the buffer to make it visible to the device
     *
     * @note Only required for non-coherent memory
     *
     * @param size (Optional) Size of the memory range to flush. Pass VK_WHOLE_SIZE to flush the complete buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the flush call
     */
    void Flush(vk::DeviceSize csize = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
    {
        return device.flushMappedMemoryRanges(vk::MappedMemoryRange{memory, offset, csize});
    }

    /**
     * Invalidate a memory range of the buffer to make it visible to the host
     *
     * @note Only required for non-coherent memory
     *
     * @param size (Optional) Size of the memory range to invalidate. Pass VK_WHOLE_SIZE to invalidate the complete
     * buffer range.
     * @param offset (Optional) Byte offset from beginning
     *
     * @return VkResult of the invalidate call
     */
    void Invalidate(vk::DeviceSize csize = VK_WHOLE_SIZE, vk::DeviceSize offset = 0)
    {
        return device.invalidateMappedMemoryRanges(vk::MappedMemoryRange{memory, offset, csize});
    }

    virtual void Destroy()
    {
        if (nullptr != mapped)
        {
            Unmap();
        }
        if (memory)
        {
            device.freeMemory(memory);
            memory = vk::DeviceMemory();
        }
    }
};
}  // namespace rt::vulkan