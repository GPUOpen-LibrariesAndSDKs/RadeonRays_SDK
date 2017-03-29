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

#include "WrapObject.h"
#include "math/matrix.h"
#include "math/float2.h"
#include "math/float3.h"
#include "RadeonProRender.h"

namespace Baikal
{
    class Light;
}

class MaterialObject;

class LightObject
    : public WrapObject
{
public:
    enum class Type
    {
        kPointLight = RPR_LIGHT_TYPE_POINT,
        kSpotLight = RPR_LIGHT_TYPE_SPOT,
        kDirectionalLight = RPR_LIGHT_TYPE_DIRECTIONAL,
        kEnvironmentLight = RPR_LIGHT_TYPE_ENVIRONMENT,
    };
    LightObject(Type type);
    virtual ~LightObject();

    Type GetType() const { return m_type; };
    Baikal::Light* GetLight() const { return m_light; };
    
    //light methods
    void SetTransform(const RadeonRays::matrix& transform);
    RadeonRays::matrix GetTransform();
    
    void SetRadiantPower(const RadeonRays::float3& p);
    RadeonRays::float3 GetRadiantPower();

    //spot light methods
    void SetSpotConeShape(const RadeonRays::float2& cone);
    RadeonRays::float2 GetSpotConeShape();

    //env light
    void SetEnvTexture(MaterialObject* img);
    MaterialObject* GetEnvTexture();
    void SetEnvMultiplier(rpr_float mult);
    rpr_float GetEnvMultiplier();
private:
    Baikal::Light* m_light;
    Type m_type;
    //only for GetEnvTexture() method
    MaterialObject* m_env_tex;
};