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
/**
    \file build_bvh.cl
    \author Dmitry Kozlov
    \version 1.0
    \brief HLBVH build implementation

    IntersectorHlbvh implementation is based on the following paper:
    "HLBVH: Hierarchical LBVH Construction for Real-Time Ray Tracing"
    Jacopo Pantaleoni (NVIDIA), David Luebke (NVIDIA), in High Performance Graphics 2010, June 2010
    https://research.nvidia.com/sites/default/files/publications/HLBVH-final.pdf

    Pros:
        -Very fast to build and update.
    Cons:
        -Poor BVH quality, slow traversal.
 */
/*************************************************************************
INCLUDES
**************************************************************************/
#include <../RadeonRays/src/kernels/CL/common.cl>

/*************************************************************************
DEFINES
**************************************************************************/
#define LEAFIDX(i) ((num_prims-1) + i)
#define NODEIDX(i) (i)
// Shortcut for delta evaluation
#define DELTA(i,j) delta(morton_codes,num_prims,i,j)

/*************************************************************************
TYPE DEFINITIONS
**************************************************************************/
typedef struct
{
    int parent;
    int left;
    int right;
    int next;
} HlbvhNode;

/*************************************************************************
FUNCTIONS
**************************************************************************/
// The following two functions are from
// http://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
// Expands a 10-bit integer into 30 bits
// by inserting 2 zeros after each bit.
INLINE uint expand_bits(uint v)
{
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

// Calculates a 30-bit Morton code for the
// given 3D point located within the unit cube [0,1].
INLINE uint calculate_morton_code(float3 p)
{
    float x = min(max(p.x * 1024.0f, 0.0f), 1023.0f);
    float y = min(max(p.y * 1024.0f, 0.0f), 1023.0f);
    float z = min(max(p.z * 1024.0f, 0.0f), 1023.0f);
    unsigned int xx = expand_bits((uint)x);
    unsigned int yy = expand_bits((uint)y);
    unsigned int zz = expand_bits((uint)z);
    return xx * 4 + yy * 2 + zz;
}

// Make a union of two bboxes
INLINE bbox bbox_union(bbox b1, bbox b2)
{
    bbox res;
    res.pmin = min(b1.pmin, b2.pmin);
    res.pmax = max(b1.pmax, b2.pmax);
    return res;
}

// Assign Morton codes to each of positions
KERNEL void calculate_morton_code_main(
    // Centers of primitive bounding boxes
    GLOBAL bbox const* restrict primitive_bounds,
    // Number of primitives
    int num_primitive_bounds,
    // Scene extents
    GLOBAL bbox const* restrict scene_bound, 
    // Morton codes
    GLOBAL int* morton_codes
    )
{
    int global_id = get_global_id(0);

    if (global_id < num_primitive_bounds)
    {
        // Fetch primitive bound
        bbox bound = primitive_bounds[global_id];
        // Calculate center and scene extents
        float3 const center = (bound.pmax + bound.pmin).xyz * 0.5f;
        float3 const scene_min = scene_bound->pmin.xyz;
        float3 const scene_extents = scene_bound->pmax.xyz - scene_bound->pmin.xyz;
        // Calculate morton code
        morton_codes[global_id] = calculate_morton_code((center - scene_min) / scene_extents);
    }
}



// Calculates longest common prefix length of bit representations
// if  representations are equal we consider sucessive indices
INLINE int delta(GLOBAL int const* morton_codes, int num_prims, int i1, int i2)
{
    // Select left end
    int left = min(i1, i2);
    // Select right end
    int right = max(i1, i2);
    // This is to ensure the node breaks if the index is out of bounds
    if (left < 0 || right >= num_prims) 
    {
        return -1;
    }
    // Fetch Morton codes for both ends
    int left_code = morton_codes[left];
    int right_code = morton_codes[right];

    // Special handling of duplicated codes: use their indices as a fallback
    return left_code != right_code ? clz(left_code ^ right_code) : (32 + clz(left ^ right));
}

// Find span occupied by internal node with index idx
INLINE int2 find_span(GLOBAL int const* restrict morton_codes, int num_prims, int idx)
{
    // Find the direction of the range
    int d = sign((float)(DELTA(idx, idx+1) - DELTA(idx, idx-1)));

    // Find minimum number of bits for the break on the other side
    int delta_min = DELTA(idx, idx-d);

    // Search conservative far end
    int lmax = 2;
    while (DELTA(idx,idx + lmax * d) > delta_min)
        lmax *= 2;

    // Search back to find exact bound
    // with binary search
    int l = 0;
    int t = lmax;
    do
    {
        t /= 2;
        if(DELTA(idx, idx + (l + t)*d) > delta_min)
        {
            l = l + t;
        }
    }
    while (t > 1);

    // Pack span 
    int2 span;
    span.x = min(idx, idx + l*d);
    span.y = max(idx, idx + l*d);
    return span;
}

// Find split idx within the span
INLINE int find_split(GLOBAL int const* restrict morton_codes, int num_prims, int2 span)
{
    // Fetch codes for both ends
    int left = span.x;
    int right = span.y;

    // Calculate the number of identical bits from higher end
    int num_identical = DELTA(left, right);

    do
    {
        // Proposed split
        int new_split = (right + left) / 2;

        // If it has more equal leading bits than left and right accept it
        if (DELTA(left, new_split) > num_identical)
        {
            left = new_split;
        }
        else
        {
            right = new_split;
        }
    }
    while (right > left + 1);

    return left;
}

// Set parent-child relationship
KERNEL void emit_hierarchy_main(
    // Sorted Morton codes of the primitives
    GLOBAL int const* restrict morton_codes,
    // Bounds
    GLOBAL bbox const* restrict bounds,
    // Primitive indices
    GLOBAL int const* restrict indices,
    // Number of primitives
    int num_prims,
    // Nodes
    GLOBAL HlbvhNode* nodes,
    // Leaf bounds
    GLOBAL bbox* bounds_sorted
    )
{
    int global_id = get_global_id(0);

    // Set child
    if (global_id < num_prims)
    {
        nodes[LEAFIDX(global_id)].left = nodes[LEAFIDX(global_id)].right = indices[global_id];
        bounds_sorted[LEAFIDX(global_id)] = bounds[indices[global_id]];
    }
    
    // Set internal nodes
    if (global_id < num_prims - 1)
    {
        // Find span occupied by the current node
        int2 range = find_span(morton_codes, num_prims, global_id);

        // Find split position inside the range
        int  split = find_split(morton_codes, num_prims, range);

        // Create child nodes if needed
        int c1idx = (split == range.x) ? LEAFIDX(split) : NODEIDX(split);
        int c2idx = (split + 1 == range.y) ? LEAFIDX(split + 1) : NODEIDX(split + 1);

        nodes[NODEIDX(global_id)].left = c1idx;
        nodes[NODEIDX(global_id)].right = c2idx;
        //nodes[NODEIDX(global_id)].next = (range.y + 1 < num_prims) ? range.y + 1 : -1;
        nodes[c1idx].parent = NODEIDX(global_id);
        //nodes[c1idx].next = c2idx;
        nodes[c2idx].parent = NODEIDX(global_id);
        //nodes[c2idx].next = nodes[NODEIDX(global_id)].next;
    }
}

// Propagate bounds up to the root
KERNEL void refit_bounds_main(
    // Node bounds
    GLOBAL bbox* bounds,
    // Number of nodes
    int num_prims,
    // Nodes
    GLOBAL HlbvhNode* nodes,
    // Atomic flags
    GLOBAL int* flags
)
{
    int global_id = get_global_id(0);

    // Start from leaf nodes
    if (global_id < num_prims)
    {
        // Get my leaf index
        int idx = LEAFIDX(global_id);

        do
        {
            // Move to parent node
            idx = nodes[idx].parent;

            // Check node's flag
            if (atomic_cmpxchg(flags + idx, 0, 1) == 1)
            {
                // If the flag was 1 the second child is ready and 
                // this thread calculates bbox for the node

                // Fetch kids
                int lc = nodes[idx].left;
                int rc = nodes[idx].right;

                // Calculate bounds
                bbox b = bbox_union(bounds[lc], bounds[rc]);

                // Write bounds
                bounds[idx] = b;
            }
            else
            {
                // If the flag was 0 set it to 1 and bail out.
                // The thread handling the second child will
                // handle this node.
                break;
            }
        }
        while (idx != 0);
    }
}