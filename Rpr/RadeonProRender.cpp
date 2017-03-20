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
#include "RadeonProRender.h"
#include "Scene/scene1.h"
#include "Scene/shape.h"
#include "Scene/material.h"
#include "Scene/camera.h"
#include "Scene/light.h"
#include "Scene/texture.h"
#include "Scene/IO/image_io.h"
#include "CLW/clwoutput.h"
#include "PT/ptrenderer.h"
#include "config_manager.h"
#include "OpenImageIO/imageio.h"
#include "math/mathutils.h"

using namespace RadeonRays;
using namespace Baikal;

typedef std::vector<rpr_material_node> MaterialSystem;
struct Context
{
    Context(): current_scene(nullptr){};
    std::vector<ConfigManager::Config> cfgs;
    Scene1* current_scene;
};



rpr_int rprRegisterPlugin(rpr_char const * path)
{
    return RPR_ERROR_UNSUPPORTED;
}

rpr_int rprCreateContext(rpr_int api_version, rpr_int * pluginIDs, size_t pluginCount, rpr_creation_flags creation_flags, rpr_context_properties const * props, rpr_char const * cache_path, rpr_context * out_context)
{
    if (api_version != RPR_API_VERSION)
    {
        return RPR_ERROR_INVALID_API_VERSION;
    }

    bool interop = (creation_flags & RPR_CREATION_FLAGS_ENABLE_GL_INTEROP);
    if (creation_flags & RPR_CREATION_FLAGS_ENABLE_GPU0)
    {
        Context* cont = new Context();
        try
        {
            //TODO: check num_bounces 
            ConfigManager::CreateConfigs(ConfigManager::kUseSingleGpu, interop, cont->cfgs, 5);
        }
        catch(...)
        {
            // failed to create context with interop
            return RPR_ERROR_UNSUPPORTED;
        }
        *out_context = static_cast<rpr_context>(cont);
    }
    else
    {
        return RPR_ERROR_UNIMPLEMENTED;
    }

    return RPR_SUCCESS;
}

rpr_int rprContextSetActivePlugin(rpr_context context, rpr_int pluginID)
{
    return RPR_ERROR_UNSUPPORTED;
}

rpr_int rprContextGetInfo(rpr_context in_context, rpr_context_info in_context_info, size_t in_size, void * out_data, size_t * out_size_ret)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //cast data
    Context* context = static_cast<Context*>(in_context);

    switch (in_context_info)
    {
    case RPR_CONTEXT_RENDER_STATISTICS:
        if (out_data)
        {
            //TODO: more statistics
            rpr_render_statistics* rs = static_cast<rpr_render_statistics*>(out_data);
            for (const auto& cfg : context->cfgs)
            {
                rs->gpumem_usage += context->cfgs[0].renderer->m_vidmemws;
                rs->gpumem_total += context->cfgs[0].renderer->m_vidmemws;
                rs->gpumem_max_allocation += context->cfgs[0].renderer->m_vidmemws;
            }
        }
        if (out_size_ret)
        {
            *out_size_ret = sizeof(rpr_render_statistics);
        }
        break;
    default:
        return RPR_ERROR_UNIMPLEMENTED;

    }
    return RPR_SUCCESS;
}

rpr_int rprContextGetParameterInfo(rpr_context context, int param_idx, rpr_parameter_info parameter_info, size_t size, void * data, size_t * size_ret)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextGetAOV(rpr_context in_context, rpr_aov in_aov, rpr_framebuffer * out_fb)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextSetAOV(rpr_context in_context, rpr_aov in_aov, rpr_framebuffer in_frame_buffer)
{
    if (!in_context || !in_frame_buffer)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    //prepare
    Context* cont = static_cast<Context*>(in_context);
    Output* buffer = static_cast<Output*>(in_frame_buffer);

    //at least one config should be available
    if (cont->cfgs.size() < 1)
    {
        return RPR_ERROR_INTERNAL_ERROR;
    }

    switch (in_aov)
    {
    case RPR_AOV_COLOR:
        for (auto& c : cont->cfgs)
            c.renderer->SetOutput(buffer);
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
        return RPR_ERROR_UNSUPPORTED;
    default:
        //TODO: add other AOV
        return RPR_ERROR_INVALID_PARAMETER;
    }

    return RPR_SUCCESS;
}

rpr_int rprContextSetScene(rpr_context in_context, rpr_scene in_scene)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    if (!in_scene)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast data
    Context* cont = static_cast<Context*>(in_context);
    Scene1* scene = static_cast<Scene1*>(in_scene);
    cont->current_scene = scene;

    return RPR_SUCCESS;
}


rpr_int rprContextGetScene(rpr_context in_context, rpr_scene * out_scene)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //cast data
    Context* cont = static_cast<Context*>(in_context);
    *out_scene = cont->current_scene;

    return RPR_SUCCESS;
}

rpr_int rprContextSetParameter1u(rpr_context in_context, rpr_char const * name, rpr_uint x)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //cast data
    Context* cont = static_cast<Context*>(in_context);
    //TODO: handle all context parameters
    if (!strcmp(name, "rendermode"))
    {
        switch (x)
        {
        case RPR_RENDER_MODE_GLOBAL_ILLUMINATION:
            break;
        default:
            return RPR_ERROR_UNIMPLEMENTED;
        }
    }

    return RPR_SUCCESS;
}

rpr_int rprContextSetParameter1f(rpr_context context, rpr_char const * name, rpr_float x)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextSetParameter3f(rpr_context context, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextSetParameter4f(rpr_context context, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z, rpr_float w)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextSetParameterString(rpr_context context, rpr_char const * name, rpr_char const * value)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextRender(rpr_context in_context)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //prepare
    Context* cont = static_cast<Context*>(in_context);
    for (auto& c : cont->cfgs)
    {
        c.renderer->Render(*cont->current_scene);
    }

    return RPR_SUCCESS;
}

rpr_int rprContextRenderTile(rpr_context context, rpr_uint xmin, rpr_uint xmax, rpr_uint ymin, rpr_uint ymax)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextClearMemory(rpr_context context)
{
    return RPR_ERROR_UNSUPPORTED;
}

rpr_int rprContextCreateImage(rpr_context in_context, rpr_image_format const in_format, rpr_image_desc const * in_image_desc, void const * in_data, rpr_image * out_image)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //cast data
    Context* context = static_cast<Context*>(in_context);
    
    //TODO: fix only 4 components data supported
    if (in_format.num_components != 4)
        return RPR_ERROR_UNIMPLEMENTED;
    
    //tex size
    int2 tex_size(in_image_desc->image_width, in_image_desc->image_height);
    
    //texture takes ownership of its data array
    //so need to copy input data
    int data_size = in_format.num_components * tex_size.x * tex_size.y;
    Texture::Format data_format = Texture::Format::kRgba8;
    switch (in_format.type)
    {
    case RPR_COMPONENT_TYPE_UINT8:
        break;
    case RPR_COMPONENT_TYPE_FLOAT16:
        data_size *= 2;
        data_format = Texture::Format::kRgba16;
        break;
    case RPR_COMPONENT_TYPE_FLOAT32:
        data_size *= 4;
        data_format = Texture::Format::kRgba32;
        break;
    default:
        return RPR_ERROR_INVALID_PARAMETER;
    }

    char* data = new char[data_size];
    memcpy(data, in_data, data_size);
    Texture* tex = new Texture(data, tex_size, data_format);
    *out_image = tex;

    return RPR_SUCCESS;
}

rpr_int rprContextCreateImageFromFile(rpr_context in_context, rpr_char const * in_path, rpr_image * out_image)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //load texture using oiio
    ImageIo* oiio = ImageIo::CreateImageIo();
    Texture* texture = oiio->LoadImage(in_path);
    *out_image = texture;
    delete oiio;

    return RPR_SUCCESS;
}

rpr_int rprContextCreateScene(rpr_context in_context, rpr_scene * out_scene)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    Context* cont = static_cast<Context*>(in_context);
    Scene1* scene = new Scene1();
    *out_scene = scene;

    return RPR_SUCCESS;
}

rpr_int rprContextCreateInstance(rpr_context in_context, rpr_shape shape, rpr_shape * out_instance)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    
    if (!shape || !out_instance)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    Context* context = static_cast<Context*>(in_context);
    Baikal::Shape* mesh = static_cast<Baikal::Shape*>(shape);

    //TODO: replace by instance
    Instance* instance = new Instance(mesh);
        
    *out_instance = instance;

    return RPR_SUCCESS;
}

template<typename T, int size = 3> std::vector<T> merge(const T* in_data, size_t in_data_num, rpr_int in_data_stride,
                                            rpr_int const * in_data_indices, rpr_int in_didx_stride, 
                                            rpr_int const * in_num_face_vertices, size_t in_num_faces)
{
    std::vector<T> result;
    int indent = 0;
    for (int i = 0; i < in_num_faces; ++i)
    {
        int face = in_num_face_vertices[i];
        for (int f = indent; f < face + indent; ++f)
        {
            int index = in_data_stride/sizeof(T) * in_data_indices[f*in_didx_stride / sizeof(rpr_int)];
            for (int s = 0; s < size; ++s)
            {
                result.push_back(in_data[index + s]);
            }
        }

        indent += face;
    }
    return result;
}

rpr_int rprContextCreateMesh(rpr_context in_context, 
                            rpr_float const * in_vertices, size_t in_num_vertices, rpr_int in_vertex_stride,
                            rpr_float const * in_normals, size_t in_num_normals, rpr_int in_normal_stride,
                            rpr_float const * in_texcoords, size_t in_num_texcoords, rpr_int in_texcoord_stride,
                            rpr_int const * in_vertex_indices, rpr_int in_vidx_stride,
                            rpr_int const * in_normal_indices, rpr_int in_nidx_stride,
                            rpr_int const * in_texcoord_indices, rpr_int in_tidx_stride,
                            rpr_int const * in_num_face_vertices, size_t in_num_faces, rpr_shape * out_mesh)
{
    Context* context = static_cast<Context*>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //merge data
    std::vector<float> verts = merge(in_vertices, in_num_vertices, in_vertex_stride,
                                    in_vertex_indices, in_vidx_stride,
                                    in_num_face_vertices, in_num_faces);
    std::vector<float> normals = merge(in_normals, in_num_normals, in_normal_stride,
                                        in_normal_indices, in_nidx_stride,
                                        in_num_face_vertices, in_num_faces);
    std::vector<float> uvs = merge<float, 2>(in_texcoords, in_num_texcoords, in_texcoord_stride,
                                    in_texcoord_indices, in_tidx_stride,
                                    in_num_face_vertices, in_num_faces);
    
    //generate indices
    std::vector<std::uint32_t> inds;
    std::uint32_t indent = 0;
    for (std::uint32_t i = 0; i < in_num_faces; ++i)
    {
        inds.push_back(indent);
        inds.push_back(indent + 1);
        inds.push_back(indent + 2);

        int face = in_num_face_vertices[i];

        //triangulation
        if (face == 4)
        {
            inds.push_back(indent + 0);
            inds.push_back(indent + 2);
            inds.push_back(indent + 3);

        }
        //only triangles and quads supported
        else if (face != 3)
        {
            return RPR_ERROR_UNIMPLEMENTED;
        }
        indent += face;
    }

    //create mesh
    Baikal::Mesh* mesh = new Baikal::Mesh();
    mesh->SetVertices(verts.data(), verts.size()/3);
    mesh->SetNormals(normals.data(), normals.size()/3);
    mesh->SetUVs(uvs.data(), uvs.size()/2);
    mesh->SetIndices(inds.data(), inds.size());

    *out_mesh = mesh;

    return RPR_SUCCESS;
}

rpr_int rprContextCreateMeshEx(rpr_context context, rpr_float const * vertices, size_t num_vertices, rpr_int vertex_stride, rpr_float const * normals, size_t num_normals, rpr_int normal_stride, rpr_int const * perVertexFlag, size_t num_perVertexFlags, rpr_int perVertexFlag_stride, rpr_int numberOfTexCoordLayers, rpr_float const ** texcoords, size_t * num_texcoords, rpr_int * texcoord_stride, rpr_int const * vertex_indices, rpr_int vidx_stride, rpr_int const * normal_indices, rpr_int nidx_stride, rpr_int const ** texcoord_indices, rpr_int * tidx_stride, rpr_int const * num_face_vertices, size_t num_faces, rpr_shape * out_mesh)
{
    return RPR_ERROR_UNSUPPORTED;
}

rpr_int rprContextCreateCamera(rpr_context in_context, rpr_camera * out_camera)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //cast data
    Context* cont = static_cast<Context*>(in_context);
    RadeonRays::float3 const eye = { 0.f, 0.f, 0.f };
    RadeonRays::float3 const at = { 0.f , 0.f , -1.f };
    RadeonRays::float3 const up = { 0.f , 1.f , 0.f };
    PerspectiveCamera* cam = new PerspectiveCamera(eye, at, up);
    
    //set cam default properties
    float2 camera_sensor_size = float2(0.036f, 0.024f);  // default full frame sensor 36x24 mm
    float2 camera_zcap = float2(0.0f, 100000.f);
    float camera_focal_length = 0.035f; // 35mm lens
    float camera_focus_distance = 1.f;
    float camera_aperture = 0.f;

    cam->SetSensorSize(camera_sensor_size);
    cam->SetDepthRange(camera_zcap);
    cam->SetFocalLength(camera_focal_length);
    cam->SetFocusDistance(camera_focus_distance);
    cam->SetAperture(camera_aperture);

    *out_camera = cam;
    return RPR_SUCCESS;
}

rpr_int rprContextCreateFrameBuffer(rpr_context in_context, rpr_framebuffer_format const in_format, rpr_framebuffer_desc const * in_fb_desc, rpr_framebuffer * out_fb)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    //TODO: implement
    if (in_format.type != RPR_COMPONENT_TYPE_FLOAT32 || in_format.num_components != 4)
    {
        return RPR_ERROR_UNIMPLEMENTED;
    }
    //prepare
    Context* cont = static_cast<Context*>(in_context);

    //TODO:: implement for several devices
    if (cont->cfgs.size() != 1)
    {
        return RPR_ERROR_INTERNAL_ERROR;
    }
    auto& c = cont->cfgs[0];
    Output* out = c.renderer->CreateOutput(in_fb_desc->fb_width, in_fb_desc->fb_height);
    *out_fb = out;

    return RPR_SUCCESS;
}

rpr_int rprCameraGetInfo(rpr_camera camera, rpr_camera_info camera_info, size_t size, void * data, size_t * size_ret)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprCameraSetFocalLength(rpr_camera in_camera, rpr_float flength)
{
    if (!in_camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast data
    PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(in_camera);
    //translate meters to mm 
    camera->SetFocalLength(flength / 1000.f);

    return RPR_SUCCESS;
}

rpr_int rprCameraSetFocusDistance(rpr_camera in_camera, rpr_float fdist)
{
    if (!in_camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast data
    PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(in_camera);

    camera->SetFocusDistance(fdist);

    return RPR_SUCCESS;
}

rpr_int rprCameraSetTransform(rpr_camera camera, rpr_bool transpose, rpr_float * transform)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprCameraSetSensorSize(rpr_camera in_camera, rpr_float in_width, rpr_float in_height)
{
    if (!in_camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //get camera and set values
    PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(in_camera);
    //convedrt meters to mm
    float2 size = { in_width / 1000.f, in_height / 1000.f };
    camera->SetSensorSize(size);

    return RPR_SUCCESS;
}

rpr_int rprCameraLookAt(rpr_camera in_camera, rpr_float posx, rpr_float posy, rpr_float posz, rpr_float atx, rpr_float aty, rpr_float atz, rpr_float upx, rpr_float upy, rpr_float upz)
{
    if (!in_camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //get camera and set values
    PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(in_camera);
    const float3 pos = { posx, posy, posz };
    const float3 at = { atx, aty, atz };
    const float3 up = { upx, upy, upz};
    camera->LookAt(pos, at, up);

    return RPR_SUCCESS;
}

rpr_int rprCameraSetFStop(rpr_camera in_camera, rpr_float fstop)
{
    if (!in_camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast
    PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(in_camera);
    //translate m -> to mm
    camera->SetAperture(fstop / 1000.f);

    return RPR_SUCCESS;
}

rpr_int rprCameraSetApertureBlades(rpr_camera camera, rpr_uint num_blades)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprCameraSetExposure(rpr_camera camera, rpr_float exposure)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprCameraSetMode(rpr_camera in_camera, rpr_camera_mode mode)
{
    if (!in_camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast
    PerspectiveCamera* camera = static_cast<PerspectiveCamera*>(in_camera);
    //TODO: only perspective camera used for now
    switch (mode)
    {
    case RPR_CAMERA_MODE_PERSPECTIVE:
        break;
    default:
        return RPR_ERROR_UNIMPLEMENTED;
    }
    
    return RPR_SUCCESS;
}

rpr_int rprCameraSetOrthoWidth(rpr_camera camera, rpr_float width)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprCameraSetFocalTilt(rpr_camera camera, rpr_float tilt)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprCameraSetIPD(rpr_camera camera, rpr_float ipd)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprCameraSetLensShift(rpr_camera camera, rpr_float shiftx, rpr_float shifty)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprCameraSetOrthoHeight(rpr_camera camera, rpr_float height)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprImageGetInfo(rpr_image image, rpr_image_info image_info, size_t size, void * data, size_t * size_ret)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprImageSetWrap(rpr_image image, rpr_image_wrap_type type)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetTransform(rpr_shape in_shape, rpr_bool transpose, rpr_float const * transform)
{
    if (!in_shape)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    Mesh* mesh = static_cast<Mesh*>(in_shape);
    matrix m;
    //fill matrix
    memcpy(m.m, transform, 16  * sizeof(rpr_float));

    if (transpose)
    {
        m.transpose();
    }

    mesh->SetTransform(m);
    return RPR_SUCCESS;
}

rpr_int rprShapeSetSubdivisionFactor(rpr_shape shape, rpr_uint factor)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetSubdivisionCreaseWeight(rpr_shape shape, rpr_float factor)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetSubdivisionBoundaryInterop(rpr_shape shape, rpr_subdiv_boundary_interfop_type type)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetDisplacementScale(rpr_shape shape, rpr_float minscale, rpr_float maxscale)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetObjectGroupID(rpr_shape shape, rpr_uint objectGroupID)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetDisplacementImage(rpr_shape shape, rpr_image image)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetMaterial(rpr_shape in_shape, rpr_material_node in_node)
{
    if (!in_shape)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    
    //cast data
    Mesh* mesh = static_cast<Mesh*>(in_shape);
    Material* mat = static_cast<Material*>(in_node);
    mesh->SetMaterial(mat);

    return RPR_SUCCESS;
}

rpr_int rprShapeSetMaterialOverride(rpr_shape shape, rpr_material_node node)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetVolumeMaterial(rpr_shape shape, rpr_material_node node)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetLinearMotion(rpr_shape shape, rpr_float x, rpr_float y, rpr_float z)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetAngularMotion(rpr_shape shape, rpr_float x, rpr_float y, rpr_float z, rpr_float w)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetVisibility(rpr_shape shape, rpr_bool visible)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetVisibilityPrimaryOnly(rpr_shape shape, rpr_bool visible)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetVisibilityInSpecular(rpr_shape shape, rpr_bool visible)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetShadowCatcher(rpr_shape shape, rpr_bool shadowCatcher)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprShapeSetShadow(rpr_shape shape, rpr_bool casts_shadow)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprLightSetTransform(rpr_light in_light, rpr_bool in_transpose, rpr_float const * in_transform)
{
    if (!in_light)
        return RPR_ERROR_INVALID_PARAMETER;
    
    // cast and prepare data
    Light* light = static_cast<Light*>(in_light);
    matrix m;
    memcpy(&m.m00, in_transform, 16 * sizeof(rpr_float));
    if (in_transpose)
    {
        m = m.transpose();
    }

    light->SetPosition(float3(m.m30, m.m31, m.m32, m.m33));
    light->SetDirection(m * float3(0, 0, -1));

    return RPR_SUCCESS;
}

rpr_int rprShapeGetInfo(rpr_shape arg0, rpr_shape_info arg1, size_t arg2, void * arg3, size_t * arg4)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprMeshGetInfo(rpr_shape mesh, rpr_mesh_info mesh_info, size_t size, void * data, size_t * size_ret)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprMeshPolygonGetInfo(rpr_shape mesh, size_t polygon_index, rpr_mesh_polygon_info polygon_info, size_t size, void * data, size_t * size_ret)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprInstanceGetBaseShape(rpr_shape shape, rpr_shape * out_shape)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextCreatePointLight(rpr_context in_context, rpr_light * out_light)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    if (!out_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast data
    Context* cont = static_cast<Context*>(in_context);
    PointLight* light = new PointLight();
    *out_light = light;

    return RPR_SUCCESS;
}

rpr_int rprPointLightSetRadiantPower3f(rpr_light in_light, rpr_float in_r, rpr_float in_g, rpr_float in_b)
{
    if (!in_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //prepare
    PointLight* light = static_cast<PointLight*>(in_light);
    float3 radiant_power = { in_r, in_g, in_b };

    light->SetEmittedRadiance(radiant_power);

    return RPR_SUCCESS;
}

rpr_int rprContextCreateSpotLight(rpr_context in_context, rpr_light * out_light)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    if (!out_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    Context* cont = static_cast<Context*>(in_context);
    SpotLight* light = new SpotLight();
    *out_light = light;

    return RPR_SUCCESS;
}

rpr_int rprSpotLightSetRadiantPower3f(rpr_light in_light, rpr_float in_r, rpr_float in_g, rpr_float in_b)
{
    if (!in_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //prepare
    SpotLight* light = static_cast<SpotLight*>(in_light);
    float3 radiant_power = { in_r, in_g, in_b };

    light->SetEmittedRadiance(radiant_power);

    return RPR_SUCCESS;
}

rpr_int rprSpotLightSetConeShape(rpr_light in_light, rpr_float in_iangle, rpr_float in_oangle)
{
    if (!in_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //prepare
    SpotLight* light = static_cast<SpotLight*>(in_light);
    float2 angle = {in_iangle, in_oangle};
    
    //set angles
    //TODO:: check
    light->SetConeShape(angle);

    return RPR_SUCCESS;
}

rpr_int rprContextCreateDirectionalLight(rpr_context in_context, rpr_light * out_light)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    if (!out_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast
    Context* cont = static_cast<Context*>(in_context);
    Light* light = new DirectionalLight();
    *out_light = light;

    return RPR_SUCCESS;
}

rpr_int rprDirectionalLightSetRadiantPower3f(rpr_light in_light, rpr_float in_r, rpr_float in_g, rpr_float in_b)
{
    if (!in_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //prepare
    DirectionalLight* light = static_cast<DirectionalLight*>(in_light);
    float3 radiant_power = { in_r, in_g, in_b };

    light->SetEmittedRadiance(radiant_power);

    return RPR_SUCCESS;
}

rpr_int rprDirectionalLightSetShadowSoftness(rpr_light in_light, rpr_float in_coeff)
{
    if (!in_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //prepare
    DirectionalLight* light = static_cast<DirectionalLight*>(in_light);

    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextCreateEnvironmentLight(rpr_context in_context, rpr_light * out_light)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    if (!out_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast data
    Context* context = static_cast<Context*>(in_context);
    //create ibl
    ImageBasedLight* ibl = new ImageBasedLight();
    //result
    *out_light = ibl;

    return RPR_SUCCESS;
}

rpr_int rprEnvironmentLightSetImage(rpr_light in_env_light, rpr_image in_image)
{
    if (!in_env_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast data
    ImageBasedLight* ibl = static_cast<ImageBasedLight*>(in_env_light);
    Texture* img = static_cast<Texture*>(in_image);
    
    //set image
    ibl->SetTexture(img);

    return RPR_SUCCESS;
}

rpr_int rprEnvironmentLightSetIntensityScale(rpr_light in_env_light, rpr_float intensity_scale)
{
    if (!in_env_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    
    //cast data
    ImageBasedLight* ibl = static_cast<ImageBasedLight*>(in_env_light);
    //set data
    ibl->SetMultiplier(intensity_scale);

    return RPR_SUCCESS;
}

rpr_int rprEnvironmentLightAttachPortal(rpr_light env_light, rpr_shape portal)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprEnvironmentLightDetachPortal(rpr_light env_light, rpr_shape portal)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextCreateSkyLight(rpr_context context, rpr_light * out_light)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSkyLightSetTurbidity(rpr_light skylight, rpr_float turbidity)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSkyLightSetAlbedo(rpr_light skylight, rpr_float albedo)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSkyLightSetScale(rpr_light skylight, rpr_float scale)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSkyLightAttachPortal(rpr_light skylight, rpr_shape portal)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSkyLightDetachPortal(rpr_light skylight, rpr_shape portal)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextCreateIESLight(rpr_context context, rpr_light * light)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprIESLightSetRadiantPower3f(rpr_light light, rpr_float r, rpr_float g, rpr_float b)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprIESLightSetImageFromFile(rpr_light env_light, rpr_char const * imagePath, rpr_int nx, rpr_int ny)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprIESLightSetImageFromIESdata(rpr_light env_light, rpr_char const * iesData, rpr_int nx, rpr_int ny)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprLightGetInfo(rpr_light in_light, rpr_light_info in_info, size_t in_size, void * out_data, size_t * out_size_ret)
{
    if (!in_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast data
    Light* light = static_cast<Light*>(in_light);
    switch (in_info)
    {
    case RPR_LIGHT_TYPE:
    {
        int* data = static_cast<int*>(out_data);
        if (data && in_size < sizeof(int))
        {
            return RPR_ERROR_INVALID_PARAMETER;
        }
        if (out_size_ret)
        {
            *out_size_ret = sizeof(int);
        }
        if (data)
        {
            //check light type
            if (dynamic_cast<PointLight*>(light))
            {
                *data = RPR_LIGHT_TYPE_POINT;
            }
            else if (dynamic_cast<DirectionalLight*>(light))
            {
                *data = RPR_LIGHT_TYPE_DIRECTIONAL;
            }
            else if (dynamic_cast<SpotLight*>(light))
            {
                *data = RPR_LIGHT_TYPE_SPOT;
            }
            else if (dynamic_cast<ImageBasedLight*>(light))
            {
                *data = RPR_LIGHT_TYPE_ENVIRONMENT;
            }
            else
            {
                //TODO: add RPR_LIGHT_TYPE_SKY and RPR_LIGHT_TYPE_IES
                return RPR_ERROR_INVALID_LIGHT;
            }
        }
        break;
    }
    case RPR_LIGHT_TRANSFORM:
    {
        float *data = static_cast<float*>(out_data);
        if (data && in_size < sizeof(matrix))
        {
            return RPR_ERROR_INVALID_PARAMETER;
        }
        if (out_size_ret)
        {
            *out_size_ret = sizeof(matrix);
        }
        if (data)
        {
            float3 pos = light->GetPosition();
            float3 dir = light->GetDirection();
            //angle between -z axis and direction
            float angle = acos(dot(dir, float3(0.f, 0.f, -1.f, 0.f)));
            float3 axis = cross(dir, float3(0.f, 0.f, -1.f, 0.f));
            if (axis.sqnorm() < std::numeric_limits<float>::min())
            {
                //rotate around x axis
                axis = float3(1.f, 0.f, 0.f, 0.f);
            }
            matrix rot = rotation(axis, angle);
            matrix m = translation(pos) * rot;
            memcpy(data, &m.m[0], sizeof(matrix));
        }
        break;
    }
    default:
        return RPR_ERROR_INVALID_PARAMETER;
    }

    return RPR_SUCCESS;
}

rpr_int rprSceneClear(rpr_scene in_scene)
{
    if (!in_scene)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast input data
    Scene1* scene = static_cast<Scene1*>(in_scene);

    //remove lights
        for (std::unique_ptr<Iterator> it_light(scene->CreateLightIterator()); it_light->IsValid();)
    {
        scene->DetachLight(it_light->ItemAs<const Light>());
        it_light.reset(scene->CreateLightIterator());
    }

    //remove shapes
    for (std::unique_ptr<Iterator> it_shape(scene->CreateShapeIterator()); it_shape->IsValid();)
    {
        scene->DetachShape(it_shape->ItemAs<const Mesh>());
        it_shape.reset(scene->CreateShapeIterator());
    }

    return RPR_SUCCESS;
}

rpr_int rprSceneAttachShape(rpr_scene in_scene, rpr_shape in_shape)
{
    if (!in_scene || !in_shape)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast input data
    Scene1* scene = static_cast<Scene1*>(in_scene);
    Mesh* mesh = static_cast<Mesh*>(in_shape);
    scene->AttachShape(mesh);

    return RPR_SUCCESS;
}

rpr_int rprSceneDetachShape(rpr_scene in_scene, rpr_shape in_shape)
{
    if (!in_scene || !in_shape)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast input data
    Scene1* scene = static_cast<Scene1*>(in_scene);
    Mesh* mesh = static_cast<Mesh*>(in_shape);
    scene->DetachShape(mesh);

    return RPR_SUCCESS;
}

rpr_int rprSceneAttachLight(rpr_scene in_scene, rpr_light in_light)
{
    if (!in_scene || !in_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //prepare
    Scene1* scene = static_cast<Scene1*>(in_scene);
    Light* light = static_cast<Light*>(in_light);
    //attach
    scene->AttachLight(light);

    return RPR_SUCCESS;
}

rpr_int rprSceneDetachLight(rpr_scene in_scene, rpr_light in_light)
{
    if (!in_scene || !in_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //prepare
    Scene1* scene = static_cast<Scene1*>(in_scene);
    Light* light = static_cast<Light*>(in_light);

    scene->DetachLight(light);

    return RPR_SUCCESS;
}

rpr_int rprSceneGetInfo(rpr_scene scene, rpr_scene_info info, size_t size, void * data, size_t * size_ret)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSceneGetEnvironmentOverride(rpr_scene scene, rpr_environment_override overrride, rpr_light * out_light)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSceneSetEnvironmentOverride(rpr_scene scene, rpr_environment_override overrride, rpr_light light)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSceneSetBackgroundImage(rpr_scene scene, rpr_image image)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSceneGetBackgroundImage(rpr_scene scene, rpr_image * out_image)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprSceneSetCamera(rpr_scene in_scene, rpr_camera in_camera)
{
    if (!in_scene)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    Scene1* scene = static_cast<Scene1*>(in_scene);
    PerspectiveCamera* cam = static_cast<PerspectiveCamera*>(in_camera);

    scene->SetCamera(cam);

    return RPR_SUCCESS;
}

rpr_int rprSceneGetCamera(rpr_scene scene, rpr_camera * out_camera)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprFrameBufferGetInfo(rpr_framebuffer in_frame_buffer, rpr_framebuffer_info in_info, size_t in_size, void * out_data, size_t * out_size)
{
    if (!in_frame_buffer)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast data
    ClwOutput* buff = static_cast<ClwOutput*>(in_frame_buffer);
    
    switch (in_info)
    {
    case RPR_FRAMEBUFFER_DATA:
        if (out_data)
            buff->GetData(static_cast<RadeonRays::float3*>(out_data));
        if (out_size)
            *out_size = sizeof(float3) * buff->width() * buff->height();
        break;
    default:
        return RPR_ERROR_UNIMPLEMENTED;
    }

    return RPR_SUCCESS;
}

rpr_int rprFrameBufferClear(rpr_framebuffer in_frame_buffer)
{
    if (!in_frame_buffer)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //prepare
    ClwOutput* buff = static_cast<ClwOutput*>(in_frame_buffer);
    buff->Clear(float3(0.f,0.f,0.f,0.f));

    return RPR_SUCCESS;
}

rpr_int rprFrameBufferSaveToFile(rpr_framebuffer in_frame_buffer, rpr_char const * file_path)
{
    if (!in_frame_buffer)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    Output* buff = static_cast<Output*>(in_frame_buffer);
    OIIO_NAMESPACE_USING;

    int width = buff->width();
    int height = buff->height();
    std::vector<float3> tempbuf(width * height);
    buff->GetData(tempbuf.data());
    std::vector<float3> data(tempbuf);

    for (auto y = 0; y < height; ++y)
    {
        for (auto x = 0; x < width; ++x)
        {

            float3 val = data[(height - 1 - y) * width + x];
            tempbuf[y * width + x] = (1.f / val.w) * val;

            tempbuf[y * width + x].x = std::pow(tempbuf[y * width + x].x, 1.f / 2.2f);
            tempbuf[y * width + x].y = std::pow(tempbuf[y * width + x].y, 1.f / 2.2f);
            tempbuf[y * width + x].z = std::pow(tempbuf[y * width + x].z, 1.f / 2.2f);
        }
    }

    ImageOutput* out = ImageOutput::create(file_path);

    if (!out)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    ImageSpec spec(width, height, 3, TypeDesc::FLOAT);

    out->open(file_path, spec);
    out->write_image(TypeDesc::FLOAT, &tempbuf[0], sizeof(float3));
    out->close();

    return RPR_SUCCESS;
}

rpr_int rprContextResolveFrameBuffer(rpr_context context, rpr_framebuffer src_frame_buffer, rpr_framebuffer dst_frame_buffer, rpr_bool normalizeOnly)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextCreateMaterialSystem(rpr_context in_context, rpr_material_system_type type, rpr_material_system * out_matsys)
{
    if (!in_context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //cast data
    Context* cont = static_cast<Context*>(in_context);
    MaterialSystem* mat_sys = new MaterialSystem();
    *out_matsys = mat_sys;

    return RPR_SUCCESS;
}

rpr_int rprMaterialSystemCreateNode(rpr_material_system in_matsys, rpr_material_node_type in_type, rpr_material_node * out_node)
{
    if (!in_matsys)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //cast data
    MaterialSystem* sys = static_cast<MaterialSystem*>(in_matsys);

    //create material
    rpr_material_node node = nullptr;
    switch (in_type)
    {
    case RPR_MATERIAL_NODE_DIFFUSE:
    case RPR_MATERIAL_NODE_WARD:
    case RPR_MATERIAL_NODE_ORENNAYAR:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
        mat->SetTwoSided(true);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_MICROFACET:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX);
        mat->SetTwoSided(true);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_MICROFACET_REFRACTION:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetRefractionGGX);
        mat->SetTwoSided(true);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_REFLECTION:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kIdealReflect);
        mat->SetTwoSided(true);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_REFRACTION:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kIdealRefract);
        mat->SetTwoSided(true);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_STANDARD:
    {
        //TODO: fix
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
        mat->SetTwoSided(true);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_DIFFUSE_REFRACTION:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kTranslucent);
        mat->SetTwoSided(true);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_FRESNEL_SCHLICK:
    case RPR_MATERIAL_NODE_FRESNEL:
    {
        //TODO: fix
        return RPR_ERROR_UNIMPLEMENTED;
    }
    case RPR_MATERIAL_NODE_EMISSIVE:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kEmissive);
        mat->SetTwoSided(true);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_BLEND:
    {
        MultiBxdf* mat = new MultiBxdf(MultiBxdf::Type::kMix);
        mat->SetTwoSided(true);
        mat->SetInputValue("weight", 0.5f);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_TRANSPARENT:
    case RPR_MATERIAL_NODE_PASSTHROUGH:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kPassthrough);
        mat->SetTwoSided(true);
        node = mat;
        break;
    }
    case RPR_MATERIAL_NODE_ADD:
    case RPR_MATERIAL_NODE_ARITHMETIC:
    case RPR_MATERIAL_NODE_BLEND_VALUE:
    case RPR_MATERIAL_NODE_VOLUME:
    case RPR_MATERIAL_NODE_INPUT_LOOKUP:
        return RPR_ERROR_UNSUPPORTED;
        break;
    case RPR_MATERIAL_NODE_NORMAL_MAP:
    case RPR_MATERIAL_NODE_IMAGE_TEXTURE:
    case RPR_MATERIAL_NODE_NOISE2D_TEXTURE:
    case RPR_MATERIAL_NODE_DOT_TEXTURE:
    case RPR_MATERIAL_NODE_GRADIENT_TEXTURE:
    case RPR_MATERIAL_NODE_CHECKER_TEXTURE:
    case RPR_MATERIAL_NODE_CONSTANT_TEXTURE:
    case RPR_MATERIAL_NODE_BUMP_MAP:
        node = new Texture();
        break;
    default:
        return RPR_ERROR_INVALID_PARAMETER_TYPE;
    }
    sys->push_back(node);
    *out_node = node;
    
    return RPR_SUCCESS;
}

rpr_int rprMaterialNodeSetInputN(rpr_material_node in_node, rpr_char const * in_input, rpr_material_node in_input_node)
{
    if (!in_node || !in_input || !in_input_node)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast data
    SingleBxdf* mat = static_cast<SingleBxdf*>(in_node);
    //in_node cab be Texture(RPR_MATERIAL_NODE_IMAGE_TEXTURE case) or material
    SceneObject* input_node = static_cast<SceneObject*>(in_input_node);
    Texture* input_tex = dynamic_cast<Texture*>(input_node);
    SingleBxdf* input_mat = dynamic_cast<SingleBxdf*>(input_node);
    
    //TODO: move to wrap
    //TODO: handle RPR_MATERIAL_NODE_BLEND
    //convert input name
    std::string input_name;
    if (!strcmp(in_input, "color"))
    {
        input_name = "albedo";
    }
    else if (!strcmp(in_input, "normal"))
    {
        input_name = in_input;
    }
    else if (!strcmp(in_input, "roughness"))
    {
        input_name = in_input;
    }
    else
    {
        return RPR_ERROR_UNIMPLEMENTED;
    }

    if (input_tex)
    {
        mat->SetInputValue(input_name, input_tex);
    }
    else if(input_mat)
    {
        mat->SetInputValue(input_name, input_mat);
    }
    else
    {
        return RPR_ERROR_INVALID_PARAMETER_TYPE;
    }

    return RPR_SUCCESS;
}

rpr_int rprMaterialNodeSetInputF(rpr_material_node in_node, rpr_char const * in_input, rpr_float in_value_x, rpr_float in_value_y, rpr_float in_value_z, rpr_float in_value_w)
{
    if (!in_node)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //cast data
    SingleBxdf* mat = static_cast<SingleBxdf*>(in_node);
    //collect input
    float4 input = { in_value_x, in_value_y, in_value_z, in_value_w };
    std::string input_name;
    
    //translate material prop name
    if (!strcmp(in_input, "color"))
    {
        input_name = "albedo";
    }
    else if (!strcmp(in_input, "ior"))
    {
        input_name = in_input;
    }
    else if (!strcmp(in_input, "roughness"))
    {
        input_name = in_input;
    }
    else
    {
        return RPR_ERROR_UNIMPLEMENTED;
    }

    mat->SetInputValue(input_name, input);
    return RPR_SUCCESS;
}

rpr_int rprMaterialNodeSetInputU(rpr_material_node in_node, rpr_char const * in_input, rpr_uint in_value)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprMaterialNodeSetInputImageData(rpr_material_node in_node, rpr_char const * in_input, rpr_image in_image)
{
    if (!in_node || !in_image)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //get material and texture
    SceneObject* node = static_cast<SceneObject*>(in_node);
    //in_node can be material or Texture(RPR_MATERIAL_NODE_IMAGE_TEXTURE case)
    SingleBxdf* mat = dynamic_cast<SingleBxdf*>(node);
    Texture* image_mat = dynamic_cast<Texture*>(node);

    Texture* tex = static_cast<Texture*>(in_image);

    if (mat)
    {
        std::string name;
        if (!strcmp("color", in_input))
        {
            name = "albedo";
        }
        else
        {
            return RPR_ERROR_UNIMPLEMENTED;
        }
        mat->SetInputValue(name, tex);
    }
    else if (image_mat)
    {
        //shpuld be "data" tag for RPR_MATERIAL_NODE_IMAGE_TEXTURE
        if (strcmp(in_input, "data"))
            return RPR_ERROR_INVALID_TAG;
        //allocate and copy data for texture
        char* tex_data = new char[tex->GetSizeInBytes()];
        memcpy(tex_data, tex->GetData(), tex->GetSizeInBytes());
        image_mat->SetData(tex_data, tex->GetSize(), tex->GetFormat());
    }
    else
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }


    return RPR_SUCCESS;
}

rpr_int rprMaterialNodeGetInfo(rpr_material_node in_node, rpr_material_node_info in_info, size_t in_size, void * in_data, size_t * out_size)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprMaterialNodeGetInputInfo(rpr_material_node in_node, rpr_int in_input_idx, rpr_material_node_input_info in_info, size_t in_size, void * in_data, size_t * out_size)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprObjectDelete(void * obj)
{
    //TODO:
    delete obj;

    return RPR_SUCCESS;
}

rpr_int rprObjectSetName(void * node, rpr_char const * name)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextCreatePostEffect(rpr_context context, rpr_post_effect_type type, rpr_post_effect * out_effect)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextAttachPostEffect(rpr_context context, rpr_post_effect effect)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprContextDetachPostEffect(rpr_context context, rpr_post_effect effect)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprPostEffectSetParameter1u(rpr_post_effect effect, rpr_char const * name, rpr_uint x)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprPostEffectSetParameter1f(rpr_post_effect effect, rpr_char const * name, rpr_float x)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprPostEffectSetParameter3f(rpr_post_effect effect, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z)
{
    return RPR_ERROR_UNIMPLEMENTED;
}

rpr_int rprPostEffectSetParameter4f(rpr_post_effect effect, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z, rpr_float w)
{
    return RPR_ERROR_UNIMPLEMENTED;
}