#include "except_vk.h"
#if defined(USE_VULKAN)

#include <cassert>
#include "misc/debug.h"
#include "wrappers/device.h"
#include "wrappers/shader_module.h"
#include "wrappers/buffer.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/command_pool.h"
#include "wrappers/compute_pipeline_manager.h"
#include "wrappers/command_buffer.h"
#include "wrappers/fence.h"
#include "wrappers/queue.h"
#include "wrappers/pipeline_layout.h"
#include "wrappers/physical_device.h"
#include "misc/glsl_to_spirv.h"

#include "calc_common.h"
#include "buffer.h"
#include "event.h"
#include "executable.h"

#include "common_vk.h"
#include "buffer_vk.h"
#include "event_vk.h"
#include "executable_vk.h"
#include "function_vk.h"
#include "device_vk.h"


namespace Calc
{    

    void LogError(  const char* in_assertion_text,
                    const char* in_condition,
                    const char* in_file_name,
                    int in_line,
                    const char* in_comment )
    {
        printf( in_assertion_text, in_condition, in_file_name, in_line, in_comment );
        int to_set_breakpoint_here = 0;
    }

    void Abort()
    {
        int to_set_breakpoint_here = 0;
        abort();
    }


    // ctor
    DeviceVulkanw::DeviceVulkanw( Anvil::Device* in_new_device, bool in_use_compute_pipe ) :
         DeviceVulkan()
         , m_anvil_device( in_new_device )
         , m_is_command_buffer_recording( false )
         , m_use_compute_pipe( in_use_compute_pipe )
         , m_cpu_fence_id( 0 )
         , m_gpu_known_fence_id( 1 )
    {
        m_anvil_device->retain();

        for( auto& fence : m_anvil_fences )
        {
            fence.reset( new Anvil::Fence(m_anvil_device, true) );
        }
    }

    // dtor
    DeviceVulkanw::~DeviceVulkanw()
    {
        m_command_buffer.reset();

        for (auto& fence : m_anvil_fences) { fence.reset(); }

        m_anvil_device->release();
    }

    // Allocate CommandBuffer used to record compute commands
    bool DeviceVulkanw::InitializeVulkanResources()
    {
        const Anvil::QueueFamilyType familyType = m_use_compute_pipe ? Anvil::QUEUE_FAMILY_TYPE_COMPUTE : Anvil::QUEUE_FAMILY_TYPE_UNIVERSAL;

        auto cmd_pool = m_anvil_device->get_command_pool( familyType );

        // if device doesn't have the type of pool asked for, bail
        if(cmd_pool == nullptr) {
            return false;
        }

        m_command_buffer.reset( cmd_pool->alloc_primary_level_command_buffer() );

        return nullptr != m_command_buffer;
    }

    bool DeviceVulkanw::InitializeVulkanCommandBuffer(Anvil::CommandPool* cmd_pool)
    {
        m_command_buffer.reset(cmd_pool->alloc_primary_level_command_buffer());

        return nullptr != m_command_buffer;
    }

    // Return specification of the device
    void DeviceVulkanw::GetSpec( DeviceSpec& spec )
    {
        const Anvil::PhysicalDevice* device = m_anvil_device->get_physical_device();

        spec.name = device->get_device_properties().deviceName;
        spec.vendor = device->get_device_properties().deviceName;
        spec.type = DeviceType::kGpu;
        spec.sourceTypes = SourceType::kGLSL;

        uint64_t localMemory = 0;
        uint64_t hostMemory = 0;

        for (uint32_t i = 0; i < device->get_memory_properties().types.size(); ++i)
        {
            const Anvil::MemoryType& memoryType = device->get_memory_properties().types[i];
            if (memoryType.flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
            {
                localMemory += memoryType.heap_ptr->size;
            }

            else if (memoryType.flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
            {
                hostMemory += memoryType.heap_ptr->size;
            }
        }

        spec.global_mem_size = static_cast< std::size_t >(hostMemory);
        spec.local_mem_size = static_cast< std::size_t >(localMemory);
        spec.min_alignment = static_cast< std::uint32_t >(device->get_device_properties().limits.minMemoryMapAlignment);
        spec.max_alloc_size = static_cast< std::size_t >(hostMemory);
        spec.max_local_size = static_cast< std::size_t >(localMemory);

        spec.has_fp16 = device->is_device_extension_supported("GL_AMD_gpu_shader_half_float");
    }

    // Buffer creation and deletion
    Buffer* DeviceVulkanw::CreateBuffer( std::size_t size, std::uint32_t flags )
    {
        return CreateBuffer( size, flags, nullptr );
    }

    Buffer* DeviceVulkanw::CreateBuffer( std::size_t size, std::uint32_t flags, void* initdata )
    {
        if(size == 0 )
        {
            throw ExceptionVk("Buffer size of 0 isn't valid" );
            return nullptr;
        }
        const Anvil::QueueFamilyBits queueToUse = (true == m_use_compute_pipe) ?
                                                Anvil::QUEUE_FAMILY_COMPUTE_BIT :
                                                Anvil::QUEUE_FAMILY_GRAPHICS_BIT;

        Anvil::Buffer* newBuffer = new Anvil::Buffer( m_anvil_device
                                                        , size
                                                        , queueToUse
                                                        , VK_SHARING_MODE_EXCLUSIVE
                                                        , VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                                                        , true
                                                        , false
                                                        , initdata );

        return new BufferVulkan( newBuffer, false );
    }

    void DeviceVulkanw::DeleteBuffer( Buffer* buffer )
    {
        if ( nullptr != buffer )
        {
            delete buffer;
        }
    }

    // Data movement
    void DeviceVulkanw::ReadBuffer( Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* dst, Event** e ) const
    {
        if (nullptr != e) {
            *e = new EventVulkan(this);
        }

        BufferVulkan* vulkanBuffer = ConstCast<BufferVulkan>( buffer );

        // make sure GPU has stopped using this buffer
        WaitForFence(vulkanBuffer->m_fence_id);

        Anvil::Buffer* anvilBuffer = vulkanBuffer->GetAnvilBuffer();
        anvilBuffer->read( offset, size, dst );
    }

    void DeviceVulkanw::WriteBuffer( Buffer const* buffer, std::uint32_t queue, std::size_t offset, std::size_t size, void* src, Event** e )
    {
        if (nullptr != e) {
            *e = new EventVulkan(this);
        }

        BufferVulkan* vulkanBuffer = ConstCast<BufferVulkan>( buffer );

        // make sure GPU has stopped using this buffer
        WaitForFence(vulkanBuffer->m_fence_id);

        Anvil::Buffer* anvilBuffer = vulkanBuffer->GetAnvilBuffer();
        anvilBuffer->write( offset, size, src );
    }

    // Buffer mapping 
    void DeviceVulkanw::MapBuffer(   Buffer const* buffer,
                                    std::uint32_t queue,
                                    std::size_t offset,
                                    std::size_t size,
                                    std::uint32_t map_type,
                                    void** mapdata,
                                    Event** e )
    {
        BufferVulkan* vulkanBuffer = ConstCast<BufferVulkan>( buffer );

        // make sure GPU has stopped using this buffer
        WaitForFence(vulkanBuffer->m_fence_id);

        if( nullptr != e ) {
            *e = new EventVulkan(this);
        }

        // allocated a proxy buffer
        uint8_t* mappedMemory = new uint8_t[ size ];
        (*mapdata) = mappedMemory;

        if ( MapType::kMapRead == map_type )
        {
            // read the Vulkan buffer
            ReadBuffer( buffer, queue, offset, size, mappedMemory, e );
        }

        else if ( MapType::kMapWrite != map_type )
        {
            VK_EMPTY_IMPLEMENTATION;
        }

        vulkanBuffer->SetMappedMemory( mappedMemory, map_type, offset, size );

    }

    void DeviceVulkanw::UnmapBuffer( Buffer const* buffer, std::uint32_t queue, void* mapdata, Event** e )
    {
        // get the allocated proxy buffer
        BufferVulkan* vulkanBuffer = ConstCast<BufferVulkan>( buffer );

        if( nullptr != e ) {
            *e = new EventVulkan(this);
        }

        const MappedMemory& mappedMemory = vulkanBuffer->GetMappedMemory();

        Assert( mappedMemory.data == mapdata );

        if ( MapType::kMapWrite == mappedMemory.type )
        {
            // write the proxy buffer data to the Vulkan buffer
            WriteBuffer( buffer, queue, mappedMemory.offset, mappedMemory.size, mapdata, e );
        }

        delete[] mappedMemory.data;

        vulkanBuffer->SetMappedMemory( nullptr, 0, 0, 0 );
    }

    // Kernel compilation
    Executable* DeviceVulkanw::CompileExecutable( char const* source_code, std::size_t size, char const* options )
    {
        return new ExecutableVulkan( m_anvil_device, source_code, size, m_use_compute_pipe );
    }

    Executable* DeviceVulkanw::CompileExecutable( std::uint8_t const* binary_code, std::size_t size, char const* options )
    {
        VK_EMPTY_IMPLEMENTATION;
        return nullptr;
    }

    Executable* DeviceVulkanw::CompileExecutable( char const* inFilename, char const** inHeaderNames, int inHeadersNum, char const* options)
    {
        return new ExecutableVulkan( m_anvil_device, inFilename, m_use_compute_pipe );
    }

    void DeviceVulkanw::DeleteExecutable( Executable* executable )
    {
        delete executable;
    }

    // Executable management
    size_t DeviceVulkanw::GetExecutableBinarySize( Executable const* executable ) const
    {
        VK_EMPTY_IMPLEMENTATION;
        return 0;
    }

    void DeviceVulkanw::GetExecutableBinary( Executable const* executable, std::uint8_t* binary ) const
    {
        VK_EMPTY_IMPLEMENTATION;
    }

    // Get queue, the execution of vulkan shaders can be done through the compute queue or the graphic queue
    Anvil::Queue* DeviceVulkanw::GetQueue() const
    {
        Anvil::Queue* toReturn = (true == m_use_compute_pipe) ?
                                 m_anvil_device->get_compute_queue( 0 ) :
                                 m_anvil_device->get_universal_queue( 0 );
        return toReturn;
    }

    // To start recording Vulkan commands to the CommandBuffer
    void DeviceVulkanw::StartRecording()
    {
        Assert( false == m_is_command_buffer_recording );

        AllocNextFenceId();
        const auto fence = GetFence( m_cpu_fence_id );
        fence->reset();

        m_is_command_buffer_recording = true;
        m_command_buffer->reset( false );
        m_command_buffer->start_recording( true, false );
    }

    // Finish recording
    void DeviceVulkanw::EndRecording( bool in_wait_till_completed, Event** out_event )
    {

        // execute CommandBuffer if either wait_till_completed
        // TODO buffer n exec's per commit
        CommitCommandBuffer( in_wait_till_completed );

        if ( nullptr != out_event )
        {
            *out_event = new EventVulkan( this );
        }
    }

    // Execute CommandBuffer
    void DeviceVulkanw::CommitCommandBuffer( bool in_wait_till_completed )
    {
        const auto fence = GetFence( m_cpu_fence_id );

        m_is_command_buffer_recording = false;
        m_command_buffer->stop_recording();

        GetQueue()->submit_command_buffer( m_command_buffer.get(), in_wait_till_completed, fence );
    }

    // Execution Not thread safe
    void DeviceVulkanw::Execute( Function const* func, std::uint32_t queue, size_t global_size, size_t local_size, Event** e )
    {
        FunctionVulkan* vulkan_function = ConstCast<FunctionVulkan>( func );

        uint32_t number_of_parameters = (uint32_t)( vulkan_function->GetParameters().size() );

        // get the Function's descriptor set group
        Anvil::DescriptorSetGroup* new_descriptor_set = vulkan_function->GetDescriptorSetGroup();

        // if it's empty, this is 1st run of the Function so we have to create it
        if ( nullptr == new_descriptor_set )
        {
            // allocate it through Anvil
            new_descriptor_set = new Anvil::DescriptorSetGroup( m_anvil_device, false, 1 );

            // add bindings and items (Buffers) to the new DSG
            for ( uint32_t i = 0; i < number_of_parameters; ++i )
            {
                const Buffer* parameter = vulkan_function->GetParameters()[ i ];
                BufferVulkan* buffer = ConstCast<BufferVulkan>( parameter );
                new_descriptor_set->add_binding( 0, i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT );
            }

            // set it to the Function to be reused during any subsequent run
            vulkan_function->SetDescriptorSetGroup( new_descriptor_set );
        }

        // bind new items (Buffers), releasing the old ones
        for ( uint32_t i = 0; i < number_of_parameters; ++i )
        {
            const Buffer* parameter = vulkan_function->GetParameters()[ i ];
            BufferVulkan* buffer = ConstCast<BufferVulkan>( parameter );
            new_descriptor_set->set_binding_item( 0, i, buffer->GetAnvilBuffer() );

        }

        // get the Function's pipeline
        Anvil::ComputePipelineID pipeline_id = vulkan_function->GetPipelineID();

        // if it is invalid, this is 1st run of the Function so we have to create it
        if ( ~0u == pipeline_id )
        {
            // create the pipeline through Anvil with the shader module as a parameter
            m_anvil_device->get_compute_pipeline_manager()->add_regular_pipeline( false, false, vulkan_function->GetFunctionEntryPoint(), &pipeline_id );

            // attach the DSG to it
            m_anvil_device->get_compute_pipeline_manager()->attach_dsg_to_pipeline( pipeline_id, new_descriptor_set );

            // remember the pipeline for any seubsequent run
            vulkan_function->SetPipelineID( pipeline_id );
        }

        // indicate we'll be recording Vulkan commands to the CommandBuffer from now on
        StartRecording();

        // attach pipeline
        m_command_buffer->record_bind_pipeline( VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_id );

        Anvil::PipelineLayout* pipeline_layout = m_anvil_device->get_compute_pipeline_manager()->get_compute_pipeline_layout( pipeline_id );
        Anvil::DescriptorSet* descriptor_set = new_descriptor_set->get_descriptor_set( 0 );

        // attach layout and 0 descriptor set (we don't use any other set currently)
        m_command_buffer->record_bind_descriptor_sets( VK_PIPELINE_BIND_POINT_COMPUTE,
                                                    pipeline_layout,
                                                    0,
                                                    1,
                                                    &descriptor_set,
                                                    0,
                                                    nullptr );

        // set memory barriers 
        for ( uint32_t i = 0; i < number_of_parameters; ++i )
        {
            const Buffer* parameter = vulkan_function->GetParameters()[ i ];
            BufferVulkan* buffer = ConstCast<BufferVulkan>( parameter );

            Anvil::BufferBarrier bufferBarrier( VK_ACCESS_HOST_WRITE_BIT,
                                                VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
                                                GetQueue()->get_queue_family_index(),
                                                GetQueue()->get_queue_family_index(),
                                                buffer->GetAnvilBuffer(),
                                                0,
                                                buffer->GetSize() );

            m_command_buffer->record_pipeline_barrier( VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                                    VK_FALSE,
                                                    0, nullptr,
                                                    1, &bufferBarrier,
                                                    0, nullptr );
            // tell buffer that we are used by this submit
            buffer->SetFenceId( GetFenceId() );

        }

        // dispatch the Function's shader module
        m_command_buffer->record_dispatch( (uint32_t)global_size, 1, 1 );

        // end recording
        EndRecording( false, e );

        // remove references to buffers. they were already referenced by the CommandBuffer.
        vulkan_function->UnreferenceParametersBuffers();
    }

    // Events handling
    void DeviceVulkanw::WaitForEvent( Event* e )
    {
        e->Wait();

    }

    void DeviceVulkanw::WaitForMultipleEvents( Event** e, std::size_t num_events )
    {
        if(nullptr == e)
            return;

        // TODO optimize
        for (size_t i = 0; i < num_events; ++i)
        {
            (*e)[i].Wait();
        }
    }

    void DeviceVulkanw::DeleteEvent( Event* e )
    {
        if ( nullptr != e )
        {
            delete e;
        }
    }

    // Queue management functions
    void DeviceVulkanw::Flush( std::uint32_t queue )
    {
        if(false == m_is_command_buffer_recording)
        {
            StartRecording();
        }
        EndRecording(true, nullptr);
    }

    void DeviceVulkanw::Finish( std::uint32_t queue )
    {
        // TODO work out semantics of these two
        Flush(queue);
    }

    bool DeviceVulkanw::HasBuiltinPrimitives() const
    {
        VK_EMPTY_IMPLEMENTATION;
        return false;
    }

    Primitives* DeviceVulkanw::CreatePrimitives() const
    {
        VK_EMPTY_IMPLEMENTATION;
        return nullptr;
    }

    void DeviceVulkanw::DeletePrimitives( Primitives* prims )
    {
        VK_EMPTY_IMPLEMENTATION;
    }

    uint64_t DeviceVulkanw::AllocNextFenceId() {
        // stall if we have run out of fences to use
        while( m_cpu_fence_id >= m_gpu_known_fence_id + NUM_FENCE_TRACKERS)
        {
            WaitForFence(m_gpu_known_fence_id+1);
        }

        return m_cpu_fence_id.fetch_add(1);
    }

    void DeviceVulkanw::WaitForFence( uint64_t id ) const
    {
        AssertEx( id < m_gpu_known_fence_id + NUM_FENCE_TRACKERS,
                "CPU too far ahead of GPU" );

        while(HasFenceBeenPassed(id) == false ) {
            vkWaitForFences(m_anvil_device->get_device_vk(), 1,
                            GetFence(m_gpu_known_fence_id)->get_fence_ptr(),
                            VK_TRUE,
                            UINT64_MAX);

            // don't known id update until wait has finished
            m_gpu_known_fence_id++;
        }
    }

}

#endif // USE_VULKAN