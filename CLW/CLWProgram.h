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
#ifndef __CLW__CLWProgram__
#define __CLW__CLWProgram__

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <string>
#include <cstdint>

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
    static CLWProgram CreateFromSource(char const* sourcecode, size_t sourcesize, char const* buildopts, CLWContext context);
    static CLWProgram CreateFromSource(char const* sourcecode,
                                       size_t sourcesize,
                                       char const** headers,
                                       char const** headernames,
                                       size_t* headersizes,
                                       int numheaders,
                                       char const* buildopts,
                                       CLWContext context);
                                       
    static CLWProgram CreateFromFile(char const* filename,
                                     char const* buildopts,
                                     CLWContext context);

    static CLWProgram CreateFromFile(char const* filename,
                                     char const** headernames,
                                     int numheaders,
                                     char const* buildopts,
                                     CLWContext context);

    static CLWProgram CreateFromBinary(std::uint8_t** binaries, std::size_t* binary_sizes, CLWContext context);

    CLWProgram() = default;
    virtual ~CLWProgram() = default;

    unsigned int GetKernelCount() const;
    CLWKernel    GetKernel(std::string const& funcName) const;

    void GetBinaries(int device, std::vector<std::uint8_t>& data) const;

private:
    CLWProgram(cl_program program);
    std::map<std::string, CLWKernel> kernels_;
};

#endif /* defined(__CLW__CLWProgram__) */
