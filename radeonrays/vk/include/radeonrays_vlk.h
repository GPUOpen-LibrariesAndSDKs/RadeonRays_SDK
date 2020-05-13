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
#ifndef RADEONRAYS_MPS_H
#define RADEONRAYS_MPS_H

#include <vulkan/vulkan.h>

#include "radeonrays.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Obtain device pointer from Vulkan buffer.
 *
 * @param context API context.
 * @param resource Vulkan device buffer (binded to memory) to request pointer to.
 * @param offset Offset within a buffer.
 * @param device_ptr RR device pointer to the resource.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrGetDevicePtrFromVkBuffer(RRContext context, VkBuffer resource, size_t offset, RRDevicePtr* device_ptr);

/** @brief Create context from existing Vulkan device.
 *
 * @param api_version API version.
 * @param device Vulkan device to use.
 * @param physical_device Vulkan physical device to use.
 * @param command_queue Vulkan handle to a queue object.
 * @param queue_family_index Vulkan family index of provided queue.
 * @param context RR context.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrCreateContextVk(uint32_t         api_version,
                                 VkDevice         device,
                                 VkPhysicalDevice physical_device,
                                 VkQueue          command_queue,
                                 uint32_t         queue_family_index,
                                 RRContext*       context);

/** @brief Obtain command stream from Vulkan command buffer.
 *
 * @param context API context.
 * @param command_buffer Vulkan command buffer.
 * @param command_stream RR command stream.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrGetCommandStreamFromVkCommandBuffer(RRContext        context,
                                                     VkCommandBuffer  command_buffer,
                                                     RRCommandStream* command_stream);

/** @brief Release command stream obtained via rrGetCommandStreamFromVkCommandList.
 *
 * @param context API context.
 * @param command_stream Command stream to release.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrReleaseExternalCommandStream(RRContext context, RRCommandStream command_stream);

/** @brief Obtain device pointer to allocated Vulkan buffer.
 * 
 * Create allocated buffer with required size with host visible and coherent properties
 *
 * @param context API context.
 * @param size Size of the required buffer.
 * @param device_ptr RR device pointer to the resource.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrAllocateDeviceBuffer(RRContext context, size_t size, RRDevicePtr* device_ptr);

/** @brief Map device pointer to host ptr.
 *
 * @param context API context.
 * @param device_ptr RR device pointer to the resource.
 * @param mapping_ptr pointer to mapping host memory.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrMapDevicePtr(RRContext context, RRDevicePtr device_ptr, void** mapping_ptr);

/** @brief Unmap device pointer to host ptr.
 *
 * @param context API context.
 * @param device_ptr RR device pointer to the resource.
 * @param mapping_ptr pointer to mapping host memory.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrUnmapDevicePtr(RRContext context, RRDevicePtr device_ptr, void** mapping_ptr);

#ifdef __cplusplus
}
#endif

#endif
