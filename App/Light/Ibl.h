#pragma once 

#include "math/float3.h"
#include "math/float2.h"
#include "math/int2.h"

#include <vector>
#include <cstdint>
#include <string>

namespace Baikal
{
    class Ibl
    {
    public:
        Ibl(RadeonRays::float3 const* data, std::uint32_t w, std::uint32_t h)
        : m_data(data)
        , m_width(w)
        , m_height(h)
        , m_u_dist((m_width + 1) * m_height)
        , m_u_int(m_height + 1)
        , m_v_dist(m_height + 1)
        , m_img(m_width * m_height)
        {
            CalculateDistributions();
        }
        
        std::uint32_t width() const { return m_width; }
        std::uint32_t height() const { return m_height; }
        
        float GetPdf(RadeonRays::float2 uv) const;
        
        
        
        // This is temporary code
        RadeonRays::int2 SampleCoord(RadeonRays::float2 uv) const;
        void DumpPdf(std::string const& filename);
        void Simulate(std::string const& filename);
        
        
        
    protected:
        void CalculateDistributions();
        
    private:
        std::uint32_t m_width;
        std::uint32_t m_height;
        
        std::vector<float> m_v_dist;
        std::vector<float> m_u_dist;
        std::vector<float> m_u_int;
        float m_v_int;
        std::vector<float> m_img;
        RadeonRays::float3 const* m_data;
    };
}
