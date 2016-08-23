
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
#pragma once
#include "radeon_rays.h"

#if USE_OPENCL

#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdlib>
#include <ctime>
#include <memory>

#include "gtest/gtest.h"
#include "calc.h"
#include "device.h"
#include "buffer.h"
#include "except.h"
#include "event.h"
#include "executable.h"

// Api creation fixture, prepares api_ for further tests
class CalcTestkOpenCL : public ::testing::Test
{
public:
    virtual void SetUp()
    {
        m_calc = CreateCalc(Calc::Platform::kOpenCL, 0);
    }

    virtual void TearDown()
    {
        DeleteCalc(m_calc);
    }

    Calc::Calc* m_calc;

    std::string const cl_source_code =
        " __kernel void add(__global int const* a, __global int const* b, __global int* c) {"
        "int idx = get_global_id(0);"
        "c[idx] = a[idx] + b[idx];}";

    std::string const gl_source_code =
        "#version 430\n"
        "layout( local_size_x = 64, local_size_y = 1, local_size_z = 1 ) in;\n"
        "layout(std430, binding = 0) buffer restrict readonly aBlk { int a[]; };\n"
        "layout(std430, binding = 1) buffer restrict readonly bBlk { int b[]; };\n"
        "layout(std430, binding = 2) buffer cBlk { int c[]; };\n"
        "void main() {\n"
        "uint idx = gl_GlobalInvocationID.x;\n"
        "c[idx] = a[idx] + b[idx];\n}\n";

    std::string const cl_source_code2 =
        " __kernel void add(__global int const* a, int b, __global int* c) {"
        "int idx = get_global_id(0);"
        "c[idx] = a[idx] + b;}";

    std::string const gl_source_code2 =
        "#version 430\n"
        "layout( local_size_x = 64, local_size_y = 1, local_size_z = 1 ) in;\n"
        "layout(std430, binding = 0) buffer restrict readonly aBlk { int a[]; };\n"
        "layout(std430, binding = 1) buffer restrict readonly bBlk { int b; };\n"
        "layout(std430, binding = 2) buffer cBlk { int c[]; };\n"
        "void main() {\n"
        "uint idx = gl_GlobalInvocationID.x;\n"
        "c[idx] = a[idx] + b;\n}\n";
};

TEST_F(CalcTestkOpenCL, Create)
{
}

TEST_F(CalcTestkOpenCL, EnumDevices)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    for (auto i = 0U; i < num_devices; ++i)
    {
        Calc::DeviceSpec spec;
        ASSERT_NO_THROW(m_calc->GetDeviceSpec(i, spec));
    }
}

TEST_F(CalcTestkOpenCL, CreateDevice)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}

TEST_F(CalcTestkOpenCL, CreateBuffer)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::Buffer* buffer = nullptr;

    ASSERT_NO_THROW(buffer = device->CreateBuffer(256, Calc::BufferType::kWrite));

    ASSERT_NO_THROW(device->DeleteBuffer(buffer));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}

TEST_F(CalcTestkOpenCL, CreateBufferZeroSize)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::Buffer* buffer = nullptr;

    // Make sure it throws an exception
    try
    {
        buffer = device->CreateBuffer(0, Calc::BufferType::kWrite);
        ASSERT_TRUE(0);
    }
    catch (Calc::Exception&)
    {
        ASSERT_TRUE(1);
    }
    catch (...)
    {
        ASSERT_TRUE(0);
    }

    ASSERT_NO_THROW(device->DeleteBuffer(buffer));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}

TEST_F(CalcTestkOpenCL, CreateBufferInitialData)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::Buffer* buffer = nullptr;

    const auto kBufferSize = 1000;
    std::vector<int> numbers(kBufferSize);

    std::generate(numbers.begin(), numbers.end(), std::rand);
    ASSERT_NO_THROW(buffer = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite, &numbers[0]));

    std::vector<int> numbers_calc(kBufferSize);

    Calc::Event* e = nullptr;
    ASSERT_NO_THROW(device->ReadBuffer(buffer, 0, 0, kBufferSize * sizeof(int), &numbers_calc[0], &e));

    e->Wait();
    device->DeleteEvent(e);

    for (auto i = 0; i < kBufferSize; ++i)
    {
        ASSERT_EQ(numbers[i], numbers_calc[i]);
    }

    ASSERT_NO_THROW(device->DeleteBuffer(buffer));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}


TEST_F(CalcTestkOpenCL, ReadWriteBuffer)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::Buffer* buffer = nullptr;

    const auto kBufferSize = 1000;
    std::vector<int> numbers(kBufferSize);

    std::generate(numbers.begin(), numbers.end(), std::rand);
    ASSERT_NO_THROW(buffer = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite));

    Calc::Event* e = nullptr;

    ASSERT_NO_THROW(device->WriteBuffer(buffer, 0, 0, kBufferSize * sizeof(int), &numbers[0], &e));

    e->Wait();
    device->DeleteEvent(e);

    std::vector<int> numbers_calc(kBufferSize);

    ASSERT_NO_THROW(device->ReadBuffer(buffer, 0, 0, kBufferSize * sizeof(int), &numbers_calc[0], &e));

    e->Wait();
    device->DeleteEvent(e);

    for (auto i = 0; i < kBufferSize; ++i)
    {
        ASSERT_EQ(numbers[i], numbers_calc[i]);
    }

    ASSERT_NO_THROW(device->DeleteBuffer(buffer));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}

TEST_F(CalcTestkOpenCL, ReadWriteTypedBuffer)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::Buffer* buffer = nullptr;

    const auto kBufferSize = 1000;
    std::vector<int> numbers(kBufferSize);

    std::generate(numbers.begin(), numbers.end(), std::rand);
    ASSERT_NO_THROW(buffer = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite));

    Calc::Event* e = nullptr;

    ASSERT_NO_THROW(device->WriteTypedBuffer(buffer, 0, 0, kBufferSize, &numbers[0], &e));

    e->Wait();
    device->DeleteEvent(e);

    std::vector<int> numbers_calc(kBufferSize);

    ASSERT_NO_THROW(device->ReadTypedBuffer(buffer, 0, 0, kBufferSize, &numbers_calc[0], &e));

    e->Wait();
    device->DeleteEvent(e);

    for (auto i = 0; i < kBufferSize; ++i)
    {
        ASSERT_EQ(numbers[i], numbers_calc[i]);
    }

    ASSERT_NO_THROW(device->DeleteBuffer(buffer));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}

TEST_F(CalcTestkOpenCL, MapBuffer)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::Buffer* buffer = nullptr;

    const auto kBufferSize = 1000;
    std::vector<int> numbers(kBufferSize);

    std::generate(numbers.begin(), numbers.end(), std::rand);
    ASSERT_NO_THROW(buffer = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite));

    int* mapdata = nullptr;
    Calc::Event* e = nullptr;

    ASSERT_NO_THROW(device->MapBuffer(buffer, 0, 0, kBufferSize * sizeof(int), RadeonRays::MapType::kMapWrite, reinterpret_cast<void**>(&mapdata), &e));

    e->Wait();
    device->DeleteEvent(e);

    int id = 0;
    for (auto i : numbers)
    {
        mapdata[id] = i;
        ++id;
    }

    ASSERT_NO_THROW(device->UnmapBuffer(buffer, 0, mapdata, &e));

    e->Wait();
    device->DeleteEvent(e);

    std::vector<int> numbers_calc(kBufferSize);

    ASSERT_NO_THROW(device->ReadBuffer(buffer, 0, 0, kBufferSize * sizeof(int), &numbers_calc[0], &e));

    e->Wait();
    device->DeleteEvent(e);

    for (auto i = 0; i < kBufferSize; ++i)
    {
        ASSERT_EQ(numbers[i], numbers_calc[i]);
    }

    ASSERT_NO_THROW(device->DeleteBuffer(buffer));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}

TEST_F(CalcTestkOpenCL, MapTypedBuffer)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::Buffer* buffer = nullptr;

    const auto kBufferSize = 1000;
    std::vector<int> numbers(kBufferSize);

    std::generate(numbers.begin(), numbers.end(), std::rand);
    ASSERT_NO_THROW(buffer = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite));

    int* mapdata = nullptr;
    Calc::Event* e = nullptr;
    ASSERT_NO_THROW(device->MapTypedBuffer(buffer, 0, 0, kBufferSize, Calc::MapType::kMapWrite, &mapdata, &e));

    e->Wait();
    device->DeleteEvent(e);

    int id = 0;
    for (auto i : numbers)
    {
        mapdata[id] = i;
        ++id;
    }

    ASSERT_NO_THROW(device->UnmapBuffer(buffer, 0, mapdata, nullptr));

    std::vector<int> numbers_calc(kBufferSize);

    ASSERT_NO_THROW(device->ReadTypedBuffer(buffer, 0, 0, kBufferSize, &numbers_calc[0], &e));

    e->Wait();
    device->DeleteEvent(e);

    for (auto i = 0; i < kBufferSize; ++i)
    {
        ASSERT_EQ(numbers[i], numbers_calc[i]);
    }

    ASSERT_NO_THROW(device->DeleteBuffer(buffer));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}


TEST_F(CalcTestkOpenCL, CompileExecutable)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::DeviceSpec spec;
    device->GetSpec(spec);

    std::string source_code;
    if ((spec.sourceTypes & Calc::SourceType::kOpenCL) == Calc::SourceType::kOpenCL)
    {
        source_code = cl_source_code;
    }
    else if ((spec.sourceTypes & Calc::SourceType::kGLSL) == Calc::SourceType::kGLSL)
    {
        source_code = gl_source_code;
    }

    Calc::Executable* executable = nullptr;
    ASSERT_NO_THROW(executable = device->CompileExecutable(source_code.c_str(), source_code.size(), ""));

    Calc::Function* func = nullptr;
    ASSERT_NO_THROW(func = executable->CreateFunction("add"));

    ASSERT_NO_THROW(executable->DeleteFunction(func));
    ASSERT_NO_THROW(device->DeleteExecutable(executable));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}

TEST_F(CalcTestkOpenCL, Execute)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::DeviceSpec spec;
    device->GetSpec(spec);

    std::string source_code;
    if ((spec.sourceTypes & Calc::SourceType::kOpenCL) == Calc::SourceType::kOpenCL)
    {
        source_code = cl_source_code;
    }
    else if ((spec.sourceTypes & Calc::SourceType::kGLSL) == Calc::SourceType::kGLSL)
    {
        source_code = gl_source_code;
    }


    Calc::Executable* executable = nullptr;
    ASSERT_NO_THROW(executable = device->CompileExecutable(source_code.c_str(), source_code.size(), ""));

    Calc::Function* func = nullptr;
    ASSERT_NO_THROW(func = executable->CreateFunction("add"));


    Calc::Buffer* buffer_a = nullptr;
    Calc::Buffer* buffer_b = nullptr;
    Calc::Buffer* buffer_c = nullptr;

    const auto kBufferSize = 1000;
    std::vector<int> numbers_a(kBufferSize);
    std::vector<int> numbers_b(kBufferSize);

    std::generate(numbers_a.begin(), numbers_a.end(), std::rand);
    std::generate(numbers_b.begin(), numbers_b.end(), std::rand);

    ASSERT_NO_THROW(buffer_a = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite, &numbers_a[0]));
    ASSERT_NO_THROW(buffer_b = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite, &numbers_b[0]));
    ASSERT_NO_THROW(buffer_c = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite));

    ASSERT_NO_THROW(func->SetArg(0, buffer_a));
    ASSERT_NO_THROW(func->SetArg(1, buffer_b));
    ASSERT_NO_THROW(func->SetArg(2, buffer_c));
    ASSERT_NO_THROW(device->Execute(func, 0, kBufferSize, 1, nullptr));

    std::vector<int> numbers_c(kBufferSize);

    Calc::Event* e = nullptr;

    ASSERT_NO_THROW(device->ReadBuffer(buffer_c, 0, 0, kBufferSize * sizeof(int), &numbers_c[0], &e));

    e->Wait();
    device->DeleteEvent(e);

    for (auto i = 0; i < kBufferSize; ++i)
    {
        ASSERT_EQ(numbers_c[i], numbers_a[i] + numbers_b[i]);
    }

    ASSERT_NO_THROW(device->DeleteBuffer(buffer_a));
    ASSERT_NO_THROW(device->DeleteBuffer(buffer_b));
    ASSERT_NO_THROW(device->DeleteBuffer(buffer_c));
    ASSERT_NO_THROW(executable->DeleteFunction(func));
    ASSERT_NO_THROW(device->DeleteExecutable(executable));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}

TEST_F(CalcTestkOpenCL, ExecuteRawParams)
{
    auto num_devices = m_calc->GetDeviceCount();

    ASSERT_GE(num_devices, 0U);

    Calc::Device* device = nullptr;

    ASSERT_NO_THROW(device = m_calc->CreateDevice(0));

    Calc::DeviceSpec spec;
    device->GetSpec(spec);

    std::string source_code;
    if ((spec.sourceTypes & Calc::SourceType::kOpenCL) == Calc::SourceType::kOpenCL)
    {
        source_code = cl_source_code2;
    }
    else if ((spec.sourceTypes & Calc::SourceType::kGLSL) == Calc::SourceType::kGLSL)
    {
        source_code = gl_source_code2;
    }

    Calc::Executable* executable = nullptr;
    ASSERT_NO_THROW(executable = device->CompileExecutable(source_code.c_str(), source_code.size(), ""));

    Calc::Function* func = nullptr;
    ASSERT_NO_THROW(func = executable->CreateFunction("add"));


    Calc::Buffer* buffer_a = nullptr;
    Calc::Buffer* buffer_c = nullptr;

    const auto kBufferSize = 1000;
    std::vector<int> numbers_a(kBufferSize);

    std::generate(numbers_a.begin(), numbers_a.end(), std::rand);

    ASSERT_NO_THROW(buffer_a = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite, &numbers_a[0]));
    ASSERT_NO_THROW(buffer_c = device->CreateBuffer(kBufferSize * sizeof(int), Calc::BufferType::kWrite));

    std::uint32_t b = 5;
    ASSERT_NO_THROW(func->SetArg(0, buffer_a));
    ASSERT_NO_THROW(func->SetArg(1, sizeof(b), &b));
    ASSERT_NO_THROW(func->SetArg(2, buffer_c));
    ASSERT_NO_THROW(device->Execute(func, 0, kBufferSize, 1, nullptr));

    std::vector<int> numbers_c(kBufferSize);

    Calc::Event* e = nullptr;

    ASSERT_NO_THROW(device->ReadBuffer(buffer_c, 0, 0, kBufferSize * sizeof(int), &numbers_c[0], &e));

    e->Wait();
    device->DeleteEvent(e);

    for (auto i = 0; i < kBufferSize; ++i)
    {
        ASSERT_EQ(numbers_c[i], numbers_a[i] + b);
    }

    ASSERT_NO_THROW(device->DeleteBuffer(buffer_a));
    ASSERT_NO_THROW(device->DeleteBuffer(buffer_c));
    ASSERT_NO_THROW(executable->DeleteFunction(func));
    ASSERT_NO_THROW(device->DeleteExecutable(executable));
    ASSERT_NO_THROW(m_calc->DeleteDevice(device));
}

#endif //USE_OPENCL