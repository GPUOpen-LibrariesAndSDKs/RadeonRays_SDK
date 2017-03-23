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

#include "WrapObject/ContextObject.h"
#include "WrapObject/SceneObject.h"
#include "WrapObject/MatSysObject.h"
#include "WrapObject/ShapeObject.h"
#include "WrapObject/CameraObject.h"
#include "WrapObject/LightObject.h"
#include "WrapObject/FramebufferObject.h"
#include "WrapObject/MaterialObject.h"
#include "WrapObject/Exception.h"

#include "Scene/scene1.h"
#include "Scene/iterator.h"
#include "Scene/material.h"
#include "Scene/light.h"

ContextObject::ContextObject(rpr_creation_flags creation_flags)
    : m_current_scene(nullptr)
{
    rpr_int result = RPR_SUCCESS;

    bool interop = (creation_flags & RPR_CREATION_FLAGS_ENABLE_GL_INTEROP);
    if (creation_flags & RPR_CREATION_FLAGS_ENABLE_GPU0)
    {
        try
        {
            //TODO: check num_bounces 
            ConfigManager::CreateConfigs(ConfigManager::kUseSingleGpu, interop, m_cfgs, 5);
        }
        catch (...)
        {
            // failed to create context with interop
            result = RPR_ERROR_UNSUPPORTED;
        }
    }
    else
    {
        result = RPR_ERROR_UNIMPLEMENTED;
    }

    if (result != RPR_SUCCESS)
    {
        throw Exception(result, "");
    }
}

void ContextObject::GetRenderStatistics(void * out_data, size_t * out_size_ret) const
{
    if (out_data)
    {
        //TODO: more statistics
        rpr_render_statistics* rs = static_cast<rpr_render_statistics*>(out_data);
        for (const auto& cfg : m_cfgs)
        {
            rs->gpumem_usage += cfg.renderer->m_vidmemws;
            rs->gpumem_total += cfg.renderer->m_vidmemws;
            rs->gpumem_max_allocation += cfg.renderer->m_vidmemws;
        }
    }
    if (out_size_ret)
    {
        *out_size_ret = sizeof(rpr_render_statistics);
    }
}

void ContextObject::SetAOV(rpr_int in_aov, FramebufferObject* buffer)
{
    switch (in_aov)
    {
    case RPR_AOV_COLOR:
        for (auto& c : m_cfgs)
            c.renderer->SetOutput(buffer->GetOutput());
        break;
    case RPR_AOV_DEPTH:
    case RPR_AOV_GEOMETRIC_NORMAL:
    case RPR_AOV_MATERIAL_IDX:
    case RPR_AOV_MAX:
    case RPR_AOV_OBJECT_GROUP_ID:
    case RPR_AOV_OBJECT_ID:
    case RPR_AOV_OPACITY:
    case RPR_AOV_SHADING_NORMAL:
    case RPR_AOV_UV:
    case RPR_AOV_WORLD_COORDINATE:
        throw Exception(RPR_ERROR_UNSUPPORTED, "Context: only RPR_AOV_COLOR supported now");
    default:
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "Context: unrecognized AOV");
    }
}


FramebufferObject* ContextObject::GetAOV(rpr_int in_aov)
{
    switch (in_aov)
    {
    case RPR_AOV_COLOR:
        //TODO: implement
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "Context: unimplemented");
        break;
    case RPR_AOV_DEPTH:
    case RPR_AOV_GEOMETRIC_NORMAL:
    case RPR_AOV_MATERIAL_IDX:
    case RPR_AOV_MAX:
    case RPR_AOV_OBJECT_GROUP_ID:
    case RPR_AOV_OBJECT_ID:
    case RPR_AOV_OPACITY:
    case RPR_AOV_SHADING_NORMAL:
    case RPR_AOV_UV:
    case RPR_AOV_WORLD_COORDINATE:
        throw Exception(RPR_ERROR_UNSUPPORTED, "Context: only RPR_AOV_COLOR supported now");
    default:
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "Context: unrecognized AOV");
    }

    return nullptr;
}

void ContextObject::Render()
{
	m_current_scene->AddEmissive();

	//render
    for (auto& c : m_cfgs)
    {
        c.renderer->Render(*m_current_scene->GetScene());
    }

}
SceneObject* ContextObject::CreateScene()
{
    return new SceneObject();
}

MatSysObject* ContextObject::CreateMaterialSystem()
{
    return new MatSysObject();
}


LightObject* ContextObject::CreateLight(LightObject::Type type)
{
    return new LightObject(type);
}


ShapeObject* ContextObject::CreateShape(rpr_float const * in_vertices, size_t in_num_vertices, rpr_int in_vertex_stride,
    rpr_float const * in_normals, size_t in_num_normals, rpr_int in_normal_stride,
    rpr_float const * in_texcoords, size_t in_num_texcoords, rpr_int in_texcoord_stride,
    rpr_int const * in_vertex_indices, rpr_int in_vidx_stride,
    rpr_int const * in_normal_indices, rpr_int in_nidx_stride,
    rpr_int const * in_texcoord_indices, rpr_int in_tidx_stride,
    rpr_int const * in_num_face_vertices, size_t in_num_faces)
{
    return ShapeObject::CreateMesh(in_vertices, in_num_vertices, in_vertex_stride,
        in_normals, in_num_normals, in_normal_stride,
        in_texcoords, in_num_texcoords, in_texcoord_stride,
        in_vertex_indices, in_vidx_stride,
        in_normal_indices, in_nidx_stride,
        in_texcoord_indices, in_tidx_stride,
        in_num_face_vertices, in_num_faces);
}
ShapeObject* ContextObject::CreateShapeInstance(ShapeObject* mesh)
{
    return mesh->CreateInstance();
}

MaterialObject* ContextObject::CreateTexture(rpr_image_format const in_format, rpr_image_desc const * in_image_desc, void const * in_data)
{
    MaterialObject* result = new MaterialObject(in_format, in_image_desc, in_data);
    return result;
}

MaterialObject* ContextObject::CreateTextureFromFile(rpr_char const * in_path)
{
    MaterialObject* result = new MaterialObject(in_path);
    return result;
}

CameraObject* ContextObject::CreateCamera()
{
    return new CameraObject();
}

FramebufferObject* ContextObject::CreateFrameBuffer(rpr_framebuffer_format const in_format, rpr_framebuffer_desc const * in_fb_desc)
{
    //TODO: implement
    if (in_format.type != RPR_COMPONENT_TYPE_FLOAT32 || in_format.num_components != 4)
    {
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "ContextObject: only 4 component RPR_COMPONENT_TYPE_FLOAT32 implemented now.");
    }

    //TODO:: implement for several devices
    if (m_cfgs.size() != 1)
    {
        throw Exception(RPR_ERROR_INTERNAL_ERROR, "ContextObject: incalid config count.");
    }
    auto& c = m_cfgs[0];
    FramebufferObject* result = new FramebufferObject();
    Baikal::Output* out = c.renderer->CreateOutput(in_fb_desc->fb_width, in_fb_desc->fb_height);
    result->SetOutput(out);
    return result;
}