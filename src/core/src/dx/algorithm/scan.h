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

#include "dx/common.h"


using Microsoft::WRL::ComPtr;

namespace rt::dx::algorithm
{
/**
 * @brief Parallel prefix sum algorithm.
 **/
class Scan
{
public:
    Scan(ID3D12Device* device) : device_(device) { Init(); }

    /**
     * @brief Record prefix sum into a command list.
     *
     * @param command_list Command list to record to.
     * @param input_keys Input data pointer.
     * @param output_keys Output data pointer.
     * @param scratch_data Scratch area pointer.
     * @param size Number of elements to scan.
     **/
    void operator()(ID3D12GraphicsCommandList* command_list,
                    D3D12_GPU_VIRTUAL_ADDRESS  input_keys,
                    D3D12_GPU_VIRTUAL_ADDRESS  output_keys,
                    D3D12_GPU_VIRTUAL_ADDRESS  scratch_data,
                    std::uint32_t              size);
    /**
     * @brief Get the amount of scratch memory in bytes required to scan specified amount of elements.
     *
     * @param size Number of elements to scan.
     **/
    std::size_t GetScratchDataSize(std::uint32_t size) const;

private:
    void Init();
    void SetState(ID3D12GraphicsCommandList* command_list,
                  ID3D12PipelineState*       state,
                  UINT                       size,
                  D3D12_GPU_VIRTUAL_ADDRESS  input_keys,
                  D3D12_GPU_VIRTUAL_ADDRESS  scratch_data,
                  D3D12_GPU_VIRTUAL_ADDRESS  output_keys);

private:
    //< D3D device.
    ComPtr<ID3D12Device> device_;
    //< Root signature common for all kernels.
    ComPtr<ID3D12RootSignature> root_signature_;
    //< Simple work group scan kernel.
    ComPtr<ID3D12PipelineState> pipeline_state_scan_;
    //< Scan kernel with carry-out from part sums.
    ComPtr<ID3D12PipelineState> pipeline_state_scan_carryout_;
    //< Group reduction kernel.
    ComPtr<ID3D12PipelineState> pipeline_state_reduce_;
};
}  // namespace rt::dx::algorithm