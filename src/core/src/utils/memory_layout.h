/**********************************************************************
Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace rt
{
// Round up val to a nearest multiple of a.
template <typename T, typename U>
inline T RoundUp(T val, U a)
{
    return T(((val + a - 1) / a) * a);
}

/**
 * @brief MemoryLayout represents a memory map of a contiguous memory space.
 *
 * Layout consists of consecutive blocks defined by starting offset and size.
 * An offset of each next block is aligned in accordacnce with an optional alignement
 * parameter.
 */
template <typename BlockID, typename AddrT>
class MemoryLayout
{
public:
    /**
     * @brief Constructor
     *
     * @param alignment Block offsets alignment.
     * @param base_offset Base offset for address caluclation.
     **/
    MemoryLayout(AddrT alignment = AddrT(), AddrT base_offset = AddrT()) noexcept
        : alignment_(alignment), base_offset_(base_offset)
    {
    }

    /**
     * @brief Append a block of size at least count * sizeof(T) bytes.
     *
     * @param id ID of the block.
     * @param count Number of items of type T in the block.
     **/
    template <typename T>
    void AppendBlock(BlockID id, std::size_t count)
    {
        if (blocks_.empty())
        {
            blocks_.push_back(Block{0, count * sizeof(T)});
            block_map_[id] = 0;
        } else
        {
            Block        block;
            const Block& last_block = blocks_.back();

            block.offset   = RoundUp(last_block.offset + last_block.size, alignment_);
            block.size     = count * sizeof(T);
            block_map_[id] = blocks_.size();
            blocks_.push_back(block);
        }
    }

    /**
     * @brief Get an offset of a specified block.
     *
     * @param id ID of the block.
     * @return Offset of the block (including base offset).
     **/
    AddrT offset_of(BlockID id) const
    {
        auto it = block_map_.find(id);
        assert(it != block_map_.end());
        return blocks_[it->second].offset + base_offset_;
    }

    /**
     * @brief Get a size in bytes of a specified block.
     *
     * @param id ID of the block.
     * @return Size of the block in bytes.
     **/
    std::size_t size_of(BlockID id) const
    {
        auto it = block_map_.find(id);
        assert(it != block_map_.end());
        return blocks_[it->second].size;
    }

    /**
     * @brief Get a size in elements of type T of a specified block.
     *
     * @param id ID of the block.
     * @return Size of the block in elements of type T.
     **/
    template <typename T>
    std::size_t size_in_elements(BlockID id) const
    {
        auto it = block_map_.find(id);
        assert(it != block_map_.end());
        return blocks_[it->second].size / sizeof(T);
    }

    /**
     * @brief Clear all the blocks and return memory layout to an empty state.
     **/
    void Reset()
    {
        block_map_.clear();
        blocks_.clear();
    }

    /**
     * @brief Get total size of memory layout in bytes.
     **/
    std::size_t total_size() const noexcept
    {
        if (!blocks_.empty())
        {
            const Block& last_block = blocks_.back();
            return RoundUp(last_block.offset + last_block.size, alignment_);
        }

        return 0;
    }

    /**
     * @brief Set base offset of a memory layout.
     *
     * @param base_offset Offset.
     **/
    void SetBaseOffset(AddrT base_offset) { base_offset_ = base_offset; }

private:
    struct Block
    {
        AddrT       offset = 0;
        std::size_t size   = 0;
    };

    std::unordered_map<BlockID, std::size_t> block_map_;
    std::vector<Block>                       blocks_;
    AddrT                                    alignment_   = 1;
    AddrT                                    base_offset_ = 0;
};
}  // namespace rt