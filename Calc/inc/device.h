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

#include <cstdint>
#include <cstddef>
#include <string>

#include "calc_common.h"

namespace Calc
{
    class Buffer;
    class Executable;
    class Function;
    class Event;
    class Primitives;

    // Calc device specification
    struct DeviceSpec
    {
        char const* name;
        char const* vendor;

        DeviceType  type;
        SourceType  sourceTypes;

        std::uint32_t min_alignment;
        std::uint32_t max_num_queues;

        unsigned long long global_mem_size;
        unsigned long long local_mem_size;
        unsigned long long max_alloc_size;
        std::size_t max_local_size;

        bool has_fp16;
    };

    // Main interface to control compute device
    //    * Can create buffers
    //    * Move data from system memory and back
    //  * Launch kernels
    //  * Control synchronization events
    //
    class CALC_API Device
    {
    public:
        Device() = default;
        virtual ~Device() = default;

        // Return specification of the device
        virtual void GetSpec(DeviceSpec& spec) = 0;
        virtual Platform GetPlatform() const = 0;

        // Buffer creation and deletion
        virtual Buffer* CreateBuffer(std::size_t size, std::uint32_t flags) = 0;
        // Create buffer having initial data
        virtual Buffer* CreateBuffer(std::size_t size, std::uint32_t flags, void* initdata) = 0;
        virtual void DeleteBuffer(Buffer* buffer) = 0;

        // Data movement
        // Calls are blocking if passed nullptr for an event, otherwise use Event to sync
        virtual void ReadBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* dst, Event** e) const = 0;
        virtual void WriteBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* src, Event** e) = 0;

        // Buffer mapping
        // Calls are blocking if passed nullptr for an event, otherwise use Event to sync
        virtual void MapBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, std::uint32_t map_type, void** mapdata, Event** e) = 0;
        virtual void UnmapBuffer(Buffer const* buffer, std::uint32_t queue, void* mapdata, Event** e) = 0;

        // Kernel compilation
        virtual Executable* CompileExecutable(char const* source_code, std::size_t size, char const* options) = 0;
        virtual Executable* CompileExecutable(std::uint8_t const* binary_code, std::size_t size, char const*  options) = 0;
        virtual Executable* CompileExecutable(char const* filename,
                                              char const** headernames,
                                              int numheaders, char const* options) = 0;

        virtual void DeleteExecutable(Executable* executable) = 0;

        // Executable management
        virtual size_t GetExecutableBinarySize(Executable const* executable) const = 0;
        virtual void GetExecutableBinary(Executable const* executable, std::uint8_t* binary) const = 0;

        // Execution
        // Calls are blocking if passed nullptr for an event, otherwise use Event to sync
        virtual void Execute(Function const* func, std::uint32_t queue, size_t global_size, size_t local_size, Event** e) = 0;

        // Events handling
        virtual void WaitForEvent(Event* e) = 0;
        virtual void WaitForMultipleEvents(Event** e, std::size_t num_events) = 0;
        virtual void DeleteEvent(Event* e) = 0;

        // Queue management functions
        virtual void Flush(std::uint32_t queue) = 0;
        virtual void Finish(std::uint32_t queue) = 0;

        // Parallel prims handling
        virtual bool HasBuiltinPrimitives() const = 0;
        virtual Primitives* CreatePrimitives() const = 0;
        virtual void DeletePrimitives(Primitives* prims) = 0;


        // Helper methods
        template <typename T> void ReadTypedBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, T* dst, Event** e) const;
        template <typename T> void WriteTypedBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, T* src, Event** e);
        template <typename T> void MapTypedBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, std::uint32_t map_type, T** mapdata, Event** e);

        // Forbidden stuff
        Device(Device const&) = delete;
        Device& operator = (Device const&) = delete;
    };


    template <typename T>
    inline void Device::ReadTypedBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, T* dst, Event** e) const
    {
        ReadBuffer(buffer, queue, offset * sizeof(T), size * sizeof(T), dst, e);
    }

    template <typename T>
    inline void Device::WriteTypedBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, T* src, Event** e)
    {
        WriteBuffer(buffer, queue, offset * sizeof(T), size * sizeof(T), src, e);
    }

    template <typename T>
    inline void Device::MapTypedBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, std::uint32_t map_type, T** mapdata, Event** e)
    {
        MapBuffer(buffer, queue, offset * sizeof(T), size * sizeof(T), map_type, reinterpret_cast<void**>(mapdata), e);
    }
}
