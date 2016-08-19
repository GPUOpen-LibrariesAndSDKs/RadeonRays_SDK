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

#include "CLW.h"
#include "calc_common.h"

namespace Calc
{
    inline cl_mem_flags Convert2ClCreationFlags(std::uint32_t flags)
    {
        // TODO: implement correctly
        return CL_MEM_READ_WRITE;
    }

    inline cl_mem_flags Convert2ClMapFlags(std::uint32_t flags)
    {
        cl_mem_flags res = 0;

        if (flags & kMapRead)
            res |= CL_MAP_READ;

        if (flags & kMapWrite)
            res |= CL_MAP_WRITE;

        return res;
    }

    inline DeviceType Convert2CalcDeviceType(cl_device_type type)
    {
        DeviceType res = DeviceType::kUnknown;
        switch (type)
        {
        case CL_DEVICE_TYPE_CPU:
            res = DeviceType::kCpu;
            break;
        case CL_DEVICE_TYPE_GPU:
            res = DeviceType::kGpu;
            break;
        case CL_DEVICE_TYPE_ACCELERATOR:
            res = DeviceType::kAccelerator;
            break;
        }

        return res;
    }
}