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
    
    CLWDevice() = default;
    virtual ~CLWDevice() = default;

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
    bool         HasGlInterop() const;

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
    cl_uint                     minAlignSize_;
    
    friend class CLWPlatform;
};

template <typename T> inline void CLWDevice::GetDeviceInfoParameter(cl_device_id id, cl_device_info param, T& value)
{
    cl_int status = clGetDeviceInfo(id, param, sizeof(T), &value, nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetDeviceInfo failed");
}

template <> void CLWDevice::GetDeviceInfoParameter<std::string>(cl_device_id id, cl_device_info param, std::string& value);


#endif
