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
 * @brief Update BVH2.
 **/
class UpdateBvh
{
public:
    UpdateBvh(ID3D12Device* device);

    /**
     * @brief Update BVH.
     *
     * Given an indexed triangle mesh, update BVH.
     *
     * @param command_list Command list to encode collapse operation into.
     * @param triangle_count Number of triangles in binary BVH.
     * @param scratch Intermediate storage required by the builder.
     * @param result Resulting BVH2 buffer.
     **/
    void operator()(ID3D12GraphicsCommandList* command_list,
                    D3D12_GPU_VIRTUAL_ADDRESS  vertices,
                    std::uint32_t              vertex_stride,
                    std::uint32_t              vertex_count,
                    D3D12_GPU_VIRTUAL_ADDRESS  indices,
                    std::uint32_t              triangle_count,
                    D3D12_GPU_VIRTUAL_ADDRESS  scratch,
                    D3D12_GPU_VIRTUAL_ADDRESS  result);

    /**
     * @brief Get size if bytes required for scratch space.
     *
     * @param triangle_count Number of triangles
     **/
    std::size_t GetScratchDataSize(std::uint32_t triangle_count) const;

private:
    void Init();
    void AdjustLayouts(std::uint32_t triangle_count) const;

private:
    static constexpr std::uint32_t kAlignment = 256u;

    ID3D12Device*               device_;
    ComPtr<ID3D12RootSignature> root_signature_;
    ComPtr<ID3D12PipelineState> ps_init_update_flags_;
    ComPtr<ID3D12PipelineState> ps_update_;

    mutable std::size_t current_triangle_count_ = 0u;
};
}  // namespace rt::dx