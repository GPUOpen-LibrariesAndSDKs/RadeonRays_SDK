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
#include <array>
#include <unordered_map>
#include <vector>

#include "vlk/vulkan_wrappers.h"

namespace rt::vulkan
{
// Stubs for allocation
static constexpr vk::DeviceSize kAlignment = 64u;
using BvhNode                              = uint8_t[64];
using Aabb                                 = uint8_t[32];
using Transform                            = uint8_t[48];

template <typename T>
inline vk::CommandBuffer command_stream_cast(T* ptr)
{
    auto base = reinterpret_cast<CommandStreamBase*>(ptr);
    if (auto command_buffer_ptr = dynamic_cast<CommandStreamBackend<BackendType::kVulkan>*>(base))
    {
        return command_buffer_ptr->Get();
    }

    throw std::runtime_error("Bad cast to command_stream");
}
// Cast device arbitrary pointer to DevicePtr.
template <typename T>
inline vk::Buffer device_ptr_cast(T* ptr)
{
    auto base = reinterpret_cast<DevicePtrBase*>(ptr);
    if (auto device_ptr = dynamic_cast<DevicePtrBackend<BackendType::kVulkan>*>(base))
    {
        return device_ptr->Get();
    }

    throw std::runtime_error("Bad cast to device pointer to resource.");
}

// Cast device arbitrary pointer to DevicePtr.
template <typename T>
inline size_t device_ptr_offset(T* ptr)
{
    auto base = reinterpret_cast<DevicePtrBase*>(ptr);
    if (auto device_ptr = dynamic_cast<DevicePtrBackend<BackendType::kVulkan>*>(base))
    {
        return device_ptr->Offset();
    }

    throw std::runtime_error("Bad cast to device pointer to resource.");
}

// Round divide.
template <typename T>
inline T CeilDivide(T val, T div)
{
    return (val + div - 1) / div;
}

template <typename T>
inline T Align(T val, T a)
{
    return (val + (a - 1)) & ~(a - 1);
}

using ChildrenBvhsContainer = std::vector<std::pair<vk::Buffer, size_t>>;
struct ChildrenBvhsDesc
{
    uint32_t                bvhs_count = 0;
    ChildrenBvhsContainer buffers;
};

template <size_t BufSize, size_t DescSize>
class DescriptorCacheTable
{
    struct BuffersHash
    {
        static void hash_combine(size_t& seed, const vk::Buffer& v)
        {
            std::hash<size_t> hasher;
            seed ^= hasher(size_t(VkBuffer(v))) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
        size_t operator()(std::array<vk::Buffer, BufSize> const& bufs) const
        {
            size_t res = 0;
            for (const auto& buf : bufs)
            {
                hash_combine(res, buf);
            }
            return res;
        }
    };

    std::unordered_map<std::array<vk::Buffer, BufSize>, std::array<vk::DescriptorSet, DescSize>, BuffersHash>
                               cache_table;
    std::shared_ptr<GpuHelper> gpu_helper;

public:
    DescriptorCacheTable(std::shared_ptr<GpuHelper> helper) : gpu_helper(helper) {}
    ~DescriptorCacheTable()
    {
        for (const auto& pair : cache_table)
        {
            for (const auto& descriptor : pair.second)
            {
                gpu_helper->device.freeDescriptorSets(gpu_helper->descriptor_pool, {descriptor});
            }
        }
    }
    bool Contains(const std::array<vk::Buffer, BufSize>& key) { return cache_table.count(key); }
    std::array<vk::DescriptorSet, DescSize> Get(const std::array<vk::Buffer, BufSize>& key)
    {
        return cache_table.at(key);
    }
    void Push(std::array<vk::Buffer, BufSize> key, std::array<vk::DescriptorSet, DescSize> value)
    {
        cache_table[key] = value;
    }
};
}  // namespace rt::vulkan