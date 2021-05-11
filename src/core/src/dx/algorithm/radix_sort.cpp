#include "radix_sort.h"

#include <stdexcept>

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
    kGroupHistograms,
    kOutputKeys,
    kInputValues,
    kOutputValues,
    kEntryCount
};

struct Constants
{
    std::uint32_t num_keys;
    std::uint32_t num_blocks;
    std::uint32_t bit_shift;
    std::uint32_t padding;
};

constexpr std::uint32_t kKeysPerThread  = 4u;
constexpr std::uint32_t kGroupSize      = 256u;
constexpr std::uint32_t kKeysPerGroup   = kKeysPerThread * kGroupSize;
constexpr std::uint32_t kNumBitsPerPass = 4;
}  // namespace

void RadixSortKeyValue::operator()(ID3D12GraphicsCommandList* command_list,
                                   D3D12_GPU_VIRTUAL_ADDRESS  input_keys,
                                   D3D12_GPU_VIRTUAL_ADDRESS  output_keys,
                                   D3D12_GPU_VIRTUAL_ADDRESS  input_values,
                                   D3D12_GPU_VIRTUAL_ADDRESS  output_values,
                                   D3D12_GPU_VIRTUAL_ADDRESS  scratch_data,
                                   std::uint32_t              size)
{
    std::uint32_t num_histogram_values = (1 << kNumBitsPerPass) * CeilDivide(size, kKeysPerGroup);
    std::uint32_t num_groups           = CeilDivide(size, kKeysPerGroup);

    D3D12_GPU_VIRTUAL_ADDRESS temp_keys        = scratch_data;
    D3D12_GPU_VIRTUAL_ADDRESS temp_values      = temp_keys + size * sizeof(int);
    D3D12_GPU_VIRTUAL_ADDRESS group_histograms = temp_values + size * sizeof(int);
    D3D12_GPU_VIRTUAL_ADDRESS scan_scratch     = group_histograms + num_histogram_values * sizeof(int);

    D3D12_GPU_VIRTUAL_ADDRESS i  = output_keys;
    D3D12_GPU_VIRTUAL_ADDRESS iv = output_values;

    D3D12_GPU_VIRTUAL_ADDRESS o  = temp_keys;
    D3D12_GPU_VIRTUAL_ADDRESS ov = temp_values;

    Constants constants{size, CeilDivide(size, kKeysPerGroup), 0, 0};
    auto      uav_barrier = CD3DX12_RESOURCE_BARRIER::UAV(nullptr);

    for (auto bitshift = 0u; bitshift < 32; bitshift += kNumBitsPerPass)
    {
        constants.bit_shift = bitshift;

        command_list->SetComputeRootSignature(root_signature_.Get());
        // Set kernel parameters.
        command_list->SetComputeRoot32BitConstants(RootSignature::kConstants, sizeof(Constants) / 4, &constants, 0);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kInputKeys, bitshift == 0 ? input_keys : i);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kGroupHistograms, group_histograms);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kOutputKeys, o);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kInputValues, bitshift == 0 ? input_values : iv);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kOutputValues, ov);

        // Calculate histograms

        command_list->SetPipelineState(pipeline_state_histograms_.Get());

        {
            command_list->Dispatch(num_groups, 1, 1);
        }

        command_list->ResourceBarrier(1u, &uav_barrier);

        {
            // Scan histograms
            scan_(command_list, group_histograms, group_histograms, scan_scratch, num_histogram_values);
        }

        command_list->SetComputeRootSignature(root_signature_.Get());
        // Set kernel parameters.
        command_list->SetComputeRoot32BitConstants(RootSignature::kConstants, sizeof(Constants) / 4, &constants, 0);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kInputKeys, bitshift == 0 ? input_keys : i);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kGroupHistograms, group_histograms);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kOutputKeys, o);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kInputValues, bitshift == 0 ? input_values : iv);
        command_list->SetComputeRootUnorderedAccessView(RootSignature::kOutputValues, ov);

        // Scatter key values
        command_list->SetPipelineState(pipeline_state_scatter_.Get());

        {
            command_list->Dispatch(num_groups, 1, 1);
        }

        command_list->ResourceBarrier(1u, &uav_barrier);

        std::swap(o, i);
        std::swap(ov, iv);
    }
}

std::size_t RadixSortKeyValue::GetScratchDataSize(std::uint32_t size) const
{
    std::uint32_t num_histogram_values = (1 << kNumBitsPerPass) * CeilDivide(size, kKeysPerGroup);

    std::size_t scratch_size = 0;
    // Histogram buffer: num bins * num groups
    scratch_size += num_histogram_values * sizeof(int);
    // Temporary buffers for ping-pong
    // additional 1024 ints are a workaround for when DX can generate out of bounds error
    // even if no actual out of bounds access happening
    scratch_size += 2 * size * sizeof(int) + 1024 * sizeof(int);
    // Scan scratch size
    scratch_size += scan_.GetScratchDataSize(num_histogram_values);

    return scratch_size;
}

void RadixSortKeyValue::Init()
{
    Shader bit_histogram_shader =
        ShaderCompiler::instance().CompileFromFile("../../src/core/src/dx/algorithm/kernels/bit_histogram.hlsl",
                                                   "cs_6_0",
                                                   "BitHistogram",
                                                   {"SORT_VALUES", "USE_WAVE_INTRINSICS"});

    Shader scatter_shader =
        ShaderCompiler::instance().CompileFromFile("../../src/core/src/dx/algorithm/kernels/scatter.hlsl",
                                                   "cs_6_0",
                                                   "Scatter",
                                                   {"SORT_VALUES", "USE_WAVE_INTRINSICS"});

    // Create root signature.
    CD3DX12_ROOT_PARAMETER root_entries[RootSignature::kEntryCount] = {};
    root_entries[kConstants].InitAsConstants(sizeof(Constants) >> 2, 0);
    root_entries[kInputKeys].InitAsUnorderedAccessView(0);
    root_entries[kGroupHistograms].InitAsUnorderedAccessView(1);
    root_entries[kOutputKeys].InitAsUnorderedAccessView(2);
    root_entries[kInputValues].InitAsUnorderedAccessView(3);
    root_entries[kOutputValues].InitAsUnorderedAccessView(4);

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
    desc.CS                                = bit_histogram_shader;
    desc.Flags                             = D3D12_PIPELINE_STATE_FLAG_NONE;

    ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline_state_histograms_)),
                  "Cannot create compute pipeline");

    desc.CS    = scatter_shader;
    desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(device_->CreateComputePipelineState(&desc, IID_PPV_ARGS(&pipeline_state_scatter_)),
                  "Cannot create compute pipeline");
}
}  // namespace rt::dx::algorithm
