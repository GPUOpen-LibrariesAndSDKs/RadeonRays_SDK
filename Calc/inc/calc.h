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

#include <string>
#include <cstdint>

#include "calc_common.h"

namespace Calc
{

    struct DeviceSpec;
    class Device;

    // Base api interface for device enumeration and creation
    //  * Can provide device specifications
    //  * Can create and delete specified devices
    //
    class CALC_API Calc
    {
    public:
        Calc() = default;
        virtual ~Calc() = default;

        // Enumerate devices
        virtual std::uint32_t GetDeviceCount() const = 0;

        // Get i-th device spec
        virtual void GetDeviceSpec(std::uint32_t idx, DeviceSpec& spec) const = 0;

        // Create the device with specified index
        virtual Device* CreateDevice(std::uint32_t idx) const = 0;

        // Delete the device
        virtual void DeleteDevice(Device* device) = 0;

        // return the platform used
        virtual Platform GetPlatform() = 0;

        // Forbidden stuff
        Calc(Calc const&) = delete;
        Calc& operator = (Calc const&) = delete;
    };
}

// Create corresponding calc
#ifdef __cplusplus
extern "C"
{
#endif
    CALC_API Calc::Calc* CreateCalc(Calc::Platform inPlatform, int reserved);
    CALC_API void DeleteCalc(Calc::Calc* calc);
#ifdef __cplusplus
}
#endif
