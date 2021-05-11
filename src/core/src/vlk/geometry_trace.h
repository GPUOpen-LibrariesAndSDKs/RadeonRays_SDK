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

#include <cstddef>
#include <cstdint>

#include "vlk/common.h"
#include "vlk/shader_manager.h"

namespace rt::vulkan
{
/**
 * @brief Trace BVH2 geometry
 **/
class TraceGeometry
{
public:
    TraceGeometry(std::shared_ptr<GpuHelper> gpu_helper, ShaderManager const& shader_manager);
    ~TraceGeometry();

    void operator()(vk::CommandBuffer      command_list,
                    RRIntersectQuery       query,
                    RRIntersectQueryOutput query_output,
                    vk::Buffer             bvh,
                    size_t                 bvh_offset,
                    uint32_t               ray_count,
                    vk::Buffer             ray_count_buffer,
                    size_t                 ray_count_buffer_offset,
                    vk::Buffer             rays,
                    size_t                 rays_offset,
                    vk::Buffer             hits,
                    size_t                 hits_offset,
                    vk::Buffer             scratch,
                    size_t                 scratch_offset);

    size_t GetScratchSize(uint32_t ray_count) const;

private:
    vk::DescriptorSet GetDescriptor(RRIntersectQuery       query,
                                    RRIntersectQueryOutput query_output,
                                    vk::Buffer             bvh,
                                    size_t                 bvh_offset,
                                    vk::Buffer             ray_count_buffer,
                                    size_t                 ray_count_buffer_offset,
                                    vk::Buffer             rays,
                                    size_t                 rays_offset,
                                    vk::Buffer             hits,
                                    size_t                 hits_offset,
                                    vk::Buffer             scratch,
                                    size_t                 scratch_offset);

    struct TraceGeometryImpl;
    std::unique_ptr<TraceGeometryImpl> impl_;
};
}  // namespace rt::vulkan