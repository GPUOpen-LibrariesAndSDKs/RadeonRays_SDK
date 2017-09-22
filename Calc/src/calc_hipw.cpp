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
#include "calc_hipw.h"
#include "device_hip.h"
#include "except_clw.h"
#include "device.h"
#include "hip/hip_runtime.h"

#define CHECK_HIP_CMD(err) if (err != hipSuccess) {throw ExceptionClw("Hip failed");}

namespace Calc
{

    CalcHipw::CalcHipw()
    {

    }

    CalcHipw::~CalcHipw()
    {

    }

    std::uint32_t CalcHipw::GetDeviceCount() const
    {
        int res = 0;
        hipError_t err = hipGetDeviceCount(&res);
        CHECK_HIP_CMD(err);
        return res;
    }

    void CalcHipw::GetDeviceSpec(std::uint32_t idx, DeviceSpec& spec) const
    {
        //get hip devbice properties and copy to device spec
        hipDeviceProp_t props;
        hipError_t err = hipGetDeviceProperties(&props, idx);
        CHECK_HIP_CMD(err);

        // TODO: fill spec by valid values
        spec.name = "hip device";
        spec.type = DeviceType::kGpu;
        spec.vendor = "";
    }

    Device* CalcHipw::CreateDevice(std::uint32_t idx) const
    {
        return new DeviceHip();
    }

    void CalcHipw::DeleteDevice(Device* device)
    {
        delete device;
    }
} //Calc

