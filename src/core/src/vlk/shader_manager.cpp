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
#include "shader_manager.h"

#include <algorithm>
#include <set>
#include <vector>
#ifdef RR_EMBEDDED_KERNELS
#include "compiled_map_spv.h"
#endif

#ifdef max
#undef max
#endif

namespace
{
static constexpr char* kKernelPath = "../../src/core/src/vlk/kernels/";
#ifdef RR_EMBEDDED_KERNELS
std::vector<std::uint32_t> GetShaderCodeFromFile(std::string const& filename)
{
    auto const&                embed_code = rt::vulkan::shaders::GetStringToCode().at(filename);
    std::vector<std::uint32_t> code(embed_code.first, embed_code.first + embed_code.second);
    return code;
}
#else
std::vector<std::uint32_t> GetShaderCodeFromFile(std::string const& filename)
{
    std::ifstream              in(kKernelPath + filename, std::ios::in | std::ios::binary);
    std::vector<std::uint32_t> code;

    if (in)
    {
        std::streamoff beg = in.tellg();
        in.seekg(0, std::ios::end);
        std::streamoff file_size = in.tellg() - beg;
        in.seekg(0, std::ios::beg);
        code.resize(static_cast<unsigned>((file_size + sizeof(std::uint32_t) - 1) / sizeof(std::uint32_t)));
        in.read((char*)&code[0], file_size);
    } else
    {
        throw std::runtime_error("Cannot read SPIR-V file.");
    }
    return code;
}
#endif
}  // namespace

namespace rt::vulkan
{
std::uint32_t ShaderManager::GetNumDescriptorSets(spirv_cross::CompilerGLSL& glsl)
{
    // The SPIR-V is now parsed, and we can perform reflection on it.
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    std::set<std::uint32_t> sets;

    for (auto& resource : resources.storage_buffers)
    {
        auto set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        sets.insert(set);
    }

    for (auto& resource : resources.storage_images)
    {
        auto set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        sets.insert(set);
    }

    for (auto& resource : resources.sampled_images)
    {
        auto set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        sets.insert(set);
    }

    for (auto& resource : resources.separate_images)
    {
        auto set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        sets.insert(set);
    }

    for (auto& resource : resources.separate_samplers)
    {
        auto set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        sets.insert(set);
    }

    for (auto& resource : resources.uniform_buffers)
    {
        auto set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        sets.insert(set);
    }

    return static_cast<std::uint32_t>(sets.size());
}

std::uint32_t ShaderManager::GetDescriptorSetIndex(spirv_cross::CompilerGLSL& glsl, std::uint32_t set_idx)
{
    // The SPIR-V is now parsed, and we can perform reflection on it.
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    std::set<std::uint32_t> sets;

    for (auto& resource : resources.storage_buffers)
    {
        auto set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
        sets.insert(set);
    }
    // other resources do not matter for us

    auto i = 0u;
    for (auto it = sets.begin(); it != sets.end(); ++it)
    {
        if (i++ == set_idx)
        {
            return *it;
        }
    }
    return UINT32_MAX;
}

void ShaderManager::PopulateBindings(spirv_cross::CompilerGLSL&                   glsl,
                                     std::uint32_t                                set_id,
                                     vk::ShaderStageFlags                         stage_flags,
                                     std::vector<vk::DescriptorSetLayoutBinding>& bindings)
{
    // The SPIR-V is now parsed, and we can perform reflection on it.
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    for (auto& resource : resources.storage_buffers)
    {
        auto set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);

        if (set_id == set)
        {
            VkDescriptorSetLayoutBinding binding;
            binding.binding            = glsl.get_decoration(resource.id, spv::DecorationBinding);
            auto& type                 = glsl.get_type(resource.type_id);
            binding.descriptorCount    = type.array.size() ? type.array[0] : 1u;
            binding.pImmutableSamplers = nullptr;
            binding.descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.stageFlags         = VkShaderStageFlags(stage_flags);
            bindings.push_back(binding);
        }
    }
    // other resources do not matter for us
}

void ShaderManager::PopulateShaderBindings(vk::ShaderStageFlags       binding_stage_flags,
                                           spirv_cross::CompilerGLSL& glsl,
                                           Shader&                    shader) const
{
    shader.bindings.clear();

    auto num_descriptor_sets = GetNumDescriptorSets(glsl);

    for (std::uint32_t i = 0; i < num_descriptor_sets; ++i)
    {
        auto descriptor_index = GetDescriptorSetIndex(glsl, i);
        shader.bindings.resize(static_cast<std::size_t>(descriptor_index) + 1);
        PopulateBindings(glsl, descriptor_index, binding_stage_flags, shader.bindings[descriptor_index]);
    }
}

void ShaderManager::PopulatePushConstants(vk::ShaderStageFlags       binding_stage_flags,
                                          spirv_cross::CompilerGLSL& glsl,
                                          Shader&                    shader)
{
    spirv_cross::ShaderResources resources = glsl.get_shader_resources();

    shader.push_constant_ranges.clear();

    if (resources.push_constant_buffers.empty())
    {
        return;
    }
    auto ranges = glsl.get_active_buffer_ranges(resources.push_constant_buffers.front().id);
    if (ranges.empty())
    {
        return;
    }

    VkPushConstantRange push_constant_range;
    push_constant_range.stageFlags = VkShaderStageFlags(binding_stage_flags);
    push_constant_range.offset     = 0;
    push_constant_range.size       = 0;

    for (std::size_t i = 0; i < ranges.size(); i++)
    {
        push_constant_range.size =
            std::max(push_constant_range.size, static_cast<std::uint32_t>(ranges[i].offset + ranges[i].range));
    }

    shader.push_constant_ranges.push_back(push_constant_range);
}

vk::DescriptorSetLayout ShaderManager::CreateDescriptorSetLayout(
    const std::vector<vk::DescriptorSetLayoutBinding>& bindings) const
{
    vk::DescriptorSetLayoutCreateInfo create_info = {{}, (uint32_t)bindings.size(), bindings.data()};
    std::vector<vk::DescriptorBindingFlags> binding_flags(bindings.size());
    int                                     i = 0;
    for (const auto& binding : bindings)
    {
        if (binding.descriptorCount == 0 && binding.descriptorCount > 1)
        {
            binding_flags[i] = {vk::DescriptorBindingFlagBits::eVariableDescriptorCount};
        }
        i++;
    }
    vk::DescriptorSetLayoutBindingFlagsCreateInfo bindings_create_info = {(uint32_t)bindings.size(),
                                                                          binding_flags.data()};
    create_info.pNext                                                  = &bindings_create_info;
    return device_.createDescriptorSetLayout(create_info);
}

ShaderManager::ShaderManager(vk::Device device) : device_(device)
{
    empty_descriptor_set_layout_ = device_.createDescriptorSetLayout({{}, 0u, nullptr});
}

ShaderManager::~ShaderManager() { device_.destroyDescriptorSetLayout(empty_descriptor_set_layout_); }

ShaderPtr ShaderManager::SetupKernel(KernelID const& id) const
{
    std::vector<std::uint32_t> code = GetShaderCodeFromFile(id);

    spirv_cross::CompilerGLSL glsl(code);

    auto                      kernel = std::shared_ptr<Shader>(new Shader(), [device = device_](Shader* shader) {
        if (shader && shader->pipeline_layout)
        {
            device.destroyPipelineLayout(shader->pipeline_layout);
        }
        if (shader && shader->pipeline)
        {
            device.destroyPipeline(shader->pipeline);
        }
        if (shader && shader->module)
        {
            device.destroyShaderModule(shader->module);
        }
        delete shader;
    });

    vk::ShaderStageFlags binding_stage_flags = {vk::ShaderStageFlagBits::eCompute};

    PopulateShaderBindings(binding_stage_flags, glsl, *kernel);
    PopulatePushConstants(binding_stage_flags, glsl, *kernel);

    kernel->module = device_.createShaderModule({{}, code.size() * sizeof(uint32_t), code.data()});

    return kernel;
}

bool ShaderManager::GetKernel(KernelID const& id, ShaderPtr& kernel) const
{
    auto found_iterator = kernels_.find(id);

    if (found_iterator != kernels_.end())
    {
        kernel = found_iterator->second;
        return true;
    }

    return false;
}

ShaderPtr ShaderManager::CreateKernel(KernelID const& id) const
{
    std::unique_lock<std::mutex> lock(kernels_mutex_);
    ShaderPtr                    kernel;
    if (!GetKernel(id, kernel))
    {
        kernel       = SetupKernel(id);
        kernels_[id] = kernel;
    }

    return kernel;
}

void ShaderManager::PrepareKernel(KernelID const& id, std::vector<DescriptorSet> const& descriptor_sets) const
{
    ShaderPtr kernel;
    if (!GetKernel(id, kernel))
    {
        throw std::runtime_error("Kernel is not registered");
    }

    if (kernel->set)
    {
        return;
    }

    std::vector<vk::DescriptorSetLayout> layouts;
    for (const auto& descriptor_set : descriptor_sets)
    {
        layouts.push_back(descriptor_set.layout_);
    }

    kernel->pipeline_layout                         = device_.createPipelineLayout({{},
                                                            (uint32_t)layouts.size(),
                                                            layouts.data(),
                                                            (uint32_t)kernel->push_constant_ranges.size(),
                                                            kernel->push_constant_ranges.data()});
    vk::PipelineCache                 cache         = nullptr;
    vk::PipelineShaderStageCreateInfo stage_info    = {{}, vk::ShaderStageFlagBits::eCompute, kernel->module, "main"};
    vk::ComputePipelineCreateInfo     pipeline_info = {{}, stage_info, kernel->pipeline_layout};

    kernel->pipeline = device_.createComputePipeline(cache, pipeline_info).value;
    kernel->set      = true;
}

std::vector<DescriptorSet> ShaderManager::CreateDescriptorSets(ShaderPtr shader) const
{
    std::vector<DescriptorSet> sets;
    auto                       num_desc_sets = shader->bindings.size();
    for (auto i = 0u; i < num_desc_sets; i++)
    {
        sets.emplace_back(CreateDescriptorSet(shader, i));
    }

    return sets;
}

void ShaderManager::EncodeDispatch1D(Shader const&      kernel,
                                     std::uint32_t      num_groups,
                                     vk::CommandBuffer& command_buffer) const
{
    command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, kernel.pipeline);
    command_buffer.dispatch(num_groups, 1u, 1u);
}

void ShaderManager::EncodeDispatch2D(Shader const&        kernel,
                                     std::uint32_t const* num_groups,
                                     vk::CommandBuffer&   command_buffer) const
{
    command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, kernel.pipeline);
    command_buffer.dispatch(num_groups[0], num_groups[1], 1u);
}

void ShaderManager::EncodeDispatch1DIndirect(Shader const&      kernel,
                                             vk::Buffer         num_groups,
                                             vk::DeviceSize     num_groups_offset,
                                             vk::CommandBuffer& command_buffer) const
{
    command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, kernel.pipeline);
    command_buffer.dispatchIndirect(num_groups, num_groups_offset);
}

DescriptorSet ShaderManager::CreateDescriptorSet(ShaderPtr shader, std::uint32_t descriptor_set_id) const
{
    DescriptorSet descriptor_set;
    const auto&   bindings_for_space = shader->bindings.at(descriptor_set_id);

    for (const auto& shader_binding : bindings_for_space)
    {
        DescriptorSet::Binding binding;
        binding.type_             = shader_binding.descriptorType;
        binding.descriptor_count_ = shader_binding.descriptorCount;
        descriptor_set.bindings_.insert({shader_binding.binding, binding});
    }

    descriptor_set.layout_ =
        bindings_for_space.empty() ? empty_descriptor_set_layout_ : CreateDescriptorSetLayout(bindings_for_space);

    return descriptor_set;
}
}  // namespace rt::vulkan