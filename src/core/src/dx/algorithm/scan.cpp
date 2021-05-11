#include "scan.h"

#include "dx/common.h"
#include "dx/shader_compiler.h"

namespace rt::dx::algorithm
{
namespace
{
enum RootSignature
{
    kConstants,
    kInputKeys,
    kPartSums,
    kOutputKeys,
    kEntryCount
};

constexpr std::uint32_t kKeysPerThread = 4u;
constexpr std::uint32_t kGroupSize     = 256u;
constexpr std::uint32_t kKeysPerGroup  = kKeysPerThread * kGroupSize;
}  // namespace

void Scan::Init()
{
    Shader scan_shader = ShaderCompiler::instance().CompileFromFile(
        "../../src/core/src/dx/algorithm/kernels/block_scan.hlsl", "cs_6_0", "BlockScanAdd", {"USE_WAVE_INTRINSICS"});

    Shader scan_carryout_shader =
        ShaderCompiler::instance().CompileFromFile("../../src/core/src/dx/algorithm/kernels/block_scan.hlsl",
                                                   "cs_6_0",
                                                   "BlockScanAdd",
                                                   {"USE_WAVE_INTRINSICS", "ADD_PART_SUM"});

    Shader reduce_shader = ShaderCompiler::instance().CompileFromFile(
        "../../src/core/src/dx/algorithm/kernels/block_reduce_part.hlsl", "cs_6_0", "BlockReducePart");

    // Create root signature.
    CD3DX12_ROOT_PARAMETER root_entries[RootSignature::kEntryCount] = {};
    root_entries[kConstants].InitAsConstants(1, 0);
    root_entries[kInputKeys].InitAsUnorderedAccessView(0);
    root_entries[kPartSums].InitAsUnorderedAccessView(1);
    root_entries[kOutputKeys].InitAsUnorderedAccessView(2);

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
    desc.CS                                = reduce_shader;
    desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline_state_reduce_)),
                  "Cannot create compute pipeline");

    desc.CS = scan_shader;
    ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline_state_scan_)),
                  "Cannot create compute pipeline");

    desc.CS = scan_carryout_shader;
    ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline_state_scan_carryout_)),
                  "Cannot create compute pipeline");
}

void Scan::SetState(ID3D12GraphicsCommandList* command_list,
                    ID3D12PipelineState*       state,
                    UINT                       size,
                    D3D12_GPU_VIRTUAL_ADDRESS  input_keys,
                    D3D12_GPU_VIRTUAL_ADDRESS  scratch_data,
                    D3D12_GPU_VIRTUAL_ADDRESS  output_keys)
{
    // Set kernel parameters.
    command_list->SetComputeRoot32BitConstant(RootSignature::kConstants, size, 0);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kInputKeys, input_keys);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kPartSums, scratch_data);
    command_list->SetComputeRootUnorderedAccessView(RootSignature::kOutputKeys, output_keys);
    command_list->SetPipelineState(state);
}

void Scan::operator()(ID3D12GraphicsCommandList* command_list,
                      D3D12_GPU_VIRTUAL_ADDRESS  input_keys,
                      D3D12_GPU_VIRTUAL_ADDRESS  output_keys,
                      D3D12_GPU_VIRTUAL_ADDRESS  scratch_data,
                      std::uint32_t              size)
{
    // Up to 3 level of hierarchical scan is supported.
    // Scratch memory map.
    // scratch_data ->| num_group_level_1 - part sums (one per group) |
    // num_group_level 2 - part sums |

    command_list->SetComputeRootSignature(root_signature_.Get());
    auto uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);

    if (size > kKeysPerGroup)
    {
        auto num_groups_level_1 = CeilDivide(size, kKeysPerGroup);

        // Do first round of part sum reduction.
        SetState(command_list, pipeline_state_reduce_.Get(), size, input_keys, scratch_data, output_keys);
        {
            command_list->Dispatch(num_groups_level_1, 1u, 1u);
        }
        command_list->ResourceBarrier(1u, &uav_barrier);

        if (num_groups_level_1 > kKeysPerGroup)
        {
            auto num_groups_level_2 = CeilDivide(num_groups_level_1, kKeysPerGroup);

            // Do second round of part sum reduction.
            SetState(command_list,
                     pipeline_state_reduce_.Get(),
                     num_groups_level_1,
                     scratch_data,
                     scratch_data + num_groups_level_1 * sizeof(int),
                     scratch_data);
            {
                command_list->Dispatch(num_groups_level_2, 1u, 1u);
            }
            command_list->ResourceBarrier(1u, &uav_barrier);

            // Scan level 2 inplace.
            SetState(command_list,
                     pipeline_state_scan_.Get(),
                     num_groups_level_2,
                     scratch_data + num_groups_level_1 * sizeof(int),
                     0,
                     scratch_data + num_groups_level_1 * sizeof(int));
            {
                command_list->Dispatch(1u, 1u, 1u);
            }
            command_list->ResourceBarrier(1u, &uav_barrier);
        }

        // Scan and add level 2 back to level 1.
        ID3D12PipelineState* state =
            num_groups_level_1 > kKeysPerGroup ? pipeline_state_scan_carryout_.Get() : pipeline_state_scan_.Get();

        D3D12_GPU_VIRTUAL_ADDRESS scratch_buffer =
            num_groups_level_1 > kKeysPerGroup ? (scratch_data + num_groups_level_1 * sizeof(int)) : 0;

        SetState(command_list, state, num_groups_level_1, scratch_data, scratch_buffer, scratch_data);
        {
            auto num_groups_scan_level_1 = CeilDivide(num_groups_level_1, kKeysPerGroup);
            command_list->Dispatch(num_groups_scan_level_1, 1u, 1u);
        }
        command_list->ResourceBarrier(1u, &uav_barrier);
    }

    // Scan and add level 1 back.
    ID3D12PipelineState* state =
        size > kKeysPerGroup ? pipeline_state_scan_carryout_.Get() : pipeline_state_scan_.Get();

    D3D12_GPU_VIRTUAL_ADDRESS scratch_buffer = size > kKeysPerGroup ? scratch_data : 0;

    SetState(command_list, state, size, input_keys, scratch_buffer, output_keys);
    {
        auto num_groups_scan_level_0 = CeilDivide(size, kKeysPerGroup);
        command_list->Dispatch(num_groups_scan_level_0, 1u, 1u);
    }

    command_list->ResourceBarrier(1u, &uav_barrier);
}

std::size_t Scan::GetScratchDataSize(std::uint32_t size) const
{
    if (size <= kKeysPerGroup)
    {
        return 0;
    } else
    {
        auto size_level_1 = CeilDivide(size, kKeysPerGroup);
        if (size_level_1 <= kKeysPerGroup)
        {
            return size_level_1 * sizeof(int);
        } else
        {
            auto size_level_2 = CeilDivide(size_level_1, kKeysPerGroup);
            return (size_level_1 + size_level_2) * sizeof(int);
        }
    }
}
}  // namespace rt::dx::algorithm
