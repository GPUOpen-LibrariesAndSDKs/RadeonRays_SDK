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

#ifndef __CLW__CLWProgram__
#define __CLW__CLWProgram__

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <string>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

#include "CLWKernel.h"
#include "ReferenceCounter.h"

class CLWContext;

class CLWProgram : public ReferenceCounter<cl_program, clRetainProgram, clReleaseProgram>
{
public:
    static CLWProgram CreateFromSource(char const* sourcecode, size_t sourcesize, CLWContext context);
    static CLWProgram CreateFromSource(char const* sourcecode,
                                       size_t sourcesize,
                                       char const** headers,
                                       char const** headernames,
                                       size_t* headersizes,
                                       int numheaders,
                                       CLWContext context);
                                       
    static CLWProgram CreateFromFile(char const* filename,
                                     CLWContext context);
    
    static CLWProgram CreateFromFile(char const* filename,
                                     char const** headernames,
                                     int numheaders,
                                     CLWContext context);

    CLWProgram() {}
    virtual      ~CLWProgram();

    unsigned int GetKernelCount() const;
    CLWKernel    GetKernel(std::string const& funcName) const;
    
private:
    CLWProgram(cl_program program);
    std::map<std::string, CLWKernel> kernels_;
};

#endif /* defined(__CLW__CLWProgram__) */
