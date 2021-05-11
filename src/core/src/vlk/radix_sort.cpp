#include "radix_sort.h"

#include <stdexcept>
#include <vector>

#include "vlk/common.h"
#include "vlk/shader_manager.h"

namespace rt::vulkan::algorithm
{
namespace
{
constexpr uint32_t       kNumBitsPerPass   = 4;
constexpr uint32_t       kHistogramNumBins = (1 << kNumBitsPerPass);

struct Parameters
{
    Parameters() = default;
    Parameters(uint32_t keys_per_thread, uint32_t group_size, std::string histogram_kernel, std::string scatter_kernel)
        : keys_per_thread_(keys_per_thread),
          group_size_(group_size),
          keys_per_group_(keys_per_thread * group_size),
          histogram_kernel_name_(std::move(histogram_kernel)),
          scatter_kernel_name_(std::move(scatter_kernel))
    {
    }
    auto     CalculateNumGroups(uint32_t num_keys) { return CeilDivide(num_keys, keys_per_group_); }
    uint32_t CalculateNumGroupHistogramElements(uint32_t num_keys)
    {
        auto num_histograms = CeilDivide(num_keys, keys_per_group_);
        return num_histograms * kHistogramNumBins;
    };

    uint32_t    keys_per_thread_    = 8u;
    uint32_t    group_size_         = 256u;
    uint32_t    keys_per_group_     = 2048u;
    std::string histogram_kernel_name_ = "radix_sort_group_histograms.comp.spv";
    std::string scatter_kernel_name_   = "radix_sort_scatter.comp.spv";
};

struct AmdParameters : public Parameters
{
    AmdParameters()
        : Parameters(4u, 256u, "radix_sort_group_histograms_amd.comp.spv", "radix_sort_scatter_amd.comp.spv")
    {
    }
};


}  // namespace

struct RadixSortKeyValue::RadixSortImpl
{
    // Scratch space layout.
    enum class ScratchLayout
    {
        kTempKeys,
        kTempValues,
        kGroupHistograms,
        kScanScratch
    };
#pragma pack(push, 1)
    struct PushConstants
    {
        uint32_t g_num_keys;
        uint32_t g_bit_shift;
    };
#pragma pack(pop)

    std::shared_ptr<GpuHelper> gpu_helper_;
    ShaderManager const&       shader_manager_;
    // Scan for histogram scans.
    Scan     scan_;
    uint32_t num_keys_;

    // shader things
    std::vector<DescriptorSet> histogram_desc_sets_;
    std::vector<DescriptorSet> scatter_desc_sets_;
    ShaderPtr                  histogram_kernel_;
    ShaderPtr                  scatter_kernel_;
    DescriptorCacheTable<5, 6> cache_;
    std::shared_ptr<Parameters> parameters_;

    using ScratchLayoutT                   = MemoryLayout<ScratchLayout, vk::DeviceSize>;
    mutable ScratchLayoutT scratch_layout_ = ScratchLayoutT(kAlignment);

    RadixSortImpl(std::shared_ptr<GpuHelper> helper, ShaderManager const& manager)
        : gpu_helper_(helper), shader_manager_(manager), scan_(helper, manager), cache_(helper)
    {
        Init();
    }

    ~RadixSortImpl()
    {
        // there are only one layout with a few descriptor sets
        gpu_helper_->device.destroyDescriptorSetLayout(histogram_desc_sets_[0].layout_);
        // the same
        gpu_helper_->device.destroyDescriptorSetLayout(scatter_desc_sets_[0].layout_);
    }

    void Init()
    {
        if (gpu_helper_->IsAmdDevice())
        {
            parameters_ = std::make_shared<AmdParameters>();
        } else
        {
            parameters_ = std::make_shared<Parameters>();
        }
        // Create hist kernel.
        histogram_kernel_    = shader_manager_.CreateKernel(parameters_->histogram_kernel_name_);
        histogram_desc_sets_ = shader_manager_.CreateDescriptorSets(histogram_kernel_);
        shader_manager_.PrepareKernel(parameters_->histogram_kernel_name_, histogram_desc_sets_);
        histogram_desc_sets_.push_back(histogram_desc_sets_[0]);
        histogram_desc_sets_.push_back(histogram_desc_sets_[0]);

        // Create scatter kernel.
        scatter_kernel_    = shader_manager_.CreateKernel(parameters_->scatter_kernel_name_);
        scatter_desc_sets_ = shader_manager_.CreateDescriptorSets(scatter_kernel_);
        shader_manager_.PrepareKernel(parameters_->scatter_kernel_name_, scatter_desc_sets_);
        scatter_desc_sets_.push_back(scatter_desc_sets_[0]);
        scatter_desc_sets_.push_back(scatter_desc_sets_[0]);
    }
    void AllocateDescriptorSets()
    {
        // Allocate histogram desc sets.
        histogram_desc_sets_[0].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(histogram_desc_sets_[0].layout_);
        histogram_desc_sets_[1].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(histogram_desc_sets_[1].layout_);
        histogram_desc_sets_[2].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(histogram_desc_sets_[2].layout_);
        // Allocate scatter desc sets.
        scatter_desc_sets_[0].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(scatter_desc_sets_[0].layout_);
        scatter_desc_sets_[1].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(scatter_desc_sets_[1].layout_);
        scatter_desc_sets_[2].descriptor_set_ = gpu_helper_->AllocateDescriptorSet(scatter_desc_sets_[2].layout_);
    }

    bool GetDescriptorSets(const std::array<vk::Buffer, 5>&)
    {
        // there is no option to assign from cache, so just allocate new one
        // if we assign and buffer is the same we will get the issue with incorrect descriptor set
        // works fine under validation but for some reason scratch buffer migth be same for different meshes
        AllocateDescriptorSets();
        return false;
    }
    void PushDescriptorSetsToCache(const std::array<vk::Buffer, 5>& keys, const std::array<vk::DescriptorSet, 6> values)
    {
        cache_.Push(keys, values);
    }
};

RadixSortKeyValue::RadixSortKeyValue(std::shared_ptr<GpuHelper> gpu_helper, ShaderManager const& manager)
    : impl_(std::make_unique<RadixSortImpl>(gpu_helper, manager))
{
}

RadixSortKeyValue::~RadixSortKeyValue() = default;

void RadixSortKeyValue::operator()(vk::CommandBuffer command_buffer,
                                   vk::Buffer        input_keys,
                                   vk::DeviceSize    input_keys_offset,
                                   vk::DeviceSize    input_keys_size,
                                   vk::Buffer        output_keys,
                                   vk::DeviceSize    output_keys_offset,
                                   vk::DeviceSize    output_keys_size,
                                   vk::Buffer        input_values,
                                   vk::DeviceSize    input_values_offset,
                                   vk::DeviceSize    input_values_size,
                                   vk::Buffer        output_values,
                                   vk::DeviceSize    output_values_offset,
                                   vk::DeviceSize    output_values_size,
                                   vk::Buffer        scratch_data,
                                   vk::DeviceSize    scratch_offset,
                                   uint32_t          num_keys)
{
    AdjustLayouts(num_keys);
    // bind memory to temporary buffers and update descriptors
    UpdateDescriptors(input_keys,
                      input_keys_offset,
                      input_keys_size,
                      output_keys,
                      output_keys_offset,
                      output_keys_size,
                      input_values,
                      input_values_offset,
                      input_values_size,
                      output_values,
                      output_values_offset,
                      output_values_size,
                      scratch_data,
                      scratch_offset);

    auto group_histograms_size =
        (vk::DeviceSize)impl_->scratch_layout_.size_of(RadixSortImpl::ScratchLayout::kGroupHistograms);
    auto temp_keys_size   = (vk::DeviceSize)impl_->scratch_layout_.size_of(RadixSortImpl::ScratchLayout::kTempKeys);
    auto temp_values_size = (vk::DeviceSize)impl_->scratch_layout_.size_of(RadixSortImpl::ScratchLayout::kTempValues);

    auto group_histograms_offset = impl_->scratch_layout_.offset_of(RadixSortImpl::ScratchLayout::kGroupHistograms);
    auto temp_keys_offset        = impl_->scratch_layout_.offset_of(RadixSortImpl::ScratchLayout::kTempKeys);
    auto temp_values_offset      = impl_->scratch_layout_.offset_of(RadixSortImpl::ScratchLayout::kTempValues);
    auto scan_offset             = impl_->scratch_layout_.offset_of(RadixSortImpl::ScratchLayout::kScanScratch);

    uint32_t num_histogram_elems = impl_->parameters_->CalculateNumGroupHistogramElements(num_keys);
    // Here is what is happening here: We are taking 8 passes and
    // sort according to 4 next bits (lsb to msb) every pass using stable sort.
    // This effectively gives us fully sorted 32-bit sequence at the end.
    // Since we are using ping-pong we are doing the following:
    // 0th iteration takes elements from input and outputs them into temporary buffer.
    // 1st iteration takes partially sorted elements from temporary buffer and outputs
    //     them into output.
    // 2nd iteration takes partially sorted elements from output buffer and outputs
    //     them into temporaty buffer.
    // 3rd iteration takes partially sorted elements from temporary buffer and outputs
    //     them into output.
    // and so on.
    // After 7th iteration we have our fully sorted sequence in the output buffer.
    for (auto bitshift = 0u; bitshift < 32; bitshift += kNumBitsPerPass)
    {
        // Calculate group histograms
        auto pass = bitshift == 0 ? 2 : ((bitshift & ((kNumBitsPerPass << 1) - 1)) ? 0u : 1u);

        // Set key count and bit shift.
        RadixSortImpl::PushConstants push_consts_hist{num_keys, bitshift};

        impl_->gpu_helper_->EncodePushConstant(
            impl_->histogram_kernel_->pipeline_layout, 0u, sizeof(push_consts_hist), &push_consts_hist, command_buffer);

        vk::DescriptorSet desc_sets_hist[] = {impl_->histogram_desc_sets_[pass].descriptor_set_};

        // Bind internal and user desc sets.
        impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets_hist,
                                                     sizeof(desc_sets_hist) / sizeof(desc_sets_hist[0]),
                                                     0u,
                                                     impl_->histogram_kernel_->pipeline_layout,
                                                     command_buffer);

        // Launch the kernel.
        auto num_groups = impl_->parameters_->CalculateNumGroups(num_keys);
        impl_->shader_manager_.EncodeDispatch1D(*impl_->histogram_kernel_, num_groups, command_buffer);

        // Make sure they are ready to be scanned
        impl_->gpu_helper_->EncodeBufferBarrier(scratch_data,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer,
                                                group_histograms_offset,
                                                group_histograms_size);

        // Perform scan over the array of histograms
        impl_->scan_(command_buffer,
                     scratch_data,
                     group_histograms_offset,
                     group_histograms_size,
                     scratch_data,
                     group_histograms_offset,
                     group_histograms_size,
                     scratch_data,
                     scan_offset,
                     num_histogram_elems);

        impl_->gpu_helper_->EncodeBufferBarrier(scratch_data,
                                                vk::AccessFlagBits::eShaderWrite,
                                                vk::AccessFlagBits::eShaderRead,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                vk::PipelineStageFlagBits::eComputeShader,
                                                command_buffer,
                                                group_histograms_offset,
                                                group_histograms_size);

        // Scatter keys at the appropriate locations
        pass = (bitshift == 0u) ? 2u : ((bitshift & ((kNumBitsPerPass << 1) - 1)) ? 0u : 1u);

        RadixSortImpl::PushConstants push_consts_scatter{num_keys, bitshift};

        // Set key count and bit shift.
        impl_->gpu_helper_->EncodePushConstant(impl_->scatter_kernel_->pipeline_layout,
                                               0u,
                                               sizeof(push_consts_scatter),
                                               &push_consts_scatter,
                                               command_buffer);

        vk::DescriptorSet desc_sets_scatter[] = {impl_->scatter_desc_sets_[pass].descriptor_set_};

        // Bind internal and user desc sets.
        impl_->gpu_helper_->EncodeBindDescriptorSets(desc_sets_scatter,
                                                     sizeof(desc_sets_scatter) / sizeof(desc_sets_scatter[0]),
                                                     0u,
                                                     impl_->scatter_kernel_->pipeline_layout,
                                                     command_buffer);

        // Launch the kernel.
        num_groups = impl_->parameters_->CalculateNumGroups(num_keys);
        impl_->shader_manager_.EncodeDispatch1D(*impl_->scatter_kernel_, num_groups, command_buffer);

        // Determine which buffer we have just written and put a barrier on it
        pass = (bitshift == 0u) ? 2u : ((bitshift & ((kNumBitsPerPass << 1) - 1)) ? 0u : 1u);

        if (pass == 0u)
        {
            impl_->gpu_helper_->EncodeBufferBarrier(output_keys,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    output_keys_offset,
                                                    output_keys_size);

            impl_->gpu_helper_->EncodeBufferBarrier(output_values,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    output_values_offset,
                                                    output_values_size);
        } else
        {
            impl_->gpu_helper_->EncodeBufferBarrier(scratch_data,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    temp_keys_offset,
                                                    temp_keys_size);

            impl_->gpu_helper_->EncodeBufferBarrier(scratch_data,
                                                    vk::AccessFlagBits::eShaderWrite,
                                                    vk::AccessFlagBits::eShaderRead,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    vk::PipelineStageFlagBits::eComputeShader,
                                                    command_buffer,
                                                    temp_values_offset,
                                                    temp_values_size);
        }
    }
}

size_t RadixSortKeyValue::GetScratchDataSize(uint32_t size) const
{
    AdjustLayouts(size);
    return impl_->scratch_layout_.total_size();
}

void RadixSortKeyValue::AdjustLayouts(uint32_t size) const
{
    if (impl_->num_keys_ == size)
    {
        return;
    }
    impl_->num_keys_ = size;

    impl_->scratch_layout_.Reset();
    auto scan_size = impl_->scan_.GetScratchDataSize(size);
    impl_->scratch_layout_.AppendBlock<uint8_t>(RadixSortImpl::ScratchLayout::kScanScratch, scan_size);
    impl_->scratch_layout_.AppendBlock<uint32_t>(RadixSortImpl::ScratchLayout::kTempKeys, size);
    impl_->scratch_layout_.AppendBlock<uint32_t>(RadixSortImpl::ScratchLayout::kTempValues, size);
    auto hist_size = impl_->parameters_->CalculateNumGroupHistogramElements(size);
    impl_->scratch_layout_.AppendBlock<uint32_t>(RadixSortImpl::ScratchLayout::kGroupHistograms, hist_size);
}

void RadixSortKeyValue::UpdateDescriptors(vk::Buffer     input_keys,
                                          vk::DeviceSize input_keys_offset,
                                          vk::DeviceSize input_keys_size,
                                          vk::Buffer     output_keys,
                                          vk::DeviceSize output_keys_offset,
                                          vk::DeviceSize output_keys_size,
                                          vk::Buffer     input_values,
                                          vk::DeviceSize input_values_offset,
                                          vk::DeviceSize input_values_size,
                                          vk::Buffer     output_values,
                                          vk::DeviceSize output_values_offset,
                                          vk::DeviceSize output_values_size,
                                          vk::Buffer     scratch_data,
                                          vk::DeviceSize scratch_offset) const
{
    impl_->scratch_layout_.SetBaseOffset(scratch_offset);
    if (!impl_->GetDescriptorSets({input_keys, output_keys, input_values, output_values, scratch_data}))
    {
        auto group_histograms_size =
            (vk::DeviceSize)impl_->scratch_layout_.size_of(RadixSortImpl::ScratchLayout::kGroupHistograms);
        auto temp_keys_size = (vk::DeviceSize)impl_->scratch_layout_.size_of(RadixSortImpl::ScratchLayout::kTempKeys);
        auto temp_values_size =
            (vk::DeviceSize)impl_->scratch_layout_.size_of(RadixSortImpl::ScratchLayout::kTempValues);

        auto group_histograms_offset = impl_->scratch_layout_.offset_of(RadixSortImpl::ScratchLayout::kGroupHistograms);
        auto temp_keys_offset        = impl_->scratch_layout_.offset_of(RadixSortImpl::ScratchLayout::kTempKeys);
        auto temp_values_offset      = impl_->scratch_layout_.offset_of(RadixSortImpl::ScratchLayout::kTempValues);
        // Since we are using ping-pong we need 3 desc sets:
        // 1) input -> temp
        // 2) temp -> output
        // 3) output -> temp
        // We only apply (1) once for getting input data into
        // the temporary buffer and then we ping-pong between
        // temp and output. Input buffer remain unchanged.

        // Update descriptors.
        vk::DescriptorBufferInfo histogram_start_infos[] = {
            {input_keys, input_keys_offset, input_keys_size},
            {scratch_data, group_histograms_offset, group_histograms_size}};

        vk::DescriptorBufferInfo histogram_ping_infos[] = {
            {scratch_data, temp_keys_offset, temp_keys_size},
            {scratch_data, group_histograms_offset, group_histograms_size}};

        vk::DescriptorBufferInfo histogram_pong_infos[] = {
            {output_keys, output_keys_offset, output_keys_size},
            {scratch_data, group_histograms_offset, group_histograms_size}};

        impl_->gpu_helper_->WriteDescriptorSet(impl_->histogram_desc_sets_[0].descriptor_set_,
                                               histogram_ping_infos,
                                               sizeof(histogram_ping_infos) / sizeof(histogram_ping_infos[0]));

        impl_->gpu_helper_->WriteDescriptorSet(impl_->histogram_desc_sets_[1].descriptor_set_,
                                               histogram_pong_infos,
                                               sizeof(histogram_pong_infos) / sizeof(histogram_pong_infos[0]));

        impl_->gpu_helper_->WriteDescriptorSet(impl_->histogram_desc_sets_[2].descriptor_set_,
                                               histogram_start_infos,
                                               sizeof(histogram_start_infos) / sizeof(histogram_start_infos[0]));

        // Update descriptors.
        vk::DescriptorBufferInfo scatter_start_infos[] = {
            {input_keys, input_keys_offset, input_keys_size},
            {scratch_data, group_histograms_offset, group_histograms_size},
            {scratch_data, temp_keys_offset, temp_keys_size},
            {input_values, input_values_offset, input_values_size},
            {scratch_data, temp_values_offset, temp_values_size}};

        vk::DescriptorBufferInfo scatter_ping_infos[] = {{scratch_data, temp_keys_offset, temp_keys_size},
                                                         {scratch_data, group_histograms_offset, group_histograms_size},
                                                         {output_keys, output_keys_offset, output_keys_size},
                                                         {scratch_data, temp_values_offset, temp_values_size},
                                                         {output_values, output_values_offset, output_values_size}};

        vk::DescriptorBufferInfo scatter_pong_infos[] = {{output_keys, output_keys_offset, output_keys_size},
                                                         {scratch_data, group_histograms_offset, group_histograms_size},
                                                         {scratch_data, temp_keys_offset, temp_keys_size},
                                                         {output_values, output_values_offset, output_values_size},
                                                         {scratch_data, temp_values_offset, temp_values_size}};

        impl_->gpu_helper_->WriteDescriptorSet(impl_->scatter_desc_sets_[0].descriptor_set_,
                                               scatter_ping_infos,
                                               sizeof(scatter_ping_infos) / sizeof(scatter_ping_infos[0]));

        impl_->gpu_helper_->WriteDescriptorSet(impl_->scatter_desc_sets_[1].descriptor_set_,
                                               scatter_pong_infos,
                                               sizeof(scatter_pong_infos) / sizeof(scatter_pong_infos[0]));

        impl_->gpu_helper_->WriteDescriptorSet(impl_->scatter_desc_sets_[2].descriptor_set_,
                                               scatter_start_infos,
                                               sizeof(scatter_start_infos) / sizeof(scatter_start_infos[0]));

        impl_->PushDescriptorSetsToCache({input_keys, output_keys, input_values, output_values, scratch_data},
                                         {impl_->histogram_desc_sets_[0].descriptor_set_,
                                          impl_->histogram_desc_sets_[1].descriptor_set_,
                                          impl_->histogram_desc_sets_[2].descriptor_set_,
                                          impl_->scatter_desc_sets_[0].descriptor_set_,
                                          impl_->scatter_desc_sets_[1].descriptor_set_,
                                          impl_->scatter_desc_sets_[2].descriptor_set_});
    }
}  // namespace rt::vulkan::algorithm

}  // namespace rt::vulkan::algorithm
