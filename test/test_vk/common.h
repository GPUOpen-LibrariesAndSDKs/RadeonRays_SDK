#pragma once
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <memory>
#include <numeric>
#include <stack>

template <typename T>
using VkScopedObject = std::shared_ptr<std::remove_pointer_t<T>>;
namespace bvh_utils
{
class float3
{
public:
    float3(float xx = 0.f, float yy = 0.f, float zz = 0.f, float ww = 0.f) : x(xx), y(yy), z(zz), w(ww) {}

    float& operator[](int i) { return *(&x + i); }
    float  operator[](int i) const { return *(&x + i); }
    float3 operator-() const { return float3(-x, -y, -z); }

    float sqnorm() const { return x * x + y * y + z * z; }
    void  normalize() { (*this) /= (std::sqrt(sqnorm())); }

    float3& operator+=(float3 const& o)
    {
        x += o.x;
        y += o.y;
        z += o.z;
        return *this;
    }
    float3& operator-=(float3 const& o)
    {
        x -= o.x;
        y -= o.y;
        z -= o.z;
        return *this;
    }
    float3& operator*=(float3 const& o)
    {
        x *= o.x;
        y *= o.y;
        z *= o.z;
        return *this;
    }
    float3& operator*=(float c)
    {
        x *= c;
        y *= c;
        z *= c;
        return *this;
    }
    float3& operator/=(float c)
    {
        const float cinv = 1.f / c;
        x *= cinv;
        y *= cinv;
        z *= cinv;
        return *this;
    }

    float x, y, z, w;
};

typedef float3 float4;

inline float3 operator+(float3 const& v1, float3 const& v2)
{
    float3 res = v1;
    return res += v2;
}

inline float3 operator-(float3 const& v1, float3 const& v2)
{
    float3 res = v1;
    return res -= v2;
}

inline float3 operator*(float3 const& v1, float3 const& v2)
{
    float3 res = v1;
    return res *= v2;
}

inline float3 operator*(float3 const& v1, float c)
{
    float3 res = v1;
    return res *= c;
}

inline float3 operator*(float c, float3 const& v1) { return operator*(v1, c); }

inline float dot(float3 const& v1, float3 const& v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

inline float3 normalize(float3 const& v)
{
    float3 res = v;
    res.normalize();
    return res;
}

inline float3 cross(float3 const& v1, float3 const& v2)
{
    /// |i     j     k|
    /// |v1x  v1y  v1z|
    /// |v2x  v2y  v2z|
    return float3(v1.y * v2.z - v2.y * v1.z, v2.x * v1.z - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x);
}

inline float3 vmin(float3 const& v1, float3 const& v2)
{
    return float3(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z));
}

inline void vmin(float3 const& v1, float3 const& v2, float3& v)
{
    v.x = std::min(v1.x, v2.x);
    v.y = std::min(v1.y, v2.y);
    v.z = std::min(v1.z, v2.z);
}

inline float3 fma(float3 const& v1, float3 const& v2, float3 const& v3) { return v1 * v2 + v3; }

inline float3 vmax(float3 const& v1, float3 const& v2)
{
    return float3(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z));
}

inline void vmax(float3 const& v1, float3 const& v2, float3& v)
{
    v.x = std::max(v1.x, v2.x);
    v.y = std::max(v1.y, v2.y);
    v.z = std::max(v1.z, v2.z);
}

// Safe RCP
inline float3 rcp(const float3& d)
{
    float dirx  = d.x;
    float diry  = d.y;
    float dirz  = d.z;
    float ooeps = 1e-5f;

    float3 invd;
    invd.x = 1.f / (abs(dirx) > ooeps ? dirx : copysign(ooeps, dirx));
    invd.y = 1.f / (abs(diry) > ooeps ? diry : copysign(ooeps, diry));
    invd.z = 1.f / (abs(dirz) > ooeps ? dirz : copysign(ooeps, dirz));
    return invd;
}

struct Aabb
{
    //<! Create AABB from a single point.
    Aabb() : pmin{FLT_MAX, FLT_MAX, FLT_MAX}, pmax{-FLT_MAX, -FLT_MAX, -FLT_MAX} {}
    //<! Create AABB from a single point.
    Aabb(const float3& p) : pmin(p), pmax(p) {}
    //<! Create AABB from min and max points.
    Aabb(const float3& mi, const float3& ma) : pmin(mi), pmax(ma) {}
    //<! Create AABB from another AABB.
    Aabb(const Aabb& rhs) : pmin(rhs.pmin), pmax(rhs.pmax) {}
    //<! Grow AABB to enclose itself and another AABB.
    Aabb& Grow(const Aabb& rhs)
    {
        pmin = vmin(pmin, rhs.pmin);
        pmax = vmax(pmax, rhs.pmax);
        return *this;
    }
    //<! Grow AABB to enclose itself and another point.
    Aabb& Grow(const float3& p)
    {
        pmin = vmin(pmin, p);
        pmax = vmax(pmax, p);
        return *this;
    }
    //<! Box center.
    float3 Center() const { return (pmax + pmin) * 0.5; }

    //<! Box extens along each axe.
    float3 Extents() const { return pmax - pmin; }
    //<! Calculate AABB union.
    static Aabb Union(const Aabb& rhs, const Aabb& lhs)
    {
        Aabb result(vmin(lhs.pmin, rhs.pmin), vmax(lhs.pmax, rhs.pmax));
        return result;
    }

    //<! Box extens along each axe.
    float Area() const
    {
        float3 ext = Extents();
        return 2 * (ext.x * ext.y + ext.x * ext.z + ext.y * ext.z);
    }

    bool Includes(Aabb const& rhs) const
    {
        float3 min_diff = rhs.pmin - pmin;
        float3 max_diff = pmax - rhs.pmax;
        return !(min_diff.x < -1e-8f || min_diff.y < -1e-8f || min_diff.z < -1e-8f || max_diff.x < -1e-8f ||
                 max_diff.y < -1e-8f || max_diff.z < -1e-8f);
    }

    //<! Min point of AABB.
    float3 pmin;
    //<! Max point of AABB.
    float3 pmax;
};

constexpr uint32_t kInvalidID = ~0u;

struct VkBvhNode
{
    float    aabb0_min_or_v0[3];
    uint32_t child0;
    float    aabb0_max_or_v1[3];
    uint32_t child1;
    float    aabb1_min_or_v2[3];
    uint32_t parent;
    float    aabb1_max_or_v3[3];
    uint32_t update;
};

inline Aabb GetAabb(VkBvhNode const& node)
{
    Aabb aabb;
    if (node.child0 != kInvalidID)
    {
        Aabb left{float3{node.aabb0_min_or_v0[0], node.aabb0_min_or_v0[1], node.aabb0_min_or_v0[2]},
                  float3{node.aabb0_max_or_v1[0], node.aabb0_max_or_v1[1], node.aabb0_max_or_v1[2]}};
        Aabb right{float3{node.aabb1_min_or_v2[0], node.aabb1_min_or_v2[1], node.aabb1_min_or_v2[2]},
                   float3{node.aabb1_max_or_v3[0], node.aabb1_max_or_v3[1], node.aabb1_max_or_v3[2]}};
        aabb = left.Grow(right);
    } else
    {
        aabb.Grow(float3{node.aabb0_min_or_v0[0], node.aabb0_min_or_v0[1], node.aabb0_min_or_v0[2]})
            .Grow(float3{node.aabb0_max_or_v1[0], node.aabb0_max_or_v1[1], node.aabb0_max_or_v1[2]})
            .Grow(float3{node.aabb1_min_or_v2[0], node.aabb1_min_or_v2[1], node.aabb1_min_or_v2[2]});
    }
    return aabb;
}

inline float CalculateSAH(VkBvhNode* nodes)
{
    std::stack<uint32_t> traversal_stack;
    traversal_stack.push(0u);

    auto root_area = GetAabb(nodes[0u]).Area();
    float total_sah = 0.f;

    while (!traversal_stack.empty())
    {
        auto node_index = traversal_stack.top();
        traversal_stack.pop();

        auto const& node          = nodes[node_index];
        float       relative_area = GetAabb(node).Area() / root_area;
        total_sah += relative_area;

        if (node.child0 != kInvalidID)
        {
            traversal_stack.push(node.child0);
            traversal_stack.push(node.child1);
        }
    }

    return total_sah;
}
}  // namespace bvh_utils