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

#include "bvh.h"


namespace RadeonRays
{
    class SplitBvh : public Bvh
    {
    public:
        SplitBvh()
        : Bvh(true)
        {
        }
        
        ~SplitBvh();
        
        
    protected:
        struct PrimRef;
        using PrimRefArray = std::vector<PrimRef>;
        
        enum class SplitType
        {
            kObject,
            kSpatial
        };
        
        // Build function
        void BuildImpl(bbox const* bounds, int numbounds) override;
        void BuildNode(SplitRequest& req, PrimRefArray& primrefs);
        
        SahSplit FindObjectSahSplit(SplitRequest const& req, PrimRefArray const& refs) const;
        SahSplit FindSpatialSahSplit(SplitRequest const& req, PrimRefArray const& refs) const;
        
        void SplitPrimRefs(SahSplit const& split, SplitRequest const& req, PrimRefArray& refs, int& extra_refs);
        bool SplitPrimRef(PrimRef const& ref, int axis, float split, PrimRef& leftref, PrimRef& rightref) const;
        
    private:
        SplitBvh(SplitBvh const&);
        SplitBvh& operator = (SplitBvh const&);
        
        std::vector<int> m_indices;
        std::vector<bbox> m_bounds;
        
        friend class PlainBvhTranslator;
        friend class FatNodeBvhTranslator;
    };
    
    struct SplitBvh::PrimRef
    {
        // Prim bounds
        bbox bounds;
        float3 center;
        int idx;
    };
    
    inline SplitBvh::~SplitBvh()
    {
    }
    
}