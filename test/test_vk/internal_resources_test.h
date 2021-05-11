#pragma once

#include <chrono>
#include <iostream>
#include <stack>

#include "gtest/gtest.h"

#include "mesh_data.h"
#include "radeonrays_vlk.h"
#include "stb_image_write.h"

//#define USE_RENDERDOC
#ifdef USE_RENDERDOC
#define NOMINMAX
#include <windows.h>

#include "C:\\Program Files\\RenderDoc\\renderdoc_app.h"
#endif

#define CHECK_RR_CALL(x) ASSERT_EQ((x), RR_SUCCESS)

using namespace std::chrono;
template <typename T>
using VkScopedObject = std::shared_ptr<std::remove_pointer_t<T>>;

class InternalResourcesTest : public ::testing::Test
{
public:
    void SetUp() override;

    void TearDown() override {}

#ifdef USE_RENDERDOC
    RENDERDOC_API_1_4_0* rdoc_api_ = NULL;
#endif
};

void InternalResourcesTest::SetUp()
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
}
TEST_F(InternalResourcesTest, CreateContext)
{
    RRContext context = nullptr;
    CHECK_RR_CALL(rrCreateContext(RR_API_VERSION, RR_API_VK, &context));
    CHECK_RR_CALL(rrDestroyContext(context));
}

TEST_F(InternalResourcesTest, BuildObj)
{
    RRContext context = nullptr;
    CHECK_RR_CALL(rrCreateContext(RR_API_VERSION, RR_API_VK, &context));
#ifdef USE_RENDERDOC
    if (rdoc_api_)
    {
        rdoc_api_->StartFrameCapture(nullptr, nullptr);
    }
#endif
    MeshData mesh_data("../../resources/sponza.obj");

    /// memory management to pass buffers to builder
    // get radeonrays ptrs to triangle description
    RRDevicePtr vertex_ptr = nullptr;
    RRDevicePtr index_ptr  = nullptr;

    CHECK_RR_CALL(rrAllocateDeviceBuffer(context, mesh_data.positions.size() * sizeof(float), &vertex_ptr));
    CHECK_RR_CALL(rrAllocateDeviceBuffer(context, mesh_data.indices.size() * sizeof(uint32_t), &index_ptr));
    void* ptr = nullptr;
    CHECK_RR_CALL(rrMapDevicePtr(context, vertex_ptr, &ptr));
    float* pos_ptr = (float*)ptr;
    for (const auto& pos : mesh_data.positions)
    {
        *pos_ptr = pos;
        pos_ptr++;
    }
    CHECK_RR_CALL(rrUnmapDevicePtr(context, vertex_ptr, &ptr));
    CHECK_RR_CALL(rrMapDevicePtr(context, index_ptr, &ptr));
    uint32_t* ind_ptr = (uint32_t*)ptr;
    for (const auto& ind : mesh_data.indices)
    {
        *ind_ptr = ind;
        ind_ptr++;
    }
    CHECK_RR_CALL(rrUnmapDevicePtr(context, index_ptr, &ptr));

    auto triangle_count = (uint32_t)mesh_data.indices.size() / 3;

    RRGeometryBuildInput    geometry_build_input                = {};
    RRTriangleMeshPrimitive mesh                                = {};
    geometry_build_input.triangle_mesh_primitives               = &mesh;
    geometry_build_input.primitive_type                         = RR_PRIMITIVE_TYPE_TRIANGLE_MESH;
    geometry_build_input.triangle_mesh_primitives->vertices     = vertex_ptr;
    geometry_build_input.triangle_mesh_primitives->vertex_count = uint32_t(mesh_data.positions.size() / 3);

    geometry_build_input.triangle_mesh_primitives->vertex_stride    = 3 * sizeof(float);
    geometry_build_input.triangle_mesh_primitives->triangle_indices = index_ptr;
    geometry_build_input.triangle_mesh_primitives->triangle_count   = triangle_count;
    geometry_build_input.triangle_mesh_primitives->index_type       = RR_INDEX_TYPE_UINT32;
    geometry_build_input.primitive_count                            = 1u;

    std::cout << "Triangle count " << triangle_count << "\n";

    RRBuildOptions options;
    options.build_flags = 0u;

    RRMemoryRequirements geometry_reqs;
    CHECK_RR_CALL(rrGetGeometryBuildMemoryRequirements(context, &geometry_build_input, &options, &geometry_reqs));

    // allocate buffers for builder and resulting geometry
    RRDevicePtr scratch_ptr  = nullptr;
    RRDevicePtr geometry_ptr = nullptr;
    CHECK_RR_CALL(rrAllocateDeviceBuffer(context, geometry_reqs.temporary_build_buffer_size, &scratch_ptr));
    CHECK_RR_CALL(rrAllocateDeviceBuffer(context, geometry_reqs.result_buffer_size, &geometry_ptr));

    std::cout << "Scratch buffer size: " << (float)geometry_reqs.temporary_build_buffer_size / 1000000 << "Mb\n";
    std::cout << "Result buffer size: " << (float)geometry_reqs.result_buffer_size / 1000000 << "Mb\n";

    RRCommandStream command_stream = nullptr;
    CHECK_RR_CALL(rrAllocateCommandStream(context, &command_stream));

    CHECK_RR_CALL(rrCmdBuildGeometry(
        context, RR_BUILD_OPERATION_BUILD, &geometry_build_input, &options, scratch_ptr, geometry_ptr, command_stream));

    RREvent wait_event = nullptr;
    CHECK_RR_CALL(rrSumbitCommandStream(context, command_stream, nullptr, &wait_event));
    CHECK_RR_CALL(rrWaitEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseCommandStream(context, command_stream));

    // built-in intersection
    using Ray = RRRay;
    using Hit = RRHit;

    constexpr uint32_t kResolution = 640;
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

    RRDevicePtr rays_ptr, hits_ptr, scratch_trace_ptr;

    CHECK_RR_CALL(rrAllocateDeviceBuffer(context, rays.size() * sizeof(Ray), &rays_ptr));
    CHECK_RR_CALL(rrAllocateDeviceBuffer(context, hits.size() * sizeof(Hit), &hits_ptr));
    size_t scratch_trace_size;
    CHECK_RR_CALL(rrGetTraceMemoryRequirements(context, kResolution * kResolution, &scratch_trace_size));
    CHECK_RR_CALL(rrAllocateDeviceBuffer(context, scratch_trace_size, &scratch_trace_ptr));
    CHECK_RR_CALL(rrMapDevicePtr(context, rays_ptr, &ptr));
    std::memcpy(ptr, rays.data(), sizeof(Ray) * rays.size());
    CHECK_RR_CALL(rrUnmapDevicePtr(context, rays_ptr, &ptr));

    RRCommandStream trace_command_stream = nullptr;
    CHECK_RR_CALL(rrAllocateCommandStream(context, &trace_command_stream));

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

    CHECK_RR_CALL(rrSumbitCommandStream(context, trace_command_stream, nullptr, &wait_event));
    CHECK_RR_CALL(rrWaitEvent(context, wait_event));

    CHECK_RR_CALL(rrReleaseEvent(context, wait_event));
    CHECK_RR_CALL(rrReleaseCommandStream(context, trace_command_stream));

    // Map staging ray buffer.
    CHECK_RR_CALL(rrMapDevicePtr(context, hits_ptr, &ptr));
    Hit*                  mapped_ptr = (Hit*)ptr;
    std::vector<uint32_t> data(kResolution * kResolution);

    for (int y = 0; y < kResolution; ++y)
    {
        for (int x = 0; x < kResolution; ++x)
        {
            int wi = kResolution * (kResolution - 1 - y) + x;
            int i  = kResolution * y + x;

            if (mapped_ptr[i].inst_id != ~0u)
            {
                data[wi] = 0xff000000 | (uint32_t(mapped_ptr[i].uv[0] * 255) << 8) |
                           (uint32_t(mapped_ptr[i].uv[1] * 255) << 16);
            } else
            {
                data[wi] = 0xff101010;
            }
        }
    }
    CHECK_RR_CALL(rrUnmapDevicePtr(context, hits_ptr, &ptr));
    stbi_write_jpg("test_vk_sponza_geom_isect_internal.jpg", kResolution, kResolution, 4, data.data(), 120);

    CHECK_RR_CALL(rrReleaseDevicePtr(context, hits_ptr));
    CHECK_RR_CALL(rrReleaseDevicePtr(context, rays_ptr));
    CHECK_RR_CALL(rrReleaseDevicePtr(context, scratch_trace_ptr));
#ifdef USE_RENDERDOC
    if (rdoc_api_)
    {
        rdoc_api_->EndFrameCapture(nullptr, nullptr);
    }
#endif
    CHECK_RR_CALL(rrReleaseDevicePtr(context, scratch_ptr));
    CHECK_RR_CALL(rrReleaseDevicePtr(context, geometry_ptr));
    CHECK_RR_CALL(rrReleaseDevicePtr(context, index_ptr));
    CHECK_RR_CALL(rrReleaseDevicePtr(context, vertex_ptr));
    CHECK_RR_CALL(rrDestroyContext(context));
}
