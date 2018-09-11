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
#include "CLWDevice.h"
#include "CLWExcept.h"

CLWDevice CLWDevice::Create(cl_device_id id)
{
    return CLWDevice(id);
}

CLWDevice::CLWDevice(cl_device_id id) : ReferenceCounter<cl_device_id, clRetainDevice, clReleaseDevice>(id)
{
    GetDeviceInfoParameter(*this, CL_DEVICE_NAME, name_);
    GetDeviceInfoParameter(*this, CL_DEVICE_EXTENSIONS, extensions_);
    GetDeviceInfoParameter(*this, CL_DEVICE_VENDOR, vendor_);
    GetDeviceInfoParameter(*this, CL_DRIVER_VERSION, version_);
    GetDeviceInfoParameter(*this, CL_DEVICE_PROFILE, profile_);
    GetDeviceInfoParameter(*this, CL_DEVICE_TYPE, type_);
    
    GetDeviceInfoParameter(*this, CL_DEVICE_MAX_WORK_GROUP_SIZE, maxWorkGroupSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_GLOBAL_MEM_SIZE, globalMemSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_LOCAL_MEM_SIZE, localMemSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_LOCAL_MEM_TYPE, localMemType_);
    GetDeviceInfoParameter(*this, CL_DEVICE_MAX_MEM_ALLOC_SIZE, maxAllocSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, minAlignSize_);
}

template <> void CLWDevice::GetDeviceInfoParameter<std::string>(cl_device_id id, cl_device_info param, std::string& value)
{
    size_t length = 0;

    cl_int status = clGetDeviceInfo(id, param, 0, nullptr, &length);
    ThrowIf(status != CL_SUCCESS, status, "clGetDeviceInfo failed");

    std::vector<char> buffer(length);
    clGetDeviceInfo(id, param, length, &buffer[0], nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetDeviceInfo failed");

    value = &buffer[0];
}

std::string const& CLWDevice::GetName() const
{
    return name_;
}

std::string const& CLWDevice::GetExtensions() const
{
    return extensions_;
}

std::string const& CLWDevice::GetVersion() const
{
    return version_;
}

std::string const& CLWDevice::GetProfile() const
{
    return profile_;
}

std::string const& CLWDevice::GetVendor() const
{
    return vendor_;
}

cl_device_type CLWDevice::GetType() const
{
    return type_;
}

// Device info
cl_ulong CLWDevice::GetLocalMemSize() const
{
    return localMemSize_;
}

cl_ulong CLWDevice::GetGlobalMemSize() const
{
    return globalMemSize_;
}

size_t   CLWDevice::GetMaxWorkGroupSize() const
{
    return maxWorkGroupSize_;
}

cl_device_id CLWDevice::GetID() const
{
    return *this;
}

cl_ulong CLWDevice::GetMaxAllocSize() const
{
    return maxAllocSize_;
}

cl_uint CLWDevice::GetMinAlignSize() const
{
    return minAlignSize_;
}

bool CLWDevice::HasGlInterop() const
{
    return extensions_.find("cl_khr_gl_sharing") != std::string::npos
    || extensions_.find("cl_APPLE_gl_sharing") != std::string::npos;
}


