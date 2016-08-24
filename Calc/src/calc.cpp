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
#include "calc.h"
#if USE_OPENCL
#include "calc_clw.h"
#endif
#if USE_VULKAN
#include "calc_vk.h"
#include "calc_vkw.h"
#endif

// Create corresponding calc
Calc::Calc* CreateCalc(Calc::Platform inPlatform, int reserved)
{
#if USE_OPENCL
    if (inPlatform & Calc::Platform::kOpenCL)
    {
        return new Calc::CalcClw();
    }
    else
#endif
#if USE_VULKAN
        if (inPlatform & Calc::Platform::kVulkan)
        {
            return new Calc::CalcVulkanw();
        }
        else
#endif // USE_VULKAN
        {
            return nullptr;
        }
}

void DeleteCalc(Calc::Calc* calc)
{
    delete calc;
}
