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
#ifndef FATNODE_BVH_TRANSLATOR_H
#define FATNODE_BVH_TRANSLATOR_H

#include <map>

#include "firerays.h"
#include "../accelerator/bvh.h"

#include "math/matrix.h"
#include "math/quaternion.h"
#include "math/float3.h"

namespace FireRays
{
    /// Fatnode translator transforms regular binary BVH into the form where:
    /// * Each node contains bounding boxes of its children
    /// * Both children follow parent node in the layuot (breadth first)
    /// * No parent informantion is stored for the node => stacked traversal only
    /// 
    class FatNodeBvhTranslator
    {
    public:
        // Constructor
        FatNodeBvhTranslator()
            : nodecnt_(0)
            , root_(0)
        {
        }

        // Fat BVH node
        // Encoding:
        // xbound.pmin.w == -1.f if x-child is an internal node otherwise triangle index
        //
        struct Node
        {
            // Node's bounding box
            bbox lbound;
            bbox rbound;
        };

        void Flush();
        void Process(Bvh& bvh);
        //void Process(Bvh const** bvhs, int const* offsets, int numbvhs);
        //void UpdateTopLevel(Bvh const& bvh);

        std::vector<Node> nodes_;
        std::vector<int>  extra_;
        std::vector<int>  roots_;
        int nodecnt_;
        int root_;

    private:
        int ProcessRootNode(Bvh::Node const* node);
        //int ProcessNode(Bvh::Node const* n, int offset);

        FatNodeBvhTranslator(FatNodeBvhTranslator const&);
        FatNodeBvhTranslator& operator =(FatNodeBvhTranslator const&);
    };
}


#endif // PLAIN_BVH_TRANSLATOR_H
