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
#include "dx/fallback_intersector.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <unordered_map>

#include "dx/algorithm/radix_sort.h"
#include "dx/algorithm/scan.h"
#include "dx/common.h"
#include "dx/common/dx12_wrappers.h"
#include "dx/geometry_trace.h"
#include "dx/hlbvh_builder.h"
#include "dx/hlbvh_top_level_builder.h"
#include "dx/restructure_hlbvh.h"
#include "dx/scene_trace.h"
#include "dx/update_hlbvh.h"
#include "utils/logger.h"

using namespace rt::dx::algorithm;

namespace rt::dx
{
namespace
{
enum SceneRootSignature
{
    kConstants,
    kBvh,
    kBottomBvhs,
    kRayCount,
    kRays,
    kHits,
    kEntryCount
};
}  // namespace

struct Dx12Intersector::Dx12IntersectorImpl
{
    Dx12IntersectorImpl(ID3D12Device* device)
        : device_(device),
          build_bvh(device),
          build_bvh_top_level(device),
          update_bvh(device),
          restructure_bvh(device),
          trace_geometry_(device),
          trace_scene_(device)
    {
        D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {};
        descriptor_heap_desc.NumDescriptors             = 4096;
        descriptor_heap_desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        descriptor_heap_desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        descriptor_heap_desc.NodeMask                   = 0;
        device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap_));
        descriptor_size_ = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }

    ID3D12Device*      device_;
    BuildHlBvh         build_bvh;
    BuildHlBvhTopLevel build_bvh_top_level;
    UpdateBvh          update_bvh;
    RestructureBvh     restructure_bvh;
    // Descriptors
    ComPtr<ID3D12DescriptorHeap> descriptor_heap_;
    UINT                         descriptors_allocated_ = 0;
    UINT                         descriptor_size_;
    std::mutex                   handle_cache_mutex_;
    struct SceneHandle
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle;
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_handle;
        std::size_t                   geometry_count;
    };
    std::unordered_map<DevicePtrBase*, SceneHandle> handle_cache_;
    // Trace things
    TraceGeometry trace_geometry_;
    TraceScene    trace_scene_;
};  // namespace rt::dx

std::unique_ptr<rt::IntersectorBase> CreateDx12Intersector(ID3D12Device* d3d_device)
{
    return std::make_unique<Dx12Intersector>(d3d_device);
}

namespace
{
std::size_t GetBvhNodeCount(std::size_t leaf_count) { return 2 * leaf_count - 1; }
#define HOST
// Include shader structures.
#include "dx/kernels/build_hlbvh_constants.hlsl"
#include "dx/kernels/bvh2il.hlsl"
#include "dx/kernels/transform.hlsl"
#undef HOST
#undef HOST
}  // namespace

void Dx12Intersector::BuildTriangleMesh(CommandStreamBase*                        command_stream_base,
                                        const std::vector<TriangleMeshBuildInfo>& build_info,
                                        const RRBuildOptions*                     build_options,
                                        DevicePtrBase*                            temporary_buffer,
                                        DevicePtrBase*                            geometry_buffer)
{
    Logger::Get().Debug("Dx12Intersector::BuildTriangleMesh()");
    auto                       command_stream = command_stream_cast(command_stream_base);
    ID3D12GraphicsCommandList* command_list   = command_stream->Get();

    D3D12_GPU_VIRTUAL_ADDRESS result_addr  = device_ptr_cast(geometry_buffer)->GetGPUVirtualAddress();
    D3D12_GPU_VIRTUAL_ADDRESS scratch_addr = device_ptr_cast(temporary_buffer)->GetGPUVirtualAddress();

    assert(build_info.size() == 1);

    D3D12_GPU_VIRTUAL_ADDRESS vertices = device_ptr_cast(build_info[0].vertices)->GetGPUVirtualAddress();
    D3D12_GPU_VIRTUAL_ADDRESS indices  = device_ptr_cast(build_info[0].triangle_indices)->GetGPUVirtualAddress();

    impl_->build_bvh(command_list,
                     vertices,
                     build_info[0].vertex_stride,
                     build_info[0].vertex_count,
                     indices,
                     build_info[0].triangle_count,
                     scratch_addr,
                     result_addr);
    if (build_options && (build_options->build_flags & RR_BUILD_FLAG_BITS_PREFER_FAST_BUILD) == 0)
    {
        impl_->restructure_bvh(command_list, build_info[0].triangle_count, scratch_addr, result_addr);
    }

    auto uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    command_stream->Get()->ResourceBarrier(1, &uav_barrier);
}

PreBuildInfo Dx12Intersector::GetTriangleMeshPreBuildInfo(const std::vector<TriangleMeshBuildInfo>& build_info,
                                                          const RRBuildOptions*                     build_options)
{
    Logger::Get().Debug("Dx12Intersector::GetTriangleMeshPreBuildInfo()");

    PreBuildInfo info;
    info.result_size         = 0;
    info.build_scratch_size  = 0;
    info.update_scratch_size = 0;

    assert(build_info.size() == 1);

    info.result_size = impl_->build_bvh.GetResultDataSize(build_info[0].triangle_count);

    info.build_scratch_size = impl_->build_bvh.GetScratchDataSize(build_info[0].triangle_count);
    size_t restructure_scratch_size =
        (build_options && (build_options->build_flags & RR_BUILD_FLAG_BITS_PREFER_FAST_BUILD) == 0)
            ? impl_->restructure_bvh.GetScratchDataSize(build_info[0].triangle_count)
            : 0;
    info.build_scratch_size  = std::max(info.build_scratch_size, restructure_scratch_size);
    info.update_scratch_size = impl_->update_bvh.GetScratchDataSize(build_info[0].triangle_count);

    return info;
}

PreBuildInfo Dx12Intersector::GetScenePreBuildInfo(uint32_t instance_count, const RRBuildOptions*)
{
    Logger::Get().Debug("Dx12Intersector::GetScenePreBuildInfo()");

    PreBuildInfo info;
    info.update_scratch_size = 0;

    info.result_size        = impl_->build_bvh_top_level.GetResultDataSize(instance_count);
    info.build_scratch_size = impl_->build_bvh_top_level.GetScratchDataSize(instance_count);

    return info;
}

void Dx12Intersector::UpdateTriangleMesh(CommandStreamBase*                        command_stream_base,
                                         const std::vector<TriangleMeshBuildInfo>& build_info,
                                         const RRBuildOptions*,
                                         DevicePtrBase* temporary_buffer,
                                         DevicePtrBase* geometry_buffer)
{
    Logger::Get().Debug("Dx12Intersector::UpdateTriangleMesh()");
    auto                       command_stream = command_stream_cast(command_stream_base);
    ID3D12GraphicsCommandList* command_list   = command_stream->Get();

    D3D12_GPU_VIRTUAL_ADDRESS result_addr  = device_ptr_cast(geometry_buffer)->GetGPUVirtualAddress();
    D3D12_GPU_VIRTUAL_ADDRESS scratch_addr = device_ptr_cast(temporary_buffer)->GetGPUVirtualAddress();

    assert(build_info.size() == 1);

    D3D12_GPU_VIRTUAL_ADDRESS vertices = device_ptr_cast(build_info[0].vertices)->GetGPUVirtualAddress();
    D3D12_GPU_VIRTUAL_ADDRESS indices  = device_ptr_cast(build_info[0].triangle_indices)->GetGPUVirtualAddress();

    impl_->update_bvh(command_list,
                      vertices,
                      build_info[0].vertex_stride,
                      build_info[0].vertex_count,
                      indices,
                      build_info[0].triangle_count,
                      scratch_addr,
                      result_addr);

    auto uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    command_stream->Get()->ResourceBarrier(1, &uav_barrier);
}

void Dx12Intersector::BuildScene(CommandStreamBase* command_stream_base,
                                 const RRInstance*  instances,
                                 uint32_t           instance_count,
                                 const RRBuildOptions*,
                                 DevicePtrBase* temporary_buffer,
                                 DevicePtrBase* scene_buffer)
{
    Logger::Get().Debug("Dx12Intersector::BuildScene()");
    Logger::Get().Debug("Recording scene build with {} instances", instance_count);

    // Allocate buffer and upload data.
    auto                       command_stream = command_stream_cast(command_stream_base);
    ID3D12GraphicsCommandList* command_list   = command_stream->Get();

    auto required_staging_size = instance_count * sizeof(InstanceDescription);
    auto desc_buffer           = command_stream->GetAllocatedTemporaryUAV(required_staging_size);

    // Fill desc buffer
    std::vector<InstanceDescription> instance_descs(instance_count);
    for (auto i = 0u; i < instance_count; ++i)
    {
        instance_descs[i].index = i;
        std::memcpy(&(instance_descs[i].transform), &(instances[i].transform[0][0]), 12 * sizeof(float));
    }

    void* mapped_ptr = nullptr;
    desc_buffer->Map(0, nullptr, &mapped_ptr);
    std::memcpy(mapped_ptr, instance_descs.data(), instance_count * sizeof(InstanceDescription));
    desc_buffer->Unmap(0, nullptr);

    D3D12_GPU_VIRTUAL_ADDRESS     result_addr  = device_ptr_cast(scene_buffer)->GetGPUVirtualAddress();
    D3D12_GPU_VIRTUAL_ADDRESS     scratch_addr = device_ptr_cast(temporary_buffer)->GetGPUVirtualAddress();
    auto                          desc_index   = impl_->descriptors_allocated_++;
    CD3DX12_CPU_DESCRIPTOR_HANDLE cpu_handle(
        impl_->descriptor_heap_->GetCPUDescriptorHandleForHeapStart(), desc_index, impl_->descriptor_size_);

    for (uint32_t i = 0; i < instance_count; i++)
    {
        auto geom_ptr       = reinterpret_cast<DevicePtrBase*>(instances[i].geometry);
        auto resource       = device_ptr_cast(geom_ptr)->Get();

        D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
        desc.ViewDimension                   = D3D12_SRV_DIMENSION_BUFFER;
        desc.Buffer.StructureByteStride      = sizeof(BvhNode);
        desc.Buffer.FirstElement             = 0;
        desc.Buffer.NumElements              = UINT(resource->GetDesc().Width / sizeof(BvhNode));
        desc.Format                          = DXGI_FORMAT_UNKNOWN;
        desc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        impl_->device_->CreateShaderResourceView(resource, &desc, cpu_handle);
        cpu_handle.Offset(impl_->descriptor_size_);
    }
    auto gpu_instance_handle = CD3DX12_GPU_DESCRIPTOR_HANDLE(
        impl_->descriptor_heap_->GetGPUDescriptorHandleForHeapStart(), desc_index, impl_->descriptor_size_);
    {
        std::scoped_lock lock(impl_->handle_cache_mutex_);
        impl_->handle_cache_[scene_buffer] = {cpu_handle, gpu_instance_handle, instance_count};
    }

    impl_->build_bvh_top_level(command_list,
                               desc_buffer->GetGPUVirtualAddress(),
                               gpu_instance_handle,
                               instance_count,
                               impl_->descriptor_heap_,
                               scratch_addr,
                               result_addr);

    auto uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    command_stream->Get()->ResourceBarrier(1, &uav_barrier);
}

size_t Dx12Intersector::GetTraceMemoryRequirements(uint32_t ray_count)
{
    return std::max<size_t>(impl_->trace_scene_.GetScratchSize(ray_count),
                            impl_->trace_geometry_.GetScratchSize(ray_count));
}

void Dx12Intersector::Intersect(CommandStreamBase*     command_stream_base,
                                DevicePtrBase*         scene_buffer,
                                RRIntersectQuery       query,
                                DevicePtrBase*         rays,
                                uint32_t               ray_count,
                                DevicePtrBase*         indirect_ray_count,
                                RRIntersectQueryOutput query_output,
                                DevicePtrBase*         hits,
                                DevicePtrBase*         scratch)
{
    Logger::Get().Debug("Dx12Intersector::Intersect()");

    // Allocate buffer and upload data.
    auto                       command_stream = command_stream_cast(command_stream_base);
    ID3D12GraphicsCommandList* command_list   = command_stream->Get();
    {
        std::scoped_lock lock(impl_->handle_cache_mutex_);
        // trace scene
        if (impl_->handle_cache_.count(scene_buffer))
        {
            auto                      gpu_handle     = impl_->handle_cache_.at(scene_buffer).gpu_handle;
            auto                      instance_count = impl_->handle_cache_.at(scene_buffer).geometry_count;
            D3D12_GPU_VIRTUAL_ADDRESS bvh_address    = device_ptr_cast(scene_buffer)->GetGPUVirtualAddress();
            D3D12_GPU_VIRTUAL_ADDRESS transforms     = RoundUp(
                bvh_address + GetBvhNodeCount(instance_count) * sizeof(BvhNode), BuildHlBvhTopLevel::kAlignment);
            D3D12_GPU_VIRTUAL_ADDRESS indirect_ray_count_address =
                indirect_ray_count ? device_ptr_cast(indirect_ray_count)->GetGPUVirtualAddress() : 0;
            D3D12_GPU_VIRTUAL_ADDRESS rays_address    = device_ptr_cast(rays)->GetGPUVirtualAddress();
            D3D12_GPU_VIRTUAL_ADDRESS hits_address    = device_ptr_cast(hits)->GetGPUVirtualAddress();
            D3D12_GPU_VIRTUAL_ADDRESS scratch_address = device_ptr_cast(scratch)->GetGPUVirtualAddress();
            impl_->trace_scene_(command_list,
                                query,
                                query_output,
                                bvh_address,
                                transforms,
                                gpu_handle,
                                impl_->descriptor_heap_,
                                ray_count,
                                indirect_ray_count_address,
                                rays_address,
                                hits_address,
                                scratch_address);
        }
        // trace geometry
        else
        {
            D3D12_GPU_VIRTUAL_ADDRESS bvh_address = device_ptr_cast(scene_buffer)->GetGPUVirtualAddress();
            D3D12_GPU_VIRTUAL_ADDRESS indirect_ray_count_address =
                indirect_ray_count ? device_ptr_cast(indirect_ray_count)->GetGPUVirtualAddress() : 0;
            D3D12_GPU_VIRTUAL_ADDRESS rays_address    = device_ptr_cast(rays)->GetGPUVirtualAddress();
            D3D12_GPU_VIRTUAL_ADDRESS hits_address    = device_ptr_cast(hits)->GetGPUVirtualAddress();
            D3D12_GPU_VIRTUAL_ADDRESS scratch_address = device_ptr_cast(scratch)->GetGPUVirtualAddress();
            impl_->trace_geometry_(command_list,
                                   query,
                                   query_output,
                                   bvh_address,
                                   ray_count,
                                   indirect_ray_count_address,
                                   rays_address,
                                   hits_address,
                                   scratch_address);
        }
    }
}

Dx12Intersector::Dx12Intersector(ID3D12Device* d3d_device) : impl_(std::make_unique<Dx12IntersectorImpl>(d3d_device)) {}
}  // namespace rt::dx