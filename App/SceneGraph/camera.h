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
 \file camera.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains declaration of camera types supported by the renderer.
 */
#pragma once

#include "math/float3.h"
#include "math/float2.h"

#include "scene_object.h"

namespace Baikal
{
    class Camera : public SceneObject
    {
    public:
        Camera() = default;
        virtual ~Camera() = default;
    };
    
    class PerspectiveCamera : public Camera
    {
    public:
        // Pass camera position, camera aim, camera up vector, depth limits, vertical field of view
        // and image plane aspect ratio
        PerspectiveCamera(RadeonRays::float3 const& eye,
                          RadeonRays::float3 const& at,
                          RadeonRays::float3 const& up);
        
        // Pass camera position, camera aim, camera up vector
        void LookAt(RadeonRays::float3 const& eye,
                    RadeonRays::float3 const& at,
                    RadeonRays::float3 const& up);
        // Set camera focus distance in meters,
        // this is essentially a distance from the lens to the focal plane.
        // Altering this is similar to rotating the focus ring on real lens.
        void SetFocusDistance(float distance);
        float GetFocusDistance() const;
        
        // Set camera focal length in meters,
        // this is essentially a distance between a camera sensor and a lens.
        // Altering this is similar to rotating zoom ring on a lens.
        void SetFocalLength(float length);
        float GetFocalLength() const;
        
        // Set aperture value in meters.
        // This is a radius of a lens.
        void SetAperture(float aperture);
        float GetAperture() const;
        
        // Set camera sensor size in meters.
        // This distinguishes APC-S vs full-frame, etc
        void SetSensorSize(RadeonRays::float2 const& size);
        RadeonRays::float2 GetSensorSize() const;
        
        // Set camera depth range.
        // Does not really make sence for physical camera
        void SetDepthRange(RadeonRays::float2 const& range);
        RadeonRays::float2 GetDepthRange() const;
        
        RadeonRays::float3 GetForwardVector() const;
        RadeonRays::float3 GetUpVector() const;
        RadeonRays::float3 GetRightVector() const;
        RadeonRays::float3 GetPosition() const;
        float GetAspectRatio() const;
        
        
        // Rotate camera around world Z axis
        void Rotate(float angle);
        // Tilt camera
        void Tilt(float angle);
        // Move along camera Z direction
        void MoveForward(float distance);
        // Move along camera X direction
        void MoveRight(float distance);
        // Move along camera Y direction
        void MoveUp(float distance);
        
        //
        void ArcballRotateHorizontally(RadeonRays::float3 c, float angle);
        //
        void ArcballRotateVertically(RadeonRays::float3 c, float angle);
        
        
    private:
        // Rotate camera around world Z axis
        void Rotate(RadeonRays::float3, float angle);
        
        // Camera coordinate frame
        RadeonRays::float3 m_forward;
        RadeonRays::float3 m_right;
        RadeonRays::float3 m_up;
        RadeonRays::float3 m_p;
        
        // Image plane width & hight in scene units
        RadeonRays::float2 m_dim;
        
        // Near and far Z
        RadeonRays::float2 m_zcap;
        
        float  m_focal_length;
        float  m_aspect;
        float  m_focus_distance;
        float  m_aperture;
        
        friend std::ostream& operator << (std::ostream& o, PerspectiveCamera const& p);
    };
    
    inline void PerspectiveCamera::SetFocusDistance(float distance)
    {
        m_focus_distance = distance;
        SetDirty(true);
    }
    
    inline void PerspectiveCamera::SetFocalLength(float length)
    {
        m_focal_length = length;
        SetDirty(true);
    }
    
    inline void PerspectiveCamera::SetAperture(float aperture)
    {
        m_aperture = aperture;
        SetDirty(true);
    }
    
    inline float PerspectiveCamera::GetFocusDistance() const
    {
        return m_focus_distance;
    }
    
    inline float PerspectiveCamera::GetFocalLength() const
    {
        return m_focal_length;
    }
    
    inline float PerspectiveCamera::GetAperture() const
    {
        return m_aperture;
    }
    
    inline RadeonRays::float2 PerspectiveCamera::GetSensorSize() const
    {
        return m_dim;
    }
    
    inline void PerspectiveCamera::SetSensorSize(RadeonRays::float2 const& size)
    {
        m_dim = size;
        SetDirty(true);
    }
    
    inline void PerspectiveCamera::SetDepthRange(RadeonRays::float2 const& range)
    {
        m_zcap = range;
        SetDirty(true);
    }
    
    inline RadeonRays::float2 PerspectiveCamera::GetDepthRange() const
    {
        return m_zcap;
    }
    
}
