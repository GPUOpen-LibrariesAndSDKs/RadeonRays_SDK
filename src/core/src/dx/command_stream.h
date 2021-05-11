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

#include "dx/common.h"
#include "dx/common/dx12_wrappers.h"

namespace rt::dx
{
/**
 * @brief DX12 command stream implementation.
 *
 * DX12 commands stream implementation is based on DX12 command list.
 **/

class CommandStream : public CommandStreamBackend<BackendType::kDx12>
{
public:
    CommandStream(DeviceBackend<BackendType::kDx12>& dev) : device_(dev), external_(false) {}
    CommandStream(DeviceBackend<BackendType::kDx12>& dev, ID3D12GraphicsCommandList* cmd_list)
        : device_(dev), command_list_(cmd_list), external_(true)
    {
    }

    ~CommandStream();

    // Get temporary UAV and attach it to this command stream.
    ID3D12Resource*        GetAllocatedTemporaryUAV(size_t size) override;
    void                   ClearTemporaryUAVs() override;
    ID3D12GraphicsCommandList* Get() const override;
    void                       Set(ID3D12GraphicsCommandList* list) override;
    ID3D12CommandAllocator*    GetAllocator() const override;
    void                       SetAllocator(ID3D12CommandAllocator* allocator) override;

private:
    DeviceBackend<BackendType::kDx12>& device_;
    /// DX12 things.
    ComPtr<ID3D12GraphicsCommandList> command_list_      = nullptr;
    ComPtr<ID3D12CommandAllocator>    command_allocator_ = nullptr;

    /// Temporary resources attached by the device.
    std::vector<ID3D12Resource*> temp_uavs_;

    bool external_ = false;
};
}  // namespace rt::dx