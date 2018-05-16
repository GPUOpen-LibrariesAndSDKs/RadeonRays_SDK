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
#ifndef __CLW__CLWContext__
#define __CLW__CLWContext__

#include <iostream>
#include <memory>
#include <vector>
#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "ReferenceCounter.h"
#include "ParameterHolder.h"
#include "CLWBuffer.h"
#include "CLWImage2D.h"
#include "CLWEvent.h"
#include "CLWDevice.h"

class CLWProgram;
class CLWCommandQueue;

class CLWContext : public ReferenceCounter<cl_context, clRetainContext, clReleaseContext>
{
public:

    static CLWContext Create(std::vector<CLWDevice> const&, cl_context_properties* props = nullptr);
    static CLWContext Create(cl_context context, cl_device_id* device, cl_command_queue* commandQueues, int numDevices);
    static CLWContext Create(CLWDevice device, cl_context_properties* props = nullptr);

    CLWContext() = default;
    virtual ~CLWContext() = default;

    unsigned int GetDeviceCount() const;
    CLWDevice GetDevice(unsigned int idx) const;
    CLWProgram CreateProgram(std::vector<char> const& sourceCode, char const* buildopts = nullptr) const;

    template <typename T> CLWBuffer<T>  CreateBuffer(size_t elementCount, cl_mem_flags flags) const;
    template <typename T> CLWBuffer<T>  CreateBuffer(size_t elementCount, cl_mem_flags flags ,void* data) const;

    CLWImage2D                  CreateImage2DFromGLTexture(cl_GLint texture) const;

    template <typename T> CLWEvent  WriteBuffer(unsigned int idx, CLWBuffer<T> buffer, T const* hostBuffer, size_t elemCount) const;
    template <typename T> CLWEvent  WriteBuffer(unsigned int idx, CLWBuffer<T> buffer, T const* hostBuffer, size_t offset, size_t elemCount) const;
    template <typename T> CLWEvent  FillBuffer(unsigned int idx, CLWBuffer<T> buffer, T const& val, size_t elemCount) const;
    template <typename T> CLWEvent  ReadBuffer(unsigned int idx,  CLWBuffer<T> buffer, T* hostBuffer, size_t elemCount) const;
    template <typename T> CLWEvent  ReadBuffer(unsigned int idx,  CLWBuffer<T> buffer, T* hostBuffer, size_t offset, size_t elemCount) const;
    template <typename T> CLWEvent  CopyBuffer(unsigned int idx,  CLWBuffer<T> source, CLWBuffer<T> dest, size_t srcOffset, size_t destOffset, size_t elemCount) const;
    template <typename T> CLWEvent  MapBuffer(unsigned int idx,  CLWBuffer<T> buffer, cl_map_flags flags, T** mappedData) const;
    template <typename T> CLWEvent  MapBuffer(unsigned int idx,  CLWBuffer<T> buffer, cl_map_flags flags, size_t offset, size_t elemCount, T** mappedData) const;
    template <typename T> CLWEvent  UnmapBuffer(unsigned int idx,  CLWBuffer<T> buffer, T* mappedData) const;

    template <size_t globalSize, size_t localSize, typename ... Types> CLWEvent Launch1D(unsigned int idx, cl_kernel kernel, Types ... args);
    template <size_t globalSize, size_t localSize, typename ... Types> CLWEvent Launch1D(unsigned int idx, cl_kernel kernel, CLWEvent depEvent, Types ... args);
    template <size_t globalSize, size_t localSize, typename ... Types> CLWEvent Launch1D(unsigned int idx, cl_kernel kernel, std::vector<CLWEvent> const& events, Types ... args);

    CLWEvent Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel);
    CLWEvent Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel, CLWEvent depEvent);
    CLWEvent Launch1D(unsigned int idx, size_t globalSize, size_t localSize, cl_kernel kernel, std::vector<CLWEvent> const& events);

    CLWEvent Launch2D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel);
    CLWEvent Launch2D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, CLWEvent depEvent);
    CLWEvent Launch2D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, std::vector<CLWEvent> const& events);

    CLWEvent Launch3D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel);
    CLWEvent Launch3D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, CLWEvent depEvent);
    CLWEvent Launch3D(unsigned int idx, size_t* globalSize, size_t* localSize, cl_kernel kernel, std::vector<CLWEvent> const& events);

    void Finish(unsigned int idx) const;
    void Flush(unsigned int idx) const;

    // GL interop 
    void AcquireGLObjects(unsigned int idx, std::vector<cl_mem> const& objects) const;
    void ReleaseGLObjects(unsigned int idx, std::vector<cl_mem> const& objects) const;

    CLWCommandQueue GetCommandQueue(unsigned int idx) const { return commandQueues_[idx]; }

private:
    void InitCL();

    CLWContext(cl_context context, std::vector<CLWDevice> const&);
    CLWContext(cl_context context, std::vector<CLWDevice> const&, std::vector<CLWCommandQueue> const&);
    CLWContext(cl_context context, std::vector<CLWDevice>&&, std::vector<CLWCommandQueue>&&);
    CLWContext(cl_context context, std::vector<CLWDevice>&&);

    CLWContext(CLWDevice device);

    std::vector<CLWDevice>       devices_;
    std::vector<CLWCommandQueue> commandQueues_;
};

template <typename T> CLWBuffer<T> CLWContext::CreateBuffer(size_t elementCount, cl_mem_flags flags) const
{
    return CLWBuffer<T>::Create(*this, flags, elementCount);
}

template <typename T> CLWBuffer<T> CLWContext::CreateBuffer(size_t elementCount, cl_mem_flags flags, void* data) const
{
    return CLWBuffer<T>::Create(*this, flags, elementCount, data);
}

template <typename T> CLWEvent  CLWContext::WriteBuffer(unsigned int idx, CLWBuffer<T> buffer, T const* hostBuffer, size_t elemCount) const
{
    return buffer.WriteDeviceBuffer(commandQueues_[idx], hostBuffer, elemCount);
}

template <typename T> CLWEvent  CLWContext::WriteBuffer(unsigned int idx, CLWBuffer<T> buffer, T const* hostBuffer, size_t offset, size_t elemCount) const
{
    return buffer.WriteDeviceBuffer(commandQueues_[idx], hostBuffer, offset, elemCount);
}

template <typename T> CLWEvent  CLWContext::FillBuffer(unsigned int idx, CLWBuffer<T> buffer, T const& val, size_t elemCount) const
{
    return buffer.FillDeviceBuffer(commandQueues_[idx], val, elemCount);
}

template <typename T> CLWEvent  CLWContext::ReadBuffer(unsigned int idx, CLWBuffer<T> buffer, T* hostBuffer, size_t elemCount) const
{
    return buffer.ReadDeviceBuffer(commandQueues_[idx], hostBuffer, elemCount);
}

template <typename T> CLWEvent  CLWContext::ReadBuffer(unsigned int idx, CLWBuffer<T> buffer, T* hostBuffer, size_t offset, size_t elemCount) const
{
    return buffer.ReadDeviceBuffer(commandQueues_[idx], hostBuffer, offset, elemCount);
}

template <size_t globalSize, size_t localSize, typename ... Types> CLWEvent CLWContext::Launch1D(unsigned int idx, cl_kernel kernel, Types ... args)
{
    std::vector<ParameterHolder> params{args...};
    for (unsigned int i = 0; i < params.size(); ++i)
    {
        params[i].SetArg(kernel, i);
    }

    return Launch1D(idx, globalSize, localSize, kernel);
}

template <size_t globalSize, size_t localSize, typename ... Types> CLWEvent CLWContext::Launch1D(unsigned int idx, cl_kernel kernel, CLWEvent depEvent, Types ... args)
{
    std::vector<ParameterHolder> params{ args... };
    for (unsigned int i = 0; i < params.size(); ++i)
    {
        params[i].SetArg(kernel, i);
    }

    return Launch1D(idx, globalSize, localSize, kernel, depEvent);
}

template <size_t globalSize, size_t localSize, typename ... Types> CLWEvent CLWContext::Launch1D(unsigned int idx, cl_kernel kernel, std::vector<CLWEvent> const& events, Types ... args)
{
    std::vector<ParameterHolder> params{ args... };
    for (unsigned int i = 0; i < params.size(); ++i)
    {
        params[i].SetArg(kernel, i);
    }

    return Launch1D(idx, globalSize, localSize, kernel, events);
}

template <typename T> CLWEvent  CLWContext::CopyBuffer(unsigned int idx, CLWBuffer<T> source, CLWBuffer<T> dest, size_t srcOffset, size_t destOffset, size_t elemCount) const
{
    cl_int status = CL_SUCCESS;
    cl_event event;

    clEnqueueCopyBuffer(commandQueues_[idx], source, dest, srcOffset * sizeof(T), destOffset* sizeof(T), elemCount * sizeof(T), 0, nullptr, &event); 
    ThrowIf(status != CL_SUCCESS, status, "clEnqueueCopyBuffer failed");

    return CLWEvent::Create(event);
}

template <typename T> CLWEvent  CLWContext::MapBuffer(unsigned int idx,  CLWBuffer<T> buffer, cl_map_flags flags, T** mappedData) const
{
    return buffer.MapDeviceBuffer(commandQueues_[idx], flags, mappedData);
}

template <typename T> CLWEvent  CLWContext::MapBuffer(unsigned int idx,  CLWBuffer<T> buffer, cl_map_flags flags, size_t offset, size_t elemCount, T** mappedData) const
{
    return buffer.MapDeviceBuffer(commandQueues_[idx], flags, offset, elemCount, mappedData);
}

template <typename T> CLWEvent  CLWContext::UnmapBuffer(unsigned int idx,  CLWBuffer<T> buffer, T* mappedData) const
{
    return buffer.UnmapDeviceBuffer(commandQueues_[idx], mappedData);
}


#endif /* defined(__CLW__CLWContext__) */
