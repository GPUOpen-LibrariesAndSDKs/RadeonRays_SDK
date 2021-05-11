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
#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "dx/common.h"

using Microsoft::WRL::ComPtr;

namespace rt::dx
{
/**
 * @brief Trace BVH2 scene
 **/
class TraceScene
{
public:
    TraceScene(ID3D12Device* device);

    void operator()(ID3D12GraphicsCommandList*    command_list,
                    RRIntersectQuery              query,
                    RRIntersectQueryOutput        query_output,
                    D3D12_GPU_VIRTUAL_ADDRESS     bvh,
                    D3D12_GPU_VIRTUAL_ADDRESS     transforms,
                    CD3DX12_GPU_DESCRIPTOR_HANDLE bottom_bvhs_handle,
                    ComPtr<ID3D12DescriptorHeap>  descriptor_heap,
                    std::uint32_t                 ray_count,
                    D3D12_GPU_VIRTUAL_ADDRESS     ray_count_buffer,
                    D3D12_GPU_VIRTUAL_ADDRESS     rays,
                    D3D12_GPU_VIRTUAL_ADDRESS     hits,
                    D3D12_GPU_VIRTUAL_ADDRESS     scratch);

    size_t GetScratchSize(uint32_t ray_count) const;
private:
    void Init();

private:
    ID3D12Device*               device_;
    ComPtr<ID3D12RootSignature> root_signature_;
    std::unordered_map<TraceKey, ComPtr<ID3D12PipelineState>, KeyHasher> ps_trace_map_;
};
}  // namespace rt::dx