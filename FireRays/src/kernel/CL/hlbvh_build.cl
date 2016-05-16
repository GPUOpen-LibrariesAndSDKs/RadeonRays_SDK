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

typedef struct
{
	float3 pmin;
	float3 pmax;
} bbox;

// The following two functions are from
// http://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
// Expands a 10-bit integer into 30 bits
// by inserting 2 zeros after each bit.
static unsigned int ExpandBits(unsigned int v)
{
    v = (v * 0x00010001u) & 0xFF0000FFu;
    v = (v * 0x00000101u) & 0x0F00F00Fu;
    v = (v * 0x00000011u) & 0xC30C30C3u;
    v = (v * 0x00000005u) & 0x49249249u;
    return v;
}

// Calculates a 30-bit Morton code for the
// given 3D point located within the unit cube [0,1].
unsigned int CalculateMortonCode(float3 p)
{
    float x = min(max(p.x * 1024.0f, 0.0f), 1023.0f);
    float y = min(max(p.y * 1024.0f, 0.0f), 1023.0f);
    float z = min(max(p.z * 1024.0f, 0.0f), 1023.0f);
    unsigned int xx = ExpandBits((unsigned int)x);
    unsigned int yy = ExpandBits((unsigned int)y);
    unsigned int zz = ExpandBits((unsigned int)z);
    return xx * 4 + yy * 2 + zz;
}

// Assign Morton codes to each of positions
__kernel void CalcMortonCode(
    // Centers of primitive bounding boxes
    __global bbox const* bounds,
    // Number of primitives
    int numpositions,
    // Morton codes
    __global int* mortoncodes
    )
{
    int globalid = get_global_id(0);

    if (globalid < numpositions)
    {
		bbox bound = bounds[globalid];
		float3 center = 0.5f * (bound.pmax + bound.pmin);
        mortoncodes[globalid] = CalculateMortonCode(center);
    }
}


bbox bboxunion(bbox b1, bbox b2)
{
    bbox res;
    res.pmin = min(b1.pmin, b2.pmin);
    res.pmax = max(b1.pmax, b2.pmax);
    return res;
}

typedef struct
{
        int parent;
        int left;
        int right;
        int next;
} HlbvhNode;

#define LEAFIDX(i) ((numprims-1) + i)
#define NODEIDX(i) (i)

// Calculates longest common prefix length of bit representations
// if  representations are equal we consider sucessive indices
int delta(__global int* mortoncodes, int numprims, int i1, int i2)
{
    // Select left end
    int left = min(i1, i2);
    // Select right end
    int right = max(i1, i2);
    // This is to ensure the node breaks if the index is out of bounds
    if (left < 0 || right >= numprims) 
    {
        return -1;
    }
    // Fetch Morton codes for both ends
    int leftcode = mortoncodes[left];
    int rightcode = mortoncodes[right];

    // Special handling of duplicated codes: use their indices as a fallback
    return leftcode != rightcode ? clz(leftcode ^ rightcode) : (32 + clz(left ^ right));
}

// Shortcut for delta evaluation
#define DELTA(i,j) delta(mortoncodes,numprims,i,j)

// Find span occupied by internal node with index idx
int2 FindSpan(__global int* mortoncodes, int numprims, int idx)
{
    // Find the direction of the range
    int d = sign((float)(DELTA(idx, idx+1) - DELTA(idx, idx-1)));

    // Find minimum number of bits for the break on the other side
    int deltamin = DELTA(idx, idx-d);

    // Search conservative far end
    int lmax = 2;
    while (DELTA(idx,idx + lmax * d) > deltamin)
        lmax *= 2;

    // Search back to find exact bound
    // with binary search
    int l = 0;
    int t = lmax;
    do
    {
        t /= 2;
        if(DELTA(idx, idx + (l + t)*d) > deltamin)
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
int FindSplit(__global int* mortoncodes, int numprims, int2 span)
{
    // Fetch codes for both ends
    int left = span.x;
    int right = span.y;

    // Calculate the number of identical bits from higher end
    int numidentical = DELTA(left, right);

    do
    {
        // Proposed split
        int newsplit = (right + left) / 2;

        // If it has more equal leading bits than left and right accept it
        if (DELTA(left, newsplit) > numidentical)
        {
            left = newsplit;
        }
        else
        {
            right = newsplit;
        }
    }
    while (right > left + 1);

    return left;
}

// Set parent-child relationship
__kernel void BuildHierarchy(
    // Sorted Morton codes of the primitives
    __global int* mortoncodes,
    // Bounds
    __global bbox* bounds,
    // Primitive indices
    __global int* indices,
    // Number of primitives
    int numprims,
    // Nodes
    __global HlbvhNode* nodes,
    // Leaf bounds
    __global bbox* boundssorted
    )
{
    int globalid = get_global_id(0);

    // Set child
    if (globalid < numprims)
    {
        nodes[LEAFIDX(globalid)].left = nodes[LEAFIDX(globalid)].right = indices[globalid];
        boundssorted[LEAFIDX(globalid)] = bounds[indices[globalid]];
    }
    
    // Set internal nodes
    if (globalid < numprims - 1)
    {
        // Find span occupied by the current node
        int2 range = FindSpan(mortoncodes, numprims, globalid);

        // Find split position inside the range
        int  split = FindSplit(mortoncodes, numprims, range);

        // Create child nodes if needed
        int c1idx = (split == range.x) ? LEAFIDX(split) : NODEIDX(split);
        int c2idx = (split + 1 == range.y) ? LEAFIDX(split + 1) : NODEIDX(split + 1);

        nodes[NODEIDX(globalid)].left = c1idx;
        nodes[NODEIDX(globalid)].right = c2idx;
        //nodes[NODEIDX(globalid)].next = (range.y + 1 < numprims) ? range.y + 1 : -1;
        nodes[c1idx].parent = NODEIDX(globalid);
        //nodes[c1idx].next = c2idx;
        nodes[c2idx].parent = NODEIDX(globalid);
        //nodes[c2idx].next = nodes[NODEIDX(globalid)].next;
    }
}

// Propagate bounds up to the root
__kernel void RefitBounds(__global bbox* bounds,
                          int numprims,
                          __global HlbvhNode* nodes,
                          __global int* flags
                          )
{
    int globalid = get_global_id(0);

    // Start from leaf nodes
    if (globalid < numprims)
    {
        // Get my leaf index
        int idx = LEAFIDX(globalid);

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
                bbox b = bboxunion(bounds[lc], bounds[rc]);

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