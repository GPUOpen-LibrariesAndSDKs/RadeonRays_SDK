/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

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

namespace Calc {

    // Class that represent Vulkan implementation of a Function
    class FunctionVulkan : public Function {
    public:
        FunctionVulkan(Anvil::Device *in_anvil_device,
                       const Anvil::ShaderModuleStageEntryPoint &in_function_entry_point,
                       Anvil::ShaderModule *in_shader_module,
                       bool in_use_compute_pipe
#if _DEBUG
                , const std::string& in_file_name
#endif
        )
                : Function(), m_anvil_device(in_anvil_device),
                  m_function_entry_point(in_function_entry_point),
                  m_shader_module(in_shader_module), m_parameters(),
                  m_descriptor_set_group(nullptr), m_pipeline_id(~0u),
                  m_use_compute_pipe(in_use_compute_pipe)
#if _DEBUG
        , FileName( in_file_name )
#endif
        { }

        ~FunctionVulkan() {
            // release pipeline_id and all its associated resources
            if (~0u != m_pipeline_id) {
                // release a pipeline layout
                Anvil::PipelineLayout *pipeline_layout = m_anvil_device->get_compute_pipeline_manager()->get_compute_pipeline_layout(
                        m_pipeline_id);
                if (nullptr != pipeline_layout) {
                    pipeline_layout->release();
                }

                m_anvil_device->get_compute_pipeline_manager()->delete_pipeline(
                        m_pipeline_id);
                m_pipeline_id = ~0u;
            }

            // release a descriptor set group
            if (nullptr != m_descriptor_set_group) {
                m_descriptor_set_group->release();
                m_descriptor_set_group = nullptr;
            }

            // release spirv shader module
            m_shader_module->release();

            // release all internally created buffers
            for (auto i = 0U; i < m_parameters.size(); ++i) {
                const Buffer *localBuffer = m_parameters[i];

                if (nullptr != localBuffer) {
                    BufferVulkan *vulkanBuffer = const_cast<BufferVulkan *>( Cast<const BufferVulkan, const Buffer>(
                            localBuffer));
                    if (true == vulkanBuffer->GetIsCreatedInternally()) {
                        delete vulkanBuffer;
                    }
                }
            }
        }

        // Argument setters
        void SetArg(std::uint32_t idx, std::size_t arg_size, void *arg) {
            const Anvil::QueueFamilyBits queueToUse =
                    true == m_use_compute_pipe ? Anvil::QUEUE_FAMILY_COMPUTE_BIT
                                               : Anvil::QUEUE_FAMILY_GRAPHICS_BIT;
            // for single values/vector an additional (internal) buffer has to be created
            Anvil::Buffer *newBuffer = new Anvil::Buffer(m_anvil_device,
                                                         arg_size, queueToUse,
                                                         VK_SHARING_MODE_EXCLUSIVE,
                                                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                                         true, true, arg);

            BufferVulkan *vBuffer = new BufferVulkan(newBuffer, true);

            // set it up as a normal buffer
            SetArg(idx, vBuffer);
        }

        void SetArg(std::uint32_t idx, Buffer const *arg) {
            // release previously attached internal buffers
            if (idx < m_parameters.size()) {
                const Buffer *localBuffer = m_parameters[idx];
                if (nullptr != localBuffer) {
                    BufferVulkan *vulkanBuffer = const_cast<BufferVulkan *>( Cast<const BufferVulkan, const Buffer>(
                            localBuffer));
                    Assert(true == vulkanBuffer->GetIsCreatedInternally());
                    if (true == vulkanBuffer->GetIsCreatedInternally()) {
                        delete vulkanBuffer;
                    }
                }

                m_parameters[idx] = arg;
            }

            else {
                Assert(idx == m_parameters.size());
                m_parameters.push_back(arg);
            }
        }

        void SetArg(std::uint32_t idx, std::size_t size, SharedMemory shmem) {
            VK_EMPTY_IMPLEMENTATION;
        }

        // release references to the parameter buffers.
        void UnreferenceParametersBuffers() {
            // release all internally created buffers
            for (auto i = 0U; i < m_parameters.size(); ++i) {
                const Buffer *localBuffer = m_parameters[i];
                Assert(nullptr != localBuffer);
                BufferVulkan *vulkanBuffer = ConstCast<BufferVulkan>( localBuffer );
                if (false == vulkanBuffer->GetIsCreatedInternally()) {
                    m_parameters[i] = nullptr;
                }
            }
        }

        // setters/getters
        const Anvil::ShaderModuleStageEntryPoint &GetFunctionEntryPoint() const { return m_function_entry_point; }

        const std::vector<Buffer const *> &GetParameters() const { return m_parameters; }

        Anvil::DescriptorSetGroup *GetDescriptorSetGroup() const { return m_descriptor_set_group; }

        void SetDescriptorSetGroup(
                Anvil::DescriptorSetGroup *in_new_descriptor_set_group) {
            m_descriptor_set_group = in_new_descriptor_set_group;
        }

        Anvil::ComputePipelineID GetPipelineID() const { return m_pipeline_id; }

        void SetPipelineID(
                Anvil::ComputePipelineID in_new_pipeline_id) { m_pipeline_id = in_new_pipeline_id; }

    private:
        Anvil::Device *m_anvil_device;
        Anvil::ShaderModuleStageEntryPoint m_function_entry_point;
        Anvil::ShaderModule *m_shader_module;
        std::vector<Buffer const *> m_parameters;

        // descriptor set group used by this function
        Anvil::DescriptorSetGroup *m_descriptor_set_group;

        // Vulkan pipeline attached with the descriptor set group and shader module defined above
        Anvil::ComputePipelineID m_pipeline_id;

        // Whether Vulkan implementation should use Compute pipe or Graphics pipe to dispatch this Function's shader
        bool m_use_compute_pipe;
#if _DEBUG
        std::string FileName;
#endif
    };

}