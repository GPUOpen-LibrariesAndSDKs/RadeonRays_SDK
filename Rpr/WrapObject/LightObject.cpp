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
#include "WrapObject/LightObject.h"
#include "WrapObject/MaterialObject.h"
#include "WrapObject/Exception.h"
#include "radeon_rays.h"
#include "App/SceneGraph/light.h"
#include "RadeonProRender.h"

void LightObject::SetTransform(const RadeonRays::matrix& t)
{
    m_light->SetPosition(RadeonRays::float3(t.m03, t.m13, t.m23, t.m33));
    //float3(0, 0, -1) is a default direction vector
    m_light->SetDirection(t * RadeonRays::float3(0, 0, -1));
}

RadeonRays::matrix LightObject::GetTransform()
{
    RadeonRays::float3 pos = m_light->GetPosition();
    RadeonRays::float3 dir = m_light->GetDirection();
    //angle between -z axis and direction
    float angle = acos(dot(dir, RadeonRays::float3(0.f, 0.f, -1.f, 0.f)));
    RadeonRays::float3 axis = cross(dir, RadeonRays::float3(0.f, 0.f, -1.f, 0.f));
    if (axis.sqnorm() < std::numeric_limits<float>::min())
    {
        //rotate around x axis
        axis = RadeonRays::float3(1.f, 0.f, 0.f, 0.f);
    }
    RadeonRays::matrix rot = rotation(axis, angle);
    RadeonRays::matrix m = translation(pos) * rot;
    return m;
}

LightObject::LightObject(Type type)
    : m_type(type)
    , m_light(nullptr)
    , m_env_tex(nullptr)
{
    switch (type)
    {
    case Type::kPointLight:
        m_light = new Baikal::PointLight();
        break;
    case Type::kSpotLight:
        m_light = new Baikal::SpotLight();
        break;
    case Type::kDirectionalLight:
        m_light = new Baikal::DirectionalLight();
        break;
    case Type::kEnvironmentLight:
        m_light = new Baikal::ImageBasedLight();
        break; 
    default:
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "LightObject: unexpected light type");
    }
}

LightObject::~LightObject()
{
    delete m_light;
    m_light = nullptr;
}

void LightObject::SetRadiantPower(const RadeonRays::float3& p)
{
    m_light->SetEmittedRadiance(p);
}

RadeonRays::float3 LightObject::GetRadiantPower()
{
    return m_light->GetEmittedRadiance();
}


void LightObject::SetSpotConeShape(const RadeonRays::float2& cone)
{
    Baikal::SpotLight* spot = dynamic_cast<Baikal::SpotLight*>(m_light);
    spot->SetConeShape(cone);
}

RadeonRays::float2 LightObject::GetSpotConeShape()
{
    Baikal::SpotLight* spot = dynamic_cast<Baikal::SpotLight*>(m_light);
    return spot->GetConeShape();
}


void LightObject::SetEnvTexture(MaterialObject* img)
{
    Baikal::ImageBasedLight* ibl = dynamic_cast<Baikal::ImageBasedLight*>(m_light);
    ibl->SetTexture(img->GetTexture());
    m_env_tex = img;
}

MaterialObject* LightObject::GetEnvTexture()
{
    return m_env_tex;
}


void LightObject::SetEnvMultiplier(rpr_float mult)
{
    Baikal::ImageBasedLight* ibl = dynamic_cast<Baikal::ImageBasedLight*>(m_light);
    ibl->SetMultiplier(mult);
}

rpr_float LightObject::GetEnvMultiplier()
{
    Baikal::ImageBasedLight* ibl = dynamic_cast<Baikal::ImageBasedLight*>(m_light);
    return ibl->GetMultiplier();
}

