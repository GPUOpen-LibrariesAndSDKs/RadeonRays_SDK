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
#include "hlbvh_builder.h"

#include "dx/common.h"
#include "dx/shader_compiler.h"

using namespace std;

namespace rt::dx
{
namespace
{
enum RootSignature
{
    kConstants,
    kVertices,
    kIndices,
    kAabb,
    kMortonCodes,
    kPrimitiveRefs,
    kBvh,
    kEntryCount
};

constexpr std::uint32_t kTrianglesPerThread = 8u;
constexpr std::uint32_t kGroupSize          = 128u;
constexpr std::uint32_t kTrianglesPerGroup  = kTrianglesPerThread * kGroupSize;

std::size_t GetBvhInternalNodeCount(std::size_t leaf_count) { return leaf_count - 1; }
std::size_t GetBvhNodeCount(std::size_t leaf_count) { return 2 * leaf_count - 1; }

// Include shader structures.
#define HOST
#include "dx/kernels/aabb.hlsl"
#include "dx/kernels/build_hlbvh_constants.hlsl"
#include "dx/kernels/bvh2il.hlsl"
#undef HOST
}  // namespace

BuildHlBvh::BuildHlBvh(ID3D12Device* device) : device_(device), radix_sort_(device) { Init(); }

std::size_t BuildHlBvh::GetResultDataSize(std::uint32_t triangle_count) const
{
    AdjustLayouts(triangle_count);
    return result_layout_.total_size();
}

std::size_t BuildHlBvh::GetScratchDataSize(std::uint32_t triangle_count) const
{
    AdjustLayouts(triangle_count);
    return scratch_layout_.total_size();
}

#define LOAD_SHADER(x)                          \
    ShaderCompiler::instance().CompileFromFile( \
        "../../src/core/src/dx/kernels/build_hlbvh_fallback.hlsl", "cs_6_0", x, {"USE_WAVE_INTRINSICS"})

#define CREATE_PIPELINE_STATE(x, s)                                                   \
    {                                                                                 \
        D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};                                  \
        desc.pRootSignature                    = root_signature_.Get();               \
        desc.CS                                = LOAD_SHADER(x);                      \
        desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;      \
        ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&(s))), \
                      "Cannot create compute pipeline");                              \
    }

void BuildHlBvh::Init()
{
    // Create root signature.
    CD3DX12_ROOT_PARAMETER root_entries[RootSignature::kEntryCount] = {};
    root_entries[kConstants].InitAsConstants(sizeof(Constants) >> 2, 0);
    root_entries[kVertices].InitAsUnorderedAccessView(0);
    root_entries[kIndices].InitAsUnorderedAccessView(1);
    root_entries[kAabb].InitAsUnorderedAccessView(2);
    root_entries[kMortonCodes].InitAsUnorderedAccessView(3);
    root_entries[kPrimitiveRefs].InitAsUnorderedAccessView(4);
    root_entries[kBvh].InitAsUnorderedAccessView(5);

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

    CREATE_PIPELINE_STATE("Init", ps_init_);
    CREATE_PIPELINE_STATE("CalculateAabb", ps_calculate_aabb_);
    CREATE_PIPELINE_STATE("CalculateMortonCodes", ps_calculate_morton_codes_);
    CREATE_PIPELINE_STATE("EmitTopology", ps_emit_topology_);
    CREATE_PIPELINE_STATE("Refit", ps_refit_);
}

void BuildHlBvh::AdjustLayouts(std::uint32_t triangle_count) const
{
    if (triangle_count == current_triangle_count_)
    {
        return;
    }

    current_triangle_count_ = triangle_count;
    result_layout_.Reset();
    scratch_layout_.Reset();
    // Result buffer
    result_layout_.AppendBlock<BvhNode>(ResultLayout::kBvh, GetBvhNodeCount(current_triangle_count_));
    // Scratch buffer.
    scratch_layout_.AppendBlock<Aabb>(ScratchLayout::kAabb, 1);
    scratch_layout_.AppendBlock<uint>(ScratchLayout::kMortonCodes, current_triangle_count_);
    scratch_layout_.AppendBlock<uint>(ScratchLayout::kPrimitiveRefs, current_triangle_count_);
    scratch_layout_.AppendBlock<uint>(ScratchLayout::kSortedMortonCodes, current_triangle_count_);
    scratch_layout_.AppendBlock<uint>(ScratchLayout::kSortedPrimitiveRefs, current_triangle_count_);

    auto sort_memory_size = radix_sort_.GetScratchDataSize(uint(current_triangle_count_));
    scratch_layout_.AppendBlock<char>(ScratchLayout::kSortMemory, sort_memory_size);
}

void BuildHlBvh::SetRootSignatureAndArguments(ID3D12GraphicsCommandList* command_list,
                                              D3D12_GPU_VIRTUAL_ADDRESS  vertices,
                                              std::uint32_t              vertex_stride,
                                              D3D12_GPU_VIRTUAL_ADDRESS  indices,
                                              std::uint32_t              triangle_count,
                                              bool                       set_sorted_codes)
{
    Constants constants{vertex_stride, triangle_count, 0, 0};

    // Set root signature (common for all the passes).
    command_list->SetComputeRootSignature(root_signature_.Get());
    command_list->SetComputeRoot32BitConstants(RootSignature::kConstants, sizeof(Constants) >> 2, &constants, 0);
    // User data inputs.
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kVertices, vertices);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kIndices, indices);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kAabb,
                                                    scratch_layout_.offset_of(ScratchLayout::kAabb));

    if (set_sorted_codes)
    {
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kMortonCodes,
                                                        scratch_layout_.offset_of(ScratchLayout::kSortedMortonCodes));
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kPrimitiveRefs,
                                                        scratch_layout_.offset_of(ScratchLayout::kSortedPrimitiveRefs));
    } else
    {
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kMortonCodes,
                                                        scratch_layout_.offset_of(ScratchLayout::kMortonCodes));
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kPrimitiveRefs,
                                                        scratch_layout_.offset_of(ScratchLayout::kPrimitiveRefs));
    }

    command_list->SetComputeRootUnorderedAccessView(RootSignature::kBvh, result_layout_.offset_of(ResultLayout::kBvh));
}

void BuildHlBvh::operator()(ID3D12GraphicsCommandList* command_list,
                            D3D12_GPU_VIRTUAL_ADDRESS  vertices,
                            std::uint32_t              vertex_stride,
                            std::uint32_t,
                            D3D12_GPU_VIRTUAL_ADDRESS indices,
                            std::uint32_t             triangle_count,
                            D3D12_GPU_VIRTUAL_ADDRESS scratch,
                            D3D12_GPU_VIRTUAL_ADDRESS result)
{
    Constants constants{vertex_stride, triangle_count, 0, 0};

    // Adjust layouts for a specified triangle count.
    // This ensures layots fit for the specified size.
    AdjustLayouts(triangle_count);

    // Set base pointers.
    result_layout_.SetBaseOffset(result);
    scratch_layout_.SetBaseOffset(scratch);

    // Set root signature (common for all the passes).
    SetRootSignatureAndArguments(command_list, vertices, vertex_stride, indices, triangle_count, false);

    // Launch init kernel.
    {
        command_list->SetPipelineState(ps_init_.Get());
        command_list->Dispatch(1u, 1u, 1u);
    }
    auto uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    // Make sure counter is visible.
    command_list->ResourceBarrier(1u, &uav_barrier);

    // Calculate AABB.
    {
        command_list->SetPipelineState(ps_calculate_aabb_.Get());
        command_list->Dispatch(CeilDivide(triangle_count, kTrianglesPerGroup), 1u, 1u);
    }

    command_list->ResourceBarrier(1u, &uav_barrier);

    // Calculate Morton codes.
    {
        command_list->SetPipelineState(ps_calculate_morton_codes_.Get());
        command_list->Dispatch(CeilDivide(triangle_count, kTrianglesPerGroup), 1u, 1u);
    }

    //// Make sure blocks are written.
    command_list->ResourceBarrier(1u, &uav_barrier);

    // Sort Morton codes and primitive refs.
    radix_sort_(command_list,
                scratch_layout_.offset_of(ScratchLayout::kMortonCodes),
                scratch_layout_.offset_of(ScratchLayout::kSortedMortonCodes),
                scratch_layout_.offset_of(ScratchLayout::kPrimitiveRefs),
                scratch_layout_.offset_of(ScratchLayout::kSortedPrimitiveRefs),
                scratch_layout_.offset_of(ScratchLayout::kSortMemory),
                triangle_count);

    //// Make sure blocks are written.
    command_list->ResourceBarrier(1u, &uav_barrier);

    // Restore root signature after sorting.
    SetRootSignatureAndArguments(command_list, vertices, vertex_stride, indices, triangle_count, true);

    // Emit topology.
    {
        command_list->SetPipelineState(ps_emit_topology_.Get());
        command_list->Dispatch(CeilDivide(triangle_count, kTrianglesPerGroup), 1u, 1u);
    }

    //// Make sure blocks are written.
    command_list->ResourceBarrier(1u, &uav_barrier);

    // Refit.
    {
        command_list->SetPipelineState(ps_refit_.Get());
        command_list->Dispatch(CeilDivide(triangle_count, kTrianglesPerGroup), 1u, 1u);
    }

    //// Make sure blocks are written.
    command_list->ResourceBarrier(1u, &uav_barrier);
}
}  // namespace rt::dx
