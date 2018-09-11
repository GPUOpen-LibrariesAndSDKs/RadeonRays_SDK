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
#ifndef __CLW__CLWPlatform_h__
#define __CLW__CLWPlatform_h__

#include <vector>
#include <string>
#include <memory>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "ReferenceCounter.h"

class CLWDevice;

// Represents OpenCL platform
// Create CLWPlatfrom with CLWPlatform::Create function
class CLWPlatform : public ReferenceCounter<cl_platform_id, nullptr, nullptr>
{
public:
    // Create platform based on platform_id passed
    // CLWInvalidID is thrown if the id is not a valid OpenCL platform ID
    static CLWPlatform Create(cl_platform_id id, cl_device_type type = CL_DEVICE_TYPE_ALL);
    static void CreateAllPlatforms(std::vector<CLWPlatform>& platforms);
    
    virtual ~CLWPlatform() = default;
    
    // Platform info
    std::string GetName() const;
    std::string GetProfile() const;
    std::string GetVersion() const;
    std::string GetVendor()  const;
    std::string GetExtensions() const;
    
    // Get number of devices
    unsigned int                GetDeviceCount() const;
    // Get idx-th device
    CLWDevice  GetDevice(unsigned int idx) const;
    
private:
    CLWPlatform(cl_platform_id id, cl_device_type type);
    void GetPlatformInfoParameter(cl_platform_id id, cl_platform_info param, std::string& result);
    void InitDeviceList(cl_device_type type) const;

    std::string name_;
    std::string profile_;
    std::string version_;
    std::string vendor_;
    std::string extensions_;

    mutable std::vector<CLWDevice> devices_;
    cl_device_type type_;
};


#endif
