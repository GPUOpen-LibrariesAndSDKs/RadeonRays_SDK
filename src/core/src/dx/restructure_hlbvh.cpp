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

#include <iostream>
#include <vector>

#include "dx/common.h"
#include "dx/shader_compiler.h"

using namespace std;

namespace rt::dx
{
namespace
{
namespace RootSignature
{
enum
{
    kConstants,
    kTreeletCount,
    kTreeletRoots,
    kPrimitiveCounts,
    kBvh,
    kEntryCount
};
}

constexpr uint32_t kGroupSize               = 64u;
constexpr uint32_t kMinPrimitivesPerTreelet = 64u;

size_t GetBvhInternalNodeCount(size_t leaf_count) { return leaf_count - 1; }
size_t GetBvhNodeCount(size_t leaf_count) { return 2 * leaf_count - 1; }

// Include shader structures.
#define HOST
#include "dx/kernels/bvh2il.hlsl"
#include "dx/kernels/restructure_bvh_constants.hlsl"
#undef HOST
}  // namespace

RestructureBvh::RestructureBvh(ID3D12Device* device) : device_(device) { Init(); }

size_t RestructureBvh::GetScratchDataSize(uint32_t triangle_count) const
{
    AdjustLayouts(triangle_count);
    return scratch_layout_.total_size();
}

#define LOAD_SHADER(x)                          \
    ShaderCompiler::instance().CompileFromFile( \
        "../../src/core/src/dx/kernels/restructure_bvh.hlsl", "cs_6_0", x, {"USE_WAVE_INTRINSICS"})

#define CREATE_PIPELINE_STATE(x, s)                                                   \
    {                                                                                 \
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};                                  \
        desc.pRootSignature                    = root_signature_.Get();               \
        desc.CS                                = LOAD_SHADER(x);                      \
        desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;      \
        ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&(s))), \
                      "Cannot create compute pipeline");                              \
    }

void RestructureBvh::Init()
{
    // Create root signature.
    CD3DX12_ROOT_PARAMETER root_entries[RootSignature::kEntryCount] = {};
    root_entries[RootSignature::kConstants].InitAsConstants(sizeof(Constants) >> 2, 0);
    root_entries[RootSignature::kTreeletCount].InitAsUnorderedAccessView(0);
    root_entries[RootSignature::kTreeletRoots].InitAsUnorderedAccessView(1);
    root_entries[RootSignature::kPrimitiveCounts].InitAsUnorderedAccessView(2);
    root_entries[RootSignature::kBvh].InitAsUnorderedAccessView(3);

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

    CREATE_PIPELINE_STATE("InitPrimitiveCounts", ps_init_);
    CREATE_PIPELINE_STATE("FindTreeletRoots", ps_find_treelet_roots_);
    CREATE_PIPELINE_STATE("Restructure", ps_restructure_);
}

void RestructureBvh::AdjustLayouts(uint32_t triangle_count) const
{
    if (triangle_count == current_triangle_count_)
        return;

    current_triangle_count_ = triangle_count;
    result_layout_.Reset();
    scratch_layout_.Reset();
    // Result buffer
    result_layout_.AppendBlock<BvhNode>(ResultLayout::kBvh, GetBvhNodeCount(current_triangle_count_));

    // Scratch buffer.
    scratch_layout_.AppendBlock<uint>(ScratchLayout::kTreeletCount, 1);
    // TODO: this is overallocation
    scratch_layout_.AppendBlock<uint>(ScratchLayout::kTreeletRoots, current_triangle_count_);
    scratch_layout_.AppendBlock<uint>(ScratchLayout::kPrimitiveCounts, GetBvhNodeCount(current_triangle_count_));
}

void RestructureBvh::operator()(ID3D12GraphicsCommandList* command_list,
                                uint32_t              triangle_count,
                                D3D12_GPU_VIRTUAL_ADDRESS  scratch,
                                D3D12_GPU_VIRTUAL_ADDRESS  result)
{
    // Adjust layouts for a specified triangle count.
    // This ensures layots fit for the specified size.
    AdjustLayouts(triangle_count);

    // Set base pointers.
    result_layout_.SetBaseOffset(result);
    scratch_layout_.SetBaseOffset(scratch);
    uint32_t min_primitive_per_treelet = kMinPrimitivesPerTreelet;

    for (auto i = 0u; i < num_iterations_; ++i)
    {
        Constants constants{min_primitive_per_treelet, triangle_count, 0, 0};

        // Set root signature (common for all the passes).
        command_list->SetComputeRootSignature(root_signature_.Get());
        command_list->SetComputeRoot32BitConstants(RootSignature::kConstants, sizeof(Constants) >> 2, &constants, 0);
        // User data inputs.
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kTreeletCount,
                                                        scratch_layout_.offset_of(ScratchLayout::kTreeletCount));
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kTreeletRoots,
                                                        scratch_layout_.offset_of(ScratchLayout::kTreeletRoots));
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kPrimitiveCounts,
                                                        scratch_layout_.offset_of(ScratchLayout::kPrimitiveCounts));
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kBvh,
                                                        result_layout_.offset_of(ResultLayout::kBvh));

        // Launch init kernel.
        {
            command_list->SetPipelineState(ps_init_.Get());
            command_list->Dispatch(CeilDivide(triangle_count, kGroupSize), 1u, 1u);
        }

        auto uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
        command_list->ResourceBarrier(1u, &uav_barrier);

        // Find treelet roots.
        {
            command_list->SetPipelineState(ps_find_treelet_roots_.Get());
            command_list->Dispatch(CeilDivide(triangle_count, kGroupSize), 1u, 1u);
        }

        command_list->ResourceBarrier(1u, &uav_barrier);

        // Restructure.
        {
            command_list->SetPipelineState(ps_restructure_.Get());
            command_list->Dispatch(CeilDivide(triangle_count, min_primitive_per_treelet), 1u, 1u);
        }

        command_list->ResourceBarrier(1u, &uav_barrier);
        min_primitive_per_treelet *= 2;
    }
}
}  // namespace rt::dx
