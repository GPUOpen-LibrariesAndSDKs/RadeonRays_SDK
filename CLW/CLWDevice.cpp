//
//  CLWDevice.cpp
//  CLW
//
//  Created by dmitryk on 01.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

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
    GetDeviceInfoParameter(*this, CL_DEVICE_VERSION, version_);
    GetDeviceInfoParameter(*this, CL_DEVICE_PROFILE, profile_);
    GetDeviceInfoParameter(*this, CL_DEVICE_TYPE, type_);
    
    GetDeviceInfoParameter(*this, CL_DEVICE_MAX_WORK_GROUP_SIZE, maxWorkGroupSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_GLOBAL_MEM_SIZE, globalMemSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_LOCAL_MEM_SIZE, localMemSize_);
    GetDeviceInfoParameter(*this, CL_DEVICE_LOCAL_MEM_TYPE, localMemType_);
    GetDeviceInfoParameter(*this, CL_DEVICE_MAX_MEM_ALLOC_SIZE, maxAllocSize_);
	GetDeviceInfoParameter(*this, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE, minAlignSize_);
}

CLWDevice::~CLWDevice()
{
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


