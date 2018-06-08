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
#include "CLWProgram.h"
#include "CLWContext.h"
#include "CLWDevice.h"
#include "CLWExcept.h"
#include "CLWKernel.h"

#include <functional>
#include <cassert>
#include <algorithm>
#include <fstream>

static void load_file_contents(std::string const& name, std::vector<char>& contents, bool binary)
{
    std::ifstream in(name, std::ios::in | (std::ios_base::openmode)(binary?std::ios::binary : 0));

    if (in)
    {
        contents.clear();

        std::streamoff beg = in.tellg();

        in.seekg(0, std::ios::end);

        std::streamoff fileSize = in.tellg() - beg;

        in.seekg(0, std::ios::beg);

        contents.resize(static_cast<unsigned>(fileSize));

        in.read(&contents[0], fileSize);
    }
    else
    {
        throw std::runtime_error("Cannot read the contents of a file");
    }
}

CLWProgram CLWProgram::CreateFromSource(char const* sourcecode, size_t sourcesize, char const* buildopts, CLWContext context)
{
    cl_int status = CL_SUCCESS;
    
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&sourcecode, &sourcesize, &status);
    
    ThrowIf(status != CL_SUCCESS, status, "clCreateProgramWithSource failed");
    
    std::vector<cl_device_id> deviceIds(context.GetDeviceCount());
    for(unsigned int i = 0; i < context.GetDeviceCount(); ++i)
    {
        deviceIds[i] = context.GetDevice(i);
    }



    status = clBuildProgram(program, context.GetDeviceCount(), &deviceIds[0], buildopts, nullptr, nullptr);

    if(status != CL_SUCCESS)
    {
        std::vector<char> buildLog;
        size_t logSize;
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);

        buildLog.resize(logSize);
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, logSize, &buildLog[0], nullptr);
        
#ifdef _DEBUG
        std::cout << &buildLog[0] << "\n";
#endif
        
        throw CLWException(status, std::string(&buildLog[0]));
    }
    
    CLWProgram prg(program);
    
    clReleaseProgram(program);

    return prg;
}

CLWProgram CLWProgram::CreateFromSource(char const* sourcecode,
                                        size_t sourcesize,
                                        char const** headers,
                                        char const** headernames,
                                        size_t* headersizes,
                                        int numheaders,
                                        char const* buildopts,
                                        CLWContext context)
{
    cl_int status = CL_SUCCESS;

    std::vector<cl_device_id> deviceIds(context.GetDeviceCount());
    for(unsigned int i = 0; i < context.GetDeviceCount(); ++i)
    {
        deviceIds[i] = context.GetDevice(i);
    }

    std::vector<cl_program> headerPrograms(numheaders);
    for (int i=0; i<numheaders; ++i)
    {
        size_t sourceSize = headersizes[i];
        char const* tempPtr = headers[i];
        headerPrograms[i] = clCreateProgramWithSource(context, 1, (const char**)&tempPtr, &sourceSize, &status);
        ThrowIf(status != CL_SUCCESS, status, "clCreateProgramWithSource failed");
    }
    
    cl_program program = clCreateProgramWithSource(context, 1, (const char**)&sourcecode, &sourcesize, &status);
    
    ThrowIf(status != CL_SUCCESS, status, "clCreateProgramWithSource failed");
    
    status = clCompileProgram(program, context.GetDeviceCount(), &deviceIds[0], buildopts, numheaders, &headerPrograms[0], headernames, nullptr, nullptr);
    
    if(status != CL_SUCCESS)
    {
        std::vector<char> buildLog;
        size_t logSize;
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        
        buildLog.resize(logSize);
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, logSize, &buildLog[0], nullptr);
        
#ifdef _DEBUG
        std::cout << &buildLog[0] << "\n";
#endif
        
        throw CLWException(status, std::string(&buildLog[0]));
    }
    
    status = clBuildProgram(program, context.GetDeviceCount(), &deviceIds[0], buildopts, nullptr, nullptr);
    
    if(status != CL_SUCCESS)
    {
        std::vector<char> buildLog;
        size_t logSize;
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
        
        buildLog.resize(logSize);
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, logSize, &buildLog[0], nullptr);
        
#ifdef _DEBUG
        std::cout << &buildLog[0] << "\n";
#endif
        
        throw CLWException(status, std::string(&buildLog[0]));
    }
    
    CLWProgram prg(program);
    
    clReleaseProgram(program);
    
    return prg;
}

CLWProgram CLWProgram::CreateFromBinary(std::uint8_t** binaries, std::size_t* binary_sizes, CLWContext context)
{
    cl_int status = CL_SUCCESS;

    std::vector<cl_device_id> deviceIds(context.GetDeviceCount());
    for (unsigned int i = 0; i < context.GetDeviceCount(); ++i)
    {
        deviceIds[i] = context.GetDevice(i);
    }

    cl_int binary_status = 0;
    cl_program program = clCreateProgramWithBinary(context, context.GetDeviceCount(), &deviceIds[0], binary_sizes, (const unsigned char**)binaries, &binary_status, &status);

    if (status != CL_SUCCESS)
    {
        std::vector<char> buildLog;
        size_t logSize;
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);

        buildLog.resize(logSize);
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, logSize, &buildLog[0], nullptr);

#ifdef _DEBUG
        std::cout << &buildLog[0] << "\n";
#endif

        throw CLWException(status, std::string(&buildLog[0]));
    }

    status = clBuildProgram(program, context.GetDeviceCount(), &deviceIds[0], nullptr, nullptr, nullptr);

    if (status != CL_SUCCESS)
    {
        std::vector<char> buildLog;
        size_t logSize;
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);

        buildLog.resize(logSize);
        clGetProgramBuildInfo(program, deviceIds[0], CL_PROGRAM_BUILD_LOG, logSize, &buildLog[0], nullptr);

#ifdef _DEBUG
        std::cout << &buildLog[0] << "\n";
#endif

        throw CLWException(status, std::string(&buildLog[0]));
    }

    CLWProgram prg(program);

    clReleaseProgram(program);

    return prg;

}

CLWProgram CLWProgram::CreateFromFile(char const* filename, char const* buildopts, CLWContext context)
{
    std::vector<char> sourcecode;
    load_file_contents(filename, sourcecode, false);
    return CreateFromSource(&sourcecode[0], sourcecode.size(), buildopts, context);
}

CLWProgram CLWProgram::CreateFromFile(char const* filename,
                                      char const** headernames,
                                      int numheaders,
                                      char const* buildopts,
                                      CLWContext context)
{
    std::vector<char> sourcecode;
    load_file_contents(filename, sourcecode, false);
    
    std::vector<std::vector<char> > headers;
    std::vector<char const*> headerstrs;
    std::vector<size_t> headerssizes;

    if (numheaders > 0)
    {
        for (int i = 0; i < numheaders; ++i)
        {
            std::vector<char> headersource;
            load_file_contents(headernames[i], headersource, false);
            headerssizes.push_back(headersource.size());
            headers.push_back(std::move(headersource));
            headerstrs.push_back(&headers[i][0]);
        }

        return CreateFromSource(&sourcecode[0], sourcecode.size(), &headerstrs[0], headernames, &headerssizes[0], numheaders, buildopts, context);
    }
    else
    {
        return CreateFromSource(&sourcecode[0], sourcecode.size(), buildopts, context);
    }
}

CLWProgram::CLWProgram(cl_program program)
: ReferenceCounter<cl_program, clRetainProgram, clReleaseProgram>(program)
{
    cl_int status = CL_SUCCESS;
    cl_uint numKernels;
    status = clCreateKernelsInProgram(*this, 0, nullptr, &numKernels);
    ThrowIf(numKernels == 0, CL_BUILD_ERROR, "clCreateKernelsInProgram return 0 kernels");

    ThrowIf(status != CL_SUCCESS, status, "clCreateKernelsInProgram failed");

    std::vector<cl_kernel> kernels(numKernels);
    status = clCreateKernelsInProgram(*this, numKernels, &kernels[0], nullptr);

    ThrowIf(status != CL_SUCCESS, status, "clCreateKernelsInProgram failed");

    std::for_each(kernels.begin(), kernels.end(), [this](cl_kernel k)
                  {
                      size_t size = 0;
                      cl_int res;

                      res = clGetKernelInfo(k, CL_KERNEL_FUNCTION_NAME, 0, nullptr, &size);
                      ThrowIf(res != CL_SUCCESS, res, "clGetKernelInfo failed");

                      std::vector<char> temp(size);
                      res = clGetKernelInfo(k, CL_KERNEL_FUNCTION_NAME, size, &temp[0], nullptr);
                      ThrowIf(res != CL_SUCCESS, res, "clGetKernelInfo failed");

                      std::string funcName(temp.begin(), temp.end()-1);
                      kernels_[funcName] = CLWKernel::Create(k);
                  });
}

unsigned int CLWProgram::GetKernelCount() const
{
    return static_cast<unsigned int>(kernels_.size());
}

CLWKernel CLWProgram::GetKernel(std::string const& funcName) const
{
    auto iter = kernels_.find(funcName);
    
    ThrowIf(iter == kernels_.end(), CL_INVALID_KERNEL_NAME, "No such kernel in program");
    
    return iter->second;
}

void CLWProgram::GetBinaries(int device, std::vector<std::uint8_t>& data) const
{
    std::uint32_t num_devices;
    auto status = clGetProgramInfo(*this, CL_PROGRAM_NUM_DEVICES,
        sizeof(uint32_t),
        &num_devices,
        nullptr);


    std::vector<std::size_t> binary_sizes(num_devices);
    status = clGetProgramInfo(*this, CL_PROGRAM_BINARY_SIZES,
        sizeof(size_t) * num_devices,
        &binary_sizes[0],
        nullptr);

    ThrowIf(status != CL_SUCCESS, status, "clGetProgramInfo failed");

    data.resize(binary_sizes[device]);

    char** temp = new char*[num_devices];

    for (auto i = 0u; i < num_devices; ++i)
    {
        if (i == device)
        {
            temp[i] = (char*)&data[0];
        }
        else
        {
            temp[i] = new char[binary_sizes[i]];
        }
    }

    status = clGetProgramInfo(*this, CL_PROGRAM_BINARIES,
        sizeof(char*),
        temp,
        nullptr);

    // TODO: fix potential leak here
    ThrowIf(status != CL_SUCCESS, status, "clGetProgramInfo failed");

    for (auto i = 0u; i < num_devices; ++i)
    {
        if (i != device)
        {
            delete temp[i];
        }
    }

    delete [] temp;
}
