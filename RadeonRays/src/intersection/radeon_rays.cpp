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
#include "radeon_rays.h"
#include "radeon_rays_impl.h"

#include "radeon_rays_cl.h"


#include <memory>

#include "calc.h"
#include "calc_cl.h"

#include "device.h"
#include "device_cl.h"

#include "../except/except.h"
#include "../device/intersection_device.h"
#include "../device/calc_intersection_device.h"
#include "../device/calc_intersection_device_cl.h"

#ifdef USE_EMBREE
    #include "../device/embree_intersection_device.h"
#endif //USE_EMBREE

namespace RadeonRays
{
    // Device manager to query and build configurations
    static Calc::Calc* GetCalc()
    {
        static Calc::Calc* s_calc = nullptr;
        if (!s_calc)
        {
            s_calc = Calc::CreateCalc(0);
        }

        return s_calc;
    }

    std::uint32_t IntersectionApi::GetDeviceCount()
    {
        std::uint32_t result = GetCalc()->GetDeviceCount();
#ifdef USE_EMBREE
        ++result; // last one - embree device
#endif //USE_EMBREE
        return result;
    }

    void IntersectionApi::GetDeviceInfo(std::uint32_t devidx, DeviceInfo& devinfo)
    {
#ifdef USE_EMBREE
        if (devidx == GetCalc()->GetDeviceCount())
        {
            devinfo.name = "embree";
            devinfo.vendor = "intel";
            devinfo.type = DeviceInfo::kCpu;
        }
        else
#endif //USE_EMBREE
        {
            Calc::DeviceSpec spec;
            GetCalc()->GetDeviceSpec(devidx, spec);

            // TODO: careful with memory management of strings
            devinfo.name = spec.name;
            devinfo.vendor = spec.vendor;
            devinfo.type = spec.type == Calc::DeviceType::kGpu ? DeviceInfo::kGpu : DeviceInfo::kCpu;
        }
    }

    IntersectionApi* IntersectionApi::Create(std::uint32_t devidx)
    {
            IntersectionApi* result = nullptr;
#ifdef USE_EMBREE
        if (devidx == GetCalc()->GetDeviceCount())
            result = new IntersectionApiImpl(new EmbreeIntersectionDevice());
        else
#endif //USE_EMBREE
            result = new IntersectionApiImpl(new CalcIntersectionDevice(GetCalc(), GetCalc()->CreateDevice(devidx)));
        return result;
    }

    // Deallocation (to simplify DLL scenario)
    void IntersectionApi::Delete(IntersectionApi* api)
    {
        delete api;
    }

    IntersectionApiCL* IntersectionApiCL::CreateFromOpenClContext(cl_context context, cl_device_id device, cl_command_queue queue)
    {
        auto calc = dynamic_cast<Calc::CalcCl*>(GetCalc());
        
        if (calc)
        {
            return new IntersectionApiImpl(new CalcIntersectionDeviceCl(calc, calc->CreateDevice(context, device, queue)));
        }

        
        return nullptr;

    }
}


