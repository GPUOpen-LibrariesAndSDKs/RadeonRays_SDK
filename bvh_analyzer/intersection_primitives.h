/**********************************************************************
Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.

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
#include "common.h"
#include <ostream>

namespace bvh
{
enum class QueryType
{
    kClosestHit,
    kAnyHit
};

struct Ray
{
    float origin[3];
    float min_t;
    float direction[3];
    float max_t;
};

struct Hit
{
    float    uv[2];
    uint32_t inst_id = kInvalidID;
    uint32_t prim_id = kInvalidID;
};

struct QualityStats
{
    bool  is_valid                     = false;
    float sah_estimation               = 0.f;
    float avg_primary_node_tests       = 0.f;
    float avg_primary_aabb_tests       = 0.f;
    float avg_primary_triangle_tests   = 0.f;
    float avg_secondary_node_tests     = 0.f;
    float avg_secondary_aabb_tests     = 0.f;
    float avg_secondary_triangle_tests = 0.f;
};

inline std::ostream& operator<<(std::ostream& oss, const QualityStats& stats)
{
    oss << "is_valid: " << stats.is_valid << std::endl;
    oss << "sah: " << stats.sah_estimation << std::endl;
    oss << "avg_primary_node_tests: " << stats.avg_primary_node_tests << std::endl;
    oss << "avg_primary_aabb_tests: " << stats.avg_primary_aabb_tests << std::endl;
    oss << "avg_primary_triangle_tests: " << stats.avg_primary_triangle_tests << std::endl;
    return oss;
}

struct TraversalStats
{
    float num_aabb_tests          = 0.f;
    float num_triangle_tests      = 0.f;
    float num_internal_node_tests = 0.f;
    float num_leaf_node_tests     = 0.f;

    void Reset()
    {
        num_aabb_tests          = 0.f;
        num_triangle_tests      = 0.f;
        num_internal_node_tests = 0.f;
        num_leaf_node_tests     = 0.f;
    }

    void Add(const TraversalStats& s)
    {
        num_aabb_tests += s.num_aabb_tests;
        num_triangle_tests += s.num_triangle_tests;
        num_internal_node_tests += s.num_internal_node_tests;
        num_leaf_node_tests += s.num_leaf_node_tests;
    }

    void Max(const TraversalStats& s)
    {
        num_aabb_tests          = std::max(num_aabb_tests, s.num_aabb_tests);
        num_triangle_tests      = std::max(num_triangle_tests, s.num_triangle_tests);
        num_internal_node_tests = std::max(num_internal_node_tests, s.num_internal_node_tests);
        num_leaf_node_tests     = std::max(num_leaf_node_tests, s.num_leaf_node_tests);
    }
};

}  // namespace bvh