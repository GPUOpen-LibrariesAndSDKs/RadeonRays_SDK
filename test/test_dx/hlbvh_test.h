#pragma once

#include <algorithm>
#include <chrono>
#include <climits>
#include <iostream>
#include <numeric>
#include <queue>
#include <random>
#include <tuple>
#include <stack>

#include "dx/dxassist.h"
#include "dx/hlbvh_builder.h"
#include "dx/restructure_hlbvh.h"
#include "dx/shader_compiler.h"
#include "dx/update_hlbvh.h"
#include "gtest/gtest.h"
#include "mesh_data.h"

#define CHECK_RR_CALL(x) ASSERT_EQ((x), RR_SUCCESS)

using namespace std::chrono;
using namespace rt::dx;

#ifdef _WIN32
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#endif

namespace
{
struct DxBvhNode
{
    uint32_t child0;
    uint32_t child1;
    uint32_t parent;
    uint32_t update;

    float aabb0_min_or_v0[3];
    float aabb0_max_or_v1[3];
    float aabb1_min_or_v2[3];
    float aabb1_max_or_v3[3];
};

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

inline Aabb GetAabb(DxBvhNode const& node)
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

inline float CalculateSAH(DxBvhNode* nodes)
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

}  // namespace

class HlbvhTest : public ::testing::Test
{
public:
    void SetUp() override {}

    void TearDown() override {}

    void        CopyToHost(ComPtr<ID3D12CommandAllocator> command_allocator,
                           ComPtr<ID3D12Resource>         src,
                           ComPtr<ID3D12Resource>         dest,
                           size_t                         size) const;
    static bool CheckConstistency(DxBvhNode* nodes);
    DxAssist    dxassist_;
};

bool HlbvhTest::CheckConstistency(DxBvhNode* nodes)
{
    std::queue<std::tuple<uint32_t, uint32_t>> q;
    q.push({0, kInvalidID});
    while (!q.empty())
    {
        auto current = q.front();
        q.pop();
        auto addr   = std::get<0>(current);
        auto parent = std::get<1>(current);

        auto node = nodes[addr];
        Aabb aabb = GetAabb(node);
        // check topology consistency
        if (parent != node.parent)
        {
            return false;
        }

        if (node.child0 != kInvalidID)
        {
            if (!aabb.Includes(GetAabb(nodes[node.child0])) || !aabb.Includes(GetAabb(nodes[node.child1])))
            {
                return false;
            }

            q.push({node.child0, addr});
            q.push({node.child1, addr});
        }
    }
    return true;
}

void HlbvhTest::CopyToHost(ComPtr<ID3D12CommandAllocator> command_allocator,
                           ComPtr<ID3D12Resource>         src,
                           ComPtr<ID3D12Resource>         dest,
                           size_t                         size) const
{
    auto                   copy_command_list = dxassist_.CreateCommandList(command_allocator.Get());
    D3D12_RESOURCE_BARRIER barrier;
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource   = src.Get();
    barrier.Transition.Subresource = 0;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_SOURCE;

    copy_command_list->ResourceBarrier(1, &barrier);
    copy_command_list->CopyBufferRegion(dest.Get(), 0, src.Get(), 0, size);
    copy_command_list->Close();

    ID3D12CommandList* copy_list[] = {copy_command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, copy_list);
    auto copy_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(copy_fence.Get(), 2000);
    while (copy_fence->GetCompletedValue() != 2000) Sleep(0);
}

TEST_F(HlbvhTest, MeshTest)
{
    auto       device = dxassist_.device();
    BuildHlBvh mesh_builder(device);

    MeshData mesh_data("../../resources/sponza.obj");
    ASSERT_TRUE(!(mesh_data.indices.size() % 3));
    ASSERT_TRUE(!(mesh_data.positions.size() % 3));

    auto vertex_buffer =
        dxassist_.CreateUploadBuffer(sizeof(float) * mesh_data.positions.size(), mesh_data.positions.data());
    auto index_buffer =
        dxassist_.CreateUploadBuffer(sizeof(std::uint32_t) * mesh_data.indices.size(), mesh_data.indices.data());
    uint32_t triangle_count = uint32_t(mesh_data.indices.size() / 3);
    uint32_t vertex_count   = uint32_t(mesh_data.positions.size() / 3);

    auto scratch_size   = mesh_builder.GetScratchDataSize(triangle_count);
    auto scratch_buffer = dxassist_.CreateUAVBuffer(scratch_size);
    auto result_size    = mesh_builder.GetResultDataSize(triangle_count);
    auto result_buffer  = dxassist_.CreateUAVBuffer(result_size);

    auto mesh_readback_buffer = dxassist_.CreateReadBackBuffer(result_size);
    auto command_allocator    = dxassist_.CreateCommandAllocator();
    auto command_list         = dxassist_.CreateCommandList(command_allocator.Get());

    mesh_builder(command_list.Get(),
                 vertex_buffer->GetGPUVirtualAddress(),
                 3 * sizeof(float),
                 vertex_count,
                 index_buffer->GetGPUVirtualAddress(),
                 triangle_count,
                 scratch_buffer->GetGPUVirtualAddress(),
                 result_buffer->GetGPUVirtualAddress());
    command_list->Close();
    ID3D12CommandList* builder_list[] = {command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, builder_list);
    auto build_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(build_fence.Get(), 1000);
    while (build_fence->GetCompletedValue() != 1000) Sleep(0);
    CopyToHost(command_allocator, result_buffer, mesh_readback_buffer, result_size);
    {
        DxBvhNode* mapped_ptr;
        mesh_readback_buffer->Map(0, nullptr, (void**)&mapped_ptr);
        ASSERT_TRUE(CheckConstistency(mapped_ptr));
        mesh_readback_buffer->Unmap(0, nullptr);
    }
}

TEST_F(HlbvhTest, RestructureTest)
{
    auto           device = dxassist_.device();
    BuildHlBvh     mesh_builder(device);
    RestructureBvh optimizer(device);
    MeshData       mesh_data("../../resources/sponza.obj");
    ASSERT_TRUE(!(mesh_data.indices.size() % 3));
    ASSERT_TRUE(!(mesh_data.positions.size() % 3));

    auto vertex_buffer =
        dxassist_.CreateUploadBuffer(sizeof(float) * mesh_data.positions.size(), mesh_data.positions.data());
    auto index_buffer =
        dxassist_.CreateUploadBuffer(sizeof(std::uint32_t) * mesh_data.indices.size(), mesh_data.indices.data());
    uint32_t triangle_count = uint32_t(mesh_data.indices.size() / 3);
    uint32_t vertex_count   = uint32_t(mesh_data.positions.size() / 3);

    auto scratch_size =
        std::max(mesh_builder.GetScratchDataSize(triangle_count), optimizer.GetScratchDataSize(triangle_count));
    auto scratch_buffer = dxassist_.CreateUAVBuffer(scratch_size);
    auto result_size    = mesh_builder.GetResultDataSize(triangle_count);
    auto result_buffer  = dxassist_.CreateUAVBuffer(result_size);

    auto mesh_readback_buffer = dxassist_.CreateReadBackBuffer(result_size);
    auto command_allocator    = dxassist_.CreateCommandAllocator();
    auto command_list         = dxassist_.CreateCommandList(command_allocator.Get());

    mesh_builder(command_list.Get(),
                 vertex_buffer->GetGPUVirtualAddress(),
                 3 * sizeof(float),
                 vertex_count,
                 index_buffer->GetGPUVirtualAddress(),
                 triangle_count,
                 scratch_buffer->GetGPUVirtualAddress(),
                 result_buffer->GetGPUVirtualAddress());

    command_list->Close();
    ID3D12CommandList* builder_list[] = {command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, builder_list);
    auto build_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(build_fence.Get(), 1000);
    while (build_fence->GetCompletedValue() != 1000) Sleep(0);

    CopyToHost(command_allocator, result_buffer, mesh_readback_buffer, result_size);
    DxBvhNode* mapped_ptr = nullptr;
    mesh_readback_buffer->Map(0, nullptr, (void**)&mapped_ptr);
    ASSERT_TRUE(CheckConstistency(mapped_ptr));
    float sah = CalculateSAH(mapped_ptr);
    mesh_readback_buffer->Unmap(0, nullptr);

    auto opt_command_list = dxassist_.CreateCommandList(command_allocator.Get());
    optimizer(opt_command_list.Get(),
              triangle_count,
              scratch_buffer->GetGPUVirtualAddress(),
              result_buffer->GetGPUVirtualAddress());
    opt_command_list->Close();
    ID3D12CommandList* opt_list[] = {opt_command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, opt_list);
    auto opt_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(opt_fence.Get(), 2000);
    while (opt_fence->GetCompletedValue() != 2000) Sleep(0);

    CopyToHost(command_allocator, result_buffer, mesh_readback_buffer, result_size);
    mesh_readback_buffer->Map(0, nullptr, (void**)&mapped_ptr);
    ASSERT_TRUE(CheckConstistency(mapped_ptr));
    float sah_opt = CalculateSAH(mapped_ptr);
    mesh_readback_buffer->Unmap(0, nullptr);
    ASSERT_TRUE(sah_opt < sah);

    std::cout << "Before optimization SAH: " << sah << std::endl;
    std::cout << "After optimization SAH: " << sah_opt << std::endl;
}

TEST_F(HlbvhTest, UpdateTest)
{
    auto       device = dxassist_.device();
    BuildHlBvh mesh_builder(device);
    UpdateBvh  updater(device);
    MeshData   mesh_data("../../resources/sponza.obj");
    ASSERT_TRUE(!(mesh_data.indices.size() % 3));
    ASSERT_TRUE(!(mesh_data.positions.size() % 3));

    auto vertex_buffer =
        dxassist_.CreateUploadBuffer(sizeof(float) * mesh_data.positions.size(), mesh_data.positions.data());
    for (size_t i = 0; i < mesh_data.positions.size(); i += 3)
    {
        mesh_data.positions[i + 1] += 10.0f;
    }
    auto vertex_upd_buffer =
        dxassist_.CreateUploadBuffer(sizeof(float) * mesh_data.positions.size(), mesh_data.positions.data());
    auto index_buffer =
        dxassist_.CreateUploadBuffer(sizeof(std::uint32_t) * mesh_data.indices.size(), mesh_data.indices.data());
    uint32_t triangle_count = uint32_t(mesh_data.indices.size() / 3);
    uint32_t vertex_count   = uint32_t(mesh_data.positions.size() / 3);

    auto scratch_size =
        std::max(mesh_builder.GetScratchDataSize(triangle_count), updater.GetScratchDataSize(triangle_count));
    auto scratch_buffer = dxassist_.CreateUAVBuffer(scratch_size);
    auto result_size    = mesh_builder.GetResultDataSize(triangle_count);
    auto result_buffer  = dxassist_.CreateUAVBuffer(result_size);

    auto mesh_readback_buffer = dxassist_.CreateReadBackBuffer(result_size);
    auto command_allocator    = dxassist_.CreateCommandAllocator();
    auto command_list         = dxassist_.CreateCommandList(command_allocator.Get());

    mesh_builder(command_list.Get(),
                 vertex_buffer->GetGPUVirtualAddress(),
                 3 * sizeof(float),
                 vertex_count,
                 index_buffer->GetGPUVirtualAddress(),
                 triangle_count,
                 scratch_buffer->GetGPUVirtualAddress(),
                 result_buffer->GetGPUVirtualAddress());
    updater(command_list.Get(),
            vertex_upd_buffer->GetGPUVirtualAddress(),
            3 * sizeof(float),
            vertex_count,
            index_buffer->GetGPUVirtualAddress(),
            triangle_count,
            scratch_buffer->GetGPUVirtualAddress(),
            result_buffer->GetGPUVirtualAddress());

    command_list->Close();
    ID3D12CommandList* builder_list[] = {command_list.Get()};
    dxassist_.command_queue()->ExecuteCommandLists(1, builder_list);
    auto scan_fence = dxassist_.CreateFence();
    dxassist_.command_queue()->Signal(scan_fence.Get(), 1000);
    while (scan_fence->GetCompletedValue() != 1000) Sleep(0);

    CopyToHost(command_allocator, result_buffer, mesh_readback_buffer, result_size);
    {
        DxBvhNode* mapped_ptr;
        mesh_readback_buffer->Map(0, nullptr, (void**)&mapped_ptr);
        ASSERT_TRUE(CheckConstistency(mapped_ptr));
        mesh_readback_buffer->Unmap(0, nullptr);
    }
}
