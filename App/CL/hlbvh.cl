#pragma once

#include <../App/CL/radiance_probe.cl>

typedef struct _bbox
{
    float3 pmin;
    float3 pmax;
} bbox;

typedef struct
{
    int parent;
    int left;
    int right;
    int next;
} HlbvhNode;


#define STARTIDX(x)     (((int)((x).left)))
#define LEAFNODE(x)     (((x).left) == ((x).right))
#define STACK_SIZE 48

inline 
float3 Bbox_GetCenter(bbox box)
{
    return 0.5f * (box.pmax + box.pmin);
}

inline 
bool Bbox_ContainsPoint(bbox box, float3 p)
{
    return p.x >= box.pmin.x && p.x <= box.pmax.x &&
        p.y >= box.pmin.y && p.y <= box.pmax.y &&
        p.z >= box.pmin.z && p.z <= box.pmax.z;
}

inline 
int Hlbvh_GetClosestItemIndex(__global HlbvhNode const* restrict bvh, __global bbox const* restrict bounds, float3 p)
{
    int stack[STACK_SIZE];
    int* ptr = stack;
    *ptr++ = -1;
    int idx = 0;

    HlbvhNode node;
    bbox lbox;
    bbox rbox;
    float min_dist = 1000000.f;
    int min_idx = -1;

    while (idx > -1)
    {
        node = bvh[idx];

        if (LEAFNODE(node))
        {
            float3 center = Bbox_GetCenter(bounds[idx]);

            float dist = length(center - p);

            if (dist < min_dist)
            {
                min_dist = dist;
                min_idx = idx;
            }
        }
        else
        {
            lbox = bounds[node.left];
            rbox = bounds[node.right];

            bool lhit = Bbox_ContainsPoint(lbox, p);
            bool rhit = Bbox_ContainsPoint(rbox, p);

            if (lhit && rhit)
            {

                idx = node.left;
                *ptr++ = node.right;
                continue;
            }
            else if (lhit)
            {
                idx = node.left;
                continue;
            }
            else if (rhit)
            {
                idx = node.right;
                continue;
            }
        }

        idx = *--ptr;
    }

    return STARTIDX(bvh[min_idx]);
}
