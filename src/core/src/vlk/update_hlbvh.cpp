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

#include "update_hlbvh.h"

#include "vlk/common.h"
#include "vlk/radix_sort.h"

namespace rt::vulkan
{
namespace
{
// Updating kernels
constexpr char const* s_reset_update_bvh_kernel_name = "lbvh_reset_update_flags.comp.spv";
constexpr char const* s_fit_aabb_kernel_name         = "lbvh_update_mesh.comp.spv";

constexpr uint32_t kTrianglesPerThread = 8u;
constexpr uint32_t kGroupSize          = 128u;
constexpr uint32_t kTrianglesPerGroup  = kTrianglesPerThread * kGroupSize;

uint32_t GetBvhInternalNodeCount(uint32_t leaf_count) { return leaf_count - 1; }
uint32_t GetBvhNodeCount(uint32_t leaf_count) { return 2 * leaf_count - 1; }
}  // namespace

struct UpdateHlBvh::UpdateHlBvhImpl
{
    std::shared_ptr<GpuHelper> gpu_helper_;
    ShaderManager const&       shader_manager_;

    // radix sort algo object
    algorithm::RadixSortKeyValue radix_sort_;

    // Descriptor sets
    std::vector<DescriptorSet> update_sets_;
    DescriptorCacheTable<3, 1> cache_;

    ShaderPtr reset_bvh_kernel_ = nullptr;
    ShaderPtr fit_aabb_kernel_  = nullptr;

    mutable uint32_t current_triangle_count_ = 0u;

    UpdateHlBvhImpl(std::shared_ptr<GpuHelper> helper, ShaderManager const& manager)
        : gpu_helper_(helper), shader_manager_(manager), radix_sort_(helper, manager), cache_(helper)
    {
        Init();
    }
    void Init()
    {
        ShaderManager::KernelID reset_bvh_id = s_reset_update_bvh_kernel_name;
        ShaderManager::KernelID fit_aabb_id  = s_fit_aabb_kernel_name;

        fit_aabb_kernel_ = shader_manager_.CreateKernel(fit_aabb_id);
        update_sets_     = shader_manager_.CreateDescriptorSets(fit_aabb_kernel_);
        shader_manager_.PrepareKernel(fit_aabb_id, update_sets_);

        reset_bvh_kernel_ = shader_manager_.CreateKernel(reset_bvh_id);
        shader_manager_.PrepareKernel(reset_bvh_id, update_sets_);
    }

    void AllocateDescriptorSets()
    {
        update_sets_[0].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(update_sets_[0].layout_);
    }
    void AssignDescriptorSetsFromCache(const std::array<vk::Buffer, 3>& keys)
    {
        auto descriptors                = cache_.Get(keys);
        update_sets_[0].descriptor_set_ = descriptors[0];
    }

    bool GetDescriptorSets(const std::array<vk::Buffer, 3>& keys)
    {
        if (cache_.Contains(keys))
        {
            AssignDescriptorSetsFromCache(keys);
            return true;
        } else
        {
            AllocateDescriptorSets();
        }
        return false;
    }
    void PushDescriptorSetsToCache(const std::array<vk::Buffer, 3>& keys, const std::array<vk::DescriptorSet, 1> values)
    {
        cache_.Push(keys, values);
    }

    ~UpdateHlBvhImpl()
    {
        for (auto& desc_set : update_sets_)
        {
            gpu_helper_->device.destroyDescriptorSetLayout(desc_set.layout_);
        }
    }
};

UpdateHlBvh::UpdateHlBvh(std::shared_ptr<GpuHelper> gpu_helper, ShaderManager const& shader_manager)
    : impl_(std::make_unique<UpdateHlBvhImpl>(gpu_helper, shader_manager))
{
}
UpdateHlBvh::~UpdateHlBvh() = default;

void UpdateHlBvh::operator()(vk::CommandBuffer command_buffer,
                             vk::Buffer        vertices,
                             size_t            vertices_offset,
                             uint32_t          vertex_stride,
                             uint32_t          /*vertex_count*/,
                             vk::Buffer        indices,
                             size_t            indices_offset,
                             uint32_t          triangle_count,
                             vk::Buffer        bvh,
                             size_t            bvh_offset)
{
    AdjustLayouts(triangle_count);
    UpdateDescriptors(vertices, vertices_offset, indices, indices_offset, bvh, bvh_offset);

    uint32_t push_consts[] = {vertex_stride, triangle_count};

    /// Reset mesh flags
    {
        // Set prim counter push constant.
        impl_->gpu_helper_->EncodePushConstant(
            impl_->reset_bvh_kernel_->pipeline_layout, 0u, sizeof(push_consts), push_consts, command_buffer);

        // Bind internal and user desc sets.
        std::vector<vk::DescriptorSet> desc_sets = {impl_->update_sets_[0].descriptor_set_};

        impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                     (std::uint32_t)desc_sets.size(),
                                                     0u,
                                                     impl_->reset_bvh_kernel_->pipeline_layout,
                                                     command_buffer);

        // Launch kernel.
        impl_->shader_manager_.EncodeDispatch1D(*impl_->reset_bvh_kernel_, 1u, command_buffer);

        impl_->gpu_helper_->EncodeBufferBarrier(bvh,
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
        std::vector<vk::DescriptorSet> desc_sets = {impl_->update_sets_[0].descriptor_set_};

        // Bind internal and user desc sets.
        impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets.data(),
                                                     (std::uint32_t)desc_sets.size(),
                                                     0u,
                                                     impl_->fit_aabb_kernel_->pipeline_layout,
                                                     command_buffer);
        // Launch kernel.
        auto num_groups = CeilDivide(triangle_count, kTrianglesPerGroup);
        impl_->shader_manager_.EncodeDispatch1D(*impl_->fit_aabb_kernel_, num_groups, command_buffer);
        impl_->gpu_helper_->EncodeBufferBarrier(bvh,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer);
    }
}

void UpdateHlBvh::AdjustLayouts(uint32_t triangle_count) const
{
    if (triangle_count == impl_->current_triangle_count_)
    {
        return;
    }
    impl_->current_triangle_count_ = triangle_count;
}

void UpdateHlBvh::UpdateDescriptors(vk::Buffer vertices,
                                    size_t     vertices_offset,
                                    vk::Buffer indices,
                                    size_t     indices_offset,
                                    vk::Buffer bvh,
                                    size_t     bvh_offset)
{
    if (!impl_->GetDescriptorSets({vertices, indices, bvh}))
    {
        // Build desc set for BVH updates.
        {
            vk::DescriptorBufferInfo buffer_infos[] = {{bvh, bvh_offset, VK_WHOLE_SIZE},
                                                       {indices, indices_offset, VK_WHOLE_SIZE},
                                                       {vertices, vertices_offset, VK_WHOLE_SIZE}};

            impl_->gpu_helper_->WriteDescriptorSet(
                impl_->update_sets_[0].descriptor_set_, buffer_infos, sizeof(buffer_infos) / sizeof(buffer_infos[0]));
        }
        impl_->PushDescriptorSetsToCache({vertices, indices, bvh}, {impl_->update_sets_[0].descriptor_set_});
    }
}

}  // namespace rt::vulkan