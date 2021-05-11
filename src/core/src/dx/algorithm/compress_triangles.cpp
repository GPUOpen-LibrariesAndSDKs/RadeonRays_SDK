#include "compress_triangles.h"

#include "dx/common.h"
#include "dx/shader_compiler.h"

namespace rt::dx::algorithm
{
namespace
{
enum RootSignature
{
    kConstants,
    kVertices,
    kIndices,
    kBlocksCounter,
    kCompressedBlocks,
    kTrianglePointers,
    kEntryCount
};

struct Constants
{
    std::uint32_t vertex_stride;
    std::uint32_t triangle_count;
};

constexpr std::uint32_t kTrianglesPerThread = 16u;
constexpr std::uint32_t kGroupSize          = 256u;
constexpr std::uint32_t kTrianglesPerGroup  = kTrianglesPerThread * kGroupSize;
}  // namespace

void CompressTriangles::Init()
{
    Shader init_shader =
        ShaderCompiler::instance().CompileFromFile("../../src/core/src/dx/algorithm/kernels/compress_triangles.hlsl",
                                                   "cs_6_0",
                                                   "InitBlockCount",
                                                   {"USE_WAVE_INTRINSICS"});

    Shader compress_shader =
        ShaderCompiler::instance().CompileFromFile("../../src/core/src/dx/algorithm/kernels/compress_triangles.hlsl",
                                                   "cs_6_0",
                                                   "CompressTriangles",
                                                   {"USE_WAVE_INTRINSICS"});

    // Create root signature.
    CD3DX12_ROOT_PARAMETER root_entries[RootSignature::kEntryCount] = {};
    root_entries[kConstants].InitAsConstants(sizeof(Constants) >> 2, 0);
    root_entries[kVertices].InitAsUnorderedAccessView(0);
    root_entries[kIndices].InitAsUnorderedAccessView(1);
    root_entries[kBlocksCounter].InitAsUnorderedAccessView(2);
    root_entries[kCompressedBlocks].InitAsUnorderedAccessView(3);
    root_entries[kTrianglePointers].InitAsUnorderedAccessView(4);

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc;
    root_signature_desc.Init(RootSignature::kEntryCount, root_entries);

    ComPtr<ID3DBlob>   error_blob          = nullptr;
    ComPtr<ID3D10Blob> root_signature_blob = nullptr;

    if (FAILED(D3D12SerializeRootSignature(
            &root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1_0, &root_signature_blob, &error_blob)))
    {
        if (error_blob)
        {
            std::string error_str(static_cast<const char*>(error_blob->GetBufferPointer()));
            throw std::runtime_error(error_str);
        } else
        {
            throw std::runtime_error("Failed to serialize root signature");
        }
    }

    ThrowIfFailed(device_->CreateRootSignature(0,
                                               root_signature_blob->GetBufferPointer(),
                                               root_signature_blob->GetBufferSize(),
                                               IID_PPV_ARGS(&root_signature_)),
                  "Failed to create root signature");

    // Create pipeline states.
    D3D12_COMPUTE_PIPELINE_STATE_DESC desc = {};
    desc.pRootSignature                    = root_signature_.Get();

    desc.CS    = compress_shader;
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline_state_)),
                  "Cannot create compute pipeline");

    desc.CS    = init_shader;
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline_state_init_)),
                  "Cannot create compute pipeline");

    blocks_count_ = AllocateUAVBuffer(device_.Get(), sizeof(std::uint32_t));
}

void CompressTriangles::operator()(ID3D12GraphicsCommandList* command_list,
                                   D3D12_GPU_VIRTUAL_ADDRESS  vertices,
                                   std::uint32_t              vertex_stride,
                                   std::uint32_t,
                                   D3D12_GPU_VIRTUAL_ADDRESS indices,
                                   std::uint32_t             triangle_count,
                                   D3D12_GPU_VIRTUAL_ADDRESS compressed_blocks,
                                   D3D12_GPU_VIRTUAL_ADDRESS triangle_pointers)
{
    Constants constants{vertex_stride, triangle_count};

    command_list->SetComputeRootSignature(root_signature_.Get());
    command_list->SetComputeRoot32BitConstants(RootSignature::kConstants, sizeof(Constants) >> 2, &constants, 0);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kVertices, vertices);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kIndices, indices);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kBlocksCounter,
                                                    blocks_count_->GetGPUVirtualAddress());
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kCompressedBlocks, compressed_blocks);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kTrianglePointers, triangle_pointers);

    // Initalize blocks counter to zero.
    {
        command_list->SetPipelineState(pipeline_state_init_.Get());
        command_list->Dispatch(1u, 1u, 1u);
    }

    // Make sure counter is visible.
    auto uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);
    command_list->ResourceBarrier(1u, &uav_barrier);

    // Compress triangles into blocks.
    {
        command_list->SetPipelineState(pipeline_state_.Get());
        command_list->Dispatch(CeilDivide(triangle_count, kTrianglesPerGroup), 1u, 1u);
    }

    // Make sure blocks are written.
    command_list->ResourceBarrier(1u, &uav_barrier);
}
}  // namespace rt::dx::algorithm
