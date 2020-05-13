#pragma once

#include <chrono>
#include <iostream>
#include <stack>

#include "mesh_data.h"
#include <iostream>

#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#include "radeonrays_mtl.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define CHECK_RR_CALL(x) do {if((x) != RR_SUCCESS) { throw std::runtime_error("Incorrect radeonrays call");}} while(false)

using namespace std::chrono;

class Application
{
public:
    void Run();
};

namespace
{
    id<MTLDevice> ChooseDevice()
    {
        auto devices = MTLCopyAllDevices();
        id<MTLDevice> dev;
   
        int result_score = 0;
        for (id<MTLDevice> device : devices)
        {
            
            if (MPSSupportsMTLDevice(device))
            {
                int score = 1;
                if (!device.removable)
                {
                    score += 2;
                }
                if (!device.lowPower)
                {
                    score += 4;
                }
                if (score > result_score)
                {
                    result_score = score;
                    dev = device;
                }
            }
        }
        return dev;
    }
}

void Application::Run()
{
    auto device = ChooseDevice();
    auto cmd_queue = [device newCommandQueue];

    RRContext context = nullptr;
    CHECK_RR_CALL(rrCreateContextMTL(RR_API_VERSION, device, cmd_queue, &context));

    MeshData mesh_data("scene.obj");

    size_t vertex_size = sizeof(float) * mesh_data.positions.size();
    size_t indices_size = sizeof(uint32_t) * mesh_data.indices.size();

    id<MTLBuffer> indices_buf = [device newBufferWithLength:indices_size options:MTLResourceStorageModeManaged];
    memcpy(indices_buf.contents, mesh_data.indices.data(), indices_buf.length);
    id<MTLBuffer> vertices_buf = [device newBufferWithLength:vertex_size options:MTLResourceStorageModeManaged];
    memcpy(vertices_buf.contents, mesh_data.positions.data(), vertices_buf.length);
    /// update buffers
    [indices_buf didModifyRange:NSMakeRange(0, indices_buf.length)];
    [vertices_buf didModifyRange:NSMakeRange(0, vertices_buf.length)];

    RRDevicePtr vertex_ptr = nullptr;
    RRDevicePtr index_ptr  = nullptr;
    CHECK_RR_CALL(rrGetDevicePtrFromMTLResource(context, vertices_buf, 0, &vertex_ptr));
    CHECK_RR_CALL(rrGetDevicePtrFromMTLResource(context, indices_buf, 0, &index_ptr));

    RRGeometryBuildInput    geometry_build_input            = {};
    RRTriangleMeshPrimitive mesh                            = {};
    geometry_build_input.triangle_mesh_primitives           = &mesh;
    geometry_build_input.primitive_type                     = RR_PRIMITIVE_TYPE_TRIANGLE_MESH;
    geometry_build_input.triangle_mesh_primitives->vertices = vertex_ptr;
    geometry_build_input.triangle_mesh_primitives->vertex_count = mesh_data.positions.size() / 3;
    geometry_build_input.triangle_mesh_primitives->vertex_stride    = 3 * sizeof(float);
    geometry_build_input.triangle_mesh_primitives->triangle_indices = index_ptr;
    geometry_build_input.triangle_mesh_primitives->triangle_count = mesh_data.indices.size() / 3;
    geometry_build_input.triangle_mesh_primitives->index_type = RR_INDEX_TYPE_UINT32;
    geometry_build_input.primitive_count                      = 1;

    RRBuildOptions options;
    options.build_flags = RR_BUILD_FLAG_BITS_ALLOW_UPDATE;
    options.backend_specific_info = nullptr;
    RRDevicePtr geometry_ptr = nullptr;
    CHECK_RR_CALL(rrGetDevicePtrFromMTLResource(context, nullptr, 0, &geometry_ptr));

    RRCommandStream build_command_stream = nullptr;
    CHECK_RR_CALL(rrAllocateCommandStream(context, &build_command_stream));

    CHECK_RR_CALL(rrCmdBuildGeometry(
        context, RR_BUILD_OPERATION_BUILD, &geometry_build_input, &options, nullptr, geometry_ptr, build_command_stream));

    RREvent wait_event = nullptr;
    CHECK_RR_CALL(rrSumbitCommandStream(context, build_command_stream, nullptr, &wait_event));

    CHECK_RR_CALL(rrWaitEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseCommandStream(context, build_command_stream));

    using Ray = RRRay;
    using Hit = RRHit;
    RRCommandStream trace_command_stream = nullptr;
    CHECK_RR_CALL(rrAllocateCommandStream(context, &trace_command_stream));

    constexpr uint32_t kResolution = 640;
    std::vector<Ray> rays(kResolution * kResolution);
    std::vector<Hit> hits(kResolution * kResolution);

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
    size_t rays_size = sizeof(Ray) * rays.size();
    size_t hits_size = sizeof(Hit) * hits.size();

    id<MTLBuffer> rays_buf = [device newBufferWithLength:rays_size options:MTLResourceStorageModeManaged];
    memcpy(rays_buf.contents, rays.data(), rays_buf.length);
    [rays_buf didModifyRange:NSMakeRange(0, rays_buf.length)];
    id<MTLBuffer> hits_buf = [device newBufferWithLength:hits_size options:MTLResourceStorageModeShared];
    
    RRDevicePtr rays_ptr = nullptr;
    RRDevicePtr hits_ptr  = nullptr;
    CHECK_RR_CALL(rrGetDevicePtrFromMTLResource(context, rays_buf, 0, &rays_ptr));
    CHECK_RR_CALL(rrGetDevicePtrFromMTLResource(context, hits_buf, 0, &hits_ptr));
    size_t scratch_size = 0;
    RRDevicePtr scratch_trace_ptr  = nullptr;
    CHECK_RR_CALL(rrGetTraceMemoryRequirements(context, rays.size(), &scratch_size));
    id<MTLBuffer> scratch_trace_buf = [device newBufferWithLength:scratch_size options:MTLResourceStorageModeManaged];
    CHECK_RR_CALL(rrGetDevicePtrFromMTLResource(context, scratch_trace_buf, 0, &scratch_trace_ptr));

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
    RREvent trace_wait_event = nullptr;
    CHECK_RR_CALL(rrSumbitCommandStream(context, trace_command_stream, nullptr, &trace_wait_event));

    CHECK_RR_CALL(rrWaitEvent(context, trace_wait_event));
    CHECK_RR_CALL(rrReleaseEvent(context, trace_wait_event));
    CHECK_RR_CALL(rrReleaseCommandStream(context, trace_command_stream));
    /// update buffer
    memcpy(hits.data(), hits_buf.contents, hits_buf.length);
    std::vector<uint32_t> data(kResolution * kResolution);
    for (int y = 0; y < kResolution; ++y)
    {
        for (int x = 0; x < kResolution; ++x)
        {
            int wi = kResolution * (kResolution - 1 - y) + x;
            int i  = kResolution * y + x;

            if (hits[i].inst_id != 0xFFFFFFFF)
            {
                data[wi] = 0xff000000 | (uint32_t(hits[i].uv[0] * 255) << 8) |
                           (uint32_t(hits[i].uv[1] * 255) << 16);
            } else
            {
                 data[wi] = 0xff101010;
            }
        }
    }
    stbi_write_jpg("mtl_geom.jpg", kResolution, kResolution, 4, data.data(), 120);

    CHECK_RR_CALL(rrDestroyContext(context));
}