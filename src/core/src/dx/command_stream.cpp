#include "command_stream.h"

namespace rt::dx
{
ID3D12Resource* CommandStream::GetAllocatedTemporaryUAV(size_t size)
{
    auto temp_uav = device_.AcquireTemporaryUAV(size);
    temp_uavs_.push_back(temp_uav);
    return temp_uav;
}

void CommandStream::ClearTemporaryUAVs()
{
    for (auto& uav : temp_uavs_)
    {
        device_.ReleaseTemporaryUAV(uav);
    }
    temp_uavs_.clear();
}

ID3D12GraphicsCommandList* CommandStream::Get() const { return command_list_.Get(); }

void                    CommandStream::Set(ID3D12GraphicsCommandList* list) { command_list_ = list; }

ID3D12CommandAllocator* CommandStream::GetAllocator() const { return command_allocator_.Get(); }

void CommandStream::SetAllocator(ID3D12CommandAllocator* allocator) { command_allocator_ = allocator; }

CommandStream::~CommandStream()
{
    if (external_)
    {
        for (auto& uav : temp_uavs_)
        {
            device_.ReleaseTemporaryUAV(uav);
        }
    }
}
}  // namespace rt::dx