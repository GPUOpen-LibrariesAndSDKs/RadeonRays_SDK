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
#if USE_OPENCL
#include "calc.h"
#include "primitives.h"
#include "device_clw.h"
#include "buffer.h"
#include "event.h"
#include "executable.h"
#include "except_clw.h"
#include "calc_clw_common.h"

namespace Calc
{    
    // Buffer implementation with CLW
    class BufferClw : public Buffer
    {
    public:
        BufferClw(CLWBuffer<char> buffer) : m_buffer(buffer) {}
        ~BufferClw() override {};

        std::size_t GetSize() const override { return m_buffer.GetElementCount(); }

        CLWBuffer<char> GetData() const { return m_buffer; }

    private:
        CLWBuffer<char> m_buffer;
    };


    // Event implementation with CLW
    class EventClw : public Event
    {
    public:
        EventClw() {}
        EventClw(CLWEvent event);
        ~EventClw();

        void Wait() override;
        bool IsComplete() const override;

        void SetEvent(CLWEvent event);

    private:
        CLWEvent m_event;
    };

    EventClw::EventClw(CLWEvent event)
        : m_event(event)
    {
    }

    EventClw::~EventClw()
    {
    }

    void EventClw::Wait()
    {
        try
        {
            m_event.Wait();
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    bool EventClw::IsComplete() const
    {
        try
        {
            return m_event.GetCommandExecutionStatus() == CL_COMPLETE;
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void EventClw::SetEvent(CLWEvent event)
    {
        m_event = event;
    }

    class FunctionClw : public Function
    {
    public:
        FunctionClw(CLWKernel kernel);
        ~FunctionClw();

        // Argument setters
        void SetArg(std::uint32_t idx, std::size_t arg_size, void* arg) override;
        void SetArg(std::uint32_t idx, Buffer const* arg) override;
        void SetArg(std::uint32_t idx, std::size_t size, SharedMemory shmem) override;

        // CLW object access
        CLWKernel GetKernel() const;

    private:
        CLWKernel m_kernel;
    };


    FunctionClw::FunctionClw(CLWKernel kernel)
        : m_kernel(kernel)
    {
    }

    FunctionClw::~FunctionClw()
    {
    }

    // Argument setters
    void FunctionClw::SetArg(std::uint32_t idx, std::size_t arg_size, void* arg)
    {
        try
        {
            m_kernel.SetArg(idx, arg_size, arg);
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void FunctionClw::SetArg(std::uint32_t idx, Buffer const* arg)
    {
        try
        {
            auto buffer_clw = static_cast<BufferClw const*>(arg);
            m_kernel.SetArg(idx, buffer_clw->GetData());
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void FunctionClw::SetArg(std::uint32_t idx, std::size_t size, SharedMemory shmem)
    {
        try
        {
            m_kernel.SetArg(idx, ::SharedMemory(static_cast<cl_uint>(size)));
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    CLWKernel FunctionClw::GetKernel() const
    {
        return m_kernel;
    }

    // Executable implementation
    class ExecutableClw : public Executable
    {
    public:
        ExecutableClw(CLWProgram program);
        ~ExecutableClw();

        // Function management
        Function* CreateFunction(char const* name) override;
        void DeleteFunction(Function* func) override;

    private:
        CLWProgram m_program;
    };

    ExecutableClw::ExecutableClw(CLWProgram program)
        : m_program(program)
    {
    }

    ExecutableClw::~ExecutableClw()
    {

    }

    Function* ExecutableClw::CreateFunction(char const* name)
    {
        try
        {

            return new FunctionClw(m_program.GetKernel(name));
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void ExecutableClw::DeleteFunction(Function* func)
    {
        delete func;
    }

    // Device
    DeviceClw::DeviceClw(CLWDevice device)
        : m_device(device)
        , m_context(CLWContext::Create(device))
    {
        // Initialize event pool
        for (auto i = 0; i < EVENT_POOL_INITIAL_SIZE; ++i)
        {
            m_event_pool.push(new EventClw());
        }
    }
    
    DeviceClw::DeviceClw(CLWDevice device, CLWContext context)
    : m_device(device)
    , m_context(context)
    {
        // Initialize event pool
        for (auto i = 0; i < EVENT_POOL_INITIAL_SIZE; ++i)
        {
            m_event_pool.push(new EventClw());
        }
    }

    DeviceClw::~DeviceClw()
    {
        while (!m_event_pool.empty())
        {
            auto event = m_event_pool.front();
            m_event_pool.pop();
            delete event;
        }
    }

    void DeviceClw::GetSpec(DeviceSpec& spec)
    {
        spec.name = m_device.GetName().c_str();
        spec.vendor = m_device.GetVendor().c_str();
        spec.sourceTypes = SourceType::kOpenCL;

        spec.type = Convert2CalcDeviceType(m_device.GetType());

        spec.global_mem_size = m_device.GetGlobalMemSize();
        spec.local_mem_size = m_device.GetLocalMemSize();
        spec.min_alignment = m_device.GetMinAlignSize();
        spec.max_alloc_size = m_device.GetMaxAllocSize();
        spec.max_local_size = m_device.GetMaxWorkGroupSize();

        spec.has_fp16 = (m_device.GetExtensions().find("cl_khr_fp16") != std::string::npos);
    }

    Buffer* DeviceClw::CreateBuffer(std::size_t size, std::uint32_t flags)
    {
        try
        {
            return new BufferClw(m_context.CreateBuffer<char>(size, Convert2ClCreationFlags(flags)));
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    Buffer* DeviceClw::CreateBuffer(std::size_t size, std::uint32_t flags, void* initdata)
    {
        try
        {
            return new BufferClw(m_context.CreateBuffer<char>(size, Convert2ClCreationFlags(flags) | CL_MEM_COPY_HOST_PTR, initdata));
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void DeviceClw::DeleteBuffer(Buffer* buffer)
    {
        delete buffer;
    }

    void DeviceClw::ReadBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* dst, Event** e) const
    {
        auto buffer_clw = static_cast<BufferClw const*>(buffer);

        try
        {
            CLWEvent event = m_context.ReadBuffer(queue, buffer_clw->GetData(), static_cast<char*>(dst), offset, size);

            if (e)
            {
                auto event_clw = CreateEventClw();
                event_clw->SetEvent(event);
                *e = event_clw;
            }
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void DeviceClw::WriteBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* src, Event** e)
    {
        auto buffer_clw = static_cast<BufferClw const*>(buffer);

        try
        {
            CLWEvent event = m_context.WriteBuffer(queue, buffer_clw->GetData(), static_cast<char*>(src), offset, size);

            if (e)
            {
                auto event_clw = CreateEventClw();
                event_clw->SetEvent(event);
                *e = event_clw;
            }
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void DeviceClw::MapBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, std::uint32_t map_type, void** mapdata, Event** e)
    {
        auto buffer_clw = static_cast<BufferClw const*>(buffer);

        try
        {
            CLWEvent event = m_context.MapBuffer(queue, buffer_clw->GetData(), Convert2ClMapFlags(map_type), offset, size, reinterpret_cast<char**>(mapdata));

            if (e)
            {
                auto event_clw = CreateEventClw();
                event_clw->SetEvent(event);
                *e = event_clw;
            }
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void DeviceClw::UnmapBuffer(Buffer const* buffer, std::uint32_t queue, void* mapdata, Event** e)
    {
        auto buffer_clw = static_cast<BufferClw const*>(buffer);

        try
        {
            CLWEvent event = m_context.UnmapBuffer(queue, buffer_clw->GetData(), static_cast<char*>(mapdata));

            if (e)
            {
                auto event_clw = CreateEventClw();
                event_clw->SetEvent(event);
                *e = event_clw;
            }
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    Executable* DeviceClw::CompileExecutable(char const* source_code, std::size_t size, char const* options)
    {
        try
        {
            std::string buildopts = options ? options : "";

            buildopts.append(" -cl-mad-enable -cl-fast-relaxed-math -cl-std=CL1.2 -I . ");

            bool isamd = m_device.GetVendor().find("AMD") != std::string::npos ||
                m_device.GetVendor().find("Advanced Micro Devices") != std::string::npos;

            bool has_mediaops = m_device.GetExtensions().find("cl_amd_media_ops2") != std::string::npos;

            if (isamd)
            {
                buildopts.append(" -D AMD ");
            }

            if (has_mediaops)
            {
                buildopts.append(" -D AMD_MEDIA_OPS ");
            }

            buildopts.append(
#if defined(__APPLE__)
                "-D APPLE "
#elif defined(_WIN32) || defined (WIN32)
                "-D WIN32 "
#elif defined(__linux__)
                "-D __linux__ "
#else
                ""
#endif
                );

            return new ExecutableClw(CLWProgram::CreateFromSource(source_code, size, buildopts.c_str(), m_context));
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }

    }
    
    Executable* DeviceClw::CompileExecutable(char const* filename,
                                             char const** headernames,
                                             int numheaders,
                                             char const* options
                                            )
    {
        try
        {
            std::string buildopts = options ? options : "";

            buildopts.append(" -cl-mad-enable -cl-fast-relaxed-math -cl-std=CL1.2 -I . ");

            bool isamd = m_device.GetVendor().find("AMD") != std::string::npos ||
                m_device.GetVendor().find("Advanced Micro Devices") != std::string::npos;

            bool has_mediaops = m_device.GetExtensions().find("cl_amd_media_ops2") != std::string::npos;

            if (isamd)
            {
                buildopts.append(" -D AMD ");
            }

            if (has_mediaops)
            {
                buildopts.append(" -D AMD_MEDIA_OPS ");
            }

            buildopts.append(
#if defined(__APPLE__)
                "-D APPLE " 
#elif defined(_WIN32) || defined (WIN32)
                "-D WIN32 "
#elif defined(__linux__)
                "-D __linux__ "
#else
                ""
#endif
                );

            return new ExecutableClw(
                                     CLWProgram::CreateFromFile(filename, headernames, numheaders, buildopts.c_str(), m_context)
                                     );
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    Executable* DeviceClw::CompileExecutable(std::uint8_t const* binary_code, std::size_t size, char const* options)
    {
        return nullptr;
    }

    void DeviceClw::DeleteExecutable(Executable* executable)
    {
        delete executable;
    }

    size_t DeviceClw::GetExecutableBinarySize(Executable const* executable) const
    {
        return 0;
    }

    void DeviceClw::GetExecutableBinary(Executable const* executable, std::uint8_t* binary) const
    {

    }

    void DeviceClw::Execute(Function const* func, std::uint32_t queue, size_t global_size, size_t local_size, Event** e)
    {
        auto func_clw = static_cast<FunctionClw const*>(func);

        try
        {
            CLWEvent event = m_context.Launch1D(queue, global_size, local_size, func_clw->GetKernel());

            if (e)
            {
                auto event_clw = CreateEventClw();
                event_clw->SetEvent(event);
                *e = event_clw;
            }
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void DeviceClw::WaitForEvent(Event* e)
    {
        e->Wait();
    }

    void DeviceClw::WaitForMultipleEvents(Event** e, std::size_t num_events)
    {
        for (auto i = 0; i < num_events; ++i)
        {
            e[i]->Wait();
        }
    }

    void DeviceClw::DeleteEvent(Event* e)
    {
        ReleaseEventClw(static_cast<EventClw*>(e));
    }

    void DeviceClw::Flush(std::uint32_t queue)
    {
        try
        {
            m_context.Flush(queue);
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    void DeviceClw::Finish(std::uint32_t queue)
    {
        try
        {
            m_context.Finish(queue);
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    EventClw* DeviceClw::CreateEventClw() const
    {
        if (m_event_pool.empty())
        {
            auto event = new EventClw();
            return event;
        }
        else
        {
            auto event = m_event_pool.front();
            m_event_pool.pop();
            return event;
        }
    }

    void DeviceClw::ReleaseEventClw(EventClw* e) const
    {
        m_event_pool.push(e);
    }
    
    Buffer* DeviceClw::CreateBuffer(cl_mem buffer)
    {
        try
        {
            return new BufferClw(CLWBuffer<char>::CreateFromClBuffer(buffer));
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }


    class PrimitivesClw : public Primitives
    {
    public:
        PrimitivesClw(CLWContext context)
        {
            std::string buildopts = "";
            
            buildopts.append(" -cl-mad-enable -cl-fast-relaxed-math -cl-std=CL1.2 -I . ");
            
            bool isamd = context.GetDevice(0).GetVendor().find("AMD") != std::string::npos ||
            context.GetDevice(0).GetVendor().find("Advanced Micro Devices") != std::string::npos;
            
            bool has_mediaops = context.GetDevice(0).GetExtensions().find("cl_amd_media_ops2") != std::string::npos;
            
            if (isamd)
            {
                buildopts.append(" -D AMD ");
            }
            
            if (has_mediaops)
            {
                buildopts.append(" -D AMD_MEDIA_OPS ");
            }
            
            buildopts.append(
#if defined(__APPLE__)
                             "-D APPLE "
#elif defined(_WIN32) || defined (WIN32)
                             "-D WIN32 "
#elif defined(__linux__)
                             "-D __linux__ "
#else
                             ""
#endif
                             );
            
            m_pp = CLWParallelPrimitives(context, buildopts.c_str());
        }

        void SortRadixInt32(std::uint32_t queueidx, Buffer const* from_key, Buffer* to_key, Buffer const* from_value, Buffer* to_value, std::size_t size) override
        {
            auto from_key_clw = static_cast<BufferClw const*>(from_key);
            auto to_key_clw = static_cast<BufferClw*>(to_key);
            auto from_value_clw = static_cast<BufferClw const*>(from_value);
            auto to_value_clw = static_cast<BufferClw*>(to_value);

            m_pp.SortRadix((int)queueidx, from_key_clw->GetData(), to_key_clw->GetData(), from_value_clw->GetData(), to_value_clw->GetData(), (int)size);
        }

    private:
        CLWParallelPrimitives m_pp;
    };


    bool DeviceClw::HasBuiltinPrimitives() const
    {
        return true;
    }

    Primitives* DeviceClw::CreatePrimitives() const
    {
        return new PrimitivesClw(m_context);
    }

    void DeviceClw::DeletePrimitives(Primitives* prims)
    {
        delete prims;
    }
}

#endif
