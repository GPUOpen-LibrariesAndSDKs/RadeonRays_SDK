
/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

#ifndef RENDERER_H
#define RENDERER_H

#include "../FireRays/math/mathutils.h"

class Scene;

class Output
{
public:
    Output(int w, int h)
    : width_(w)
    , height_(h)
    {
    }
    
    virtual ~Output(){}
    virtual void GetData(FireRays::float3* data) const = 0;
    
    int width() const { return width_; }
    int height() const { return height_; }
    
private:
    int width_;
    int height_;
};

///< Renderer interface
class Renderer
{
public:
    // Destructor
    virtual ~Renderer(){}
    
    // Create output
    virtual Output* CreateOutput(int w, int h) = 0;
    
    // Delete output
    virtual void DeleteOutput(Output* output) = 0;
    
    // Clear output
    virtual void Clear(FireRays::float3 const& val, Output& output) = 0;
    
    // Do necessary precalculation and initialization
    virtual void Preprocess(Scene const& scene) = 0;
    
    // Render the scene into the output
    virtual void Render(Scene const& scene) = 0;

    // Render the scene occlusion into the output
    virtual void RenderAmbientOcclusion(Scene const& scene, float radius) = 0;
    
    // Set output
    virtual void SetOutput(Output* output) = 0;
};


#endif // RENDERER_H
