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
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARIS
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/

#include "hlbvh_top_level_builder.h"

#include "utils/memory_layout.h"
#include "vlk/common.h"
#include "vlk/radix_sort.h"

namespace rt::vulkan
{
namespace
{
// Building kernels
constexpr char const* s_calc_morton_codes_kernel_name = "lbvh_calc_morton_codes_scene.comp.spv";
constexpr char const* s_calc_scene_aabb_kernel_name   = "lbvh_calc_scene_aabb.comp.spv";
constexpr char const* s_emit_bvh_kernel_name          = "lbvh_emit_hierarchy_scene.comp.spv";
constexpr char const* s_fit_aabb_kernel_name          = "lbvh_fit_aabb_scene.comp.spv";
constexpr char const* s_init_kernel_name              = "lbvh_init_scene.comp.spv";

// there is only option to have 1 instance per thread due to descriptor indexing extension implemention
constexpr uint32_t kInstancesPerThread = 8u;
constexpr uint32_t kGroupSize          = 128u;
constexpr uint32_t kInstancesPerGroup  = kInstancesPerThread * kGroupSize;

uint32_t GetBvhInternalNodeCount(uint32_t leaf_count) { return leaf_count - 1; }
uint32_t GetBvhNodeCount(uint32_t leaf_count) { return 2 * leaf_count - 1; }
uint32_t GetTransformsCount(uint32_t leaf_count) { return 2 * leaf_count; }
}  // namespace

struct BuildHlBvhTopLevel::HlBvhTopLevelImpl
{
    // Result buffer layout.
    enum class ResultLayout
    {
        kBvh,
        kTransforms
    };

    // Scratch space layout.
    enum class ScratchLayout
    {
        kAabb,
        kMortonCodes,
        kPrimitiveRefs,
        kSortedMortonCodes,
        kSortedPrimitiveRefs,
        kSortMemory
    };

    std::shared_ptr<GpuHelper> gpu_helper_;
    ShaderManager const&       shader_manager_;

    // radix sort algo object
    algorithm::RadixSortKeyValue radix_sort_;

    // Descriptor sets
    std::vector<DescriptorSet> build_sets_;
    std::vector<DescriptorSet> build_sorted_sets_;
    DescriptorCacheTable<4, 3> cache_;

    ShaderPtr calc_aabb_kernel_         = nullptr;
    ShaderPtr calc_morton_codes_kernel_ = nullptr;
    ShaderPtr emit_bvh_kernel_          = nullptr;
    ShaderPtr fit_aabb_kernel_          = nullptr;
    ShaderPtr init_kernel_              = nullptr;

    using ResultLayoutT  = MemoryLayout<ResultLayout, vk::DeviceSize>;
    using ScratchLayoutT = MemoryLayout<ScratchLayout, vk::DeviceSize>;

    mutable uint32_t       current_instance_count_ = 0u;
    mutable ResultLayoutT  result_layout_          = ResultLayoutT(kAlignment);
    mutable ScratchLayoutT scratch_layout_         = ScratchLayoutT(kAlignment);

    HlBvhTopLevelImpl(std::shared_ptr<GpuHelper> helper, ShaderManager const& manager)
        : gpu_helper_(helper), shader_manager_(manager), radix_sort_(helper, manager), cache_(helper)
    {
        Init();
    }
    void Init()
    {
        ShaderManager::KernelID calc_morton_id = s_calc_morton_codes_kernel_name;
        ShaderManager::KernelID calc_aabb_id   = s_calc_scene_aabb_kernel_name;
        ShaderManager::KernelID emit_bvh_id    = s_emit_bvh_kernel_name;
        ShaderManager::KernelID fit_aabb_id    = s_fit_aabb_kernel_name;
        ShaderManager::KernelID init_aabb_id   = s_init_kernel_name;

        calc_aabb_kernel_ = shader_manager_.CreateKernel(calc_aabb_id);
        build_sets_       = shader_manager_.CreateDescriptorSets(calc_aabb_kernel_);
        shader_manager_.PrepareKernel(calc_aabb_id, build_sets_);

        calc_morton_codes_kernel_ = shader_manager_.CreateKernel(calc_morton_id);
        shader_manager_.PrepareKernel(calc_morton_id, build_sets_);

        init_kernel_ = shader_manager_.CreateKernel(init_aabb_id);
        shader_manager_.PrepareKernel(init_aabb_id, build_sets_);

        emit_bvh_kernel_   = shader_manager_.CreateKernel(emit_bvh_id);
        build_sorted_sets_ = shader_manager_.CreateDescriptorSets(emit_bvh_kernel_);
        shader_manager_.PrepareKernel(emit_bvh_id, build_sorted_sets_);

        fit_aabb_kernel_ = shader_manager_.CreateKernel(fit_aabb_id);
        shader_manager_.PrepareKernel(fit_aabb_id, build_sorted_sets_);
    }

    void AllocateDescriptorSets()
    {
        build_sets_[0].descriptor_set_        = gpu_helper_->AllocateDescriptorSet(build_sets_[0].layout_);
        build_sets_[1].descriptor_set_        = gpu_helper_->AllocateDescriptorSet(build_sets_[1].layout_);
        build_sorted_sets_[0].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(build_sorted_sets_[0].layout_);
        // no need to allocate 1 set since it has the same buffers as build_sets_
    }

    bool GetDescriptorSets(const std::array<vk::Buffer, 4>&)
    {
        // there is no option to assign from cache, so just allocate new one
        // if we assign and buffer is the same we will get the issue with incorrect descriptor set
        // works fine under validation but for some reason scratch buffer migth be same for different scenes
        AllocateDescriptorSets();
        return false;
    }
    void PushDescriptorSetsToCache(const std::array<vk::Buffer, 4>& keys, const std::array<vk::DescriptorSet, 3> values)
    {
        cache_.Push(keys, values);
    }

    ~HlBvhTopLevelImpl()
    {
        for (auto& desc_set : build_sets_)
        {
            gpu_helper_->device.destroyDescriptorSetLayout(desc_set.layout_);
        }
        for (auto& desc_set : build_sorted_sets_)
        {
            gpu_helper_->device.destroyDescriptorSetLayout(desc_set.layout_);
        }
    }
};

BuildHlBvhTopLevel::BuildHlBvhTopLevel(std::shared_ptr<GpuHelper> gpu_helper, ShaderManager const& shader_manager)
    : impl_(std::make_unique<HlBvhTopLevelImpl>(gpu_helper, shader_manager))
{
}
BuildHlBvhTopLevel::~BuildHlBvhTopLevel() = default;

void BuildHlBvhTopLevel::operator()(vk::CommandBuffer            command_buffer,
                                    vk::Buffer                   instance_descs,
                                    size_t                       instance_desc_offset,
                                    ChildrenBvhsContainer const& instances,
                                    uint32_t                     instance_count,
                                    vk::Buffer                   scratch,
                                    size_t                       scratch_offset,
                                    vk::Buffer                   result,
                                    size_t                       result_offset)
{
    AdjustLayouts(instance_count);
    UpdateDescriptors(instance_descs, instance_desc_offset, instances, scratch, scratch_offset, result, result_offset);
    // scratch layout: temporary buffers
    auto aabb_offset          = impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kAabb);
    auto aabb_size            = impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kAabb);
    auto morton_offset        = impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kMortonCodes);
    auto morton_size          = impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kMortonCodes);
    auto primitive_offset     = impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kPrimitiveRefs);
    auto primitive_size       = impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kPrimitiveRefs);
    auto sorted_morton_offset = impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kSortedMortonCodes);
    auto sorted_morton_size   = impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kSortedMortonCodes);
    auto sorted_primitive_offset =
        impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kSortedPrimitiveRefs);
    auto sorted_primitive_size = impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kSortedPrimitiveRefs);
    auto sort_offset           = impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kSortMemory);

    uint32_t push_consts[] = {instance_count};

    /// Init scene AABB
    {
        // Bind internal and user desc sets.
        std::vector<vk::DescriptorSet> desc_sets = {impl_->build_sets_[0].descriptor_set_,
                                                    impl_->build_sets_[1].descriptor_set_};

        impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                     (std::uint32_t)desc_sets.size(),
                                                     0u,
                                                     impl_->init_kernel_->pipeline_layout,
                                                     command_buffer);

        // Launch kernel.
        impl_->shader_manager_.EncodeDispatch1D(*impl_->init_kernel_, 1u, command_buffer);

        impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer,
                                                aabb_offset,
                                                aabb_size);
    }

    /// Calc scene AABB
    {
        // Set prim counter push constant.
        impl_->gpu_helper_->EncodePushConstant(
            impl_->calc_aabb_kernel_->pipeline_layout, 0u, sizeof(push_consts), push_consts, command_buffer);

        // Bind internal and user desc sets.
        std::vector<vk::DescriptorSet> desc_sets = {impl_->build_sets_[0].descriptor_set_,
                                                    impl_->build_sets_[1].descriptor_set_};

        impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                     (std::uint32_t)desc_sets.size(),
                                                     0u,
                                                     impl_->calc_aabb_kernel_->pipeline_layout,
                                                     command_buffer);

        // Launch kernel.
        auto num_groups = CeilDivide(instance_count, kInstancesPerGroup);
        impl_->shader_manager_.EncodeDispatch1D(*impl_->calc_aabb_kernel_, num_groups, command_buffer);

        impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer,
                                                aabb_offset,
                                                aabb_size);
    }
    /// Calculate Morton codes for prims
    {
        // Set prim counter push constant.
        impl_->gpu_helper_->EncodePushConstant(
            impl_->calc_morton_codes_kernel_->pipeline_layout, 0u, sizeof(push_consts), push_consts, command_buffer);

        // Bind internal and user desc sets.
        std::vector<vk::DescriptorSet> desc_sets = {impl_->build_sets_[0].descriptor_set_,
                                                    impl_->build_sets_[1].descriptor_set_};

        // Bind internal and user desc sets.
        impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                     (std::uint32_t)desc_sets.size(),
                                                     0u,
                                                     impl_->calc_morton_codes_kernel_->pipeline_layout,
                                                     command_buffer);

        // Launch kernel.
        auto num_groups = CeilDivide(instance_count, kInstancesPerGroup);
        impl_->shader_manager_.EncodeDispatch1D(*impl_->calc_morton_codes_kernel_, num_groups, command_buffer);

        impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer,
                                                morton_offset,
                                                morton_size);

        impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer,
                                                primitive_offset,
                                                primitive_size);
    }
    /// Sort Morton codes and prim indices
    impl_->radix_sort_(command_buffer,
                       scratch,
                       morton_offset,
                       morton_size,
                       scratch,
                       sorted_morton_offset,
                       (vk::DeviceSize)sorted_morton_size,
                       scratch,
                       primitive_offset,
                       (vk::DeviceSize)primitive_size,
                       scratch,
                       sorted_primitive_offset,
                       (vk::DeviceSize)sorted_primitive_size,
                       scratch,
                       sort_offset,
                       instance_count);

    /// Emit BVH
    {
        // Set prim counter push constant.
        impl_->gpu_helper_->EncodePushConstant(
            impl_->emit_bvh_kernel_->pipeline_layout, 0u, sizeof(push_consts), push_consts, command_buffer);

        // Bind internal and user desc sets.
        std::vector<vk::DescriptorSet> desc_sets = {impl_->build_sorted_sets_[0].descriptor_set_,
                                                    impl_->build_sets_[1].descriptor_set_};

        // Bind internal and user desc sets.
        impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                     (std::uint32_t)desc_sets.size(),
                                                     0u,
                                                     impl_->emit_bvh_kernel_->pipeline_layout,
                                                     command_buffer);
        // Launch kernel.
        auto num_groups = CeilDivide(instance_count, kInstancesPerGroup);
        impl_->shader_manager_.EncodeDispatch1D(*impl_->emit_bvh_kernel_, num_groups, command_buffer);
        impl_->gpu_helper_->EncodeBufferBarrier(result,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer);
    }
    /// Fit bounds
    {
        // Set prim counter push constant.
        impl_->gpu_helper_->EncodePushConstant(
            impl_->fit_aabb_kernel_->pipeline_layout, 0u, sizeof(push_consts), push_consts, command_buffer);

        // Bind internal and user desc sets.
        std::vector<vk::DescriptorSet> desc_sets = {impl_->build_sorted_sets_[0].descriptor_set_,
                                                    impl_->build_sets_[1].descriptor_set_};

        // Bind internal and user desc sets.
        impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                     (std::uint32_t)desc_sets.size(),
                                                     0u,
                                                     impl_->fit_aabb_kernel_->pipeline_layout,
                                                     command_buffer);
        // Launch kernel.
        auto num_groups = CeilDivide(instance_count, kInstancesPerGroup);
        impl_->shader_manager_.EncodeDispatch1D(*impl_->fit_aabb_kernel_, num_groups, command_buffer);
        impl_->gpu_helper_->EncodeBufferBarrier(result,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer);
    }
}

size_t BuildHlBvhTopLevel::GetResultDataSize(uint32_t instance_count) const
{
    AdjustLayouts(instance_count);
    return impl_->result_layout_.total_size();
}

size_t BuildHlBvhTopLevel::GetScratchDataSize(uint32_t instance_count) const
{
    AdjustLayouts(instance_count);
    return impl_->scratch_layout_.total_size();
}

void BuildHlBvhTopLevel::AdjustLayouts(uint32_t instance_count) const
{
    if (instance_count == impl_->current_instance_count_)
    {
        return;
    }
    impl_->current_instance_count_ = instance_count;
    impl_->result_layout_.Reset();
    impl_->scratch_layout_.Reset();
    // Result buffer
    impl_->result_layout_.AppendBlock<BvhNode>(HlBvhTopLevelImpl::ResultLayout::kBvh,
                                               GetBvhNodeCount(impl_->current_instance_count_));
    impl_->result_layout_.AppendBlock<Transform>(HlBvhTopLevelImpl::ResultLayout::kTransforms,
                                                 GetTransformsCount(impl_->current_instance_count_));
    // Scratch buffer.
    impl_->scratch_layout_.AppendBlock<Aabb>(HlBvhTopLevelImpl::ScratchLayout::kAabb, 1);
    impl_->scratch_layout_.AppendBlock<uint32_t>(HlBvhTopLevelImpl::ScratchLayout::kMortonCodes,
                                                 impl_->current_instance_count_);
    impl_->scratch_layout_.AppendBlock<uint32_t>(HlBvhTopLevelImpl::ScratchLayout::kPrimitiveRefs,
                                                 impl_->current_instance_count_);
    impl_->scratch_layout_.AppendBlock<uint32_t>(HlBvhTopLevelImpl::ScratchLayout::kSortedMortonCodes,
                                                 impl_->current_instance_count_);
    impl_->scratch_layout_.AppendBlock<uint32_t>(HlBvhTopLevelImpl::ScratchLayout::kSortedPrimitiveRefs,
                                                 impl_->current_instance_count_);

    auto sort_memory_size = impl_->radix_sort_.GetScratchDataSize(impl_->current_instance_count_);
    impl_->scratch_layout_.AppendBlock<char>(HlBvhTopLevelImpl::ScratchLayout::kSortMemory, sort_memory_size);
}

void BuildHlBvhTopLevel::UpdateDescriptors(vk::Buffer                   instance_desc,
                                           size_t                       instance_desc_offset,
                                           ChildrenBvhsContainer const& instances,
                                           vk::Buffer                   scratch,
                                           size_t                       scratch_offset,
                                           vk::Buffer                   result,
                                           size_t                       result_offset)
{
    if (!impl_->GetDescriptorSets({instance_desc, scratch, result}))
    {
        // result layout: bvh buffer
        impl_->result_layout_.SetBaseOffset((VkDeviceSize)result_offset);
        auto bvh_offset        = impl_->result_layout_.offset_of(HlBvhTopLevelImpl::ResultLayout::kBvh);
        auto bvh_size          = impl_->result_layout_.size_of(HlBvhTopLevelImpl::ResultLayout::kBvh);
        auto transforms_offset = impl_->result_layout_.offset_of(HlBvhTopLevelImpl::ResultLayout::kTransforms);
        auto transforms_size   = impl_->result_layout_.size_of(HlBvhTopLevelImpl::ResultLayout::kTransforms);
        // scratch layout: temporary buffers
        impl_->scratch_layout_.SetBaseOffset((VkDeviceSize)scratch_offset);
        auto aabb_offset      = impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kAabb);
        auto aabb_size        = impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kAabb);
        auto morton_offset    = impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kMortonCodes);
        auto morton_size      = impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kMortonCodes);
        auto primitive_offset = impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kPrimitiveRefs);
        auto primitive_size   = impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kPrimitiveRefs);
        auto sorted_morton_offset =
            impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kSortedMortonCodes);
        auto sorted_morton_size = impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kSortedMortonCodes);
        auto sorted_primitive_offset =
            impl_->scratch_layout_.offset_of(HlBvhTopLevelImpl::ScratchLayout::kSortedPrimitiveRefs);
        auto sorted_primitive_size =
            impl_->scratch_layout_.size_of(HlBvhTopLevelImpl::ScratchLayout::kSortedPrimitiveRefs);

        {
            vk::DescriptorBufferInfo buffer_infos[] = {{result, bvh_offset, bvh_size},
                                                       {result, transforms_offset, transforms_size},
                                                       {scratch, morton_offset, morton_size},
                                                       {scratch, primitive_offset, primitive_size},
                                                       {scratch, aabb_offset, aabb_size}};

            impl_->gpu_helper_->WriteDescriptorSet(
                impl_->build_sets_[0].descriptor_set_, buffer_infos, sizeof(buffer_infos) / sizeof(buffer_infos[0]));
        }

        {
            vk::DescriptorBufferInfo buffer_infos[] = {{result, bvh_offset, bvh_size},
                                                       {result, transforms_offset, transforms_size},
                                                       {scratch, sorted_morton_offset, sorted_morton_size},
                                                       {scratch, sorted_primitive_offset, sorted_primitive_size},
                                                       {scratch, aabb_offset, aabb_size}};

            impl_->gpu_helper_->WriteDescriptorSet(impl_->build_sorted_sets_[0].descriptor_set_,
                                                   buffer_infos,
                                                   sizeof(buffer_infos) / sizeof(buffer_infos[0]));
        }

        {
            vk::DescriptorBufferInfo instance_info{instance_desc, instance_desc_offset, VK_WHOLE_SIZE};
            impl_->gpu_helper_->WriteDescriptorSet(impl_->build_sets_[1].descriptor_set_, &instance_info, 1u, 0u);

            std::vector<vk::DescriptorBufferInfo> buffer_infos;
            for (const auto& instance : instances)
            {
                buffer_infos.push_back({instance.first, instance.second, VK_WHOLE_SIZE});
            }

            impl_->gpu_helper_->WriteDescriptorSet(
                impl_->build_sets_[1].descriptor_set_, buffer_infos.data(), (uint32_t)buffer_infos.size(), 1u);
        }
        impl_->PushDescriptorSetsToCache({instance_desc, scratch, result},
                                         {impl_->build_sets_[0].descriptor_set_,
                                          impl_->build_sets_[1].descriptor_set_,
                                          impl_->build_sorted_sets_[0].descriptor_set_});
    }
}

}  // namespace rt::vulkan