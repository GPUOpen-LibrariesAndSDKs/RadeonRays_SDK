#pragma once

#include <chrono>
#include <iostream>
#include <stack>
#include <cassert>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "dx/dxassist.h"
#include "dx/shader_compiler.h"
#include "mesh_data.h"
#include "radeonrays_dx.h"
#include "stb_image_write.h"

#define CHECK_RR_CALL(x) do {if((x) != RR_SUCCESS) { throw std::runtime_error("Incorrect radeonrays call");}} while(false)

using namespace std::chrono;

class Application
{
public:
    void Run();
private:
    DxAssist dxassist_;
};

void Application::Run()
{
    auto dev = dxassist_.device();

    ComPtr<ID3D12QueryHeap> query_heap;
    D3D12_QUERY_HEAP_DESC   query_heap_desc = {};
    query_heap_desc.Count                   = 2;
    query_heap_desc.Type                    = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    dev->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(&query_heap));

    RRContext context = nullptr;
    CHECK_RR_CALL(rrCreateContextDX(RR_API_VERSION, dxassist_.device(), dxassist_.command_queue(), &context));
    MeshData mesh_data("scene.obj");

    auto vertex_buffer =
        dxassist_.CreateUploadBuffer(sizeof(float) * mesh_data.positions.size(), mesh_data.positions.data());
    auto index_buffer =
        dxassist_.CreateUploadBuffer(sizeof(std::uint32_t) * mesh_data.indices.size(), mesh_data.indices.data());
    auto timestamp_buffer          = dxassist_.CreateUAVBuffer(sizeof(std::uint64_t) * 8);
    auto timestamp_readback_buffer = dxassist_.CreateReadBackBuffer(sizeof(std::uint64_t) * 8);

    RRDevicePtr vertex_ptr = nullptr;
    RRDevicePtr index_ptr  = nullptr;
    rrGetDevicePtrFromD3D12Resource(context, vertex_buffer.Get(), 0, &vertex_ptr);
    rrGetDevicePtrFromD3D12Resource(context, index_buffer.Get(), 0, &index_ptr);

    auto triangle_count = static_cast<UINT>(index_buffer->GetDesc().Width) / sizeof(UINT32) / 3;

    RRGeometryBuildInput    geometry_build_input            = {};
    RRTriangleMeshPrimitive mesh                            = {};
    geometry_build_input.triangle_mesh_primitives           = &mesh;
    geometry_build_input.primitive_type                     = RR_PRIMITIVE_TYPE_TRIANGLE_MESH;
    geometry_build_input.triangle_mesh_primitives->vertices = vertex_ptr;
    geometry_build_input.triangle_mesh_primitives->vertex_count =
        static_cast<UINT>(vertex_buffer->GetDesc().Width) / (3 * sizeof(float));

    geometry_build_input.triangle_mesh_primitives->vertex_stride    = 3 * sizeof(float);
    geometry_build_input.triangle_mesh_primitives->triangle_indices = index_ptr;
    geometry_build_input.triangle_mesh_primitives->triangle_count   = (UINT)triangle_count;
    geometry_build_input.triangle_mesh_primitives->index_type       = RR_INDEX_TYPE_UINT32;
    geometry_build_input.primitive_count                            = 1u;

    std::cout << "Triangle count " << triangle_count << "\n";

    RRBuildOptions options;
    options.build_flags = 0u;

    RRMemoryRequirements geometry_reqs;
    CHECK_RR_CALL(rrGetGeometryBuildMemoryRequirements(context, &geometry_build_input, &options, &geometry_reqs));

    D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    auto geometry = dxassist_.CreateUAVBuffer(geometry_reqs.result_buffer_size);
    std::cout << "Geometry buffer size: " << geometry_reqs.result_buffer_size / 1000000 << "Mb\n";

    RRDevicePtr geometry_ptr;
    CHECK_RR_CALL(rrGetDevicePtrFromD3D12Resource(context, geometry.Get(), 0, &geometry_ptr));

    auto scratch_buffer = dxassist_.CreateUAVBuffer(
        max(geometry_reqs.temporary_build_buffer_size, geometry_reqs.temporary_update_buffer_size));
    std::cout << "Scratch buffer size: "
              << max(geometry_reqs.temporary_build_buffer_size, geometry_reqs.temporary_update_buffer_size) / 1000000
              << "Mb\n";

    RRDevicePtr scratch_ptr = nullptr;
    CHECK_RR_CALL(rrGetDevicePtrFromD3D12Resource(context, scratch_buffer.Get(), 0, &scratch_ptr));

    RRCommandStream command_stream = nullptr;
    CHECK_RR_CALL(rrAllocateCommandStream(context, &command_stream));

    CHECK_RR_CALL(rrCmdBuildGeometry(
        context, RR_BUILD_OPERATION_BUILD, &geometry_build_input, &options, scratch_ptr, geometry_ptr, command_stream));

    RREvent wait_event = nullptr;
    CHECK_RR_CALL(rrSumbitCommandStream(context, command_stream, nullptr, &wait_event));
    CHECK_RR_CALL(rrWaitEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseCommandStream(context, command_stream));

    // built-in intersection
    using Ray = RRRay;
    using Hit = RRHit;

    constexpr uint32_t kResolution = 2048;
    std::vector<Ray>   rays(kResolution * kResolution);

    for (int x = 0; x < kResolution; ++x)
    {
        for (int y = 0; y < kResolution; ++y)
        {
            auto i = kResolution * y + x;

            rays[i].origin[0] = 0.f;
            rays[i].origin[1] = 15.f;
            rays[i].origin[2] = 0.f;

            rays[i].direction[0] = -1.f;
            rays[i].direction[1] = -1.f + (2.f / kResolution) * y;
            rays[i].direction[2] = -1.f + (2.f / kResolution) * x;

            rays[i].min_t = 0.001f;
            rays[i].max_t = 100000.f;
        }
    }

    auto temp_ray_buffer = dxassist_.CreateUploadBuffer(kResolution * kResolution * sizeof(Ray), rays.data());
    auto ray_buffer =
        dxassist_.CreateUAVBuffer(kResolution * kResolution * sizeof(Ray), D3D12_RESOURCE_STATE_COPY_DEST);

    RRDevicePtr rays_ptr = nullptr;
    rrGetDevicePtrFromD3D12Resource(context, ray_buffer.Get(), 0, &rays_ptr);

    auto temp_hit_buffer = dxassist_.CreateReadBackBuffer(kResolution * kResolution * sizeof(Hit));
    auto hit_buffer      = dxassist_.CreateUAVBuffer(kResolution * kResolution * sizeof(Hit));

    RRDevicePtr hits_ptr = nullptr;
    rrGetDevicePtrFromD3D12Resource(context, hit_buffer.Get(), 0, &hits_ptr);

    // Copy ray data.
    auto command_allocator = dxassist_.CreateCommandAllocator();
    auto copy_rays_command_list = dxassist_.CreateCommandList(command_allocator.Get());

    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = ray_buffer.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    copy_rays_command_list->CopyBufferRegion(
        ray_buffer.Get(), 0, temp_ray_buffer.Get(), 0, kResolution * kResolution * sizeof(Ray));
    copy_rays_command_list->ResourceBarrier(1, &barrier);
    copy_rays_command_list->Close();

    auto copy_hits_command_list    = dxassist_.CreateCommandList(command_allocator.Get());
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = hit_buffer.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

    copy_hits_command_list->ResourceBarrier(1, &barrier);
    copy_hits_command_list->CopyBufferRegion(
        temp_hit_buffer.Get(), 0, hit_buffer.Get(), 0, kResolution * kResolution * sizeof(Hit));
    copy_hits_command_list->Close();

    ID3D12CommandList* lists[] = {copy_rays_command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, lists);
    auto ray_copy_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(ray_copy_fence.Get(), 2000);
    while (ray_copy_fence->GetCompletedValue() != 2000) Sleep(0);

    RRCommandStream trace_command_stream = nullptr;
    CHECK_RR_CALL(rrAllocateCommandStream(context, &trace_command_stream));

    size_t scratch_size = 0;
    CHECK_RR_CALL(rrGetTraceMemoryRequirements(context, kResolution * kResolution, &scratch_size));
    auto scratch_trace_buffer = dxassist_.CreateUAVBuffer(scratch_size);
    RRDevicePtr scratch_trace_ptr    = nullptr;
    rrGetDevicePtrFromD3D12Resource(context, scratch_trace_buffer.Get(), 0, &scratch_trace_ptr);

    CHECK_RR_CALL(rrCmdIntersect(context,
                                 geometry_ptr,
                                 RR_INTERSECT_QUERY_CLOSEST,
                                 rays_ptr,
                                 kResolution * kResolution,
                                 nullptr,
                                 RR_INTERSECT_QUERY_OUTPUT_FULL_HIT,
                                 hits_ptr,
                                 scratch_trace_ptr,
                                 trace_command_stream));

    CHECK_RR_CALL(rrSumbitCommandStream(context, trace_command_stream, nullptr, &wait_event));
    CHECK_RR_CALL(rrWaitEvent(context, wait_event));

    CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseCommandStream(context, trace_command_stream));

    ID3D12CommandList* lists1[] = {copy_hits_command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, lists1);
    auto hit_copy_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(hit_copy_fence.Get(), 3000);
    while (hit_copy_fence->GetCompletedValue() != 3000) Sleep(0);

    {
        Hit* mapped_ptr;
        temp_hit_buffer->Map(0, nullptr, (void**)&mapped_ptr);

        std::vector<uint32_t> data(kResolution * kResolution);

        for (int y = 0; y < kResolution; ++y)
        {
            for (int x = 0; x < kResolution; ++x)
            {
                int wi = kResolution * (kResolution - 1 - y) + x;
                int i  = kResolution * y + x;

                if (mapped_ptr[i].inst_id != ~0u)
                {
                    data[wi] = 0xff000000 | (uint32_t(mapped_ptr[i].uv[0] * 255) << 8) |
                               (uint32_t(mapped_ptr[i].uv[1] * 255) << 16);
                } else
                {
                    data[wi] = 0xff101010;
                }
            }
        }

        stbi_write_jpg("isect_result.jpg", kResolution, kResolution, 4, data.data(), 120);

        temp_hit_buffer->Unmap(0, nullptr);
    }

    CHECK_RR_CALL(rrDestroyContext(context));
}
