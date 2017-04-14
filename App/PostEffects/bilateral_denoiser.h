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
#include "post_effect_clw.h"

#include "CLW.h"
#include "../CLW/clwoutput.h"

namespace Baikal
{
    /**
    \brief Simple bilateral denoiser.

    \details BilateralDenoiser does selective gaussian blur of pixels based
    on distance metric taking color, normal and world position into account. 
    Filter is not smoothing out normal maps and rapid normal changes. 
    Parameters:
        * radius - Filter radius in pixels
        * color_sensitivity - Higher the sensitivity the more it smoothes out depending on color difference.
        * normal_sensitivity - Higher the sensitivity the more it smoothes out depending on normal difference.
        * position_sensitivity - Higher the sensitivity the more it smoothes out depending on position difference.
    Required AOVs in input set:
        * kColor
        * kWorldShadingNormal
        * kWorldPosition
    */
    class BilateralDenoiser : public PostEffectClw
    {
    public:
        // Constructor
        BilateralDenoiser(CLWContext context);
        // Apply filter
        void Apply(InputSet const& input_set, Output& output) override;

    private: 
        // Find required output
        ClwOutput* FindOutput(InputSet const& input_set, Renderer::OutputType type);

        CLWProgram m_program;
    };

    inline BilateralDenoiser::BilateralDenoiser(CLWContext context)
        : PostEffectClw(context)
    {
        std::string buildopts;
        
        buildopts.append(" -cl-mad-enable -cl-fast-relaxed-math -cl-std=CL1.2 -I . ");
        
        buildopts.append(
#if defined(__APPLE__)
                         "-D APPLE "
#elif defined(_WIN32) || defined (WIN32)
                         "-D WIN32 "
#elif defined(__linux__)
                         "-D __linux__ "
#else
                         ""
#endif
                         );
        
        // Compile kernels
        m_program = CLWProgram::CreateFromFile("../App/CL/denoise.cl", buildopts.c_str(), GetContext());

        // Add necessary params
        RegisterParameter("radius", RadeonRays::float4(5.f, 0.f, 0.f, 0.f));
        RegisterParameter("color_sensitivity", RadeonRays::float4(5.f, 0.f, 0.f, 0.f));
        RegisterParameter("position_sensitivity", RadeonRays::float4(5.f, 0.f, 0.f, 0.f));
        RegisterParameter("normal_sensitivity", RadeonRays::float4(0.1f, 0.f, 0.f, 0.f));
        RegisterParameter("albedo_sensitivity", RadeonRays::float4(0.1f, 0.f, 0.f, 0.f));
    }

    inline ClwOutput* BilateralDenoiser::FindOutput(InputSet const& input_set, Renderer::OutputType type)
    {
        auto iter = input_set.find(type);

        if (iter == input_set.cend())
        {
            return nullptr;
        }

        return static_cast<ClwOutput*>(iter->second);
    }

    inline void BilateralDenoiser::Apply(InputSet const& input_set, Output& output)
    {
        auto radius = static_cast<std::uint32_t>(GetParameter("radius").x);
        auto sigma_color = GetParameter("color_sensitivity").x;
        auto sigma_position = GetParameter("position_sensitivity").x;
        auto sigma_normal = GetParameter("normal_sensitivity").x;
        auto sigma_albedo = GetParameter("albedo_sensitivity").x;

        auto color = FindOutput(input_set, Renderer::OutputType::kColor);
        auto normal = FindOutput(input_set, Renderer::OutputType::kWorldShadingNormal);
        auto position = FindOutput(input_set, Renderer::OutputType::kWorldPosition);
        auto albedo = FindOutput(input_set, Renderer::OutputType::kAlbedo);
        auto out_color = static_cast<ClwOutput*>(&output);

        auto denoise_kernel = m_program.GetKernel("BilateralDenoise_main");

        int num_pixels = color->width() * color->height();

        // Set kernel parameters
        int argc = 0;
        denoise_kernel.SetArg(argc++, color->data());
        denoise_kernel.SetArg(argc++, normal->data());
        denoise_kernel.SetArg(argc++, position->data());
        denoise_kernel.SetArg(argc++, albedo->data());
        denoise_kernel.SetArg(argc++, color->width());
        denoise_kernel.SetArg(argc++, color->height());
        denoise_kernel.SetArg(argc++, radius);
        denoise_kernel.SetArg(argc++, sigma_color);
        denoise_kernel.SetArg(argc++, sigma_normal);
        denoise_kernel.SetArg(argc++, sigma_position);
        denoise_kernel.SetArg(argc++, sigma_albedo);
        denoise_kernel.SetArg(argc++, out_color->data());

        // Run shading kernel
        {
            size_t gs[] = { static_cast<size_t>((output.width() + 7) / 8 * 8), static_cast<size_t>((output.height() + 7) / 8 * 8) };
            size_t ls[] = { 8, 8 };

            GetContext().Launch2D(0, gs, ls, denoise_kernel);
        }
    }

}
