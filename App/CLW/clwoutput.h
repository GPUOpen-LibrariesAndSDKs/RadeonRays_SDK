#pragma once

#include "Core/output.h"
#include "CLW.h"

namespace Baikal
{
    class ClwOutput : public Output
    {
    public:
        ClwOutput(std::uint32_t w, std::uint32_t h, CLWContext context)
        : Output(w, h)
        , m_context(context)
        , m_data(context.CreateBuffer<RadeonRays::float3>(w*h, CL_MEM_READ_WRITE))
        , m_position(context.CreateBuffer<RadeonRays::float3>(w*h, CL_MEM_READ_WRITE))
        , m_shadow(context.CreateBuffer<RadeonRays::float3>(w*h, CL_MEM_READ_WRITE))
        {
        }

        void GetData(RadeonRays::float3* data) const
        {
            m_context.ReadBuffer(0, m_data, data, m_data.GetElementCount()).Wait();
        }

        void Clear(RadeonRays::float3 const& val)
        {
            m_context.FillBuffer(0, m_data, val, m_data.GetElementCount()).Wait();
            m_context.FillBuffer(0, m_position, float3(0, 0, 0), m_position.GetElementCount()).Wait();
            m_context.FillBuffer(0, m_shadow, float3(0, 0, 0), m_position.GetElementCount()).Wait();
        }

        CLWBuffer<RadeonRays::float3> data() const { return m_data; }
        CLWBuffer<RadeonRays::float3> position() const { return m_position; }
        CLWBuffer<RadeonRays::float3> shadow() const { return m_shadow; }

    private:
        CLWBuffer<RadeonRays::float3> m_data;
        CLWBuffer<RadeonRays::float3> m_position;
        CLWBuffer<RadeonRays::float3> m_shadow;
        CLWContext m_context;
    };
}
