#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <tuple>

#include "common.h"
#include "gtest/gtest.h"
#include "mesh_data.h"
#include "vlk/buffer.h"
#include "vlk/gpu_helper.h"
#include "vlk/hlbvh_builder.h"
#include "vlk/restructure_hlbvh.h"
#include "vlk/shader_manager.h"
#include "vlk/update_hlbvh.h"
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

class HlbvhTest : public ::testing::Test
{
    static auto constexpr VK_VENDOR_ID_AMD    = 0x1002;
    static auto constexpr VK_VENDOR_ID_NVIDIA = 0x10de;
    static auto constexpr VK_VENDOR_ID_INTEL  = 0x8086;

public:
    void SetUp() override;

    void        TearDown() override {}
    static bool CheckConstistency(bvh_utils::VkBvhNode* nodes);

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

bool HlbvhTest::CheckConstistency(bvh_utils::VkBvhNode* nodes)
{
    std::queue<std::tuple<uint32_t, uint32_t>> q;
    q.push({0, bvh_utils::kInvalidID});
    while (!q.empty())
    {
        auto current = q.front();
        q.pop();
        auto addr   = std::get<0>(current);
        auto parent = std::get<1>(current);

        auto node = nodes[addr];
        auto aabb = GetAabb(node);
        // check topology consistency
        if (parent != node.parent)
        {
            return false;
        }

        if (node.child0 != bvh_utils::kInvalidID)
        {
            if (!aabb.Includes(GetAabb(nodes[node.child0])) || !aabb.Includes(GetAabb(nodes[node.child1])))
            {
                return false;
            }

            q.push({node.child0, addr});
            q.push({node.child1, addr});
        }
    }
    return true;
}

void HlbvhTest::SetUp()
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

TEST_F(HlbvhTest, BuildTest)
{
    MeshData mesh_data("../../resources/sponza.obj");
    ASSERT_TRUE(!(mesh_data.indices.size() % 3));
    ASSERT_TRUE(!(mesh_data.positions.size() % 3));

    auto    device = device_.get();
    VkQueue queue  = nullptr;
    vkGetDeviceQueue(device_.get(), queue_family_index_, 0, &queue);

    auto          helper = std::make_shared<GpuHelper>(device, phdevice_, queue, queue_family_index_);
    ShaderManager shader_mngr(device);
    BuildHlBvh    mesh_builder(helper, shader_mngr);
    auto vertex_buffer = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, mesh_data.positions);
    auto index_buffer  = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, mesh_data.indices);

    uint32_t triangle_count = uint32_t(mesh_data.indices.size() / 3);
    uint32_t vertex_count   = uint32_t(mesh_data.positions.size() / 3);

    auto scratch_size   = mesh_builder.GetScratchDataSize(triangle_count);
    auto scratch_buffer = helper->CreateScratchDeviceBuffer(scratch_size);
    auto result_size    = mesh_builder.GetResultDataSize(triangle_count);
    auto result_buffer =
        helper->CreateBuffer(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eDeviceLocal,
                             result_size);
    auto result_staging_buffer = helper->CreateStagingBuffer(result_size);

    helper->WithPrimaryCommandBuffer([&](vk::CommandBuffer build_cmd_buffer) {
        mesh_builder(build_cmd_buffer,
                     vertex_buffer.buffer,
                     0u,
                     3 * sizeof(float),
                     vertex_count,
                     index_buffer.buffer,
                     0u,
                     triangle_count,
                     scratch_buffer.buffer,
                     0u,
                     result_buffer.buffer,
                     0u);
        build_cmd_buffer.copyBuffer(
            result_buffer.buffer, result_staging_buffer.buffer, vk::BufferCopy(0, 0, result_size));
    });

    {
        bvh_utils::VkBvhNode* mapped_ptr = (bvh_utils::VkBvhNode*)result_staging_buffer.Map();
        ASSERT_TRUE(CheckConstistency(mapped_ptr));
        result_staging_buffer.Unmap();
    }

    index_buffer.Destroy();
    vertex_buffer.Destroy();
    scratch_buffer.Destroy();
    result_buffer.Destroy();
    result_staging_buffer.Destroy();
}

TEST_F(HlbvhTest, RestructureTest)
{
    MeshData mesh_data("../../resources/sponza.obj");
    ASSERT_TRUE(!(mesh_data.indices.size() % 3));
    ASSERT_TRUE(!(mesh_data.positions.size() % 3));

    auto    device = device_.get();
    VkQueue queue  = nullptr;
    vkGetDeviceQueue(device_.get(), queue_family_index_, 0, &queue);

    auto             helper = std::make_shared<GpuHelper>(device, phdevice_, queue, queue_family_index_);
    ShaderManager    shader_mngr(device);
    BuildHlBvh       mesh_builder(helper, shader_mngr);
    RestructureHlBvh optimizer(helper, shader_mngr);
    auto vertex_buffer = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, mesh_data.positions);
    auto index_buffer  = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, mesh_data.indices);

    uint32_t triangle_count = uint32_t(mesh_data.indices.size() / 3);
    uint32_t vertex_count   = uint32_t(mesh_data.positions.size() / 3);

    auto scratch_size =
        std::max(mesh_builder.GetScratchDataSize(triangle_count), optimizer.GetScratchDataSize(triangle_count));
    auto scratch_buffer = helper->CreateScratchDeviceBuffer(scratch_size);
    auto result_size    = mesh_builder.GetResultDataSize(triangle_count);
    auto result_buffer =
        helper->CreateBuffer(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eDeviceLocal,
                             result_size);
    auto result_staging_buffer = helper->CreateStagingBuffer(result_size);
    auto result_staging_opt_buffer = helper->CreateStagingBuffer(result_size);

    helper->WithPrimaryCommandBuffer([&](vk::CommandBuffer build_cmd_buffer) {
        mesh_builder(build_cmd_buffer,
                     vertex_buffer.buffer,
                     0u,
                     3 * sizeof(float),
                     vertex_count,
                     index_buffer.buffer,
                     0u,
                     triangle_count,
                     scratch_buffer.buffer,
                     0u,
                     result_buffer.buffer,
                     0u);
        build_cmd_buffer.copyBuffer(
            result_buffer.buffer, result_staging_buffer.buffer, vk::BufferCopy(0, 0, result_size));
        optimizer(build_cmd_buffer, triangle_count, scratch_buffer.buffer, 0u, result_buffer.buffer, 0u);
        build_cmd_buffer.copyBuffer(
            result_buffer.buffer, result_staging_opt_buffer.buffer, vk::BufferCopy(0, 0, result_size));
    });

    {
        bvh_utils::VkBvhNode* mapped_ptr = (bvh_utils::VkBvhNode*)result_staging_buffer.Map();
        ASSERT_TRUE(CheckConstistency(mapped_ptr));
        float sah = CalculateSAH(mapped_ptr);
        result_staging_buffer.Unmap();
        bvh_utils::VkBvhNode* mapped_opt_ptr = (bvh_utils::VkBvhNode*)result_staging_opt_buffer.Map();
        ASSERT_TRUE(CheckConstistency(mapped_opt_ptr));
        float sah_opt = CalculateSAH(mapped_opt_ptr);
        result_staging_opt_buffer.Unmap();
        ASSERT_TRUE(sah_opt <= sah);
        std::cout << "Before optimization SAH: " << sah << std::endl;
        std::cout << "After optimization SAH: " << sah_opt << std::endl;
    }

    index_buffer.Destroy();
    vertex_buffer.Destroy();
    scratch_buffer.Destroy();
    result_buffer.Destroy();
    result_staging_buffer.Destroy();
    result_staging_opt_buffer.Destroy();
}

TEST_F(HlbvhTest, UpdateTest)
{
    MeshData mesh_data("../../resources/sponza.obj");
    ASSERT_TRUE(!(mesh_data.indices.size() % 3));
    ASSERT_TRUE(!(mesh_data.positions.size() % 3));

    auto    device = device_.get();
    VkQueue queue  = nullptr;
    vkGetDeviceQueue(device_.get(), queue_family_index_, 0, &queue);

    auto          helper = std::make_shared<GpuHelper>(device, phdevice_, queue, queue_family_index_);
    ShaderManager shader_mngr(device);
    BuildHlBvh    mesh_builder(helper, shader_mngr);
    UpdateHlBvh   updater(helper, shader_mngr);
    auto vertex_buffer = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, mesh_data.positions);
    for (size_t i = 0; i < mesh_data.positions.size(); i += 3)
    {
        mesh_data.positions[i + 1] += 10.0f;
    }
    auto vertex_upd_buffer = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, mesh_data.positions);
    auto index_buffer      = helper->StageToDeviceBuffer(vk::BufferUsageFlagBits::eStorageBuffer, mesh_data.indices);

    uint32_t triangle_count = uint32_t(mesh_data.indices.size() / 3);
    uint32_t vertex_count   = uint32_t(mesh_data.positions.size() / 3);

    auto scratch_size = mesh_builder.GetScratchDataSize(triangle_count);
    auto scratch_buffer = helper->CreateScratchDeviceBuffer(scratch_size);
    auto result_size    = mesh_builder.GetResultDataSize(triangle_count);
    auto result_buffer =
        helper->CreateBuffer(vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc,
                             vk::MemoryPropertyFlagBits::eDeviceLocal,
                             result_size);
    auto result_staging_buffer = helper->CreateStagingBuffer(result_size);

    helper->WithPrimaryCommandBuffer([&](vk::CommandBuffer build_cmd_buffer) {
        mesh_builder(build_cmd_buffer,
                     vertex_buffer.buffer,
                     0u,
                     3 * sizeof(float),
                     vertex_count,
                     index_buffer.buffer,
                     0u,
                     triangle_count,
                     scratch_buffer.buffer,
                     0u,
                     result_buffer.buffer,
                     0u);
        updater(build_cmd_buffer,
                vertex_upd_buffer.buffer,
                0u,
                3 * sizeof(float),
                vertex_count,
                index_buffer.buffer,
                0u,
                triangle_count,
                result_buffer.buffer,
                0u);
        build_cmd_buffer.copyBuffer(
            result_buffer.buffer, result_staging_buffer.buffer, vk::BufferCopy(0, 0, result_size));
    });

    {
        bvh_utils::VkBvhNode* mapped_ptr = (bvh_utils::VkBvhNode*)result_staging_buffer.Map();
        ASSERT_TRUE(CheckConstistency(mapped_ptr));
        result_staging_buffer.Unmap();
    }

    index_buffer.Destroy();
    vertex_buffer.Destroy();
    vertex_upd_buffer.Destroy();
    scratch_buffer.Destroy();
    result_buffer.Destroy();
    result_staging_buffer.Destroy();
}
