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

#ifndef __CLW__CLWBuffer__
#define __CLW__CLWBuffer__

#include <iostream>
#include <memory>
#include <vector>
#include <cassert>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "ReferenceCounter.h"
#include "ParameterHolder.h"
#include "CLWCommandQueue.h"
#include "CLWEvent.h"
#include "CLWExcept.h"


template <typename T> class CLWBuffer : public ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>
{
public:
    static CLWBuffer<T> Create(cl_context context, cl_mem_flags flags, size_t elementCount);
    static CLWBuffer<T> Create(cl_context context, cl_mem_flags flags, size_t elementCount, void* data);
    static CLWBuffer<T> CreateFromClBuffer(cl_mem buffer);

    CLWBuffer() = default;
    virtual ~CLWBuffer() = default;
    
    size_t GetElementCount() const { return elementCount_; }
    
    operator ParameterHolder() const
    {
        return ParameterHolder((cl_mem)*this);
    }
    
private:
    CLWEvent WriteDeviceBuffer(CLWCommandQueue cmdQueue, T const* hostBuffer, size_t elemCount);
    CLWEvent WriteDeviceBuffer(CLWCommandQueue cmdQueue, T const* hostBuffer, size_t offset, size_t elemCount);
    CLWEvent FillDeviceBuffer(CLWCommandQueue cmdQueue, T const& val, size_t elemCount);
    CLWEvent ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t elemCount);
    CLWEvent ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t offset, size_t elemCount);
    CLWEvent MapDeviceBuffer(CLWCommandQueue cmdQueue, cl_map_flags flags, T** mappedData);
    CLWEvent MapDeviceBuffer(CLWCommandQueue cmdQueue, cl_map_flags flags, size_t offset, size_t elemCount, T** mappedData);
    CLWEvent UnmapDeviceBuffer(CLWCommandQueue cmdQueue, T* mappedData);

    CLWBuffer(cl_mem buffer, size_t elementCount);

    size_t elementCount_ = 0;
    
    friend class CLWContext;
};

template <typename T> CLWBuffer<T> CLWBuffer<T>::Create(cl_context context, cl_mem_flags flags, size_t elementCount)
{
    cl_int status = CL_SUCCESS;
    cl_mem deviceBuffer = clCreateBuffer(context, flags, elementCount * sizeof(T), nullptr, &status);

    ThrowIf(status != CL_SUCCESS, status, "clCreateBuffer failed");

    CLWBuffer<T> buffer(deviceBuffer, elementCount);

    clReleaseMemObject(deviceBuffer);

    return buffer;
}

template <typename T> CLWBuffer<T> CLWBuffer<T>::Create(cl_context context, cl_mem_flags flags, size_t elementCount, void* data)
{
    cl_int status = CL_SUCCESS;
    cl_mem deviceBuffer = clCreateBuffer(context, flags | CL_MEM_COPY_HOST_PTR, elementCount * sizeof(T), data, &status);

    ThrowIf(status != CL_SUCCESS, status, "clCreateBuffer failed");
    
    CLWBuffer<T> buffer(deviceBuffer, elementCount);

    clReleaseMemObject(deviceBuffer);

    return buffer;
}

template <typename T> CLWBuffer<T> CLWBuffer<T>::CreateFromClBuffer(cl_mem buffer)
{
    cl_int status = CL_SUCCESS;
    
    // Request the size
    size_t bufferSize = 0;
    status = clGetMemObjectInfo(buffer, CL_MEM_SIZE, sizeof(bufferSize), &bufferSize, nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetMemObjectInfo failed");
    
    return CLWBuffer(buffer, bufferSize / sizeof(T));
}

template <typename T> CLWBuffer<T>::CLWBuffer(cl_mem buffer, size_t elementCount)
: ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>(buffer)
, elementCount_(elementCount)
{
   
}

template <typename T> CLWEvent CLWBuffer<T>::WriteDeviceBuffer(CLWCommandQueue cmdQueue, T const* hostBuffer, size_t elemCount)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    status = clEnqueueWriteBuffer(cmdQueue, *this, false, 0, sizeof(T)*elemCount, hostBuffer, 0, nullptr, &event);
    
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueWriteBuffer failed");
    
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::WriteDeviceBuffer(CLWCommandQueue cmdQueue, T const* hostBuffer, size_t offset, size_t elemCount)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    status = clEnqueueWriteBuffer(cmdQueue, *this, false, sizeof(T)*offset, sizeof(T)*elemCount, hostBuffer, 0, nullptr, &event);

    ThrowIf(status != CL_SUCCESS, status, "clEnqueueWriteBuffer failed");

    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t elemCount)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    status = clEnqueueReadBuffer(cmdQueue, *this, false, 0, sizeof(T)*elemCount, hostBuffer, 0, nullptr, &event);
    
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueReadBuffer failed");
    
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::ReadDeviceBuffer(CLWCommandQueue cmdQueue, T* hostBuffer, size_t offset, size_t elemCount)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    status = clEnqueueReadBuffer(cmdQueue, *this, false, sizeof(T)*offset, sizeof(T)*elemCount, hostBuffer, 0, nullptr, &event);
    
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueReadBuffer failed");
    
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::MapDeviceBuffer(CLWCommandQueue cmdQueue, cl_map_flags flags, T** mappedData)
{
    assert(mappedData);

    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;

    T* data = (T*)clEnqueueMapBuffer(cmdQueue, *this, false, flags, 0, sizeof(T)*elementCount_, 0, nullptr, &event, &status);

    ThrowIf(status != CL_SUCCESS, status, "clEnqueueMapBuffer failed");

    *mappedData = data;

    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::MapDeviceBuffer(CLWCommandQueue cmdQueue, cl_map_flags flags, size_t offset, size_t elemCount, T** mappedData)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    
    T* data = (T*)clEnqueueMapBuffer(cmdQueue, *this, false, flags, sizeof(T) * offset, sizeof(T)*elemCount, 0, nullptr, &event, &status);
    
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueMapBuffer failed");
    
    *mappedData = data;
    
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::UnmapDeviceBuffer(CLWCommandQueue cmdQueue, T* mappedData)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;

    status = clEnqueueUnmapMemObject(cmdQueue, *this,  mappedData, 0, nullptr, &event);

    ThrowIf(status != CL_SUCCESS, status, "clEnqueueUnmapMemObject failed");
    
    return CLWEvent::Create(event);
}

template <typename T> CLWEvent CLWBuffer<T>::FillDeviceBuffer(CLWCommandQueue cmdQueue, T const& val, size_t elemCount)
{
    cl_int status = CL_SUCCESS;
    cl_event event = nullptr;
    status = clEnqueueFillBuffer(cmdQueue, *this, &val, sizeof(T), 0, sizeof(T)*elemCount, 0, nullptr, &event);
    
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueFillBuffer failed");
    
    return CLWEvent::Create(event);
}


#endif /* defined(__CLW__CLWBuffer__) */
