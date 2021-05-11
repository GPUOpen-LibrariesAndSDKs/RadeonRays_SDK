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
#include "utils/memory_layout.h"
#include "vlk/gpu_helper.h"
#include "vlk/shader_manager.h"

namespace rt::vulkan::algorithm
{
/**
 * @brief Parallel prefix sum algorithm.
 **/
class Scan
{
public:
    Scan(std::shared_ptr<GpuHelper> helper, ShaderManager const& shader_manager);
    ~Scan();
    /**
     * @brief Record prefix sum into a command list.
     *
     * @param command_list Command list to record to.
     * @param input_keys Input data pointer.
     * @param output_keys Output data pointer.
     * @param scratch_data Scratch area pointer.
     * @param size Number of elements to scan.
     **/
    void operator()(vk::CommandBuffer command_buffer,
                    vk::Buffer        input_keys,
                    vk::DeviceSize    input_offset,
                    vk::DeviceSize    input_size,
                    vk::Buffer        output_keys,
                    vk::DeviceSize    output_offset,
                    vk::DeviceSize    output_size,
                    vk::Buffer        scratch_data,
                    vk::DeviceSize    scratch_offset,
                    uint32_t          num_keys);
    /**
     * @brief Get the amount of scratch memory in bytes required to scan specified amount of elements.
     *
     * @param size Number of elements to scan.
     **/
    size_t GetScratchDataSize(uint32_t num_keys) const;

private:
    void AdjustLayouts(uint32_t num_keys) const;
    void UpdateDescriptorSets(vk::Buffer     input_keys,
                              vk::DeviceSize input_offset,
                              vk::DeviceSize input_size,
                              vk::Buffer     output_keys,
                              vk::DeviceSize output_offset,
                              vk::DeviceSize output_size,
                              vk::Buffer     scratch_data,
                              vk::DeviceSize scratch_offset);

private:
    struct ScanImpl;
    std::unique_ptr<ScanImpl> impl_;

    static constexpr std::uint32_t num_iterations_ = 8;
};
}  // namespace rt::vulkan::algorithm