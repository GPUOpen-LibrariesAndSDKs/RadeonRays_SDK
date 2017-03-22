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
 \file light.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains declaration of various light types supported by the renderer.
 */
#pragma once

#include "math/float3.h"
#include "math/float2.h"
#include <memory>
#include <string>
#include <set>

#include "iterator.h"

#include "scene_object.h"
#include "shape.h"

namespace Baikal
{
    class Texture;
    
    /**
     \brief Light base interface.
     
     High-level interface all light classes need to implement.
     */
    class Light : public SceneObject
    {
    public:
        // Constructor
        Light();
        // Destructor
        virtual ~Light() = 0;
        
        // Get total radiant power (integral)
        //virtual RadeonRays::float3 GetRadiantPower() const = 0;
        
        // Set and get position
        RadeonRays::float3 GetPosition() const;
        void SetPosition(RadeonRays::float3 const& p);
        
        // Set and get direction
        RadeonRays::float3 GetDirection() const;
        void SetDirection(RadeonRays::float3 const& d);
        
        // Set and get emitted radiance (differential)
        RadeonRays::float3 GetEmittedRadiance() const;
        void SetEmittedRadiance(RadeonRays::float3 const& e);
        
        // Iterator for all the textures used by the light
        virtual Iterator* CreateTextureIterator() const;
        
    private:
        // Position
        RadeonRays::float3 m_p;
        // Direction
        RadeonRays::float3 m_d;
        // Emmited radiance
        RadeonRays::float3 m_e;
    };
    
    inline Light::Light()
    : m_d(0.f, -1.f, 0.f)
    , m_e(1.f, 1.f, 1.f)
    {
    }
    
    inline Light::~Light()
    {
    }
    
    /**
     \brief Simple point light source.
     
     Non-physical light emitting power from a single point in all directions.
     */
    class PointLight: public Light
    {
    public:
    };
    
    /**
     \brief Simple directional light source.
     
     Non-physical light emitting power from a single direction.
     */
    class DirectionalLight: public Light
    {
    public:
    };
    
    /**
     \brief Simple cone-shaped light source.
     
     Non-physical light emitting power from a single point in a cone of directions.
     */
    class SpotLight: public Light
    {
    public:
        SpotLight();
        // Get and set inner and outer falloff angles: they are set as cosines of angles between light direction
        // and cone opening.
        void SetConeShape(RadeonRays::float2 angles);
        RadeonRays::float2 GetConeShape() const;
        
    private:
        // Opening angles (x - inner, y - outer)
        RadeonRays::float2 m_angles;
    };
    
    inline SpotLight::SpotLight()
    : m_angles(0.25f, 0.5f)
    {
    }
    
    /**
     \brief Image-based distant light source.
     
     Physical light emitting from all spherical directions.
     */
    class ImageBasedLight: public Light
    {
    public:
        ImageBasedLight();
        // Get and set illuminant texture
        void SetTexture(Texture const* texture);
        Texture const* GetTexture() const;
        
        // Get and set multiplier.
        // Multiplier is used to adjust emissive power.
        float GetMultiplier() const;
        void SetMultiplier(float m);
        
        // Iterator for all the textures used by the light
        Iterator* CreateTextureIterator() const override;
        
    private:
        // Illuminant texture
        Texture const* m_texture;
        // Emissive multiplier
        float m_multiplier;
    };
    
    // Area light
    class AreaLight: public Light
    {
    public:
        AreaLight(Shape const* shape, std::size_t idx);
        // Get parent shape
        Shape const* GetShape() const;
        // Get parent prim idx
        std::size_t GetPrimitiveIdx() const;
        
    private:
        // Parent shape
        Shape const* m_shape;
        // Parent primitive index
        std::size_t m_prim_idx;
    };

    inline AreaLight::AreaLight(Shape const* shape, std::size_t idx)
    : m_shape(shape)
    , m_prim_idx(idx)
    {
    }

    inline RadeonRays::float3 Light::GetPosition() const
    {
        return m_p;
    }
    
    inline void Light::SetPosition(RadeonRays::float3 const& p)
    {
        m_p = p;
        SetDirty(true);
    }
    
    inline RadeonRays::float3 Light::GetDirection() const
    {
        return m_d;
    }
    
    inline void Light::SetDirection(RadeonRays::float3 const& d)
    {
        m_d = normalize(d);
        SetDirty(true);
    }
    
    inline Iterator* Light::CreateTextureIterator() const
    {
        return new EmptyIterator();
    }
    
    inline RadeonRays::float3 Light::GetEmittedRadiance() const
    {
        return m_e;
    }
    
    inline void Light::SetEmittedRadiance(RadeonRays::float3 const& e)
    {
        m_e = e;
        SetDirty(true);
    }
    
    inline void SpotLight::SetConeShape(RadeonRays::float2 angles)
    {
        m_angles = angles;
        SetDirty(true);
    }
    
    inline RadeonRays::float2 SpotLight::GetConeShape() const
    {
        return m_angles;
    }
    
    inline void ImageBasedLight::SetTexture(Texture const* texture)
    {
        m_texture = texture;
        SetDirty(true);
    }
    
    inline Texture const* ImageBasedLight::GetTexture() const
    {
        return m_texture;
    }
    
    inline std::size_t AreaLight::GetPrimitiveIdx() const
    {
        return m_prim_idx;
    }
    
    inline Shape const* AreaLight::GetShape() const
    {
        return m_shape;
    }
    
    inline ImageBasedLight::ImageBasedLight()
    : m_texture(nullptr)
    , m_multiplier(1.f)
    {
    }
    
    inline float ImageBasedLight::GetMultiplier() const
    {
        return m_multiplier;
    }
    
    inline void ImageBasedLight::SetMultiplier(float m)
    {
        m_multiplier = m;
        SetDirty(true);
    }
    
    inline Iterator* ImageBasedLight::CreateTextureIterator() const
    {
        std::set<Texture const*> result;
        
        if (m_texture)
        {
            result.insert(m_texture);
        }
        
        return new ContainerIterator<std::set<Texture const*>>(std::move(result));
    }
}
