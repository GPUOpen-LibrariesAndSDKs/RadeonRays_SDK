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
    kVertices,
    kIndices,
    kBvh,
    kUpdateFlags,
    kEntryCount
};
}

constexpr std::uint32_t kGroupSize = 256u;

std::size_t GetBvhInternalNodeCount(size_t leaf_count) { return leaf_count - 1; }
std::size_t GetBvhNodeCount(size_t leaf_count) { return 2 * leaf_count - 1; }
// Include shader structures.
#define HOST
#include "dx/kernels/build_hlbvh_constants.hlsl"
#include "dx/kernels/bvh2il.hlsl"
#undef HOST
}  // namespace

UpdateBvh::UpdateBvh(ID3D12Device* device) : device_(device) { Init(); }

std::size_t UpdateBvh::GetScratchDataSize(std::uint32_t triangle_count) const
{
    AdjustLayouts(triangle_count);
    return sizeof(std::uint32_t) * triangle_count;
}

#define LOAD_SHADER(x)                          \
    ShaderCompiler::instance().CompileFromFile( \
        "../../src/core/src/dx/kernels/build_hlbvh_fallback.hlsl", "cs_6_0", x, {"UPDATE_KERNEL"})

#define CREATE_PIPELINE_STATE(x, s)                                                   \
    {                                                                                 \
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};                                  \
        desc.pRootSignature                    = root_signature_.Get();               \
        desc.CS                                = LOAD_SHADER(x);                      \
        desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;      \
        ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&(s))), \
                      "Cannot create compute pipeline");                              \
    }

void UpdateBvh::Init()
{
    // Create root signature.
    CD3DX12_ROOT_PARAMETER root_entries[RootSignature::kEntryCount] = {};
    root_entries[RootSignature::kConstants].InitAsConstants(sizeof(Constants) >> 2, 0);
    root_entries[RootSignature::kVertices].InitAsUnorderedAccessView(0);
    root_entries[RootSignature::kIndices].InitAsUnorderedAccessView(1);
    root_entries[RootSignature::kBvh].InitAsUnorderedAccessView(5);
    root_entries[RootSignature::kUpdateFlags].InitAsUnorderedAccessView(6);

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

    CREATE_PIPELINE_STATE("InitUpdateFlags", ps_init_update_flags_);
    CREATE_PIPELINE_STATE("Refit", ps_update_);
}

void UpdateBvh::AdjustLayouts(std::uint32_t triangle_count) const
{
    if (triangle_count == current_triangle_count_)
    {
        return;
    }

    current_triangle_count_ = triangle_count;
}

void UpdateBvh::operator()(ID3D12GraphicsCommandList* command_list,
                           D3D12_GPU_VIRTUAL_ADDRESS  vertices,
                           std::uint32_t              vertex_stride,
                           std::uint32_t,
                           D3D12_GPU_VIRTUAL_ADDRESS indices,
                           std::uint32_t             triangle_count,
                           D3D12_GPU_VIRTUAL_ADDRESS scratch,
                           D3D12_GPU_VIRTUAL_ADDRESS result)
{
    // Adjust layouts for a specified triangle count.
    // This ensures layots fit for the specified size.
    AdjustLayouts(triangle_count);

    Constants constants{vertex_stride, triangle_count, 0, 0};

    // Set root signature (common for all the passes).
    command_list->SetComputeRootSignature(root_signature_.Get());
    command_list->SetComputeRoot32BitConstants(RootSignature::kConstants, sizeof(Constants) >> 2, &constants, 0);
    // User data inputs.
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kVertices, vertices);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kIndices, indices);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kBvh, result);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kUpdateFlags, scratch);

    // Launch init kernel.
    {
        command_list->SetPipelineState(ps_init_update_flags_.Get());
        command_list->Dispatch(CeilDivide(triangle_count, kGroupSize), 1u, 1u);
    }

    auto uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    command_list->ResourceBarrier(1u, &uav_barrier);

    // Update BVH nodes.
    {
        command_list->SetPipelineState(ps_update_.Get());
        command_list->Dispatch(CeilDivide(triangle_count, kGroupSize), 1u, 1u);
    }
}
}  // namespace rt::dx
