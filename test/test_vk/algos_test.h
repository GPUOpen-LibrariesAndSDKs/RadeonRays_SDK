#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>

#include "common.h"
#include "gtest/gtest.h"
#include "vlk/buffer.h"
#include "vlk/gpu_helper.h"
#include "vlk/radix_sort.h"
#include "vlk/scan.h"
#include "vlk/shader_manager.h"
#include "vulkan/vulkan.hpp"

//#define USE_RENDERDOC
#ifdef USE_RENDERDOC
#define NOMINMAX
#include <windows.h>

#include "C:\\Program Files\\RenderDoc\\renderdoc_app.h"
#endif

using namespace std::chrono;
using namespace rt::vulkan;

#ifdef _WIN32
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

namespace
{
static constexpr uint32_t kKeysCount    = 1024u * 1024u;
static constexpr uint32_t kKeysBigCount = 1024u * 1024u * 16u;
}  // namespace

class AlgosTest : public ::testing::Test
{
    static auto constexpr VK_VENDOR_ID_AMD    = 0x1002;
    static auto constexpr VK_VENDOR_ID_NVIDIA = 0x10de;
    static auto constexpr VK_VENDOR_ID_INTEL  = 0x8086;

public:
    void SetUp() override;

    void TearDown() override {}

    // Vulkan data.
    VkScopedObject<VkInstance>    instance_;
    VkScopedObject<VkDevice>      device_;
    VkScopedObject<VkCommandPool> command_pool_;
    VkScopedObject<VkQueryPool>   query_pool_;
    VkPhysicalDevice              phdevice_;
    uint32_t                      queue_family_index_;
    float                         timestamp_period_;
#ifdef USE_RENDERDOC
    RENDERDOC_API_1_4_0* rdoc_api_ = NULL;
#endif

    std::string device_name;
};

void AlgosTest::SetUp()
{
#ifdef USE_RENDERDOC
    // At init, on windows
    if (HMODULE mod = LoadLibraryA("C:\\Program Files\\RenderDoc\\renderdoc.dll"))
    {
        pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI)GetProcAddress(mod, "RENDERDOC_GetAPI");
        int               ret              = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_4_0, (void**)&rdoc_api_);
        assert(ret == 1);
    }
#endif

    VkApplicationInfo app_info;
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = NULL;
    app_info.pApplicationName   = "RadeonRays Test";
    app_info.applicationVersion = 1;
    app_info.pEngineName        = "RadeonRays";
    app_info.engineVersion      = 1;
    app_info.apiVersion         = VK_API_VERSION_1_2;

    const std::vector<const char*> layers              = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> instance_extensions = {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME};

    VkInstanceCreateInfo instance_info;
    instance_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pNext                   = NULL;
    instance_info.flags                   = 0;
    instance_info.pApplicationInfo        = &app_info;
    instance_info.enabledExtensionCount   = (uint32_t)instance_extensions.size();
    instance_info.ppEnabledExtensionNames = instance_extensions.data();

#ifdef _DEBUG
    instance_info.enabledLayerCount   = 1u;
    instance_info.ppEnabledLayerNames = layers.data();
#else
    instance_info.enabledLayerCount        = 0u;
    instance_info.ppEnabledLayerNames      = nullptr;
#endif

    VkInstance instance = nullptr;
    VkResult   res      = vkCreateInstance(&instance_info, nullptr, &instance);

    if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
    {
        throw std::runtime_error("Cannot find a compatible Vulkan ICD\n");
    } else if (res)
    {
        throw std::runtime_error("Unknown error\n");
    }

    instance_ = VkScopedObject<VkInstance>(instance, [](VkInstance instance) { vkDestroyInstance(instance, nullptr); });

    // Enumerate devices
    auto gpu_count = 0u;
    vkEnumeratePhysicalDevices(instance, &gpu_count, nullptr);

    if (gpu_count == 0)
    {
        throw std::runtime_error("No compatible devices found\n");
    }

    std::vector<VkPhysicalDevice> gpus(gpu_count);
    res = vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());

    float                   queue_priority = 0.f;
    VkDeviceQueueCreateInfo queue_create_info;
    queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info.pNext            = nullptr;
    queue_create_info.flags            = 0;
    queue_create_info.queueCount       = 1u;
    queue_create_info.pQueuePriorities = &queue_priority;

    auto queue_family_count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queue_family_count, nullptr);

    VkPhysicalDeviceProperties physical_device_properties;
    vkGetPhysicalDeviceProperties(gpus[0], &physical_device_properties);
    device_name = std::string(physical_device_properties.deviceName);

    timestamp_period_ = physical_device_properties.limits.timestampPeriod;

    std::vector<VkQueueFamilyProperties> queue_props(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpus[0], &queue_family_count, queue_props.data());

    // Look for a queue supporing both compute and transfer
    bool found = false;
    for (unsigned int i = 0; i < queue_family_count; i++)
    {
        if (queue_props[i].queueFlags & (VK_QUEUE_COMPUTE_BIT))
        {
            queue_create_info.queueFamilyIndex = i;
            found                              = true;
            break;
        }
    }

    if (!found)
    {
        throw std::runtime_error("No compute/transfer queues found\n");
    }

    phdevice_           = gpus[0];
    queue_family_index_ = queue_create_info.queueFamilyIndex;

    std::vector<const char*> extensions;

    if (physical_device_properties.vendorID == VK_VENDOR_ID_NVIDIA)
    {
        extensions.push_back(VK_NV_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME);
    } else if (physical_device_properties.vendorID == VK_VENDOR_ID_AMD)
    {
        extensions.push_back(VK_AMD_SHADER_BALLOT_EXTENSION_NAME);
    }
    extensions.push_back(VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME);
    extensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
    extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

    VkDeviceCreateInfo device_create_info;
    device_create_info.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.flags                = 0;
    device_create_info.queueCreateInfoCount = 1u;
    device_create_info.pQueueCreateInfos    = &queue_create_info;
#ifdef _DEBUG
    device_create_info.enabledLayerCount   = 1u;
    device_create_info.ppEnabledLayerNames = layers.data();
#else
    device_create_info.enabledLayerCount   = 0u;
    device_create_info.ppEnabledLayerNames = nullptr;
#endif

    device_create_info.enabledExtensionCount   = (std::uint32_t)extensions.size();
    device_create_info.ppEnabledExtensionNames = extensions.data();
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(phdevice_, &features);

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures{};
    physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    physicalDeviceDescriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
    physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray                     = VK_TRUE;
    physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount   = VK_TRUE;
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};

    physicalDeviceFeatures2.sType       = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    physicalDeviceFeatures2.features    = features;
    physicalDeviceFeatures2.pNext       = &physicalDeviceDescriptorIndexingFeatures;
    device_create_info.pEnabledFeatures = nullptr;
    device_create_info.pNext            = &physicalDeviceFeatures2;

    VkDevice device = nullptr;
    res             = vkCreateDevice(phdevice_, &device_create_info, nullptr, &device);

    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Vulkan device\n");
    }

    device_ = VkScopedObject<VkDevice>(device, [](VkDevice device) { vkDestroyDevice(device, nullptr); });

    VkCommandPoolCreateInfo command_pool_info;
    command_pool_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.pNext            = nullptr;
    command_pool_info.queueFamilyIndex = queue_create_info.queueFamilyIndex;
    command_pool_info.flags            = 0;

    VkCommandPool command_pool = nullptr;
    res                        = vkCreateCommandPool(device_.get(), &command_pool_info, nullptr, &command_pool);

    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create command pool\n");
    }

    command_pool_ = VkScopedObject<VkCommandPool>(command_pool, [device = device_.get()](VkCommandPool command_pool) {
        vkDestroyCommandPool(device, command_pool, nullptr);
    });
    VkQueryPoolCreateInfo query_pool_info;
    query_pool_info.sType              = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    query_pool_info.pNext              = nullptr;
    query_pool_info.queryType          = VK_QUERY_TYPE_TIMESTAMP;
    query_pool_info.queryCount         = 2;
    query_pool_info.pipelineStatistics = 0;
    query_pool_info.flags              = 0;

    VkQueryPool query_pool = nullptr;

    res = vkCreateQueryPool(device, &query_pool_info, nullptr, &query_pool);

    if (res != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create query pool\n");
    }

    query_pool_ = VkScopedObject<VkQueryPool>(
        query_pool, [device = device_.get()](VkQueryPool pool) { vkDestroyQueryPool(device, pool, nullptr); });
}

inline std::vector<uint32_t> generate_data(size_t size)
{
    static std::uniform_int_distribution<uint32_t> distribution(0u, 1000u);
    static std::default_random_engine                generator;

    std::vector<uint32_t> data(size);
    std::generate(data.begin(), data.end(), []() { return distribution(generator); });
    return data;
}

TEST_F(AlgosTest, ScanTest)
{
    using algorithm::Scan;
    auto    device = device_.get();
    VkQueue queue  = nullptr;
    vkGetDeviceQueue(device_.get(), queue_family_index_, 0, &queue);

    auto          helper = std::make_shared<GpuHelper>(device, phdevice_, queue, queue_family_index_);
    ShaderManager shader_mngr(device);
    Scan          scan_kernel(helper, shader_mngr);

    auto scratch_size   = scan_kernel.GetScratchDataSize(kKeysCount);
    auto scratch_buffer = helper->CreateScratchDeviceBuffer(scratch_size);

    auto            to_scan           = generate_data(kKeysCount);
    AllocatedBuffer keys_buffer       = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, to_scan);
    auto            buffer_size       = to_scan.size() * sizeof(uint32_t);
    AllocatedBuffer output_buffer     = helper->CreateScratchDeviceBuffer(buffer_size);
    AllocatedBuffer output_cpu_buffer = helper->CreateStagingBuffer(buffer_size);

    helper->WithPrimaryCommandBuffer([&](vk::CommandBuffer scan_cmd_buffer) {
        scan_kernel(scan_cmd_buffer,
                    keys_buffer.buffer,
                    vk::DeviceSize(0u),
                    vk::DeviceSize(buffer_size),
                    output_buffer.buffer,
                    vk::DeviceSize(0u),
                    vk::DeviceSize(buffer_size),
                    scratch_buffer.buffer,
                    vk::DeviceSize(0u),
                    kKeysCount);
        scan_cmd_buffer.copyBuffer(
            output_buffer.buffer, output_cpu_buffer.buffer, vk::BufferCopy(0, 0, vk::DeviceSize(buffer_size)));
    });

    std::vector<uint32_t> scanned_cpu;
    scanned_cpu.reserve(kKeysCount);
    std::exclusive_scan(to_scan.begin(), to_scan.end(), std::back_inserter(scanned_cpu), 0);
    {
        uint32_t* mapped_ptr = output_cpu_buffer.Map<uint32_t>();
        for (const auto scanned_value : scanned_cpu)
        {
            ASSERT_EQ(*(mapped_ptr++), scanned_value);
        }
        output_cpu_buffer.Unmap();
    }
    keys_buffer.Destroy();
    output_buffer.Destroy();
    output_cpu_buffer.Destroy();
    scratch_buffer.Destroy();
}

TEST_F(AlgosTest, SortTest)
{
    using algorithm::RadixSortKeyValue;
    auto    device = device_.get();
    VkQueue queue  = nullptr;
    vkGetDeviceQueue(device_.get(), queue_family_index_, 0, &queue);

    auto              helper = std::make_shared<GpuHelper>(device, phdevice_, queue, queue_family_index_);
    ShaderManager     shader_mngr(device);
    RadixSortKeyValue sort_kernel(helper, shader_mngr);

    auto            scratch_size   = sort_kernel.GetScratchDataSize(kKeysCount);
    AllocatedBuffer scratch_buffer = helper->CreateScratchDeviceBuffer(scratch_size);

    auto            to_sort            = generate_data(kKeysCount);
    AllocatedBuffer keys_buffer        = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, to_sort);
    AllocatedBuffer vals_buffer        = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, to_sort);
    auto            buffer_size        = to_sort.size() * sizeof(uint32_t);
    AllocatedBuffer output_keys_buffer = helper->CreateScratchDeviceBuffer(buffer_size);
    AllocatedBuffer output_vals_buffer = helper->CreateScratchDeviceBuffer(scratch_size);
    AllocatedBuffer output_keys_cpu_buffer = helper->CreateStagingBuffer(buffer_size);
    helper->WithPrimaryCommandBuffer([&](vk::CommandBuffer scan_cmd_buffer) {
        sort_kernel(scan_cmd_buffer,
                    keys_buffer.buffer,
                    vk::DeviceSize(0u),
                    vk::DeviceSize(buffer_size),
                    output_keys_buffer.buffer,
                    vk::DeviceSize(0u),
                    vk::DeviceSize(buffer_size),
                    vals_buffer.buffer,
                    vk::DeviceSize(0u),
                    vk::DeviceSize(buffer_size),
                    output_vals_buffer.buffer,
                    vk::DeviceSize(0u),
                    vk::DeviceSize(buffer_size),
                    scratch_buffer.buffer,
                    vk::DeviceSize(0u),
                    kKeysCount);
        scan_cmd_buffer.copyBuffer(output_keys_buffer.buffer,
                                   output_keys_cpu_buffer.buffer,
                                   vk::BufferCopy(0, 0, vk::DeviceSize(buffer_size)));
    });

    std::sort(to_sort.begin(), to_sort.end());
    {
        uint32_t* mapped_ptr = output_keys_cpu_buffer.Map<uint32_t>();
        for (const auto sorted_key : to_sort)
        {
            auto val = *mapped_ptr;
            ASSERT_EQ(val, sorted_key);
            mapped_ptr++;
        }
        output_keys_cpu_buffer.Unmap();
    }

    keys_buffer.Destroy();
    vals_buffer.Destroy();
    output_keys_buffer.Destroy();
    output_vals_buffer.Destroy();
    output_keys_cpu_buffer.Destroy();
    scratch_buffer.Destroy();
}

//#define PERFORMANCE_TESTING

#ifdef PERFORMANCE_TESTING
TEST_F(AlgosTest, ScanTest4Perf)
{
    using algorithm::Scan;
    auto    device = device_.get();
    VkQueue queue  = nullptr;
    vkGetDeviceQueue(device_.get(), queue_family_index_, 0, &queue);

    auto          helper = std::make_shared<GpuHelper>(device, phdevice_, queue, queue_family_index_);
    ShaderManager shader_mngr(device);
    Scan          scan_kernel(helper, shader_mngr);

    auto scratch_size   = scan_kernel.GetScratchDataSize(kKeysBigCount);
    auto scratch_buffer = helper->CreateScratchDeviceBuffer(scratch_size);

    auto            to_scan       = generate_data(kKeysBigCount);
    AllocatedBuffer keys_buffer   = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, to_scan);
    auto            buffer_size   = to_scan.size() * sizeof(uint32_t);
    AllocatedBuffer output_buffer = helper->CreateScratchDeviceBuffer(buffer_size);
    helper->WithPrimaryCommandBuffer([&](vk::CommandBuffer scan_cmd_buffer) {
        scan_kernel(scan_cmd_buffer,
                    keys_buffer.buffer,
                    vk::DeviceSize(0u),
                    vk::DeviceSize(buffer_size),
                    output_buffer.buffer,
                    vk::DeviceSize(0u),
                    vk::DeviceSize(buffer_size),
                    scratch_buffer.buffer,
                    vk::DeviceSize(0u),
                    kKeysBigCount);
        vkCmdResetQueryPool(scan_cmd_buffer, query_pool_.get(), 0, 2);
        vkCmdWriteTimestamp(scan_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_.get(), 0);
        for (auto i = 0; i < 10; i++)
        {
            scan_kernel(scan_cmd_buffer,
                        keys_buffer.buffer,
                        vk::DeviceSize(0u),
                        vk::DeviceSize(buffer_size),
                        output_buffer.buffer,
                        vk::DeviceSize(0u),
                        vk::DeviceSize(buffer_size),
                        scratch_buffer.buffer,
                        vk::DeviceSize(0u),
                        kKeysBigCount);
        }
        vkCmdWriteTimestamp(scan_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_.get(), 1);
    });
    std::uint64_t time[2];
    vkGetQueryPoolResults(device_.get(),
                          query_pool_.get(),
                          0,
                          2,
                          2 * sizeof(std::uint64_t),
                          time,
                          sizeof(std::uint64_t),
                          VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    float scan_time = (time[1] - time[0]) * timestamp_period_ / 1e6f;
    scan_time /= 10.f;

    std::cout << "Avg execution time: " << scan_time << "ms\n";
    std::cout << "Throughput: " << (kKeysBigCount) / (scan_time * 1e-3) * 1e-6 << " MKeys/s\n";

    keys_buffer.Destroy();
    output_buffer.Destroy();
    scratch_buffer.Destroy();
}

TEST_F(AlgosTest, SortTest4Perf)
{
    using algorithm::RadixSortKeyValue;
    auto    device = device_.get();
    VkQueue queue  = nullptr;
    vkGetDeviceQueue(device_.get(), queue_family_index_, 0, &queue);

    auto              helper = std::make_shared<GpuHelper>(device, phdevice_, queue, queue_family_index_);
    ShaderManager     shader_mngr(device);
    RadixSortKeyValue sort_kernel(helper, shader_mngr);

    auto            scratch_size   = sort_kernel.GetScratchDataSize(kKeysBigCount);
    AllocatedBuffer scratch_buffer = helper->CreateScratchDeviceBuffer(scratch_size);

    auto            to_sort            = generate_data(kKeysBigCount);
    AllocatedBuffer keys_buffer        = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, to_sort);
    AllocatedBuffer vals_buffer        = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, to_sort);
    auto            buffer_size        = to_sort.size() * sizeof(uint32_t);
    AllocatedBuffer output_keys_buffer = helper->CreateScratchDeviceBuffer(buffer_size);
    AllocatedBuffer output_vals_buffer = helper->CreateScratchDeviceBuffer(scratch_size);

    helper->WithPrimaryCommandBuffer([&](vk::CommandBuffer scan_cmd_buffer) {
        vkCmdResetQueryPool(scan_cmd_buffer, query_pool_.get(), 0, 2);
        vkCmdWriteTimestamp(scan_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_.get(), 0);
        for (auto i = 0; i < 10; i++)
        {
            sort_kernel(scan_cmd_buffer,
                        keys_buffer.buffer,
                        vk::DeviceSize(0u),
                        vk::DeviceSize(buffer_size),
                        output_keys_buffer.buffer,
                        vk::DeviceSize(0u),
                        vk::DeviceSize(buffer_size),
                        vals_buffer.buffer,
                        vk::DeviceSize(0u),
                        vk::DeviceSize(buffer_size),
                        output_vals_buffer.buffer,
                        vk::DeviceSize(0u),
                        vk::DeviceSize(buffer_size),
                        scratch_buffer.buffer,
                        vk::DeviceSize(0u),
                        kKeysBigCount);
        }
        vkCmdWriteTimestamp(scan_cmd_buffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_.get(), 1);
    });
    std::uint64_t time[2];
    vkGetQueryPoolResults(device_.get(),
                          query_pool_.get(),
                          0,
                          2,
                          2 * sizeof(std::uint64_t),
                          time,
                          sizeof(std::uint64_t),
                          VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
    float sort_time = (time[1] - time[0]) * timestamp_period_ / 1e6f;
    sort_time /= 10.f;

    std::cout << "Avg execution time: " << sort_time << "ms\n";
    std::cout << "Throughput: " << (kKeysBigCount) / (sort_time * 1e-3) * 1e-6 << " MKeys/s\n";

    keys_buffer.Destroy();
    vals_buffer.Destroy();
    output_keys_buffer.Destroy();
    output_vals_buffer.Destroy();
    scratch_buffer.Destroy();
}
#endif