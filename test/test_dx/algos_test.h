#pragma once

#include <chrono>
#include <climits>
#include <iostream>
#include <numeric>
#include <random>
#include <stack>

#include "dx/algorithm/radix_sort.h"
#include "dx/algorithm/scan.h"
#include "dx/dxassist.h"
#include "dx/shader_compiler.h"
#include "gtest/gtest.h"

#define CHECK_RR_CALL(x) ASSERT_EQ((x), RR_SUCCESS)

using namespace std::chrono;
using namespace rt::dx;
static constexpr uint32_t kKeysCount    = 1024u * 1024u;
static constexpr uint32_t kKeysBigCount = 1024u * 1024u * 16u;

#ifdef _WIN32
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

class AlgosTest : public ::testing::Test
{
public:
    void SetUp() override {}

    void TearDown() override {}

    DxAssist dxassist_;
};

inline std::vector<uint32_t> generate_data(size_t size)
{
    static std::uniform_int_distribution<uint32_t> distribution(std::numeric_limits<uint32_t>::min(), 1000u);
    static std::default_random_engine                generator;

    std::vector<uint32_t> data(size);
    std::generate(data.begin(), data.end(), []() { return distribution(generator); });
    return data;
}

TEST_F(AlgosTest, ScanTest)
{
    using algorithm::Scan;
    auto device = dxassist_.device();
    Scan scan_kernel(device);
    auto to_scan                 = generate_data(kKeysCount);
    auto scratch_size            = scan_kernel.GetScratchDataSize(kKeysCount);
    auto scratch_buffer          = dxassist_.CreateUAVBuffer(scratch_size);
    auto to_scan_buffer          = dxassist_.CreateUploadBuffer(to_scan.size() * sizeof(uint32_t), to_scan.data());
    auto scanned_buffer          = dxassist_.CreateUAVBuffer(to_scan.size() * sizeof(uint32_t));
    auto scanned_readback_buffer = dxassist_.CreateReadBackBuffer(to_scan.size() * sizeof(uint32_t));
    auto command_allocator       = dxassist_.CreateCommandAllocator();
    auto command_list            = dxassist_.CreateCommandList(command_allocator.Get());

    scan_kernel(command_list.Get(),
                to_scan_buffer->GetGPUVirtualAddress(),
                scanned_buffer->GetGPUVirtualAddress(),
                scratch_buffer->GetGPUVirtualAddress(),
                kKeysCount);
    command_list->Close();
    ID3D12CommandList* scan_list[] = {command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, scan_list);
    auto scan_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(scan_fence.Get(), 1000);
    while (scan_fence->GetCompletedValue() != 1000) Sleep(0);

    auto                   copy_keys_command_list = dxassist_.CreateCommandList(command_allocator.Get());
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = scanned_buffer.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

    copy_keys_command_list->ResourceBarrier(1, &barrier);
    copy_keys_command_list->CopyBufferRegion(
        scanned_readback_buffer.Get(), 0, scanned_buffer.Get(), 0, kKeysCount * sizeof(uint32_t));
    copy_keys_command_list->Close();

    ID3D12CommandList* copy_list[] = {copy_keys_command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, copy_list);
    auto keys_copy_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(keys_copy_fence.Get(), 2000);
    while (keys_copy_fence->GetCompletedValue() != 2000) Sleep(0);

    std::vector<uint32_t> scanned_cpu;
    scanned_cpu.reserve(kKeysCount);
    std::exclusive_scan(to_scan.begin(), to_scan.end(), std::back_inserter(scanned_cpu), 0);

    {
        uint32_t* mapped_ptr;
        scanned_readback_buffer->Map(0, nullptr, (void**)&mapped_ptr);
        for (const auto scanned_value : scanned_cpu)
        {
            ASSERT_EQ(*(mapped_ptr++), scanned_value);
        }
        scanned_readback_buffer->Unmap(0, nullptr);
    }
}

TEST_F(AlgosTest, RadixSortTest)
{
    using algorithm::RadixSortKeyValue;
    auto              device = dxassist_.device();
    RadixSortKeyValue sort_kernel(device);
    auto              to_sort_values = generate_data(kKeysCount);
    auto              scratch_size   = sort_kernel.GetScratchDataSize(kKeysCount);
    auto              scratch_buffer = dxassist_.CreateUAVBuffer(scratch_size);

    auto to_sort_keys_buffer =
        dxassist_.CreateUploadBuffer(to_sort_values.size() * sizeof(uint32_t), to_sort_values.data());
    auto to_sort_vals_buffer =
        dxassist_.CreateUploadBuffer(to_sort_values.size() * sizeof(uint32_t), to_sort_values.data());
    auto sorted_keys_buffer = dxassist_.CreateUAVBuffer(to_sort_values.size() * sizeof(uint32_t));
    auto sorted_vals_buffer = dxassist_.CreateUAVBuffer(to_sort_values.size() * sizeof(uint32_t));

    auto sorted_vals_readback_buffer = dxassist_.CreateReadBackBuffer(to_sort_values.size() * sizeof(uint32_t));
    auto command_allocator           = dxassist_.CreateCommandAllocator();
    auto command_list                = dxassist_.CreateCommandList(command_allocator.Get());

    sort_kernel(command_list.Get(),
                to_sort_keys_buffer->GetGPUVirtualAddress(),
                sorted_keys_buffer->GetGPUVirtualAddress(),
                to_sort_vals_buffer->GetGPUVirtualAddress(),
                sorted_vals_buffer->GetGPUVirtualAddress(),
                scratch_buffer->GetGPUVirtualAddress(),
                kKeysCount);
    command_list->Close();
    ID3D12CommandList* sort_list[] = {command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, sort_list);
    auto sort_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(sort_fence.Get(), 1000);
    while (sort_fence->GetCompletedValue() != 1000) Sleep(0);

    auto                   copy_vals_command_list = dxassist_.CreateCommandList(command_allocator.Get());
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = sorted_vals_buffer.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

    copy_vals_command_list->ResourceBarrier(1, &barrier);
    copy_vals_command_list->CopyBufferRegion(
        sorted_vals_readback_buffer.Get(), 0, sorted_vals_buffer.Get(), 0, kKeysCount * sizeof(uint32_t));
    copy_vals_command_list->Close();

    ID3D12CommandList* copy_list[] = {copy_vals_command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, copy_list);
    auto vals_copy_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(vals_copy_fence.Get(), 2000);
    while (vals_copy_fence->GetCompletedValue() != 2000) Sleep(0);

    std::sort(to_sort_values.begin(), to_sort_values.end());

    {
        uint32_t* mapped_ptr;
        sorted_vals_readback_buffer->Map(0, nullptr, (void**)&mapped_ptr);
        for (const auto scanned_value : to_sort_values)
        {
            ASSERT_EQ(*(mapped_ptr++), scanned_value);
        }
        sorted_vals_readback_buffer->Unmap(0, nullptr);
    }
}

//#define PERFORMANCE_TESTING
#ifdef PERFORMANCE_TESTING

TEST_F(AlgosTest, ScanTest4Perf)
{
    using algorithm::Scan;
    auto                    device = dxassist_.device();
    ComPtr<ID3D12QueryHeap> query_heap;
    D3D12_QUERY_HEAP_DESC   query_heap_desc = {};
    query_heap_desc.Count                   = 2;
    query_heap_desc.Type                    = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    device->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(&query_heap));

    Scan scan_kernel(device);
    auto to_scan                   = generate_data(kKeysBigCount);
    auto scratch_size              = scan_kernel.GetScratchDataSize(kKeysBigCount);
    auto scratch_buffer            = dxassist_.CreateUAVBuffer(scratch_size);
    auto to_scan_buffer            = dxassist_.CreateUploadBuffer(to_scan.size() * sizeof(uint32_t), to_scan.data());
    auto to_scan_gpu_buffer        = dxassist_.CreateUAVBuffer(to_scan.size() * sizeof(uint32_t));
    auto scanned_buffer            = dxassist_.CreateUAVBuffer(to_scan.size() * sizeof(uint32_t));
    auto scanned_readback_buffer   = dxassist_.CreateReadBackBuffer(to_scan.size() * sizeof(uint32_t));
    auto timestamp_buffer          = dxassist_.CreateUAVBuffer(sizeof(std::uint64_t) * 8);
    auto timestamp_readback_buffer = dxassist_.CreateReadBackBuffer(sizeof(std::uint64_t) * 8);

    auto command_allocator = dxassist_.CreateCommandAllocator();

    auto                   copy_keys_command_list = dxassist_.CreateCommandList(command_allocator.Get());
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = to_scan_gpu_buffer.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

    copy_keys_command_list->ResourceBarrier(1, &barrier);
    copy_keys_command_list->CopyBufferRegion(
        to_scan_gpu_buffer.Get(), 0, to_scan_buffer.Get(), 0, to_scan.size() * sizeof(uint32_t));
    copy_keys_command_list->Close();

    ID3D12CommandList* copy_list[] = {copy_keys_command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, copy_list);
    auto keys_copy_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(keys_copy_fence.Get(), 2000);
    while (keys_copy_fence->GetCompletedValue() != 2000) Sleep(0);

    D3D12_RESOURCE_BARRIER ts_buffer_barrier;
    ts_buffer_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    ts_buffer_barrier.Transition.pResource   = timestamp_buffer.Get();
    ts_buffer_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    ts_buffer_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    ts_buffer_barrier.Transition.Subresource = 0;
    ts_buffer_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;

    auto command_list = dxassist_.CreateCommandList(command_allocator.Get());
    command_list->ResourceBarrier(1, &ts_buffer_barrier);
    command_list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    for (auto i = 0; i < 10; i++)
    {
        scan_kernel(command_list.Get(),
                    to_scan_gpu_buffer->GetGPUVirtualAddress(),
                    scanned_buffer->GetGPUVirtualAddress(),
                    scratch_buffer->GetGPUVirtualAddress(),
                    kKeysBigCount);
    }
    command_list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
    command_list->ResolveQueryData(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, timestamp_buffer.Get(), 0);
    ts_buffer_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    ts_buffer_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
    command_list->ResourceBarrier(1, &ts_buffer_barrier);
    command_list->CopyBufferRegion(timestamp_readback_buffer.Get(), 0, timestamp_buffer.Get(), 0, sizeof(UINT64) * 8);
    command_list->Close();

    ID3D12CommandList* scan_list[] = {command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, scan_list);
    auto scan_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(scan_fence.Get(), 1000);
    while (scan_fence->GetCompletedValue() != 1000) Sleep(0);
    float delta_s = 0.f;
    {
        UINT64 freq = 0;
        dxassist_.command_queue()->GetTimestampFrequency(&freq);

        UINT64* mapped_ptr;
        timestamp_readback_buffer->Map(0, nullptr, (void**)&mapped_ptr);

        auto num_ticks = mapped_ptr[1] - mapped_ptr[0];
        delta_s        = float(num_ticks) / freq / 10;

        // Bounding boxes
        timestamp_readback_buffer->Unmap(0, nullptr);
    }
    std::cout << "Avg execution time: " << delta_s * 1000 << "ms\n";
    std::cout << "Throughput: " << (kKeysBigCount) / delta_s * 1e-6 << " MKeys/s\n";
}

TEST_F(AlgosTest, RadixSortTest4Perf)
{
    using algorithm::RadixSortKeyValue;
    auto                    device = dxassist_.device();
    ComPtr<ID3D12QueryHeap> query_heap;
    D3D12_QUERY_HEAP_DESC   query_heap_desc = {};
    query_heap_desc.Count                   = 2;
    query_heap_desc.Type                    = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    device->CreateQueryHeap(&query_heap_desc, IID_PPV_ARGS(&query_heap));

    RadixSortKeyValue sort_kernel(device);
    auto              to_sort_values = generate_data(kKeysBigCount);
    auto              scratch_size   = sort_kernel.GetScratchDataSize(kKeysBigCount);
    auto              scratch_buffer = dxassist_.CreateUAVBuffer(scratch_size);

    auto to_sort_buffer =
        dxassist_.CreateUploadBuffer(to_sort_values.size() * sizeof(uint32_t), to_sort_values.data());
    auto to_sort_keys_gpu_buffer = dxassist_.CreateUAVBuffer(to_sort_values.size() * sizeof(uint32_t));
    auto to_sort_vals_gpu_buffer = dxassist_.CreateUAVBuffer(to_sort_values.size() * sizeof(uint32_t));
    auto sorted_keys_buffer      = dxassist_.CreateUAVBuffer(to_sort_values.size() * sizeof(uint32_t));
    auto sorted_vals_buffer      = dxassist_.CreateUAVBuffer(to_sort_values.size() * sizeof(uint32_t));

    auto timestamp_buffer          = dxassist_.CreateUAVBuffer(sizeof(std::uint64_t) * 8);
    auto timestamp_readback_buffer = dxassist_.CreateReadBackBuffer(sizeof(std::uint64_t) * 8);

    auto                   command_allocator      = dxassist_.CreateCommandAllocator();
    auto                   copy_keys_command_list = dxassist_.CreateCommandList(command_allocator.Get());
    D3D12_RESOURCE_BARRIER barrier_keys;
    barrier_keys.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier_keys.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier_keys.Transition.pResource   = to_sort_keys_gpu_buffer.Get();
    barrier_keys.Transition.Subresource = 0;
    barrier_keys.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier_keys.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_RESOURCE_BARRIER barrier_vals;
    barrier_vals.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier_vals.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier_vals.Transition.pResource   = to_sort_vals_gpu_buffer.Get();
    barrier_vals.Transition.Subresource = 0;
    barrier_vals.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier_vals.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;

    D3D12_RESOURCE_BARRIER barriers[] = {barrier_keys, barrier_vals};
    copy_keys_command_list->ResourceBarrier(2, barriers);
    copy_keys_command_list->CopyBufferRegion(
        to_sort_keys_gpu_buffer.Get(), 0, to_sort_buffer.Get(), 0, to_sort_values.size() * sizeof(uint32_t));
    copy_keys_command_list->CopyBufferRegion(
        to_sort_vals_gpu_buffer.Get(), 0, to_sort_buffer.Get(), 0, to_sort_values.size() * sizeof(uint32_t));

    copy_keys_command_list->Close();

    ID3D12CommandList* copy_list[] = {copy_keys_command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, copy_list);
    auto keys_copy_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(keys_copy_fence.Get(), 2000);
    while (keys_copy_fence->GetCompletedValue() != 2000) Sleep(0);

    auto command_list = dxassist_.CreateCommandList(command_allocator.Get());

    D3D12_RESOURCE_BARRIER ts_buffer_barrier;
    ts_buffer_barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    ts_buffer_barrier.Transition.pResource   = timestamp_buffer.Get();
    ts_buffer_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    ts_buffer_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
    ts_buffer_barrier.Transition.Subresource = 0;
    ts_buffer_barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;

    command_list->ResourceBarrier(1, &ts_buffer_barrier);
    command_list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0);
    for (auto i = 0; i < 10; i++)
    {
        sort_kernel(command_list.Get(),
                    to_sort_keys_gpu_buffer->GetGPUVirtualAddress(),
                    sorted_keys_buffer->GetGPUVirtualAddress(),
                    to_sort_vals_gpu_buffer->GetGPUVirtualAddress(),
                    sorted_vals_buffer->GetGPUVirtualAddress(),
                    scratch_buffer->GetGPUVirtualAddress(),
                    kKeysBigCount);
    }
    command_list->EndQuery(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 1);
    command_list->ResolveQueryData(query_heap.Get(), D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, timestamp_buffer.Get(), 0);
    ts_buffer_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    ts_buffer_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;
    command_list->ResourceBarrier(1, &ts_buffer_barrier);
    command_list->CopyBufferRegion(timestamp_readback_buffer.Get(), 0, timestamp_buffer.Get(), 0, sizeof(UINT64) * 8);
    command_list->Close();

    ID3D12CommandList* sort_list[] = {command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, sort_list);
    auto sort_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(sort_fence.Get(), 1000);
    while (sort_fence->GetCompletedValue() != 1000) Sleep(0);

    float delta_s = 0.f;
    {
        UINT64 freq = 0;
        dxassist_.command_queue()->GetTimestampFrequency(&freq);

        UINT64* mapped_ptr;
        timestamp_readback_buffer->Map(0, nullptr, (void**)&mapped_ptr);

        auto num_ticks = mapped_ptr[1] - mapped_ptr[0];
        delta_s        = float(num_ticks) / freq / 10;

        // Bounding boxes
        timestamp_readback_buffer->Unmap(0, nullptr);
    }
    std::cout << "Avg execution time: " << delta_s * 1000 << "ms\n";
    std::cout << "Throughput: " << (kKeysBigCount) / delta_s * 1e-6 << " MKeys/s\n";
}

#endif