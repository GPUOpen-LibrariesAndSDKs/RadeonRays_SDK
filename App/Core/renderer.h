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
 \file renderer.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains declaration of Baikal::Renderer class, core interface representing representing the renderer.
 */
#pragma once

#include "math/float3.h"
#include "math/int2.h"
#include <cstdint>

namespace Baikal
{
    class Output;
    class Scene1;

    /**
     \brief Interface for the renderer.
     
     Renderer implemenation is taking the scene and producing its image into an output surface.
     */
    class Renderer
    {
    public:
        Renderer() = default;
        virtual ~Renderer() = default;
        
        /**
         \brief Create output of a given size.
         
         \param w Output surface width
         \param h Output surface height
         */
        virtual Output* CreateOutput(std::uint32_t w, std::uint32_t h) const = 0;
        
        /**
         \brief Delete given output.
         
         \param output The output to delete
         */
        virtual void DeleteOutput(Output* output) const = 0;
        
        /**
         \brief Clear output surface using given value.
         
         \param val Value to clear to
         \param output Output to clear
         */
        virtual void Clear(RadeonRays::float3 const& val, Output& output) const = 0;

        /**
         \brief Do necessary scene dependent computations and caching.
         
         \param scene The scene to process
         */
        virtual void Preprocess(Scene1 const& scene) = 0;

        /**
         \brief Render single iteration.
         
         \param scene Scene to render
         */
        virtual void Render(Scene1 const& scene) = 0;

        /**
        \brief Render single iteration.

        \param scene Scene to render
        */
        //virtual void RenderTile(Scene1 const& scene, int2 const& tile_origin, int2 const& tile_size){}

        /**
         \brief Set the output for rendering.
         
         \param output The output to render into.
         */
        virtual void SetOutput(Output* output) = 0;

        /**
            Disallow copies and moves.
         */
        Renderer(Renderer const&) = delete;
        Renderer& operator = (Renderer const&) = delete;

        // Temporary functionality
        struct BenchmarkStats
        {
            std::uint32_t num_passes;
            
            RadeonRays::int2 resolution;
            float primary_rays_time_in_ms;
            float secondary_rays_time_in_ms;
            float shadow_rays_time_in_ms;
        };
        
        virtual void RunBenchmark(Scene1 const& scene, std::uint32_t num_passes, BenchmarkStats& stats) {}
    };
}
