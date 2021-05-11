/**********************************************************************
Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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
#pragma once

#include <cstddef>
#include <cstdint>

#include "base/command_stream_base.h"
#include "dx/algorithm/radix_sort.h"
#include "dx/common.h"
#include "utils/memory_layout.h"

using Microsoft::WRL::ComPtr;

namespace rt::dx
{
/**
 * @brief HLBVH builder.
 *
 * Build linear BVH.
 **/
class BuildHlBvhTopLevel
{
public:
    static constexpr std::uint32_t kAlignment = 256u;

    BuildHlBvhTopLevel(ID3D12Device* device);
    /**
     * @brief Build BVH.
     *
     * Given an a set of instance descs, build a linear BVH.
     *
     * @param scratch Intermediate storage required by the builder.
     * @param result Resulting BVH2 buffer.
     **/
    void operator()(ID3D12GraphicsCommandList*    command_list,
                    D3D12_GPU_VIRTUAL_ADDRESS     instance_descs,
                    CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_instance_handle,
                    uint32_t                      instance_count,
                    ComPtr<ID3D12DescriptorHeap>  descriptor_heap,
                    D3D12_GPU_VIRTUAL_ADDRESS     scratch,
                    D3D12_GPU_VIRTUAL_ADDRESS     result);

    /**
     * @brief Get size if bytes required to hold resulting BVH.
     *
     * @param instance_count Number of instances
     **/
    std::size_t GetResultDataSize(std::uint32_t instance_count) const;

    /**
     * @brief Get size if bytes required for scratch space.
     *
     * @param instance_count Number of instances
     **/
    std::size_t GetScratchDataSize(std::uint32_t instance_count) const;
private:
    void Init();
    void AdjustLayouts(std::uint32_t instance_count) const;
    void SetRootSignatureAndArguments(ID3D12GraphicsCommandList*    command_list,
                                      D3D12_GPU_VIRTUAL_ADDRESS     instance_descs,
                                      CD3DX12_GPU_DESCRIPTOR_HANDLE gpu_instance_handle,
                                      std::uint32_t                 instance_count,
                                      ComPtr<ID3D12DescriptorHeap>  descriptor_heap,
                                      bool                          set_sorted_codes);

private:
    // Result buffer layout.
    enum class ResultLayout
    {
        kBvh,
        kTransforms
    };

    // Scratch space layout.
    enum class ScratchLayout
    {
        kAabb,
        kMortonCodes,
        kPrimitiveRefs,
        kSortedMortonCodes,
        kSortedPrimitiveRefs,
        kSortMemory
    };

    ID3D12Device*               device_;
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> ps_init_;
    ComPtr<ID3D12PipelineState> ps_calculate_aabb_;
    ComPtr<ID3D12PipelineState> ps_calculate_morton_codes_;
    ComPtr<ID3D12PipelineState> ps_emit_topology_;
    ComPtr<ID3D12PipelineState> ps_refit_;

    algorithm::RadixSortKeyValue radix_sort_;

    using ResultLayoutT  = MemoryLayout<ResultLayout, D3D12_GPU_VIRTUAL_ADDRESS>;
    using ScratchLayoutT = MemoryLayout<ScratchLayout, D3D12_GPU_VIRTUAL_ADDRESS>;

    mutable std::size_t    current_instance_count_ = 0u;
    mutable ResultLayoutT  result_layout_          = ResultLayoutT(kAlignment);
    mutable ScratchLayoutT scratch_layout_         = ScratchLayoutT(kAlignment);
};
}  // namespace rt::dx