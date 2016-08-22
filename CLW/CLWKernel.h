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
#ifndef __CLW__CLWKernel__
#define __CLW__CLWKernel__

#include <iostream>

#include "ReferenceCounter.h"

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

class ParameterHolder;

class CLWKernel : public ReferenceCounter<cl_kernel, clRetainKernel, clReleaseKernel>
{
public:
    static CLWKernel Create(cl_kernel kernel);
    /// to use in std::map
    CLWKernel(){}
    virtual ~CLWKernel(){}

    virtual void SetArg(unsigned int idx, ParameterHolder param);
    virtual void SetArg(unsigned int idx, size_t size, void* ptr);

private:
    CLWKernel(cl_kernel kernel);
};

#endif /* defined(__CLW__CLWKernel__) */
