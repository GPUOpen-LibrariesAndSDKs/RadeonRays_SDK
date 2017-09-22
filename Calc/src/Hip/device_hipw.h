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

#include <vector>

#include "calc_common.h"
#include "device.h"

namespace Calc
{
    class DeviceHip : public DeviceHip
    {
    public:

        // Buffer mapping
        // Calls are blocking if passed nullptr for an event, otherwise use Event to sync
        virtual void MapBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, std::uint32_t map_type, void** mapdata, Event** e);
        virtual void UnmapBuffer(Buffer const* buffer, std::uint32_t queue, void* mapdata, Event** e);

        // Kernel compilation
        virtual Executable* CompileExecutable(char const* source_code, std::size_t size, char const* options);
        virtual Executable* CompileExecutable(std::uint8_t const* binary_code, std::size_t size, char const*  options);
        virtual Executable* CompileExecutable(char const* filename,
                                              char const** headernames,
                                              int numheaders, char const* options);

        virtual void DeleteExecutable(Executable* executable);

        // Executable management
        virtual size_t GetExecutableBinarySize(Executable const* executable) const;
        virtual void GetExecutableBinary(Executable const* executable, std::uint8_t* binary) const;

        // Execution
        // Calls are blocking if passed nullptr for an event, otherwise use Event to sync
        virtual void Execute(Function const* func, std::uint32_t queue, size_t global_size, size_t local_size, Event** e);

        // Events handling
        virtual void WaitForEvent(Event* e);
        virtual void WaitForMultipleEvents(Event** e, std::size_t num_events);
        virtual void DeleteEvent(Event* e);

        // Queue management functions
        virtual void Flush(std::uint32_t queue);
        virtual void Finish(std::uint32_t queue);

        // Parallel prims handling
        virtual bool HasBuiltinPrimitives() const;
        virtual Primitives* CreatePrimitives() const;
        virtual void DeletePrimitives(Primitives* prims);
    private:
    };
}