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

    CLWContext(){}
    virtual                     ~CLWContext();

    unsigned int                GetDeviceCount() const;
    CLWDevice                   GetDevice(unsigned int idx) const;
    CLWProgram                  CreateProgram(std::vector<char> const& sourceCode) const;

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
