#include "calc_vkw.h"
#if defined(USE_VULKAN)

#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include <vulkan/vk_layer.h>

#include "misc/debug.h"
#include "wrappers/instance.h"

#include "common_vk.h"
#include "except_vk.h"
#include "calc_vk.h"
#include "device_vk.h"
#include "device_vkw.h"

namespace Calc
{

    // Whether to use Compute pipe for compute shaders. Setting to false will cause RadeonRays to use Graphics pipe for the shaders.
    bool g_use_compute_pipe = false;

    // Debug callback called when any of Vulkan layers report an error
    VkBool32 DebugCallbackProc( VkDebugReportFlagsEXT      message_flags,
                                  VkDebugReportObjectTypeEXT object_type,
                                  const char*                layer_prefix,
                                  const char*                message,
                                  void*                      user_arg )
    {
        printf( "VK:- %s %s\n", layer_prefix, message);
        int to_set_breakpoint_here = 0;
        /* Returning false will forward the Vulkan API call, which generated this call-back,
        * to the driver. */
        return false;
    }

    DeviceVulkan* CreateDeviceFromVulkan(Anvil::Device* device, Anvil::CommandPool* cmd_pool)
    {
        // TODO graphics or compute pipe selection
        DeviceVulkanw* toReturn = new DeviceVulkanw(device, true);
        bool initVkOk = (nullptr != toReturn) ? toReturn->InitializeVulkanCommandBuffer(cmd_pool) : false;
        if (initVkOk == false)
        {
            delete toReturn;
            return nullptr;
        }
        else
        {
            return toReturn;
        }
    }
    void CalcVulkanw::InitializeInstance()
    {
        m_anvil_instance = new Anvil::Instance( "RadeonRays", "RadeonRays", &DebugCallbackProc, nullptr );
    }

    CalcVulkanw::CalcVulkanw() :
        m_anvil_instance( nullptr )
    {
        try
        {
            InitializeInstance();
        }
        catch ( const ExceptionVk& exception )
        {
            throw ExceptionVk( exception.what() );
        }
    }

    CalcVulkanw::~CalcVulkanw()
    {
        if (m_anvil_instance) { m_anvil_instance->release(); }
    }

    // Enumerate devices 
    std::uint32_t CalcVulkanw::GetDeviceCount() const
    {
        return nullptr != m_anvil_instance ? m_anvil_instance->get_n_physical_devices() : 0;
    }

    // Get i-th device spec
    void CalcVulkanw::GetDeviceSpec( std::uint32_t idx, DeviceSpec& spec ) const
    {
        if ( idx < GetDeviceCount() )
        {
            const Anvil::PhysicalDevice* device = m_anvil_instance->get_physical_device( idx );

            spec.name = device->get_device_properties().deviceName;
            spec.vendor = device->get_device_properties().deviceName;
            spec.type = DeviceType::kGpu;
            spec.sourceTypes = SourceType::kGLSL;

            uint64_t localMemory = 0;
            uint64_t hostMemory = 0;

            for ( uint32_t i = 0; i < device->get_memory_properties().types.size(); ++i )
            {
                const Anvil::MemoryType& memoryType = device->get_memory_properties().types[ i ];
                if ( memoryType.flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT )
                {
                    localMemory += memoryType.heap_ptr->size;
                }

                else if ( memoryType.flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT )
                {
                    hostMemory += memoryType.heap_ptr->size;
                }
            }

            spec.global_mem_size = static_cast< std::size_t >(hostMemory);
            spec.local_mem_size = static_cast< std::size_t >(localMemory);
            spec.min_alignment = static_cast< std::uint32_t >( device->get_device_properties().limits.minMemoryMapAlignment );
            spec.max_alloc_size = static_cast< std::size_t >(hostMemory);
            spec.max_local_size = static_cast< std::size_t >(localMemory);
        }

        else
        {
            throw ExceptionVk( "Index is out of bounds" );
        }
    }

    // Create the device with specified index
    Device* CalcVulkanw::CreateDevice( std::uint32_t idx ) const
    {
        Device* toReturn = nullptr;

        if ( idx < GetDeviceCount() )
        {
            Anvil::PhysicalDevice* physical_device = m_anvil_instance->get_physical_device( idx );

            std::vector<const char*> required_extension_names;
            std::vector<const char*> required_layer_names;

//            required_extension_names.push_back( "VK_KHR_swapchain" );

            Anvil::Device* newDevice = new Anvil::Device( physical_device, required_extension_names, required_layer_names, false, true );

            // see if we have a compute device available
            DeviceVulkanw* toReturn = new DeviceVulkanw( newDevice, true );
            bool initVkOk = (nullptr != toReturn) ? toReturn->InitializeVulkanResources() : false;

            // if not and we didn't specificy a compute pipe, try a graphics device
            if( (initVkOk == false) && (g_use_compute_pipe == false) )
            {
                delete toReturn;
                toReturn = new DeviceVulkanw( newDevice, false );
                initVkOk = (nullptr != toReturn) ? toReturn->InitializeVulkanResources() : false;
            }

            if( initVkOk == false )
            {
                throw ExceptionVk( "No valid Vulkan device found" );
            }

            // pass ownership to DeviceVk
            newDevice->release();            

            return toReturn;
        }

        else
        {
            throw ExceptionVk( "Index is out of bounds" );
        }

        return toReturn;
    }

    Device* CalcVulkanw::CreateDevice(Anvil::Device* device, Anvil::CommandPool* cmd_pool)
    {
        // TODO graphics or compute pipe selection
        DeviceVulkanw* toReturn = new DeviceVulkanw(device, true);
        bool initVkOk = (nullptr != toReturn) ? toReturn->InitializeVulkanCommandBuffer(cmd_pool) : false;
        if (initVkOk == false)
        {
            delete toReturn;
            return nullptr;
        } else
        {
            return toReturn;
        }
    }    

    // Delete the device
    void CalcVulkanw::DeleteDevice( Device* device )
    {
        delete device;
    }
}

#endif // USE_VULKAN
