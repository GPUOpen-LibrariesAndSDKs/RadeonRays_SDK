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

#include "calc_common.h"

#include <cstddef>
#include <cstdint>

namespace Calc
{
    class Buffer;

    struct SharedMemory
    {
    };

    // Represents individual callable unit within an executable
    class CALC_API Function
    {
    public:
        Function() = default;
        virtual ~Function() = default;

        // Argument setters
        virtual void SetArg(std::uint32_t idx, std::size_t arg_size, void* arg) = 0;
        virtual void SetArg(std::uint32_t idx, Buffer const* arg) = 0;
        virtual void SetArg(std::uint32_t idx, std::size_t size, SharedMemory shmem) = 0;

        Function(Function const&) = delete;
        Function& operator = (Function const&) = delete;
    };

    // Executable which is capable of being launched on a device
    class CALC_API Executable
    {
    public:
        Executable() = default;
        virtual ~Executable() = default;

        // Function management
        virtual Function* CreateFunction(char const* name) = 0;
        virtual void DeleteFunction(Function* func) = 0;

        Executable(Executable const&) = delete;
        Executable& operator = (Executable const&) = delete;
    };

}
