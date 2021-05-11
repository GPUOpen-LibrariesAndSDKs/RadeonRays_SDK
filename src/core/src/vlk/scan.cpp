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
#include "scan.h"

#include "vlk/common.h"

namespace rt::vulkan::algorithm
{
namespace
{
enum class ScanType
{
    kBlock,
    kTwoLevel,
    kThreeLevel
};

struct Parameters
{
    Parameters() = default;
    Parameters(uint32_t keys_per_thread, uint32_t group_size, std::string scan_kernel, std::string reduce_kernel)
        : keys_per_thread_(keys_per_thread),
          group_size_(group_size),
          keys_per_group_(keys_per_thread * group_size),
          scan_kernel_name_(std::move(scan_kernel)),
          reduce_kernel_name_(std::move(reduce_kernel))
    {
    }
    auto     CalculateNumGroups(uint32_t num_keys) { return CeilDivide(num_keys, keys_per_group_); }
    ScanType GetScanType(std::uint32_t num_keys)
    {
        if (num_keys <= keys_per_group_)
        {
            return ScanType::kBlock;
        } else
        {
            num_keys = CalculateNumGroups(num_keys);
            if (num_keys <= keys_per_group_)
            {
                return ScanType::kTwoLevel;
            } else
            {
                return ScanType::kThreeLevel;
            }
        }
    }

    uint32_t    keys_per_thread_    = 2u;
    uint32_t    group_size_         = 256u;
    uint32_t    keys_per_group_     = 512u;
    std::string scan_kernel_name_   = "scan_exclusive_add.comp.spv";
    std::string reduce_kernel_name_ = "scan_exclusive_add_group_reduce.comp.spv";
};

struct AmdParameters : public Parameters
{
    AmdParameters()
        : Parameters(4u, 256u, "scan_exclusive_add_amd.comp.spv", "scan_exclusive_add_group_reduce_amd.comp.spv")
    {
    }
};

}  // namespace

struct Scan::ScanImpl
{
    ScanImpl(std::shared_ptr<GpuHelper> helper, ShaderManager const& manager)
        : gpu_helper_(helper), shader_manager_(manager), cache_(helper)
    {
        Init();
    }

    // vulkan internals
    std::shared_ptr<GpuHelper> gpu_helper_;
    ShaderManager const&       shader_manager_;

    // shader things
    std::vector<DescriptorSet>  scan_desc_sets_;
    std::vector<DescriptorSet>  reduce_desc_sets_;
    ShaderPtr                   scan_kernel_;
    ShaderPtr                   reduce_kernel_;
    uint32_t                    num_keys_;
    DescriptorCacheTable<3, 5>  cache_;
    std::shared_ptr<Parameters> parameters_;
    // Scratch space layout.
    enum class ScratchLayout
    {
        kPartSums0,
        kPartSums1
    };

    using ScratchLayoutT                   = MemoryLayout<ScratchLayout, vk::DeviceSize>;
    mutable ScratchLayoutT scratch_layout_ = ScratchLayoutT(kAlignment);

    void Init()
    {
        if (gpu_helper_->IsAmdDevice())
        {
            parameters_ = std::make_shared<AmdParameters>();
        } else
        {
            parameters_ = std::make_shared<Parameters>();
        }
        scan_kernel_    = shader_manager_.CreateKernel(parameters_->scan_kernel_name_);
        scan_desc_sets_ = shader_manager_.CreateDescriptorSets(scan_kernel_);
        shader_manager_.PrepareKernel(parameters_->scan_kernel_name_, scan_desc_sets_);
        scan_desc_sets_.push_back(scan_desc_sets_[0]);
        scan_desc_sets_.push_back(scan_desc_sets_[0]);

        reduce_kernel_    = shader_manager_.CreateKernel(parameters_->reduce_kernel_name_);
        reduce_desc_sets_ = shader_manager_.CreateDescriptorSets(reduce_kernel_);
        shader_manager_.PrepareKernel(parameters_->reduce_kernel_name_, reduce_desc_sets_);
        reduce_desc_sets_.push_back(reduce_desc_sets_[0]);
    }
    void AllocateDescriptorSets()
    {
        reduce_desc_sets_[0].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(reduce_desc_sets_[0].layout_);
        reduce_desc_sets_[1].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(reduce_desc_sets_[1].layout_);
        scan_desc_sets_[0].descriptor_set_   = gpu_helper_->AllocateDescriptorSet(scan_desc_sets_[0].layout_);
        scan_desc_sets_[1].descriptor_set_   = gpu_helper_->AllocateDescriptorSet(scan_desc_sets_[1].layout_);
        scan_desc_sets_[2].descriptor_set_   = gpu_helper_->AllocateDescriptorSet(scan_desc_sets_[2].layout_);
    }

    bool GetDescriptorSets(const std::array<vk::Buffer, 3>&)
    {
        // there is no option to assign from cache, so just allocate new one
        // if we assign and buffer is the same we will get the issue with incorrect descriptor set
        // works fine under validation but for some reason scratch buffer migth be same for different meshes
        AllocateDescriptorSets();
        return false;
    }
    void PushDescriptorSetsToCache(const std::array<vk::Buffer, 3>& keys, const std::array<vk::DescriptorSet, 5> values)
    {
        cache_.Push(keys, values);
    }
    ~ScanImpl()
    {
        // there are only one layout with a few descriptor sets
        // the same
        gpu_helper_->device.destroyDescriptorSetLayout(reduce_desc_sets_[0].layout_);
        gpu_helper_->device.destroyDescriptorSetLayout(scan_desc_sets_[0].layout_);
    }
};

Scan::Scan(std::shared_ptr<GpuHelper> helper, ShaderManager const& shader_manager)
    : impl_(std::make_unique<ScanImpl>(helper, shader_manager))
{
}
Scan::~Scan() = default;

void Scan::operator()(vk::CommandBuffer command_buffer,
                      vk::Buffer        input_keys,
                      vk::DeviceSize    input_offset,
                      vk::DeviceSize    input_size,
                      vk::Buffer        output_keys,
                      vk::DeviceSize    output_offset,
                      vk::DeviceSize    output_size,
                      vk::Buffer        scratch_data,
                      vk::DeviceSize    scratch_offset,
                      uint32_t          num_keys)
{
    // we call adjusting since we may have called it with different parameters before
    AdjustLayouts(num_keys);
    UpdateDescriptorSets(
        input_keys, input_offset, input_size, output_keys, output_offset, output_size, scratch_data, scratch_offset);
    auto scan_type         = impl_->parameters_->GetScanType(num_keys);
    auto part_sums0_offset = impl_->scratch_layout_.offset_of(ScanImpl::ScratchLayout::kPartSums0);
    auto part_sums0_size   = (vk::DeviceSize)impl_->scratch_layout_.size_of(ScanImpl::ScratchLayout::kPartSums0);
    auto part_sums1_offset = impl_->scratch_layout_.offset_of(ScanImpl::ScratchLayout::kPartSums1);
    auto part_sums1_size   = (vk::DeviceSize)impl_->scratch_layout_.size_of(ScanImpl::ScratchLayout::kPartSums1);

    if (scan_type != ScanType::kBlock)
    {
        // Reduce to part sum 0
        uint32_t reduce0_push_consts[] = {num_keys};

        impl_->gpu_helper_->EncodePushConstant(impl_->reduce_kernel_->pipeline_layout,
                                               0u,
                                               sizeof(reduce0_push_consts),
                                               reduce0_push_consts,
                                               command_buffer);

        vk::DescriptorSet reduce0_desc_sets[] = {impl_->reduce_desc_sets_[0].descriptor_set_};

        // Bind internal and user desc sets.
        impl_->gpu_helper_->EncodeBindDescriptorSets(reduce0_desc_sets,
                                                     sizeof(reduce0_desc_sets) / sizeof(reduce0_desc_sets[0]),
                                                     0u,
                                                     impl_->reduce_kernel_->pipeline_layout,
                                                     command_buffer);

        // Launch the kernel.
        uint32_t num_groups_level_1 = impl_->parameters_->CalculateNumGroups(num_keys);
        impl_->shader_manager_.EncodeDispatch1D(*impl_->reduce_kernel_, num_groups_level_1, command_buffer);

        // Part sum barrier.
        impl_->gpu_helper_->EncodeBufferBarrier(scratch_data,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer,
                                                part_sums0_offset,
                                                part_sums0_size);

        if (scan_type != ScanType::kTwoLevel)
        {
            // Reduce to part sum 1
            uint32_t reduce1_push_consts[] = {num_groups_level_1};

            impl_->gpu_helper_->EncodePushConstant(impl_->reduce_kernel_->pipeline_layout,
                                                   0u,
                                                   sizeof(reduce1_push_consts),
                                                   reduce1_push_consts,
                                                   command_buffer);

            vk::DescriptorSet reduce1_desc_sets[] = {impl_->reduce_desc_sets_[1].descriptor_set_};

            // Bind internal and user desc sets.
            impl_->gpu_helper_->EncodeBindDescriptorSets(reduce1_desc_sets,
                                                         sizeof(reduce1_desc_sets) / sizeof(reduce1_desc_sets[0]),
                                                         0u,
                                                         impl_->reduce_kernel_->pipeline_layout,
                                                         command_buffer);

            // Launch the kernel.
            uint32_t num_groups_level_2 = impl_->parameters_->CalculateNumGroups(num_groups_level_1);
            impl_->shader_manager_.EncodeDispatch1D(*impl_->reduce_kernel_, num_groups_level_2, command_buffer);

            // Part sum barrier.
            impl_->gpu_helper_->EncodeBufferBarrier(scratch_data,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    part_sums1_offset,
                                                    part_sums1_size);

            // Scan part sum 1, no add
            uint32_t scan2_push_consts[] = {num_groups_level_2, 0};

            impl_->gpu_helper_->EncodePushConstant(
                impl_->scan_kernel_->pipeline_layout, 0u, sizeof(scan2_push_consts), scan2_push_consts, command_buffer);

            vk::DescriptorSet scan2_desc_sets[] = {impl_->scan_desc_sets_[2].descriptor_set_};

            // Bind internal and user desc sets.
            impl_->gpu_helper_->EncodeBindDescriptorSets(scan2_desc_sets,
                                                         sizeof(scan2_desc_sets) / sizeof(scan2_desc_sets[0]),
                                                         0u,
                                                         impl_->scan_kernel_->pipeline_layout,
                                                         command_buffer);

            // Launch the kernel.
            impl_->shader_manager_.EncodeDispatch1D(*impl_->scan_kernel_, 1u, command_buffer);

            // Part sum barrier.
            impl_->gpu_helper_->EncodeBufferBarrier(scratch_data,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    part_sums1_offset,
                                                    part_sums1_size);
        }

        // Scan part sum 0, add if not two level
        uint32_t scan1_push_consts[] = {num_groups_level_1, (scan_type != ScanType::kTwoLevel) ? 1u : 0u};

        impl_->gpu_helper_->EncodePushConstant(
            impl_->scan_kernel_->pipeline_layout, 0u, sizeof(scan1_push_consts), scan1_push_consts, command_buffer);

        vk::DescriptorSet scan1_desc_sets[] = {impl_->scan_desc_sets_[1].descriptor_set_};

        // Bind internal and user desc sets.
        impl_->gpu_helper_->EncodeBindDescriptorSets(scan1_desc_sets,
                                                     sizeof(scan1_desc_sets) / sizeof(scan1_desc_sets[0]),
                                                     0u,
                                                     impl_->scan_kernel_->pipeline_layout,
                                                     command_buffer);

        // Launch the kernel.
        uint32_t num_groups_scan_level_1 = impl_->parameters_->CalculateNumGroups(num_groups_level_1);
        impl_->shader_manager_.EncodeDispatch1D(*impl_->scan_kernel_, num_groups_scan_level_1, command_buffer);

        // Part sum barrier.
        impl_->gpu_helper_->EncodeBufferBarrier(scratch_data,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer,
                                                part_sums0_offset,
                                                part_sums0_size);
    }

    // Scan input to output, add if not block
    std::uint32_t scan0_push_consts[] = {num_keys, (scan_type != ScanType::kBlock) ? 1u : 0u};

    impl_->gpu_helper_->EncodePushConstant(
        impl_->scan_kernel_->pipeline_layout, 0u, sizeof(scan0_push_consts), scan0_push_consts, command_buffer);

    vk::DescriptorSet scan0_desc_sets[] = {impl_->scan_desc_sets_[0].descriptor_set_};

    // Bind internal and user desc sets.
    impl_->gpu_helper_->EncodeBindDescriptorSets(scan0_desc_sets,
                                                 sizeof(scan0_desc_sets) / sizeof(scan0_desc_sets[0]),
                                                 0u,
                                                 impl_->scan_kernel_->pipeline_layout,
                                                 command_buffer);

    // Launch the kernel.
    uint32_t num_groups_scan_level_0 = impl_->parameters_->CalculateNumGroups(num_keys);
    impl_->shader_manager_.EncodeDispatch1D(*impl_->scan_kernel_, num_groups_scan_level_0, command_buffer);

    // Output barrier.
    impl_->gpu_helper_->EncodeBufferBarrier(output_keys,
                                            vk::AccessFlagBits::eShaderWrite,
                                            vk::AccessFlagBits::eShaderRead,
                                            vk::PipelineStageFlagBits::eComputeShader,
                                            vk::PipelineStageFlagBits::eComputeShader,
                                            command_buffer,
                                            output_offset,
                                            output_size);
}

size_t Scan::GetScratchDataSize(std::uint32_t num_keys) const
{
    AdjustLayouts(num_keys);
    return impl_->scratch_layout_.total_size();
}

void Scan::AdjustLayouts(uint32_t num_keys) const
{
    if (impl_->num_keys_ == num_keys)
    {
        return;
    }
    impl_->num_keys_ = num_keys;

    impl_->scratch_layout_.Reset();
    auto size_level_1 = CeilDivide(num_keys, impl_->parameters_->keys_per_group_);
    impl_->scratch_layout_.AppendBlock<uint32_t>(ScanImpl::ScratchLayout::kPartSums0, size_level_1);
    uint32_t size_level_2 = 0u;
    if (size_level_1 > impl_->parameters_->keys_per_group_)
    {
        size_level_2 = CeilDivide(size_level_1, impl_->parameters_->keys_per_group_);
    }
    // in case of level 2 not needed just fake the allocation size
    impl_->scratch_layout_.AppendBlock<uint32_t>(ScanImpl::ScratchLayout::kPartSums1,
                                                 size_level_2 ? size_level_2 : kAlignment);
}

void Scan::UpdateDescriptorSets(vk::Buffer     input_keys,
                                vk::DeviceSize input_offset,
                                vk::DeviceSize input_size,
                                vk::Buffer     output_keys,
                                vk::DeviceSize output_offset,
                                vk::DeviceSize output_size,
                                vk::Buffer     scratch_data,
                                vk::DeviceSize scratch_offset)
{
    impl_->scratch_layout_.SetBaseOffset(scratch_offset);
    if (!impl_->GetDescriptorSets({input_keys, output_keys, scratch_data}))
    {
        vk::DeviceSize part_sums0_offset = impl_->scratch_layout_.offset_of(ScanImpl::ScratchLayout::kPartSums0);
        vk::DeviceSize part_sums0_size =
            (vk::DeviceSize)impl_->scratch_layout_.size_of(ScanImpl::ScratchLayout::kPartSums0);
        vk::DeviceSize part_sums1_offset = impl_->scratch_layout_.offset_of(ScanImpl::ScratchLayout::kPartSums1);
        vk::DeviceSize part_sums1_size =
            (vk::DeviceSize)impl_->scratch_layout_.size_of(ScanImpl::ScratchLayout::kPartSums1);

        vk::DescriptorBufferInfo reduce0_infos[] = {{input_keys, input_offset, input_size},
                                                    {scratch_data, part_sums0_offset, part_sums0_size}};

        vk::DescriptorBufferInfo reduce1_infos[] = {{scratch_data, part_sums0_offset, part_sums0_size},
                                                    {scratch_data, part_sums1_offset, part_sums1_size}};

        impl_->gpu_helper_->WriteDescriptorSet(impl_->reduce_desc_sets_[0].descriptor_set_,
                                               reduce0_infos,
                                               sizeof(reduce0_infos) / sizeof(reduce0_infos[0]));

        impl_->gpu_helper_->WriteDescriptorSet(impl_->reduce_desc_sets_[1].descriptor_set_,
                                               reduce1_infos,
                                               sizeof(reduce1_infos) / sizeof(reduce1_infos[0]));

        // Update descriptors.
        vk::DescriptorBufferInfo scan0_infos[] = {{input_keys, input_offset, input_size},
                                                  {scratch_data, part_sums0_offset, part_sums0_size},
                                                  {output_keys, output_offset, output_size}};

        vk::DescriptorBufferInfo scan1_infos[] = {{scratch_data, part_sums0_offset, part_sums0_size},
                                                  {scratch_data, part_sums1_offset, part_sums1_size},
                                                  {scratch_data, part_sums0_offset, part_sums0_size}};

        vk::DescriptorBufferInfo scan2_infos[] = {{scratch_data, part_sums1_offset, part_sums1_size},
                                                  {scratch_data, part_sums1_offset, part_sums1_size},
                                                  {scratch_data, part_sums1_offset, part_sums1_size}};

        impl_->gpu_helper_->WriteDescriptorSet(
            impl_->scan_desc_sets_[0].descriptor_set_, scan0_infos, sizeof(scan0_infos) / sizeof(scan0_infos[0]));

        impl_->gpu_helper_->WriteDescriptorSet(
            impl_->scan_desc_sets_[1].descriptor_set_, scan1_infos, sizeof(scan1_infos) / sizeof(scan1_infos[0]));

        impl_->gpu_helper_->WriteDescriptorSet(
            impl_->scan_desc_sets_[2].descriptor_set_, scan2_infos, sizeof(scan2_infos) / sizeof(scan2_infos[0]));

        impl_->PushDescriptorSetsToCache({input_keys, output_keys, scratch_data},
                                         {impl_->reduce_desc_sets_[0].descriptor_set_,
                                          impl_->reduce_desc_sets_[1].descriptor_set_,
                                          impl_->scan_desc_sets_[0].descriptor_set_,
                                          impl_->scan_desc_sets_[1].descriptor_set_,
                                          impl_->scan_desc_sets_[2].descriptor_set_});
    }
}

}  // namespace rt::vulkan::algorithm