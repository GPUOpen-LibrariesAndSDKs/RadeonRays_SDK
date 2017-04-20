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
#include "WrapObject/LightObject.h"

#include "App/Utils/config_manager.h"
#include "App/Renderers/PT/ptrenderer.h"

#include <vector>
#include "RadeonProRender.h"

class TextureObject;
class FramebufferObject;
class SceneObject;
class MatSysObject;
class ShapeObject;
class CameraObject;
class MaterialObject;

//this class represent rpr_context
class ContextObject
    : public WrapObject
{
public:
    ContextObject(rpr_creation_flags creation_flags);
    virtual ~ContextObject() = default;
    //cur. scene
    SceneObject* GetCurrentScene() { return m_current_scene; }
    void SetCurrenScene(SceneObject* scene) { m_current_scene = scene; }
    
    //context info
    void GetRenderStatistics(void * out_data, size_t * out_size_ret) const;
    void SetParameter(const std::string& input, float x, float y = 0.f, float z = 0.f, float w = 0.f);
    void SetParameter(const std::string& input, const std::string& value);

    //AOV
    void SetAOV(rpr_int in_aov, FramebufferObject* buffer);
    FramebufferObject* GetAOV(rpr_int in_aov);

    //render
    void Render();
    void RenderTile(rpr_uint xmin, rpr_uint xmax, rpr_uint ymin, rpr_uint ymax);

    //create methods
    SceneObject* CreateScene();
    MatSysObject* CreateMaterialSystem();
    LightObject* CreateLight(LightObject::Type type);
    ShapeObject* CreateShape(rpr_float const * in_vertices, size_t in_num_vertices, rpr_int in_vertex_stride,
                            rpr_float const * in_normals, size_t in_num_normals, rpr_int in_normal_stride,
                            rpr_float const * in_texcoords, size_t in_num_texcoords, rpr_int in_texcoord_stride,
                            rpr_int const * in_vertex_indices, rpr_int in_vidx_stride,
                            rpr_int const * in_normal_indices, rpr_int in_nidx_stride,
                            rpr_int const * in_texcoord_indices, rpr_int in_tidx_stride,
                            rpr_int const * in_num_face_vertices, size_t in_num_faces);
    ShapeObject* CreateShapeInstance(ShapeObject* mesh);
    MaterialObject* CreateTexture(rpr_image_format const in_format, rpr_image_desc const * in_image_desc, void const * in_data);
    MaterialObject* CreateTextureFromFile(rpr_char const * in_path);
    CameraObject* CreateCamera();
    FramebufferObject* CreateFrameBuffer(rpr_framebuffer_format const in_format, rpr_framebuffer_desc const * in_fb_desc);
private:
    //render configs
    std::vector<ConfigManager::Config> m_cfgs;
    //know framefubbers used as AOV outputs
    std::set<FramebufferObject*> m_output_framebuffers;
    SceneObject* m_current_scene;
};