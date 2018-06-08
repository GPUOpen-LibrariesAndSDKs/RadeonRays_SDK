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

#if USE_OPENCL
#include "calc_clw.h"
#include "device_clw.h"
#include "except_clw.h"

#include "calc_clw_common.h"

namespace Calc
{
    CalcClw::CalcClw()
    {
        try
        {
            CLWPlatform::CreateAllPlatforms(m_platforms);

            // Collect all devices
            for (auto const& platform : m_platforms)
            {
                auto num_devices = platform.GetDeviceCount();

                for (auto i = 0U; i < num_devices; ++i)
                {
                    auto device = platform.GetDevice(i);
                    m_devices.push_back(device);
                }
            }
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    // Enumerate devices 
    std::uint32_t CalcClw::GetDeviceCount() const
    {
        return static_cast<std::uint32_t>(m_devices.size());
    }

    // Get i-th device spec
    void CalcClw::GetDeviceSpec(std::uint32_t idx, DeviceSpec& spec) const
    {
        if (idx >= m_devices.size())
        {
            throw ExceptionClw("Index is out of bounds");
        }

        // Strings leave until the destructor of this object is called
        // which is pretty much ok
        spec.name = m_devices[idx].GetName().c_str();
        spec.vendor = m_devices[idx].GetVendor().c_str();
        spec.type = Convert2CalcDeviceType(m_devices[idx].GetType());
        spec.global_mem_size = m_devices[idx].GetGlobalMemSize();
        spec.local_mem_size = m_devices[idx].GetLocalMemSize();
        spec.min_alignment = m_devices[idx].GetMinAlignSize();
        spec.max_alloc_size = m_devices[idx].GetMaxAllocSize();
        spec.max_local_size = m_devices[idx].GetMaxWorkGroupSize();
    }

    // Create the device with specified index
    Device* CalcClw::CreateDevice(std::uint32_t idx) const
    {
        if (idx >= m_devices.size())
        {
            throw ExceptionClw("Index is out of bounds");
        }

        try
        {
            return new DeviceClw(m_devices[idx]);
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }
    
    DeviceCl* CalcClw::CreateDevice(cl_context context, cl_device_id device, cl_command_queue queue) const
    {
        try
        {
            auto clcontext = CLWContext::Create(context, &device, &queue, 1);
            auto cldev = clcontext.GetDevice(0);
            
            return new DeviceClw(cldev, clcontext);
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    // Delete the device
    void CalcClw::DeleteDevice(Device* device)
    {
        delete device;
    }

}

Calc::DeviceCl* CreateDeviceFromOpenCL(cl_context context, cl_device_id device, cl_command_queue queue)
{
    CLWContext clwContext = CLWContext::Create(context, &device, &queue, 1);
    auto clwDevice = clwContext.GetDevice(0);
    return new Calc::DeviceClw(clwDevice, clwContext);
}

#endif //use_opencl