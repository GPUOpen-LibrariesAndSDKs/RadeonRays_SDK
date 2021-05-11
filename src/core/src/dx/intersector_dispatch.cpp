#include "intersector_dispatch.h"

#include "dx/device.h"
#include "dx/fallback_intersector.h"

namespace rt::dx
{
std::unique_ptr<IntersectorBase> CreateIntersector(DeviceBase& device, IntersectorType type)
{
    auto d3d_device = dynamic_cast<DeviceBackend<BackendType::kDx12>&>(device).Get();

    switch (type)
    {
    case IntersectorType::kCompute:
        return CreateDx12Intersector(d3d_device);
    default:
        throw std::runtime_error("Unsupported intersector type");
    }
}

}  // namespace rt::dx