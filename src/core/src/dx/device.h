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

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

#include "src/dx/command_stream.h"
#include "src/dx/common.h"
#include "src/dx/common/dx12_wrappers.h"
#include "src/dx/device_ptr.h"
#include "src/dx/event.h"
#include "src/utils/pool.h"

using Microsoft::WRL::ComPtr;

namespace rt::dx
{
/**
 * @brief DX12 raytracing device implementation.
 *
 * Provides basic functionality for resource allocation / pooling, command stream submission.
 **/
class Device : public DeviceBackend<BackendType::kDx12>
{
public:
    // Constructor.
    Device();
    explicit Device(ID3D12Device* d3d_device, ID3D12CommandQueue* command_queue);
    // Destructor.
    ~Device();

    /*************************************************************
     * DeviceBase interface overrrides.
     *************************************************************/
    /**
     * @brief Allocate a command stream.
     *
     * This operation does not usually involve any system memory allocations and is relatively lightweight.
     *
     * @return Allocated command stream.
     **/
    CommandStreamBase* AllocateCommandStream() override;

    /**
     * @brief Release a command stream.
     *
     * If the command stream is still executing on GPU, deferred deletion happens.
     *
     * @param command_stream_base Command stream to release.
     **/
    void ReleaseCommandStream(CommandStreamBase* command_stream_base) override;

    /**
     * @brief Submit a command stream.
     *
     * Waits for wait_event and signals returned event upon completion.
     *
     * @param command_stream_base Command stream to submit.
     * @param wait_event_base Event to wait for before submitting (or in the submission queue).
     * @return Event marking a submission.
     **/
    EventBase* SubmitCommandStream(CommandStreamBase* command_stream_base, EventBase* wait_event_base) override;

    /**
     * @brief Releases an event.
     *
     * Release an event allocated by a submission. This call is relatively lightweight and do not usually trigger any
     * memory allocations / deallocations.
     *
     * @param event_base Event to release.
     **/
    void ReleaseEvent(EventBase* event_base) override;

    /**
     * @brief Wait for an event.
     *
     * Block on CPU until the event has signaled.
     **/
    void WaitEvent(EventBase* event_base) override;

    /// DeviceBackend<BackendType::kDx12>
    // Get an underlying D3D device.
    ID3D12Device* Get() const override { return device_.Get(); }
    /// Allocate temporary staging buffer.
    ID3D12Resource* AcquireTemporaryUAV(size_t) override { return temporary_uav_pool_.AcquireObject(); }
    void            ReleaseTemporaryUAV(ID3D12Resource* uav) override { temporary_uav_pool_.ReleaseObject(uav); }

private:
    void CreateDXGIAdapter(D3D_FEATURE_LEVEL feature_level);
    void CreateDeviceAndCommandQueue(D3D_FEATURE_LEVEL feature_level);
    void InitD3D();
    void InitializePools();

private:
    // DX12 stuff.
    ComPtr<IDXGIFactory4>      dxgi_factory_  = nullptr;
    ComPtr<IDXGIAdapter>       dxgi_adapter_  = nullptr;
    ComPtr<ID3D12Device>       device_        = nullptr;
    ComPtr<ID3D12CommandQueue> command_queue_ = nullptr;

    // Number of executed command lists so far.
    std::atomic<std::uint32_t> command_list_counter_ = 0u;
    // Event pool for submission events.
    Pool<EventBackend<BackendType::kDx12>> event_pool_;
    // Command stream pool for submissions pulling.
    Pool<CommandStreamBackend<BackendType::kDx12>> command_stream_pool_;
    // Temporary buffer pool.
    Pool<ID3D12Resource> temporary_uav_pool_;

    friend class CommandStream;
};

/// Create device from scratch.
std::unique_ptr<rt::DeviceBase> CreateDevice();
/// Create device from existing D3D device and command queue.
std::unique_ptr<rt::DeviceBase> CreateDevice(ID3D12Device* d3d_device, ID3D12CommandQueue* command_queue);

/// Create device pointer from D3D resource.
DevicePtr* CreateDevicePtr(ID3D12Resource* resource, size_t offset);
}  // namespace rt::dx