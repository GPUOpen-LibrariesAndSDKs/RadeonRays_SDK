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
#include "WrapObject/WrapObject.h"
#include "WrapObject/ContextObject.h"
#include "WrapObject/CameraObject.h"
#include "WrapObject/FramebufferObject.h"
#include "WrapObject/LightObject.h"
#include "WrapObject/MaterialObject.h"
#include "WrapObject/MatSysObject.h"
#include "WrapObject/SceneObject.h"
#include "WrapObject/ShapeObject.h"
#include "WrapObject/Exception.h"
#include "math/float2.h"
#include "math/float3.h"
#include "math/matrix.h"
#include "math/mathutils.h"

//defines behaviour for unimplemented API part
//#define UNIMLEMENTED_FUNCTION return RPR_SUCCESS;
#define UNIMLEMENTED_FUNCTION return RPR_ERROR_UNIMPLEMENTED;

#define UNSUPPORTED_FUNCTION return RPR_SUCCESS;
//#define UNSUPPORTED_FUNCTION return RPR_ERROR_UNSUPPORTED;

rpr_int rprRegisterPlugin(rpr_char const * path)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprCreateContext(rpr_int api_version, rpr_int * pluginIDs, size_t pluginCount, rpr_creation_flags creation_flags, rpr_context_properties const * props, rpr_char const * cache_path, rpr_context * out_context)
{
    if (api_version != RPR_API_VERSION)
    {
        return RPR_ERROR_INVALID_API_VERSION;
    }

    rpr_int result = RPR_SUCCESS;
    try
    {
        //TODO: handle other options
        *out_context = new ContextObject(creation_flags);
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }
    return result;
}

rpr_int rprContextSetActivePlugin(rpr_context context, rpr_int pluginID)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextGetInfo(rpr_context in_context, rpr_context_info in_context_info, size_t in_size, void * out_data, size_t * out_size_ret)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    switch (in_context_info)
    {
    case RPR_CONTEXT_RENDER_STATISTICS:
        context->GetRenderStatistics(out_data, out_size_ret);
        break;
    case RPR_CONTEXT_PARAMETER_COUNT:
        break;
    case RPR_OBJECT_NAME:
    {
        std::string name = context->GetName();
        if (out_data)
        {
            memcpy(out_data, name.c_str(), name.size() + 1);
        }
        if (out_size_ret)
        {
            *out_size_ret = name.size() + 1;
        }
        break;
    }
    default:
        UNIMLEMENTED_FUNCTION

    }
    return RPR_SUCCESS;
}

rpr_int rprContextGetParameterInfo(rpr_context context, int param_idx, rpr_parameter_info parameter_info, size_t size, void * data, size_t * size_ret)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextGetAOV(rpr_context in_context, rpr_aov in_aov, rpr_framebuffer * out_fb)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    rpr_int result = RPR_SUCCESS;
    try
    {
        *out_fb = context->GetAOV(in_aov);
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }

    return result;
}

rpr_int rprContextSetAOV(rpr_context in_context, rpr_aov in_aov, rpr_framebuffer in_frame_buffer)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    FramebufferObject* buffer = WrapObject::Cast<FramebufferObject>(in_frame_buffer);

    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    if (!buffer)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    rpr_int result = RPR_SUCCESS;
    try
    {
        context->SetAOV(in_aov, buffer);
    }
    catch(Exception& e)
    {
        result = e.m_error;
    }

    return result;
}

rpr_int rprContextSetScene(rpr_context in_context, rpr_scene in_scene)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    SceneObject* scene = WrapObject::Cast<SceneObject>(in_scene);

    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    context->SetCurrenScene(scene);

    return RPR_SUCCESS;
}


rpr_int rprContextGetScene(rpr_context in_context, rpr_scene * out_scene)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    SceneObject* scene = context->GetCurrentScene();
    *out_scene = scene;

    return RPR_SUCCESS;
}

rpr_int rprContextSetParameter1u(rpr_context in_context, rpr_char const * name, rpr_uint x)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    //TODO: handle context parameters
	return RPR_SUCCESS;

    if (!strcmp(name, "rendermode"))
    {
        switch (x)
        {
        case RPR_RENDER_MODE_GLOBAL_ILLUMINATION:
            break;
        default:
            UNIMLEMENTED_FUNCTION
        }
    }
    else
    {
        UNIMLEMENTED_FUNCTION
    }

    return RPR_SUCCESS;
}

rpr_int rprContextSetParameter1f(rpr_context in_context, rpr_char const * name, rpr_float x)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    try
    {
        context->SetParameter(name, x);
    }
    catch (Exception& e)
    {
        return e.m_error;
    }

    return RPR_SUCCESS;
}

rpr_int rprContextSetParameter3f(rpr_context in_context, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    try
    {
        context->SetParameter(name, x, y, z);
    }
    catch (Exception& e)
    {
        return e.m_error;
    }

    return RPR_SUCCESS;
}

rpr_int rprContextSetParameter4f(rpr_context in_context, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z, rpr_float w)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    try
    {
        context->SetParameter(name, x, y, z, w);
    }
    catch (Exception& e)
    {
        return e.m_error;
    }

    return RPR_SUCCESS;
}

rpr_int rprContextSetParameterString(rpr_context in_context, rpr_char const * name, rpr_char const * value)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    try
    {
        context->SetParameter(name, value);
    }
    catch (Exception& e)
    {
        return e.m_error;
    }

    return RPR_SUCCESS;
}

rpr_int rprContextRender(rpr_context in_context)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    context->Render();

    return RPR_SUCCESS;
}

rpr_int rprContextRenderTile(rpr_context in_context, rpr_uint xmin, rpr_uint xmax, rpr_uint ymin, rpr_uint ymax)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    context->RenderTile(xmin, xmax, ymin, ymax);

    return RPR_SUCCESS;
}

rpr_int rprContextClearMemory(rpr_context context)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextCreateImage(rpr_context in_context, rpr_image_format const in_format, rpr_image_desc const * in_image_desc, void const * in_data, rpr_image * out_image)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    rpr_int result = RPR_SUCCESS;
    try
    {
        *out_image = context->CreateTexture(in_format, in_image_desc, in_data);
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }

    return result;
}

rpr_int rprContextCreateImageFromFile(rpr_context in_context, rpr_char const * in_path, rpr_image * out_image)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    rpr_int result = RPR_SUCCESS;
    try
    {
        *out_image = context->CreateTextureFromFile(in_path);
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }

    return result;
}

rpr_int rprContextCreateScene(rpr_context in_context, rpr_scene * out_scene)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    rpr_int result = RPR_SUCCESS;
    try
    {
        *out_scene = context->CreateScene();
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }

    return result;
}

rpr_int rprContextCreateInstance(rpr_context in_context, rpr_shape shape, rpr_shape * out_instance)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    ShapeObject* mesh = WrapObject::Cast<ShapeObject>(shape);

    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    
    if (!mesh || !out_instance)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    rpr_int result = RPR_SUCCESS;
    try
    {
        *out_instance = context->CreateShapeInstance(mesh);
    }
    catch (Exception& e)
    {
        result = e.m_error;
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
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    rpr_int result = RPR_SUCCESS;
    try
    {
        *out_mesh = context->CreateShape(in_vertices, in_num_vertices, in_vertex_stride,
                                        in_normals, in_num_normals, in_normal_stride,
                                        in_texcoords, in_num_texcoords, in_texcoord_stride,
                                        in_vertex_indices, in_vidx_stride,
                                        in_normal_indices, in_nidx_stride,
                                        in_texcoord_indices, in_tidx_stride,
                                        in_num_face_vertices, in_num_faces);
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }

    return result;
}

rpr_int rprContextCreateMeshEx(rpr_context context, 
                                rpr_float const * vertices, size_t num_vertices, rpr_int vertex_stride, 
                                rpr_float const * normals, size_t num_normals, rpr_int normal_stride, 
                                rpr_int const * perVertexFlag, size_t num_perVertexFlags, rpr_int perVertexFlag_stride, 
                                rpr_int numberOfTexCoordLayers, 
                                rpr_float const ** texcoords, size_t * num_texcoords, rpr_int * texcoord_stride, 
                                rpr_int const * vertex_indices, rpr_int vidx_stride, 
                                rpr_int const * normal_indices, rpr_int nidx_stride, 
                                rpr_int const ** texcoord_indices, rpr_int * tidx_stride, 
                                rpr_int const * num_face_vertices, size_t num_faces, rpr_shape * out_mesh)
{
    if (num_perVertexFlags == 0 && numberOfTexCoordLayers == 1)
    {
        //can use rprContextCreateMesh
        return rprContextCreateMesh(context,
                                vertices, num_vertices, vertex_stride,
                                normals, num_normals, normal_stride,
                                texcoords[0], num_texcoords[0], texcoord_stride[0],
                                vertex_indices, vidx_stride,
                                normal_indices, nidx_stride,
                                texcoord_indices[0], tidx_stride[0],
                                num_face_vertices, num_faces, out_mesh);
    }
    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextCreateCamera(rpr_context in_context, rpr_camera * out_camera)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    rpr_int result = RPR_SUCCESS;
    try
    {
        *out_camera = context->CreateCamera();
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }
    return result;
}

rpr_int rprContextCreateFrameBuffer(rpr_context in_context, rpr_framebuffer_format const in_format, rpr_framebuffer_desc const * in_fb_desc, rpr_framebuffer * out_fb)
{
    //cast data
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    rpr_int result = RPR_SUCCESS;
    try
    {
        *out_fb = context->CreateFrameBuffer(in_format, in_fb_desc);
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }
    return result;

    return RPR_SUCCESS;
}

rpr_int rprCameraGetInfo(rpr_camera in_camera, rpr_camera_info in_camera_info, size_t in_size, void * out_data, size_t * out_size_ret)
{
    CameraObject* cam = WrapObject::Cast<CameraObject>(in_camera);
    if (!cam)
    {
        return RPR_ERROR_INVALID_OBJECT;
    }

    std::vector<char> data;
    size_t size_ret = 0;
    switch (in_camera_info)
    {
    case RPR_CAMERA_FSTOP:
    {
        rpr_float value = cam->GetAperture();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_CAMERA_FOCAL_LENGTH:
    {
        rpr_float value = cam->GetFocalLength();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_CAMERA_SENSOR_SIZE:
    {
        RadeonRays::float2 value = cam->GetSensorSize();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_CAMERA_MODE:
    {
        //TODO: only prespective camera supported now
        rpr_camera_mode value = RPR_CAMERA_MODE_PERSPECTIVE;
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_CAMERA_FOCUS_DISTANCE:
    {
        //TODO: only prespective camera supported now
        rpr_float value = cam->GetFocusDistance();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_CAMERA_POSITION:
    {
        RadeonRays::float3 pos;
        RadeonRays::float3 at;
        RadeonRays::float3 up;
        cam->GetLookAt(pos, at, up);
        size_ret = sizeof(pos);
        data.resize(size_ret);
        memcpy(&data[0], &pos, size_ret);
        break;
    }
    case RPR_CAMERA_LOOKAT:
    {
        RadeonRays::float3 pos;
        RadeonRays::float3 at;
        RadeonRays::float3 up;
        cam->GetLookAt(pos, at, up);
        size_ret = sizeof(at);
        data.resize(size_ret);
        memcpy(&data[0], &at, size_ret);
        break;
    }
    case RPR_CAMERA_UP:
    {
        RadeonRays::float3 pos;
        RadeonRays::float3 at;
        RadeonRays::float3 up;
        cam->GetLookAt(pos, at, up);
        size_ret = sizeof(up);
        data.resize(size_ret);
        memcpy(&data[0], &up, size_ret);
        break;
    }
    case RPR_OBJECT_NAME:
    {
        std::string name = cam->GetName();
        size_ret = name.size() + 1;
        data.resize(size_ret);
        memcpy(&data[0], name.c_str(), size_ret);
        break;
    }
    case RPR_CAMERA_APERTURE_BLADES:
    case RPR_CAMERA_EXPOSURE:
    case RPR_CAMERA_ORTHO_WIDTH:
    case RPR_CAMERA_FOCAL_TILT:
    case RPR_CAMERA_IPD:
    case RPR_CAMERA_LENS_SHIFT:
    case RPR_CAMERA_ORTHO_HEIGHT:

        UNSUPPORTED_FUNCTION
        break;
    default:
        UNIMLEMENTED_FUNCTION
    }

    if (out_size_ret)
    {
        *out_size_ret = size_ret;
    }
    if (out_data)
    {
        memcpy(out_data, &data[0], size_ret);
    }
    return RPR_SUCCESS;
}

rpr_int rprCameraSetFocalLength(rpr_camera in_camera, rpr_float flength)
{
    //cast data
    CameraObject* camera = WrapObject::Cast<CameraObject>(in_camera);
    if (!camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //translate meters to mm 
    camera->SetFocalLength(flength);

    return RPR_SUCCESS;
}

rpr_int rprCameraSetFocusDistance(rpr_camera in_camera, rpr_float fdist)
{
    //cast data
    CameraObject* camera = WrapObject::Cast<CameraObject>(in_camera);
    if (!camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    camera->SetFocusDistance(fdist);

    return RPR_SUCCESS;
}

rpr_int rprCameraSetTransform(rpr_camera in_camera, rpr_bool transpose, rpr_float * transform)
{
	//cast data
	CameraObject* camera = WrapObject::Cast<CameraObject>(in_camera);
	if (!camera)
	{
		return RPR_ERROR_INVALID_PARAMETER;
	}

	RadeonRays::matrix m;
	//fill matrix
	memcpy(m.m, transform, 16 * sizeof(rpr_float));

	if (!transpose)
	{
		m = m.transpose();
	}

	camera->SetTransform(m);
	return RPR_SUCCESS;
}

rpr_int rprCameraSetSensorSize(rpr_camera in_camera, rpr_float in_width, rpr_float in_height)
{
    //cast data
    CameraObject* camera = WrapObject::Cast<CameraObject>(in_camera);
    if (!camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //convedrt meters to mm
    RadeonRays::float2 size = { in_width, in_height};
    camera->SetSensorSize(size);

    return RPR_SUCCESS;
}

rpr_int rprCameraLookAt(rpr_camera in_camera, rpr_float posx, rpr_float posy, rpr_float posz, rpr_float atx, rpr_float aty, rpr_float atz, rpr_float upx, rpr_float upy, rpr_float upz)
{
    //cast data
    CameraObject* camera = WrapObject::Cast<CameraObject>(in_camera);
    if (!camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //get camera and set values
    const RadeonRays::float3 pos = { posx, posy, posz };
    const RadeonRays::float3 at = { atx, aty, atz };
    const RadeonRays::float3 up = { upx, upy, upz};
    camera->LookAt(pos, at, up);

    return RPR_SUCCESS;
}

rpr_int rprCameraSetFStop(rpr_camera in_camera, rpr_float fstop)
{
    //cast data
    CameraObject* camera = WrapObject::Cast<CameraObject>(in_camera);
    if (!camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //translate m -> to mm
    camera->SetAperture(fstop);

    return RPR_SUCCESS;
}

rpr_int rprCameraSetApertureBlades(rpr_camera camera, rpr_uint num_blades)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprCameraSetExposure(rpr_camera camera, rpr_float exposure)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprCameraSetMode(rpr_camera in_camera, rpr_camera_mode mode)
{
    //cast data
    CameraObject* camera = WrapObject::Cast<CameraObject>(in_camera);
    if (!camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //TODO: only perspective camera used for now
    switch (mode)
    {
    case RPR_CAMERA_MODE_PERSPECTIVE:
        break;
    default:
        UNIMLEMENTED_FUNCTION
    }
    
    return RPR_SUCCESS;
}

rpr_int rprCameraSetOrthoWidth(rpr_camera camera, rpr_float width)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprCameraSetFocalTilt(rpr_camera camera, rpr_float tilt)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprCameraSetIPD(rpr_camera camera, rpr_float ipd)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprCameraSetLensShift(rpr_camera camera, rpr_float shiftx, rpr_float shifty)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprCameraSetOrthoHeight(rpr_camera camera, rpr_float height)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprImageGetInfo(rpr_image in_image, rpr_image_info in_image_info, size_t in_size, void * in_data, size_t * in_size_ret)
{
    MaterialObject* img = WrapObject::Cast<MaterialObject>(in_image);
    if (!img || !img->IsImg())
    {
        return RPR_ERROR_INVALID_IMAGE;
    }

    std::vector<char> data;
    size_t size_ret = 0;
    switch (in_image_info)
    {
    case RPR_IMAGE_FORMAT:
    {
        //texture data always stored as 4 component FLOAT32
        rpr_image_format value = img->GetTextureFormat();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_IMAGE_DESC:
    {
        rpr_image_desc value = img->GetTextureDesc();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_IMAGE_DATA:
    {
        rpr_image_desc desc = img->GetTextureDesc();
        const char* value = img->GetTextureData();
        size_ret = desc.image_width * desc.image_height * desc.image_depth;
        data.resize(size_ret);
        memcpy(&data[0], value, size_ret);
        break;
    }
    case RPR_OBJECT_NAME:
    {
        std::string name = img->GetName();
        size_ret = name.size() + 1;
        data.resize(size_ret);
        memcpy(&data[0], name.c_str(), size_ret);
        break;
    }
    case RPR_IMAGE_WRAP:
        UNSUPPORTED_FUNCTION
        break;
    default:
        UNIMLEMENTED_FUNCTION
    }

    if (in_size_ret)
    {
        *in_size_ret = size_ret;
    }
    if (in_data)
    {
        memcpy(in_data, &data[0], size_ret);
    }

    return RPR_SUCCESS;
}

rpr_int rprImageSetWrap(rpr_image image, rpr_image_wrap_type type)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetTransform(rpr_shape in_shape, rpr_bool transpose, rpr_float const * transform)
{
    //cast data
    ShapeObject* shape = WrapObject::Cast<ShapeObject>(in_shape);
    if (!shape)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    RadeonRays::matrix m;
    //fill matrix
    memcpy(m.m, transform, 16  * sizeof(rpr_float));

    if (!transpose)
    {
        m = m.transpose();
    }

    shape->SetTransform(m);
    return RPR_SUCCESS;
}

rpr_int rprShapeSetSubdivisionFactor(rpr_shape shape, rpr_uint factor)
{	
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetSubdivisionCreaseWeight(rpr_shape shape, rpr_float factor)
{	
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetSubdivisionBoundaryInterop(rpr_shape shape, rpr_subdiv_boundary_interfop_type type)
{	
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetDisplacementScale(rpr_shape shape, rpr_float minscale, rpr_float maxscale)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetObjectGroupID(rpr_shape shape, rpr_uint objectGroupID)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetDisplacementImage(rpr_shape shape, rpr_image image)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetMaterial(rpr_shape in_shape, rpr_material_node in_node)
{
    //cast data
    ShapeObject* shape = WrapObject::Cast<ShapeObject>(in_shape);
    MaterialObject* mat = WrapObject::Cast<MaterialObject>(in_node);
    if (!shape)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    
    rpr_int result = RPR_SUCCESS;
    try
    {
        //can throw exception if mat is a Texture
        shape->SetMaterial(mat);
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }
    return result;
}

rpr_int rprShapeSetMaterialOverride(rpr_shape shape, rpr_material_node node)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetVolumeMaterial(rpr_shape shape, rpr_material_node node)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetLinearMotion(rpr_shape in_shape, rpr_float x, rpr_float y, rpr_float z)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetAngularMotion(rpr_shape shape, rpr_float x, rpr_float y, rpr_float z, rpr_float w)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetVisibility(rpr_shape shape, rpr_bool visible)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetVisibilityPrimaryOnly(rpr_shape shape, rpr_bool visible)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetVisibilityInSpecular(rpr_shape shape, rpr_bool visible)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetShadowCatcher(rpr_shape shape, rpr_bool shadowCatcher)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprShapeSetShadow(rpr_shape shape, rpr_bool casts_shadow)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprLightSetTransform(rpr_light in_light, rpr_bool in_transpose, rpr_float const * in_transform)
{
    //cast data
    LightObject* light = WrapObject::Cast<LightObject>(in_light);
    if (!light)
        return RPR_ERROR_INVALID_PARAMETER;
    
    RadeonRays::matrix m;
    memcpy(&m.m00, in_transform, 16 * sizeof(rpr_float));
    if (!in_transpose)
    {
        m = m.transpose();
    }

    light->SetTransform(m);

    return RPR_SUCCESS;
}

rpr_int rprShapeGetInfo(rpr_shape in_shape, rpr_shape_info in_info, size_t in_size, void * in_data, size_t * in_size_ret)
{
    ShapeObject* shape = WrapObject::Cast<ShapeObject>(in_shape);
    if (!shape)
    {
        return RPR_ERROR_INVALID_OBJECT;
    }

    std::vector<char> data;
    size_t size_ret = 0;
    switch (in_info)
    {
    case RPR_SHAPE_TYPE:
    {   
        int value = shape->IsInstance() ? RPR_SHAPE_TYPE_INSTANCE : RPR_SHAPE_TYPE_MESH;
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_SHAPE_MATERIAL:
    {
        MaterialObject* value = shape->GetMaterial();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_SHAPE_TRANSFORM:
    {
        RadeonRays::matrix value = shape->GetTransform();
        value = value.transpose();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_OBJECT_NAME:
    {
        std::string name = shape->GetName();
        size_ret = name.size() + 1;
        data.resize(size_ret);
        memcpy(&data[0], name.c_str(), size_ret);
        break;
    }
    //these properties of shape are unsupported
    case RPR_SHAPE_LINEAR_MOTION:
    case RPR_SHAPE_ANGULAR_MOTION:
    case RPR_SHAPE_VISIBILITY_FLAG:
    case RPR_SHAPE_SHADOW_FLAG:
    case RPR_SHAPE_SHADOW_CATCHER_FLAG:
    case RPR_SHAPE_SUBDIVISION_FACTOR:
    case RPR_SHAPE_SUBDIVISION_CREASEWEIGHT:
    case RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP:
    case RPR_SHAPE_DISPLACEMENT_SCALE:
    case RPR_SHAPE_OBJECT_GROUP_ID:
    case RPR_SHAPE_DISPLACEMENT_IMAGE:
        UNSUPPORTED_FUNCTION
    default:
        UNIMLEMENTED_FUNCTION
    }

    if (in_size_ret)
    {
        *in_size_ret = size_ret;
    }
    if (in_data)
    {
        memcpy(in_data, &data[0], size_ret);
    }
    return RPR_SUCCESS;
}

rpr_int rprMeshGetInfo(rpr_shape in_mesh, rpr_mesh_info in_mesh_info, size_t in_size, void * in_data, size_t * in_size_ret)
{
    ShapeObject* mesh = WrapObject::Cast<ShapeObject>(in_mesh);
    if (!mesh || mesh->IsInstance())
    {
        return RPR_ERROR_INVALID_OBJECT;
    }

    std::vector<char> data;
    size_t size_ret = 0;
    switch (in_mesh_info)
    {
    case RPR_MESH_POLYGON_COUNT:
    {
        uint64_t value = mesh->GetVertexCount() / 3;
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_MESH_VERTEX_COUNT:
    {
        uint64_t value = mesh->GetVertexCount();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_MESH_NORMAL_COUNT:
    {
        uint64_t value = mesh->GetNormalCount();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_MESH_UV_COUNT:
    {
        uint64_t value = mesh->GetUVCount();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_MESH_UV2_COUNT:
    {
        //UV2 unsupported
        uint64_t value = 0;
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_MESH_VERTEX_ARRAY:
    {
        size_ret = sizeof(float) * 3 * mesh->GetVertexCount();
        data.resize(size_ret);
        mesh->GetVertexData(reinterpret_cast<float*>(data.data()));
        break;
    }
    case RPR_MESH_NORMAL_ARRAY:
    {
        size_ret = sizeof(float) * 3 * mesh->GetNormalCount();
        data.resize(size_ret);
        mesh->GetNormalData(reinterpret_cast<float*>(data.data()));
        break;

    }
    case RPR_MESH_UV_ARRAY:
    {
        const RadeonRays::float2* value = mesh->GetUVData();
        size_ret = sizeof(RadeonRays::float2) * mesh->GetUVCount();
        data.resize(size_ret);
        memcpy(&data[0], value, size_ret);
        break;
    }
    case RPR_MESH_VERTEX_INDEX_ARRAY:
    case RPR_MESH_UV_INDEX_ARRAY:
    case RPR_MESH_NORMAL_INDEX_ARRAY:
    {
        const uint32_t* value = mesh->GetIndicesData();
        size_ret = sizeof(uint32_t) * mesh->GetIndicesCount();
        data.resize(size_ret);
        memcpy(&data[0], value, size_ret);
        break;
    }
    case RPR_MESH_NUM_FACE_VERTICES_ARRAY:
    {
        //only triangles used in Baikal mesh
        std::vector<int32_t> value(mesh->GetIndicesCount() / 3, 3);
        size_ret = sizeof(int32_t) * value.size();
        data.resize(size_ret);
        memcpy(&data[0], value.data(), size_ret);
        break;
    }
    default:
        UNIMLEMENTED_FUNCTION
    }

    if (in_size_ret)
    {
        *in_size_ret = size_ret;
    }
    if (in_data)
    {
        memcpy(in_data, &data[0], size_ret);
    }
    return RPR_SUCCESS;
}

rpr_int rprMeshPolygonGetInfo(rpr_shape mesh, size_t polygon_index, rpr_mesh_polygon_info polygon_info, size_t size, void * data, size_t * size_ret)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprInstanceGetBaseShape(rpr_shape in_shape, rpr_shape * out_shape)
{
    ShapeObject* instance = WrapObject::Cast<ShapeObject>(in_shape);
    if (!instance || !instance->IsInstance())
    {
        return RPR_ERROR_INVALID_OBJECT;
    }

    *out_shape = instance->GetBaseShape();
    return RPR_SUCCESS;
}

rpr_int rprContextCreatePointLight(rpr_context in_context, rpr_light * out_light)
{
    //cast
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);

    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    if (!out_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    *out_light = context->CreateLight(LightObject::Type::kPointLight);

    return RPR_SUCCESS;
}

rpr_int rprPointLightSetRadiantPower3f(rpr_light in_light, rpr_float in_r, rpr_float in_g, rpr_float in_b)
{
    //cast
    LightObject* light = WrapObject::Cast<LightObject>(in_light);
    if (!light || light->GetType() != LightObject::Type::kPointLight)
    {
        return RPR_ERROR_INVALID_LIGHT;
    }

    RadeonRays::float3 radiant_power = { in_r, in_g, in_b };
    light->SetRadiantPower(radiant_power);

    return RPR_SUCCESS;
}

rpr_int rprContextCreateSpotLight(rpr_context in_context, rpr_light * out_light)
{
    //cast
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    if (!out_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    *out_light = context->CreateLight(LightObject::Type::kSpotLight);

    return RPR_SUCCESS;
}

rpr_int rprSpotLightSetRadiantPower3f(rpr_light in_light, rpr_float in_r, rpr_float in_g, rpr_float in_b)
{
    //cast
    LightObject* light = WrapObject::Cast<LightObject>(in_light);
    if (!light || light->GetType() != LightObject::Type::kSpotLight)
    {
        return RPR_ERROR_INVALID_LIGHT;
    }

    RadeonRays::float3 radiant_power = { in_r, in_g, in_b };
    light->SetRadiantPower(radiant_power);

    return RPR_SUCCESS;
}

rpr_int rprSpotLightSetConeShape(rpr_light in_light, rpr_float in_iangle, rpr_float in_oangle)
{
    //cast
    LightObject* light = WrapObject::Cast<LightObject>(in_light);
    if (!light || light->GetType() != LightObject::Type::kSpotLight)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //set angles
    RadeonRays::float2 angle = {in_iangle, in_oangle};
    rpr_int result = RPR_SUCCESS;
    try
    {
        light->SetSpotConeShape(angle);
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }
    return result;
}

rpr_int rprContextCreateDirectionalLight(rpr_context in_context, rpr_light * out_light)
{
    //cast
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    if (!out_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    *out_light = context->CreateLight(LightObject::Type::kDirectionalLight);

    return RPR_SUCCESS;
}

rpr_int rprDirectionalLightSetRadiantPower3f(rpr_light in_light, rpr_float in_r, rpr_float in_g, rpr_float in_b)
{
    //cast
    LightObject* light = WrapObject::Cast<LightObject>(in_light);
    if (!light || light->GetType() != LightObject::Type::kDirectionalLight)
    {
        return RPR_ERROR_INVALID_LIGHT;
    }

    RadeonRays::float3 radiant_power = { in_r, in_g, in_b };
    light->SetRadiantPower(radiant_power);

    return RPR_SUCCESS;
}

rpr_int rprDirectionalLightSetShadowSoftness(rpr_light in_light, rpr_float in_coeff)
{
    //cast
    LightObject* light = WrapObject::Cast<LightObject>(in_light);
    if (!light || light->GetType() != LightObject::Type::kDirectionalLight)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextCreateEnvironmentLight(rpr_context in_context, rpr_light * out_light)
{
    //cast
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }
    if (!out_light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    *out_light = context->CreateLight(LightObject::Type::kEnvironmentLight);

    return RPR_SUCCESS;
}

rpr_int rprEnvironmentLightSetImage(rpr_light in_env_light, rpr_image in_image)
{
    //cast
    LightObject* light = WrapObject::Cast<LightObject>(in_env_light);
    MaterialObject* img = WrapObject::Cast<MaterialObject>(in_image);
    if (!light || light->GetType() != LightObject::Type::kEnvironmentLight || !img || !img->IsImg())
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    rpr_int result = RPR_SUCCESS;
    try
    {
        //set image
        light->SetEnvTexture(img);
    }
    catch (Exception& e)
    {
        result = e.m_error;
    }
    return result;
}

rpr_int rprEnvironmentLightSetIntensityScale(rpr_light in_env_light, rpr_float intensity_scale)
{
    //cast
    LightObject* light = WrapObject::Cast<LightObject>(in_env_light);
    if (!light || light->GetType() != LightObject::Type::kEnvironmentLight)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    
    //set data
    light->SetEnvMultiplier(intensity_scale);

    return RPR_SUCCESS;
}

rpr_int rprEnvironmentLightAttachPortal(rpr_light env_light, rpr_shape portal)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprEnvironmentLightDetachPortal(rpr_light env_light, rpr_shape portal)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextCreateSkyLight(rpr_context context, rpr_light * out_light)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprSkyLightSetTurbidity(rpr_light skylight, rpr_float turbidity)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprSkyLightSetAlbedo(rpr_light skylight, rpr_float albedo)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprSkyLightSetScale(rpr_light skylight, rpr_float scale)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprSkyLightAttachPortal(rpr_light skylight, rpr_shape portal)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprSkyLightDetachPortal(rpr_light skylight, rpr_shape portal)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextCreateIESLight(rpr_context context, rpr_light * light)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprIESLightSetRadiantPower3f(rpr_light light, rpr_float r, rpr_float g, rpr_float b)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprIESLightSetImageFromFile(rpr_light env_light, rpr_char const * imagePath, rpr_int nx, rpr_int ny)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprIESLightSetImageFromIESdata(rpr_light env_light, rpr_char const * iesData, rpr_int nx, rpr_int ny)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprLightGetInfo(rpr_light in_light, rpr_light_info in_info, size_t in_size, void * out_data, size_t * out_size_ret)
{
    //cast
    LightObject* light = WrapObject::Cast<LightObject>(in_light);
    if (!light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    std::vector<char> data;
    size_t size_ret = 0;
    switch (in_info)
    {
    case RPR_LIGHT_TYPE:
    {
        rpr_light_type value = (rpr_light_type)light->GetType();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_LIGHT_TRANSFORM:
    {
        RadeonRays::matrix value = light->GetTransform();
        value = value.transpose();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_ENVIRONMENT_LIGHT_IMAGE:
    {
        MaterialObject* value = light->GetEnvTexture();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_POINT_LIGHT_RADIANT_POWER:
    case RPR_SPOT_LIGHT_RADIANT_POWER:
    case RPR_DIRECTIONAL_LIGHT_RADIANT_POWER:
    {
        RadeonRays::float3 value = light->GetRadiantPower();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_SPOT_LIGHT_CONE_SHAPE:
    {
        RadeonRays::float2 value = light->GetSpotConeShape();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_ENVIRONMENT_LIGHT_INTENSITY_SCALE:
    {
        rpr_float value = light->GetEnvMultiplier();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_OBJECT_NAME:
    {
        std::string name = light->GetName();
        size_ret = name.size() + 1;
        data.resize(size_ret);
        memcpy(&data[0], name.c_str(), size_ret);
        break;
    }
    case RPR_DIRECTIONAL_LIGHT_SHADOW_SOFTNESS:
    case RPR_ENVIRONMENT_LIGHT_PORTAL_COUNT:
    case RPR_ENVIRONMENT_LIGHT_PORTAL_LIST:
    case RPR_SKY_LIGHT_SCALE:
    case RPR_SKY_LIGHT_ALBEDO:
    case RPR_SKY_LIGHT_TURBIDITY:
    case RPR_SKY_LIGHT_PORTAL_COUNT:
    case RPR_SKY_LIGHT_PORTAL_LIST:
    case RPR_IES_LIGHT_RADIANT_POWER:
    case RPR_IES_LIGHT_IMAGE_DESC:        
        UNSUPPORTED_FUNCTION
        break;
    default:
        return RPR_ERROR_INVALID_PARAMETER;
    }

    if (out_size_ret)
    {
        *out_size_ret = size_ret;
    }
    if (out_data)
    {
        memcpy(out_data, &data[0], size_ret);
    }

    return RPR_SUCCESS;
}

rpr_int rprSceneClear(rpr_scene in_scene)
{
    //cast
    SceneObject* scene = WrapObject::Cast<SceneObject>(in_scene);
    if (!scene)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    scene->Clear();
    return RPR_SUCCESS;
}

rpr_int rprSceneAttachShape(rpr_scene in_scene, rpr_shape in_shape)
{
    //cast
    SceneObject* scene = WrapObject::Cast<SceneObject>(in_scene);
    ShapeObject* shape = WrapObject::Cast<ShapeObject>(in_shape);
    if (!scene || !shape)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast input data
    scene->AttachShape(shape);

    return RPR_SUCCESS;
}

rpr_int rprSceneDetachShape(rpr_scene in_scene, rpr_shape in_shape)
{
    //cast
    SceneObject* scene = WrapObject::Cast<SceneObject>(in_scene);
    ShapeObject* shape = WrapObject::Cast<ShapeObject>(in_shape);
    if (!scene || !shape)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //cast input data
    scene->DetachShape(shape);

    return RPR_SUCCESS;
}

rpr_int rprSceneAttachLight(rpr_scene in_scene, rpr_light in_light)
{
    //cast
    SceneObject* scene = WrapObject::Cast<SceneObject>(in_scene);
    LightObject* light = WrapObject::Cast<LightObject>(in_light);
    if (!scene || !light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //attach
    scene->AttachLight(light);

    return RPR_SUCCESS;
}

rpr_int rprSceneDetachLight(rpr_scene in_scene, rpr_light in_light)
{
    //cast
    SceneObject* scene = WrapObject::Cast<SceneObject>(in_scene);
    LightObject* light = WrapObject::Cast<LightObject>(in_light);
    if (!scene || !light)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //attach
    scene->DetachLight(light);

    return RPR_SUCCESS;
}

rpr_int rprSceneGetInfo(rpr_scene in_scene, rpr_scene_info in_info, size_t in_size, void * out_data, size_t * out_size_ret)
{
	//cast
	SceneObject* scene = WrapObject::Cast<SceneObject>(in_scene);
	if (!scene)
	{
		return RPR_ERROR_INVALID_PARAMETER;
	}
	
    std::vector<char> data;
    size_t size_ret = 0;
    switch (in_info)
    {
    case RPR_SCENE_SHAPE_COUNT:
    {
        size_t value = scene->GetShapeCount();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_SCENE_SHAPE_LIST:
    {
        size_t value = scene->GetShapeCount();
        size_ret = sizeof(rpr_shape) * value;
        data.resize(size_ret);
        scene->GetShapeList(data.data());
        break;
    }
    case RPR_SCENE_LIGHT_COUNT:
    {
        size_t value = scene->GetLightCount();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_SCENE_LIGHT_LIST:
    {
        size_t value = scene->GetLightCount();
        size_ret = sizeof(rpr_light) * value;
        data.resize(size_ret);
        scene->GetLightList(data.data());
        break;
    }
    case RPR_SCENE_CAMERA:
    {
        rpr_camera value = scene->GetCamera();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_OBJECT_NAME:
    {
        std::string name = scene->GetName();
        size_ret = name.size() + 1;
        data.resize(size_ret);
        memcpy(&data[0], name.c_str(), size_ret);
        break;
    }
    case RPR_SCENE_BACKGROUND_IMAGE:
    case RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION:
    case RPR_SCENE_ENVIRONMENT_OVERRIDE_REFRACTION:
    case RPR_SCENE_ENVIRONMENT_OVERRIDE_TRANSPARENCY:
    case RPR_SCENE_ENVIRONMENT_OVERRIDE_BACKGROUND:
    case RPR_SCENE_AXIS_ALIGNED_BOUNDING_BOX:
        UNSUPPORTED_FUNCTION
    default:
        UNIMLEMENTED_FUNCTION
    }

    if (out_size_ret)
    {
        *out_size_ret = size_ret;
    }
    if (out_data)
    {
        memcpy(out_data, &data[0], size_ret);
    }

    return RPR_SUCCESS;
}

rpr_int rprSceneGetEnvironmentOverride(rpr_scene scene, rpr_environment_override overrride, rpr_light * out_light)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprSceneSetEnvironmentOverride(rpr_scene scene, rpr_environment_override overrride, rpr_light light)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprSceneSetBackgroundImage(rpr_scene scene, rpr_image image)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprSceneGetBackgroundImage(rpr_scene scene, rpr_image * out_image)
{
    UNSUPPORTED_FUNCTION
}

rpr_int rprSceneSetCamera(rpr_scene in_scene, rpr_camera in_camera)
{
    //cast
    SceneObject* scene = WrapObject::Cast<SceneObject>(in_scene);
    CameraObject* camera = WrapObject::Cast<CameraObject>(in_camera);
    if (!scene)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    scene->SetCamera(camera);

    return RPR_SUCCESS;
}

rpr_int rprSceneGetCamera(rpr_scene in_scene, rpr_camera * out_camera)
{
    //cast
    SceneObject* scene = WrapObject::Cast<SceneObject>(in_scene);
    if (!scene || !out_camera)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    *out_camera = scene->GetCamera();

    return RPR_SUCCESS;
}

rpr_int rprFrameBufferGetInfo(rpr_framebuffer in_frame_buffer, rpr_framebuffer_info in_info, size_t in_size, void * out_data, size_t * out_size)
{
    //cast
    FramebufferObject* buff = WrapObject::Cast<FramebufferObject>(in_frame_buffer);
    if (!buff)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    int buff_size = sizeof(RadeonRays::float3) * buff->GetWidth() * buff->GetHeight();
    switch (in_info)
    {
    case RPR_FRAMEBUFFER_DATA:
        if (out_size)
        {
            *out_size = buff_size;
        }
        if (out_data && in_size < buff_size)
        {
            return RPR_ERROR_INVALID_PARAMETER;
        }
        if (out_data)
        {
            buff->GetData(static_cast<RadeonRays::float3*>(out_data));
        }
        break;
    default:
        UNIMLEMENTED_FUNCTION
    }

    return RPR_SUCCESS;
}

rpr_int rprFrameBufferClear(rpr_framebuffer in_frame_buffer)
{
    //cast
    FramebufferObject* buff = WrapObject::Cast<FramebufferObject>(in_frame_buffer);
    if (!buff)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    buff->Clear();

    return RPR_SUCCESS;
}

rpr_int rprFrameBufferSaveToFile(rpr_framebuffer in_frame_buffer, rpr_char const * file_path)
{
    //cast
    FramebufferObject* buff = WrapObject::Cast<FramebufferObject>(in_frame_buffer);
    if (!buff)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    try
    {
        buff->SaveToFile(file_path);
    }
    catch (Exception& e)
    {
        return e.m_error;
    }
    return RPR_SUCCESS;
}

rpr_int rprContextResolveFrameBuffer(rpr_context context, rpr_framebuffer src_frame_buffer, rpr_framebuffer dst_frame_buffer, rpr_bool normalizeOnly)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextCreateMaterialSystem(rpr_context in_context, rpr_material_system_type type, rpr_material_system * out_matsys)
{
    //cast
    ContextObject* context = WrapObject::Cast<ContextObject>(in_context);
    if (!context)
    {
        return RPR_ERROR_INVALID_CONTEXT;
    }

    *out_matsys = context->CreateMaterialSystem();

    return RPR_SUCCESS;
}

rpr_int rprMaterialSystemCreateNode(rpr_material_system in_matsys, rpr_material_node_type in_type, rpr_material_node * out_node)
{
    //cast
    MatSysObject* sys = WrapObject::Cast<MatSysObject>(in_matsys);
    if (!sys)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    
    //create material
    try
    {
        *out_node = sys->CreateMaterial(in_type);
    }
    catch (Exception& e)
    {
        return e.m_error;
    }
    return RPR_SUCCESS;
}

rpr_int rprMaterialNodeSetInputN(rpr_material_node in_node, rpr_char const * in_input, rpr_material_node in_input_node)
{
    //cast
    MaterialObject* mat = WrapObject::Cast<MaterialObject>(in_node);
    MaterialObject* input_node = WrapObject::Cast<MaterialObject>(in_input_node);

    if (!mat || !in_input || !input_node)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    try
    {
        mat->SetInputMaterial(in_input, input_node);
    }
    catch (Exception& e)
    {
        return e.m_error;
    }
    return RPR_SUCCESS;
}

rpr_int rprMaterialNodeSetInputF(rpr_material_node in_node, rpr_char const * in_input, rpr_float in_value_x, rpr_float in_value_y, rpr_float in_value_z, rpr_float in_value_w)
{
    //cast
    MaterialObject* mat = WrapObject::Cast<MaterialObject>(in_node);
    if (!mat)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    //collect input
    RadeonRays::float4 input = { in_value_x, in_value_y, in_value_z, in_value_w };
    
    try
    {
        mat->SetInputValue(in_input, input);
    }
    catch (Exception& e)
    {
        return e.m_error;
    }
    return RPR_SUCCESS;
}

rpr_int rprMaterialNodeSetInputU(rpr_material_node in_node, rpr_char const * in_input, rpr_uint in_value)
{
	UNIMLEMENTED_FUNCTION
}

rpr_int rprMaterialNodeSetInputImageData(rpr_material_node in_node, rpr_char const * in_input, rpr_image in_image)
{
    //cast
    MaterialObject* mat = WrapObject::Cast<MaterialObject>(in_node);
    MaterialObject* img = WrapObject::Cast<MaterialObject>(in_image);
    if (!mat || !img || !img->IsImg())
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }
    
    try
    {
        mat->SetInputMaterial(in_input, img);
    }
    catch (Exception& e)
    {
        return e.m_error;
    }

    return RPR_SUCCESS;
}

rpr_int rprMaterialNodeGetInfo(rpr_material_node in_node, rpr_material_node_info in_info, size_t in_size, void * out_data, size_t * out_size)
{
    MaterialObject* mat = WrapObject::Cast<MaterialObject>(in_node);
    if (!mat)
    {
        return RPR_ERROR_INVALID_OBJECT;
    }

    std::vector<char> data;
    size_t size_ret = 0;
    switch (in_info)
    {
    case RPR_MATERIAL_NODE_TYPE:
    {
        rpr_material_node_type value = mat->GetType();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_MATERIAL_NODE_INPUT_COUNT:
    {
        uint64_t value = mat->GetInputCount();
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_OBJECT_NAME:
    {
        std::string name = mat->GetName();
        size_ret = name.size() + 1;
        data.resize(size_ret);
        memcpy(&data[0], name.c_str(), size_ret);
        break;
    }
    default:
        UNIMLEMENTED_FUNCTION
    }

    if (out_size)
    {
        *out_size = size_ret;
    }
    if (out_data)
    {
        memcpy(out_data, &data[0], size_ret);
    }
    return RPR_SUCCESS;
}

rpr_int rprMaterialNodeGetInputInfo(rpr_material_node in_node, rpr_int in_input_idx, rpr_material_node_input_info in_info, size_t in_size, void * in_data, size_t * in_out_size)
{
    MaterialObject* mat = WrapObject::Cast<MaterialObject>(in_node);
    if (!mat)
    {
        return RPR_ERROR_INVALID_OBJECT;
    }

    std::vector<char> data;
    size_t size_ret = 0;
    switch (in_info)
    {
    case RPR_MATERIAL_NODE_INPUT_NAME_STRING:
    {
        std::string name = mat->GetInputName(in_input_idx);
        size_ret = name.size() + 1;
        data.resize(size_ret);
        memcpy(&data[0], name.data(), size_ret);
        break;
    }
    case RPR_MATERIAL_NODE_INPUT_TYPE:
    {
        rpr_uint value = mat->GetInputType(in_input_idx);
        size_ret = sizeof(value);
        data.resize(size_ret);
        memcpy(&data[0], &value, size_ret);
        break;
    }
    case RPR_MATERIAL_NODE_INPUT_VALUE:
    {
        //sizeof(RadeonRays::float4) should be enough to store any input data
        data.resize(sizeof(RadeonRays::float4));
        mat->GetInput(in_input_idx, data.data(), &size_ret);
        break;
    }
    default:
        UNIMLEMENTED_FUNCTION
    }

    if (in_out_size)
    {
        *in_out_size = size_ret;
    }
    if (in_data)
    {
        memcpy(in_data, &data[0], size_ret);
    }
    return RPR_SUCCESS;
}

rpr_int rprObjectDelete(void * in_obj)
{
    WrapObject* obj = static_cast<WrapObject*>(in_obj);
    delete obj;

    return RPR_SUCCESS;
}

rpr_int rprObjectSetName(void * in_node, rpr_char const * in_name)
{
    //check nullptr
    if (!in_node)
    {
        return RPR_ERROR_INVALID_OBJECT;
    }

    if (!in_name)
    {
        return RPR_ERROR_INVALID_PARAMETER;
    }

    //set name
    WrapObject* node = WrapObject::Cast<WrapObject>(in_node);
    node->SetName(in_name);

    return RPR_SUCCESS;
}

rpr_int rprContextCreatePostEffect(rpr_context context, rpr_post_effect_type type, rpr_post_effect * out_effect)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextAttachPostEffect(rpr_context context, rpr_post_effect effect)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprContextDetachPostEffect(rpr_context context, rpr_post_effect effect)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprPostEffectSetParameter1u(rpr_post_effect effect, rpr_char const * name, rpr_uint x)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprPostEffectSetParameter1f(rpr_post_effect effect, rpr_char const * name, rpr_float x)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprPostEffectSetParameter3f(rpr_post_effect effect, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z)
{
    UNIMLEMENTED_FUNCTION
}

rpr_int rprPostEffectSetParameter4f(rpr_post_effect effect, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z, rpr_float w)
{
    UNIMLEMENTED_FUNCTION
}
