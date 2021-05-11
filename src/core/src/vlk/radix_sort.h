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

#include <cstddef>
#include <cstdint>

#include "base/command_stream_base.h"
#include "vlk/common.h"
#include "vlk/gpu_helper.h"
#include "vlk/scan.h"

namespace rt::vulkan::algorithm
{
/**
 * @brief Radix sort algorithm.
 **/
class RadixSortKeyValue
{
public:
    RadixSortKeyValue(std::shared_ptr<GpuHelper> gpu_helper, ShaderManager const& manager);
    ~RadixSortKeyValue();
    /**
     * @brief Record sort into a command list.
     *
     * @param command_list Command list to record to.
     * @param input_keys Input keys pointer.
     * @param output_keys Output keys pointer.
     * @param input_value Input values pointer.
     * @param output_values Output values pointer.
     * @param scratch_data Scratch area pointer.
     * @param size Number of elements to sort.
     **/
    void operator()(vk::CommandBuffer command_buffer,
                    vk::Buffer        input_keys,
                    vk::DeviceSize    input_keys_offset,
                    vk::DeviceSize    input_keys_size,
                    vk::Buffer        output_keys,
                    vk::DeviceSize    output_keys_offset,
                    vk::DeviceSize    output_keys_size,
                    vk::Buffer        input_values,
                    vk::DeviceSize    input_values_offset,
                    vk::DeviceSize    input_values_size,
                    vk::Buffer        output_values,
                    vk::DeviceSize    output_values_offset,
                    vk::DeviceSize    output_values_size,
                    vk::Buffer        scratch_data,
                    vk::DeviceSize    scratch_offset,
                    uint32_t          size);

    /**
     * @brief Get the amount of scratch memory in bytes required to sort specified amount of elements.
     *
     * @param size Number of elements to sort.
     **/
    size_t GetScratchDataSize(uint32_t size) const;

private:
    void AdjustLayouts(uint32_t size) const;
    void UpdateDescriptors(vk::Buffer     input_keys,
                           vk::DeviceSize input_keys_offset,
                           vk::DeviceSize input_keys_size,
                           vk::Buffer     output_keys,
                           vk::DeviceSize output_keys_offset,
                           vk::DeviceSize output_keys_size,
                           vk::Buffer     input_values,
                           vk::DeviceSize input_values_offset,
                           vk::DeviceSize input_values_size,
                           vk::Buffer     output_values,
                           vk::DeviceSize output_values_offset,
                           vk::DeviceSize output_values_size,
                           vk::Buffer     scratch_data,
                           vk::DeviceSize scratch_offset) const;

    struct RadixSortImpl;
    std::unique_ptr<RadixSortImpl> impl_;
};
}  // namespace rt::vulkan::algorithm