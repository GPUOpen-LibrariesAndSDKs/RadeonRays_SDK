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
#include <fstream>
#include <mutex>
#include <unordered_map>
#include <vector>

// clang-format off
#include "utils/warning_push.h"
#include "vlk/spirv_tools/spirv_glsl.hpp"
#include <vulkan/vulkan.hpp>
#include "utils/warning_ignore_general.h"
#include "utils/warning_pop.h"
// clang-format on

namespace rt::vulkan
{
struct Shader
{
    vk::ShaderModule                                         module = nullptr;
    std::vector<vk::PushConstantRange>                       push_constant_ranges;
    std::vector<std::vector<vk::DescriptorSetLayoutBinding>> bindings;
    vk::PipelineLayout                                       pipeline_layout = nullptr;
    vk::Pipeline                                             pipeline        = nullptr;
    bool                                                     set             = false;
};

using ShaderPtr = std::shared_ptr<Shader>;

struct DescriptorSet
{
    struct BufferBinding
    {
        BufferBinding() : buffer(nullptr), offset(0), range(VK_WHOLE_SIZE) {}

        BufferBinding(vk::Buffer buffer, vk::DeviceSize offset = 0, vk::DeviceSize range = VK_WHOLE_SIZE)
            : buffer(buffer), offset(offset), range(range)
        {
        }

        BufferBinding(const vk::DescriptorBufferInfo& buffer_info)
            : buffer(buffer_info.buffer), offset(buffer_info.offset), range(buffer_info.range)
        {
        }

        vk::Buffer     buffer;
        vk::DeviceSize offset;
        vk::DeviceSize range;

        void swap(BufferBinding& other)
        {
            std::swap(buffer, other.buffer);
            std::swap(offset, other.offset);
            std::swap(range, other.range);
        }

        vk::Buffer GetBuffer() const { return buffer; }

        friend bool operator!=(const BufferBinding& bb1, const BufferBinding& bb2)
        {
            return bb1.buffer != bb2.buffer || bb1.range != bb2.range || bb1.offset != bb2.offset;
        }
    };

    struct Binding
    {
        BufferBinding buffer_binding_;

        uint32_t           descriptor_count_ = 0;
        vk::DescriptorType type_             = vk::DescriptorType::eStorageBuffer;

        bool is_enabled = true;
    };

    vk::DescriptorSetLayout               layout_;
    vk::DescriptorSet                     descriptor_set_;
    std::unordered_map<uint32_t, Binding> bindings_;
};

class ShaderManager
{
public:
    using KernelID = std::string;

    ShaderManager(vk::Device device);
    ~ShaderManager();

    /// Initialize kernel with given pipeline layout and cache it
    ShaderPtr CreateKernel(KernelID const& id) const;

    /// Initialize compute pipelines
    void PrepareKernel(KernelID const& id, std::vector<DescriptorSet> const& descriptor_sets) const;

    /// Create descriptor set based on shader
    std::vector<DescriptorSet> CreateDescriptorSets(ShaderPtr shader) const;

    /// Create descriptor set based on shader
    DescriptorSet CreateDescriptorSet(ShaderPtr shader, std::uint32_t descriptor_set_id) const;

    /**
        Dispatch 1D grid of threads.
    */
    void EncodeDispatch1D(Shader const& kernel, std::uint32_t num_groups, vk::CommandBuffer& command_buffer) const;

    /**
        Dispatch 2D grid of threads.
    */
    void EncodeDispatch2D(Shader const&        kernel,
                          std::uint32_t const* num_groups,
                          vk::CommandBuffer&   command_buffer) const;

    /**
        Dispatch 1D grid of threads using parameters from the buffer.
    */
    void EncodeDispatch1DIndirect(Shader const&      kernel,
                                  vk::Buffer         num_groups,
                                  vk::DeviceSize     num_groups_offset,
                                  vk::CommandBuffer& command_buffer) const;

private:
    /// Get cached kernel
    bool GetKernel(KernelID const& id, ShaderPtr& kernel) const;

    /// Set kernel with provided layout
    ShaderPtr SetupKernel(KernelID const& id) const;

    void PopulateShaderBindings(vk::ShaderStageFlags       binding_stage_flags,
                                spirv_cross::CompilerGLSL& glsl,
                                Shader&                    shader) const;

    static std::uint32_t GetNumDescriptorSets(spirv_cross::CompilerGLSL& glsl);
    static std::uint32_t GetDescriptorSetIndex(spirv_cross::CompilerGLSL& glsl, std::uint32_t set_idx);

    static void PopulateBindings(spirv_cross::CompilerGLSL&                   glsl,
                                 std::uint32_t                                set_id,
                                 vk::ShaderStageFlags                         stage_flags,
                                 std::vector<vk::DescriptorSetLayoutBinding>& bindings);

    static void PopulatePushConstants(vk::ShaderStageFlags       binding_stage_flags,
                                      spirv_cross::CompilerGLSL& glsl,
                                      Shader&                    shader);

    vk::DescriptorSetLayout CreateDescriptorSetLayout(
        const std::vector<vk::DescriptorSetLayoutBinding>& bindings) const;

    vk::Device device_;
    // Cached kernels.
    mutable std::unordered_map<KernelID, ShaderPtr> kernels_;

    // mutex to protect write to kernels_
    mutable std::mutex kernels_mutex_;

    // default layout
    vk::DescriptorSetLayout empty_descriptor_set_layout_;
};

}  // namespace rt::vulkan

