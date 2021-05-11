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
#pragma once

#include "base/intersector_base.h"
#include "dx/shader_compiler.h"

namespace rt::dx
{
/**
 * @brief DX12 raytracing interface implementation.
 *
 * DX12 fallback intersector is using common DirectX12 features.
 */
class Dx12Intersector : public IntersectorBase
{
public:
    /// Constructor.
    Dx12Intersector(ID3D12Device* d3d_device);
    /// Destructor.
    ~Dx12Intersector() = default;

    /********************************************
     * IntersectorBase interface overrrides.
     ********************************************/
    /**
     * @brief Get an info on build memory requirements.
     *
     * @param build_info Array of mesh build data.
     *
     * @return Memory requirements to build specified mesh.
     **/
    PreBuildInfo GetTriangleMeshPreBuildInfo(const std::vector<TriangleMeshBuildInfo>& build_info,
                                             const RRBuildOptions*                     build_options) override;

    /**
     * @brief Get an info on build memory requirements.
     *
     * @param instance_count Number of instances in the scene.
     *
     * @return Memory requirements to build specified mesh.
     **/
    PreBuildInfo GetScenePreBuildInfo(uint32_t instance_count, const RRBuildOptions* build_options) override;

    /**
     * @brief Build triangle mesh.
     *
     * Record a command to build a triangle mesh.
     *
     * @param command_stream Command stream to record to.
     * @param build_info Array of mesh data (mesh can have multiple sub-meshes)
     * @param temporary_buffer Temporary memory needed during the build.
     * @param geometry_buffer A buffer to put result into.
     **/
    void BuildTriangleMesh(CommandStreamBase*                        command_stream_base,
                           const std::vector<TriangleMeshBuildInfo>& build_info,
                           const RRBuildOptions*                     build_options,
                           DevicePtrBase*                            temporary_buffer,
                           DevicePtrBase*                            geometry_buffer) override;

    /**
     * @brief Update triangle mesh.
     *
     * Record a command to update a triangle mesh. Updates vertex data only, can't
     * change mesh topology.
     *
     * @param command_stream Command stream to record to.
     * @param build_info Array of mesh data (mesh can have multiple sub-meshes)
     * @param temporary_buffer Temporary memory needed during the build.
     * @param geometry_buffer A buffer to put result into.
     **/
    void UpdateTriangleMesh(CommandStreamBase*                        command_stream_base,
                            const std::vector<TriangleMeshBuildInfo>& build_info,
                            const RRBuildOptions*                     build_options,
                            DevicePtrBase*                            temporary_buffer,
                            DevicePtrBase*                            geometry_buffer) override;

    /**
     * @brief Build a scene.
     *
     * Record a command to build a scene.
     *
     * @param command_stream Command stream to record to.
     * @param instances Array of instance data.
     * @param instance_count Number of instances in the array.
     * @param temporary_buffer Temporary memory needed during the build.
     * @param scene_buffer A buffer to put result into.
     **/
    void BuildScene(CommandStreamBase*    command_stream_base,
                    const RRInstance*     instances,
                    uint32_t              instance_count,
                    const RRBuildOptions* build_options,
                    DevicePtrBase*        temporary_buffer,
                    DevicePtrBase*        scene_buffer) override;

    /**
     * @brief Record intersect command into command stream.
     *
     * Intersects ray_count rays from rays buffer with a scene from scene_buffer.
     * Optionally takes a counter from GPU memory in indirect_ray_count buffer (if it is not nullptr).
     * In this case ray_count threads are launched, but indirect_ray_count rays are used =>
     * ray_count should be >= *indirect_ray_count.
     *
     * @param command_stream Command stream to record to.
     * @param scene_buffer Scene acceleration structure.
     * @param query Type of raytracing query.
     * @param ray Buffer containing rays.
     * @param ray_count Number of rays to intersect.
     * @param indirect_ray_count Optional buffer containing number of rays in GPU memory.
     * @param query_output Type of output produced by the intersector.
     * @param hits Buffer to write hit data to.
     **/
    void Intersect(CommandStreamBase*     command_stream,
                   DevicePtrBase*         scene_buffer,
                   RRIntersectQuery       query,
                   DevicePtrBase*         rays,
                   uint32_t               ray_count,
                   DevicePtrBase*         indirect_ray_count,
                   RRIntersectQueryOutput query_output,
                   DevicePtrBase*         hits,
                   DevicePtrBase*         scratch) override;

    /**
     * @brief Get memory requirements for trace scracth buffer.
     *
     * @param context RR API context.
     * @param ray_count Number of rays in the buffer (or max number of rays if indirect_ray_count is supplied).
     * @return Size of scratch buffer.
     */
    size_t GetTraceMemoryRequirements(uint32_t ray_count) override;

private:
    // Pimpl.
    struct Dx12IntersectorImpl;
    std::unique_ptr<Dx12IntersectorImpl> impl_;
};

/// Create Dx12 intersector from existing device.
std::unique_ptr<IntersectorBase> CreateDx12Intersector(ID3D12Device* d3d_device);
}  // namespace rt::dx