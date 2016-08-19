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
#include "CLWKernel.h"
#include "ParameterHolder.h"
#include "CLWExcept.h"

CLWKernel CLWKernel::Create(cl_kernel kernel)
{
    CLWKernel k(kernel);
    clReleaseKernel(kernel);
    return k;
}

CLWKernel::CLWKernel(cl_kernel kernel)
: ReferenceCounter<cl_kernel, clRetainKernel, clReleaseKernel>(kernel)
{
}

void CLWKernel::SetArg(unsigned int idx, ParameterHolder param)
{
    param.SetArg(*this, idx); 
}

void CLWKernel::SetArg(unsigned int idx, size_t size, void* ptr)
{
    cl_int status = CL_SUCCESS;
    
    status = clSetKernelArg(*this, idx, size, ptr);
        
    ThrowIf(status != CL_SUCCESS, status, "clSetKernelArg failed");
}