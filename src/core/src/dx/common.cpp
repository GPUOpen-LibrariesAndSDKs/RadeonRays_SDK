#include "common.h"

namespace rt::dx
{
ID3D12Resource* AllocateUAVBuffer(ID3D12Device* device, size_t size, D3D12_RESOURCE_STATES initial_state)
{
    ID3D12Resource* resource        = nullptr;
    auto            heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    auto            buffer_desc     = CD3DX12_RESOURCE_DESC::Buffer(size, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

    ThrowIfFailed(
        device->CreateCommittedResource(
            &heap_properties, D3D12_HEAP_FLAG_NONE, &buffer_desc, initial_state, nullptr, IID_PPV_ARGS(&resource)),
        "Cannot create UAV");

    return resource;
}

ID3D12Resource* AllocateUploadBuffer(ID3D12Device* device, size_t size, void* data)
{
    ID3D12Resource* resource        = nullptr;
    auto            heap_properties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto            buffer_desc     = CD3DX12_RESOURCE_DESC::Buffer(size);
    ThrowIfFailed(device->CreateCommittedResource(&heap_properties,
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
}  // namespace rt::dx