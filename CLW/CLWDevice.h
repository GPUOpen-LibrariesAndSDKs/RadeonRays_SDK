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
#ifndef __CLW__CLWDevice_h__
#define __CLW__CLWDevice_h__

#include <memory>
#include <cassert>
#include <vector>
#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "ReferenceCounter.h"
#include "CLWExcept.h"

class CLWPlatform;

class CLWDevice : public ReferenceCounter<cl_device_id, clRetainDevice, clReleaseDevice>
{
public:
    static CLWDevice Create(cl_device_id id);
    
    CLWDevice(){}
    virtual ~CLWDevice();

    // Device info
    cl_ulong GetLocalMemSize() const;
    cl_ulong GetGlobalMemSize() const;
    cl_ulong GetMaxAllocSize() const;
    size_t   GetMaxWorkGroupSize() const;
    cl_device_type GetType() const;
    cl_device_id GetID() const;
	cl_uint GetMinAlignSize() const;

    // ... GetExecutionCapabilties() const;
    std::string const& GetName() const;
    std::string const& GetVendor() const;
    std::string const& GetVersion() const;
    std::string const& GetProfile() const;
    std::string const& GetExtensions() const;

	//
	bool		 HasGlInterop() const;

    // unsigned int GetGlobalMemCacheSize() const;
    // ...
    // ...

private:
    template <typename T> void GetDeviceInfoParameter(cl_device_id id, cl_device_info param, T& value);

    //CLWDevice(CLWDevice const&);
    //CLWDevice& operator = (CLWDevice const&);
    CLWDevice(cl_device_id id);
    
    std::string              name_;
    std::string              vendor_;
    std::string              version_;
    std::string              profile_;
    std::string              extensions_;
    cl_device_type           type_;
    
    cl_ulong                 localMemSize_;
    cl_ulong                 globalMemSize_;
    cl_ulong                 maxAllocSize_;
    cl_device_local_mem_type localMemType_;
    size_t                   maxWorkGroupSize_;
	cl_uint					 minAlignSize_;
    
    friend class CLWPlatform;
};

template <typename T> inline void CLWDevice::GetDeviceInfoParameter(cl_device_id id, cl_device_info param, T& value)
{
    cl_int status = clGetDeviceInfo(id, param, sizeof(T), &value, nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetDeviceInfo failed");
}

template <> void CLWDevice::GetDeviceInfoParameter<std::string>(cl_device_id id, cl_device_info param, std::string& value);


#endif
