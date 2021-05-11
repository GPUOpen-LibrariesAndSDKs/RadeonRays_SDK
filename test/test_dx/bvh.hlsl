#define INVALID_NODE 0xFFFFFFFFu
#define IS_INTERNAL_NODE(i, c) ((i < (c - 1)) ? 1 : 0)
#define IS_LEAF_NODE(i, c) (!IS_INTERNAL_NODE(i, c))

struct BvhNode
{
    uint left_child;
    uint right_child;
    uint unused0;
    uint unused1;

    float3 left_aabb_min_or_v0;
    float3 left_aabb_max_or_v1;
    float3 right_aabb_min_or_v2;
    float3 right_aabb_max_or_v3;
};

bool fast_intersect_triangle(in float3 ray_origin,
                             in float3 ray_direction,
                             in float3 v1,
                             in float3 v2,
                             in float3 v3,
                             in float tmin,
                             inout float closest_t,
                             inout float2 barycentric)
{
    float3 e1;
    float3 e2;

    // Determine edge vectors for clockwise triangle vertices
    e1 = v2 - v1;
    e2 = v3 - v1;

    const float3 s1 = cross(ray_direction, e2);
    const float determinant = dot(s1, e1);
    const float invd = rcp(determinant);

    const float3 d = ray_origin - v1;
    const float u = dot(d, s1) * invd;

    // Barycentric coordinate U is outside range
    if ((u < 0.f) || (u > 1.f))
    {
        return false;
    }

    const float3 s2 = cross(d, e1);
    const float v = dot(ray_direction, s2) * invd;

    // Barycentric coordinate V is outside range
    if ((v < 0.f) || (u + v > 1.f))
    {
        return false;
    }

    // Check parametric distance
    const float t = dot(e2, s2) * invd;
    if (t < tmin || t > closest_t)
    {
        return false;
    }

    // Accept hit
    closest_t = t;
    barycentric = float2(u, v);

    return true;
}

float min3(float3 val) { return min(min(val.x, val.y), val.z); }

float max3(float3 val) { return max(max(val.x, val.y), val.z); }

float2 fast_intersect_bbox(float3 ray_origin, float3 ray_inv_dir, float3 box_min, float3 box_max, float t_min, float t_max)
{
    float3 oxinvdir = -ray_origin * ray_inv_dir;
    float3 f = box_max * ray_inv_dir + oxinvdir;
    float3 n = box_min * ray_inv_dir + oxinvdir;
    float3 tmax = max(f, n);
    float3 tmin = min(f, n);
    float max_t = min(min3(tmax), t_max);
    float min_t = max(max3(tmin), t_min);

    return float2(min_t, max_t);
}


float mycopysign(float a, float b)
{
   return b < 0.f ? -a : a;
}

// Invert ray direction.
float3 safe_invdir(float3 d)
{
    float dirx = d.x;
    float diry = d.y;
    float dirz = d.z;
    float ooeps = 1e-5;
    float3 invdir;
    invdir.x = 1.0 / (abs(dirx) > ooeps ? dirx : mycopysign(ooeps, dirx));
    invdir.y = 1.0 / (abs(diry) > ooeps ? diry : mycopysign(ooeps, diry));
    invdir.z = 1.0 / (abs(dirz) > ooeps ? dirz : mycopysign(ooeps, dirz));
    return invdir;
}

uint2 IntersectInternalNode(in BvhNode node, in float3 invd, in float3 o, in float tmin, in float tmax)
{
    float2 s0 = fast_intersect_bbox(o, invd, node.left_aabb_min_or_v0, node.left_aabb_max_or_v1, tmin, tmax);
    float2 s1 = fast_intersect_bbox(o, invd, node.right_aabb_min_or_v2, node.right_aabb_max_or_v3, tmin, tmax);

    uint traverse0 = (s0.x <= s0.y) ? node.left_child : INVALID_NODE;
    uint traverse1 = (s1.x <= s1.y) ? node.right_child : INVALID_NODE;

    return (s0.x < s1.x && traverse0 != INVALID_NODE) ? uint2(traverse0, traverse1) : uint2(traverse1, traverse0);
}

bool IntersectLeafNode(in BvhNode node, in float3 d, in float3 o, in float tmin, inout float closest_t, inout float2 uv)
{
    return fast_intersect_triangle(
        o, d, node.left_aabb_min_or_v0, node.left_aabb_max_or_v1, node.right_aabb_min_or_v2, tmin, closest_t, uv);
}