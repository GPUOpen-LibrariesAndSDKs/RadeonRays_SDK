/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
