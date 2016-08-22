/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

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

#include <atomic>
#include <cassert>
#include "buffer.h"

namespace Calc {

    // Structure for mappedmemory.
    struct MappedMemory
    {
        uint8_t*    data;
        uint32_t    type;        // one of Calc::MapType
        size_t        offset;
        size_t        size;
    };

    // Class that represent Vulkan implementation of a Buffer
    class BufferVulkan : public Buffer {
        friend class DeviceVulkanw;
    public:
        BufferVulkan(Anvil::Buffer *inBuffer, bool inCreatedInternally)
                : Buffer(), m_anvil_buffer(inBuffer)
                  , m_created_internally(inCreatedInternally)
                  , m_fence_id(0) {
            ::memset(&m_mapped_memory, 0, sizeof(m_mapped_memory));
        }

        ~BufferVulkan() {
            m_anvil_buffer->release();
        }

        std::size_t GetSize() const override {
            return static_cast< std::size_t >( m_anvil_buffer->get_size());
        }

        Anvil::Buffer *GetAnvilBuffer() const { return m_anvil_buffer; }

        // set mapped memory info. the memory is mapped using a proxy allocation. if read, first the content of Vulkan buffer is copied to the proxy allocation. if write, the proxy buffer is filled with data and then copied to Vulkan buffer.
        void SetMappedMemory(uint8_t *inMappedMemory, uint32_t inMapType,
                             size_t inOffset, size_t inSize) {
            assert((nullptr == m_mapped_memory.data) ||
                   (nullptr == inMappedMemory));
            m_mapped_memory.data = inMappedMemory;
            m_mapped_memory.type = inMapType;
            m_mapped_memory.size = inSize;
            m_mapped_memory.offset = inOffset;
        }

        const MappedMemory &GetMappedMemory() const {
            return m_mapped_memory;
        }

        // whether a buffer is internally created by RadeonRays Vulkan implementation. It's used when single value is passed to shaders.
        bool GetIsCreatedInternally() const { return m_created_internally; }

    private:
        void SetFenceId( uint64_t id ) { m_fence_id = id; }

        Anvil::Buffer *m_anvil_buffer;
        MappedMemory m_mapped_memory;
        bool m_created_internally;

        std::atomic<uint64_t> m_fence_id;
    };

}