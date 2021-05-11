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
#include "geometry_trace.h"

#include <unordered_map>

namespace rt::vulkan
{
namespace
{
constexpr char const* s_trace_full_closest_kernel_name     = "trace_geometry_full_closest.comp.spv";
constexpr char const* s_trace_full_any_kernel_name         = "trace_geometry_full_any.comp.spv";
constexpr char const* s_trace_instance_closest_kernel_name = "trace_geometry_instance_closest.comp.spv";
constexpr char const* s_trace_instance_any_kernel_name     = "trace_geometry_instance_any.comp.spv";

constexpr char const* s_trace_full_closest_indirect_kernel_name     = "trace_geometry_full_closest_i.comp.spv";
constexpr char const* s_trace_full_any_indirect_kernel_name         = "trace_geometry_full_any_i.comp.spv";
constexpr char const* s_trace_instance_closest_indirect_kernel_name = "trace_geometry_instance_closest_i.comp.spv";
constexpr char const* s_trace_instance_any_indirect_kernel_name     = "trace_geometry_instance_any_i.comp.spv";

struct TraceKey
{
    RRIntersectQuery       query;
    RRIntersectQueryOutput query_output;
    bool                   indirect;
    bool                   operator==(const TraceKey& other) const
    {
        return (query == other.query && query_output == other.query_output && indirect == other.indirect);
    }
};
struct KeyHasher
{
    size_t operator()(const TraceKey& k) const
    {
        using std::hash;
        constexpr size_t kNumber = 17;
        return kNumber * kNumber * k.query + kNumber * k.query_output + uint32_t(k.indirect);
    }
};
struct TraceValue
{
    TraceValue(char const* kernel_name) : name(kernel_name) {}
    char const*                name;
    std::vector<DescriptorSet> desc_set;
    ShaderPtr                  kernel;
};

constexpr uint32_t kGroupSize = 128u;
constexpr uint32_t kStackSize = 64u;
}  // namespace

struct TraceGeometry::TraceGeometryImpl
{
    std::shared_ptr<GpuHelper> gpu_helper_;
    ShaderManager const&       shader_manager_;

    // shader things
    std::unordered_map<TraceKey, TraceValue, KeyHasher> trace_kernels_ = {
        {{RR_INTERSECT_QUERY_CLOSEST, RR_INTERSECT_QUERY_OUTPUT_FULL_HIT, false}, {s_trace_full_closest_kernel_name}},
        {{RR_INTERSECT_QUERY_CLOSEST, RR_INTERSECT_QUERY_OUTPUT_INSTANCE_ID, false},
         {s_trace_instance_closest_kernel_name}},
        {{RR_INTERSECT_QUERY_ANY, RR_INTERSECT_QUERY_OUTPUT_FULL_HIT, false}, {s_trace_full_any_kernel_name}},
        {{RR_INTERSECT_QUERY_ANY, RR_INTERSECT_QUERY_OUTPUT_INSTANCE_ID, false}, {s_trace_instance_any_kernel_name}},

        {{RR_INTERSECT_QUERY_CLOSEST, RR_INTERSECT_QUERY_OUTPUT_FULL_HIT, true},
         {s_trace_full_closest_indirect_kernel_name}},
        {{RR_INTERSECT_QUERY_CLOSEST, RR_INTERSECT_QUERY_OUTPUT_INSTANCE_ID, true},
         {s_trace_instance_closest_indirect_kernel_name}},
        {{RR_INTERSECT_QUERY_ANY, RR_INTERSECT_QUERY_OUTPUT_FULL_HIT, true}, {s_trace_full_any_indirect_kernel_name}},
        {{RR_INTERSECT_QUERY_ANY, RR_INTERSECT_QUERY_OUTPUT_INSTANCE_ID, true},
         {s_trace_instance_any_indirect_kernel_name}}};
    DescriptorCacheTable<5, 1> cache_;

    TraceGeometryImpl(std::shared_ptr<GpuHelper> helper, ShaderManager const& shader_manager)
        : gpu_helper_(helper), shader_manager_(shader_manager), cache_(gpu_helper_)
    {
        Init();
    }
    void Init()
    {
        for (auto& kernel : trace_kernels_)
        {
            kernel.second.kernel   = shader_manager_.CreateKernel(kernel.second.name);
            kernel.second.desc_set = shader_manager_.CreateDescriptorSets(kernel.second.kernel);
            shader_manager_.PrepareKernel(kernel.second.name, kernel.second.desc_set);
        }
    }
    ~TraceGeometryImpl()
    {
        for (auto kernel : trace_kernels_)
        {
            for (auto& desc : kernel.second.desc_set)
            {
                gpu_helper_->device.destroyDescriptorSetLayout(desc.layout_);
            }
        }
    }
};

TraceGeometry::TraceGeometry(std::shared_ptr<GpuHelper> gpu_helper, ShaderManager const& shader_manager)
    : impl_(std::make_unique<TraceGeometryImpl>(gpu_helper, shader_manager))
{
}
TraceGeometry::~TraceGeometry() = default;
void TraceGeometry::operator()(vk::CommandBuffer      command_buffer,
                               RRIntersectQuery       query,
                               RRIntersectQueryOutput query_output,
                               vk::Buffer             bvh,
                               size_t                 bvh_offset,
                               uint32_t               ray_count,
                               vk::Buffer             ray_count_buffer,
                               size_t                 ray_count_buffer_offset,
                               vk::Buffer             rays,
                               size_t                 rays_offset,
                               vk::Buffer             hits,
                               size_t                 hits_offset,
                               vk::Buffer             scratch,
                               size_t                 scratch_offset)
{
    auto descriptor_set = GetDescriptor(query,
                                        query_output,
                                        bvh,
                                        bvh_offset,
                                        ray_count_buffer,
                                        ray_count_buffer_offset,
                                        rays,
                                        rays_offset,
                                        hits,
                                        hits_offset,
                                        scratch,
                                        scratch_offset);

    ShaderPtr kernel         = impl_->trace_kernels_.at({query, query_output, bool(ray_count_buffer)}).kernel;
    uint32_t  num_groups     = CeilDivide(ray_count, kGroupSize);
    uint32_t  constants[]    = {ray_count};

    // Set prim counter push constant.
    impl_->gpu_helper_->EncodePushConstant(kernel->pipeline_layout, 0u, sizeof(constants), constants, command_buffer);

    impl_->gpu_helper_->EncodeBindDescriptorSets(&descriptor_set, 1u, 0u, kernel->pipeline_layout, command_buffer);

    // Launch kernel.
    impl_->shader_manager_.EncodeDispatch1D(*kernel, num_groups, command_buffer);

    impl_->gpu_helper_->EncodeBufferBarrier(hits,
                                            vk::AccessFlagBits::eShaderWrite,
                                            vk::AccessFlagBits::eShaderRead,
                                            vk::PipelineStageFlagBits::eComputeShader,
                                            vk::PipelineStageFlagBits::eComputeShader,
                                            command_buffer);
}

size_t TraceGeometry::GetScratchSize(uint32_t ray_count) const { return sizeof(uint32_t) * kStackSize * ray_count; }

vk::DescriptorSet TraceGeometry::GetDescriptor(RRIntersectQuery       query,
                                               RRIntersectQueryOutput query_output,
                                               vk::Buffer             bvh,
                                               size_t                 bvh_offset,
                                               vk::Buffer             ray_count_buffer,
                                               size_t                 ray_count_buffer_offset,
                                               vk::Buffer             rays,
                                               size_t                 rays_offset,
                                               vk::Buffer             hits,
                                               size_t                 hits_offset,
                                               vk::Buffer             scratch,
                                               size_t                 scratch_offset)
{
    std::array<vk::Buffer, 5> key = {bvh, ray_count_buffer, rays, hits, scratch};
    if (impl_->cache_.Contains(key))
    {
        return impl_->cache_.Get(key)[0];
    }
    std::vector<vk::DescriptorBufferInfo> info = {{bvh, bvh_offset, VK_WHOLE_SIZE}, {rays, rays_offset, VK_WHOLE_SIZE}};
    if (ray_count_buffer)
    {
        info.emplace_back(ray_count_buffer, ray_count_buffer_offset, VK_WHOLE_SIZE);
    }
    info.emplace_back(hits, hits_offset, VK_WHOLE_SIZE);
    info.emplace_back(scratch, scratch_offset, VK_WHOLE_SIZE);

    auto              trace_value    = impl_->trace_kernels_.at({query, query_output, bool(ray_count_buffer)});
    vk::DescriptorSet trace_desc_set = impl_->gpu_helper_->AllocateDescriptorSet(trace_value.desc_set[0].layout_);
    impl_->gpu_helper_->WriteDescriptorSet(trace_desc_set, info.data(), (uint32_t)info.size());

    impl_->cache_.Push(key, {trace_desc_set});
    return trace_desc_set;
}
}  // namespace rt::vulkan
