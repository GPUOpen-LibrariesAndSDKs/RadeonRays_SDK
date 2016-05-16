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
#include "CLWEvent.h"
#include "CLWExcept.h"

CLWEvent CLWEvent::Create(cl_event event)
{
    CLWEvent e(event);
    clReleaseEvent(event);
    return e;
}

CLWEvent::CLWEvent(cl_event event)
: ReferenceCounter<cl_event, clRetainEvent, clReleaseEvent>(event)
{
}

void CLWEvent::Wait()
{
    cl_event event = *this;
    cl_int status = clWaitForEvents(1, &event);
    ThrowIf(status != CL_SUCCESS, status, "clWaitForEvents failed");
}

CLWEvent::~CLWEvent()
{
}

float CLWEvent::GetDuration() const
{
    cl_ulong commandStart, commandEnd;
    cl_int status = CL_SUCCESS;

    status = clGetEventProfilingInfo(*this, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &commandStart, nullptr);

    ThrowIf(status != CL_SUCCESS, status, "clGetEventProfilingInfo failed");

    status = clGetEventProfilingInfo(*this, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &commandEnd, nullptr);

    ThrowIf(status != CL_SUCCESS, status, "clGetEventProfilingInfo failed");

    return (float)(commandEnd - commandStart) / 1000000.f;
}

cl_int CLWEvent::GetCommandExecutionStatus() const
{
    cl_int status, execstatus;

    status = clGetEventInfo(*this, CL_EVENT_COMMAND_EXECUTION_STATUS, sizeof(cl_int), &execstatus, nullptr);

    ThrowIf(status != CL_SUCCESS, status, "clGetEventInfo failed");

    return execstatus;
}