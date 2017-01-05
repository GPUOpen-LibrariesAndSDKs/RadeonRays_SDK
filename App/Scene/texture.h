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
 \file texture.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains declaration of a texture class.
 */
#pragma once

#include "math/float3.h"
#include "math/float2.h"
#include "math/int2.h"
#include <memory>
#include <string>

namespace Baikal
{
    class Material;
    
    /**
     \brief Texture class.
     
     Texture is used to host CPU memory for image data.
     */
    class Texture
    {
    public:
        enum class Format
        {
            kRgba8,
            kRgba16,
            kRgba32
        };
        
        Texture() = default;
        virtual ~Texture() = 0;
        
        virtual RadeonRays::int2 GetSize() const = 0;
        virtual char const* GetData() const = 0;
        virtual Format GetFormat() const = 0;
        virtual std::size_t GetSizeInBytes() const = 0;
        
        Texture(Texture const&) = delete;
        Texture& operator = (Texture const&) = delete;
        
    private:
        
    };
    
    inline Texture::~Texture()
    {
        
    }
}
