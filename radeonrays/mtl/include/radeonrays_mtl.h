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

#include "radeonrays.h"
#import <Metal/Metal.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Obtain device pointer from Metal buffer.
 *
 * @param context API context.
 * @param resource Metal resource to request pointer to.
 * @param offset Offset within a buffer.
 * @param device_ptr RR device pointer to the resource.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrGetDevicePtrFromMTLResource(RRContext       context,
                                             id<MTLBuffer>   resource,
                                             size_t          offset,
                                             RRDevicePtr*    device_ptr);

/** @brief Create context from existing Metal device.
 *
 * @param api_version API version.
 * @param device Metal device to use.
 * @param command_queue Metal command queue for the context.
 * @param context RR context.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrCreateContextMTL(uint32_t            api_version,
                                  id<MTLDevice>       device,
                                  id<MTLCommandQueue> command_queue,
                                  RRContext*          context);

 /** @brief Create MPS group to combine geometries with scenes
 * Should be passed as backend_specific_info to build API functions
 * @param context RR context.
 * @param device_ptr RR device pointer to group.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrCreateGroupMPS(RRContext context,
                                RRDevicePtr* device_ptr);                         


/** @brief Obtain command stream from MTL command buffer.
 * 
 * @param context API context.
 * @param command_list Metal command buffer.
 * @param command_stream RR command stream.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrGetCommandStreamFromMTLCommandBuffer(RRContext            context,
                                                      id<MTLCommandBuffer> command_buffer,
                                                      RRCommandStream*     command_stream);

/** @brief Release command stream obtained via rrGetCommandStreamFromMTLCommandList.
 *
 * @param context API context.
 * @param command_stream Command stream to release.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrReleaseExternalCommandStream(RRContext context, RRCommandStream command_stream);

#ifdef __cplusplus
}
#endif

#endif
