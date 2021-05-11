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
#include "scene_trace.h"

#include "dx/shader_compiler.h"

using namespace std;

namespace rt::dx
{
namespace
{
enum RootSignature
{
    kConstants,
    kBvh,
    kTransforms,
    kRayCount,
    kRays,
    kHits,
    kScratch,
    kBottomBvhs,
    kEntryCount
};

constexpr uint32_t kGroupSize = 128u;
constexpr uint32_t kStackSize = 64u;

// Include shader structures.
#define HOST
#include "dx/kernels/intersector_constants.hlsl"
#undef HOST
}  // namespace

TraceScene::TraceScene(ID3D12Device* device) : device_(device) { Init(); }

#define LOAD_SHADER(x, ...)                     \
    ShaderCompiler::instance().CompileFromFile( \
        "../../src/core/src/dx/kernels/top_level_intersector.hlsl", "cs_6_0", x, {__VA_ARGS__})

#define CREATE_PIPELINE_STATE(x, s, ...)                                              \
    {                                                                                 \
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};                                  \
        desc.pRootSignature                    = root_signature_.Get();               \
        desc.CS                                = LOAD_SHADER(x, __VA_ARGS__);         \
        desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;      \
        ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&(s))), \
                      "Cannot create compute pipeline");                              \
    }

void TraceScene::Init()
{
    CD3DX12_DESCRIPTOR_RANGE ranges[1];
    ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, kMaxInstances, 0);
    CD3DX12_ROOT_PARAMETER root_entries[kEntryCount] = {};
    root_entries[kConstants].InitAsConstants(sizeof(Constants) >> 2, 0);
    root_entries[kBvh].InitAsUnorderedAccessView(0);
    root_entries[kTransforms].InitAsUnorderedAccessView(1);
    root_entries[kRayCount].InitAsUnorderedAccessView(2);
    root_entries[kRays].InitAsUnorderedAccessView(3);
    root_entries[kHits].InitAsUnorderedAccessView(4);
    root_entries[kScratch].InitAsUnorderedAccessView(5);
    root_entries[kBottomBvhs].InitAsDescriptorTable(1, &ranges[0]);

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

    ThrowIfFailed(device_->CreateRootSignature(0,
                                               root_signature_blob->GetBufferPointer(),
                                               root_signature_blob->GetBufferSize(),
                                               IID_PPV_ARGS(&root_signature_)),
                  "Failed to create root signature");
    ComPtr<ID3D12PipelineState> ps_trace_full_closest;
    ComPtr<ID3D12PipelineState> ps_trace_full_any;
    ComPtr<ID3D12PipelineState> ps_trace_instance_closest;
    ComPtr<ID3D12PipelineState> ps_trace_instance_any;
    ComPtr<ID3D12PipelineState> ps_trace_full_closest_i;
    ComPtr<ID3D12PipelineState> ps_trace_full_any_i;
    ComPtr<ID3D12PipelineState> ps_trace_instance_closest_i;
    ComPtr<ID3D12PipelineState> ps_trace_instance_any_i;

    CREATE_PIPELINE_STATE("TraceRays", ps_trace_full_closest, "FULL_HIT", "CLOSEST_HIT");
    CREATE_PIPELINE_STATE("TraceRays", ps_trace_full_any, "FULL_HIT", "ANY_HIT");
    CREATE_PIPELINE_STATE("TraceRays", ps_trace_instance_closest, "INSTANCE_ONLY", "CLOSEST_HIT");
    CREATE_PIPELINE_STATE("TraceRays", ps_trace_instance_any, "INSTANCE_ONLY", "ANY_HIT");
    CREATE_PIPELINE_STATE("TraceRays", ps_trace_full_closest_i, "FULL_HIT", "CLOSEST_HIT", "INDIRECT_TRACE");
    CREATE_PIPELINE_STATE("TraceRays", ps_trace_full_any_i, "FULL_HIT", "ANY_HIT", "INDIRECT_TRACE");
    CREATE_PIPELINE_STATE("TraceRays", ps_trace_instance_closest_i, "INSTANCE_ONLY", "CLOSEST_HIT", "INDIRECT_TRACE");
    CREATE_PIPELINE_STATE("TraceRays", ps_trace_instance_any_i, "INSTANCE_ONLY", "ANY_HIT", "INDIRECT_TRACE");

    ps_trace_map_[{RR_INTERSECT_QUERY_CLOSEST, RR_INTERSECT_QUERY_OUTPUT_FULL_HIT, false}] = ps_trace_full_closest;
    ps_trace_map_[{RR_INTERSECT_QUERY_ANY, RR_INTERSECT_QUERY_OUTPUT_FULL_HIT, false}]     = ps_trace_full_any;
    ps_trace_map_[{RR_INTERSECT_QUERY_CLOSEST, RR_INTERSECT_QUERY_OUTPUT_INSTANCE_ID, false}] =
        ps_trace_instance_closest;
    ps_trace_map_[{RR_INTERSECT_QUERY_ANY, RR_INTERSECT_QUERY_OUTPUT_INSTANCE_ID, false}] = ps_trace_instance_any;

    ps_trace_map_[{RR_INTERSECT_QUERY_CLOSEST, RR_INTERSECT_QUERY_OUTPUT_FULL_HIT, true}] = ps_trace_full_closest_i;
    ps_trace_map_[{RR_INTERSECT_QUERY_ANY, RR_INTERSECT_QUERY_OUTPUT_FULL_HIT, true}]     = ps_trace_full_any_i;
    ps_trace_map_[{RR_INTERSECT_QUERY_CLOSEST, RR_INTERSECT_QUERY_OUTPUT_INSTANCE_ID, true}] =
        ps_trace_instance_closest_i;
    ps_trace_map_[{RR_INTERSECT_QUERY_ANY, RR_INTERSECT_QUERY_OUTPUT_INSTANCE_ID, true}] = ps_trace_instance_any_i;
}

void TraceScene::operator()(ID3D12GraphicsCommandList*    command_list,
                            RRIntersectQuery              query,
                            RRIntersectQueryOutput        query_output,
                            D3D12_GPU_VIRTUAL_ADDRESS     bvh,
                            D3D12_GPU_VIRTUAL_ADDRESS     transforms,
                            CD3DX12_GPU_DESCRIPTOR_HANDLE bottom_bvhs_handle,
                            ComPtr<ID3D12DescriptorHeap>  descriptor_heap,
                            uint32_t                      ray_count,
                            D3D12_GPU_VIRTUAL_ADDRESS     ray_count_buffer,
                            D3D12_GPU_VIRTUAL_ADDRESS     rays,
                            D3D12_GPU_VIRTUAL_ADDRESS     hits,
                            D3D12_GPU_VIRTUAL_ADDRESS     scratch)
{
    Constants constants{ray_count, 0, 0, 0};

    command_list->SetComputeRootSignature(root_signature_.Get());
    command_list->SetComputeRoot32BitConstants(kConstants, sizeof(Constants) >> 2, &constants, 0);
    command_list->SetComputeRootUnorderedAccessView(kBvh, bvh);
    command_list->SetComputeRootUnorderedAccessView(kTransforms, transforms);
    command_list->SetComputeRootUnorderedAccessView(kRayCount, ray_count_buffer);
    command_list->SetComputeRootUnorderedAccessView(kRays, rays);
    command_list->SetComputeRootUnorderedAccessView(kHits, hits);
    command_list->SetComputeRootUnorderedAccessView(kScratch, scratch);
    // bottom bvhs
    command_list->SetDescriptorHeaps(1, descriptor_heap.GetAddressOf());
    command_list->SetComputeRootDescriptorTable(RootSignature::kBottomBvhs, bottom_bvhs_handle);
    auto ps_trace = ps_trace_map_.at({query, query_output, bool(ray_count_buffer)});
    command_list->SetPipelineState(ps_trace.Get());
    command_list->Dispatch(CeilDivide(ray_count, kGroupSize), 1u, 1u);

    //// Make sure blocks are written.
    auto uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    command_list->ResourceBarrier(1u, &uav_barrier);
}

size_t TraceScene::GetScratchSize(uint32_t ray_count) const { return sizeof(uint32_t) * kStackSize * ray_count; }

}  // namespace rt::dx
