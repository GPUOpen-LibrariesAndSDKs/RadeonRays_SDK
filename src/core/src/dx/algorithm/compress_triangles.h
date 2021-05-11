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


namespace rt::dx::algorithm
{
//< Compressed block layout.
struct CompressedBlock
{
    float         v0[3];
    float         v1[3];
    float         v2[3];
    float         v3[3];
    float         v4[3];
    std::uint32_t triangle_id;  // Identifies which triangles are occupied.
};

/**
 * @brief Triangle compressor.
 *
 *  Compresses a list of indexed triangles loselessly into a set of blocks up to 4 triangles each (fan, consisting of 5
 * vertices).
 */
class CompressTriangles
{
public:
    CompressTriangles(ID3D12Device* device) : device_(device) { Init(); }

    /**
     * @brief Record compression into command list.
     *
     * Given a list of N triangles, it encodes them into M blocks (N/4 <= M <= N), where each block contains up to
     * 4 triangles. Triangle pointers are written into triangle_pointers array, one per original triangle. Number
     * of blocks written can be queried later using GetLastWrittenBlocksCountBuffer().
     *
     * @param vertices GPU VA of a vertex buffer.
     * @param vertex_stride Number of bytes between to successive vertices.
     * @param vertex_count Number of vertices in the array.
     * @param indices GPU VA of an index buffer.
     * @param triangle_count Number of trinagles in index buffer.
     * @param compressed_blocks GPU VA to output blocks at.
     * @param triangle_pointers Encoded triangle pointers (in the order,
     *  in which triangles are in an index buffer.)
     **/
    void operator()(ID3D12GraphicsCommandList* command_list,
                    D3D12_GPU_VIRTUAL_ADDRESS  vertices,
                    std::uint32_t              vertex_stride,
                    std::uint32_t              vertex_count,
                    D3D12_GPU_VIRTUAL_ADDRESS  indices,
                    std::uint32_t              triangle_count,
                    D3D12_GPU_VIRTUAL_ADDRESS  compressed_blocks,
                    D3D12_GPU_VIRTUAL_ADDRESS  triangle_pointers);

    /**
     * @brief Get size if bytes required to hold compressed blocks for triangle_count triangles.
     *
     * @param triangle_count Number of triangles to compress.
     * @return Size in bytes required for resulting compressed blocks.
     **/
    std::size_t GetResultDataSize(std::uint32_t triangle_count) const
    {
        return triangle_count * sizeof(CompressedBlock);
    }

    /**
     * @brief Get the buffer with the amount of compressed blocks written last time.
     * @return UAV buffer containing the counter.
     **/
    ID3D12Resource* GetLastWrittenBlocksCountBuffer() { return blocks_count_.Get(); }

private:
    void Init();

private:
    //< D3D device.
    ComPtr<ID3D12Device> device_;
    //< Root signature common for all kernels.
    ComPtr<ID3D12RootSignature> root_signature_;
    //< Compression kernel.
    ComPtr<ID3D12PipelineState> pipeline_state_;
    //< Init kernel.
    ComPtr<ID3D12PipelineState> pipeline_state_init_;
    // Buffer to keep track of written compressed blocks.
    ComPtr<ID3D12Resource> blocks_count_;
};
}  // namespace rt::dx::algorithm