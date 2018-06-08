#include "intersector.h"
#include "device.h"

namespace RadeonRays
{
    Intersector::Intersector(Calc::Device *device)
        : m_device(device),
        m_counter(device->CreateBuffer(sizeof(int), Calc::BufferType::kRead),
                  [device](Calc::Buffer* buffer) { device->DeleteBuffer(buffer); })
    {
    }
    
    void Intersector::SetWorld(World const &world)
    {
        Process(world);
    }

    bool Intersector::IsCompatible(World const& world) const
    {
        return IsCompatibleImpl(world);
    }

    bool Intersector::IsCompatibleImpl(World const& world) const
    {
        return true;
    }
    
    void Intersector::QueryIntersection(std::uint32_t queue_idx, Calc::Buffer const *rays, std::uint32_t num_rays,
        Calc::Buffer *hits, Calc::Event const *wait_event, Calc::Event **event) const
    {
        m_device->WriteBuffer(m_counter.get(), 0, 0, sizeof(num_rays), &num_rays, nullptr);
        m_device->Finish(0);
        Intersect(queue_idx, rays, m_counter.get(), num_rays, hits, wait_event, event);
    }

    void Intersector::QueryOcclusion(std::uint32_t queue_idx, Calc::Buffer const *rays, std::uint32_t num_rays,
        Calc::Buffer *hits, Calc::Event const *wait_event, Calc::Event **event) const
    {
        m_device->WriteBuffer(m_counter.get(), 0, 0, sizeof(num_rays), &num_rays, nullptr);
        m_device->Finish(0);
        Occluded(queue_idx, rays, m_counter.get(), num_rays, hits, wait_event, event);
    }

    void Intersector::QueryIntersection(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays,
        std::uint32_t max_rays, Calc::Buffer *hits, Calc::Event const *wait_event, Calc::Event **event) const
    {
        Intersect(queue_idx, rays, num_rays, max_rays, hits, wait_event, event);
    }

    void Intersector::QueryOcclusion(std::uint32_t queue_idx, Calc::Buffer const *rays, Calc::Buffer const *num_rays,
        std::uint32_t max_rays, Calc::Buffer *hits, Calc::Event const *wait_event, Calc::Event **event) const
    {
        Occluded(queue_idx, rays, num_rays, max_rays, hits, wait_event, event);
    }
}
