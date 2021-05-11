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
#include "base/command_stream_base.h"
#include "base/device_base.h"
#include "base/device_ptr_base.h"
#include "base/event_base.h"
#include "base/intersector_base.h"

// clang-format off
#include "utils/warning_push.h"
#include "utils/warning_ignore_general.h"
#include <d3d12.h>
#include "utils/warning_pop.h"
// clang-format on

namespace rt
{
template <>
class CommandStreamBackend<BackendType::kDx12> : public CommandStreamBase
{
public:
    virtual ID3D12Resource*            GetAllocatedTemporaryUAV(size_t)      = 0;
    virtual void                       ClearTemporaryUAVs()                  = 0;
    virtual ID3D12CommandAllocator*    GetAllocator() const                  = 0;
    virtual void                       SetAllocator(ID3D12CommandAllocator*) = 0;
    virtual ID3D12GraphicsCommandList* Get() const                           = 0;
    virtual void                       Set(ID3D12GraphicsCommandList* list)  = 0;
};

template <>
class EventBackend<BackendType::kDx12> : public EventBase
{
public:
    virtual ID3D12Fence* Get() const              = 0;
    virtual void         Set(ID3D12Fence* f)      = 0;
    virtual uint32_t     Value() const            = 0;
    virtual void         SetValue(uint32_t value) = 0;
    virtual HANDLE       Handle() const           = 0;
    virtual void         SetHandle(HANDLE h)      = 0;
};
template <>
class DeviceBackend<BackendType::kDx12> : public DeviceBase
{
public:
    virtual ID3D12Device*   Get() const                          = 0;
    virtual ID3D12Resource* AcquireTemporaryUAV(size_t)          = 0;
    virtual void            ReleaseTemporaryUAV(ID3D12Resource*) = 0;
};

template <>
class DevicePtrBackend<BackendType::kDx12> : public DevicePtrBase
{
public:
    virtual D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress() const = 0;
    virtual ID3D12Resource*           Get() const                  = 0;
};

}  // namespace rt