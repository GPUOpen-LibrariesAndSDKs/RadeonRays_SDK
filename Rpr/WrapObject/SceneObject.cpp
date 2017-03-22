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

#include "WrapObject/SceneObject.h"
#include "WrapObject/ShapeObject.h"
#include "WrapObject/LightObject.h"
#include "WrapObject/CameraObject.h"
#include "App/Scene/scene1.h"
#include "App/Scene/light.h"
#include "App/Scene/shape.h"
#include "App/Scene/iterator.h"

SceneObject::SceneObject()
    : m_scene(nullptr)
{
    m_scene = new Baikal::Scene1();
}
SceneObject::~SceneObject()
{
    delete m_scene;
    m_scene = nullptr;
}

void SceneObject::Clear()
{
    //remove lights
    for (std::unique_ptr<Baikal::Iterator> it_light(m_scene->CreateLightIterator()); it_light->IsValid();)
    {
        m_scene->DetachLight(it_light->ItemAs<const Baikal::Light>());
        it_light.reset(m_scene->CreateLightIterator());
    }

    //remove shapes
    for (std::unique_ptr<Baikal::Iterator> it_shape(m_scene->CreateShapeIterator()); it_shape->IsValid();)
    {
        m_scene->DetachShape(it_shape->ItemAs<const Baikal::Mesh>());
        it_shape.reset(m_scene->CreateShapeIterator());
    }
}

void SceneObject::AttachShape(ShapeObject* shape)
{
    m_scene->AttachShape(shape->GetShape());
}

void SceneObject::DetachShape(ShapeObject* shape)
{
    m_scene->DetachShape(shape->GetShape());
}

void SceneObject::AttachLight(LightObject* light)
{
    m_scene->AttachLight(light->GetLight());
}

void SceneObject::DetachLight(LightObject* light)
{
    m_scene->DetachLight(light->GetLight());
}

void SceneObject::SetCamera(CameraObject* cam)
{
    m_current_camera = cam;
    Baikal::Camera* baikal_cam = cam ? cam->GetCamera() : nullptr;
    m_scene->SetCamera(baikal_cam);
}
