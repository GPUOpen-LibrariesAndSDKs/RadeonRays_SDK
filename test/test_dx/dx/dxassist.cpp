#include "dxassist.h"

#include "common.h"
#include "common/d3dx12.h"

DxAssist::DxAssist() { InitDX12(); }

void DxAssist::InitD3D(D3D_FEATURE_LEVEL feature_level)
{
    // Create the DX12 API device object.
    ThrowIfFailed(D3D12CreateDevice(dxgi_adapter_.Get(), feature_level, IID_PPV_ARGS(&device_)),
                  "Cannot create D3D12 device");

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_)),
                  "Cannot create command queue");
}

ComPtr<ID3D12GraphicsCommandList> DxAssist::CreateCommandList(ID3D12CommandAllocator* command_allocator) const
{
    ComPtr<ID3D12GraphicsCommandList> command_list;

    ThrowIfFailed(device()->CreateCommandList(
                      0u, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator, nullptr, IID_PPV_ARGS(&command_list)),
                  "Cannot create command stream");

    return command_list;
}

ComPtr<ID3D12CommandAllocator> DxAssist::CreateCommandAllocator() const
{
    ComPtr<ID3D12CommandAllocator> command_allocator;
    ThrowIfFailed(device()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&command_allocator)),
                  "Cannot create command allocator");

    return command_allocator;
}

ComPtr<ID3D12Fence> DxAssist::CreateFence() const
{
    ComPtr<ID3D12Fence> fence;
    ThrowIfFailed(device()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "Cannot create fence");
    return fence;
}

ComPtr<ID3D12Resource> DxAssist::CreateUploadBuffer(UINT64 size, void* data) const
{
    ComPtr<ID3D12Resource> resource        = nullptr;
    auto                   heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto                   buffer_desc     = CD3DX12_RESOURCE_DESC::Buffer(size);
    ThrowIfFailed(device()->CreateCommittedResource(&heap_properties,
                                                    D3D12_HEAP_FLAG_NONE,
                                                    &buffer_desc,
                                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                                    nullptr,
                                                    IID_PPV_ARGS(&resource)),
                  "Cannot create upload buffer");

    if (data)
    {
        void* mapped_ptr;
        resource->Map(0, nullptr, &mapped_ptr);
        std::memcpy(mapped_ptr, data, size);
        resource->Unmap(0, nullptr);
    }

    return resource;
}


ComPtr<ID3D12Resource> DxAssist::CreateUAVBuffer(UINT64 size, D3D12_RESOURCE_STATES initial_state) const
{
    ComPtr<ID3D12Resource> resource        = nullptr;
    auto                   heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto buffer_desc = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    ThrowIfFailed(
        device()->CreateCommittedResource(
            &heap_properties, D3D12_HEAP_FLAG_NONE, &buffer_desc, initial_state, nullptr, IID_PPV_ARGS(&resource)),
        "Cannot create UAV");

    return resource;
}

ComPtr<ID3D12Resource> DxAssist::CreateReadBackBuffer(UINT64 size) const
{
    ComPtr<ID3D12Resource> resource        = nullptr;
    auto                   heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
    auto                   buffer_desc     = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_NONE);

    ThrowIfFailed(device()->CreateCommittedResource(&heap_properties,
                                                    D3D12_HEAP_FLAG_NONE,
                                                    &buffer_desc,
                                                    D3D12_RESOURCE_STATE_COPY_DEST,
                                                    nullptr,
                                                    IID_PPV_ARGS(&resource)),
                  "Cannot create UAV");

    return resource;
}

void DxAssist::InitDX12()
{
    InitDXGI(D3D_FEATURE_LEVEL_12_0);
    InitD3D(D3D_FEATURE_LEVEL_12_0);
}

void DxAssist::InitDXGI(D3D_FEATURE_LEVEL feature_level)
{
    bool debug_dxgi = false;
#if defined(_DEBUG)
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the
    // active device.
    {
        ComPtr<ID3D12Debug> debug_controller;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller))))
        {
            debug_controller->EnableDebugLayer();
        } else
        {
            Throw("Direct3D Debug Device is not available");
        }

        ComPtr<IDXGIInfoQueue> dxgi_info_queue;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgi_info_queue))))
        {
            debug_dxgi = true;

            ThrowIfFailed(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgi_factory_)),
                          "Cannot create debug DXGI factory");

            dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
            dxgi_info_queue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        }
    }
#endif

    DXGIGetDebugInterface1(0, IID_PPV_ARGS(&graphics_analysis_));

    if (!debug_dxgi)
    {
        ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory_)), "Cannot create DXGI factory");
    }

    // Now try to find the adapter.
    UINT selected_id = UINT_MAX;

    ComPtr<IDXGIAdapter1> adapter;
    for (UINT id = 0; DXGI_ERROR_NOT_FOUND != dxgi_factory_->EnumAdapters1(id, &adapter); ++id)
    {
        DXGI_ADAPTER_DESC1 desc;
        ThrowIfFailed(adapter->GetDesc1(&desc), "Cannot obtain adapter description");

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            // Don't select the Basic Render Driver adapter.
            continue;
        }

        // Check to see if the adapter supports Direct3D 12, but don't create
        // the actual device yet.
        ComPtr<ID3D12Device> test_device;
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), feature_level, IID_PPV_ARGS(&test_device))))
        {
            selected_id = id;
            break;
        }
    }

    if (!adapter)
    {
        Throw("No compatible adapters found");
    }

    dxgi_adapter_ = adapter.Detach();
}
