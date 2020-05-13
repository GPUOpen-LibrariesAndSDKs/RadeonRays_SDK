#pragma once

#include <chrono>
#include <iostream>
#include <stack>

#include "mesh_data.h"
#include "radeonrays_vlk.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace std::chrono;
template <typename T>
using VkScopedObject = std::shared_ptr<std::remove_pointer_t<T>>;

#define CHECK_RR_CALL(x) do {if((x) != RR_SUCCESS) { throw std::runtime_error("Incorrect radeonrays call");}} while(false)

using namespace std::chrono;

namespace
{
	static auto constexpr VK_VENDOR_ID_AMD = 0x1002;
	static auto constexpr VK_VENDOR_ID_NVIDIA = 0x10de;
	static auto constexpr VK_VENDOR_ID_INTEL = 0x8086;
}

class Application
{
public:
	Application()
	{
		VkApplicationInfo app_info;
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pNext = NULL;
		app_info.pApplicationName = "RadeonRays Test";
		app_info.applicationVersion = 1;
		app_info.pEngineName = "RadeonRays";
		app_info.engineVersion = 1;
		app_info.apiVersion = VK_API_VERSION_1_2;

		const std::vector<const char*> layers = { "VK_LAYER_KHRONOS_validation" };
		const std::vector<const char*> instance_extensions = { VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME };

		VkInstanceCreateInfo instance_info;
		instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instance_info.pNext = NULL;
		instance_info.flags = 0;
		instance_info.pApplicationInfo = &app_info;
		instance_info.enabledExtensionCount = (uint32_t)instance_extensions.size();
		instance_info.ppEnabledExtensionNames = instance_extensions.data();

#ifdef _DEBUG
		instance_info.enabledLayerCount = 1u;
		instance_info.ppEnabledLayerNames = layers.data();
#else
		instance_info.enabledLayerCount = 0u;
		instance_info.ppEnabledLayerNames = nullptr;
#endif

		VkInstance instance = nullptr;
		VkResult   res = vkCreateInstance(&instance_info, nullptr, &instance);

		if (res == VK_ERROR_INCOMPATIBLE_DRIVER)
		{
			throw std::runtime_error("Cannot find a compatible Vulkan ICD\n");
		}
		else if (res)
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
		queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_create_info.pNext = nullptr;
		queue_create_info.flags = 0;
		queue_create_info.queueCount = 1u;
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
				found = true;
				break;
			}
		}

		if (!found)
		{
			throw std::runtime_error("No compute/transfer queues found\n");
		}

		phdevice_ = gpus[0];
		queue_family_index_ = queue_create_info.queueFamilyIndex;

		std::vector<const char*> extensions;

		if (physical_device_properties.vendorID == VK_VENDOR_ID_NVIDIA)
		{
			extensions.push_back(VK_NV_SHADER_SUBGROUP_PARTITIONED_EXTENSION_NAME);
		}
		else if (physical_device_properties.vendorID == VK_VENDOR_ID_AMD)
		{
			extensions.push_back(VK_AMD_SHADER_BALLOT_EXTENSION_NAME);
		}
		extensions.push_back(VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME);
		extensions.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
		extensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);

		VkDeviceCreateInfo device_create_info;
		device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		device_create_info.flags = 0;
		device_create_info.queueCreateInfoCount = 1u;
		device_create_info.pQueueCreateInfos = &queue_create_info;
#ifdef _DEBUG
		device_create_info.enabledLayerCount = 1u;
		device_create_info.ppEnabledLayerNames = layers.data();
#else
		device_create_info.enabledLayerCount = 0u;
		device_create_info.ppEnabledLayerNames = nullptr;
#endif

		device_create_info.enabledExtensionCount = (std::uint32_t)extensions.size();
		device_create_info.ppEnabledExtensionNames = extensions.data();
		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures(phdevice_, &features);

		VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures{};
		physicalDeviceDescriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
		physicalDeviceDescriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
		physicalDeviceDescriptorIndexingFeatures.runtimeDescriptorArray = VK_TRUE;
		physicalDeviceDescriptorIndexingFeatures.descriptorBindingVariableDescriptorCount = VK_TRUE;
		VkPhysicalDeviceFeatures2 physicalDeviceFeatures2{};

		physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDeviceFeatures2.features = features;
		physicalDeviceFeatures2.pNext = &physicalDeviceDescriptorIndexingFeatures;
		device_create_info.pEnabledFeatures = nullptr;
		device_create_info.pNext = &physicalDeviceFeatures2;

		VkDevice device = nullptr;
		res = vkCreateDevice(phdevice_, &device_create_info, nullptr, &device);

		if (res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create Vulkan device\n");
		}

		device_ = VkScopedObject<VkDevice>(device, [](VkDevice device) { vkDestroyDevice(device, nullptr); });

		VkCommandPoolCreateInfo command_pool_info;
		command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		command_pool_info.pNext = nullptr;
		command_pool_info.queueFamilyIndex = queue_create_info.queueFamilyIndex;
		command_pool_info.flags = 0;

		VkCommandPool command_pool = nullptr;
		res = vkCreateCommandPool(device_.get(), &command_pool_info, nullptr, &command_pool);

		if (res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create command pool\n");
		}

		command_pool_ = VkScopedObject<VkCommandPool>(command_pool, [device = device_.get()](VkCommandPool command_pool) {
			vkDestroyCommandPool(device, command_pool, nullptr);
		});
		VkQueryPoolCreateInfo query_pool_info;
		query_pool_info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		query_pool_info.pNext = nullptr;
		query_pool_info.queryType = VK_QUERY_TYPE_TIMESTAMP;
		query_pool_info.queryCount = 2;
		query_pool_info.pipelineStatistics = 0;
		query_pool_info.flags = 0;

		VkQueryPool query_pool = nullptr;

		res = vkCreateQueryPool(device, &query_pool_info, nullptr, &query_pool);

		if (res != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create query pool\n");
		}

		query_pool_ = VkScopedObject<VkQueryPool>(
			query_pool, [device = device_.get()](VkQueryPool pool) { vkDestroyQueryPool(device, pool, nullptr); });

	}
	void Run();
private:
	/// Allocate a region of memory of a given type.
	VkScopedObject<VkDeviceMemory> AllocateDeviceMemory(std::uint32_t memory_type_index, std::size_t size) const;

	/// Create buffer (w/o any allocations).
	VkScopedObject<VkBuffer> CreateBuffer(VkBufferUsageFlags usage, std::size_t size) const;

	/// Find memory index by its properties.
	uint32_t FindDeviceMemoryIndex(VkMemoryPropertyFlags flags) const;

	/// Upload the specified range of memory.
	template <typename TYPE>
	void UploadMemory(std::vector<TYPE> const& source, VkBuffer const& destination) const;

	// Vulkan data.
	VkScopedObject<VkInstance>    instance_;
	VkScopedObject<VkDevice>      device_;
	VkScopedObject<VkCommandPool> command_pool_;
	VkScopedObject<VkQueryPool>   query_pool_;
	VkPhysicalDevice              phdevice_;
	uint32_t                      queue_family_index_;
	float                         timestamp_period_;
	std::string device_name;
};

void Application::Run()
{
	RRContext context = nullptr;
	VkQueue   queue = nullptr;
	vkGetDeviceQueue(device_.get(), queue_family_index_, 0, &queue);
	CHECK_RR_CALL(rrCreateContextVk(RR_API_VERSION, device_.get(), phdevice_, queue, queue_family_index_, &context));
	auto local_memory_index = FindDeviceMemoryIndex(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	MeshData mesh_data("scene.obj");

	/// memory management to pass buffers to builder
	// create buffers
	auto vertex_buffer = CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		mesh_data.positions.size() * sizeof(float));
	auto index_buffer = CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		mesh_data.indices.size() * sizeof(uint32_t));
	// gather memory requirements
	VkMemoryRequirements vertex_buffer_mem_reqs;
	VkMemoryRequirements index_buffer_mem_reqs;
	vkGetBufferMemoryRequirements(device_.get(), vertex_buffer.get(), &vertex_buffer_mem_reqs);
	vkGetBufferMemoryRequirements(device_.get(), index_buffer.get(), &index_buffer_mem_reqs);
	// allocate local memory
	auto vertex_buffer_memory = AllocateDeviceMemory(local_memory_index, vertex_buffer_mem_reqs.size);
	auto index_buffer_memory = AllocateDeviceMemory(local_memory_index, index_buffer_mem_reqs.size);
	// bind it
	vkBindBufferMemory(device_.get(), vertex_buffer.get(), vertex_buffer_memory.get(), 0u);
	vkBindBufferMemory(device_.get(), index_buffer.get(), index_buffer_memory.get(), 0u);
	// upload to previously allocated buffers
	UploadMemory(mesh_data.positions, vertex_buffer.get());
	UploadMemory(mesh_data.indices, index_buffer.get());

	// get radeonrays ptrs to triangle description
	RRDevicePtr vertex_ptr = nullptr;
	RRDevicePtr index_ptr = nullptr;
	rrGetDevicePtrFromVkBuffer(context, vertex_buffer.get(), 0, &vertex_ptr);
	rrGetDevicePtrFromVkBuffer(context, index_buffer.get(), 0, &index_ptr);

	auto triangle_count = (uint32_t)mesh_data.indices.size() / 3;

	RRGeometryBuildInput    geometry_build_input = {};
	RRTriangleMeshPrimitive mesh = {};
	geometry_build_input.triangle_mesh_primitives = &mesh;
	geometry_build_input.primitive_type = RR_PRIMITIVE_TYPE_TRIANGLE_MESH;
	geometry_build_input.triangle_mesh_primitives->vertices = vertex_ptr;
	geometry_build_input.triangle_mesh_primitives->vertex_count = uint32_t(mesh_data.positions.size() / 3);

	geometry_build_input.triangle_mesh_primitives->vertex_stride = 3 * sizeof(float);
	geometry_build_input.triangle_mesh_primitives->triangle_indices = index_ptr;
	geometry_build_input.triangle_mesh_primitives->triangle_count = triangle_count;
	geometry_build_input.triangle_mesh_primitives->index_type = RR_INDEX_TYPE_UINT32;
	geometry_build_input.primitive_count = 1u;

	std::cout << "Triangle count " << triangle_count << "\n";

	RRBuildOptions options;
	options.build_flags = 0u;

	RRMemoryRequirements geometry_reqs;
	CHECK_RR_CALL(rrGetGeometryBuildMemoryRequirements(context, &geometry_build_input, &options, &geometry_reqs));

	// allocate buffers for builder and resulting geometry
	auto scratch_memory = AllocateDeviceMemory(local_memory_index, geometry_reqs.temporary_build_buffer_size);
	auto local_memory = AllocateDeviceMemory(local_memory_index, geometry_reqs.result_buffer_size);
	auto scratch = CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, geometry_reqs.temporary_build_buffer_size);
	auto geometry = CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, geometry_reqs.result_buffer_size);
	vkBindBufferMemory(device_.get(), scratch.get(), scratch_memory.get(), 0u);
	vkBindBufferMemory(device_.get(), geometry.get(), local_memory.get(), 0u);

	RRDevicePtr geometry_ptr;
	CHECK_RR_CALL(rrGetDevicePtrFromVkBuffer(context, geometry.get(), 0, &geometry_ptr));

	RRDevicePtr scratch_ptr = nullptr;
	CHECK_RR_CALL(rrGetDevicePtrFromVkBuffer(context, scratch.get(), 0, &scratch_ptr));

	std::cout << "Scratch buffer size: " << (float)geometry_reqs.temporary_build_buffer_size / 1000000 << "Mb\n";
	std::cout << "Result buffer size: " << (float)geometry_reqs.result_buffer_size / 1000000 << "Mb\n";

	VkCommandBuffer cmd_buf = VK_NULL_HANDLE;

	VkCommandBufferAllocateInfo alloc_info;
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.commandPool = command_pool_.get();
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1u;

	auto status = vkAllocateCommandBuffers(device_.get(), &alloc_info, &cmd_buf);
	if (status != VK_SUCCESS)
	{
		throw std::runtime_error("Unable to allocate command buffer");
	}

	auto command_buffer = VkScopedObject<VkCommandBuffer>(cmd_buf, [&](VkCommandBuffer command_buffer)
		{
			vkFreeCommandBuffers(device_.get(), command_pool_.get(), 1, &command_buffer);
		});

	RRCommandStream command_stream = nullptr;
	CHECK_RR_CALL(rrGetCommandStreamFromVkCommandBuffer(context, command_buffer.get(), &command_stream));

	VkCommandBufferBeginInfo begin_info;
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(cmd_buf, &begin_info);
	CHECK_RR_CALL(rrCmdBuildGeometry(
		context, RR_BUILD_OPERATION_BUILD, &geometry_build_input, &options, scratch_ptr, geometry_ptr, command_stream));
	vkCmdResetQueryPool(cmd_buf, query_pool_.get(), 0, 2);
	vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_.get(), 0);
	for (int i = 0; i < 10; i++)
	{
		CHECK_RR_CALL(rrCmdBuildGeometry(context,
			RR_BUILD_OPERATION_BUILD,
			&geometry_build_input,
			&options,
			scratch_ptr,
			geometry_ptr,
			command_stream));
	}

	vkCmdWriteTimestamp(cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_.get(), 1);

	RREvent wait_event = nullptr;
	CHECK_RR_CALL(rrSumbitCommandStream(context, command_stream, nullptr, &wait_event));
	CHECK_RR_CALL(rrWaitEvent(context, wait_event));
	CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
	CHECK_RR_CALL(rrReleaseExternalCommandStream(context, command_stream));

	std::uint64_t timebuild_mesh[2];
	vkGetQueryPoolResults(device_.get(),
		query_pool_.get(),
		0,
		2,
		2 * sizeof(std::uint64_t),
		timebuild_mesh,
		sizeof(std::uint64_t),
		VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
	float build_mesh_time = (timebuild_mesh[1] - timebuild_mesh[0]) * timestamp_period_ / 1e6f;
	std::cout << "Build time: " << build_mesh_time / 10 << "ms\n";
	std::cout << "Build throughput: " << triangle_count / (build_mesh_time * 1e-3f) * 1e-6f * 10 << "MPrims/s\n";

	// built-in intersection

	constexpr uint32_t kResolution = 2048;
	using Ray = RRRay;
	using Hit = RRHit;
	std::vector<Ray>   rays(kResolution * kResolution);
	std::vector<Hit>   hits(kResolution * kResolution);

	for (int x = 0; x < kResolution; ++x)
	{
		for (int y = 0; y < kResolution; ++y)
		{
			auto i = kResolution * y + x;

			rays[i].origin[0] = 0.f;
			rays[i].origin[1] = 15.f;
			rays[i].origin[2] = 0.f;

			rays[i].direction[0] = -1.f;
			rays[i].direction[1] = -1.f + (2.f / kResolution) * y;
			rays[i].direction[2] = -1.f + (2.f / kResolution) * x;

			rays[i].min_t = 0.001f;
			rays[i].max_t = 100000.f;
		}
	}

	auto rays_buffer =
		CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, rays.size() * sizeof(Ray));
	auto hits_buffer =
		CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT, hits.size() * sizeof(Hit));

	// gather memory requirements
	VkMemoryRequirements rays_buffer_mem_reqs;
	VkMemoryRequirements hits_buffer_mem_reqs;
	vkGetBufferMemoryRequirements(device_.get(), rays_buffer.get(), &rays_buffer_mem_reqs);
	vkGetBufferMemoryRequirements(device_.get(), hits_buffer.get(), &hits_buffer_mem_reqs);
	// allocate local memory
	auto rays_buffer_memory = AllocateDeviceMemory(local_memory_index, rays_buffer_mem_reqs.size);
	auto hits_buffer_memory = AllocateDeviceMemory(local_memory_index, hits_buffer_mem_reqs.size);
	// bind it
	vkBindBufferMemory(device_.get(), rays_buffer.get(), rays_buffer_memory.get(), 0u);
	vkBindBufferMemory(device_.get(), hits_buffer.get(), hits_buffer_memory.get(), 0u);
	// upload to previously allocated buffers
	UploadMemory(rays, rays_buffer.get());

	RRDevicePtr rays_ptr;
	CHECK_RR_CALL(rrGetDevicePtrFromVkBuffer(context, rays_buffer.get(), 0, &rays_ptr));

	RRDevicePtr hits_ptr = nullptr;
	CHECK_RR_CALL(rrGetDevicePtrFromVkBuffer(context, hits_buffer.get(), 0, &hits_ptr));

	VkCommandBuffer trace_cmd_buf = VK_NULL_HANDLE;

	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.commandPool = command_pool_.get();
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1u;

	status = vkAllocateCommandBuffers(device_.get(), &alloc_info, &trace_cmd_buf);
	if (status != VK_SUCCESS)
	{
		throw std::runtime_error("Unable to allocate command buffer");
	}
	auto trace_command_buffer = VkScopedObject<VkCommandBuffer>(trace_cmd_buf, [&](VkCommandBuffer command_buffer)
		{
			vkFreeCommandBuffers(device_.get(), command_pool_.get(), 1, &command_buffer);
		});

	RRCommandStream trace_command_stream = nullptr;
	CHECK_RR_CALL(rrGetCommandStreamFromVkCommandBuffer(context, trace_command_buffer.get(), &trace_command_stream));

	// get scratch trace buffer parameters
	size_t scratch_size;
	rrGetTraceMemoryRequirements(context, kResolution * kResolution, &scratch_size);
	auto                 scratch_buffer = CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, scratch_size);
	VkMemoryRequirements scratch_buffer_mem_reqs;
	vkGetBufferMemoryRequirements(device_.get(), scratch_buffer.get(), &scratch_buffer_mem_reqs);
	auto scratch_buffer_memory = AllocateDeviceMemory(local_memory_index, scratch_buffer_mem_reqs.size);
	vkBindBufferMemory(device_.get(), scratch_buffer.get(), scratch_buffer_memory.get(), 0u);
	RRDevicePtr scratch_trace_ptr = nullptr;
	CHECK_RR_CALL(rrGetDevicePtrFromVkBuffer(context, scratch_buffer.get(), 0, &scratch_trace_ptr));

	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(trace_cmd_buf, &begin_info);
	vkCmdResetQueryPool(trace_cmd_buf, query_pool_.get(), 0, 2);

	CHECK_RR_CALL(rrCmdIntersect(context,
		geometry_ptr,
		RR_INTERSECT_QUERY_CLOSEST,
		rays_ptr,
		kResolution * kResolution,
		nullptr,
		RR_INTERSECT_QUERY_OUTPUT_FULL_HIT,
		hits_ptr,
		scratch_trace_ptr,
		trace_command_stream));
	vkCmdWriteTimestamp(trace_cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_.get(), 0);
	for (int i = 0; i < 10; i++)
	{
		CHECK_RR_CALL(rrCmdIntersect(context,
			geometry_ptr,
			RR_INTERSECT_QUERY_CLOSEST,
			rays_ptr,
			kResolution * kResolution,
			nullptr,
			RR_INTERSECT_QUERY_OUTPUT_FULL_HIT,
			hits_ptr,
			scratch_trace_ptr,
			trace_command_stream));
	}
	vkCmdWriteTimestamp(trace_cmd_buf, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, query_pool_.get(), 1);
	CHECK_RR_CALL(rrSumbitCommandStream(context, trace_command_stream, nullptr, &wait_event));
	CHECK_RR_CALL(rrWaitEvent(context, wait_event));

	CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
	CHECK_RR_CALL(rrReleaseExternalCommandStream(context, trace_command_stream));

	// Get results
	std::uint64_t time[2];
	vkGetQueryPoolResults(device_.get(),
		query_pool_.get(),
		0,
		2,
		2 * sizeof(std::uint64_t),
		time,
		sizeof(std::uint64_t),
		VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);

	float delta_time = (time[1] - time[0]) * timestamp_period_ / 1e6f;

	std::cout << "Trace time: " << delta_time << "ms\n";
	std::cout << "Throughput: " << (kResolution * kResolution) / (delta_time * 1e-3) * 1e-6 * 10 << " MRays/s\n";

	auto staging_memory_index =
		FindDeviceMemoryIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	auto staging_hits_memory = AllocateDeviceMemory(staging_memory_index, hits_buffer_mem_reqs.size);
	auto staging_hits_buffer = CreateBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, hits_buffer_mem_reqs.size);
	vkBindBufferMemory(device_.get(), staging_hits_buffer.get(), staging_hits_memory.get(), 0);

	VkCommandBuffer             copy_command_buffer = nullptr;
	VkCommandBufferAllocateInfo command_buffer_info;
	command_buffer_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	command_buffer_info.pNext = nullptr;
	command_buffer_info.commandPool = command_pool_.get();
	command_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	command_buffer_info.commandBufferCount = 1u;

	// Allocate copy command buffer.
	status = vkAllocateCommandBuffers(device_.get(), &command_buffer_info, &copy_command_buffer);
	if (status != VK_SUCCESS)
	{
		throw std::runtime_error("Unable to allocate command buffer");
	}

	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(copy_command_buffer, &begin_info);

	VkBufferCopy copy_region;
	copy_region.srcOffset = 0u;
	copy_region.dstOffset = 0u;
	copy_region.size = hits_buffer_mem_reqs.size;

	// Cmd copy command.
	vkCmdCopyBuffer(copy_command_buffer, hits_buffer.get(), staging_hits_buffer.get(), 1u, &copy_region);
	vkEndCommandBuffer(copy_command_buffer);

	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.commandBufferCount = 1u;
	submit_info.pCommandBuffers = &copy_command_buffer;
	submit_info.pSignalSemaphores = nullptr;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.pWaitDstStageMask = nullptr;
	submit_info.signalSemaphoreCount = 0u;
	submit_info.waitSemaphoreCount = 0u;
	status = vkQueueSubmit(queue, 1u, &submit_info, nullptr);
	if (status != VK_SUCCESS)
	{
		throw std::runtime_error("Unable to submit queue");
	}

	status = vkQueueWaitIdle(queue);
	if (status != VK_SUCCESS)
	{
		throw std::runtime_error("Unable to wait queue");
	}

	// Map staging ray buffer.
	Hit* mapped_ptr = nullptr;
	status = vkMapMemory(device_.get(), staging_hits_memory.get(), 0, VK_WHOLE_SIZE, 0, (void**)&mapped_ptr);
	if (status != VK_SUCCESS)
	{
		throw std::runtime_error("Unable to map memory");
	}

	std::vector<uint32_t> data(kResolution * kResolution);

	for (int y = 0; y < kResolution; ++y)
	{
		for (int x = 0; x < kResolution; ++x)
		{
			int wi = kResolution * (kResolution - 1 - y) + x;
			int i = kResolution * y + x;

			if (mapped_ptr[i].inst_id != ~0u)
			{
				data[wi] = 0xff000000 | (uint32_t(mapped_ptr[i].uv[0] * 255) << 8) |
					(uint32_t(mapped_ptr[i].uv[1] * 255) << 16);
			}
			else
			{
				data[wi] = 0xff101010;
			}
		}
	}

	stbi_write_jpg("vk_geom.jpg", kResolution, kResolution, 4, data.data(), 120);

	CHECK_RR_CALL(rrDestroyContext(context));
}


inline VkScopedObject<VkDeviceMemory> Application::AllocateDeviceMemory(std::uint32_t memory_type_index,
	std::size_t   size) const
{
	VkMemoryAllocateInfo info;
	info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	info.pNext = nullptr;
	info.memoryTypeIndex = memory_type_index;
	info.allocationSize = size;

	VkDeviceMemory memory = nullptr;
	auto           res = vkAllocateMemory(device_.get(), &info, nullptr, &memory);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Cannot allocate device memory");
	}

	return VkScopedObject<VkDeviceMemory>(
		memory, [device = device_.get()](VkDeviceMemory memory) { vkFreeMemory(device, memory, nullptr); });
}

inline std::uint32_t Application::FindDeviceMemoryIndex(VkMemoryPropertyFlags flags) const
{
	VkPhysicalDeviceMemoryProperties mem_props;
	vkGetPhysicalDeviceMemoryProperties(phdevice_, &mem_props);

	for (auto i = 0u; i < mem_props.memoryTypeCount; i++)
	{
		auto& memory_type = mem_props.memoryTypes[i];
		if ((memory_type.propertyFlags & flags) == flags)
		{
			if (memory_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			{
				assert(memory_type.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			}
			return i;
		}
	}

	throw std::runtime_error("Cannot find specified memory type");
	return 0xffffffffu;
}

inline VkScopedObject<VkBuffer> Application::CreateBuffer(VkBufferUsageFlags usage, std::size_t size) const
{
	VkBufferCreateInfo buffer_info;
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.pNext = nullptr;
	buffer_info.queueFamilyIndexCount = 0u;
	buffer_info.pQueueFamilyIndices = nullptr;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	buffer_info.usage = usage;
	buffer_info.flags = 0;
	buffer_info.size = size;

	VkBuffer buffer = nullptr;
	auto     res = vkCreateBuffer(device_.get(), &buffer_info, nullptr, &buffer);

	if (res != VK_SUCCESS)
	{
		throw std::runtime_error("Cannot create buffer");
	}

	return VkScopedObject<VkBuffer>(
		buffer, [device = device_.get()](VkBuffer buffer) { vkDestroyBuffer(device, buffer, nullptr); });
}

template <typename TYPE>
inline void Application::UploadMemory(std::vector<TYPE> const& source, VkBuffer const& destination) const
{
	auto const buffer_size = source.size() * sizeof(TYPE);

	auto staging_buffer = CreateBuffer(VK_BUFFER_USAGE_TRANSFER_SRC_BIT, buffer_size);

	VkMemoryRequirements staging_mem_reqs;

	vkGetBufferMemoryRequirements(device_.get(), staging_buffer.get(), &staging_mem_reqs);

	auto staging_memory_index =
		FindDeviceMemoryIndex(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	auto staging_memory = AllocateDeviceMemory(staging_memory_index, staging_mem_reqs.size);

	vkBindBufferMemory(device_.get(), staging_buffer.get(), staging_memory.get(), 0u);

	void* memory = nullptr;

	vkMapMemory(device_.get(), staging_memory.get(), 0u, VK_WHOLE_SIZE, 0, &memory);

	memcpy(memory, source.data(), buffer_size);

	vkUnmapMemory(device_.get(), staging_memory.get());

	VkCommandBuffer cmd_buf = VK_NULL_HANDLE;

	VkCommandBufferAllocateInfo alloc_info;
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.pNext = nullptr;
	alloc_info.commandPool = command_pool_.get();
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	alloc_info.commandBufferCount = 1u;

	auto status = vkAllocateCommandBuffers(device_.get(), &alloc_info, &cmd_buf);
	if (status != VK_SUCCESS)
	{
		throw std::runtime_error("Unable to allocate command buffer");
	}

	auto command_buffer = VkScopedObject<VkCommandBuffer>(cmd_buf, [&](VkCommandBuffer command_buffer)
		{
			vkFreeCommandBuffers(device_.get(), command_pool_.get(), 1, &command_buffer);
		});

	VkCommandBufferBeginInfo begin_info;
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
	begin_info.flags = 0;
	begin_info.pInheritanceInfo = nullptr;

	vkBeginCommandBuffer(cmd_buf, &begin_info);

	VkBufferCopy buffer_copy;

	buffer_copy.srcOffset = 0u;
	buffer_copy.dstOffset = 0u;
	buffer_copy.size = buffer_size;

	vkCmdCopyBuffer(cmd_buf, staging_buffer.get(), destination, 1, &buffer_copy);

	vkEndCommandBuffer(cmd_buf);

	VkQueue queue = nullptr;
	vkGetDeviceQueue(device_.get(), queue_family_index_, 0u, &queue);
	if (queue == nullptr)
	{
		throw std::runtime_error("Unable to get queue");
	}

	VkSubmitInfo submit_info;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext = nullptr;
	submit_info.waitSemaphoreCount = 0;
	submit_info.pWaitSemaphores = nullptr;
	submit_info.pWaitDstStageMask = nullptr;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &cmd_buf;
	submit_info.signalSemaphoreCount = 0;
	submit_info.pSignalSemaphores = nullptr;

	vkQueueSubmit(queue, 1, &submit_info, VK_NULL_HANDLE);
	vkQueueWaitIdle(queue);
}

