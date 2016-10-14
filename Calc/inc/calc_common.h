/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

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

#ifndef CALC_STATIC_LIBRARY
#ifdef WIN32
    #ifdef CALC_EXPORT_API
        #define CALC_API __declspec(dllexport)
    #else
        #define CALC_API __declspec(dllimport)
    #endif
#elif defined(__GNUC__)
    #ifdef CALC_EXPORT_API
        #define CALC_API __attribute__((visibility ("default")))
    #else
        #define CALC_API
    #endif
#endif
#else
#define CALC_API
#endif

#include <cstdint>
#include <type_traits>

namespace Calc
{
    enum BufferType
    {
        kRead = 0x1,
        kWrite = 0x2,
        kPinned = 0x4
    };

    enum MapType
    {
        kMapRead = 0x1,
        kMapWrite = 0x2
    };

    enum class DeviceType : std::uint8_t
    {
        kUnknown,
        kGpu,
        kCpu,
        kAccelerator
    };

    enum Platform
    {
        kOpenCL            = (1 << 0),
        kVulkan            = (1 << 1),

        kAny            = 0xFF
    };

    enum class SourceType : std::uint8_t
    {
        kOpenCL            = (1 << 0),
        kGLSL            = (1 << 1),
        kCL_SPIRV        = (1 << 2),
        kGLSL_SPIRV        = (1 << 3),

        kHostNative        = (1 << 4),
        kAccelNative    = (1 << 5),
    };

    inline SourceType operator | (const SourceType lhs, const SourceType rhs)
    {
        using T = std::underlying_type<SourceType>::type;
        return static_cast<SourceType>(static_cast<T>(lhs) | static_cast<T>(rhs));
    }

    inline SourceType operator & (const SourceType lhs, const SourceType rhs)
    {
        using T = std::underlying_type<SourceType>::type;
        return static_cast<SourceType>(static_cast<T>(lhs) & static_cast<T>(rhs));
    }

}
