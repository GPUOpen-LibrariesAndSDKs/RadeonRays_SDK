#include "device.h"

#include <vector>

#include "dx/command_stream.h"
#include "dx/common.h"
#include "dx/device_ptr.h"
#include "dx/event.h"
#include "dx/shader_compiler.h"
#include "utils/logger.h"


namespace
{
bool IsRaytracingSupported(ID3D12Device* device)
{
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 support_data = {};
    return SUCCEEDED(device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &support_data, sizeof(support_data))) &&
           support_data.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}
}  // namespace

namespace rt::dx
{
std::unique_ptr<rt::DeviceBase> CreateDevice() { return std::make_unique<Device>(); }

std::unique_ptr<rt::DeviceBase> CreateDevice(ID3D12Device* d3d_device, ID3D12CommandQueue* command_queue)
{
    return std::make_unique<Device>(d3d_device, command_queue);
}

DevicePtr* CreateDevicePtr(ID3D12Resource* resource, size_t offset) { return new DevicePtr(resource, offset); }

void Device::CreateDXGIAdapter(D3D_FEATURE_LEVEL feature_level)
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
            Logger::Get().Warn("Direct3D Debug Device is not available");
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

    if (!debug_dxgi)
    {
        ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory_)), "Cannot create DXGI factory");
    }

    Logger::Get().Debug("DXGI factory created");

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
        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), feature_level, _uuidof(ID3D12Device), &test_device)))
        {
            selected_id = id;

            Logger::Get().Debug("Direct3D Adapter {}: VID:{}, DID:{} - {}",
                 id,
                 desc.VendorId,
                 desc.DeviceId,
                 WcharToString(desc.Description));
            break;
        }
    }

    if (!adapter)
    {
        Throw("No compatible adapters found");
    }

    dxgi_adapter_ = adapter.Detach();
}

void Device::CreateDeviceAndCommandQueue(D3D_FEATURE_LEVEL feature_level)
{
    Logger::Get().Debug("Initalizing DirectX in standalone mode");

    // Create the DX12 API device object.
    ThrowIfFailed(D3D12CreateDevice(dxgi_adapter_.Get(), feature_level, IID_PPV_ARGS(&device_)),
                  "Cannot create D3D12 device");

    Logger::Get().Debug("D3D12 device successfully created");

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags                    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type                     = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_)),
                  "Cannot create command queue");

    Logger::Get().Debug("Command queue successfully created");
}

void Device::InitD3D()
{
    Logger::Get().Debug("Initializing resource pools");
    InitializePools();
    Logger::Get().Debug("Resource pools initialized");
}

void Device::InitializePools()
{
    // Initalize fence pool creation functions.
    event_pool_.SetCreateFn([device = device_]() {
        EventBackend<BackendType::kDx12>* event = new Event();
        ID3D12Fence*                      fence = nullptr;
        ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)), "Cannot create fence");
        event->Set(fence);
        event->SetHandle(CreateEvent(nullptr, FALSE, FALSE, nullptr));
        return event;
    });

    event_pool_.SetDeleteFn([](EventBackend<BackendType::kDx12>* event) {
        event->Get()->Release();
        event->Set(nullptr);
        CloseHandle(event->Handle());
        delete event;
    });

    // Initialize command stream pool funcs.
    command_stream_pool_.SetCreateFn([me = this, device = device_]() {
        CommandStreamBackend<BackendType::kDx12>* stream    = new CommandStream(*me);
        ID3D12CommandAllocator*                   allocator = nullptr;
        ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)),
                      "Cannot create command allocator");

        stream->SetAllocator(allocator);
        ID3D12GraphicsCommandList* list = nullptr;
        ThrowIfFailed(device->CreateCommandList(
                          0u, D3D12_COMMAND_LIST_TYPE_DIRECT, stream->GetAllocator(), nullptr, IID_PPV_ARGS(&list)),
                      "Cannot create command stream");
        stream->Set(list);
        return stream;
    });

    command_stream_pool_.SetDeleteFn(
        [](CommandStreamBackend<BackendType::kDx12>* command_stream) { delete command_stream; });

    // Prepare temporary resource pool for staging buffers.
    temporary_uav_pool_.SetCreateFn([device = device_.Get()]() {
        constexpr size_t kMaxTempUAVSize = 256 * 1024 * 1024;

        return AllocateUploadBuffer(device, kMaxTempUAVSize);
    });

    temporary_uav_pool_.SetDeleteFn([](ID3D12Resource* uav) { uav->Release(); });
}

CommandStreamBase* Device::AllocateCommandStream() { return command_stream_pool_.AcquireObject(); }

void Device::ReleaseCommandStream(CommandStreamBase* command_stream_base)
{
    auto command_stream = dynamic_cast<CommandStreamBackend<BackendType::kDx12>*>(command_stream_base);

    // Release temporary objects back to the pool.
    command_stream->ClearTemporaryUAVs();

    // Reset allocators.
    command_stream->GetAllocator()->Reset();
    command_stream->Get()->Reset(command_stream->GetAllocator(), nullptr);

    // Release command stream back to the pool.
    command_stream_pool_.ReleaseObject(command_stream);
}

EventBase* Device::SubmitCommandStream(CommandStreamBase* command_stream_base, EventBase* wait_event_base)
{
    Logger::Get().Debug("Device::SubmitCommandStream()");

    auto command_stream =
        dynamic_cast<CommandStreamBackend<BackendType::kDx12>*>(command_stream_base);

    EventBackend<BackendType::kDx12>* event = event_pool_.AcquireObject();
    event->SetValue(command_list_counter_.fetch_add(1) + 1);

    if (wait_event_base)
    {
        EventBackend<BackendType::kDx12>* wait_event = dynamic_cast<EventBackend<BackendType::kDx12>*>(wait_event_base);
        command_queue_->Wait(wait_event->Get(), wait_event->Value());
    }

    command_stream->Get()->Close();

    ID3D12CommandList* command_lists[] = {command_stream->Get()};

    command_queue_->ExecuteCommandLists(1u, command_lists);
    command_queue_->Signal(event->Get(), command_list_counter_);
    return event;
}

void Device::ReleaseEvent(EventBase* event_base)
{
    Logger::Get().Debug("Device::ReleaseEvent()");

    EventBackend<BackendType::kDx12>* event = dynamic_cast<EventBackend<BackendType::kDx12>*>(event_base);
    event_pool_.ReleaseObject(event);
}

void Device::WaitEvent(EventBase* event_base)
{
    Logger::Get().Debug("Device::WaitEvent()");

    Event* event = reinterpret_cast<Event*>(event_base);

    auto completed_value = event->Get()->GetCompletedValue();
    if (completed_value < event->Value())
    {
        ResetEvent(event->Handle());
        event->Get()->SetEventOnCompletion(event->Value(), event->Handle());
        WaitForSingleObject(event->Handle(), INFINITE);
    }
}

Device::Device()
{
    constexpr auto feature_level = D3D_FEATURE_LEVEL_12_0;
    CreateDXGIAdapter(feature_level);
    CreateDeviceAndCommandQueue(feature_level);
    InitD3D();
}

Device::Device(ID3D12Device* d3d_device, ID3D12CommandQueue* command_queue)
    : device_(d3d_device), command_queue_(command_queue)
{
    // Skip DXGI part.
    InitD3D();
}
Device::~Device() = default;
}  // namespace rt::dx