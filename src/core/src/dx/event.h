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
 * @brief DX12 implementation of a sync event.
 *
 * Sync event is synchronizing GPU work submission / completion.
 **/
class Event : public EventBackend<BackendType::kDx12>
{
public:
    /// Constructor.
    Event() = default;
    ID3D12Fence* Get() const override { return fence; }
    uint32_t     Value() const override { return value; }
    HANDLE       Handle() const override { return fence_event; }
    void         SetHandle(HANDLE h) override { fence_event = h; }
    void         SetValue(uint32_t val) override { value = val; }
    void         Set(ID3D12Fence* f) override { fence = f; }

private:
    /// Fence for GPU work submission sync.
    ID3D12Fence* fence = nullptr;
    /// Fence value this event is waiting for.
    uint32_t value = 0u;
    /// Handle for Win32 event associated with the fence.
    HANDLE fence_event;
};
}  // namespace rt::dx