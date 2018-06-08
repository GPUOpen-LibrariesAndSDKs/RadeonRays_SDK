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
#pragma once

#include "calc_cl.h"
#include "calc.h"
#include "CLW.h"
#include <vector>

namespace Calc
{
    // Implementation of Calc interface using CLW library
    class CalcClw : public Calc
    {
    public:
        CalcClw();
        ~CalcClw() = default;

        // Enumerate devices 
        std::uint32_t GetDeviceCount() const override;

        // Get i-th device spec
        void GetDeviceSpec(std::uint32_t idx, DeviceSpec& spec) const override;

        // Create the device with specified index
        Device* CreateDevice(std::uint32_t idx) const override;
        
        // Create the device from specified OpenCL device
        DeviceCl* CreateDevice(cl_context context, cl_device_id device, cl_command_queue queue) const;

        // Delete the device
        void DeleteDevice(Device* device) override;

        Platform GetPlatform() final override { return Platform::kOpenCL; };


    private:
        std::vector<CLWPlatform> m_platforms;
        std::vector<CLWDevice> m_devices;
    };
}

