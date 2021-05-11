/**********************************************************************
Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
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
// clang-format off
#include "utils/warning_push.h"
#include "utils/warning_ignore_general.h"
#include "radeonrays.h"
#include "utils/warning_pop.h"
// clang-format on

#include <functional>
#include <mutex>
#include <vector>

#include "base/command_stream_base.h"
#include "base/device_base.h"
#include "base/device_ptr_base.h"
#include "base/event_base.h"
#include "base/intersector_base.h"
#include "utils/logger.h"
#include "context.h"

#ifdef RR_ENABLE_DX12
#include "dx/command_stream.h"
#include "dx/device.h"
#include "dx/intersector_dispatch.h"
#include "dx/shader_compiler.h"
#include "radeonrays_dx.h"
#endif

#ifdef RR_ENABLE_VK
#include "radeonrays_vlk.h"
#include "vlk/command_stream.h"
#include "vlk/device.h"
#include "vlk/device_ptr.h"
#include "vlk/intersector_dispatch.h"
#endif

using namespace rt;

namespace
{
/// Get triangle mesh build info from a build input.
std::vector<TriangleMeshBuildInfo> GetTriangleMeshBuildInfo(const RRGeometryBuildInput& build_input)
{
    std::vector<TriangleMeshBuildInfo> build_info(build_input.primitive_count);
    for (uint32_t i = 0; i < build_input.primitive_count; ++i)
    {
        build_info[i].vertices = reinterpret_cast<DevicePtrBase*>(build_input.triangle_mesh_primitives[i].vertices);
        build_info[i].vertex_stride = build_input.triangle_mesh_primitives[i].vertex_stride;
        build_info[i].vertex_count  = build_input.triangle_mesh_primitives[i].vertex_count;

        build_info[i].triangle_indices =
            reinterpret_cast<DevicePtrBase*>(build_input.triangle_mesh_primitives[i].triangle_indices);
        build_info[i].triangle_count = build_input.triangle_mesh_primitives[i].triangle_count;
        build_info[i].index_type     = build_input.triangle_mesh_primitives[i].index_type;
    }
    return build_info;
}

}  // namespace

RRError rrCreateContext(uint32_t api_version, RRApi api, RRContext* context)
{
    Logger::Get().Info("rrCreateContext({})", api_version);

    if (!context)
    {
        Logger::Get().Error("Context is nullptr");
        return RR_ERROR_INVALID_PARAMETER;
    }

    try
    {
        Context* rtctx = new Context;

#if RR_ENABLE_DX12
        if (api == RR_API_DX)
        {
            Logger::Get().Info("Creating DX12 default context");
            rtctx->device      = dx::CreateDevice();
            rtctx->intersector = dx::CreateIntersector(*(rtctx->device), dx::IntersectorType::kCompute);
            rtctx->api         = RR_API_DX;
        }
#endif
#if RR_ENABLE_VK
        if (api == RR_API_VK)
        {
            Logger::Get().Info("Creating Vulkan context");
            rtctx->device      = vulkan::CreateDevice();
            rtctx->intersector = vulkan::CreateIntersector(*(rtctx->device), vulkan::IntersectorType::kCompute);
            rtctx->api         = RR_API_VK;
        }
#endif

        *context = reinterpret_cast<RRContext>(rtctx);
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Context successfully created");
    return RR_SUCCESS;
}

RRError rrSetLogLevel(RRLogLevel log_level)
{
    try
    {
        Logger::Get().SetLogLevel(log_level);
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INVALID_PARAMETER;
    }

    // logging it aftewards since off is possible
    Logger::Get().Info("rrSetLogLevel({})", log_level);
    return RR_SUCCESS;
}

RRError rrSetLogFile(char const* filename)
{
    if (!filename)
    {
        Logger::Get().Error("Invalid filename passed");
        return RR_ERROR_INVALID_PARAMETER;
    }
    try
    {
        Logger::Get().SetFileLogger(filename);
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    // logging it aftewards since we just changed logging to file
    Logger::Get().Info("rrSetLogFile({})", filename);
    return RR_SUCCESS;
}


#if RR_ENABLE_DX12
RRError rrCreateContextDX(uint32_t            api_version,
                          ID3D12Device*       d3d_device,
                          ID3D12CommandQueue* command_queue,
                          RRContext*          context)
{
    Logger::Get().Info("rrCreateContextDX({})", api_version);

    if (!context || !d3d_device || !command_queue)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    try
    {
        Context* ctx     = new Context;
        ctx->device      = dx::CreateDevice(d3d_device, command_queue);
        ctx->intersector = dx::CreateIntersector(*ctx->device, dx::IntersectorType::kCompute);
        ctx->api         = RR_API_DX;
        *context         = reinterpret_cast<RRContext>(ctx);
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Context successfully created");
    return RR_SUCCESS;
}
#endif

#if RR_ENABLE_VK
RRError rrCreateContextVk(uint32_t         api_version,
                          VkDevice         device,
                          VkPhysicalDevice physical_device,
                          VkQueue          command_queue,
                          uint32_t         queue_family_index,
                          RRContext*       context)
{
    Logger::Get().Info("rrCreateContextVk({})", api_version);

    if (!context || !device || !physical_device || !command_queue)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    try
    {
        Context* ctx     = new Context;
        ctx->device      = vulkan::CreateDevice(device, physical_device, command_queue, queue_family_index);
        ctx->intersector = vulkan::CreateIntersector(*ctx->device, vulkan::IntersectorType::kCompute);
        ctx->api         = RR_API_VK;
        *context         = reinterpret_cast<RRContext>(ctx);
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Context successfully created");
    return RR_SUCCESS;
}
#endif

RRError rrDestroyContext(RRContext context)
{
    Logger::Get().Info("rrDestroyContext");

    if (!context)
    {
        Logger::Get().Error("Context is nullptr");
        return RR_ERROR_INVALID_PARAMETER;
    }

    delete reinterpret_cast<Context*>(context);

    Logger::Get().Debug("Context successfully destroyed");
    return RR_SUCCESS;
}

RRError rrCmdBuildGeometry(RRContext                   context,
                           RRBuildOperation            build_operation,
                           const RRGeometryBuildInput* build_input,
                           const RRBuildOptions*       build_options,
                           RRDevicePtr                 temporary_buffer,
                           RRDevicePtr                 geometry_buffer,
                           RRCommandStream             command_stream)
{
    Logger::Get().Info("rrCmdBuildGeometry");

    if (!context || !command_stream || !build_input)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx               = reinterpret_cast<Context*>(context);
    auto rt_command_stream = reinterpret_cast<CommandStreamBase*>(command_stream);

    try
    {
        switch (build_input->primitive_type)
        {
        case RR_PRIMITIVE_TYPE_TRIANGLE_MESH: {
            std::vector<TriangleMeshBuildInfo> build_info = GetTriangleMeshBuildInfo(*build_input);

            if (build_operation == RR_BUILD_OPERATION_BUILD)
            {
                ctx->intersector->BuildTriangleMesh(rt_command_stream,
                                                    build_info,
                                                    build_options,
                                                    reinterpret_cast<DevicePtrBase*>(temporary_buffer),
                                                    reinterpret_cast<DevicePtrBase*>(geometry_buffer));
            }
            else
            {
                ctx->intersector->UpdateTriangleMesh(rt_command_stream,
                                                     build_info,
                                                     build_options,
                                                     reinterpret_cast<DevicePtrBase*>(temporary_buffer),
                                                     reinterpret_cast<DevicePtrBase*>(geometry_buffer));
            }
            break;
        }
        default: {
            Logger::Get().Error("Build input type not supported");
            return RR_ERROR_NOT_IMPLEMENTED;
        }
        }
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Geometry build command successfully recorded");
    return RR_SUCCESS;
}

RRError rrGetGeometryBuildMemoryRequirements(RRContext                   context,
                                             const RRGeometryBuildInput* build_input,
                                             const RRBuildOptions*       build_options,
                                             RRMemoryRequirements*       memory_requirements)
{
    Logger::Get().Info("rrGetGeometryBuildMemoryRequirements");

    if (!context || !build_input || !memory_requirements)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        switch (build_input->primitive_type)
        {
        case RR_PRIMITIVE_TYPE_TRIANGLE_MESH: {
            std::vector<TriangleMeshBuildInfo> build_info = GetTriangleMeshBuildInfo(*build_input);

            PreBuildInfo info = ctx->intersector->GetTriangleMeshPreBuildInfo(build_info, build_options);
            memory_requirements->result_buffer_size           = info.result_size;
            memory_requirements->temporary_build_buffer_size  = info.build_scratch_size;
            memory_requirements->temporary_update_buffer_size = info.update_scratch_size;
            break;
        }
        default: {
            Logger::Get().Error("Build input type not supported");
            return RR_ERROR_NOT_IMPLEMENTED;
        }
        }
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Successfully provided geometry memory requirements");
    return RR_SUCCESS;
}

RRError rrCmdBuildScene(RRContext                context,
                        const RRSceneBuildInput* build_input,
                        const RRBuildOptions*    build_options,
                        RRDevicePtr              temporary_buffer,
                        RRDevicePtr              scene_buffer,
                        RRCommandStream          command_stream)
{
    Logger::Get().Info("rrCmdBuildScene");

    if (!context || !command_stream || !build_input)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx               = reinterpret_cast<Context*>(context);
    auto rt_command_stream = reinterpret_cast<CommandStreamBase*>(command_stream);

    try
    {
        ctx->intersector->BuildScene(rt_command_stream,
                                     build_input->instances,
                                     build_input->instance_count,
                                     build_options,
                                     reinterpret_cast<DevicePtrBase*>(temporary_buffer),
                                     reinterpret_cast<DevicePtrBase*>(scene_buffer));
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Scene build command successfully recorded");
    return RR_SUCCESS;
}

RRError rrGetSceneBuildMemoryRequirements(RRContext                context,
                                          const RRSceneBuildInput* build_input,
                                          const RRBuildOptions*    build_options,
                                          RRMemoryRequirements*    memory_requirements)
{
    Logger::Get().Info("rrGetSceneBuildMemoryRequirements");

    if (!context || !build_input || !memory_requirements)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        PreBuildInfo info = ctx->intersector->GetScenePreBuildInfo(build_input->instance_count, build_options);
        memory_requirements->result_buffer_size           = info.result_size;
        memory_requirements->temporary_build_buffer_size  = info.build_scratch_size;
        memory_requirements->temporary_update_buffer_size = info.update_scratch_size;
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Successfully provided scene memory requirements");
    return RR_SUCCESS;
}

RRError rrCmdIntersect(RRContext              context,
                       RRDevicePtr            scene_buffer,
                       RRIntersectQuery       query,
                       RRDevicePtr            rays,
                       uint32_t               ray_count,
                       RRDevicePtr            indirect_ray_count,
                       RRIntersectQueryOutput query_output,
                       RRDevicePtr            hits,
                       RRDevicePtr            scratch,
                       RRCommandStream        command_stream)
{
    Logger::Get().Info("rrCmdIntersect");

    if (!context || !scene_buffer || !rays || !hits || !scratch || !command_stream)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        ctx->intersector->Intersect(reinterpret_cast<CommandStreamBase*>(command_stream),
                                    reinterpret_cast<DevicePtrBase*>(scene_buffer),
                                    query,
                                    reinterpret_cast<DevicePtrBase*>(rays),
                                    ray_count,
                                    reinterpret_cast<DevicePtrBase*>(indirect_ray_count),
                                    query_output,
                                    reinterpret_cast<DevicePtrBase*>(hits),
                                    reinterpret_cast<DevicePtrBase*>(scratch));
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Batch intersect command successfully recorded");
    return RR_SUCCESS;
}

RRError rrAllocateCommandStream(RRContext context, RRCommandStream* command_stream)
{
    Logger::Get().Info("rrAllocateCommandStream");

    if (!context || !command_stream)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    try
    {
        *command_stream = reinterpret_cast<RRCommandStream>(ctx->device->AllocateCommandStream());
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Command stream successfully allocated");
    return RR_SUCCESS;
}

RRError rrReleaseCommandStream(RRContext context, RRCommandStream command_stream)
{
    Logger::Get().Info("rrReleaseCommandStream");

    if (!context || !command_stream)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx        = reinterpret_cast<Context*>(context);
    auto cmd_stream = reinterpret_cast<CommandStreamBase*>(command_stream);

    try
    {
        ctx->device->ReleaseCommandStream(cmd_stream);
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Command stream successfully released");
    return RR_SUCCESS;
}

RRError rrSumbitCommandStream(RRContext context, RRCommandStream command_stream, RREvent wait_event, RREvent* event)
{
    Logger::Get().Info("rrSumbitCommandStream");

    if (!context || !command_stream || !event)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx           = reinterpret_cast<Context*>(context);
    auto cmd_stream    = reinterpret_cast<CommandStreamBase*>(command_stream);
    auto rt_wait_event = reinterpret_cast<EventBase*>(wait_event);

    try
    {
        *event = reinterpret_cast<RREvent>(ctx->device->SubmitCommandStream(cmd_stream, rt_wait_event));
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Command stream successfully submitted");
    return RR_SUCCESS;
}

RRError rrReleaseEvent(RRContext context, RREvent event)
{
    Logger::Get().Info("rrReleaseEvent");

    if (!context || !event)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);
    auto evt = reinterpret_cast<EventBase*>(event);

    try
    {
        ctx->device->ReleaseEvent(evt);
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Event successfully released");
    return RR_SUCCESS;
}

RRError rrWaitEvent(RRContext context, RREvent event)
{
    Logger::Get().Info("rrReleaseEvent");

    if (!context || !event)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);
    auto evt = reinterpret_cast<EventBase*>(event);

    try
    {
        ctx->device->WaitEvent(evt);
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Successful wait on event.");
    return RR_SUCCESS;
}

RRError rrReleaseDevicePtr(RRContext context, RRDevicePtr ptr)
{
    Logger::Get().Info("rrReleaseDevicePtr");

    if (!context || !ptr)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    delete reinterpret_cast<DevicePtrBase*>(ptr);

    Logger::Get().Debug("Device pointer successfully released");
    return RR_SUCCESS;
}

#if RR_ENABLE_DX12
RRError rrGetDevicePtrFromD3D12Resource(RRContext       context,
                                        ID3D12Resource* resource,
                                        size_t          offset,
                                        RRDevicePtr*    device_ptr)
{
    Logger::Get().Info("rrGetDevicePtrFromD3D12Resource");

    if (!context || !resource || !device_ptr)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    if (ctx->api != RR_API_DX)
    {
        Logger::Get().Error("Not supported for selected API");
        return RR_ERROR_UNSUPPORTED_INTEROP;
    }

    *device_ptr = reinterpret_cast<RRDevicePtr>(dx::CreateDevicePtr(resource, offset));

    Logger::Get().Debug("Device pointer obtained from D3D12Resource");
    return RR_SUCCESS;
}

RRError rrGetCommandStreamFromD3D12CommandList(RRContext                  context,
                                               ID3D12GraphicsCommandList* command_list,
                                               RRCommandStream*           command_stream)
{
    Logger::Get().Info("rrGetCommandStreamFromD3D12CommandList");

    if (!context || !command_list || !command_stream)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    if (ctx->api != RR_API_DX)
    {
        Logger::Get().Error("Not supported for selected API");
        return RR_ERROR_UNSUPPORTED_INTEROP;
    }

    // TODO: Pool this allocation.
    try
    {
        *command_stream = reinterpret_cast<RRCommandStream>(
            new dx::CommandStream(reinterpret_cast<dx::Device&>(*ctx->device), command_list));
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Command stream obtained from D3D12CommandList");
    return RR_SUCCESS;
}
#endif
#if RR_ENABLE_VK
RRError rrGetDevicePtrFromVkBuffer(RRContext context, VkBuffer buffer, size_t offset, RRDevicePtr* device_ptr)
{
    Logger::Get().Info("rrGetDevicePtrFromVkBuffer");

    if (!context || !buffer || !device_ptr)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    if (ctx->api != RR_API_VK)
    {
        Logger::Get().Error("Not supported for selected API");
        return RR_ERROR_UNSUPPORTED_INTEROP;
    }

    *device_ptr = reinterpret_cast<RRDevicePtr>(vulkan::CreateDevicePtr(buffer, offset));
    Logger::Get().Debug("Device pointer obtained from VkBuffer");
    return RR_SUCCESS;
}

RRError rrGetCommandStreamFromVkCommandBuffer(RRContext        context,
                                              VkCommandBuffer  command_list,
                                              RRCommandStream* command_stream)
{
    Logger::Get().Info("rrGetCommandStreamFromVkCommandBuffer");

    if (!context || !command_list || !command_stream)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    if (ctx->api != RR_API_VK)
    {
        Logger::Get().Error("Not supported for selected API");
        return RR_ERROR_UNSUPPORTED_INTEROP;
    }

    try
    {
        *command_stream = reinterpret_cast<RRCommandStream>(
            new vulkan::CommandStream(reinterpret_cast<vulkan::Device&>(*ctx->device), command_list));
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Command stream obtained from VkCommandBuffer");
    return RR_SUCCESS;
}
#endif

RRError rrReleaseExternalCommandStream(RRContext context, RRCommandStream command_stream)
{
    Logger::Get().Info("rrReleaseExternalCommandStream");

    if (!context || !command_stream)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    delete reinterpret_cast<CommandStreamBase*>(command_stream);

    Logger::Get().Debug("External command stream successfully released");
    return RR_SUCCESS;
}

RRError rrGetTraceMemoryRequirements(RRContext context, uint32_t ray_count, size_t* scratch_size)
{
    Logger::Get().Info("rrGetTraceMemoryRequirements");

    if (!context || !scratch_size || !ray_count)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }
    auto ctx      = reinterpret_cast<Context*>(context);
    *scratch_size = ctx->intersector->GetTraceMemoryRequirements(ray_count);

    Logger::Get().Debug("Successfully provided trace memory requirements");
    return RR_SUCCESS;
}

#ifdef RR_ENABLE_VK
RRError rrAllocateDeviceBuffer(RRContext context, size_t size, RRDevicePtr* device_ptr)
{
    Logger::Get().Info("rrAllocateDeviceBuffer");

    if (!context || !size || !device_ptr)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    if (ctx->api != RR_API_VK)
    {
        Logger::Get().Error("Not supported for selected API");
        return RR_ERROR_UNSUPPORTED_INTEROP;
    }

    auto& device = reinterpret_cast<DeviceBackend<BackendType::kVulkan>&>(*ctx->device);
    try
    {
        *device_ptr = reinterpret_cast<RRDevicePtr>(device.CreateAllocatedBuffer(size));
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Allocated vulkan buffer with size {}", size);
    return RR_SUCCESS;
}

RRError rrMapDevicePtr(RRContext context, RRDevicePtr device_ptr, void** mapping_ptr)
{
    Logger::Get().Info("rrMapDevicePtr");

    if (!context || !device_ptr)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    if (ctx->api != RR_API_VK)
    {
        Logger::Get().Error("Not supported for selected API");
        return RR_ERROR_UNSUPPORTED_INTEROP;
    }
    auto base             = reinterpret_cast<DevicePtrBase*>(device_ptr);
    auto allocated_buffer = dynamic_cast<DevicePtrBackend<BackendType::kVulkan>*>(base);
    if (!allocated_buffer)
    {
        Logger::Get().Error("Device pointer cannot be mapped");
        return RR_ERROR_INVALID_PARAMETER;
    }
    try
    {
        *mapping_ptr = allocated_buffer->Map();
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }
    if (!*mapping_ptr)
    {
        Logger::Get().Error("Device pointer cannot be mapped");
        return RR_ERROR_INVALID_PARAMETER;
    }

    Logger::Get().Debug("Map vulkan buffer");
    return RR_SUCCESS;
}

RRError rrUnmapDevicePtr(RRContext context, RRDevicePtr device_ptr, void** mapping_ptr)
{
    Logger::Get().Info("rrUnmapDevicePtr");

    if (!context || !device_ptr)
    {
        Logger::Get().Error("Invalid pointer passed");
        return RR_ERROR_INVALID_PARAMETER;
    }

    auto ctx = reinterpret_cast<Context*>(context);

    if (ctx->api != RR_API_VK)
    {
        Logger::Get().Error("Not supported for selected API");
        return RR_ERROR_UNSUPPORTED_INTEROP;
    }

    auto base             = reinterpret_cast<DevicePtrBase*>(device_ptr);
    auto allocated_buffer = dynamic_cast<DevicePtrBackend<BackendType::kVulkan>*>(base);
    if (!allocated_buffer)
    {
        Logger::Get().Error("Device pointer cannot be unmapped");
        return RR_ERROR_INVALID_PARAMETER;
    }

    try
    {
        allocated_buffer->Unmap();
        *mapping_ptr = nullptr;
    } catch (std::exception& e)
    {
        Logger::Get().Error(e.what());
        return RR_ERROR_INTERNAL;
    }

    Logger::Get().Debug("Unmap vulkan buffer");
    return RR_SUCCESS;
}
#endif
