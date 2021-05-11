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
#ifndef RADEONRAYS_DX_H
#define RADEONRAYS_DX_H

#include "radeonrays.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ID3D12Resource;
struct ID3D12Device;
struct ID3D12CommandQueue;
struct ID3D12GraphicsCommandList;

/** @brief Obtain device pointer from D3D12 buffer.
 *
 * @param context API context.
 * @param resource D3D12 resource to request pointer to.
 * @param offset Offset within a buffer.
 * @param device_ptr RadeonRays device pointer to the resource.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrGetDevicePtrFromD3D12Resource(RRContext       context,
                                               ID3D12Resource* resource,
                                               size_t          offset,
                                               RRDevicePtr*    device_ptr);

/** @brief Create DX12 compute context from existing D3D device.
 *
 * @param api_version API API version.
 * @param d3d_device D3D12 device to use.
 * @param command_queue D3D12 command queue for the context.
 * @param context RadeonRays context.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrCreateContextDX(uint32_t            api_version,
                                 ID3D12Device*       d3d_device,
                                 ID3D12CommandQueue* command_queue,
                                 RRContext*          context);

/** @brief Obtain command stream from D3D12 graphics command list.
 *
 * @param context API context.
 * @param command_list D3D12 command list.
 * @param command_stream Command stream.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrGetCommandStreamFromD3D12CommandList(RRContext                  context,
                                                      ID3D12GraphicsCommandList* command_list,
                                                      RRCommandStream*           command_stream);

#ifdef __cplusplus
}
#endif

#endif // RADEONRAYS_DX_H
