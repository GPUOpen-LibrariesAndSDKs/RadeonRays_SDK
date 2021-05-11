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

#include "intersector.h"

#include <unordered_map>

#include "utils/logger.h"
#include "vlk/common.h"
#include "vlk/geometry_trace.h"
#include "vlk/hlbvh_builder.h"
#include "vlk/hlbvh_top_level_builder.h"
#include "vlk/restructure_hlbvh.h"
#include "vlk/scene_trace.h"
#include "vlk/shader_manager.h"
#include "vlk/update_hlbvh.h"

namespace rt::vulkan
{
namespace
{
constexpr size_t kMaxInstances = 2048;

struct InstanceDescription
{
    float    transform[12];
    uint32_t index;
    uint32_t padding[3];
};
struct BufferHasher
{
    std::size_t operator()(std::pair<vk::Buffer, size_t> const& k) const
    {
        using std::hash;
        const size_t number = 23;
        return 23 * size_t(VkBuffer(k.first)) + k.second;
    }
};
}  // namespace

struct Intersector::IntersectorImpl
{
    IntersectorImpl(std::shared_ptr<GpuHelper> gpu_helper)
        : gpu_helper_(gpu_helper),
          shader_manager_(gpu_helper->device),
          build_bvh_(gpu_helper_, shader_manager_),
          build_bvh_top_level_(gpu_helper_, shader_manager_),
          update_bvh_(gpu_helper, shader_manager_),
          restructure_bvh_(gpu_helper, shader_manager_),
          trace_geometry_(gpu_helper, shader_manager_),
          trace_scene_(gpu_helper, shader_manager_)
    {
        // allocate temporary buffer for scene building to avoid validation issues
        temporary_buffer_ = gpu_helper_->CreateDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, 4);
    }

    ~IntersectorImpl() { temporary_buffer_.Destroy(); }

    std::shared_ptr<GpuHelper> gpu_helper_;
    ShaderManager              shader_manager_;

    BuildHlBvh         build_bvh_;
    BuildHlBvhTopLevel build_bvh_top_level_;
    UpdateHlBvh        update_bvh_;
    RestructureHlBvh   restructure_bvh_;

    // Trace things
    TraceGeometry                                                                     trace_geometry_;
    TraceScene                                                                        trace_scene_;
    std::unordered_map<std::pair<vk::Buffer, size_t>, ChildrenBvhsDesc, BufferHasher> buffers_cache_;
    AllocatedBuffer                                                                   temporary_buffer_;
};

Intersector::Intersector(std::shared_ptr<GpuHelper> gpu_helper) : impl_(std::make_unique<IntersectorImpl>(gpu_helper))
{
}
Intersector::~Intersector() = default;

std::unique_ptr<IntersectorBase> CreateIntersector(std::shared_ptr<GpuHelper> gpu_helper)
{
    return std::make_unique<Intersector>(gpu_helper);
}

PreBuildInfo Intersector::GetTriangleMeshPreBuildInfo(const std::vector<TriangleMeshBuildInfo>& build_info,
                                                      const RRBuildOptions*                     build_options)
{
    Logger::Get().Debug("Intersector::GetTriangleMeshPreBuildInfo()");

    PreBuildInfo info;
    info.result_size         = 0;
    info.build_scratch_size  = 0;
    info.update_scratch_size = 0;

    assert(build_info.size() == 1);

    info.result_size = impl_->build_bvh_.GetResultDataSize(build_info[0].triangle_count);

    info.build_scratch_size = impl_->build_bvh_.GetScratchDataSize(build_info[0].triangle_count);
    size_t restructure_scratch_size =
        (build_options && (build_options->build_flags & RR_BUILD_FLAG_BITS_PREFER_FAST_BUILD) == 0)
            ? impl_->restructure_bvh_.GetScratchDataSize(build_info[0].triangle_count)
            : 0;
    info.build_scratch_size = std::max(info.build_scratch_size, restructure_scratch_size);

    return info;
}
PreBuildInfo Intersector::GetScenePreBuildInfo(uint32_t instance_count, const RRBuildOptions*)
{
    Logger::Get().Debug("Intersector::GetScenePreBuildInfo()");

    PreBuildInfo info;
    info.result_size         = 0;
    info.build_scratch_size  = 0;
    info.update_scratch_size = 0;

    info.result_size        = impl_->build_bvh_top_level_.GetResultDataSize(instance_count);
    info.build_scratch_size = impl_->build_bvh_top_level_.GetScratchDataSize(instance_count);

    return info;
}
void Intersector::BuildTriangleMesh(CommandStreamBase*                        command_stream_base,
                                    const std::vector<TriangleMeshBuildInfo>& build_info,
                                    const RRBuildOptions*                     build_options,
                                    DevicePtrBase*                            temporary_buffer,
                                    DevicePtrBase*                            geometry_buffer)
{
    Logger::Get().Debug("Intersector::BuildTriangleMesh()");
    vk::CommandBuffer command_buffer = command_stream_cast(command_stream_base);

    assert(build_info.size() == 1);

    vk::Buffer vertices       = device_ptr_cast(build_info[0].vertices);
    size_t     vert_offset    = device_ptr_offset(build_info[0].vertices);
    vk::Buffer indices        = device_ptr_cast(build_info[0].triangle_indices);
    size_t     ind_offset     = device_ptr_offset(build_info[0].triangle_indices);
    vk::Buffer scratch        = device_ptr_cast(temporary_buffer);
    size_t     scratch_offset = device_ptr_offset(temporary_buffer);
    vk::Buffer result         = device_ptr_cast(geometry_buffer);
    size_t     result_offset  = device_ptr_offset(geometry_buffer);

    impl_->build_bvh_(command_buffer,
                      vertices,
                      vert_offset,
                      build_info[0].vertex_stride,
                      build_info[0].vertex_count,
                      indices,
                      ind_offset,
                      build_info[0].triangle_count,
                      scratch,
                      scratch_offset,
                      result,
                      result_offset);

    if (build_options && (build_options->build_flags & RR_BUILD_FLAG_BITS_PREFER_FAST_BUILD) == 0)
    {
        impl_->restructure_bvh_(
            command_buffer, build_info[0].triangle_count, scratch, scratch_offset, result, result_offset);
    }
}
void Intersector::UpdateTriangleMesh(CommandStreamBase*                        command_stream_base,
                                     const std::vector<TriangleMeshBuildInfo>& build_info,
                                     const RRBuildOptions*,
                                     DevicePtrBase*,
                                     DevicePtrBase* geometry_buffer)
{
    Logger::Get().Debug("Intersector::UpdateTriangleMesh()");
    vk::CommandBuffer command_buffer = command_stream_cast(command_stream_base);

    assert(build_info.size() == 1);

    vk::Buffer vertices        = device_ptr_cast(build_info[0].vertices);
    size_t     vertices_offset = device_ptr_offset(build_info[0].vertices);
    vk::Buffer indices         = device_ptr_cast(build_info[0].triangle_indices);
    size_t     indices_offset  = device_ptr_offset(build_info[0].triangle_indices);
    vk::Buffer result          = device_ptr_cast(geometry_buffer);
    size_t     result_offset   = device_ptr_offset(geometry_buffer);

    impl_->update_bvh_(command_buffer,
                       vertices,
                       vertices_offset,
                       build_info[0].vertex_stride,
                       build_info[0].vertex_count,
                       indices,
                       indices_offset,
                       build_info[0].triangle_count,
                       result,
                       result_offset);
}
void Intersector::BuildScene(CommandStreamBase* command_stream_base,
                             const RRInstance*  instances,
                             uint32_t           instance_count,
                             const RRBuildOptions*,
                             DevicePtrBase* temporary_buffer,
                             DevicePtrBase* scene_buffer)
{
    Logger::Get().Debug("Intersector::BuildScene()");
    Logger::Get().Debug("Recording scene build with {} instances", instance_count);

    // Allocate buffer and upload data.
    auto command_stream = dynamic_cast<CommandStreamBackend<BackendType::kVulkan>*>(command_stream_base);

    auto required_staging_size = instance_count * sizeof(InstanceDescription);
    auto desc_buffer           = command_stream->GetAllocatedTemporaryBuffer(required_staging_size);

    // Fill desc buffer
    std::vector<InstanceDescription> instance_descs(instance_count);
    ChildrenBvhsContainer            geometries;
    geometries.reserve(kMaxInstances);
    if (kMaxInstances < instance_count)
    {
        constexpr const char* message = "Too big amount of instances per top-level acc structure";
        Logger::Get().Error(message);
        throw std::runtime_error(message);
    }
    for (auto i = 0u; i < instance_count; ++i)
    {
        instance_descs[i].index = i;
        std::memcpy(&(instance_descs[i].transform), &(instances[i].transform[0][0]), 12 * sizeof(float));
        geometries.push_back(
            std::make_pair(device_ptr_cast(instances[i].geometry), device_ptr_offset(instances[i].geometry)));
    }
    // there are kMaxInstances created buffer in descriptor binding
    // it's needed to fill it out
    for (auto i = instance_count; i < kMaxInstances; ++i)
    {
        geometries.push_back(std::make_pair(impl_->temporary_buffer_.buffer, 0u));
    }

    void* mapped_ptr = desc_buffer->Map();
    std::memcpy(mapped_ptr, instance_descs.data(), instance_count * sizeof(InstanceDescription));
    desc_buffer->Unmap();

    vk::Buffer scratch        = device_ptr_cast(temporary_buffer);
    size_t     scratch_offset = device_ptr_offset(temporary_buffer);
    vk::Buffer result         = device_ptr_cast(scene_buffer);
    size_t     result_offset  = device_ptr_offset(scene_buffer);

    impl_->build_bvh_top_level_(command_stream->Get(),
                                desc_buffer->buffer,
                                0,
                                geometries,
                                instance_count,
                                scratch,
                                scratch_offset,
                                result,
                                result_offset);
    impl_->buffers_cache_[std::make_pair(result, result_offset)] = ChildrenBvhsDesc{instance_count, geometries};
}
void Intersector::Intersect(CommandStreamBase*     command_stream_base,
                            DevicePtrBase*         scene,
                            RRIntersectQuery       query,
                            DevicePtrBase*         rays,
                            uint32_t               ray_count,
                            DevicePtrBase*         indirect_ray_count,
                            RRIntersectQueryOutput query_output,
                            DevicePtrBase*         hits,
                            DevicePtrBase*         scratch)
{
    Logger::Get().Debug("Intersector::Intersect()");
    auto              command_stream = dynamic_cast<CommandStreamBackend<BackendType::kVulkan>*>(command_stream_base);
    vk::CommandBuffer command_buffer = command_stream->Get();

    vk::Buffer scene_buffer     = device_ptr_cast(scene);
    size_t     scene_offset     = device_ptr_offset(scene);
    vk::Buffer rays_buffer      = device_ptr_cast(rays);
    size_t     rays_offset      = device_ptr_offset(rays);
    vk::Buffer ray_count_buffer = indirect_ray_count ? device_ptr_cast(indirect_ray_count) : vk::Buffer();
    size_t     ray_count_offset = indirect_ray_count ? device_ptr_offset(indirect_ray_count) : 0;
    vk::Buffer hits_buffer      = device_ptr_cast(hits);
    size_t     hits_offset      = device_ptr_offset(hits);
    vk::Buffer scratch_buffer   = device_ptr_cast(scratch);
    size_t     scratch_offset   = device_ptr_offset(scratch);
    auto       key              = std::make_pair(scene_buffer, scene_offset);
    if (!impl_->buffers_cache_.count(key))
    {
        impl_->trace_geometry_(command_buffer,
                               query,
                               query_output,
                               scene_buffer,
                               scene_offset,
                               ray_count,
                               ray_count_buffer,
                               ray_count_offset,
                               rays_buffer,
                               rays_offset,
                               hits_buffer,
                               hits_offset,
                               scratch_buffer,
                               scratch_offset);
    } else
    {
        impl_->trace_scene_(command_buffer,
                            query,
                            query_output,
                            scene_buffer,
                            scene_offset,
                            impl_->buffers_cache_.at(key),
                            ray_count,
                            ray_count_buffer,
                            ray_count_offset,
                            rays_buffer,
                            rays_offset,
                            hits_buffer,
                            hits_offset,
                            scratch_buffer,
                            scratch_offset);
    }
}
size_t Intersector::GetTraceMemoryRequirements(uint32_t ray_count)
{
    return std::max<size_t>(impl_->trace_scene_.GetScratchSize(ray_count),
                            impl_->trace_geometry_.GetScratchSize(ray_count));
}
}  // namespace rt::vulkan