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
#include "hip/hip_runtime.h"
#include "except_clw.h"
#include "device_hip.h"
#include "event.h"
#include "executable.h"
#include "Hip/executable_hipw.h"
#include <functional>
#include <map>
#include <cassert>

#define CHECK_HIP_CMD(err) if (err != hipSuccess) {throw ExceptionClw(std::string("Hip failed") + hipGetErrorString(err));}

namespace Calc
{
    HipBuffer::HipBuffer(std::size_t size, void* init_data)
        : m_size(size)
    {
        auto err = hipMalloc(&m_device_buffer, m_size);
        CHECK_HIP_CMD(err);

        if (init_data)
        {
            err = hipMemcpy(m_device_buffer, init_data, size, hipMemcpyHostToDevice);
            CHECK_HIP_CMD(err);
        }
    }

    HipBuffer::~HipBuffer()
    {
        auto err = hipFree(m_device_buffer);
        CHECK_HIP_CMD(err);
    }

    std::size_t HipBuffer::GetSize() const
    {
        return m_size;
    }

    void HipBuffer::Read(void* dst, std::size_t size) const
    {
        auto err = hipMemcpy(dst, m_device_buffer, size, hipMemcpyDeviceToHost);
        CHECK_HIP_CMD(err);
    }
    void HipBuffer::Write(void* src, std::size_t size) const
    {
        auto err = hipMemcpy(m_device_buffer, src, size, hipMemcpyHostToDevice);
        CHECK_HIP_CMD(err);
    }
    void HipBuffer::Map(std::size_t offset, std::size_t size, void** mapdata) const
    {
        // TODO: support buffer offset
        assert(offset == 0);
        auto err = hipHostMalloc(mapdata, size, hipDeviceScheduleYield);
        CHECK_HIP_CMD(err);
        
        err = hipMemcpy(*mapdata, m_device_buffer, size, hipMemcpyDeviceToHost);
        CHECK_HIP_CMD(err);
    }
    void HipBuffer::Unmap(void* mapdata) const
    {
        auto err = hipMemcpy(m_device_buffer, mapdata, m_size, hipMemcpyHostToDevice);
        CHECK_HIP_CMD(err);

        err = hipHostFree(mapdata);
        CHECK_HIP_CMD(err);
    }

    class HipEvent : public Event
    {
    public:
        HipEvent()
        {
            auto err = hipEventCreate(&m_event);
            CHECK_HIP_CMD(err);

            //use default steam
            err = hipEventRecord(m_event, 0);
            CHECK_HIP_CMD(err);

        }
        virtual ~HipEvent()
        {
            auto err = hipEventDestroy(m_event);
            CHECK_HIP_CMD(err);
        }
        
        void Wait() override
        {
            auto err = hipEventSynchronize(m_event);
            CHECK_HIP_CMD(err);
        }

        bool IsComplete() const override
        {
            auto status = hipEventQuery(m_event);
            //hipSuccess - finished, hipErrorNotReady - not finished
            //other values - something goes wrong
            if (status!= hipSuccess && status != hipErrorNotReady)
            {
                //process error
                CHECK_HIP_CMD(status);
            }

            return status == hipSuccess;
        }
    private:
        hipEvent_t m_event;
    };

    DeviceHip::DeviceHip()
    {
        hipStream_t stream;
        auto err = hipStreamCreate(&stream);
        CHECK_HIP_CMD(err);
    }

    DeviceHip::~DeviceHip()
    {

    }

    void DeviceHip::GetSpec(DeviceSpec& spec)
    {

    }

    Platform DeviceHip::GetPlatform() const
    {
        return Calc::Platform::kHip;
    }

    Buffer* DeviceHip::CreateBuffer(std::size_t size, std::uint32_t flags)
    {
        return new HipBuffer(size);
    }

    Buffer* DeviceHip::CreateBuffer(std::size_t size, std::uint32_t flags, void* initdata)
    {
        return new HipBuffer(size, initdata);
    }

    void DeviceHip::DeleteBuffer(Buffer* buffer)
    {
        delete buffer;
    }

    void DeviceHip::ReadBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* dst, Event** e) const
    {
        const HipBuffer* hip_buffer = dynamic_cast<HipBuffer const *>(buffer);
        assert(hip_buffer);

        hip_buffer->Read(dst, size);

        if (e)
        {
            *e = new HipEvent();
        }
    }

    void DeviceHip::WriteBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* src, Event** e)
    {
        HipBuffer const* hip_buffer = dynamic_cast<HipBuffer const*>(buffer);
        assert(hip_buffer);

        hip_buffer->Write(src, size);

        if (e)
        {
            *e = new HipEvent();
        }
    }

    void DeviceHip::MapBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, std::uint32_t map_type, void** mapdata, Event** e)
    {
        HipBuffer const* hip_buffer = dynamic_cast<HipBuffer const*>(buffer);
        assert(hip_buffer);

        hip_buffer->Map(offset, size, mapdata);

        if (e)
        {
            *e = new HipEvent();
        }
    }

    void DeviceHip::UnmapBuffer(Buffer const* buffer, std::uint32_t queue, void* mapdata, Event** e)
    {
        HipBuffer const* hip_buffer = dynamic_cast<HipBuffer const*>(buffer);
        assert(hip_buffer);

        hip_buffer->Unmap(mapdata);

        if (e)
        {
            *e = new HipEvent();
        }
    }

    Executable* DeviceHip::CompileExecutable(char const* source_code, std::size_t size, char const* options)
    {
        //all kernels already compiled with C++ compiler, so nothing to do here 
    }

    Executable* DeviceHip::CompileExecutable(std::uint8_t const* binary_code, std::size_t size, char const*  options)
    {
        //all kernels already compiled with C++ compiler, so nothing to do here 
        return nullptr;
    }

    Executable* DeviceHip::CompileExecutable(char const* filename,
                                          char const** headernames,
                                          int numheaders, char const* options)
    {
        return new HipExecutable(filename);
    }


    void DeviceHip::DeleteExecutable(Executable* executable)
    {
        delete executable;
    }

    size_t DeviceHip::GetExecutableBinarySize(Executable const* executable) const
    {
        throw ExceptionClw("hip GetExecutableBinary not implemented.");
    }

    void DeviceHip::GetExecutableBinary(Executable const* executable, std::uint8_t* binary) const
    {
        throw ExceptionClw("hip GetExecutableBinary not implemented.");
    }

    void DeviceHip::Execute(Function const* func, std::uint32_t queue, size_t global_size, size_t local_size, Event** e)
    {
        //only default stream used
        assert(queue == 0);

        HipFunction const* hip_func = dynamic_cast<HipFunction const*>(func);
        assert(hip_func);

        hip_func->Execute(queue, global_size, local_size);

        if (e)
        {
            *e = new HipEvent();
        }
    }

    void DeviceHip::WaitForEvent(Event* e)
    {
        e->Wait();
    }

    void DeviceHip::WaitForMultipleEvents(Event** e, std::size_t num_events)
    {
        for(int i = 0; i < num_events; ++i)
        {
            e[i]->Wait();
        }
    }

    void DeviceHip::DeleteEvent(Event* e)
    {
        delete e;
    }

    void DeviceHip::Flush(std::uint32_t queue)
    {

    }

    void DeviceHip::Finish(std::uint32_t queue)
    {
        auto err = hipStreamSynchronize(0);
        CHECK_HIP_CMD(err);
    }

    bool DeviceHip::HasBuiltinPrimitives() const
    {
        assert(false);
    }

    Primitives* DeviceHip::CreatePrimitives() const
    {
        assert(false);
    }

    void DeviceHip::DeletePrimitives(Primitives* prims)
    {
        assert(false);
    }

} //Calc
