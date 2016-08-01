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

#include "calc.h"
#include "device.h"

#include "../device/calc_intersection_device.h"

#if USE_VULKAN
#	include "../device/calc_intersection_device_vk.h"
#	include <calc_vk.h>
#endif

#ifdef USE_EMBREE
    #include "../device/embree_intersection_device.h"
#endif //USE_EMBREE

namespace RadeonRays
{
	static RadeonRays::DeviceInfo::Platform s_calc_platform = RadeonRays::DeviceInfo::Platform::kAny;

#define GetCalc_impl(platform)														\
    static Calc::Calc* GetCalc##platform()											\
    {																				\
        static Calc::Calc* s_calc##platform = nullptr;								\
        if (!s_calc##platform)														\
        {																			\
            s_calc##platform = Calc::CreateCalc(Calc::Platform::k##platform, 0);	\
        }																			\
        return s_calc##platform;													\
    }

	GetCalc_impl(OpenCL)

	GetCalc_impl(Vulkan)
#undef GetCalc_impl

	static Calc::Calc* GetCalc()
	{
		// if CL allowed see if we have any devices if not
		// try Vulkan if allowed, if neither try embree if allowed
#if USE_OPENCL
		if( s_calc_platform & DeviceInfo::Platform::kOpenCL )
		{
			auto* calc = GetCalcOpenCL();
			if (calc != nullptr) { return calc; }
		}
#endif
#if USE_VULKAN
		if ( s_calc_platform & DeviceInfo::Platform::kVulkan )
		{
			auto* calc = GetCalcVulkan();
			if (calc != nullptr) { return calc; }
		}
#endif
		return nullptr;
	}
	void IntersectionApi::SetPlatform(const DeviceInfo::Platform platform)
	{
		s_calc_platform = platform;
	}

    std::uint32_t IntersectionApi::GetDeviceCount()
    {
		auto* calc = GetCalc();
		std::uint32_t result = 0;
		if (calc != nullptr)
		{
			result += GetCalc()->GetDeviceCount();
		} else 
		{
#ifdef USE_EMBREE
	        ++result;
#endif //USE_EMBREE
		}
        return result;
    }

    void IntersectionApi::GetDeviceInfo(std::uint32_t devidx, DeviceInfo& devinfo)
    {
		auto* calc = GetCalc();
		std::uint32_t result = 0;
		if (calc != nullptr)
		{
			Calc::DeviceSpec spec;
			calc->GetDeviceSpec(devidx, spec);

			// TODO: careful with memory management of strings
			devinfo.name = spec.name;
			devinfo.vendor = spec.vendor;
			devinfo.type = spec.type == Calc::DeviceType::kGpu ? DeviceInfo::kGpu : DeviceInfo::kCpu;
		}
		else
		{
#ifdef USE_EMBREE
			if (devidx == 0) 
			{
				devinfo.name = "embree";
				devinfo.vendor = "intel";
				devinfo.type = DeviceInfo::kCpu;
				devinfo.platform = kEmbree;
			}
			else
			{
				assert(false);
			}
#endif //USE_EMBREE
		}
    }

    IntersectionApi* IntersectionApi::Create(std::uint32_t devidx)
    {
		auto* calc = GetCalc();
		if (calc != nullptr)
		{
			return new IntersectionApiImpl(new CalcIntersectionDevice(calc, calc->CreateDevice(devidx)));
		}
		else
		{
#ifdef USE_EMBREE
			if (devidx == 0)
			{
				return new IntersectionApiImpl(new EmbreeIntersectionDevice());
			}
#endif
		}

        return nullptr;
    }

    // Deallocation (to simplify DLL scenario)
    void IntersectionApi::Delete(IntersectionApi* api)
    {
        delete api;
    }
#ifdef USE_VULKAN
	RRAPI IntersectionApi* CreateFromVulkan(Anvil::Device* device, Anvil::CommandPool* cmd_pool)
	{
		auto calc = dynamic_cast<Calc::Calc*>(GetCalcVulkan());
		if (calc)
		{
			return new IntersectionApiImpl(new CalcIntersectionDeviceVK(calc, Calc::CreateDeviceFromVulkan(device, cmd_pool)));
		}
		else
		{
			return nullptr;	
		}
	}
#endif
#ifdef USE_OPENCL
    RRAPI IntersectionApi* CreateFromOpenClContext(cl_context		context, cl_device_id device, cl_command_queue queue)
    {
        auto calc = dynamic_cast<Calc::Calc*>(GetCalcOpenCL());
		if (calc)
        {
            return new IntersectionApiImpl(new CalcIntersectionDeviceCl(calc, Calc::CreateDeviceFromOpenCL(context, device, queue)));
		}
		else
		{
			return nullptr;
		}
    }
#endif

}


