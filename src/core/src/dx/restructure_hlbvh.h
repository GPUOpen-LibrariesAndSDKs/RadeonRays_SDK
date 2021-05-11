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
#include "dx/common.h"
#include "utils/memory_layout.h"

using Microsoft::WRL::ComPtr;

namespace rt::dx
{
/**
 * @brief Restructure BVH2
 **/
class RestructureBvh
{
public:
    RestructureBvh(ID3D12Device* device);

    /**
     * @brief Restructure BVH.
     *
     * Given an indexed triangle mesh, update BVH.
     *
     * @param command_list Command list to encode collapse operation into.
     * @param triangle_count Number of triangles in binary BVH.
     * @param scratch Intermediate storage required by the builder.
     * @param result Resulting BVH2 buffer.
     **/
    void operator()(ID3D12GraphicsCommandList* command_list,
                    std::uint32_t              triangle_count,
                    D3D12_GPU_VIRTUAL_ADDRESS  scratch,
                    D3D12_GPU_VIRTUAL_ADDRESS  result);

    /**
     * @brief Get size if bytes required for scratch space.
     *
     * @param triangle_count Number of triangles
     **/
    size_t GetScratchDataSize(uint32_t triangle_count) const;

    /**
     * @brief Set the number of times treelet restructure is applied
     **/
    void SetNumIterations(uint32_t num_iterations) { num_iterations_ = num_iterations; }

private:
    void Init();
    void AdjustLayouts(uint32_t triangle_count) const;

private:
    static constexpr uint32_t kAlignment = 256u;

    // Result buffer layout.
    enum class ResultLayout
    {
        kBvh
    };

    // Scratch space layout.
    enum class ScratchLayout
    {
        kTreeletCount,
        kTreeletRoots,
        kPrimitiveCounts
    };

    ID3D12Device*               device_;
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> ps_init_;
    ComPtr<ID3D12PipelineState> ps_find_treelet_roots_;
    ComPtr<ID3D12PipelineState> ps_restructure_;

    using ResultLayoutT  = MemoryLayout<ResultLayout, D3D12_GPU_VIRTUAL_ADDRESS>;
    using ScratchLayoutT = MemoryLayout<ScratchLayout, D3D12_GPU_VIRTUAL_ADDRESS>;

    mutable size_t         current_triangle_count_ = 0u;
    mutable ResultLayoutT  result_layout_          = ResultLayoutT(kAlignment);
    mutable ScratchLayoutT scratch_layout_         = ScratchLayoutT(kAlignment);

    uint32_t num_iterations_ = 3;
};
}  // namespace rt::dx