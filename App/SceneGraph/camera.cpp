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
#include "camera.h"

#include <cmath>
#include <cassert>

#include "math/quaternion.h"
#include "math/matrix.h"
#include "math/mathutils.h"


namespace Baikal
{
    using namespace RadeonRays;
    
    PerspectiveCamera::PerspectiveCamera(float3 const& eye, float3 const& at, float3 const& up)
    : m_p(eye)
    , m_aperture(0.f)
    , m_focus_distance(0.f)
    , m_focal_length(0.f)
    , m_zcap(0.f, 0.f)
	, m_aspect(0.f)
    {
        // Construct camera frame
        LookAt(eye, at, up);
    }

    void PerspectiveCamera::LookAt(RadeonRays::float3 const& eye,
                                    RadeonRays::float3 const& at,
                                    RadeonRays::float3 const& up)
    {
        m_p = eye;
        m_forward = normalize(at - eye);
        m_right = cross(m_forward, normalize(up));
        m_up = cross(m_right, m_forward);
        SetDirty(true);
    }
    
    // Rotate camera around world Z axis, use for FPS camera
    void PerspectiveCamera::Rotate(float angle)
    {
        Rotate(float3(0.f, 1.f, 0.f), angle);
        SetDirty(true);
    }
    
    void PerspectiveCamera::Rotate(float3 v, float angle)
    {
        /// matrix should have basis vectors in rows
        /// to be used for quaternion construction
        /// would be good to add several options
        /// to quaternion class
        matrix cam_matrix = matrix(
                                   m_up.x, m_up.y, m_up.z, 0.f,
                                   m_right.x, m_right.y, m_right.z, 0.f,
                                   m_forward.x, m_forward.y, m_forward.z, 0.f,
                                   0.f, 0.f, 0.f, 1.f);
        
        // Create camera orientation quaternion
        quaternion q = normalize(quaternion(cam_matrix));
        
        // Rotate camera frame around v
        q = q * rotation_quaternion(v, -angle);
        
        // Uncomress back to lookat
        q.to_matrix(cam_matrix);
        
        m_up = normalize(float3(cam_matrix.m00, cam_matrix.m01, cam_matrix.m02));
        m_right = normalize(float3(cam_matrix.m10, cam_matrix.m11, cam_matrix.m12));
        m_forward = normalize(float3(cam_matrix.m20, cam_matrix.m21, cam_matrix.m22));
        SetDirty(true);
    }
    
    // Tilt camera
    void PerspectiveCamera::Tilt(float angle)
    {
        Rotate(m_right, angle);
        SetDirty(true);
    }
    
    // Move along camera Z direction
    void PerspectiveCamera::MoveForward(float distance)
    {
        m_p += distance * m_forward;
        SetDirty(true);
    }
    
    // Move along camera X direction
    void PerspectiveCamera::MoveRight(float distance)
    {
        m_p += distance * m_right;
        SetDirty(true);
    }
    
    // Move along camera Y direction
    void PerspectiveCamera::MoveUp(float distance)
    {
        m_p += distance * m_up;
        SetDirty(true);
    }
    
    RadeonRays::float3 PerspectiveCamera::GetForwardVector() const
    {
        return m_forward;
    }
    
    RadeonRays::float3 PerspectiveCamera::GetUpVector() const
    {
        return m_up;
    }
    
    RadeonRays::float3 PerspectiveCamera::GetRightVector() const
    {
        return m_right;
    }
    
    RadeonRays::float3 PerspectiveCamera::GetPosition() const
    {
        return m_p;
    }
    
    float PerspectiveCamera::GetAspectRatio() const
    {
        return m_aspect;
    }
    
    // Arcball rotation
    void PerspectiveCamera::ArcballRotateHorizontally(float3 c, float angle)
    {
        // Build camera matrix
        matrix cam_matrix = matrix(
                                   m_up.x, m_up.y, m_up.z, 0,
                                   m_right.x, m_right.y, m_right.z, 0,
                                   m_forward.x, m_forward.y, m_forward.z, 0,
                                   0, 0, 0, 1);
        
        // Create camera orientation quaternion
        quaternion q = normalize(quaternion(cam_matrix));
        
        // Rotation of camera around the center of interest by a == rotation of the frame by -a and rotation of the position by a
        q = q * rotation_quaternion(float3(0.f, 1.f, 0.f), -angle);
        
        // Rotate the frame
        q.to_matrix(cam_matrix);
        
        m_up = normalize(float3(cam_matrix.m00, cam_matrix.m01, cam_matrix.m02));
        m_right = normalize(float3(cam_matrix.m10, cam_matrix.m11, cam_matrix.m12));
        m_forward = normalize(float3(cam_matrix.m20, cam_matrix.m21, cam_matrix.m22));
        
        // Rotate the position
        float3 dir = c - m_p;
        dir = rotate_vector(dir, rotation_quaternion(float3(0.f, 1.f, 0.f), angle));
        
        m_p = c - dir;
        SetDirty(true);
    }
    
    void PerspectiveCamera::ArcballRotateVertically(float3 c, float angle)
    {
        // Build camera matrix
        matrix cam_matrix = matrix(
                                   m_up.x, m_up.y, m_up.z, 0,
                                   m_right.x, m_right.y, m_right.z, 0,
                                   m_forward.x, m_forward.y, m_forward.z, 0,
                                   0, 0, 0, 1);
        
        // Create camera orientation quaternion
        quaternion q = normalize(quaternion(cam_matrix));
        
        // Rotation of camera around the center of interest by a == rotation of the frame by -a and rotation of the position by a
        q = q * rotation_quaternion(m_right, -angle);
        
        // Rotate the frame
        q.to_matrix(cam_matrix);
        
        m_up = normalize(float3(cam_matrix.m00, cam_matrix.m01, cam_matrix.m02));
        m_right = normalize(float3(cam_matrix.m10, cam_matrix.m11, cam_matrix.m12));
        m_forward = normalize(float3(cam_matrix.m20, cam_matrix.m21, cam_matrix.m22));
        
        // Rotate the position
        float3 dir = c - m_p;
        dir = rotate_vector(dir, rotation_quaternion(m_right, angle));
        
        m_p = c - dir;
        SetDirty(true);
    }

    matrix PerspectiveCamera::GetViewMatrix() const
    {
        matrix cam_matrix = matrix(
            m_right.x, m_right.y, m_right.z, 0,
            m_up.x, m_up.y, m_up.z, 0,
            -m_forward.x, -m_forward.y, -m_forward.z, 0,
            0, 0, 0, 1);

        cam_matrix.m03 = -dot(m_right, m_p);
        cam_matrix.m13 = -dot(m_up, m_p);
        cam_matrix.m23 = -dot(-m_forward, m_p);
        return cam_matrix.transpose();
    }

    matrix PerspectiveCamera::GetProjectionMatrix() const
    {
        auto aspect = m_dim.x / m_dim.y;
        auto fovy = 2.f * atan2(m_dim.y * 0.5f, m_focal_length);
        return perspective_proj_fovy_lh_gl(fovy, aspect, m_focal_length, m_zcap.y).transpose();
    }
}
