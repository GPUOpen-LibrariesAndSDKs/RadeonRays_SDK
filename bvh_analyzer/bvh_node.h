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
#include "aabb.h"
#include "common.h"

namespace bvh
{
template <uint32_t FACTOR>
struct BvhNode
{
    uint32_t children_addr[FACTOR];
    Aabb     children_aabb[FACTOR];
    bool     children_is_prim[FACTOR];
    uint32_t children_count;
    uint32_t parent;
    uint32_t flag;

    Aabb GetAabb() const
    {
        Aabb aabb;
        for (uint32_t i = 0; i < children_count; i++)
        {
            aabb.Grow(children_aabb[i]);
        }
        return aabb;
    }
};

using BvhNode2 = BvhNode<2u>;

}  // namespace bvh
