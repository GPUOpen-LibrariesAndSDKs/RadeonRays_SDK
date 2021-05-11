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

#include "dx/common/dx12_wrappers.h"
#include "dx/common.h"

namespace rt::dx
{
/**
 * @brief DX12 implementation of a device pointer.
 **/
class DevicePtr : public DevicePtrBackend<BackendType::kDx12>
{
public:
    /// Constructor.
    DevicePtr() = default;
    DevicePtr(ID3D12Resource* res, size_t off) : buffer(res), offset(off) {}
    /// Destructor.
    virtual ~DevicePtr() = default;

    /// Get DX12 virtual address of a resource.
    D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const override { return buffer->GetGPUVirtualAddress() + offset; }
    ID3D12Resource*           Get() const override { return buffer; }

private:
    // DX12 buffer.
    ID3D12Resource* buffer = nullptr;
    // Offset in the buffer.
    size_t offset = 0;
};
}  // namespace rt::dx