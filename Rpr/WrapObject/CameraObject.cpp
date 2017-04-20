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

#include "WrapObject/CameraObject.h"
#include "SceneGraph/camera.h"
#include "radeon_rays.h"

using namespace Baikal;
using namespace RadeonRays;

CameraObject::CameraObject()
    : m_cam(nullptr)
{

    //create perspective camera by default
    //set cam default properties
    RadeonRays::float3 const eye = { 0.f, 0.f, 0.f };
    RadeonRays::float3 const at = { 0.f , 0.f , -1.f };
    RadeonRays::float3 const up = { 0.f , 1.f , 0.f };
    PerspectiveCamera* camera = new PerspectiveCamera(eye, at, up);

    float2 camera_sensor_size = float2(0.036f, 0.024f);  // default full frame sensor 36x24 mm
    float2 camera_zcap = float2(0.0f, 100000.f);
    float camera_focal_length = 0.035f; // 35mm lens
    float camera_focus_distance = 1.f;
    float camera_aperture = 0.f;

    camera->SetSensorSize(camera_sensor_size);
    camera->SetDepthRange(camera_zcap);
    camera->SetFocalLength(camera_focal_length);
    camera->SetFocusDistance(camera_focus_distance);
    camera->SetAperture(camera_aperture);
    m_cam = camera;
}

CameraObject::~CameraObject()
{
    delete m_cam;
    m_cam = nullptr;
}

void CameraObject::SetTransform(const RadeonRays::matrix& m)
{
	//default values
	RadeonRays::float3 eye(0.f, 0.f, 0.f);
	RadeonRays::float3 at(0.f, 0.f, -1.f);
	RadeonRays::float3 up(0.f, 1.f, 0.f);
	eye = m * eye;
	at = m * at;
	up = m * up;
	m_cam->LookAt(eye, at, up);
}

void CameraObject::GetLookAt(RadeonRays::float3& eye,
    RadeonRays::float3& at,
    RadeonRays::float3& up)
{
    eye = m_cam->GetPosition();
    at = m_cam->GetForwardVector() + eye;
    up = m_cam->GetUpVector();
}