#include "calc_metal.h"

#ifdef USE_METAL

#include "device.h"
#include "event.h"
#include "buffer.h"
#include "except.h"
#include "executable.h"

#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <queue>
#include <map>

namespace Calc
{
    class DeviceMetal;
    
    class ExceptionMetal : public Exception
    {
    public:
        ExceptionMetal(std::string what) : m_what(what) {}
        ~ExceptionMetal() {}
        
        char const* what() const override { return m_what.c_str(); }
        
    private:
        std::string m_what;
    };
    
    
    class FunctionMetal : public Function
    {
    public:
        FunctionMetal(DeviceMetal* device, id<MTLFunction> kernel);
        ~FunctionMetal();
        
        // Argument setters
        void SetArg(std::uint32_t idx, std::size_t arg_size, void* arg) override;
        void SetArg(std::uint32_t idx, Buffer const* arg) override;
        void SetArg(std::uint32_t idx, std::size_t size, SharedMemory shmem) override;
        
        // CLW object access
        id<MTLFunction> GetKernel() const;
        
    private:
        id<MTLFunction> m_kernel;
        DeviceMetal* m_device;
        
        struct BufferHolder;
        
        std::map<std::uint32_t, BufferHolder> m_args;
        
        friend class DeviceMetal;
    };
    
    struct FunctionMetal::BufferHolder
    {
        Buffer const* buffer;
        bool autorelease;
    };
    
    
    FunctionMetal::FunctionMetal(DeviceMetal* device, id<MTLFunction> kernel)
    : m_device(device)
    , m_kernel(kernel)
    {
    }
    


    
    void FunctionMetal::SetArg(std::uint32_t idx, Buffer const* arg)
    {
        m_args[idx] = BufferHolder { arg, false };
    }
    
    void FunctionMetal::SetArg(std::uint32_t idx, std::size_t size, SharedMemory shmem)
    {
        // Not supported
    }
    
    id<MTLFunction> FunctionMetal::GetKernel() const
    {
        return m_kernel;
    }
    
    
    class ExecutableMetal : public Executable
    {
    public:
        ExecutableMetal(DeviceMetal* device, id<MTLLibrary> lib)
        : m_device(device)
        , m_library(lib){}
        
        ExecutableMetal()
        : m_device(nil)
        , m_library(nil) {}
        
        // Function management
        Function* CreateFunction(char const* name) override;
        void DeleteFunction(Function* func) override;
        
    private:
        id<MTLLibrary> m_library;
        DeviceMetal* m_device;
    };
    
    // Function management
    Function* ExecutableMetal::CreateFunction(char const* name)
    {
        NSString* ns_name = [NSString stringWithFormat:@"%s", name];
        id<MTLFunction> func = [m_library newFunctionWithName:ns_name];
        
        if (func == nil)
        {
            std::string message = "Function ";
            message.append(name);
            message.append(" not found in specified executable\n");
            throw ExceptionMetal(message.c_str());
        }
        
        return new FunctionMetal(m_device, func);
    }
    
    void ExecutableMetal::DeleteFunction(Function* func)
    {
        delete func;
    }
    
    class BufferMetal : public Buffer
    {
    public:
        BufferMetal(id<MTLBuffer> buffer)
        : m_buffer(buffer)
        {
        }
        
        
        std::size_t GetSize() const override
        {
            return m_buffer.length;
        }
        
        BufferMetal(BufferMetal const&) = delete;
        BufferMetal& operator = (BufferMetal const&) = delete;

        id<MTLBuffer> m_buffer;
    };
    
    class EventMetal : public Event
    {
    public:
        EventMetal()
        {
        }
        
        void Wait() override {}
        bool IsComplete() const override {return true;}
        
        
        EventMetal(EventMetal const&) = delete;
        EventMetal& operator = (EventMetal const&) = delete;
    };
    
    
    class DeviceMetal : public Device
    {
    public:
        DeviceMetal();
        ~DeviceMetal();
        
        // Device overrides
        // Return specification of the device
        void GetSpec(DeviceSpec& spec) override;
        
        // Buffer creation and deletion
        Buffer* CreateBuffer(std::size_t size, std::uint32_t flags) override;
        Buffer* CreateBuffer(std::size_t size, std::uint32_t flags, void* initdata) override;
        void DeleteBuffer(Buffer* buffer) override;
        
        // Data movement
        void ReadBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* dst, Event** e) const override;
        void WriteBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* src, Event** e) override;
        
        // Buffer mapping
        void MapBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, std::uint32_t map_type, void** mapdata, Event** e) override;
        void UnmapBuffer(Buffer const* buffer, std::uint32_t queue, void* mapdata, Event** e) override;
        
        // Kernel compilation
        Executable* CompileExecutable(char const* source_code, std::size_t size, char const* options) override;
        Executable* CompileExecutable(std::uint8_t const* binary_code, std::size_t size, char const* options) override;
        Executable* CompileExecutable(char const* filename,
                                      char const** headernames,
                                      int numheaders) override;
        
        void DeleteExecutable(Executable* executable) override;
        
        // Executable management
        size_t GetExecutableBinarySize(Executable const* executable) const override;
        void GetExecutableBinary(Executable const* executable, std::uint8_t* binary) const override;
        
        // Execution
        void Execute(Function const* func, std::uint32_t queue, size_t global_size, size_t local_size, Event** e) override;
        
        // Events handling
        void WaitForEvent(Event* e) override;
        void WaitForMultipleEvents(Event** e, std::size_t num_events) override;
        void DeleteEvent(Event* e) override;
        
        // Queue management functions
        void Flush(std::uint32_t queue) override;
        void Finish(std::uint32_t queue) override;
        
        // Parallel prims handling
        bool HasBuiltinPrimitives() const override;
        Primitives* CreatePrimitives() const override;
        void DeletePrimitives(Primitives* prims) override;
        
        
        Platform GetPlatform() const override { return Platform::kMetal; }
        
    protected:
        EventMetal* CreateEventMetal() const;
        void      ReleaseEventMetal(EventMetal* e) const;

        
    private:
        id<MTLDevice> m_device;
        id<MTLCommandQueue> m_cmdqueue;
        
        // Initial number of events in the pool
        static const std::size_t EVENT_POOL_INITIAL_SIZE = 100;
        // Event pool
        mutable std::queue<EventMetal*> m_event_pool;
    };
    
    EventMetal* DeviceMetal::CreateEventMetal() const
    {
        if (m_event_pool.empty())
        {
            auto event = new EventMetal();
            return event;
        }
        else
        {
            auto event = m_event_pool.front();
            m_event_pool.pop();
            return event;
        }
    }
    
    void DeviceMetal::ReleaseEventMetal(EventMetal* e) const
    {
        m_event_pool.push(e);
    }
    
    DeviceMetal::DeviceMetal()
    {
        // TODO: dirty hack
        m_device = MTLCreateSystemDefaultDevice();
        m_cmdqueue = [m_device newCommandQueue];
        
        // Initialize event pool
        for (auto i = 0; i < EVENT_POOL_INITIAL_SIZE; ++i)
        {
            m_event_pool.push(new EventMetal());
        }
    }
    
    DeviceMetal::~DeviceMetal()
    {
        while (!m_event_pool.empty())
        {
            auto event = m_event_pool.front();
            m_event_pool.pop();
            delete event;
        }
    }
    
    void DeviceMetal::GetSpec(DeviceSpec& spec)
    {
    }
    
    // Buffer creation and deletion
    Buffer* DeviceMetal::CreateBuffer(std::size_t size, std::uint32_t flags)
    {
        // TODO: handle flags correctly
        if (size == 0) throw ExceptionMetal("Attempt to create a buffer with zero size");
        
        id<MTLBuffer> buffer = [m_device newBufferWithLength:(int)size options:MTLResourceStorageModeShared];
        return new BufferMetal(buffer);
    }
    
    Buffer* DeviceMetal::CreateBuffer(std::size_t size, std::uint32_t flags, void* initdata)
    {
        if (size == 0) throw ExceptionMetal("Attempt to create a buffer with zero size");
        
        // TODO: handle flags correctly
        id<MTLBuffer> buffer = [m_device newBufferWithBytes:initdata length:(int)size options:MTLResourceStorageModeShared];
        return new BufferMetal(buffer);
    }
    
    void DeviceMetal::DeleteBuffer(Buffer* buffer)
    {
        delete buffer;
    }
    
    // Data movement
    void DeviceMetal::ReadBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* dst, Event** e) const
    {
        auto buffer_metal = static_cast<BufferMetal const*>(buffer);
        
        auto data = static_cast<std::uint8_t*>([buffer_metal->m_buffer contents]);
        
        assert(data);
        
        std::memcpy(dst, data + offset, size);
        
        if (e) *e = CreateEventMetal();
    }
    
    void DeviceMetal::WriteBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* src, Event** e)
    {
        auto buffer_metal = static_cast<BufferMetal const*>(buffer);
        
        auto data = static_cast<std::uint8_t*>([buffer_metal->m_buffer contents]);
        
        assert(data);
        
        std::memcpy(data + offset, src, size);
        
        if (e) *e = CreateEventMetal();
    }
    
    // Buffer mapping
    void DeviceMetal::MapBuffer(Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, std::uint32_t map_type, void** mapdata, Event** e)
    {
        auto buffer_metal = static_cast<BufferMetal const*>(buffer);
        
        *mapdata = [buffer_metal->m_buffer contents];
        
        if (e) *e = CreateEventMetal();
    }
    
    void DeviceMetal::UnmapBuffer(Buffer const* buffer, std::uint32_t queue, void* mapdata, Event** e)
    {
        if (e) *e = CreateEventMetal();
    }
    
    // Kernel compilation
    Executable* DeviceMetal::CompileExecutable(char const* source_code, std::size_t size, char const* options)
    {
        NSString* ns_source_code = [NSString stringWithFormat:@"%s", source_code];
        NSError* error = nil;
        
        id<MTLLibrary> lib = [m_device newLibraryWithSource:ns_source_code options:nil error:&error];
        
        if (lib == nil)
        {
            NSString* desc = [error description];
            throw ExceptionMetal([desc UTF8String]);
        }
        
        return new ExecutableMetal(this, lib);
    }
    
    Executable* DeviceMetal::CompileExecutable(std::uint8_t const* binary_code, std::size_t size, char const* options)
    {
        return nullptr;
    }
    
    Executable* DeviceMetal::CompileExecutable(char const* filename,
                                  char const** headernames,
                                  int numheaders)
    {
        return nullptr;
    }
    
    void DeviceMetal::DeleteExecutable(Executable* executable)
    {
        delete executable;
    }
    
    // Executable management
    size_t DeviceMetal::GetExecutableBinarySize(Executable const* executable) const
    {
        return 0;
    }
    
    void DeviceMetal::GetExecutableBinary(Executable const* executable, std::uint8_t* binary) const
    {
    }
    
    // Execution
    void DeviceMetal::Execute(Function const* func, std::uint32_t queue, size_t global_size, size_t local_size, Event** e)
    {
        auto func_metal = static_cast<FunctionMetal const*>(func);
        
        // Create command buffer first
        id<MTLCommandBuffer> cmd_buffer = [m_cmdqueue commandBuffer];
        
        id<MTLComputeCommandEncoder> cmd_encoder = [cmd_buffer computeCommandEncoder];
        
        id<MTLComputePipelineState> state = [m_device newComputePipelineStateWithFunction:func_metal->GetKernel() error:nil];
        
        [cmd_encoder setComputePipelineState:state];
        
        
        for (auto iter = func_metal->m_args.cbegin(); iter != func_metal->m_args.cend(); ++iter)
        {
            auto buffer_metal = static_cast<BufferMetal const*>(iter->second.buffer);
            
            [cmd_encoder setBuffer:buffer_metal->m_buffer offset:0 atIndex:iter->first];
        }
        
        MTLSize threads_per_group = {local_size, 1, 1};
        MTLSize num_groups = { (global_size + local_size - 1) / local_size, 1, 1};
        
        [cmd_encoder dispatchThreadgroups:num_groups
                  threadsPerThreadgroup:threads_per_group];
        
        [cmd_encoder endEncoding];
        
        [cmd_buffer commit];
        
        // TODO: dirty hack to do explicit sync
        [cmd_buffer waitUntilCompleted];
    }
    
    // Events handling
    void DeviceMetal::WaitForEvent(Event* e)
    {
        e->Wait();
    }
    
    void DeviceMetal::WaitForMultipleEvents(Event** e, std::size_t num_events)
    {
        for (auto i = 0; i < num_events; ++i)
        {
            e[i]->Wait();
        }
    }
    
    void DeviceMetal::DeleteEvent(Event* e)
    {
        ReleaseEventMetal(static_cast<EventMetal*>(e));
    }
    
    // Queue management functions
    void DeviceMetal::Flush(std::uint32_t queue)
    {
        
    }
    
    void DeviceMetal::Finish(std::uint32_t queue)
    {
        
    }
    
    // Parallel prims handling
    bool DeviceMetal::HasBuiltinPrimitives() const
    {
        return false;
    }
    
    Primitives* DeviceMetal::CreatePrimitives() const
    {
        return nullptr;
    }
    
    
    void DeviceMetal::DeletePrimitives(Primitives* prims)
    {
        
    }
    
    
    CalcMetal::CalcMetal() = default;
    
    CalcMetal::~CalcMetal() = default;
    
    // Enumerate devices
    std::uint32_t CalcMetal::GetDeviceCount() const
    {
        return 1U;
    }
    
    // Get i-th device spec
    void CalcMetal::GetDeviceSpec(std::uint32_t idx, DeviceSpec& spec) const
    {
    }
    
    // Create the device with specified index
    Device* CalcMetal::CreateDevice(std::uint32_t idx) const
    {
        if (idx == 0)
        {
            return new DeviceMetal();
        }
        
        return nullptr;
    }
    
    // Delete the device
    void CalcMetal::DeleteDevice(Device* device)
    {
        delete device;
    }
    
    // Argument setters
    void FunctionMetal::SetArg(std::uint32_t idx, std::size_t arg_size, void* arg)
    {
        // Create temporary buffer
        auto buffer = m_device->CreateBuffer(arg_size, BufferType::kRead, arg);
        auto iter = m_args.find(idx);
        
        if (iter != m_args.cend())
        {
            if (iter->second.autorelease)
            {
                m_device->DeleteBuffer(const_cast<Buffer*>(iter->second.buffer));
            }
        }
        
        m_args[idx] = BufferHolder { buffer, true };
    }
    
    FunctionMetal::~FunctionMetal()
    {
        std::for_each(m_args.cbegin(), m_args.cend(), [this](std::pair<std::uint32_t, BufferHolder> const& v)
                      {
                          if (v.second.autorelease)
                          {
                              m_device->DeleteBuffer(const_cast<Buffer*>(v.second.buffer));
                          }
                      });
    }
    
}

#endif