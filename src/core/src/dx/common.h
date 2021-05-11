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

#define NOMINMAX
// clang-format off
#include "utils/warning_push.h"
#include "utils/warning_ignore_general.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <windows.h>
#include <wrl.h>
#include "dx/common/d3dx12.h"
#include "radeonrays.h"
#include "utils/warning_pop.h"
// clang-format on

#include <stdexcept>
#include <string>

#include "dx/common/dx12_wrappers.h"
#include "dx/device_ptr.h"

using Microsoft::WRL::ComPtr;
using std::size_t;
using std::uint32_t;

//< HLSL compat.
using uint   = std::uint32_t;
using float3 = float[3];
using float4 = float[4];
using uint4  = uint[4];
using uint2  = uint[2];

namespace rt::dx
{
// Intersector types for DX.
enum IntersectorType
{
    kCompute
};

constexpr static std::uint32_t kMaxInstances = 1024 * 1024;

// Helper function to throw runtime_error.
template <typename T>
inline void Throw(T&& data)
{
    throw std::runtime_error(std::forward<T>(data));
}

// Helper function to throw runtime_error.
template <typename T>
inline void ThrowIfFailed(HRESULT hr, T&& data)
{
    if (FAILED(hr))
    {
        Throw(std::forward<T>(data));
    }
}

// Helper function to throw runtime_error.
template <typename T>
inline void ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw std::runtime_error("Internal error");
    }
}

// Convert wide char string to std::string.
inline std::string WcharToString(const WCHAR* wstr)
{
    // convert from wide char to narrow char array
    char ch[256];
    char def_char = '.';
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, ch, 256, &def_char, NULL);

    return std::string(ch);
}

// Convert std::string to std::wstring.
template <typename S>
inline std::wstring StringToWideString(S&& s)
{
    std::wstring temp(s.length(), L' ');
    std::copy(s.begin(), s.end(), temp.begin());
    return temp;
}

inline DevicePtrBackend<BackendType::kDx12>* device_ptr_cast(DevicePtrBase* ptr)
{
    return dynamic_cast<DevicePtrBackend<BackendType::kDx12>*>(ptr);
}
inline CommandStreamBackend<BackendType::kDx12>* command_stream_cast(CommandStreamBase* stream)
{
    return dynamic_cast<CommandStreamBackend<BackendType::kDx12>*>(stream);
}

// Round divide.
template <typename T>
inline T CeilDivide(T val, T div)
{
    return (val + div - 1) / div;
}

template <typename T>
inline T Align(T val, T a)
{
    return (val + (a - 1)) & ~(a - 1);
}

// Allocate UAV buffer of a specified size.
ID3D12Resource* AllocateUAVBuffer(ID3D12Device*         device,
                                  size_t                size,
                                  D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COMMON);

// Allocate upload buffer of a specified type.
ID3D12Resource* AllocateUploadBuffer(ID3D12Device* device, size_t size, void* data = nullptr);

struct TraceKey
{
    RRIntersectQuery       query;
    RRIntersectQueryOutput query_output;
    bool                   indirect;
    bool                   operator==(const TraceKey& other) const
    {
        return (query == other.query && query_output == other.query_output && indirect == other.indirect);
    }
};
struct KeyHasher
{
    size_t operator()(const TraceKey& k) const
    {
        using std::hash;
        constexpr size_t kNumber = 17;
        return kNumber * kNumber * k.query + kNumber * k.query_output + uint32_t(k.indirect);
    }
};

}  // namespace rt::dx