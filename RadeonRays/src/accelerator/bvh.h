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
#ifndef BVH_H
#define BVH_H

#include <memory>
#include <vector>
#include <atomic>


#include "math/bbox.h"

//#define USE_TBB
#ifdef USE_TBB
#define NOMINMAX
#include <tbb/task_group.h>
#include <tbb/mutex.h>
#endif

namespace RadeonRays
{
    ///< The class represents bounding volume hierarachy
    ///< intersection accelerator
    ///<
    class Bvh
    {
    public:
        Bvh(bool usesah = false)
            : root_(nullptr)
            , usesah_(usesah)
			, height_(0)
        {
        }

        ~Bvh();

        // World space bounding box
        bbox const& Bounds() const;

        // Build function
        void Build(bbox const* bounds, int numbounds);
        
        // Get reordered prim indices
        int const* GetIndices() const { return &primids_[0]; }

		// Tree height
		int height() const { return height_; }

    protected:
        // Build function
        virtual void BuildImpl(bbox const* bounds, int numbounds);
        // BVH node
        struct Node;
        // Node allocation
        virtual Node* AllocateNode();
        virtual void  InitNodeAllocator(size_t maxnum);

        struct SplitRequest
        {
            // Starting index of a request
            int startidx;
            // Number of primitives
            int numprims;
            // Root node
            Node** ptr;
            // Bounding box
            bbox bounds;
            // Centroid bounds
            bbox centroid_bounds;
            // Level
            int level;
        };

        struct SahSplit
        {
            int dim;
            float split;
        };

        void BuildNode(SplitRequest const& req, bbox const* bounds, float3 const* centroids, int* primindices);

        SahSplit FindSahSplit(SplitRequest const& req, bbox const* bounds, float3 const* centroids, int* primindices) const;

        // Enum for node type
        enum NodeType
        {
            kInternal,
            kLeaf
        };

        // Bvh nodes
        std::vector<Node> nodes_;
        // Identifiers of leaf primitives
        std::vector<int> primids_;

#ifdef USE_TBB
        tbb::atomic<int> nodecnt_;
        tbb::mutex primitive_mutex_;
        tbb::task_group taskgroup_;
#else
        // Node allocator counter, atomic for thread safety
        std::atomic<int> nodecnt_;
#endif
        // Bounding box containing all primitives
        bbox bounds_;
        // Root node
        Node* root_;
        // SAH flag
        bool usesah_;
		// Tree height
		int height_;


    private:
        Bvh(Bvh const&);
        Bvh& operator = (Bvh const&);

        friend class PlainBvhTranslator;
        friend class FatNodeBvhTranslator;
    };

    struct Bvh::Node
    {
        // Node bounds in world space
        bbox bounds;
        // Type of the node
        NodeType type;

        union
        {
            // For internal nodes: left and right children
            struct
            {
                Node* lc;
                Node* rc;
            };

            // For leaves: starting primitive index and number of primitives
            struct
            {
                int startidx;
                int numprims;
            };
        };
    };

    inline Bvh::~Bvh()
    {
    }
}

#endif // BVH_H