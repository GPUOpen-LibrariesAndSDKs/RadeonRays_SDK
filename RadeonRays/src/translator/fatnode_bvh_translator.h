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

#include "radeon_rays.h"
#include "../accelerator/bvh.h"

#include "math/matrix.h"
#include "math/quaternion.h"
#include "math/float3.h"

#include "../util/perfect_hash_map.h"

namespace RadeonRays
{
    /// Fatnode translator transforms regular binary BVH into the form where:
    /// * Each node contains bounding boxes of its children
    /// * Both children follow parent node in the layout (breadth first)
    /// * No parent informantion is stored for the node => stacked traversal only
    ///
    class FatNodeBvhTranslator
    {
    public:
        struct Face
        {
            // Up to 3 indices
            int idx[3];
            // Shape index
            int shapeidx;
            // Primitive ID within the mesh
            int id;
            // Shape mask
            int shape_mask;
        };

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
            union
            {
                struct
                {
                    // Node's bounding box
                    bbox bounds[2];
                }s0;

                struct
                {
                    // If node is a leaf we keep vertex indices here
                    int i0, i1, i2;
                    // Address of a left child
                    int child0;
                    // Shape mask
                    int shape_mask;
                    // Shape ID
                    int shape_id;
                    // Primitive ID
                    int prim_id;
                    // Address of a right child
                    int child1;
                }s1;
            };

            Node()
                : s0()
            {

            }
        };

        //void Flush();
        void Process(Bvh& bvh);
        void InjectIndices(Face const* faces);
        //void Process(Bvh const** bvhs, int const* offsets, int numbvhs);
        //void UpdateTopLevel(Bvh const& bvh);

        std::vector<Node> nodes_;
        std::vector<int> extra_;
        std::vector<int> roots_;
        std::vector<int> indices_;
        std::vector<int> addresses_;
        int nodecnt_;
        int root_;
        std::unique_ptr<PerfectHashMap<int, int>> m_hash_map;
        int max_idx_;

    private:
        int ProcessRootNode(Bvh::Node const* node);
        //int ProcessNode(Bvh::Node const* n, int offset);

        FatNodeBvhTranslator(FatNodeBvhTranslator const&);
        FatNodeBvhTranslator& operator =(FatNodeBvhTranslator const&);
    };
}


#endif // PLAIN_BVH_TRANSLATOR_H
