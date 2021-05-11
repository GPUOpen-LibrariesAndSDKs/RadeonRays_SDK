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

#include "dx/algorithm/scan.h"
#include "dx/common.h"


using Microsoft::WRL::ComPtr;

namespace rt::dx::algorithm
{
/**
 * @brief Radix sort algorithm.
 **/
class RadixSortKeyValue
{
public:
    RadixSortKeyValue(ID3D12Device* device) : device_(device), scan_(device) { Init(); }

    /**
     * @brief Record sort into a command list.
     *
     * @param command_list Command list to record to.
     * @param input_keys Input keys pointer.
     * @param output_keys Output keys pointer.
     * @param input_value Input values pointer.
     * @param output_values Output values pointer.
     * @param scratch_data Scratch area pointer.
     * @param size Number of elements to sort.
     **/
    void operator()(ID3D12GraphicsCommandList* command_list,
                    D3D12_GPU_VIRTUAL_ADDRESS  input_keys,
                    D3D12_GPU_VIRTUAL_ADDRESS  output_keys,
                    D3D12_GPU_VIRTUAL_ADDRESS  input_values,
                    D3D12_GPU_VIRTUAL_ADDRESS  output_values,
                    D3D12_GPU_VIRTUAL_ADDRESS  scratch_data,
                    std::uint32_t              size);

    /**
     * @brief Get the amount of scratch memory in bytes required to sort specified amount of elements.
     *
     * @param size Number of elements to sort.
     **/
    std::size_t GetScratchDataSize(std::uint32_t size) const;

private:
    void Init();

private:
    //< D3D device.
    ComPtr<ID3D12Device> device_ = nullptr;
    //< Root signature common for all kernels.
    ComPtr<ID3D12RootSignature> root_signature_ = nullptr;
    //< Histogram calculation kernel.
    ComPtr<ID3D12PipelineState> pipeline_state_histograms_ = nullptr;
    //< Key-value scatter kernel.
    ComPtr<ID3D12PipelineState> pipeline_state_scatter_ = nullptr;
    //< Scan for histogram scans.
    algorithm::Scan scan_;
};
}  // namespace rt::dx::algorithm