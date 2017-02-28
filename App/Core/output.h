/**********************************************************************
 Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ********************************************************************/

/**
 \file output.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains declaration of Baikal::Output class, core interface representing renderer output surface.
 */

#pragma once

#include "math/float3.h"

#include <cstdint>

namespace Baikal
{
    
    /**
     \brief Interface for rendering output surface.
     
     Represents discrete 2D surface with [0..w]x[0..h] coordinate ranges.
     */
    class Output
    {
    public:
        /**
         \brief Create output of a given size
         
         \param w Output surface width
         \param h Output surface height
         */
        Output(std::uint32_t w, std::uint32_t h)
        : m_width(w)
        , m_height(h)
        {
        }

        virtual ~Output() = default;

        /**
         \brief Interface for rendering output.
         
         Represents discrete 2D surface with [0..w]x[0..h] coordinate ranges.
         */
        virtual void GetData(RadeonRays::float3* data) const = 0;

        // Get surface width
        std::uint32_t width() const;
        // Get surface height
        std::uint32_t height() const;

    private:
        // Surface width
        std::uint32_t m_width;
        // Surface height
        std::uint32_t m_height;
    };
    
    inline std::uint32_t Output::width() const { return m_width; }
    inline std::uint32_t Output::height() const { return m_height; }
}
