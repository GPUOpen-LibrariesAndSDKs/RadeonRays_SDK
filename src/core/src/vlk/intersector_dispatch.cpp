#include "intersector_dispatch.h"

#include "src/vlk/intersector.h"
#include "src/vlk/vulkan_wrappers.h"

namespace rt::vulkan
{
std::unique_ptr<IntersectorBase> CreateIntersector(DeviceBase& device, IntersectorType type)
{
    auto vk_gpu_helper = dynamic_cast<DeviceBackend<BackendType::kVulkan>&>(device).Get();

    switch (type)
    {
    case IntersectorType::kCompute:
        return CreateIntersector(vk_gpu_helper);
    default:
        throw std::runtime_error("Unsupported intersector type");
    }
}
}