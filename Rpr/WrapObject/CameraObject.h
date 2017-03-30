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
#include "SceneGraph/camera.h"

#include "math/matrix.h"
#include "RadeonProRender.h"

namespace Baikal {
    class PerspectiveCamera;
}

//this class represent rpr_camera
class CameraObject
    : public WrapObject
{
public:
    CameraObject();
    virtual ~CameraObject();

    //camera data
    //Note: some values are converted between meters and mm
    void SetFocalLength(rpr_float flen) { m_cam->SetFocalLength(flen / 1000.f); }
    rpr_float GetFocalLength() { return m_cam->GetFocalLength() * 1000.f; }

    void SetFocusDistance(rpr_float fdist) { m_cam->SetFocusDistance(fdist); }
    rpr_float GetFocusDistance() { return m_cam->GetFocusDistance(); }

    void SetSensorSize(RadeonRays::float2 size) { m_cam->SetSensorSize(size * 0.001f); }
    RadeonRays::float2 GetSensorSize() { return m_cam->GetSensorSize() * 1000.f; }

    void GetLookAt(RadeonRays::float3& eye,
                RadeonRays::float3& at,
                RadeonRays::float3& up);
    void LookAt(RadeonRays::float3 const& eye,
        RadeonRays::float3 const& at,
        RadeonRays::float3 const& up) { m_cam->LookAt(eye, at, up); };

    void SetAperture(rpr_float fstop) { m_cam->SetAperture(fstop / 1000.f); }
    rpr_float GetAperture() { return m_cam->GetAperture() * 1000.f; }

    void SetTransform(const RadeonRays::matrix& m);

    Baikal::Camera* GetCamera() { return m_cam; }
private:
    Baikal::PerspectiveCamera* m_cam;
};