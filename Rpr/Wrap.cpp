#include "RadeonProRender.h"

fr_int frRegisterPlugin(fr_char const * path)
{
    return rprRegisterPlugin(path);
}

fr_int frCreateContext(fr_int api_version, fr_int * pluginIDs, size_t pluginCount, fr_creation_flags creation_flags, fr_context_properties const * props, fr_char const * cache_path, fr_context * out_context)
{
    return rprCreateContext(api_version, pluginIDs, pluginCount, creation_flags, props, cache_path, out_context);
}

fr_int frContextSetActivePlugin(fr_context context, fr_int pluginID)
{
    return rprContextSetActivePlugin(context, pluginID);
}

fr_int frContextGetInfo(fr_context context, fr_context_info context_info, size_t size, void * data, size_t * size_ret)
{
    return rprContextGetInfo(context, context_info, size, data, size_ret);
}

fr_int frContextGetParameterInfo(fr_context context, int param_idx, fr_parameter_info parameter_info, size_t size, void * data, size_t * size_ret)
{
    return rprContextGetParameterInfo(context, param_idx, parameter_info, size, data, size_ret);
}

fr_int frContextGetAOV(fr_context context, fr_aov aov, fr_framebuffer * out_fb)
{
    return rprContextGetAOV(context, aov, out_fb);
}

fr_int frContextSetAOV(fr_context context, fr_aov aov, fr_framebuffer frame_buffer)
{
    return rprContextSetAOV(context, aov, frame_buffer);
}

fr_int frContextSetScene(fr_context context, fr_scene scene)
{
    return rprContextSetScene(context, scene);
}

fr_int frContextGetScene(fr_context arg0, fr_scene * out_scene)
{
    return rprContextGetScene(arg0, out_scene);
}

fr_int frContextSetParameter1u(fr_context context, fr_char const * name, fr_uint x)
{
    return rprContextSetParameter1u(context, name, x);
}

fr_int frContextSetParameter1f(fr_context context, fr_char const * name, fr_float x)
{
    return rprContextSetParameter1f(context, name, x);
}

fr_int frContextSetParameter3f(fr_context context, fr_char const * name, fr_float x, fr_float y, fr_float z)
{
    return rprContextSetParameter3f(context, name, x, y, z);
}

fr_int frContextSetParameter4f(fr_context context, fr_char const * name, fr_float x, fr_float y, fr_float z, fr_float w)
{
    return rprContextSetParameter4f(context, name, x, y, z, w);
}

fr_int frContextSetParameterString(fr_context context, fr_char const * name, fr_char const * value)
{
    return rprContextSetParameterString(context, name, value);
}

fr_int frContextRender(fr_context context)
{
    return rprContextRender(context);
}

fr_int frContextRenderTile(fr_context context, fr_uint xmin, fr_uint xmax, fr_uint ymin, fr_uint ymax)
{
    return rprContextRenderTile(context, xmin, xmax, ymin, ymax);
}

fr_int frContextClearMemory(fr_context context)
{
    return rprContextClearMemory(context);
}

fr_int frContextCreateImage(fr_context context, fr_image_format const format, fr_image_desc const * image_desc, void const * data, fr_image * out_image)
{
    return rprContextCreateImage(context, format, image_desc, data, out_image);
}

fr_int frContextCreateImageFromFile(fr_context context, fr_char const * path, fr_image * out_image)
{
    return rprContextCreateImageFromFile(context, path, out_image);
}

fr_int frContextCreateScene(fr_context context, fr_scene * out_scene)
{
    return rprContextCreateScene(context, out_scene);
}

fr_int frContextCreateInstance(fr_context context, fr_shape shape, fr_shape * out_instance)
{
    return rprContextCreateInstance(context, shape, out_instance);
}

fr_int frContextCreateMesh(fr_context context, fr_float const * vertices, size_t num_vertices, fr_int vertex_stride, fr_float const * normals, size_t num_normals, fr_int normal_stride, fr_float const * texcoords, size_t num_texcoords, fr_int texcoord_stride, fr_int const * vertex_indices, fr_int vidx_stride, fr_int const * normal_indices, fr_int nidx_stride, fr_int const * texcoord_indices, fr_int tidx_stride, fr_int const * num_face_vertices, size_t num_faces, fr_shape * out_mesh)
{
    return rprContextCreateMesh(context, vertices, num_vertices, vertex_stride, normals, num_normals, normal_stride, texcoords, num_texcoords, texcoord_stride, vertex_indices, vidx_stride, normal_indices, nidx_stride, texcoord_indices, tidx_stride, num_face_vertices, num_faces, out_mesh);
}

fr_int frContextCreateMeshEx(fr_context context, fr_float const * vertices, size_t num_vertices, fr_int vertex_stride, fr_float const * normals, size_t num_normals, fr_int normal_stride, fr_int const * perVertexFlag, size_t num_perVertexFlags, fr_int perVertexFlag_stride, fr_int numberOfTexCoordLayers, fr_float const ** texcoords, size_t * num_texcoords, fr_int * texcoord_stride, fr_int const * vertex_indices, fr_int vidx_stride, fr_int const * normal_indices, fr_int nidx_stride, fr_int const ** texcoord_indices, fr_int * tidx_stride, fr_int const * num_face_vertices, size_t num_faces, fr_shape * out_mesh)
{
    return rprContextCreateMeshEx(context, vertices, num_vertices, vertex_stride, normals, num_normals, normal_stride, perVertexFlag, num_perVertexFlags, perVertexFlag_stride, numberOfTexCoordLayers, texcoords, num_texcoords, texcoord_stride, vertex_indices, vidx_stride, normal_indices, nidx_stride, texcoord_indices, tidx_stride, num_face_vertices, num_faces, out_mesh);
}

fr_int frContextCreateCamera(fr_context context, fr_camera * out_camera)
{
    return rprContextCreateCamera(context, out_camera);
}

fr_int frContextCreateFrameBuffer(fr_context context, fr_framebuffer_format const format, fr_framebuffer_desc const * fb_desc, fr_framebuffer * out_fb)
{
    return rprContextCreateFrameBuffer(context, format, fb_desc, out_fb);
}

fr_int frCameraGetInfo(fr_camera camera, fr_camera_info camera_info, size_t size, void * data, size_t * size_ret)
{
    return rprCameraGetInfo(camera, camera_info, size, data, size_ret);
}

fr_int frCameraSetFocalLength(fr_camera camera, fr_float flength)
{
    return rprCameraSetFocalLength(camera, flength);
}

fr_int frCameraSetFocusDistance(fr_camera camera, fr_float fdist)
{
    return rprCameraSetFocusDistance(camera, fdist);
}

fr_int frCameraSetTransform(fr_camera camera, fr_bool transpose, fr_float * transform)
{
    return rprCameraSetTransform(camera, transpose, transform);
}

fr_int frCameraSetSensorSize(fr_camera camera, fr_float width, fr_float height)
{
    return rprCameraSetSensorSize(camera, width, height);
}

fr_int frCameraLookAt(fr_camera camera, fr_float posx, fr_float posy, fr_float posz, fr_float atx, fr_float aty, fr_float atz, fr_float upx, fr_float upy, fr_float upz)
{
    return rprCameraLookAt(camera, posx, posy, posz, atx, aty, atz, upx, upy, upz);
}

fr_int frCameraSetFStop(fr_camera camera, fr_float fstop)
{
    return rprCameraSetFStop(camera, fstop);
}

fr_int frCameraSetApertureBlades(fr_camera camera, fr_uint num_blades)
{
    return rprCameraSetApertureBlades(camera, num_blades);
}

fr_int frCameraSetExposure(fr_camera camera, fr_float exposure)
{
    return rprCameraSetExposure(camera, exposure);
}

fr_int frCameraSetMode(fr_camera camera, fr_camera_mode mode)
{
    return rprCameraSetMode(camera, mode);
}

fr_int frCameraSetOrthoWidth(fr_camera camera, fr_float width)
{
    return rprCameraSetOrthoWidth(camera, width);
}

fr_int frCameraSetFocalTilt(fr_camera camera, fr_float tilt)
{
    return rprCameraSetFocalTilt(camera, tilt);
}

fr_int frCameraSetIPD(fr_camera camera, fr_float ipd)
{
    return rprCameraSetIPD(camera, ipd);
}

fr_int frCameraSetLensShift(fr_camera camera, fr_float shiftx, fr_float shifty)
{
    return rprCameraSetLensShift(camera, shiftx, shifty);
}

fr_int frCameraSetOrthoHeight(fr_camera camera, fr_float height)
{
    return rprCameraSetOrthoHeight(camera, height);
}

fr_int frImageGetInfo(fr_image image, fr_image_info image_info, size_t size, void * data, size_t * size_ret)
{
    return rprImageGetInfo(image, image_info, size, data, size_ret);
}

fr_int frImageSetWrap(fr_image image, fr_image_wrap_type type)
{
    return rprImageSetWrap(image, type);
}

fr_int frShapeSetTransform(fr_shape shape, fr_bool transpose, fr_float const * transform)
{
    return rprShapeSetTransform(shape, transpose, transform);
}

fr_int frShapeSetSubdivisionFactor(fr_shape shape, fr_uint factor)
{
    return rprShapeSetSubdivisionFactor(shape, factor);
}

fr_int frShapeSetSubdivisionCreaseWeight(fr_shape shape, fr_float factor)
{
    return rprShapeSetSubdivisionCreaseWeight(shape, factor);
}

fr_int frShapeSetSubdivisionBoundaryInterop(fr_shape shape, fr_subdiv_boundary_interfop_type type)
{
    return rprShapeSetSubdivisionBoundaryInterop(shape, type);
}

fr_int frShapeSetDisplacementScale(fr_shape shape, fr_float minscale, fr_float maxscale)
{
    return rprShapeSetDisplacementScale(shape, minscale, maxscale);
}

fr_int frShapeSetObjectGroupID(fr_shape shape, fr_uint objectGroupID)
{
    return rprShapeSetObjectGroupID(shape, objectGroupID);
}

fr_int frShapeSetDisplacementImage(fr_shape shape, fr_image image)
{
    return rprShapeSetDisplacementImage(shape, image);
}

fr_int frShapeSetMaterial(fr_shape shape, fr_material_node node)
{
    return rprShapeSetMaterial(shape, node);
}

fr_int frShapeSetMaterialOverride(fr_shape shape, fr_material_node node)
{
    return rprShapeSetMaterialOverride(shape, node);
}

fr_int frShapeSetVolumeMaterial(fr_shape shape, fr_material_node node)
{
    return rprShapeSetVolumeMaterial(shape, node);
}

fr_int frShapeSetLinearMotion(fr_shape shape, fr_float x, fr_float y, fr_float z)
{
    return rprShapeSetLinearMotion(shape, x, y, z);
}

fr_int frShapeSetAngularMotion(fr_shape shape, fr_float x, fr_float y, fr_float z, fr_float w)
{
    return rprShapeSetAngularMotion(shape, x, y, z, w);
}

fr_int frShapeSetVisibility(fr_shape shape, fr_bool visible)
{
    return rprShapeSetVisibility(shape, visible);
}

fr_int frShapeSetVisibilityPrimaryOnly(fr_shape shape, fr_bool visible)
{
    return rprShapeSetVisibilityPrimaryOnly(shape, visible);
}

fr_int frShapeSetVisibilityInSpecular(fr_shape shape, fr_bool visible)
{
    return rprShapeSetVisibilityInSpecular(shape, visible);
}

fr_int frShapeSetShadowCatcher(fr_shape shape, fr_bool shadowCatcher)
{
    return rprShapeSetShadowCatcher(shape, shadowCatcher);
}

fr_int frShapeSetShadow(fr_shape shape, fr_bool casts_shadow)
{
    return rprShapeSetShadow(shape, casts_shadow);
}

fr_int frLightSetTransform(fr_light light, fr_bool transpose, fr_float const * transform)
{
    return rprLightSetTransform(light, transpose, transform);
}

fr_int frShapeGetInfo(fr_shape arg0, fr_shape_info arg1, size_t arg2, void * arg3, size_t * arg4)
{
    return rprShapeGetInfo(arg0, arg1, arg2, arg3, arg4);
}

fr_int frMeshGetInfo(fr_shape mesh, fr_mesh_info mesh_info, size_t size, void * data, size_t * size_ret)
{
    return rprMeshGetInfo(mesh, mesh_info, size, data, size_ret);
}

fr_int frMeshPolygonGetInfo(fr_shape mesh, size_t polygon_index, fr_mesh_polygon_info polygon_info, size_t size, void * data, size_t * size_ret)
{
    return rprMeshPolygonGetInfo(mesh, polygon_index, polygon_info, size, data, size_ret);
}

fr_int frInstanceGetBaseShape(fr_shape shape, fr_shape * out_shape)
{
    return rprInstanceGetBaseShape(shape, out_shape);
}

fr_int frContextCreatePointLight(fr_context context, fr_light * out_light)
{
    return rprContextCreatePointLight(context, out_light);
}

fr_int frPointLightSetRadiantPower3f(fr_light light, fr_float r, fr_float g, fr_float b)
{
    return rprPointLightSetRadiantPower3f(light, r, g, b);
}

fr_int frContextCreateSpotLight(fr_context context, fr_light * light)
{
    return rprContextCreateSpotLight(context, light);
}

fr_int frSpotLightSetRadiantPower3f(fr_light light, fr_float r, fr_float g, fr_float b)
{
    return rprSpotLightSetRadiantPower3f(light, r, g, b);
}

fr_int frSpotLightSetConeShape(fr_light light, fr_float iangle, fr_float oangle)
{
    return rprSpotLightSetConeShape(light, iangle, oangle);
}

fr_int frContextCreateDirectionalLight(fr_context context, fr_light * out_light)
{
    return rprContextCreateDirectionalLight(context, out_light);
}

fr_int frDirectionalLightSetRadiantPower3f(fr_light light, fr_float r, fr_float g, fr_float b)
{
    return rprDirectionalLightSetRadiantPower3f(light, r, g, b);
}

fr_int frDirectionalLightSetShadowSoftness(fr_light light, fr_float coeff)
{
    return rprDirectionalLightSetShadowSoftness(light, coeff);
}

fr_int frContextCreateEnvironmentLight(fr_context context, fr_light * out_light)
{
    return rprContextCreateEnvironmentLight(context, out_light);
}

fr_int frEnvironmentLightSetImage(fr_light env_light, fr_image image)
{
    return rprEnvironmentLightSetImage(env_light, image);
}

fr_int frEnvironmentLightSetIntensityScale(fr_light env_light, fr_float intensity_scale)
{
    return rprEnvironmentLightSetIntensityScale(env_light, intensity_scale);
}

fr_int frEnvironmentLightAttachPortal(fr_light env_light, fr_shape portal)
{
    return rprEnvironmentLightAttachPortal(env_light, portal);
}

fr_int frEnvironmentLightDetachPortal(fr_light env_light, fr_shape portal)
{
    return rprEnvironmentLightDetachPortal(env_light, portal);
}

fr_int frContextCreateSkyLight(fr_context context, fr_light * out_light)
{
    return rprContextCreateSkyLight(context, out_light);
}

fr_int frSkyLightSetTurbidity(fr_light skylight, fr_float turbidity)
{
    return rprSkyLightSetTurbidity(skylight, turbidity);
}

fr_int frSkyLightSetAlbedo(fr_light skylight, fr_float albedo)
{
    return rprSkyLightSetAlbedo(skylight, albedo);
}

fr_int frSkyLightSetScale(fr_light skylight, fr_float scale)
{
    return rprSkyLightSetScale(skylight, scale);
}

fr_int frSkyLightAttachPortal(fr_light skylight, fr_shape portal)
{
    return rprSkyLightAttachPortal(skylight, portal);
}

fr_int frSkyLightDetachPortal(fr_light skylight, fr_shape portal)
{
    return rprSkyLightDetachPortal(skylight, portal);
}

fr_int frContextCreateIESLight(fr_context context, fr_light * light)
{
    return rprContextCreateIESLight(context, light);
}

fr_int frIESLightSetRadiantPower3f(fr_light light, fr_float r, fr_float g, fr_float b)
{
    return rprIESLightSetRadiantPower3f(light, r, g, b);
}

fr_int frIESLightSetImageFromFile(fr_light env_light, fr_char const * imagePath, fr_int nx, fr_int ny)
{
    return rprIESLightSetImageFromFile(env_light, imagePath, nx, ny);
}

fr_int frIESLightSetImageFromIESdata(fr_light env_light, fr_char const * iesData, fr_int nx, fr_int ny)
{
    return rprIESLightSetImageFromIESdata(env_light, iesData, nx, ny);
}

fr_int frLightGetInfo(fr_light light, fr_light_info info, size_t size, void * data, size_t * size_ret)
{
    return rprLightGetInfo(light, info, size, data, size_ret);
}

fr_int frSceneClear(fr_scene scene)
{
    return rprSceneClear(scene);
}

fr_int frSceneAttachShape(fr_scene scene, fr_shape shape)
{
    return rprSceneAttachShape(scene, shape);
}

fr_int frSceneDetachShape(fr_scene scene, fr_shape shape)
{
    return rprSceneDetachShape(scene, shape);
}

fr_int frSceneAttachLight(fr_scene scene, fr_light light)
{
    return rprSceneAttachLight(scene, light);
}

fr_int frSceneDetachLight(fr_scene scene, fr_light light)
{
    return rprSceneDetachLight(scene, light);
}

fr_int frSceneGetInfo(fr_scene scene, fr_scene_info info, size_t size, void * data, size_t * size_ret)
{
    return rprSceneGetInfo(scene, info, size, data, size_ret);
}

fr_int frSceneGetEnvironmentOverride(fr_scene scene, fr_environment_override overrride, fr_light * out_light)
{
    return rprSceneGetEnvironmentOverride(scene, overrride, out_light);
}

fr_int frSceneSetEnvironmentOverride(fr_scene scene, fr_environment_override overrride, fr_light light)
{
    return rprSceneSetEnvironmentOverride(scene, overrride, light);
}

fr_int frSceneSetBackgroundImage(fr_scene scene, fr_image image)
{
    return rprSceneSetBackgroundImage(scene, image);
}

fr_int frSceneGetBackgroundImage(fr_scene scene, fr_image * out_image)
{
    return rprSceneGetBackgroundImage(scene, out_image);
}

fr_int frSceneSetCamera(fr_scene scene, fr_camera camera)
{
    return rprSceneSetCamera(scene, camera);
}

fr_int frSceneGetCamera(fr_scene scene, fr_camera * out_camera)
{
    return rprSceneGetCamera(scene, out_camera);
}

fr_int frFrameBufferGetInfo(fr_framebuffer framebuffer, fr_framebuffer_info info, size_t size, void * data, size_t * size_ret)
{
    return rprFrameBufferGetInfo(framebuffer, info, size, data, size_ret);
}

fr_int frFrameBufferClear(fr_framebuffer frame_buffer)
{
    return rprFrameBufferClear(frame_buffer);
}

fr_int frFrameBufferSaveToFile(fr_framebuffer frame_buffer, fr_char const * file_path)
{
    return rprFrameBufferSaveToFile(frame_buffer, file_path);
}

fr_int frContextResolveFrameBuffer(fr_context context, fr_framebuffer src_frame_buffer, fr_framebuffer dst_frame_buffer, fr_bool normalizeOnly)
{
    return rprContextResolveFrameBuffer(context, src_frame_buffer, dst_frame_buffer, normalizeOnly);
}

fr_int frContextCreateMaterialSystem(fr_context in_context, fr_material_system_type type, fr_material_system * out_matsys)
{
    return rprContextCreateMaterialSystem(in_context, type, out_matsys);
}

fr_int frMaterialSystemCreateNode(fr_material_system in_matsys, fr_material_node_type in_type, fr_material_node * out_node)
{
    return rprMaterialSystemCreateNode(in_matsys, in_type, out_node);
}

fr_int frMaterialNodeSetInputN(fr_material_node in_node, fr_char const * in_input, fr_material_node in_input_node)
{
    return rprMaterialNodeSetInputN(in_node, in_input, in_input_node);
}

fr_int frMaterialNodeSetInputF(fr_material_node in_node, fr_char const * in_input, fr_float in_value_x, fr_float in_value_y, fr_float in_value_z, fr_float in_value_w)
{
    return rprMaterialNodeSetInputF(in_node, in_input, in_value_x, in_value_y, in_value_z, in_value_w);
}

fr_int frMaterialNodeSetInputU(fr_material_node in_node, fr_char const * in_input, fr_uint in_value)
{
    return rprMaterialNodeSetInputU(in_node, in_input, in_value);
}

fr_int frMaterialNodeSetInputImageData(fr_material_node in_node, fr_char const * in_input, fr_image image)
{
    return rprMaterialNodeSetInputImageData(in_node, in_input, image);
}

fr_int frMaterialNodeGetInfo(fr_material_node in_node, fr_material_node_info in_info, size_t in_size, void * in_data, size_t * out_size)
{
    return rprMaterialNodeGetInfo(in_node, in_info, in_size, in_data, out_size);
}

fr_int frMaterialNodeGetInputInfo(fr_material_node in_node, fr_int in_input_idx, fr_material_node_input_info in_info, size_t in_size, void * in_data, size_t * out_size)
{
    return rprMaterialNodeGetInputInfo(in_node, in_input_idx, in_info, in_size, in_data, out_size);
}

fr_int frObjectDelete(void * obj)
{
    return rprObjectDelete(obj);
}

fr_int frObjectSetName(void * node, fr_char const * name)
{
    return rprObjectSetName(node, name);
}

fr_int frContextCreatePostEffect(fr_context context, fr_post_effect_type type, fr_post_effect * out_effect)
{
    return rprContextCreatePostEffect(context, type, out_effect);
}

fr_int frContextAttachPostEffect(fr_context context, fr_post_effect effect)
{
    return rprContextAttachPostEffect(context, effect);
}

fr_int frContextDetachPostEffect(fr_context context, fr_post_effect effect)
{
    return rprContextDetachPostEffect(context, effect);
}

fr_int frPostEffectSetParameter1u(fr_post_effect effect, fr_char const * name, fr_uint x)
{
    return rprPostEffectSetParameter1u(effect, name, x);
}

fr_int frPostEffectSetParameter1f(fr_post_effect effect, fr_char const * name, fr_float x)
{
    return rprPostEffectSetParameter1f(effect, name, x);
}

fr_int frPostEffectSetParameter3f(fr_post_effect effect, fr_char const * name, fr_float x, fr_float y, fr_float z)
{
    return rprPostEffectSetParameter3f(effect, name, x, y, z);
}

fr_int frPostEffectSetParameter4f(fr_post_effect effect, fr_char const * name, fr_float x, fr_float y, fr_float z, fr_float w)
{
    return rprPostEffectSetParameter4f(effect, name, x, y, z, w);
}

//TODO: RadeonProRender_GL.h
//fr_int frContextCreateFramebufferFromGLTexture2D(fr_context context, fr_GLenum target, fr_GLint miplevel, fr_GLuint texture, fr_framebuffer * out_fb)
//{
//    return rprContextCreateFramebufferFromGLTexture2D(context, target, miplevel, texture, out_fb);
//}

