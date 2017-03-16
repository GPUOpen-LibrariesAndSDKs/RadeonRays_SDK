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
#include "plain_bvh_translator.h"

#include "../primitive/mesh.h"
#include "../primitive/instance.h"
#include "../except/except.h"

#include <cassert>
#include <stack>
#include <iostream>

namespace RadeonRays
{
    void PlainBvhTranslator::Process(Bvh& bvh)
    {
        // WARNING: this is crucial in order for the nodes not to migrate in memory as push_back adds nodes
        nodecnt_ = 0;
        int newsize = bvh.m_nodecnt;
        nodes_.resize(newsize);
        extra_.resize(newsize);

        // Check if we have been initialized
        assert(bvh.m_root);

        // Save current root position
        int rootidx = 0;

        // Process root
        ProcessNode(bvh.m_root);

        // Set next ptr
        nodes_[rootidx].bounds.pmax.w = -1;

        //
        for (int i = rootidx; i < (int)nodes_.size(); ++i)
        {
            if (nodes_[i].bounds.pmin.w != -1.f)
            {
                nodes_[i + 1].bounds.pmax.w = nodes_[i].bounds.pmin.w;
                nodes_[(int)(nodes_[i].bounds.pmin.w)].bounds.pmax.w = nodes_[i].bounds.pmax.w;
            }
        }

        for (int i = rootidx; i < (int)nodes_.size(); ++i)
        {
            if (nodes_[i].bounds.pmin.w == -1.f)
            {
                nodes_[i].bounds.pmin.w = (float)extra_[i];
            }
            else
            {
                nodes_[i].bounds.pmin.w = -1.f;
            }
        }
    }

    void PlainBvhTranslator::UpdateTopLevel(Bvh const& bvh)
    {
        nodecnt_ = root_;

        // Process root
        ProcessNode(bvh.m_root);

        // Set next ptr
        nodes_[root_].bounds.pmax.w = -1;

        for (int j = root_; j < root_ + bvh.m_nodecnt; ++j)
        {
            if (nodes_[j].bounds.pmin.w != -1.f)
            {
                nodes_[j + 1].bounds.pmax.w = nodes_[j].bounds.pmin.w;
                nodes_[(int)(nodes_[j].bounds.pmin.w)].bounds.pmax.w = nodes_[j].bounds.pmax.w;
            }
        }

        for (int j = root_; j < root_ + bvh.m_nodecnt; ++j)
        {
            if (nodes_[j].bounds.pmin.w == -1.f)
            {
                nodes_[j].bounds.pmin.w = (float)extra_[j];
            }
            else
            {
                nodes_[j].bounds.pmin.w = -1.f;
            }
        }

    }

    void PlainBvhTranslator::Process(Bvh const** bvhs, int const* offsets, int numbvhs)
    {
        // First of all count the number of required nodes for all BVH's
        int nodecnt = 0;
        for (int i = 0; i < numbvhs + 1; ++i)
        {
            if (!bvhs[i])
            {
                continue;
            }

            nodecnt += bvhs[i]->m_nodecnt;
        }

        nodes_.resize(nodecnt);
        extra_.resize(nodecnt);
        roots_.resize(numbvhs);

        for (int i = 0; i < numbvhs; ++i)
        {
            if (!bvhs[i])
            {
                continue;
            }

            int currentroot = nodecnt_;

            roots_[i] = currentroot;
            
            // Process root
            ProcessNode(bvhs[i]->m_root, offsets[i]);

            // Set next ptr
            nodes_[currentroot].bounds.pmax.w = -1;

            for (int j = currentroot; j < currentroot + bvhs[i]->m_nodecnt; ++j)
            {
                if (nodes_[j].bounds.pmin.w != -1.f)
                {
                    nodes_[j + 1].bounds.pmax.w = nodes_[j].bounds.pmin.w;
                    nodes_[(int)(nodes_[j].bounds.pmin.w)].bounds.pmax.w = nodes_[j].bounds.pmax.w;
                }
            }

            for (int j = currentroot; j < currentroot + bvhs[i]->m_nodecnt; ++j)
            {
                if (nodes_[j].bounds.pmin.w == -1.f)
                {
                    nodes_[j].bounds.pmin.w = (float)extra_[j];
                }
                else
                {
                    nodes_[j].bounds.pmin.w = -1.f;
                }
            }
        }

        // The final one
        root_ = nodecnt_;

        // Process root
        ProcessNode(bvhs[numbvhs]->m_root);

        // Set next ptr
        nodes_[root_].bounds.pmax.w = -1;

        for (int j = root_; j < root_ + bvhs[numbvhs]->m_nodecnt; ++j)
        {
            if (nodes_[j].bounds.pmin.w != -1.f)
            {
                nodes_[j + 1].bounds.pmax.w = nodes_[j].bounds.pmin.w;
                nodes_[(int)(nodes_[j].bounds.pmin.w)].bounds.pmax.w = nodes_[j].bounds.pmax.w;
            }
        }

        for (int j = root_; j < root_ + bvhs[numbvhs]->m_nodecnt; ++j)
        {
            if (nodes_[j].bounds.pmin.w == -1.f)
            {
                nodes_[j].bounds.pmin.w = (float)extra_[j];
            }
            else
            {
                nodes_[j].bounds.pmin.w = -1.f;
            }
        }

    }

    int PlainBvhTranslator::ProcessNode(Bvh::Node const* n)
    {
        int idx = nodecnt_;
        //std::cout << "Index " << idx << "\n";
        Node& node = nodes_[nodecnt_];
        node.bounds = n->bounds;
        int& extra = extra_[nodecnt_++];

        if (n->type == Bvh::kLeaf)
        {
            int startidx = n->startidx;
            extra = (startidx << 4) | (n->numprims & 0xF);
            node.bounds.pmin.w = -1.f;
        }
        else
        {
            ProcessNode(n->lc);
            node.bounds.pmin.w = (float)ProcessNode(n->rc);
        }

        return idx;
    }

    int PlainBvhTranslator::ProcessNode(Bvh::Node const* n, int offset)
    {
        int idx = nodecnt_;
        //std::cout << "Index " << idx << "\n";
        Node& node = nodes_[nodecnt_];
        node.bounds = n->bounds;
        int& extra = extra_[nodecnt_++];

        if (n->type == Bvh::kLeaf)
        {
            int startidx = n->startidx + offset;
            extra = (startidx << 4) | (n->numprims & 0xF);
            node.bounds.pmin.w = -1.f;
        }
        else
        {
            ProcessNode(n->lc, offset);
            node.bounds.pmin.w = (float)ProcessNode(n->rc, offset);
        }

        return idx;
    }


    void PlainBvhTranslator::Flush()
    {
        nodecnt_ = 0;
        root_ = 0;
        roots_.resize(0);
        nodes_.resize(0);
        extra_.resize(0);
    }
}
