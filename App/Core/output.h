#pragma once

#include "math/float3.h"

#include <cstdint>

namespace Baikal
{
    class Output
    {
    public:
        Output(std::uint32_t w, std::uint32_t h)
        : m_width(w)
        , m_height(h)
        {
        }

        virtual ~Output() = default;

        virtual void GetData(FireRays::float3* data) const = 0;

        std::uint32_t width() const { return m_width; }
        std::uint32_t height() const { return m_height; }

    private:
        std::uint32_t m_width;
        std::uint32_t m_height;
    };
}
