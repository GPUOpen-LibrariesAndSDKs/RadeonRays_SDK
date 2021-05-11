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

#include "restructure_hlbvh.h"

#include "vlk/common.h"
#include "vlk/radix_sort.h"

namespace rt::vulkan
{
namespace
{
// Building kernels
constexpr char const* s_init_primitive_count_kernel_name = "init_primitive_count.comp.spv";
constexpr char const* s_find_roots_kernel_name           = "find_treelet_roots.comp.spv";
constexpr char const* s_restructure_bvh_kernel_name      = "restructure_bvh.comp.spv";
constexpr uint32_t    kGroupSize                         = 64u;
constexpr uint32_t    kMinPrimitivesPerTreelet           = 64u;
constexpr uint32_t    kOptimizationSteps                 = 3u;

uint32_t GetBvhInternalNodeCount(uint32_t leaf_count) { return leaf_count - 1; }
uint32_t GetBvhNodeCount(uint32_t leaf_count) { return 2 * leaf_count - 1; }
}  // namespace

struct RestructureHlBvh::RestructureHlBvhImpl
{
    // Result buffer layout.
    enum class ResultLayout
    {
        kBvh
    };

    // Scratch space layout.
    enum class ScratchLayout
    {
        kTreeletCount,
        kTreeletRoots,
        kPrimitiveCounts
    };

    std::shared_ptr<GpuHelper> gpu_helper_;
    ShaderManager const&       shader_manager_;

    // radix sort algo object
    algorithm::RadixSortKeyValue radix_sort_;

    // Descriptor sets
    std::vector<DescriptorSet> restructure_sets_;
    DescriptorCacheTable<2, 1> cache_;

    ShaderPtr init_primitive_count_kernel_ = nullptr;
    ShaderPtr find_roots_kernel_           = nullptr;
    ShaderPtr restructure_bvh_kernel_      = nullptr;

    using ResultLayoutT  = MemoryLayout<ResultLayout, vk::DeviceSize>;
    using ScratchLayoutT = MemoryLayout<ScratchLayout, vk::DeviceSize>;

    mutable uint32_t       current_triangle_count_ = 0u;
    mutable ResultLayoutT  result_layout_          = ResultLayoutT(kAlignment);
    mutable ScratchLayoutT scratch_layout_         = ScratchLayoutT(kAlignment);

    RestructureHlBvhImpl(std::shared_ptr<GpuHelper> helper, ShaderManager const& manager)
        : gpu_helper_(helper), shader_manager_(manager), radix_sort_(helper, manager), cache_(helper)
    {
        Init();
    }
    void Init()
    {
        ShaderManager::KernelID init_primitive_count_id = s_init_primitive_count_kernel_name;
        ShaderManager::KernelID find_roots_id           = s_find_roots_kernel_name;
        ShaderManager::KernelID restructure_bvh_id      = s_restructure_bvh_kernel_name;

        find_roots_kernel_ = shader_manager_.CreateKernel(find_roots_id);
        restructure_sets_  = shader_manager_.CreateDescriptorSets(find_roots_kernel_);
        shader_manager_.PrepareKernel(find_roots_id, restructure_sets_);

        restructure_bvh_kernel_ = shader_manager_.CreateKernel(restructure_bvh_id);
        shader_manager_.PrepareKernel(restructure_bvh_id, restructure_sets_);

        init_primitive_count_kernel_ = shader_manager_.CreateKernel(init_primitive_count_id);
        shader_manager_.PrepareKernel(init_primitive_count_id, restructure_sets_);
    }

    void AllocateDescriptorSets()
    {
        restructure_sets_[0].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(restructure_sets_[0].layout_);
    }

    bool GetDescriptorSets(const std::array<vk::Buffer, 2>&)
    {
        // there is no option to assign from cache, so just allocate new one
        // if we assign and buffer is the same we will get the issue with incorrect descriptor set
        // works fine under validation but for some reason scratch buffer migth be same for different meshes
        AllocateDescriptorSets();
        return false;
    }
    void PushDescriptorSetsToCache(const std::array<vk::Buffer, 2>& keys, const std::array<vk::DescriptorSet, 1> values)
    {
        cache_.Push(keys, values);
    }

    ~RestructureHlBvhImpl()
    {
        for (auto& desc_set : restructure_sets_)
        {
            gpu_helper_->device.destroyDescriptorSetLayout(desc_set.layout_);
        }
    }
};

RestructureHlBvh::RestructureHlBvh(std::shared_ptr<GpuHelper> gpu_helper, ShaderManager const& shader_manager)
    : impl_(std::make_unique<RestructureHlBvhImpl>(gpu_helper, shader_manager))
{
}
RestructureHlBvh::~RestructureHlBvh() = default;

void RestructureHlBvh::operator()(vk::CommandBuffer command_buffer,
                                  uint32_t          triangle_count,
                                  vk::Buffer        scratch,
                                  size_t            scratch_offset,
                                  vk::Buffer        result,
                                  size_t            result_offset)
{
    AdjustLayouts(triangle_count);
    UpdateDescriptors(scratch, scratch_offset, result, result_offset);
    // scratch layout: temporary buffers
    auto treelet_counts_offset = impl_->scratch_layout_.offset_of(RestructureHlBvhImpl::ScratchLayout::kTreeletCount);
    auto treelet_counts_size   = impl_->scratch_layout_.size_of(RestructureHlBvhImpl::ScratchLayout::kTreeletCount);
    auto treelet_roots_offset  = impl_->scratch_layout_.offset_of(RestructureHlBvhImpl::ScratchLayout::kTreeletRoots);
    auto treelet_roots_size    = impl_->scratch_layout_.size_of(RestructureHlBvhImpl::ScratchLayout::kTreeletRoots);
    auto primitive_counts_offset =
        impl_->scratch_layout_.offset_of(RestructureHlBvhImpl::ScratchLayout::kPrimitiveCounts);
    auto primitive_counts_size = impl_->scratch_layout_.size_of(RestructureHlBvhImpl::ScratchLayout::kPrimitiveCounts);

    uint32_t min_prims_per_treelet = kMinPrimitivesPerTreelet;

    for (uint32_t step = 0u; step < kOptimizationSteps; step++)
    {
        uint32_t push_consts[] = {min_prims_per_treelet, triangle_count};
        /// Init structures
        {
            // Set prim counter push constant.
            impl_->gpu_helper_->EncodePushConstant(impl_->init_primitive_count_kernel_->pipeline_layout,
                                                   0u,
                                                   sizeof(push_consts),
                                                   push_consts,
                                                   command_buffer);

            // Bind internal and user desc sets.
            std::vector<vk::DescriptorSet> desc_sets = {impl_->restructure_sets_[0].descriptor_set_};

            impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                         (std::uint32_t)desc_sets.size(),
                                                         0u,
                                                         impl_->init_primitive_count_kernel_->pipeline_layout,
                                                         command_buffer);

            // Launch kernel.
            auto num_groups = CeilDivide(triangle_count, kGroupSize);
            impl_->shader_manager_.EncodeDispatch1D(*impl_->init_primitive_count_kernel_, num_groups, command_buffer);

            impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    primitive_counts_offset,
                                                    primitive_counts_size);
            impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    treelet_counts_offset,
                                                    treelet_counts_size);
        }

        /// Find roots of treelet to start from
        {
            // Set prim counter push constant.
            impl_->gpu_helper_->EncodePushConstant(
                impl_->find_roots_kernel_->pipeline_layout, 0u, sizeof(push_consts), push_consts, command_buffer);

            // Bind internal and user desc sets.
            std::vector<vk::DescriptorSet> desc_sets = {impl_->restructure_sets_[0].descriptor_set_};

            impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                         (std::uint32_t)desc_sets.size(),
                                                         0u,
                                                         impl_->find_roots_kernel_->pipeline_layout,
                                                         command_buffer);

            // Launch kernel.
            auto num_groups = CeilDivide(triangle_count, kGroupSize);
            impl_->shader_manager_.EncodeDispatch1D(*impl_->find_roots_kernel_, num_groups, command_buffer);

            impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    treelet_counts_offset,
                                                    treelet_counts_size);
            impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    treelet_roots_offset,
                                                    treelet_roots_size);
            impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    primitive_counts_offset,
                                                    primitive_counts_size);
        }

        /// Restructure given BVH
        {
            // Set prim counter push constant.
            impl_->gpu_helper_->EncodePushConstant(
                impl_->restructure_bvh_kernel_->pipeline_layout, 0u, sizeof(push_consts), push_consts, command_buffer);

            // Bind internal and user desc sets.
            std::vector<vk::DescriptorSet> desc_sets = {impl_->restructure_sets_[0].descriptor_set_};

            impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                         (std::uint32_t)desc_sets.size(),
                                                         0u,
                                                         impl_->restructure_bvh_kernel_->pipeline_layout,
                                                         command_buffer);

            // Launch kernel.
            auto num_groups = CeilDivide(triangle_count, min_prims_per_treelet);
            impl_->shader_manager_.EncodeDispatch1D(*impl_->restructure_bvh_kernel_, num_groups, command_buffer);

            impl_->gpu_helper_->EncodeBufferBarrier(scratch,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    primitive_counts_offset,
                                                    primitive_counts_size);

            impl_->gpu_helper_->EncodeBufferBarrier(result,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer);
        }
        min_prims_per_treelet *= 2;
    }
}

size_t RestructureHlBvh::GetResultDataSize(uint32_t triangle_count) const
{
    AdjustLayouts(triangle_count);
    return impl_->result_layout_.total_size();
}

size_t RestructureHlBvh::GetScratchDataSize(uint32_t triangle_count) const
{
    AdjustLayouts(triangle_count);
    return impl_->scratch_layout_.total_size();
}

void RestructureHlBvh::AdjustLayouts(uint32_t triangle_count) const
{
    if (triangle_count == impl_->current_triangle_count_)
    {
        return;
    }

    impl_->current_triangle_count_ = triangle_count;
    impl_->result_layout_.Reset();
    impl_->scratch_layout_.Reset();
    // Result buffer
    impl_->result_layout_.AppendBlock<BvhNode>(RestructureHlBvhImpl::ResultLayout::kBvh,
                                               GetBvhNodeCount(impl_->current_triangle_count_));
    // Scratch buffer.
    impl_->scratch_layout_.AppendBlock<uint32_t>(RestructureHlBvhImpl::ScratchLayout::kTreeletCount, 1);
    impl_->scratch_layout_.AppendBlock<uint32_t>(RestructureHlBvhImpl::ScratchLayout::kTreeletRoots,
                                                 impl_->current_triangle_count_);
    impl_->scratch_layout_.AppendBlock<uint32_t>(RestructureHlBvhImpl::ScratchLayout::kPrimitiveCounts,
                                                 GetBvhNodeCount(impl_->current_triangle_count_));
}

void RestructureHlBvh::UpdateDescriptors(vk::Buffer scratch,
                                         size_t     scratch_offset,
                                         vk::Buffer result,
                                         size_t     result_offset)
{
    if (!impl_->GetDescriptorSets({scratch, result}))
    {
        // result layout: bvh buffer
        impl_->result_layout_.SetBaseOffset(result_offset);
        auto bvh_offset = impl_->result_layout_.offset_of(RestructureHlBvhImpl::ResultLayout::kBvh);
        auto bvh_size   = impl_->result_layout_.size_of(RestructureHlBvhImpl::ResultLayout::kBvh);
        // scratch layout: temporary buffers
        impl_->scratch_layout_.SetBaseOffset(scratch_offset);
        auto treelet_counts_offset =
            impl_->scratch_layout_.offset_of(RestructureHlBvhImpl::ScratchLayout::kTreeletCount);
        auto treelet_counts_size = impl_->scratch_layout_.size_of(RestructureHlBvhImpl::ScratchLayout::kTreeletCount);
        auto treelet_roots_offset =
            impl_->scratch_layout_.offset_of(RestructureHlBvhImpl::ScratchLayout::kTreeletRoots);
        auto treelet_roots_size = impl_->scratch_layout_.size_of(RestructureHlBvhImpl::ScratchLayout::kTreeletRoots);
        auto primitive_counts_offset =
            impl_->scratch_layout_.offset_of(RestructureHlBvhImpl::ScratchLayout::kPrimitiveCounts);
        auto primitive_counts_size =
            impl_->scratch_layout_.size_of(RestructureHlBvhImpl::ScratchLayout::kPrimitiveCounts);

        // Build desc set for BVH restructuring.
        {
            vk::DescriptorBufferInfo buffer_infos[] = {{result, bvh_offset, bvh_size},
                                                       {scratch, treelet_counts_offset, treelet_counts_size},
                                                       {scratch, treelet_roots_offset, treelet_roots_size},
                                                       {scratch, primitive_counts_offset, primitive_counts_size}};

            impl_->gpu_helper_->WriteDescriptorSet(impl_->restructure_sets_[0].descriptor_set_,
                                                   buffer_infos,
                                                   sizeof(buffer_infos) / sizeof(buffer_infos[0]));
        }

        impl_->PushDescriptorSetsToCache({scratch, result}, {impl_->restructure_sets_[0].descriptor_set_});
    }
}

}  // namespace rt::vulkan