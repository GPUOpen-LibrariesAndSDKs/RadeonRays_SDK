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

#include "command_stream.h"

namespace rt::vulkan
{
AllocatedBuffer* CommandStream::GetAllocatedTemporaryBuffer(size_t size)
{
    auto temp_buffer = device_.AcquireTemporaryBuffer(size);
    temp_buffers_.push_back(temp_buffer);
    return temp_buffer;
}

CommandStream::~CommandStream()
{
    if (external_)
    {
        for (auto& buffer : temp_buffers_)
        {
            device_.ReleaseTemporaryBuffer(buffer);
        }
    }
}
vk::CommandBuffer CommandStream::Get() const { return command_buffer_; }
void              CommandStream::Set(vk::CommandBuffer command_buffer) { command_buffer_ = command_buffer; }

void     rt::vulkan::CommandStream::ClearTemporaryBuffers()
{
    for (auto& buffer : temp_buffers_)
    {
        device_.ReleaseTemporaryBuffer(buffer);
    }
    temp_buffers_.clear();
}
}  // namespace rt::vulkan