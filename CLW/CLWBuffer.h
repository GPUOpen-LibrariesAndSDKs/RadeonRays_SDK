/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

    CLWBuffer(){}
    virtual ~CLWBuffer();
    
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

    size_t elementCount_;
    
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

template <typename T> CLWBuffer<T>::~CLWBuffer()
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
