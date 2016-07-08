#include "Light/Ibl.h"

#include "math/mathutils.h"

#include "OpenImageIO/imageio.h"

#define _USE_MATH_DEFINES
#include <math.h>

namespace Baikal
{
    void Ibl::CalculateDistributions()
    {
        // Calculate intensities
        for (auto y = 0U; y < m_height; ++y)
            for (auto x = 0U; x < m_width; ++x)
            {
                auto sin_theta = sinf((float)M_PI * float(y + .5f) / m_height);
                auto val = m_data[y * m_width + x];
                m_img[y * m_width + x] = (0.2126f * val.x + 0.7152f * val.y + 0.0722f * val.z) * sin_theta;
            }
        
        // Calculate U-direction CDFs
        for (auto y = 0U; y < m_height; ++y)
        {
            // Start with 0
            m_u_dist[y * (m_width + 1)] = 0.f;
            
            // CDF (i) = CDF(i-1) + f(x) / n * c
            for (auto x = 1U; x < m_width + 1; ++x)
            {
                m_u_dist[y * (m_width + 1) + x] = m_u_dist[y * (m_width + 1) + x - 1] + m_img[y * m_width + x - 1] / m_width;
            }
            
            // Save ints
            m_u_int[y] = m_u_dist[y * (m_width + 1) + m_width];
            
            // Normalize
            for (auto x = 1U; x < m_width + 1; ++x)
            {
                m_u_dist[y * (m_width + 1) + x] /= m_u_int[y];
            }
        }
        
        // Start with 0
        m_v_dist[0] = 0.f;
        
        // CDF (i) = CDF(i-1) + f(x) / n * c
        for (auto y = 1U; y < m_height + 1; ++y)
        {
            m_v_dist[y] = m_v_dist[y - 1] + m_u_int[y - 1] / m_height;
        }
        
        // Save integral
        m_v_int = m_v_dist[m_height];
        
        // Normalize
        for (auto y = 0U; y < m_height + 1; ++y)
        {
            m_v_dist[y] /= m_v_int;
        }
    }
    
    float Ibl::GetPdf(FireRays::float2 uv) const
    {
        //auto v_iter = std::lower_bound(m_v_dist.cbegin(), m_v_dist.cend(), uv.y) - 1;
        
        //auto v_offset = std::distance(m_v_dist.cbegin(), v_iter);
        
        auto v_offset = FireRays::clamp((std::uint32_t)(uv.y * m_height), 0U, m_height - 1);
        
        float v_pdf = m_u_int[v_offset] / m_v_int;
        
        //auto u_iter = std::lower_bound(m_u_dist.cbegin() + v_offset * (m_width + 1),
        //m_u_dist.cbegin() + (v_offset + 1) * (m_width + 1), uv.x) - 1;
        
        auto u_offset = FireRays::clamp((std::uint32_t)(uv.x * m_width), 0U, m_width - 1);
        //std::distance(m_u_dist.cbegin() + v_offset * (m_width + 1), u_iter);
        
        
        float u_pdf = m_img[v_offset * m_width + u_offset] / m_u_int[v_offset];
        
        return u_pdf * v_pdf;
    }
    
    FireRays::int2 Ibl::SampleCoord(FireRays::float2 uv) const
    {
        auto v_iter = std::lower_bound(m_v_dist.cbegin(), m_v_dist.cend(), uv.y) - 1;
        
        auto v_offset = std::distance(m_v_dist.cbegin(), v_iter);
        
        v_offset = FireRays::clamp(v_offset, 0U, m_height - 1);
        
        auto u_iter = std::lower_bound(m_u_dist.cbegin() + v_offset * (m_width + 1),
                                       m_u_dist.cbegin() + (v_offset + 1) * (m_width + 1), uv.x) - 1;
        
        auto u_offset = std::distance(m_u_dist.cbegin() + v_offset * (m_width + 1), u_iter);
        
        u_offset = FireRays::clamp(u_offset, 0U, m_width - 1);
    
        return FireRays::int2(u_offset, v_offset);
    }
    
    void Ibl::DumpPdf(std::string const& filename)
    {
        auto width = 2000U;
        auto height = 1000U;
        
        
        std::vector<float> data(width * height);
        
        for (auto x = 0U; x < width; ++x)
            for (auto y = 0U; y < height; ++y)
            {
                data[y * width + x] = GetPdf(FireRays::float2((float)x / m_width, (float)y / m_height));
            }
        
        auto max = std::max_element(data.cbegin(), data.cend());
        
        std::for_each(data.begin(), data.end(), [=](float& v)
                      {
                          v /= *max;
                      });
        
        using namespace OpenImageIO;
        
        ImageOutput* out = ImageOutput::create(filename);
        
        if (!out)
        {
            throw std::runtime_error("Can't create image file on disk");
        }
        
        ImageSpec spec(width, height, 1, TypeDesc::UINT8);
        
        out->open(filename, spec);
        out->write_image(TypeDesc::FLOAT, &data[0]);
        out->close();
        
        delete out;
    }
    
    void Ibl::Simulate(std::string const& filename)
    {
        auto width = 2000U;
        auto height = 1000U;
        
        
        std::vector<float> data(width * height);
        std::fill(data.begin(), data.end(), 0.f);
        
        for (int i = 0; i < 100000; ++i)
        {
            auto xy = SampleCoord(FireRays::float2(FireRays::rand_float(), FireRays::rand_float()));
            data[xy.y * width + xy.x] += 1.f;
        }
        
        using namespace OpenImageIO;
        
        ImageOutput* out = ImageOutput::create(filename);
        
        if (!out)
        {
            throw std::runtime_error("Can't create image file on disk");
        }
        
        ImageSpec spec(width, height, 1, TypeDesc::UINT8);
        
        out->open(filename, spec);
        out->write_image(TypeDesc::FLOAT, &data[0]);
        out->close();
        
        delete out;
    }
}