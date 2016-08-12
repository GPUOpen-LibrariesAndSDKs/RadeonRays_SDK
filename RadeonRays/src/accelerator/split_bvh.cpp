#include "split_bvh.h"
#include "mathutils.h"

#include <cassert>

namespace RadeonRays
{
    static float3 clamp3(float3 val, float3 a, float3 b)
    {
        return float3{ clamp(val.x, a.x, b.x), clamp(val.y, a.y, b.y), clamp(val.z, a.z, b.z)};
    }
    
    void SplitBvh::BuildImpl(bbox const* bounds, int numbounds)
    {
        // Initialize prim refs structures
        PrimRefArray primrefs(numbounds);
        
        // Keep centroids to speed up partitioning
        std::vector<float3> centroids(numbounds);
        bbox centroid_bounds;
        
        for(auto i = 0; i < numbounds; ++i)
        {
            primrefs[i] = PrimRef {bounds[i], bounds[i].center(), i};
            
            auto c = bounds[i].center();
            centroid_bounds.grow(c);
        }
        
        // TODO: remove this
        InitNodeAllocator(100 * numbounds);
        
        SplitRequest init = { 0, numbounds, nullptr, bounds_, centroid_bounds, 0 };
        
        // Start from the top
        BuildNode(init, primrefs);
    }
    
    void SplitBvh::BuildNode(SplitRequest& req, PrimRefArray& primrefs)
    {
        // Update current height
        height_ = std::max(height_, req.level);
        
        // Allocate new node
        Node* node = AllocateNode();
        node->bounds = req.bounds;
        
        // Create leaf node if we have enough prims
        if (req.numprims < 2)
        {
            node->type = kLeaf;
            node->startidx = m_indices.size();
            node->numprims = req.numprims;
            
            for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
            {
                m_indices.push_back(primrefs[i].idx);
                m_bounds.push_back(primrefs[i].bounds);
            }
        }
        else
        {
            node->type = kInternal;
            
            // Choose the maximum extent
            int axis = req.centroid_bounds.maxdim();
            float border = req.centroid_bounds.center()[axis];
            
            SahSplit os = FindObjectSahSplit(req, primrefs);
            SahSplit ss = FindSpatialSahSplit(req, primrefs);
            
            auto split_type = SplitType::kObject;
            
            if (req.level < 10)
            {
                if (!is_nan(ss.split) && ss.sah < os.sah)
                {
                    split_type = SplitType::kSpatial;
                }
            }
            
            if (split_type == SplitType::kSpatial)
            {
                // First we need maximum 2x numprims elements allocated
                if (primrefs.size() < req.startidx + req.numprims * 2)
                {
                    primrefs.resize(req.startidx + req.numprims * 2);
                }
                
                // Split prim refs and add extra refs to request
                int extra_refs = 0;
                SplitPrimRefs(ss, req, primrefs, extra_refs);
                req.numprims += extra_refs;
            }
            
            // Start partitioning and updating extents for children at the same time
            bbox leftbounds, rightbounds, leftcentroid_bounds, rightcentroid_bounds;
            int splitidx = req.startidx;
            
            bool near2far = (req.numprims + req.startidx) & 0x1;
            
            auto cmpl = [](float a, float b) -> bool { return a < b; };
            auto cmpge = [](float a, float b) -> bool { return a >= b; };
            auto cmp1 = near2far ? cmpl : cmpge;
            auto cmp2 = near2far ? cmpge : cmpl;
            
            if (req.centroid_bounds.extents()[axis] > 0.f)
            {
                auto first = req.startidx;
                auto last = req.startidx + req.numprims;

                    while (1)
                    {
                        while ((first != last) && cmp1(primrefs[first].center[axis], border))
                        {
                            leftbounds.grow(primrefs[first].bounds);
                            leftcentroid_bounds.grow(primrefs[first].center);
                            ++first;
                        }
                        
                        if (first == last--) break;
                        
                        rightbounds.grow(primrefs[first].bounds);
                        rightcentroid_bounds.grow(primrefs[first].center);
                        
                        while ((first != last) && cmp2(primrefs[last].center[axis], border))
                        {
                            rightbounds.grow(primrefs[last].bounds);
                            rightcentroid_bounds.grow(primrefs[last].center);
                            --last;
                        }
                        
                        if (first == last) break;
                        
                        leftbounds.grow(primrefs[last].bounds);
                        leftcentroid_bounds.grow(primrefs[last].center);
                        
                        std::swap(primrefs[first++], primrefs[last]);
                    }

                
                splitidx = first;
            }
            
            
            if (splitidx == req.startidx || splitidx == req.startidx + req.numprims)
            {
                splitidx = req.startidx + (req.numprims >> 1);
                
                for (int i = req.startidx; i < splitidx; ++i)
                {
                    leftbounds.grow(primrefs[i].bounds);
                    leftcentroid_bounds.grow(primrefs[i].center);
                }
                
                for (int i = splitidx; i < req.startidx + req.numprims; ++i)
                {
                    rightbounds.grow(primrefs[i].bounds);
                    rightcentroid_bounds.grow(primrefs[i].center);
                }
            }
            
            // Left request
            SplitRequest leftrequest = { req.startidx, splitidx - req.startidx, &node->lc, leftbounds, leftcentroid_bounds, req.level + 1 };
            // Right request
            SplitRequest rightrequest = { splitidx, req.numprims - (splitidx - req.startidx), &node->rc, rightbounds, rightcentroid_bounds, req.level + 1 };
            
            
            // The order is very important here since right node uses the space at the end of the array to partition
            {
                BuildNode(rightrequest, primrefs);
            }
            
            {
                // Put those to stack
                BuildNode(leftrequest, primrefs);
            }
        }
        
        // Set parent ptr if any
        if (req.ptr) *req.ptr = node;
    }
    
    SplitBvh::SahSplit SplitBvh::FindObjectSahSplit(SplitRequest const& req, PrimRefArray const& refs) const
    {
        // SAH implementation
        // calc centroids histogram
        int const kNumBins = 64;
        // moving split bin index
        int splitidx = -1;
        // Set SAH to maximum float value as a start
        auto sah = std::numeric_limits<float>::max();
        SahSplit split;
        split.dim = 0;
        split.split = std::numeric_limits<float>::quiet_NaN();
        split.sah = sah;
        
        // if we cannot apply histogram algorithm
        // put NAN sentinel as split border
        // PerformObjectSplit simply splits in half
        // in this case
        float3 centroid_extents = req.centroid_bounds.extents();
        if (centroid_extents.sqnorm() == 0.f)
        {
            return split;
        }
        
        // Bin has bbox and occurence count
        struct Bin
        {
            bbox bounds;
            int count;
        };
        
        // Keep bins for each dimension
        Bin   bins[3][kNumBins];
        // Precompute inverse parent area
        auto invarea = 1.f / req.bounds.surface_area();
        // Precompute min point
        auto rootmin = req.centroid_bounds.pmin;
        
        // Evaluate all dimensions
        for (int axis = 0; axis < 3; ++axis)
        {
            float rootminc = rootmin[axis];
            // Range for histogram
            auto centroid_rng = centroid_extents[axis];
            auto invcentroid_rng = 1.f / centroid_rng;
            
            // If the box is degenerate in that dimension skip it
            if (centroid_rng == 0.f) continue;
            
            // Initialize bins
            for (unsigned i = 0; i < kNumBins; ++i)
            {
                bins[axis][i].count = 0;
                bins[axis][i].bounds = bbox();
            }
            
            // Calc primitive refs histogram
            for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
            {
                auto idx = i;
                auto binidx = (int)std::min<float>(kNumBins * ((refs[idx].center[axis] - rootminc) * invcentroid_rng), kNumBins - 1);
                
                ++bins[axis][binidx].count;
                bins[axis][binidx].bounds.grow(refs[idx].bounds);
            }
            
            bbox rightbounds[kNumBins - 1];
            
            // Start with 1-bin right box
            bbox rightbox = bbox();
            for (int i = kNumBins - 1; i > 0; --i)
            {
                rightbox.grow(bins[axis][i].bounds);
                rightbounds[i - 1] = rightbox;
            }
            
            bbox leftbox = bbox();
            int  leftcount = 0;
            int  rightcount = req.numprims;
            
            // Start best SAH search
            // i is current split candidate (split between i and i + 1)
            float sahtmp = 0.f;
            for (int i = 0; i < kNumBins - 1; ++i)
            {
                leftbox.grow(bins[axis][i].bounds);
                leftcount += bins[axis][i].count;
                rightcount -= bins[axis][i].count;
                
                // Compute SAH
                sahtmp = 10.f + (leftcount * leftbox.surface_area() + rightcount * rightbounds[i].surface_area()) * invarea;
                
                // Check if it is better than what we found so far
                if (sahtmp < sah)
                {
                    split.dim = axis;
                    splitidx = i;
                    sah = sahtmp;
                }
            }
        }
        
        // Choose split plane
        if (splitidx != -1)
        {
            split.split = rootmin[split.dim] + (splitidx + 1) * (centroid_extents[split.dim] / kNumBins);
            split.sah = sah;
        }
        
        return split;
    }
    
    SplitBvh::SahSplit SplitBvh::FindSpatialSahSplit(SplitRequest const& req, PrimRefArray const& refs) const
    {
        // SAH implementation
        // calc centroids histogram
        int const kNumBins = 128;
        // Set SAH to maximum float value as a start
        auto sah = std::numeric_limits<float>::max();
        SahSplit split;
        split.dim = 0;
        split.split = std::numeric_limits<float>::quiet_NaN();
        split.sah = sah;
        

        // Extents
        float3 extents = req.bounds.extents();
        auto invarea = 1.f / req.bounds.surface_area();
        
        // If there are too few primitives don't split them
        if (extents.sqnorm() == 0.f)
        {
            return split;
        }
        
        // Bin has start and exit counts + bounds
        struct Bin
        {
            bbox bounds;
            int enter;
            int exit;
        };
        
        Bin bins[3][kNumBins];
        
        // Prepcompute some useful stuff
        float3 origin  = req.bounds.pmin;
        float3 binsize = req.bounds.extents() * (1.f / kNumBins);
        float3 invbinsize = float3(1.f / binsize.x, 1.f / binsize.y, 1.f / binsize.z);
        
        // Initialize bins
        for (int axis = 0; axis < 3; ++axis)
        {
            for (int i = 0; i < kNumBins; ++i)
            {
                bins[axis][i].bounds = bbox();
                bins[axis][i].enter = 0;
                bins[axis][i].exit = 0;
            }
        }
        
        // Iterate thru all primitive refs
        for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
        {
            PrimRef const& primref(refs[i]);
            // Determine starting bin for this primitive
            float3 firstbin = clamp3((primref.bounds.pmin - origin) * invbinsize, float3(0,0,0), float3(kNumBins-1, kNumBins-1, kNumBins-1));
            // Determine finishing bin
            float3 lastbin = clamp3((primref.bounds.pmax - origin) * invbinsize, firstbin, float3(kNumBins-1, kNumBins-1, kNumBins-1));
            // Iterate over axis
            for (int axis = 0; axis < 3; ++axis)
            {
                // Skip in case of a degenerate dimension
                if (extents[axis] == 0.f) continue;
                // Break the prim into bins
                auto tempref = primref;
                
                for (int j = (int)firstbin[axis]; j < (int)lastbin[axis]; ++j)
                {
                    PrimRef leftref, rightref;
                    // Split primitive ref into left and right
                    float splitval = origin[axis] + binsize[axis] * (j + 1);
                    if (SplitPrimRef(tempref, axis, splitval, leftref, rightref))
                    {
                        // Add left one
                        bins[axis][j].bounds.grow(leftref.bounds);
                        // Save right to add part of it into the next bin
                        tempref = rightref;
                    }
                }
                // Add the last piece into the last bin
                bins[axis][(int)lastbin[axis]].bounds.grow(tempref.bounds);
                // Adjust enter & exit counters
                bins[axis][(int)firstbin[axis]].enter++;
                bins[axis][(int)lastbin[axis]].exit++;
            }
        }
        
        // Prepare moving window data
        bbox rightbounds[kNumBins - 1];
        split.sah = std::numeric_limits<float>::max();
        
        // Iterate over axis
        for (int axis = 0; axis < 3; ++axis)
        {
            // Skip if the extent is degenerate in that direction
            if (extents[axis] == 0.f)
                continue;
            
            // Start with 1-bin right box
            bbox rightbox = bbox();
            for (int i = kNumBins - 1; i > 0; --i)
            {
                rightbox = bboxunion(rightbox, bins[axis][i].bounds);
                rightbounds[i-1] = rightbox;
            }
            
            bbox leftbox = bbox();
            int  leftcount = 0;
            int  rightcount = req.numprims;
            
            // Start moving border to the right
            for (int i = 1; i < kNumBins; ++i)
            {
                // New left box
                leftbox.grow(bins[axis][i-1].bounds);
                // New left box count
                leftcount += bins[axis][i-1].enter;
                // Adjust right box
                rightcount -= bins[axis][i-1].exit;
                // Calc SAH
                float sah = 10.f + (leftbox.surface_area() *
                                          + rightbounds[i-1].surface_area() * rightcount)  * invarea;
                
                // Update SAH if it is needed
                if (sah < split.sah)
                {
                    split.sah = sah;
                    split.dim = axis;
                    split.split = origin[axis] + binsize[axis] * (float)i;
                }
            }
        }
        
        return split;
    }
    
    bool SplitBvh::SplitPrimRef(PrimRef const& ref, int axis, float split, PrimRef& leftref, PrimRef& rightref) const
    {
        // Start with left and right refs equal to original ref
        leftref.idx = rightref.idx = ref.idx;
        leftref.bounds = rightref.bounds = ref.bounds;
        
        // Only split if split value is within our bounds range
        if (split > ref.bounds.pmin[axis] && split < ref.bounds.pmax[axis])
        {
            // Trim left box on the right
            leftref.bounds.pmax[axis] = split;
            // Trim right box on the left
            rightref.bounds.pmin[axis] = split;
            return true;
        }
        
        return false;
    }
    
    void SplitBvh::SplitPrimRefs(SahSplit const& split, SplitRequest const& req, PrimRefArray& refs, int& extra_refs)
    {
        // We are going to append new primitives at the end of the array
        int appendprims = req.numprims;
        
        // Split refs if any of them require to be split
        for (int i = req.startidx; i < req.startidx + req.numprims; ++i)
        {
            assert(req.startidx + appendprims < refs.size());
            
            PrimRef leftref, rightref;
            if (SplitPrimRef(refs[i], split.dim, split.split, leftref, rightref))
            {
                // Copy left ref instead of original
                refs[i] = leftref;
                // Append right one at the end
                refs[req.startidx + appendprims++] = rightref;
            }
        }
    
        // Return number of primitives after this operation
        extra_refs = appendprims - req.numprims;
    }
}
