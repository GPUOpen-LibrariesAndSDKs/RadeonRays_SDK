#pragma once

#include <chrono>
#include <iostream>
#include <stack>

#include "gtest/gtest.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "dx/dxassist.h"
#include "dx/shader_compiler.h"
#include "mesh_data.h"
#include "radeonrays_dx.h"
#include "stb_image_write.h"

#define CHECK_RR_CALL(x) ASSERT_EQ((x), RR_SUCCESS)

using namespace std::chrono;
using namespace rt::dx;

class BasicTest : public ::testing::Test
{
public:
    void SetUp() override {}

    void TearDown() override {}

    DxAssist dxassist_;
};

TEST_F(BasicTest, BuildObjDxCompute)
{
    auto dev = dxassist_.device();

    ComPtr<ID3D12QueryHeap> query_heap;
    D3D12_QUERY_HEAP_DESC   query_heap_desc = {};
    query_heap_desc.Count                   = 2;
    query_heap_desc.Type                    = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    dev->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(&query_heap));

    RRContext context = nullptr;
    CHECK_RR_CALL(rrCreateContextDX(RR_API_VERSION, dxassist_.device(), dxassist_.command_queue(), &context));

    auto     device = dxassist_.device();
    MeshData mesh_data("../../resources/sponza.obj");

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
        std::max(geometry_reqs.temporary_build_buffer_size, geometry_reqs.temporary_update_buffer_size));
    std::cout << "Scratch buffer size: "
              << std::max(geometry_reqs.temporary_build_buffer_size, geometry_reqs.temporary_update_buffer_size) / 1000000
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

    auto command_allocator = dxassist_.CreateCommandAllocator();
    auto command_list      = dxassist_.CreateCommandList(command_allocator.Get());

    auto shader =
        ShaderCompiler::instance().CompileFromFile("../../test/test_dx/raytrace_bvh2.hlsl", "cs_6_2", "TraceRays");

    enum RootSignature
    {
        kConstants,
        kVertices,
        kIndices,
        kBvh,
        kOutput,
        kEntryCount
    };

    // Create root signature.
    struct Constants
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t triangle_count;
        std::uint32_t padding;
    };

    CD3DX12_ROOT_PARAMETER root_entries[RootSignature::kEntryCount] = {};
    root_entries[kConstants].InitAsConstants(sizeof(Constants) >> 2, 0);
    root_entries[kVertices].InitAsUnorderedAccessView(0);
    root_entries[kIndices].InitAsUnorderedAccessView(1);
    root_entries[kBvh].InitAsUnorderedAccessView(2);
    root_entries[kOutput].InitAsUnorderedAccessView(3);

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init(RootSignature::kEntryCount, root_entries);

    ComPtr<ID3DBlob>   error_blob          = nullptr;
    ComPtr<ID3D10Blob> root_signature_blob = nullptr;

    if (FAILED(D3D12SerializeRootSignature(
            &root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, &error_blob)))
    {
        if (error_blob)
        {
            std::string error_str(static_cast<const char*>(error_blob->GetBufferPointer()));
            throw std::runtime_error(error_str);
        } else
        {
            throw std::runtime_error("Failed to serialize root signature");
        }
    }

    ComPtr<ID3D12RootSignature> root_signature;
    ThrowIfFailed(dxassist_.device()->CreateRootSignature(0,
                                                          root_signature_blob->GetBufferPointer(),
                                                          root_signature_blob->GetBufferSize(),
                                                          IID_PPV_ARGS(&root_signature)),
                  "Failed to create root signature");

    ComPtr<ID3D12PipelineState>       pipeline_state;
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature                    = root_signature.Get();
    desc.CS                                = shader;
    desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(dxassist_.device()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline_state)),
                  "Cannot create compute pipeline");

    Constants constants{2048, 2048, (std::uint32_t)triangle_count, 0};

    auto color = dxassist_.CreateUploadBuffer(constants.width * constants.height * sizeof(UINT));

    command_list->SetComputeRootSignature(root_signature.Get());
    command_list->SetPipelineState(pipeline_state.Get());

    command_list->SetComputeRoot32BitConstants(RootSignature::kConstants, sizeof(Constants) >> 2, &constants, 0);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kVertices, vertex_buffer->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kIndices, index_buffer->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kBvh, geometry->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kOutput, color->GetGPUVirtualAddress());

    D3D12_RESOURCE_BARRIER ts_buffer_barrier;

    ts_buffer_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    ts_buffer_barrier.Transition.pResource   = timestamp_buffer.Get();
    ts_buffer_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    ts_buffer_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    ts_buffer_barrier.Transition.Subresource = 0;
    ts_buffer_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;

    command_list->ResourceBarrier(1, &ts_buffer_barrier);
    command_list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    for (auto i = 0; i < 8; ++i) command_list->Dispatch(constants.width / 8, constants.height / 8, 1);
    command_list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
    command_list->ResolveQueryData(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, timestamp_buffer.Get(), 0);
    ts_buffer_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    ts_buffer_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
    command_list->ResourceBarrier(1, &ts_buffer_barrier);
    command_list->CopyBufferRegion(timestamp_readback_buffer.Get(), 0, timestamp_buffer.Get(), 0, sizeof(UINT64) * 8);
    command_list->Close();

    // Warm up.
    {
        ID3D12CommandList* cmd_lists[] = {command_list.Get()};
        dxassist_.command_queue()->ExecuteCommandLists(1, cmd_lists);

        auto fence = dxassist_.CreateFence();
        dxassist_.command_queue()->Signal(fence.Get(), 1000);

        while (fence->GetCompletedValue() != 1000) Sleep(0);
    }

    ID3D12CommandList* cmd_lists[] = {command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, cmd_lists);

    auto fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(fence.Get(), 1000);

    while (fence->GetCompletedValue() != 1000) Sleep(0);

    float delta_s = 0.f;
    {
        UINT64 freq = 0;
        dxassist_.command_queue()->GetTimestampFrequency(&freq);

        UINT64* mapped_ptr;
        timestamp_readback_buffer->Map(0, nullptr, (void**)&mapped_ptr);

        auto num_ticks = mapped_ptr[1] - mapped_ptr[0];
        delta_s        = float(num_ticks) / freq;

        // Bounding boxes
        timestamp_readback_buffer->Unmap(0, nullptr);
    }

    std::cout << "Ray throughput: " << ((constants.width * constants.height * 8.f) / delta_s) / 1e6f << " Mrays/s\n";
    std::cout << "Avg time: " << delta_s * 1e2f / 8.f << " ms\n";
    {
        void* mapped_ptr;
        color->Map(0, nullptr, &mapped_ptr);

        stbi_write_jpg("test_dx_compute_buildobj_result.jpg", constants.width, constants.height, 4, mapped_ptr, 120);

        // Bounding boxes
        color->Unmap(0, nullptr);
    }
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
    auto        scratch_trace_buffer = dxassist_.CreateUAVBuffer(scratch_size);
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

        stbi_write_jpg("test_dx_compute_sponza_geom_isect_result.jpg", kResolution, kResolution, 4, data.data(), 120);

        temp_hit_buffer->Unmap(0, nullptr);
    }
    CHECK_RR_CALL(rrDestroyContext(context));
}

TEST_F(BasicTest, UpdateObjDxCompute)
{
    auto dev = dxassist_.device();

    RRContext context = nullptr;
    CHECK_RR_CALL(rrCreateContextDX(RR_API_VERSION, dxassist_.device(), dxassist_.command_queue(), &context));

    auto     device = dxassist_.device();
    MeshData mesh_data("../../resources/sponza.obj");

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
    options.build_flags = RR_BUILD_FLAG_BITS_ALLOW_UPDATE | RR_BUILD_FLAG_BITS_PREFER_FAST_BUILD;

    RRMemoryRequirements geometry_reqs;
    CHECK_RR_CALL(rrGetGeometryBuildMemoryRequirements(context, &geometry_build_input, &options, &geometry_reqs));

    D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    auto geometry = dxassist_.CreateUAVBuffer(geometry_reqs.result_buffer_size);
    std::cout << "Geometry buffer size: " << geometry_reqs.result_buffer_size / 1000000 << "Mb\n";

    RRDevicePtr geometry_ptr;
    CHECK_RR_CALL(rrGetDevicePtrFromD3D12Resource(context, geometry.Get(), 0, &geometry_ptr));

    auto scratch_buffer = dxassist_.CreateUAVBuffer(
        std::max(geometry_reqs.temporary_build_buffer_size, geometry_reqs.temporary_update_buffer_size));
    std::cout << "Scratch buffer size: "
              << std::max(geometry_reqs.temporary_build_buffer_size, geometry_reqs.temporary_update_buffer_size) / 1000000
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

    // Update vertex data.
    float* vptr = nullptr;
    vertex_buffer->Map(0, nullptr, (void**)&vptr);

    for (uint32_t i = 0; i < mesh_data.positions.size() / 3; ++i)
    {
        vptr[3 * i + 1] -= 40.f;
    }

    vertex_buffer->Unmap(0, nullptr);
    CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseCommandStream(context, command_stream));

    CHECK_RR_CALL(rrAllocateCommandStream(context, &command_stream));

    CHECK_RR_CALL(rrCmdBuildGeometry(context,
                                     RR_BUILD_OPERATION_UPDATE,
                                     &geometry_build_input,
                                     &options,
                                     scratch_ptr,
                                     geometry_ptr,
                                     command_stream));
    CHECK_RR_CALL(rrSumbitCommandStream(context, command_stream, nullptr, &wait_event));
    CHECK_RR_CALL(rrWaitEvent(context, wait_event));

    auto command_allocator = dxassist_.CreateCommandAllocator();
    auto command_list      = dxassist_.CreateCommandList(command_allocator.Get());

    auto shader =
        ShaderCompiler::instance().CompileFromFile("../../test/test_dx/raytrace_bvh2.hlsl", "cs_6_2", "TraceRays");

    enum RootSignature
    {
        kConstants,
        kVertices,
        kIndices,
        kBvh,
        kOutput,
        kEntryCount
    };

    // Create root signature.
    struct Constants
    {
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t triangle_count;
        std::uint32_t padding;
    };

    CD3DX12_ROOT_PARAMETER root_entries[RootSignature::kEntryCount] = {};
    root_entries[kConstants].InitAsConstants(sizeof(Constants) >> 2, 0);
    root_entries[kVertices].InitAsUnorderedAccessView(0);
    root_entries[kIndices].InitAsUnorderedAccessView(1);
    root_entries[kBvh].InitAsUnorderedAccessView(2);
    root_entries[kOutput].InitAsUnorderedAccessView(3);

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init(RootSignature::kEntryCount, root_entries);

    ComPtr<ID3DBlob>   error_blob          = nullptr;
    ComPtr<ID3D10Blob> root_signature_blob = nullptr;

    if (FAILED(D3D12SerializeRootSignature(
            &root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, &error_blob)))
    {
        if (error_blob)
        {
            std::string error_str(static_cast<const char*>(error_blob->GetBufferPointer()));
            throw std::runtime_error(error_str);
        } else
        {
            throw std::runtime_error("Failed to serialize root signature");
        }
    }

    ComPtr<ID3D12RootSignature> root_signature;
    ThrowIfFailed(dxassist_.device()->CreateRootSignature(0,
                                                          root_signature_blob->GetBufferPointer(),
                                                          root_signature_blob->GetBufferSize(),
                                                          IID_PPV_ARGS(&root_signature)),
                  "Failed to create root signature");

    ComPtr<ID3D12PipelineState>       pipeline_state;
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature                    = root_signature.Get();
    desc.CS                                = shader;
    desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(dxassist_.device()->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline_state)),
                  "Cannot create compute pipeline");

    Constants constants{2048, 2048, (std::uint32_t)triangle_count, 0};

    auto color = dxassist_.CreateUploadBuffer(constants.width * constants.height * sizeof(UINT));

    command_list->SetComputeRootSignature(root_signature.Get());
    command_list->SetPipelineState(pipeline_state.Get());

    command_list->SetComputeRoot32BitConstants(RootSignature::kConstants, sizeof(Constants) >> 2, &constants, 0);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kVertices, vertex_buffer->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kIndices, index_buffer->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kBvh, geometry->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kOutput, color->GetGPUVirtualAddress());

    command_list->Dispatch(constants.width / 8, constants.height / 8, 1);
    command_list->Close();

    ID3D12CommandList* cmd_lists[] = {command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, cmd_lists);

    auto fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(fence.Get(), 1000);

    while (fence->GetCompletedValue() != 1000) Sleep(0);
    {
        void* mapped_ptr;
        color->Map(0, nullptr, &mapped_ptr);

        stbi_write_jpg("test_dx_compute_updateobj_result.jpg", constants.width, constants.height, 4, mapped_ptr, 120);

        // Bounding boxes
        color->Unmap(0, nullptr);
    }

    CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseCommandStream(context, command_stream));
    CHECK_RR_CALL(rrDestroyContext(context));
}

TEST_F(BasicTest, BuildObjTwoLevelDxCompute)
{
    RRContext context = nullptr;
    CHECK_RR_CALL(rrCreateContext(RR_API_VERSION, RR_API_DX, &context));

    // SceneData scene_data("../../resources/cornell_box.obj");
    SceneData                           scene_data("../../resources/sponza.obj");
    std::vector<ComPtr<ID3D12Resource>> geometry_resources;
    std::vector<RRDevicePtr>            geometry_ptrs;
    RRBuildOptions                      options;
    options.build_flags = RR_BUILD_FLAG_BITS_PREFER_FAST_BUILD;
    RREvent wait_event  = nullptr;
    int     i           = -1;
    for (auto& mesh_data : scene_data.meshes)
    {
        auto vertex_buffer =
            dxassist_.CreateUploadBuffer(sizeof(float) * mesh_data.positions.size(), mesh_data.positions.data());
        auto index_buffer =
            dxassist_.CreateUploadBuffer(sizeof(std::uint32_t) * mesh_data.indices.size(), mesh_data.indices.data());

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

        RRMemoryRequirements geometry_reqs;
        CHECK_RR_CALL(rrGetGeometryBuildMemoryRequirements(context, &geometry_build_input, &options, &geometry_reqs));
        auto geometry = dxassist_.CreateUAVBuffer(geometry_reqs.result_buffer_size);
        geometry_resources.push_back(geometry);
        RRDevicePtr geometry_ptr;
        CHECK_RR_CALL(rrGetDevicePtrFromD3D12Resource(context, geometry.Get(), 0, &geometry_ptr));
        geometry_ptrs.push_back(geometry_ptr);
        auto scratch_buffer = dxassist_.CreateUAVBuffer(
            std::max(geometry_reqs.temporary_build_buffer_size, geometry_reqs.temporary_update_buffer_size));
        RRDevicePtr scratch_ptr = nullptr;
        CHECK_RR_CALL(rrGetDevicePtrFromD3D12Resource(context, scratch_buffer.Get(), 0, &scratch_ptr));
        RRCommandStream command_stream = nullptr;
        CHECK_RR_CALL(rrAllocateCommandStream(context, &command_stream));

        CHECK_RR_CALL(rrCmdBuildGeometry(context,
                                         RR_BUILD_OPERATION_BUILD,
                                         &geometry_build_input,
                                         &options,
                                         scratch_ptr,
                                         geometry_ptr,
                                         command_stream));
        CHECK_RR_CALL(rrSumbitCommandStream(context, command_stream, nullptr, &wait_event));
        CHECK_RR_CALL(rrWaitEvent(context, wait_event));
        CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
        CHECK_RR_CALL(rrReleaseCommandStream(context, command_stream));
    }

    RRSceneBuildInput scene_build_input = {};
    scene_build_input.instance_count    = (uint32_t)geometry_ptrs.size();

    RRMemoryRequirements scene_reqs;
    CHECK_RR_CALL(rrGetSceneBuildMemoryRequirements(context, &scene_build_input, &options, &scene_reqs));

    D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;

    auto        scene = dxassist_.CreateUAVBuffer(scene_reqs.result_buffer_size);
    RRDevicePtr scene_ptr;
    CHECK_RR_CALL(rrGetDevicePtrFromD3D12Resource(context, scene.Get(), 0, &scene_ptr));
    auto scratch_buffer =
        dxassist_.CreateUAVBuffer(std::max(scene_reqs.temporary_build_buffer_size, scene_reqs.temporary_update_buffer_size));
    RRDevicePtr scratch_ptr = nullptr;
    CHECK_RR_CALL(rrGetDevicePtrFromD3D12Resource(context, scratch_buffer.Get(), 0, &scratch_ptr));

    std::vector<RRInstance> instances;
    for (auto& geometry_ptr : geometry_ptrs)
    {
        RRInstance instance;
        instance.geometry = geometry_ptr;
        std::memset(&instance.transform[0][0], 0, sizeof(instance.transform));
        instance.transform[0][0] = instance.transform[1][1] = instance.transform[2][2] = 1;
        instances.push_back(instance);
    }

    scene_build_input.instances         = instances.data();
    RRCommandStream command_stream_2lvl = nullptr;
    RREvent         wait_event_2lvl     = nullptr;
    CHECK_RR_CALL(rrAllocateCommandStream(context, &command_stream_2lvl));
    CHECK_RR_CALL(rrCmdBuildScene(context, &scene_build_input, &options, scratch_ptr, scene_ptr, command_stream_2lvl));

    CHECK_RR_CALL(rrSumbitCommandStream(context, command_stream_2lvl, nullptr, &wait_event_2lvl));
    CHECK_RR_CALL(rrWaitEvent(context, wait_event_2lvl));

    CHECK_RR_CALL(rrReleaseEvent(context, wait_event_2lvl));
    CHECK_RR_CALL(rrReleaseCommandStream(context, command_stream_2lvl));

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
    auto command_allocator      = dxassist_.CreateCommandAllocator();
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
    auto        scratch_trace_buffer = dxassist_.CreateUAVBuffer(scratch_size);
    RRDevicePtr scratch_trace_ptr    = nullptr;
    rrGetDevicePtrFromD3D12Resource(context, scratch_trace_buffer.Get(), 0, &scratch_trace_ptr);

    CHECK_RR_CALL(rrCmdIntersect(context,
                                 scene_ptr,
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

        stbi_write_jpg("test_dx_compute_sponza_scene_isect_result.jpg", kResolution, kResolution, 4, data.data(), 120);

        temp_hit_buffer->Unmap(0, nullptr);
    }
    CHECK_RR_CALL(rrDestroyContext(context));
}
