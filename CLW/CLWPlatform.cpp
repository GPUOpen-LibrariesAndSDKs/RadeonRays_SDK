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
#include "CLWPlatform.h"
#include "CLWDevice.h"
#include "CLWExcept.h"

#include <cassert>
#include <algorithm>
#include <functional>
#include <iterator>

// Create platform based on platform_id passed
// CLWInvalidID is thrown if the id is not a valid OpenCL platform ID
CLWPlatform CLWPlatform::Create(cl_platform_id id, cl_device_type type)
{
    return CLWPlatform(id, type);
}

void CLWPlatform::CreateAllPlatforms(std::vector<CLWPlatform>& platforms)
{
    cl_int status = CL_SUCCESS;
    cl_uint numPlatforms;
    status = clGetPlatformIDs(0, nullptr, &numPlatforms);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformIDs failed");

    std::vector<cl_platform_id> platformIds(numPlatforms);
    std::vector<cl_platform_id> validIds;

    status = clGetPlatformIDs(numPlatforms, &platformIds[0], nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformIDs failed");


#ifdef RR_ALLOW_CPU_DEVICES
	cl_device_type type = CL_DEVICE_TYPE_ALL;
#else
	cl_device_type type = CL_DEVICE_TYPE_GPU;
#endif

    // TODO: this is a workaround for nasty Apple's OpenCL runtime
    // which doesn't allow to have work group sizes > 1 on CPU devices
    // so disable useless CPU
#ifdef __APPLE__
    type = CL_DEVICE_TYPE_GPU;
#endif

    platforms.clear();
    // Only use CL1.2+ platforms
    for (int i = 0; i < (int)platformIds.size(); ++i)
    {
        size_t size = 0;
        status = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VERSION, 0, nullptr, &size);

        std::vector<char> version(size);

        status = clGetPlatformInfo(platformIds[i], CL_PLATFORM_VERSION, size, &version[0], 0);

        std::string versionstr(version.begin(), version.end());

        if (versionstr.find("OpenCL 1.0 ") != std::string::npos ||
            versionstr.find("OpenCL 1.1") != std::string::npos)
        {
            continue;
        }

        validIds.push_back(platformIds[i]);
    }

    std::transform(validIds.cbegin(), validIds.cend(), std::back_inserter(platforms),
        std::bind(&CLWPlatform::Create, std::placeholders::_1, type));
}

CLWPlatform::~CLWPlatform()
{
}

void CLWPlatform::GetPlatformInfoParameter(cl_platform_id id, cl_platform_info param, std::string& result)
{
    size_t length = 0;

    cl_int status = clGetPlatformInfo(id, param, 0, nullptr, &length);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformInfo failed");

    std::vector<char> buffer(length);
    status = clGetPlatformInfo(id, param, length, &buffer[0], nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetPlatformInfo failed");

    result = &buffer[0];
}

CLWPlatform::CLWPlatform(cl_platform_id id, cl_device_type type)
: ReferenceCounter<cl_platform_id, nullptr, nullptr>(id)
, type_(type)
{
    GetPlatformInfoParameter(*this, CL_PLATFORM_NAME, name_);
    GetPlatformInfoParameter(*this, CL_PLATFORM_PROFILE, profile_);
    GetPlatformInfoParameter(*this, CL_PLATFORM_VENDOR, vendor_);
    GetPlatformInfoParameter(*this, CL_PLATFORM_VERSION, version_);
}

// Platform info
std::string CLWPlatform::GetName() const
{
    return name_;
}

std::string CLWPlatform::GetProfile() const
{
    return profile_;
}

std::string CLWPlatform::GetVersion() const
{
    return version_;
}

std::string CLWPlatform::GetVendor()  const
{
    return vendor_;
}

std::string CLWPlatform::GetExtensions() const
{
    return extensions_;
}

// Get number of devices
unsigned int                CLWPlatform::GetDeviceCount() const
{
    if (devices_.size() == 0)
    {
        InitDeviceList(type_);
    }

    return (unsigned int)devices_.size();
}

// Get idx-th device
CLWDevice CLWPlatform::GetDevice(unsigned int idx) const
{
    if (devices_.size() == 0)
    {
        InitDeviceList(type_);
    }

    return devices_[idx];
}

void CLWPlatform::InitDeviceList(cl_device_type type) const
{
    cl_uint numDevices = 0;
    cl_int status = clGetDeviceIDs(*this, type, 0, nullptr, &numDevices);

    if (status == CL_DEVICE_NOT_FOUND)
    {
        return;
    }

    ThrowIf(status != CL_SUCCESS, status, "clGetDeviceIDs failed");

    std::vector<cl_device_id> deviceIds(numDevices);
    status = clGetDeviceIDs(*this, type, numDevices, &deviceIds[0], nullptr);
    ThrowIf(status != CL_SUCCESS, status, "clGetDeviceIDs failed");

    for (cl_uint i = 0; i < numDevices; ++i)
    {
        devices_.push_back(CLWDevice(deviceIds[i]));
    }
}
