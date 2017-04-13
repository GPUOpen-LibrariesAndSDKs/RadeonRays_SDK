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

#include "Core/renderer.h"
#include "Core/output.h"

#include <map>
#include <string>
#include <stdexcept>
#include <cassert>

namespace Baikal
{
    /**
    \brief Interface for post-processing effects.

    \details Post processing effects are operating on 2D renderer outputs and 
    do not know anything on rendreing methonds or renderer internal implementation.
    However not all Outputs are compatible with a given post-processing effect, 
    it is done to optimize the implementation and forbid running say OpenCL 
    post-processing effects on Vulkan output. Post processing effects may have 
    scalar parameters and may rely on the presense of certain content type (like 
    normals or diffuse albedo) in a given input set.
    */
    class PostEffect
    {
    public:
        // Data type to pass all necessary content into the post effect. 
        using InputSet = std::map<Renderer::OutputType, Output*>;

        // Default constructor & destructor
        PostEffect() = default;
        virtual ~PostEffect() = default;

        // Check if the output is compatible with this effect
        virtual bool IsCompatible(Output const& output) const = 0;

        // Apply post effect and use output for the result
        virtual void Apply(InputSet const& input_set, Output& output) = 0;

        // Set scalar parameter
        void SetParameter(std::string const& name, RadeonRays::float4 const& value);

        // Get scalar parameter
        RadeonRays::float4 GetParameter(std::string const& name) const;

    protected:
        // Adds scalar parameter into the parameter map
        void RegisterParameter(std::string const& name, RadeonRays::float4 const& initial_value);

    private:
        // Parameter map
        std::map<std::string, RadeonRays::float4> m_parameters;
    };

    inline void PostEffect::SetParameter(std::string const& name, RadeonRays::float4 const& value)
    {
        auto iter = m_parameters.find(name);

        if (iter == m_parameters.cend())
        {
            throw std::runtime_error("PostEffect: no such parameter " + name);
        }

        iter->second = value;
    }

    inline RadeonRays::float4 PostEffect::GetParameter(std::string const& name) const
    {
        auto iter = m_parameters.find(name);

        if (iter == m_parameters.cend())
        {
            throw std::runtime_error("PostEffect: no such parameter " + name);
        }

        return iter->second;
    }

    inline void PostEffect::RegisterParameter(std::string const& name, RadeonRays::float4 const& initial_value)
    {
        auto iter = m_parameters.find(name);

        assert(iter == m_parameters.cend());

        m_parameters.emplace(name, initial_value);
    }
}
