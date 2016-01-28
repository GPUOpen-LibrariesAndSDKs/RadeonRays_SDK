//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#ifndef RENDERER_H
#define RENDERER_H

#include "math/mathutils.h"

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
