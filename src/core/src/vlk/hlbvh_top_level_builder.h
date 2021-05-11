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

#include "base/command_stream_base.h"
#include "vlk/common.h"
#include "vlk/gpu_helper.h"
#include "vlk/shader_manager.h"

namespace rt::vulkan
{
/**
 * @brief HLBVH builder.
 *
 * Build linear BVH.
 **/
class BuildHlBvhTopLevel
{
public:
    BuildHlBvhTopLevel(std::shared_ptr<GpuHelper> gpu_helper, ShaderManager const& shader_manager);
    ~BuildHlBvhTopLevel();
    /**
     * @brief Build BVH.
     *
     *  Given an a set of instance descs, build a linear BVH.
     *
     **/
    void operator()(vk::CommandBuffer            command_buffer,
                    vk::Buffer                   instance_descs,
                    size_t                       instance_desc_offset,
                    ChildrenBvhsContainer const& instances,
                    uint32_t                     instance_count,
                    vk::Buffer                   scratch,
                    size_t                       scratch_offset,
                    vk::Buffer                   result,
                    size_t                       result_offset);

    /**
     * @brief Get size if bytes required to hold resulting BVH.
     *
     * @param instance_count Number of instances
     **/
    size_t GetResultDataSize(uint32_t instance_count) const;

    /**
     * @brief Get size if bytes required for scratch space.
     *
     * @param instance_count Number of instances
     **/
    size_t GetScratchDataSize(uint32_t instance_count) const;

private:
    void AdjustLayouts(uint32_t triangle_count) const;
    void UpdateDescriptors(vk::Buffer                   instance_descs,
                           size_t                       instance_desc_offset,
                           ChildrenBvhsContainer const& instances,
                           vk::Buffer                   scratch,
                           size_t                       scratch_offset,
                           vk::Buffer                   result,
                           size_t                       result_offset);

private:
    struct HlBvhTopLevelImpl;
    std::unique_ptr<HlBvhTopLevelImpl> impl_;
};

}  // namespace rt::vulkan