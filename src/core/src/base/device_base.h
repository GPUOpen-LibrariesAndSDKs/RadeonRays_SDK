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

#include <memory>
#include <vector>

#include "backend.h"

namespace rt
{
/// Forward decls.
class EventBase;
class CommandStreamBase;
class DevicePtrBase;
class IntersectorBase;

/**
 * @brief Base class for all raytracing devices.
 *
 * Provides an interface for command stream allocation and submission.
 **/
class DeviceBase
{
public:
    DeviceBase()                  = default;
    DeviceBase(const DeviceBase&) = delete;
    DeviceBase& operator=(const DeviceBase&) = delete;

    virtual ~DeviceBase() = default;

    /**
     * @brief Allocate a command stream.
     *
     * This operation does not usually involve any system memory allocations and is relatively lightweight.
     *
     * @return Allocated command stream.
     **/
    virtual CommandStreamBase* AllocateCommandStream() = 0;

    /**
     * @brief Release a command stream.
     *
     * If the command stream is still executing on GPU, deferred deletion happens.
     *
     * @param command_stream_base Command stream to release.
     **/
    virtual void ReleaseCommandStream(CommandStreamBase* command_stream_base) = 0;

    /**
     * @brief Submit a command stream.
     *
     * Waits for wait_event and signals returned event upon completion.
     *
     * @param command_stream_base Command stream to submit.
     * @param wait_event_base Event to wait for before submitting (or in the submission queue).
     * @return Event marking a submission.
     **/
    virtual EventBase* SubmitCommandStream(CommandStreamBase* command_stream_base, EventBase* wait_event_base) = 0;

    /**
     * @brief Releases an event.
     *
     * Release an event allocated by a submission. This call is relatively lightweight and do not usually trigger any
     * memory allocations / deallocations.
     *
     * @param event_base Event to release.
     **/
    virtual void ReleaseEvent(EventBase* event_base) = 0;

    /**
     * @brief Wait for an event.
     *
     * Block on CPU until the event has signaled.
     **/
    virtual void WaitEvent(EventBase* event_base) = 0;
};


template <BackendType type>
class DeviceBackend;

}  // namespace rt