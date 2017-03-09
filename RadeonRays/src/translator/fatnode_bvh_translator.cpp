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
#include "fatnode_bvh_translator.h"

#include "../primitive/mesh.h"
#include "../primitive/instance.h"
#include "../except/except.h"

#include <cassert>
#include <queue>
#include <iostream>

namespace RadeonRays
{
    void FatNodeBvhTranslator::Process(Bvh& bvh)
    {
        // WARNING: this is crucial in order for the nodes not to migrate in memory as push_back adds nodes
        nodecnt_ = 0;
        max_idx_ = -1;
        int newsize = bvh.m_nodecnt;
        nodes_.resize(newsize);
        extra_.resize(newsize);
        indices_.resize(newsize);
        addresses_.resize(newsize);

        // Check if we have been initialized
        assert(bvh.m_root);

        // Process root
        ProcessRootNode(bvh.m_root);

        nodes_.resize(nodecnt_);
        extra_.resize(nodecnt_);
        indices_.resize(nodecnt_);
        addresses_.resize(nodecnt_);

        // Build a hash
        m_hash_map.reset(new PerfectHashMap<int, int>(max_idx_, &indices_[0], &addresses_[0], (int)indices_.size(), -1));


        std::cout << "Finished hash building\n";

//#define CONSISTENCY_TEST
//#ifdef CONSISTENCY_TEST
//        for (int i = 0; i < indices_.size(); ++i)
//        {
//            auto val = (*m_hash_map)[indices_[i]];
//            assert(val == addresses_[i]);
//        }
//#endif
    }


    int FatNodeBvhTranslator::ProcessRootNode(Bvh::Node const* root)
    {
        // Keep the nodes to process here
        std::queue<std::pair<Bvh::Node const*, int> > workqueue;

        workqueue.push(std::make_pair(root, 0));

        while (!workqueue.empty())
        {
            auto current = workqueue.front();
            workqueue.pop();

            Node& node(nodes_[nodecnt_]);
            indices_[nodecnt_] = current.first->index;
            addresses_[nodecnt_] = nodecnt_;
            ++nodecnt_;

            if (current.first->index > max_idx_)
            {
                max_idx_ = current.first->index;
            }

            node.lbound = current.first->lc->bounds;
            if (current.first->lc->type == Bvh::NodeType::kInternal)
            {
                node.lbound.pmin.w = -1.f;
                workqueue.push(std::make_pair(current.first->lc, nodecnt_));
            }
            else
            {
                node.lbound.pmin.w = (float)((current.first->lc->startidx << 4 ) | (current.first->lc->numprims & 0xF));
            }

            node.rbound = current.first->rc->bounds;
            if (current.first->rc->type == Bvh::NodeType::kInternal)
            {
                node.rbound.pmin.w = -1.f;
                workqueue.push(std::make_pair(current.first->rc, -nodecnt_));
            }
            else
            {
                node.rbound.pmin.w = (float)((current.first->rc->startidx << 4) | (current.first->rc->numprims & 0xF));
            }

            if (current.second > 0)
            {
                nodes_[current.second - 1].lbound.pmax.w = (float)(nodecnt_ - 1);
            }
            else if(current.second < 0)
            {
                nodes_[-current.second - 1].rbound.pmax.w = (float)(nodecnt_ - 1);
            }
        }

        return 0;
    }
}
