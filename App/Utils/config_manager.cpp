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
#include "config_manager.h"

#include "CLW.h"
#include "Renderers/PT/ptrenderer.h"
#include "Renderers/BDPT/bdptrenderer.h"

#ifndef APP_BENCHMARK

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include "GL/glew.h"
#else
#include <CL/cl.h>
#include <GL/glew.h>
#include <GL/glx.h>
#endif

void ConfigManager::CreateConfigs(Mode mode, bool interop, std::vector<Config>& configs, int initial_num_bounces)
{
    std::vector<CLWPlatform> platforms;

    CLWPlatform::CreateAllPlatforms(platforms);

    if (platforms.size() == 0)
    {
        throw std::runtime_error("No OpenCL platforms installed.");
    }

    configs.clear();

    bool hasprimary = false;
    for (int i = 0; i < platforms.size(); ++i)
    {
        std::vector<CLWDevice> devices;

        for (int d = 0; d < (int)platforms[i].GetDeviceCount(); ++d)
        {
            if ((mode == kUseGpus || mode == kUseSingleGpu) && platforms[i].GetDevice(d).GetType() != CL_DEVICE_TYPE_GPU)
                continue;

            if ((mode == kUseCpus || mode == kUseSingleCpu) && platforms[i].GetDevice(d).GetType() != CL_DEVICE_TYPE_CPU)
                continue;

            Config cfg;
            cfg.caninterop = false;
#ifdef WIN32
            if (platforms[i].GetDevice(d).HasGlInterop() && !hasprimary && interop)
            {
                cl_context_properties props[] =
                {
                    //OpenCL platform
                    CL_CONTEXT_PLATFORM, (cl_context_properties)((cl_platform_id)platforms[i]),
                    //OpenGL context
                    CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
                    //HDC used to create the OpenGL context
                    CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
                    0
                };

                cfg.context = CLWContext::Create(platforms[i].GetDevice(d), props);
                devices.push_back(platforms[i].GetDevice(d));
                cfg.devidx = 0;
                cfg.type = kPrimary;
                cfg.caninterop = true;
                hasprimary = true;
            }
            else
#elif __linux__

            if (platforms[i].GetDevice(d).HasGlInterop() && !hasprimary && interop)
            {
                cl_context_properties props[] =
                {
                    //OpenCL platform
                    CL_CONTEXT_PLATFORM, (cl_context_properties)((cl_platform_id)platforms[i]),
                    //OpenGL context
                    CL_GL_CONTEXT_KHR, (cl_context_properties)glXGetCurrentContext(),
                    //HDC used to create the OpenGL context
                    CL_GLX_DISPLAY_KHR, (cl_context_properties)glXGetCurrentDisplay(),
                    0
                };

                cfg.context = CLWContext::Create(platforms[i].GetDevice(d), props);
                devices.push_back(platforms[i].GetDevice(d));
                cfg.devidx = 0;
                cfg.type = kPrimary;
                cfg.caninterop = true;
                hasprimary = true;
            }
            else
#elif __APPLE__
                if (platforms[i].GetDevice(d).HasGlInterop() && !hasprimary && interop)
                {
                    CGLContextObj kCGLContext = CGLGetCurrentContext();
                    CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);
                    // Create CL context properties, add handle & share-group enum !
                    cl_context_properties props[] = {
                    CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
                    (cl_context_properties)kCGLShareGroup, 0
                    };

                    cfg.context = CLWContext::Create(platforms[i].GetDevice(d), props);
                    devices.push_back(platforms[i].GetDevice(d));
                    cfg.devidx = 0;
                    cfg.type = kPrimary;
                    cfg.caninterop = true;
                    hasprimary = true;
                }
                else
#endif
            {
                cfg.context = CLWContext::Create(platforms[i].GetDevice(d));
                cfg.devidx = 0;
                cfg.type = kSecondary;
            }

            configs.push_back(cfg);

            if (mode == kUseSingleGpu || mode == kUseSingleCpu)
                break;
        }

        if (configs.size() == 1 && (mode == kUseSingleGpu || mode == kUseSingleCpu))
            break;
    }

    if (!hasprimary)
    {
        configs[0].type = kPrimary;
    }

    for (int i = 0; i < configs.size(); ++i)
    {
        configs[i].renderer = new Baikal::MyRenderer(configs[i].context, configs[i].devidx, initial_num_bounces);
    }
}

#else
void ConfigManager::CreateConfigs(Mode mode, bool interop, std::vector<Config>& configs, int initial_num_bounces)
{
    std::vector<CLWPlatform> platforms;

    CLWPlatform::CreateAllPlatforms(platforms);

    if (platforms.size() == 0)
    {
        throw std::runtime_error("No OpenCL platforms installed.");
    }

    configs.clear();

    bool hasprimary = false;
    for (int i = 0; i < platforms.size(); ++i)
    {
        std::vector<CLWDevice> devices;
        int startidx = (int)configs.size();

        for (int d = 0; d < (int)platforms[i].GetDeviceCount(); ++d)
        {
            if ((mode == kUseGpus || mode == kUseSingleGpu) && platforms[i].GetDevice(d).GetType() != CL_DEVICE_TYPE_GPU)
                continue;

            if ((mode == kUseCpus || mode == kUseSingleCpu) && platforms[i].GetDevice(d).GetType() != CL_DEVICE_TYPE_CPU)
                continue;

            Config cfg;
            cfg.caninterop = false;
            cfg.context = CLWContext::Create(platforms[i].GetDevice(d));
            cfg.devidx = 0;
            cfg.type = kSecondary;

            configs.push_back(cfg);

            if (mode == kUseSingleGpu || mode == kUseSingleCpu)
                break;
        }

        if (configs.size() == 1 && (mode == kUseSingleGpu || mode == kUseSingleCpu))
            break;
    }

    if (!hasprimary)
    {
        configs[0].type = kPrimary;
    }

    for (int i = 0; i < configs.size(); ++i)
    {
        configs[i].renderer = new Baikal::PtRenderer(configs[i].context, configs[i].devidx, initial_num_bounces);
    }
}
#endif //APP_BENCHMARK
