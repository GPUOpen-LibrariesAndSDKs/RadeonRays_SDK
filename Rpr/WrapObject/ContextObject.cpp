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

#include "SceneGraph/scene1.h"
#include "SceneGraph/iterator.h"
#include "SceneGraph/material.h"
#include "SceneGraph/light.h"

namespace
{
    struct ParameterDesc
    {
        std::string name;
        std::string desc;
        rpr_parameter_type type;

        ParameterDesc() {}
        ParameterDesc(std::string const& n, std::string const& d, rpr_parameter_type t)
            : name(n)
            , desc(d)
            , type(t)
        {
        }

    };
    std::map<uint32_t, ParameterDesc> kContextParameterDescriptions = {
    { RPR_CONTEXT_AA_CELL_SIZE,{ "aacellsize", "Numbers of cells for stratified sampling", RPR_PARAMETER_TYPE_UINT } },
    { RPR_CONTEXT_AA_SAMPLES,{ "aasamples", "Numbers of samples per pixel", RPR_PARAMETER_TYPE_UINT } },
    { RPR_CONTEXT_IMAGE_FILTER_TYPE,{ "imagefilter.type", "Image filter to use", RPR_PARAMETER_TYPE_UINT } },
    { RPR_CONTEXT_IMAGE_FILTER_BOX_RADIUS,{ "imagefilter.box.radius", "Image filter to use", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_IMAGE_FILTER_GAUSSIAN_RADIUS,{ "imagefilter.gaussian.radius", "Filter radius", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_IMAGE_FILTER_TRIANGLE_RADIUS,{ "imagefilter.triangle.radius", "Filter radius", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_IMAGE_FILTER_MITCHELL_RADIUS,{ "imagefilter.mitchell.radius", "Filter radius", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_IMAGE_FILTER_LANCZOS_RADIUS,{ "imagefilter.lanczos.radius", "Filter radius", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_IMAGE_FILTER_BLACKMANHARRIS_RADIUS,{ "imagefilter.blackmanharris.radius", "Filter radius", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_TONE_MAPPING_TYPE,{ "tonemapping.type", "Tonemapping operator", RPR_PARAMETER_TYPE_UINT } },
    { RPR_CONTEXT_TONE_MAPPING_LINEAR_SCALE,{ "tonemapping.linear.scale", "Linear scale", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_SENSITIVITY,{ "tonemapping.photolinear.sensitivity", "Linear sensitivity", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_EXPOSURE,{ "tonemapping.photolinear.exposure", "Photolinear exposure", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_FSTOP,{ "tonemapping.photolinear.fstop", "Photolinear fstop", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_TONE_MAPPING_REINHARD02_PRE_SCALE,{ "tonemapping.reinhard02.prescale", "Reinhard prescale", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_TONE_MAPPING_REINHARD02_POST_SCALE,{ "tonemapping.reinhard02.postscale", "Reinhard postscale", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_TONE_MAPPING_REINHARD02_BURN,{ "tonemapping.reinhard02.burn", "Reinhard burn", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_MAX_RECURSION,{ "maxRecursion", "Ray trace depth", RPR_PARAMETER_TYPE_UINT } },
    { RPR_CONTEXT_RAY_CAST_EPISLON,{ "raycastepsilon", "Ray epsilon", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_RADIANCE_CLAMP,{ "radianceclamp", "Max radiance value", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_X_FLIP,{ "xflip", "Flip framebuffer output along X axis", RPR_PARAMETER_TYPE_UINT } },
    { RPR_CONTEXT_Y_FLIP,{ "yflip", "Flip framebuffer output along Y axis", RPR_PARAMETER_TYPE_UINT } },
    { RPR_CONTEXT_TEXTURE_GAMMA,{ "texturegamma", "Texture gamma", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_DISPLAY_GAMMA,{ "displaygamma", "Display gamma", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_CLIPPING_PLANE,{ "clippingplane", "Clipping Plane", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_MATERIAL_STACK_SIZE,{ "stacksize", "Maximum number of nodes that a material graph can support. Constant value.", RPR_PARAMETER_TYPE_UINT } },
    { RPR_CONTEXT_PDF_THRESHOLD,{ "pdfThreshold", "pdf Threshold", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_RENDER_MODE,{ "stage", "render mode", RPR_PARAMETER_TYPE_UINT } },
    { RPR_CONTEXT_ROUGHNESS_CAP,{ "roughnessCap", "roughness Cap", RPR_PARAMETER_TYPE_FLOAT } },
    { RPR_CONTEXT_GPU0_NAME,{ "gpu0name", "Name of the GPU index 0 in context. Constant value.", RPR_PARAMETER_TYPE_STRING } },
    { RPR_CONTEXT_GPU1_NAME,{ "gpu1name", "Name of the GPU index 1 in context. Constant value.", RPR_PARAMETER_TYPE_STRING } },
    { RPR_CONTEXT_GPU2_NAME,{ "gpu2name", "Name of the GPU index 2 in context. Constant value.", RPR_PARAMETER_TYPE_STRING } },
    { RPR_CONTEXT_GPU3_NAME,{ "gpu3name", "Name of the GPU index 3 in context. Constant value.", RPR_PARAMETER_TYPE_STRING } },
    { RPR_CONTEXT_GPU4_NAME,{ "gpu4name", "Name of the GPU index 4 in context. Constant value.", RPR_PARAMETER_TYPE_STRING } },
    { RPR_CONTEXT_GPU5_NAME,{ "gpu5name", "Name of the GPU index 5 in context. Constant value.", RPR_PARAMETER_TYPE_STRING } },
    { RPR_CONTEXT_GPU6_NAME,{ "gpu6name", "Name of the GPU index 6 in context. Constant value.", RPR_PARAMETER_TYPE_STRING } },
    { RPR_CONTEXT_GPU7_NAME,{ "gpu7name", "Name of the GPU index 7 in context. Constant value.", RPR_PARAMETER_TYPE_STRING } },
    { RPR_CONTEXT_CPU_NAME,{ "cpuname", "Name of the CPU in context. Constant value.", RPR_PARAMETER_TYPE_STRING } },
    };

    std::map<uint32_t, Baikal::Renderer::OutputType> kOutputTypeMap = { {RPR_AOV_COLOR, Baikal::Renderer::OutputType::kColor},
                                                                        {RPR_AOV_GEOMETRIC_NORMAL, Baikal::Renderer::OutputType::kWorldGeometricNormal},
                                                                        {RPR_AOV_SHADING_NORMAL, Baikal::Renderer::OutputType::kWorldShadingNormal},
                                                                        {RPR_AOV_UV, Baikal::Renderer::OutputType::kUv},
                                                                        {RPR_AOV_WORLD_COORDINATE, Baikal::Renderer::OutputType::kWorldPosition}, 
                                                                        };

}// anonymous

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
    FramebufferObject* old_buf = GetAOV(in_aov);

    auto aov = kOutputTypeMap.find(in_aov);
    if (aov == kOutputTypeMap.end())
    {
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "Context: requested AOV not implemented.");
    }
    
    for (auto& c : m_cfgs)
    {
        c.renderer->SetOutput(aov->second, buffer->GetOutput());
    }

    //update registered output framebuffer
    m_output_framebuffers.erase(old_buf);
    m_output_framebuffers.insert(buffer);
}


FramebufferObject* ContextObject::GetAOV(rpr_int in_aov)
{
    auto aov = kOutputTypeMap.find(in_aov);
    if (aov == kOutputTypeMap.end())
    {
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "Context: requested AOV not implemented.");
    }

    Baikal::Output* out = m_cfgs[0].renderer->GetOutput(aov->second);
    if (!out)
    {
        return nullptr;
    }
    
    //find framebuffer
    auto it = find_if(m_output_framebuffers.begin(), m_output_framebuffers.end(), [out](FramebufferObject* buff)
    {
        return buff->GetOutput() == out;
    });
    if (it == m_output_framebuffers.end())
    {
        throw Exception(RPR_ERROR_INTERNAL_ERROR, "Context: unknown framebuffer.");
    }

    return *it;
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

void ContextObject::RenderTile(rpr_uint xmin, rpr_uint xmax, rpr_uint ymin, rpr_uint ymax)
{
    m_current_scene->AddEmissive();
    const RadeonRays::int2 origin = { (int)xmin, (int)ymin };
    const RadeonRays::int2 size = { (int)xmax - (int)xmin, (int)ymax - (int)ymin };
    //render
    for (auto& c : m_cfgs)
    {
        c.renderer->RenderTile(*m_current_scene->GetScene(), origin, size);
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

void ContextObject::SetParameter(const std::string& input, float x, float y, float z, float w)
{
    auto it = std::find_if(kContextParameterDescriptions.begin(), kContextParameterDescriptions.end(), [input](std::pair<uint32_t, ParameterDesc> desc) { return desc.second.name == input; });
    if (it == kContextParameterDescriptions.end())
    {
        throw Exception(RPR_ERROR_INVALID_TAG, "ContextObject: invalid context input parameter.");
    }

    it = std::find_if(kContextParameterDescriptions.begin(), kContextParameterDescriptions.end(), [input](std::pair<uint32_t, ParameterDesc> desc){ return desc.second.type == RPR_PARAMETER_TYPE_FLOAT; });
    if (it == kContextParameterDescriptions.end())
    {
        throw Exception(RPR_ERROR_INVALID_PARAMETER_TYPE, "ContextObject: invalid context input type.");
    }
}

void ContextObject::SetParameter(const std::string& input, const std::string& value)
{
    auto it = std::find_if(kContextParameterDescriptions.begin(), kContextParameterDescriptions.end(), [input](std::pair<uint32_t, ParameterDesc> desc) { return desc.second.name == input; });
    if (it == kContextParameterDescriptions.end())
    {
        throw Exception(RPR_ERROR_INVALID_TAG, "ContextObject: invalid context input parameter.");
    }

    it = std::find_if(kContextParameterDescriptions.begin(), kContextParameterDescriptions.end(), [input](std::pair<uint32_t, ParameterDesc> desc) { return desc.second.type == RPR_PARAMETER_TYPE_STRING; });
    if (it == kContextParameterDescriptions.end())
    {
        throw Exception(RPR_ERROR_INVALID_PARAMETER_TYPE, "ContextObject: invalid context input type.");
    }
}