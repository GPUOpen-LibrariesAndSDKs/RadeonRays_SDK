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
#include <cassert>

#if USE_OPENCL
#include "../device/calc_intersection_device_cl.h"
#endif

#if USE_VULKAN
#include "../device/calc_intersection_device_vk.h"
#include "calc_vk.h"
#endif

#ifdef USE_EMBREE
    #include "../device/embree_intersection_device.h"
#endif //USE_EMBREE

#ifndef CALC_STATIC_LIBRARY

#ifdef WIN32
// Windows
#define NOMINMAX
#include <Windows.h>
#define LOADLIBRARY(name) LoadLibrary(name)
#define GETFUNC GetProcAddress
#define HANDLE_TYPE HMODULE

#ifndef _DEBUG
#define LIBNAME "Calc64.dll"
#define LONGNAME "../Bin/Release/x64/##LIBNAME"
#else
#define LIBNAME "Calc64D.dll"
#define LONGNAME "../Bin/Debug/x64/##LIBNAME"
#endif
#elif __linux__
// Linux
#include <dlfcn.h>
#define LOADLIBRARY(name) dlopen(name, RTLD_LAZY)
#define GETFUNC dlsym
#define HANDLE_TYPE void*

#ifndef _DEBUG
#define LIBNAME "libCalc64.so"
#define LONGNAME "../Bin/Release/x64/##LIBNAME"
#else
#define LIBNAME "libCalc64D.so"
#define LONGNAME "../Bin/Debug/x64/##LIBNAME"
#endif
#else
// MacOS
#include <dlfcn.h>
#define LOADLIBRARY(name) dlopen(name, RTLD_LAZY)
#define GETFUNC dlsym
#define HANDLE_TYPE void*

#ifndef _DEBUG
#define LIBNAME "libCalc64.dylib"
#define LONGNAME "../Bin/Release/x64/##LIBNAME"
#else
#define LIBNAME "libCalc64D.dylib"
#define LONGNAME "../Bin/Debug/x64/##LIBNAME"
#endif
#endif
#endif

namespace RadeonRays
{
    static RadeonRays::DeviceInfo::Platform s_calc_platform = RadeonRays::DeviceInfo::Platform::kAny;

#ifndef CALC_STATIC_LIBRARY
    static void* GetCalcEntryPoint(Calc::Platform platform, char const* name)
    {
        HANDLE_TYPE hdll = LOADLIBRARY(LIBNAME);

        if (!hdll)
        {
            hdll = LOADLIBRARY(LONGNAME);
        }

        if (!hdll)
        {
            return nullptr;
        }

        return GETFUNC(hdll, name);
    }
#endif

#define GetCalc_impl(platform)                                                        \
    static Calc::Calc* GetCalc##platform()                                            \
    {                                                                                \
        static Calc::Calc* s_calc##platform = nullptr;                                \
        if (!s_calc##platform)                                                        \
        {                                                                            \
            s_calc##platform = CreateCalc(Calc::Platform::k##platform, 0);    \
        }                                                                            \
        return s_calc##platform;                                                    \
    }

    //GetCalc_impl(OpenCL)

    GetCalc_impl(Vulkan)
#undef GetCalc_impl

    static Calc::Calc* GetCalcOpenCL()
    {
        static Calc::Calc* s_calcOpenCL = nullptr;

        if (!s_calcOpenCL)
        {
#ifndef CALC_STATIC_LIBRARY
            auto pfn_create_calc = GetCalcEntryPoint(Calc::Platform::kOpenCL, "CreateCalc");

            if (pfn_create_calc)
            {
                auto create_calc = reinterpret_cast<decltype(CreateCalc)*>(pfn_create_calc);
                s_calcOpenCL = create_calc(Calc::Platform::kOpenCL, 0);
            }
#else
            s_calcOpenCL = CreateCalc(Calc::Platform::kOpenCL, 0);
#endif
        }

        return s_calcOpenCL;
    }

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
        }
        // embree is always the last device
#ifdef USE_EMBREE
        if (s_calc_platform & DeviceInfo::Platform::kEmbree)
        {
            ++result;
        }
#endif //USE_EMBREE

        return result;
    }
    static bool IsDeviceIndexEmbree(uint32_t devidx)
    {

#ifdef USE_EMBREE
        auto* calc = GetCalc();
        if (calc != nullptr)
        {
            if (devidx == GetCalc()->GetDeviceCount())
            {
                return true;
            }
        }
        else if(devidx == 0)
        {
            return true;
        }
#endif //USE_EMBREE
        return false;
    }

    void IntersectionApi::GetDeviceInfo(std::uint32_t devidx, DeviceInfo& devinfo)
    {

        auto* calc = GetCalc();

        if (IsDeviceIndexEmbree(devidx))
        {
#ifdef USE_EMBREE
            devinfo.name = "embree";
            devinfo.vendor = "intel";
            devinfo.type = DeviceInfo::kCpu;
            devinfo.platform = DeviceInfo::kEmbree;
#else
            assert(false);
#endif //USE_EMBREE
            return;
        }
        assert(calc);

        Calc::DeviceSpec spec;
        calc->GetDeviceSpec(devidx, spec);

        // TODO: careful with memory management of strings
        devinfo.name = spec.name;
        devinfo.vendor = spec.vendor;
        devinfo.type = spec.type == Calc::DeviceType::kGpu ? DeviceInfo::kGpu : DeviceInfo::kCpu;
    }

    IntersectionApi* IntersectionApi::Create(std::uint32_t devidx)
    {
        if (IsDeviceIndexEmbree(devidx))
        {
#ifdef USE_EMBREE
            return new IntersectionApiImpl(new EmbreeIntersectionDevice());
#endif //USE_EMBREE
        }
        else
        {
            auto* calc = GetCalc();
            if (calc != nullptr)
            {
                return new IntersectionApiImpl(new CalcIntersectionDevice(calc, calc->CreateDevice(devidx)));
            }
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
    RRAPI IntersectionApi* CreateFromOpenClContext(cl_context        context, cl_device_id device, cl_command_queue queue)
    {
        auto calc = dynamic_cast<Calc::Calc*>(GetCalcOpenCL());

        if (calc)
        {
            Calc::DeviceCl* calc_device = nullptr;

#ifndef CALC_STATIC_LIBRARY
            auto pfn_create_device_from_cl = GetCalcEntryPoint(Calc::Platform::kOpenCL, "CreateDeviceFromOpenCL");

            if (pfn_create_device_from_cl)
            {
                auto create_device_from_cl = reinterpret_cast<decltype(CreateDeviceFromOpenCL)*>(pfn_create_device_from_cl);
                calc_device = create_device_from_cl(context, device, queue);
            }
#else
            calc_device = CreateDeviceFromOpenCL(context, device, queue);
#endif
            if (calc_device)
            {
                return new IntersectionApiImpl(new CalcIntersectionDeviceCl(calc, calc_device));
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            return nullptr;
        }
    }
#endif
}
