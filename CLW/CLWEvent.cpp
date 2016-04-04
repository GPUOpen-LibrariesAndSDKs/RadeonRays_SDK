//
//  CLWEvent.cpp
//  CLW
//
//  Created by dmitryk on 26.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

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