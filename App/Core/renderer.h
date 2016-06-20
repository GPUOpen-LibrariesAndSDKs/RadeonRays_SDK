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
#pragma once

#include "math/float3.h"
#include <cstdint>

namespace Baikal
{
    class Output;
    class Scene;

    ///< Renderer interface
    class Renderer
    {
    public:
        Renderer() = default;
        // Destructor
        virtual ~Renderer() = default;
        // Create output
        virtual Output* CreateOutput(std::uint32_t w, std::uint32_t h) const = 0;
        // Delete output
        virtual void DeleteOutput(Output* output) const = 0;
        // Clear output
        virtual void Clear(FireRays::float3 const& val, Output& output) const = 0;
        // Do necessary precalculation and initialization
        // TODO: is it really necessary? can be async? progress reporting?
        virtual void Preprocess(Scene const& scene) = 0;
        // Render single iteration
        virtual void Render(Scene const& scene) = 0;
        // Set output
        virtual void SetOutput(Output* output) = 0;

        // Does not make sense to copy it
        Renderer(Renderer const&) = delete;
        Renderer& operator = (Renderer const&) = delete;
    };
}
