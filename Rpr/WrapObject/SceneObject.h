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

#include "WrapObject/WrapObject.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/shape.h"
#include "SceneGraph/light.h"

#include <vector>

class ShapeObject;
class LightObject;
class CameraObject;

//this class represent rpr_context
class SceneObject
    : public WrapObject
{
public:
    SceneObject();
    virtual ~SceneObject();

    void Clear();
    
    //shape
    void AttachShape(ShapeObject* shape);
    void DetachShape(ShapeObject* shape);

    //light
    void AttachLight(LightObject* light);
    void DetachLight(LightObject* light);
    
    //camera
    void SetCamera(CameraObject* cam);
    CameraObject* GetCamera() { return m_current_camera; }

	void GetShapeList(void* out_list);
	size_t GetShapeCount() { return m_scene->GetNumShapes(); }
    
    void GetLightList(void* out_list);
    size_t GetLightCount() { return m_scene->GetNumLights(); }

	void AddEmissive();
	void RemoveEmissive();
	Baikal::Scene1* GetScene() { return m_scene; };
private:
    Baikal::Scene1* m_scene;
    CameraObject* m_current_camera;
	std::vector<Baikal::AreaLight*> m_emmisive_lights;//area lights fro emissive shapes
    std::vector<ShapeObject*> m_shapes;
    std::vector<LightObject*> m_lights;

};