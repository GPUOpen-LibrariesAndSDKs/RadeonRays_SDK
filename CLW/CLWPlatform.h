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
    
    virtual ~CLWPlatform();
    
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
