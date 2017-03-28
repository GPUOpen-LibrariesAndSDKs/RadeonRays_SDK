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
 \file image_io.h
 \author Dmitry Kozlov
 \version 1.0
 \brief
 */
#pragma once

#include <string>

namespace Baikal
{
    class Texture;
    
    /**
     \brief Interface for image loading and writing
     
     ImageIO is responsible for texture loading from disk and keeping track of image reuse.
     */
    class ImageIo
    {
    public:
        // Create default image IO
        static ImageIo* CreateImageIo();
        
        // Constructor
        ImageIo() = default;
        // Destructor
        virtual ~ImageIo() = default;
        
        // Load texture from file
        virtual Texture* LoadImage(std::string const& filename) const = 0;
        virtual void SaveImage(std::string const& filename, Texture const* texture) const = 0;
        
        // Disallow copying
        ImageIo(ImageIo const&) = delete;
        ImageIo& operator = (ImageIo const&) = delete;
    };
    

}
