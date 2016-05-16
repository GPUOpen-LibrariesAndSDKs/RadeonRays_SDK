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
#ifndef __CLW__ParameterHolder__
#define __CLW__ParameterHolder__

#include <iostream>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif


struct SharedMemory
{
    SharedMemory(cl_uint size) : size_(size) {}
    
    cl_uint size_;
};

class ParameterHolder
{
public:
    enum Type
    {
        kMem,
        kInt,
        kUInt,
        kFloat,
        kFloat2,
        kFloat4,
        kDouble,
        kShmem
    };
    
    ParameterHolder(cl_mem    mem) : mem_(mem) { type_ = kMem; }
    ParameterHolder(cl_int    intValue) : intValue_(intValue) { type_ = kInt; }
    ParameterHolder(cl_uint   uintValue) : uintValue_(uintValue) { type_ = kUInt; }
    ParameterHolder(cl_float  floatValue) : floatValue_(floatValue) { type_ = kFloat; }
    ParameterHolder(cl_float2  floatValue) : floatValue2_(floatValue) { type_ = kFloat2; }
    ParameterHolder(cl_float4  floatValue) : floatValue4_(floatValue) { type_ = kFloat4; }
    ParameterHolder(cl_double doubleValue) : doubleValue_(doubleValue) { type_ = kDouble; }
    ParameterHolder(SharedMemory shMem) : uintValue_(shMem.size_) { type_ = kShmem; }
    
    void SetArg(cl_kernel kernel, unsigned int idx);
    
private:
    
    Type type_;
    
    union {
        cl_mem mem_;
        cl_int intValue_;
        cl_uint uintValue_;
        cl_float floatValue_;
        cl_float2 floatValue2_;
        cl_float4 floatValue4_;
        cl_double doubleValue_;
    };
};

#endif /* defined(__CLW__ParameterHolder__) */
