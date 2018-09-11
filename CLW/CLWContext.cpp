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
#include "CLWContext.h"
#include "CLWCommandQueue.h"
#include "CLWDevice.h"
#include "CLWProgram.h"
#include "CLWPlatform.h"
#include "CLWExcept.h"

#include <algorithm>

CLWContext CLWContext::Create(std::vector<CLWDevice> const& devices, cl_context_properties* props)
{
    std::vector<cl_device_id> deviceIds;
    std::for_each(devices.cbegin(), devices.cend(),
                  [&deviceIds](CLWDevice const& device)
                  {
                      deviceIds.push_back(device);
                  });
    cl_int status = CL_SUCCESS;
    cl_context ctx = clCreateContext(props, static_cast<cl_int>(deviceIds.size()), &deviceIds[0], nullptr, nullptr, &status);
    ThrowIf(status != CL_SUCCESS, status, "clCreateContext failed");
    
    CLWContext context(ctx, devices);
    
    clReleaseContext(ctx);
    
    return context;
}

CLWContext CLWContext::Create(cl_context context, cl_device_id* device, cl_command_queue* commandQueues, int numDevices)
{
    std::vector<CLWDevice> devices(numDevices);
    std::vector<CLWCommandQueue> cmdQueues(numDevices);

    for (int i=0; i<numDevices; ++i)
    {
        devices[i] = CLWDevice::Create(device[i]);
        cmdQueues[i] = CLWCommandQueue::Create(commandQueues[i]);
    }

    return CLWContext(context, devices, cmdQueues);
}

CLWEvent CLWContext::Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel)
{
    cl_int status = CL_SUCCESS;
    size_t wgLocalSize = localSize;
    size_t wgGlobalSize = globalSize;
    cl_event event = nullptr;

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 1, nullptr, &wgGlobalSize, &wgLocalSize, 0, nullptr, &event);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueNDRangeKernel failed");

    return CLWEvent::Create(event);
}


CLWEvent CLWContext::Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel, CLWEvent depEvent)
{
    cl_int status = CL_SUCCESS;
    size_t wgLocalSize = localSize;
    size_t wgGlobalSize = globalSize;
    cl_event event = nullptr;
    cl_event eventToWait = depEvent;

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 1, nullptr, &wgGlobalSize, &wgLocalSize, 1, &eventToWait, &event);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueNDRangeKernel failed");

    return CLWEvent::Create(event);
}


CLWEvent CLWContext::Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel, std::vector<CLWEvent> const& events)
{
    cl_int status = CL_SUCCESS;
    size_t wgLocalSize = localSize;
    size_t wgGlobalSize = globalSize;
    cl_event event = nullptr;
    std::vector<cl_event> eventsToWait(events.size());

    for (unsigned i = 0; i < events.size(); ++i)
        eventsToWait[i] = events[i];

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 2, nullptr, &wgGlobalSize, &wgLocalSize, (cl_uint)eventsToWait.size(), &eventsToWait[0], &event);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueNDRangeKernel failed");

    return CLWEvent::Create(event);
}

CLWEvent CLWContext::Launch2D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 2, nullptr, globalSize, localSize, 0, nullptr, &event);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueNDRangeKernel failed");

    return CLWEvent::Create(event);
}

CLWEvent CLWContext::Launch2D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, CLWEvent depEvent)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    cl_event eventToWait = depEvent;

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 2, nullptr, globalSize, localSize, 1, &eventToWait, &event);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueNDRangeKernel failed");

    return CLWEvent::Create(event);
}

CLWEvent CLWContext::Launch2D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, std::vector<CLWEvent> const& events)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;

    std::vector<cl_event> eventsToWait(events.size());

    for (unsigned i = 0; i < events.size(); ++i)
        eventsToWait[i] = events[i];

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 2, nullptr, globalSize, localSize, (cl_uint)eventsToWait.size(), &eventsToWait[0], &event);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueNDRangeKernel failed");

    return CLWEvent::Create(event);
}


CLWEvent CLWContext::Launch3D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 3, nullptr, globalSize, localSize, 0, nullptr, &event);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueNDRangeKernel failed");

    return CLWEvent::Create(event);
}

CLWEvent CLWContext::Launch3D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, CLWEvent depEvent)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    cl_event eventToWait = depEvent;

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 3, nullptr, globalSize, localSize, 1, &eventToWait, &event);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueNDRangeKernel failed");

    return CLWEvent::Create(event);
}

CLWEvent CLWContext::Launch3D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, std::vector<CLWEvent> const& events)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;

    std::vector<cl_event> eventsToWait(events.size());

    for (unsigned i = 0; i < events.size(); ++i)
        eventsToWait[i] = events[i];

    status = clEnqueueNDRangeKernel(commandQueues_[idx], kernel, 3, nullptr, globalSize, localSize, (cl_uint)eventsToWait.size(), &eventsToWait[0], &event);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueNDRangeKernel failed");

    return CLWEvent::Create(event);
}

CLWContext CLWContext::Create(CLWDevice device, cl_context_properties* props)
{
    std::vector<CLWDevice> devices;
    devices.push_back(device);
    return CLWContext::Create(devices, props);
}

CLWContext::CLWContext(cl_context context, std::vector<CLWDevice> const& devices)
: ReferenceCounter<cl_context, clRetainContext, clReleaseContext>(context)
, devices_(devices)
{
    InitCL();
}

CLWContext::CLWContext(cl_context context, std::vector<CLWDevice>&& devices)
: ReferenceCounter<cl_context, clRetainContext, clReleaseContext>(context)
, devices_(devices)
{
    InitCL();
}

CLWContext::CLWContext(cl_context context, std::vector<CLWDevice> const& devices, std::vector<CLWCommandQueue> const& commandQueues)
: ReferenceCounter<cl_context, clRetainContext, clReleaseContext>(context)
, devices_(devices)
, commandQueues_(commandQueues)
{
    
}

CLWContext::CLWContext(cl_context context, std::vector<CLWDevice>&& devices, std::vector<CLWCommandQueue>&& commandQueues)
: ReferenceCounter<cl_context, clRetainContext, clReleaseContext>(context)
, devices_(devices)
, commandQueues_(commandQueues)
{
    
}

CLWContext::CLWContext(CLWDevice device)
{
    devices_.push_back(device);
    InitCL();
}

unsigned int CLWContext::GetDeviceCount() const
{
    return (unsigned int)devices_.size();
}

CLWDevice CLWContext::GetDevice(unsigned int idx) const
{
    return devices_[idx];
}

void CLWContext::InitCL()
{
    std::for_each(devices_.begin(), devices_.end(),
                  [this](CLWDevice const& device)
                  {
                      commandQueues_.push_back(CLWCommandQueue::Create(device, *this));
                  });
}

CLWProgram CLWContext::CreateProgram(std::vector<char> const& sourceCode, char const* buildopts) const
{
    return CLWProgram::CreateFromSource(&sourceCode[0], sourceCode.size(), buildopts, *this);
}

CLWImage2D CLWContext::CreateImage2DFromGLTexture(cl_GLint texture) const
{
    return CLWImage2D::CreateFromGLTexture(*this, texture);
}

void CLWContext::AcquireGLObjects(unsigned int idx, std::vector<cl_mem> const& objects) const
{
    cl_int status = clEnqueueAcquireGLObjects(commandQueues_[idx], (cl_uint)objects.size(), &objects[0], 0, nullptr, nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueAcquireGLObjects failed");
}

void CLWContext::ReleaseGLObjects(unsigned int idx, std::vector<cl_mem> const& objects) const
{
    cl_int status = clEnqueueReleaseGLObjects(commandQueues_[idx], (cl_uint)objects.size(), &objects[0], 0, nullptr, nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueReleaseGLObjects failed");
}

void CLWContext::Finish(unsigned int idx) const
{
    cl_int status = clFinish(commandQueues_[idx]);
    ThrowIf(status != CL_SUCCESS, status, "clFinish failed");
}

void CLWContext::Flush(unsigned int idx) const
{
    cl_int status = clFlush(commandQueues_[idx]);
    ThrowIf(status != CL_SUCCESS, status, "clFlush failed");
}
