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

#include "../accelerator/bvh2.h"

namespace RadeonRays
{

    class QBvhTranslator
    {
    public:
        static constexpr std::uint32_t kInvalidId = 0xffffffffu;

        // Encoded node format.
        // In QBVH we store 4x16-bits AABBs per node
        struct Node
        {
            // Min points of AABB 0 and 1 for internal node
            // or first triangle vertex for the leaf
            std::uint32_t aabb01_min_or_v0[3];
            // Address of the first child for internal node,
            // RR_INVALID_ID for the leaf
            std::uint32_t addr0 = kInvalidId;
            // Max points of AABB 0 and 1 for internal node
            // or second triangle vertex for the leaf
            std::uint32_t aabb01_max_or_v1[3];
            // Address of the second child for internal node,
            // mesh ID for the leaf
            std::uint32_t addr1_or_mesh_id = kInvalidId;
            // Min points of AABB 2 and 3 for internal node
            // or third triangle vertex for the leaf
            std::uint32_t aabb23_min_or_v2[3];
            // Address of the third child for internal node,
            // primitive ID for the leaf
            std::uint32_t addr2_or_prim_id = kInvalidId;
            // Max points of AABB 2 and 3 for internal node
            std::uint32_t aabb23_max[3];
            // Address of the fourth child for internal node
            std::uint32_t addr3 = kInvalidId;
        };

        // Constructor
        QBvhTranslator()
        {
        }

        void Process(const Bvh2 &bvh);

        inline std::size_t GetSizeInBytes() const
        {
            return nodes_.size() * sizeof(Node);
        }

        std::vector<Node> nodes_;
    };

}
