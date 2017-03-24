/*****************************************************************************\
*
*  Module Name    FireRender.h
*  Project        FireRender Engine
*
*  Description    Fire Render Interface header
*
*  Copyright 2015 Advanced Micro Devices, Inc.
*
*  All rights reserved.  This notice is intended as a precaution against
*  inadvertent publication and does not imply publication or any waiver
*  of confidentiality.  The year included in the foregoing notice is the
*  year of creation of the work.
*  @author Dmitry Kozlov (dmitry.kozlov@amd.com)
*  @author Takahiro Harada (takahiro.harada@amd.com)
*  @bug No known bugs.
*
\*****************************************************************************/
#ifndef __RADEONPRORENDER_H
#define __RADEONPRORENDER_H

#define RPR_API_ENTRY

#ifdef __cplusplus
extern "C" {
#endif

#include "cstddef"

#define RPR_API_VERSION 0x010000252 

/* rpr_status */
#define RPR_SUCCESS 0 
#define RPR_ERROR_COMPUTE_API_NOT_SUPPORTED -1 
#define RPR_ERROR_OUT_OF_SYSTEM_MEMORY -2 
#define RPR_ERROR_OUT_OF_VIDEO_MEMORY -3 
#define RPR_ERROR_INVALID_LIGHTPATH_EXPR -5 
#define RPR_ERROR_INVALID_IMAGE -6 
#define RPR_ERROR_INVALID_AA_METHOD -7 
#define RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT -8 
#define RPR_ERROR_INVALID_GL_TEXTURE -9 
#define RPR_ERROR_INVALID_CL_IMAGE -10 
#define RPR_ERROR_INVALID_OBJECT -11 
#define RPR_ERROR_INVALID_PARAMETER -12 
#define RPR_ERROR_INVALID_TAG -13 
#define RPR_ERROR_INVALID_LIGHT -14 
#define RPR_ERROR_INVALID_CONTEXT -15 
#define RPR_ERROR_UNIMPLEMENTED -16 
#define RPR_ERROR_INVALID_API_VERSION -17 
#define RPR_ERROR_INTERNAL_ERROR -18 
#define RPR_ERROR_IO_ERROR -19 
#define RPR_ERROR_UNSUPPORTED_SHADER_PARAMETER_TYPE -20 
#define RPR_ERROR_MATERIAL_STACK_OVERFLOW -21 
#define RPR_ERROR_INVALID_PARAMETER_TYPE -22 
#define RPR_ERROR_UNSUPPORTED -23 
#define RPR_ERROR_OPENCL_OUT_OF_HOST_MEMORY -24 

/* rpr_parameter_type */
#define RPR_PARAMETER_TYPE_FLOAT 0x1 
#define RPR_PARAMETER_TYPE_FLOAT2 0x2 
#define RPR_PARAMETER_TYPE_FLOAT3 0x3 
#define RPR_PARAMETER_TYPE_FLOAT4 0x4 
#define RPR_PARAMETER_TYPE_IMAGE 0x5 
#define RPR_PARAMETER_TYPE_STRING 0x6 
#define RPR_PARAMETER_TYPE_SHADER 0x7 
#define RPR_PARAMETER_TYPE_UINT 0x8 

/* rpr_image_type */
#define RPR_IMAGE_TYPE_1D 0x1 
#define RPR_IMAGE_TYPE_2D 0x2 
#define RPR_IMAGE_TYPE_3D 0x3 

/* rpr_context_type */
#define RPR_CONTEXT_OPENCL (1 << 0) 
#define RPR_CONTEXT_DIRECTCOMPUTE (1 << 1) 
#define RPR_CONTEXT_REFERENCE (1 << 2) 
#define RPR_CONTEXT_OPENGL (1 << 3) 
#define RPR_CONTEXT_METAL (1 << 4) 

/* rpr_creation_flags */
#define RPR_CREATION_FLAGS_ENABLE_GPU0 (1 << 0) 
#define RPR_CREATION_FLAGS_ENABLE_GPU1 (1 << 1) 
#define RPR_CREATION_FLAGS_ENABLE_GPU2 (1 << 2) 
#define RPR_CREATION_FLAGS_ENABLE_GPU3 (1 << 3) 
#define RPR_CREATION_FLAGS_ENABLE_CPU (1 << 4) 
#define RPR_CREATION_FLAGS_ENABLE_GL_INTEROP (1 << 5) 
#define RPR_CREATION_FLAGS_ENABLE_GPU4 (1 << 6) 
#define RPR_CREATION_FLAGS_ENABLE_GPU5 (1 << 7) 
#define RPR_CREATION_FLAGS_ENABLE_GPU6 (1 << 8) 
#define RPR_CREATION_FLAGS_ENABLE_GPU7 (1 << 9) 

/* rpr_aa_filter */
#define RPR_FILTER_BOX 0x1 
#define RPR_FILTER_TRIANGLE 0x2 
#define RPR_FILTER_GAUSSIAN 0x3 
#define RPR_FILTER_MITCHELL 0x4 
#define RPR_FILTER_LANCZOS 0x5 
#define RPR_FILTER_BLACKMANHARRIS 0x6 

/* rpr_shape_type */
#define RPR_SHAPE_TYPE_MESH 0x1 
#define RPR_SHAPE_TYPE_INSTANCE 0x2 

/* rpr_light_type */
#define RPR_LIGHT_TYPE_POINT 0x1 
#define RPR_LIGHT_TYPE_DIRECTIONAL 0x2 
#define RPR_LIGHT_TYPE_SPOT 0x3 
#define RPR_LIGHT_TYPE_ENVIRONMENT 0x4 
#define RPR_LIGHT_TYPE_SKY 0x5 
#define RPR_LIGHT_TYPE_IES 0x6 

/* rpr_object_info */
#define RPR_OBJECT_NAME 0x777777 

/* rpr_context_info */
#define RPR_CONTEXT_CREATION_FLAGS 0x102 
#define RPR_CONTEXT_CACHE_PATH 0x103 
#define RPR_CONTEXT_RENDER_STATUS 0x104 
#define RPR_CONTEXT_RENDER_STATISTICS 0x105 
#define RPR_CONTEXT_DEVICE_COUNT 0x106 
#define RPR_CONTEXT_PARAMETER_COUNT 0x107 
#define RPR_CONTEXT_ACTIVE_PLUGIN 0x108 
#define RPR_CONTEXT_SCENE 0x109 
#define RPR_CONTEXT_AA_CELL_SIZE 0x10A 
#define RPR_CONTEXT_AA_SAMPLES 0x10B 
#define RPR_CONTEXT_IMAGE_FILTER_TYPE 0x10C 
#define RPR_CONTEXT_IMAGE_FILTER_BOX_RADIUS 0x10D 
#define RPR_CONTEXT_IMAGE_FILTER_GAUSSIAN_RADIUS 0x10E 
#define RPR_CONTEXT_IMAGE_FILTER_TRIANGLE_RADIUS 0x10F 
#define RPR_CONTEXT_IMAGE_FILTER_MITCHELL_RADIUS 0x110 
#define RPR_CONTEXT_IMAGE_FILTER_LANCZOS_RADIUS 0x111 
#define RPR_CONTEXT_IMAGE_FILTER_BLACKMANHARRIS_RADIUS 0x112 
#define RPR_CONTEXT_TONE_MAPPING_TYPE 0x113 
#define RPR_CONTEXT_TONE_MAPPING_LINEAR_SCALE 0x114 
#define RPR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_SENSITIVITY 0x115 
#define RPR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_EXPOSURE 0x116 
#define RPR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_FSTOP 0x117 
#define RPR_CONTEXT_TONE_MAPPING_REINHARD02_PRE_SCALE 0x118 
#define RPR_CONTEXT_TONE_MAPPING_REINHARD02_POST_SCALE 0x119 
#define RPR_CONTEXT_TONE_MAPPING_REINHARD02_BURN 0x11A 
#define RPR_CONTEXT_MAX_RECURSION 0x11B 
#define RPR_CONTEXT_RAY_CAST_EPISLON 0x11C 
#define RPR_CONTEXT_RADIANCE_CLAMP 0x11D 
#define RPR_CONTEXT_X_FLIP 0x11E 
#define RPR_CONTEXT_Y_FLIP 0x11F 
#define RPR_CONTEXT_TEXTURE_GAMMA 0x120 
#define RPR_CONTEXT_PDF_THRESHOLD 0x121 
#define RPR_CONTEXT_RENDER_MODE 0x122 
#define RPR_CONTEXT_ROUGHNESS_CAP 0x123 
#define RPR_CONTEXT_DISPLAY_GAMMA 0x124 
#define RPR_CONTEXT_MATERIAL_STACK_SIZE 0x125 
#define RPR_CONTEXT_CLIPPING_PLANE 0x126 
#define RPR_CONTEXT_GPU0_NAME 0x127 
#define RPR_CONTEXT_GPU1_NAME 0x128 
#define RPR_CONTEXT_GPU2_NAME 0x129 
#define RPR_CONTEXT_GPU3_NAME 0x12A 
#define RPR_CONTEXT_CPU_NAME 0x12B 
#define RPR_CONTEXT_GPU4_NAME 0x12C 
#define RPR_CONTEXT_GPU5_NAME 0x12D 
#define RPR_CONTEXT_GPU6_NAME 0x12E 
#define RPR_CONTEXT_GPU7_NAME 0x12F 
#define RPR_CONTEXT_TONE_MAPPING_EXPONENTIAL_INTENSITY 0x130 
#define RPR_CONTEXT_FRAMECOUNT 0x131 

/* last of the RPR_CONTEXT_* */
#define RPR_CONTEXT_MAX 0x132 

/* rpr_camera_info */
#define RPR_CAMERA_TRANSFORM 0x201 
#define RPR_CAMERA_FSTOP 0x202 
#define RPR_CAMERA_APERTURE_BLADES 0x203 
#define RPR_CAMERA_RESPONSE 0x204 
#define RPR_CAMERA_EXPOSURE 0x205 
#define RPR_CAMERA_FOCAL_LENGTH 0x206 
#define RPR_CAMERA_SENSOR_SIZE 0x207 
#define RPR_CAMERA_MODE 0x208 
#define RPR_CAMERA_ORTHO_WIDTH 0x209 
#define RPR_CAMERA_ORTHO_HEIGHT 0x20A 
#define RPR_CAMERA_FOCUS_DISTANCE 0x20B 
#define RPR_CAMERA_POSITION 0x20C 
#define RPR_CAMERA_LOOKAT 0x20D 
#define RPR_CAMERA_UP 0x20E 
#define RPR_CAMERA_FOCAL_TILT 0x20F 
#define RPR_CAMERA_LENS_SHIFT 0x210 
#define RPR_CAMERA_IPD 0x211 

/* rpr_image_info */
#define RPR_IMAGE_FORMAT 0x301 
#define RPR_IMAGE_DESC 0x302 
#define RPR_IMAGE_DATA 0x303 
#define RPR_IMAGE_DATA_SIZEBYTE 0x304 
#define RPR_IMAGE_WRAP 0x305 

/* rpr_shape_info */
#define RPR_SHAPE_TYPE 0x401 
#define RPR_SHAPE_VIDMEM_USAGE 0x402 
#define RPR_SHAPE_TRANSFORM 0x403 
#define RPR_SHAPE_MATERIAL 0x404 
#define RPR_SHAPE_LINEAR_MOTION 0x405 
#define RPR_SHAPE_ANGULAR_MOTION 0x406 
#define RPR_SHAPE_VISIBILITY_FLAG 0x407 
#define RPR_SHAPE_SHADOW_FLAG 0x408 
#define RPR_SHAPE_SUBDIVISION_FACTOR 0x409 
#define RPR_SHAPE_DISPLACEMENT_SCALE 0x40A 
#define RPR_SHAPE_DISPLACEMENT_IMAGE 0X40B 
#define RPR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG 0x40C 
#define RPR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG 0x40D 
#define RPR_SHAPE_SHADOW_CATCHER_FLAG 0x40E 
#define RPR_SHAPE_VOLUME_MATERIAL 0x40F 
#define RPR_SHAPE_OBJECT_GROUP_ID 0x410 
#define RPR_SHAPE_SUBDIVISION_CREASEWEIGHT 0x411 
#define RPR_SHAPE_SUBDIVISION_BOUNDARYINTEROP 0x412 
#define RPR_SHAPE_MATERIAL_OVERRIDE 0x413 

/* rpr_mesh_info */
#define RPR_MESH_POLYGON_COUNT 0x501 
#define RPR_MESH_VERTEX_COUNT 0x502 
#define RPR_MESH_NORMAL_COUNT 0x503 
#define RPR_MESH_UV_COUNT 0x504 
#define RPR_MESH_VERTEX_ARRAY 0x505 
#define RPR_MESH_NORMAL_ARRAY 0x506 
#define RPR_MESH_UV_ARRAY 0x507 
#define RPR_MESH_VERTEX_INDEX_ARRAY 0x508 
#define RPR_MESH_NORMAL_INDEX_ARRAY 0x509 
#define RPR_MESH_UV_INDEX_ARRAY 0x50A 
#define RPR_MESH_VERTEX_STRIDE 0x50C 
#define RPR_MESH_NORMAL_STRIDE 0x50D 
#define RPR_MESH_UV_STRIDE 0x50E 
#define RPR_MESH_VERTEX_INDEX_STRIDE 0x50F 
#define RPR_MESH_NORMAL_INDEX_STRIDE 0x510 
#define RPR_MESH_UV_INDEX_STRIDE 0x511 
#define RPR_MESH_NUM_FACE_VERTICES_ARRAY 0x512 
#define RPR_MESH_UV2_COUNT 0x513 
#define RPR_MESH_UV2_ARRAY 0x514 
#define RPR_MESH_UV2_INDEX_ARRAY 0x515 
#define RPR_MESH_UV2_STRIDE 0x516 
#define RPR_MESH_UV2_INDEX_STRIDE 0x517 

/* rpr_scene_info */
#define RPR_SCENE_SHAPE_COUNT 0x701 
#define RPR_SCENE_LIGHT_COUNT 0x702 
#define RPR_SCENE_SHAPE_LIST 0x704 
#define RPR_SCENE_LIGHT_LIST 0x705 
#define RPR_SCENE_CAMERA 0x706 
#define RPR_SCENE_BACKGROUND_IMAGE 0x708 
#define RPR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION 0x709 
#define RPR_SCENE_ENVIRONMENT_OVERRIDE_REFRACTION 0x70A 
#define RPR_SCENE_ENVIRONMENT_OVERRIDE_TRANSPARENCY 0x70B 
#define RPR_SCENE_ENVIRONMENT_OVERRIDE_BACKGROUND 0x70C 
#define RPR_SCENE_AXIS_ALIGNED_BOUNDING_BOX 0x70D

/* rpr_light_info */
#define RPR_LIGHT_TYPE 0x801 
#define RPR_LIGHT_TRANSFORM 0x803 

/* rpr_light_info - point light */
#define RPR_POINT_LIGHT_RADIANT_POWER 0x804 

/* rpr_light_info - directional light */
#define RPR_DIRECTIONAL_LIGHT_RADIANT_POWER 0x808 
#define RPR_DIRECTIONAL_LIGHT_SHADOW_SOFTNESS 0x809 

/* rpr_light_info - spot light */
#define RPR_SPOT_LIGHT_RADIANT_POWER 0x80B 
#define RPR_SPOT_LIGHT_CONE_SHAPE 0x80C 

/* rpr_light_info - environment light */
#define RPR_ENVIRONMENT_LIGHT_IMAGE 0x80F 
#define RPR_ENVIRONMENT_LIGHT_INTENSITY_SCALE 0x810 
#define RPR_ENVIRONMENT_LIGHT_PORTAL_LIST 0x818 
#define RPR_ENVIRONMENT_LIGHT_PORTAL_COUNT 0x819 

/* rpr_light_info - sky light */
#define RPR_SKY_LIGHT_TURBIDITY 0x812 
#define RPR_SKY_LIGHT_ALBEDO 0x813 
#define RPR_SKY_LIGHT_SCALE 0x814 
#define RPR_SKY_LIGHT_PORTAL_LIST 0x820 
#define RPR_SKY_LIGHT_PORTAL_COUNT 0x821 

/* rpr_light_info - IES light */
#define RPR_IES_LIGHT_RADIANT_POWER 0x816 
#define RPR_IES_LIGHT_IMAGE_DESC 0x817 

/* rpr_parameter_info */
#define RPR_PARAMETER_NAME 0x1201 
#define RPR_PARAMETER_NAME_STRING 0x1202 
#define RPR_PARAMETER_TYPE 0x1203 
#define RPR_PARAMETER_DESCRIPTION 0x1204 
#define RPR_PARAMETER_VALUE 0x1205 

/* rpr_framebuffer_info */
#define RPR_FRAMEBUFFER_FORMAT 0x1301 
#define RPR_FRAMEBUFFER_DESC 0x1302 
#define RPR_FRAMEBUFFER_DATA 0x1303 
#define RPR_FRAMEBUFFER_GL_TARGET 0x1304 
#define RPR_FRAMEBUFFER_GL_MIPLEVEL 0x1305 
#define RPR_FRAMEBUFFER_GL_TEXTURE 0x1306 

/* rpr_mesh_polygon_info */
#define RPR_MESH_POLYGON_VERTEX_COUNT 0x1401 

/* rpr_mesh_polygon_vertex_info */
#define RPR_MESH_POLYGON_VERTEX_POSITION 0x1501 
#define RPR_MESH_POLYGON_VERTEX_NORMAL 0x1502 
#define RPR_MESH_POLYGON_VERTEX_TEXCOORD 0x1503 

/* rpr_instance_info */
#define RPR_INSTANCE_PARENT_SHAPE 0x1601 

/* rpr_image_format */
/* rpr_component_type */
#define RPR_COMPONENT_TYPE_UINT8 0x1 
#define RPR_COMPONENT_TYPE_FLOAT16 0x2 
#define RPR_COMPONENT_TYPE_FLOAT32 0x3 

/* rpr_render_mode */
#define RPR_RENDER_MODE_GLOBAL_ILLUMINATION 0x1 
#define RPR_RENDER_MODE_DIRECT_ILLUMINATION 0x2 
#define RPR_RENDER_MODE_DIRECT_ILLUMINATION_NO_SHADOW 0x3 
#define RPR_RENDER_MODE_WIREFRAME 0x4 
#define RPR_RENDER_MODE_MATERIAL_INDEX 0x5 
#define RPR_RENDER_MODE_POSITION 0x6 
#define RPR_RENDER_MODE_NORMAL 0x7 
#define RPR_RENDER_MODE_TEXCOORD 0x8 
#define RPR_RENDER_MODE_AMBIENT_OCCLUSION 0x9 

/* rpr_camera_mode */
#define RPR_CAMERA_MODE_PERSPECTIVE 0x1 
#define RPR_CAMERA_MODE_ORTHOGRAPHIC 0x2 
#define RPR_CAMERA_MODE_LATITUDE_LONGITUDE_360 0x3 
#define RPR_CAMERA_MODE_LATITUDE_LONGITUDE_STEREO 0x4 
#define RPR_CAMERA_MODE_CUBEMAP 0x5 
#define RPR_CAMERA_MODE_CUBEMAP_STEREO 0x6 

/* rpr_tonemapping_operator */
#define RPR_TONEMAPPING_OPERATOR_NONE 0x0 
#define RPR_TONEMAPPING_OPERATOR_LINEAR 0x1 
#define RPR_TONEMAPPING_OPERATOR_PHOTOLINEAR 0x2 
#define RPR_TONEMAPPING_OPERATOR_AUTOLINEAR 0x3 
#define RPR_TONEMAPPING_OPERATOR_MAXWHITE 0x4 
#define RPR_TONEMAPPING_OPERATOR_REINHARD02 0x5 
#define RPR_TONEMAPPING_OPERATOR_EXPONENTIAL 0x6 

/* rpr_volume_type */
#define RPR_VOLUME_TYPE_NONE 0xFFFF 
#define RPR_VOLUME_TYPE_HOMOGENEOUS 0x0 
#define RPR_VOLUME_TYPE_HETEROGENEOUS 0x1 

/* rpr_material_node_info*/
#define RPR_MATERIAL_NODE_TYPE 0x1101 
#define RPR_MATERIAL_NODE_SYSTEM 0x1102 
#define RPR_MATERIAL_NODE_INPUT_COUNT 0x1103 

/* rpr_material_node_input_info */
#define RPR_MATERIAL_NODE_INPUT_NAME 0x1103 
#define RPR_MATERIAL_NODE_INPUT_NAME_STRING 0x1104 
#define RPR_MATERIAL_NODE_INPUT_DESCRIPTION 0x1105 
#define RPR_MATERIAL_NODE_INPUT_VALUE 0x1106 
#define RPR_MATERIAL_NODE_INPUT_TYPE 0x1107 

/* rpr_material_node_type */
#define RPR_MATERIAL_NODE_DIFFUSE 0x1 
#define RPR_MATERIAL_NODE_MICROFACET 0x2 
#define RPR_MATERIAL_NODE_REFLECTION 0x3 
#define RPR_MATERIAL_NODE_REFRACTION 0x4 
#define RPR_MATERIAL_NODE_MICROFACET_REFRACTION 0x5 
#define RPR_MATERIAL_NODE_TRANSPARENT 0x6 
#define RPR_MATERIAL_NODE_EMISSIVE 0x7 
#define RPR_MATERIAL_NODE_WARD 0x8 
#define RPR_MATERIAL_NODE_ADD 0x9 
#define RPR_MATERIAL_NODE_BLEND 0xA 
#define RPR_MATERIAL_NODE_ARITHMETIC 0xB 
#define RPR_MATERIAL_NODE_FRESNEL 0xC 
#define RPR_MATERIAL_NODE_NORMAL_MAP 0xD 
#define RPR_MATERIAL_NODE_IMAGE_TEXTURE 0xE 
#define RPR_MATERIAL_NODE_NOISE2D_TEXTURE 0xF 
#define RPR_MATERIAL_NODE_DOT_TEXTURE 0x10 
#define RPR_MATERIAL_NODE_GRADIENT_TEXTURE 0x11 
#define RPR_MATERIAL_NODE_CHECKER_TEXTURE 0x12 
#define RPR_MATERIAL_NODE_CONSTANT_TEXTURE 0x13 
#define RPR_MATERIAL_NODE_INPUT_LOOKUP 0x14 
#define RPR_MATERIAL_NODE_STANDARD 0x15 
#define RPR_MATERIAL_NODE_BLEND_VALUE 0x16 
#define RPR_MATERIAL_NODE_PASSTHROUGH 0x17 
#define RPR_MATERIAL_NODE_ORENNAYAR 0x18 
#define RPR_MATERIAL_NODE_FRESNEL_SCHLICK 0x19 
#define RPR_MATERIAL_NODE_DIFFUSE_REFRACTION 0x1B 
#define RPR_MATERIAL_NODE_BUMP_MAP 0x1C 
#define RPR_MATERIAL_NODE_VOLUME 0x1D 

/* rpr_material_node_input */
#define RPR_MATERIAL_INPUT_COLOR 0x0 
#define RPR_MATERIAL_INPUT_COLOR0 0x1 
#define RPR_MATERIAL_INPUT_COLOR1 0x2 
#define RPR_MATERIAL_INPUT_NORMAL 0x3 
#define RPR_MATERIAL_INPUT_UV 0x4 
#define RPR_MATERIAL_INPUT_DATA 0x5 
#define RPR_MATERIAL_INPUT_ROUGHNESS 0x6 
#define RPR_MATERIAL_INPUT_IOR 0x7 
#define RPR_MATERIAL_INPUT_ROUGHNESS_X 0x8 
#define RPR_MATERIAL_INPUT_ROUGHNESS_Y 0x9 
#define RPR_MATERIAL_INPUT_ROTATION 0xA 
#define RPR_MATERIAL_INPUT_WEIGHT 0xB 
#define RPR_MATERIAL_INPUT_OP 0xC 
#define RPR_MATERIAL_INPUT_INVEC 0xD 
#define RPR_MATERIAL_INPUT_UV_SCALE 0xE 
#define RPR_MATERIAL_INPUT_VALUE 0xF 
#define RPR_MATERIAL_INPUT_REFLECTANCE 0x10 
#define RPR_MATERIAL_INPUT_SCALE 0x11 
#define RPR_MATERIAL_INPUT_SCATTERING 0x12 
#define RPR_MATERIAL_INPUT_ABSORBTION 0x13 
#define RPR_MATERIAL_INPUT_EMISSION 0x14 
#define RPR_MATERIAL_INPUT_G 0x15 
#define RPR_MATERIAL_INPUT_MULTISCATTER 0x16 
#define RPR_MATERIAL_INPUT_COLOR2 0x17 
#define RPR_MATERIAL_INPUT_COLOR3 0x18 
#define RPR_MATERIAL_INPUT_MAX 0x19 

  
#define RPR_MATERIAL_STANDARD_INPUT_DIFFUSE_COLOR 0x112 
#define RPR_MATERIAL_STANDARD_INPUT_DIFFUSE_NORMAL 0x113 
#define RPR_MATERIAL_STANDARD_INPUT_GLOSSY_COLOR 0x114 
#define RPR_MATERIAL_STANDARD_INPUT_GLOSSY_NORMAL 0x115 
#define RPR_MATERIAL_STANDARD_INPUT_GLOSSY_ROUGHNESS_X 0x116 
#define RPR_MATERIAL_STANDARD_INPUT_CLEARCOAT_COLOR 0x117 
#define RPR_MATERIAL_STANDARD_INPUT_CLEARCOAT_NORMAL 0x118 
#define RPR_MATERIAL_STANDARD_INPUT_REFRACTION_COLOR 0x119 
#define RPR_MATERIAL_STANDARD_INPUT_REFRACTION_NORMAL 0x11A 
#define RPR_MATERIAL_STANDARD_INPUT_REFRACTION_IOR 0x11C 
#define RPR_MATERIAL_STANDARD_INPUT_DIFFUSE_TO_REFRACTION_WEIGHT 0x11D 
#define RPR_MATERIAL_STANDARD_INPUT_GLOSSY_TO_DIFFUSE_WEIGHT 0x11E 
#define RPR_MATERIAL_STANDARD_INPUT_CLEARCOAT_TO_GLOSSY_WEIGHT 0x11F 
#define RPR_MATERIAL_STANDARD_INPUT_TRANSPARENCY 0x120 
#define RPR_MATERIAL_STANDARD_INPUT_TRANSPARENCY_COLOR 0x121 
#define RPR_MATERIAL_STANDARD_INPUT_REFRACTION_ROUGHNESS 0x122 
#define RPR_MATERIAL_STANDARD_INPUT_GLOSSY_ROUGHNESS_Y 0x123 
#define RPR_MATERIAL_INPUT_RASTER_METALLIC 0x901 
#define RPR_MATERIAL_INPUT_RASTER_ROUGHNESS 0x902 
#define RPR_MATERIAL_INPUT_RASTER_SUBSURFACE 0x903 
#define RPR_MATERIAL_INPUT_RASTER_ANISOTROPIC 0x904 
#define RPR_MATERIAL_INPUT_RASTER_SPECULAR 0x905 
#define RPR_MATERIAL_INPUT_RASTER_SPECULARTINT 0x906 
#define RPR_MATERIAL_INPUT_RASTER_SHEEN 0x907 
#define RPR_MATERIAL_INPUT_RASTER_SHEENTINT 0x908 
#define RPR_MATERIAL_INPUT_RASTER_CLEARCOAT 0x90A 
#define RPR_MATERIAL_INPUT_RASTER_CLEARCOATGLOSS 0x90B 
#define RPR_MATERIAL_INPUT_RASTER_COLOR 0x90C 
#define RPR_MATERIAL_INPUT_RASTER_NORMAL 0x90D 

/* rpr_material_node_arithmetic_operation */
#define RPR_MATERIAL_NODE_OP_ADD 0x00 
#define RPR_MATERIAL_NODE_OP_SUB 0x01 
#define RPR_MATERIAL_NODE_OP_MUL 0x02 
#define RPR_MATERIAL_NODE_OP_DIV 0x03 
#define RPR_MATERIAL_NODE_OP_SIN 0x04 
#define RPR_MATERIAL_NODE_OP_COS 0x05 
#define RPR_MATERIAL_NODE_OP_TAN 0x06 
#define RPR_MATERIAL_NODE_OP_SELECT_X 0x07 
#define RPR_MATERIAL_NODE_OP_SELECT_Y 0x08 
#define RPR_MATERIAL_NODE_OP_SELECT_Z 0x09 
#define RPR_MATERIAL_NODE_OP_SELECT_W 0x0A 
#define RPR_MATERIAL_NODE_OP_COMBINE 0x0B 
#define RPR_MATERIAL_NODE_OP_DOT3 0x0C 
#define RPR_MATERIAL_NODE_OP_DOT4 0x0D 
#define RPR_MATERIAL_NODE_OP_CROSS3 0x0E 
#define RPR_MATERIAL_NODE_OP_LENGTH3 0x0F 
#define RPR_MATERIAL_NODE_OP_NORMALIZE3 0x10 
#define RPR_MATERIAL_NODE_OP_POW 0x11 
#define RPR_MATERIAL_NODE_OP_ACOS 0x12 
#define RPR_MATERIAL_NODE_OP_ASIN 0x13 
#define RPR_MATERIAL_NODE_OP_ATAN 0x14 
#define RPR_MATERIAL_NODE_OP_AVERAGE_XYZ 0x15 
#define RPR_MATERIAL_NODE_OP_AVERAGE 0x16 
#define RPR_MATERIAL_NODE_OP_MIN 0x17 
#define RPR_MATERIAL_NODE_OP_MAX 0x18 
#define RPR_MATERIAL_NODE_OP_FLOOR 0x19 
#define RPR_MATERIAL_NODE_OP_MOD 0x1A 
#define RPR_MATERIAL_NODE_OP_ABS 0x1B 
#define RPR_MATERIAL_NODE_OP_SHUFFLE_YZWX 0x1C 
#define RPR_MATERIAL_NODE_OP_SHUFFLE_ZWXY 0x1D 
#define RPR_MATERIAL_NODE_OP_SHUFFLE_WXYZ 0x1E 
#define RPR_MATERIAL_NODE_OP_MAT_MUL 0x1F 

/* rpr_material_node_lookup_value */
#define RPR_MATERIAL_NODE_LOOKUP_UV 0x0 
#define RPR_MATERIAL_NODE_LOOKUP_N 0x1 
#define RPR_MATERIAL_NODE_LOOKUP_P 0x2 
#define RPR_MATERIAL_NODE_LOOKUP_INVEC 0x3 
#define RPR_MATERIAL_NODE_LOOKUP_OUTVEC 0x4 
#define RPR_MATERIAL_NODE_LOOKUP_UV1 0x5 

/* rpr_post_effect_info */
#define RPR_POST_EFFECT_TYPE 0x0 
#define RPR_POST_EFFECT_PARAMETER_COUNT 0x1 

/* rpr_post_effect_type - white balance */
#define RPR_POST_EFFECT_WHITE_BALANCE_COLOR_SPACE 0x1 
#define RPR_POST_EFFECT_WHITE_BALANCE_COLOR_TEMPERATURE 0x2 

/* rpr_post_effect_type - simple tonemap */
#define RPR_POST_EFFECT_SIMPLE_TONEMAP_EXPOSURE 0x1 
#define RPR_POST_EFFECT_SIMPLE_TONEMAP_CONTRAST 0x2 

/*rpr_aov*/
#define RPR_AOV_COLOR 0x0 
#define RPR_AOV_OPACITY 0x1 
#define RPR_AOV_WORLD_COORDINATE 0x2 
#define RPR_AOV_UV 0x3 
#define RPR_AOV_MATERIAL_IDX 0x4 
#define RPR_AOV_GEOMETRIC_NORMAL 0x5 
#define RPR_AOV_SHADING_NORMAL 0x6 
#define RPR_AOV_DEPTH 0x7 
#define RPR_AOV_OBJECT_ID 0x8 
#define RPR_AOV_OBJECT_GROUP_ID 0x9 
#define RPR_AOV_MAX 0xa 

/*rpr_post_effect_type*/
#define RPR_POST_EFFECT_TONE_MAP 0x0 
#define RPR_POST_EFFECT_WHITE_BALANCE 0x1 
#define RPR_POST_EFFECT_SIMPLE_TONEMAP 0x2 
#define RPR_POST_EFFECT_NORMALIZATION 0x3 
#define RPR_POST_EFFECT_GAMMA_CORRECTION 0x4 

/*rpr_color_space*/
#define RPR_COLOR_SPACE_SRGB 0x1 
#define RPR_COLOR_SPACE_ADOBE_RGB 0x2 
#define RPR_COLOR_SPACE_REC2020 0x3 
#define RPR_COLOR_SPACE_DCIP3 0x4 

/* rpr_material_node_type */
#define RPR_MATERIAL_NODE_INPUT_TYPE_FLOAT4 0x1 
#define RPR_MATERIAL_NODE_INPUT_TYPE_UINT 0x2 
#define RPR_MATERIAL_NODE_INPUT_TYPE_NODE 0x3 
#define RPR_MATERIAL_NODE_INPUT_TYPE_IMAGE 0x4 

/* Additional Raster context properties ("raster.shadows.filter") */
#define RPR_RASTER_SHADOWS_FILTER_NONE 0x90E
#define RPR_RASTER_SHADOWS_FILTER_PCF 0x90F
#define RPR_RASTER_SHADOWS_FILTER_PCSS 0x910

/* Additional Raster context properties ("raster.shadows.sampling") */
#define RPR_RASTER_SHADOWS_SAMPLING_BILINEAR 0x911
#define RPR_RASTER_SHADOWS_SAMPLING_HAMMERSLEY 0x912
#define RPR_RASTER_SHADOWS_SAMPLING_MULTIJITTERED 0x913

/* rpr_subdiv_boundary_interfop_type */
#define RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_AND_CORNER 0x1 
#define RPR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_ONLY 0x2 

/* rpr_image_wrap_type */
#define RPR_IMAGE_WRAP_TYPE_REPEAT 0x1 
#define RPR_IMAGE_WRAP_TYPE_MIRRORED_REPEAT 0x2 
#define RPR_IMAGE_WRAP_TYPE_CLAMP_TO_EDGE 0x3 
#define RPR_IMAGE_WRAP_TYPE_CLAMP_TO_BORDER 0x4 

/* Constants */
#define RPR_MAX_AA_SAMPLES 32 
#define RPR_MAX_AA_GRID_SIZE 16 

/* rpr_bool */
#define RPR_FALSE 0 
#define RPR_TRUE 1 

/* Library types */
/* This is going to be moved to rpr_platform.h or similar */
typedef char rpr_char;
typedef unsigned char rpr_uchar;
typedef int rpr_int;
typedef unsigned int rpr_uint;
typedef long int rpr_long;
typedef long unsigned int rpr_ulong;
typedef short int rpr_short;
typedef short unsigned int rpr_ushort;
typedef float rpr_float;
typedef double rpr_double;
typedef long long int rpr_longlong;
typedef int rpr_bool;
typedef rpr_uint rpr_bitfield;
typedef void * rpr_context;
typedef void * rpr_camera;
typedef void * rpr_shape;
typedef void * rpr_light;
typedef void * rpr_scene;
typedef void * rpr_image;
typedef void * rpr_framebuffer;
typedef void * rpr_material_system;
typedef void * rpr_material_node;
typedef void * rpr_post_effect;
typedef void * rpr_context_properties;
typedef rpr_uint rpr_light_type;
typedef rpr_uint rpr_image_type;
typedef rpr_uint rpr_shape_type;
typedef rpr_uint rpr_context_type;
typedef rpr_bitfield rpr_creation_flags;
typedef rpr_uint rpr_aa_filter;
typedef rpr_uint rpr_context_info;
typedef rpr_uint rpr_camera_info;
typedef rpr_uint rpr_image_info;
typedef rpr_uint rpr_shape_info;
typedef rpr_uint rpr_mesh_info;
typedef rpr_uint rpr_mesh_polygon_info;
typedef rpr_uint rpr_mesh_polygon_vertex_info;
typedef rpr_uint rpr_light_info;
typedef rpr_uint rpr_scene_info;
typedef rpr_uint rpr_parameter_info;
typedef rpr_uint rpr_framebuffer_info;
typedef rpr_uint rpr_channel_order;
typedef rpr_uint rpr_channel_type;
typedef rpr_uint rpr_parameter_type;
typedef rpr_uint rpr_render_mode;
typedef rpr_uint rpr_component_type;
typedef rpr_uint rpr_camera_mode;
typedef rpr_uint rpr_tonemapping_operator;
typedef rpr_uint rpr_volume_type;
typedef rpr_uint rpr_material_system_type;
typedef rpr_uint rpr_material_node_type;
typedef rpr_uint rpr_material_node_input;
typedef rpr_uint rpr_material_node_info;
typedef rpr_uint rpr_material_node_input_info;
typedef rpr_uint rpr_aov;
typedef rpr_uint rpr_post_effect_type;
typedef rpr_uint rpr_post_effect_info;
typedef rpr_uint rpr_color_space;
typedef rpr_uint rpr_environment_override;
typedef rpr_uint rpr_subdiv_boundary_interfop_type;
typedef rpr_uint rpr_material_node_lookup_value;
typedef rpr_uint rpr_image_wrap_type;

struct _rpr_image_desc
{
    rpr_uint image_width;
    rpr_uint image_height;
    rpr_uint image_depth;
    rpr_uint image_row_pitch;
    rpr_uint image_slice_pitch;
};

typedef _rpr_image_desc rpr_image_desc;

struct _rpr_framebuffer_desc
{
    rpr_uint fb_width;
    rpr_uint fb_height;
};

typedef _rpr_framebuffer_desc rpr_framebuffer_desc;

struct _rpr_render_statistics
{
    rpr_longlong gpumem_usage;
    rpr_longlong gpumem_total;
    rpr_longlong gpumem_max_allocation;
};

typedef _rpr_render_statistics rpr_render_statistics;

struct _rpr_image_format
{
    rpr_uint num_components;
    rpr_component_type type;
};

typedef _rpr_image_format rpr_image_format;

struct _rpr_ies_image_desc
{
    rpr_int w;
    rpr_int h;
    rpr_char const * data;
    rpr_char const * filename;
};

typedef _rpr_ies_image_desc rpr_ies_image_desc;
typedef rpr_image_format rpr_framebuffer_format;

/* API functions */
/** @brief Register rendering plugin
*
*  <Description>
*
*  @param path     Path of plugin to load
*  @return         unique identifier of plugin, -1 otherwise
*/
extern RPR_API_ENTRY rpr_int rprRegisterPlugin(rpr_char const * path);

/** @brief Create rendering context
  *
  *  Rendering context is a root concept encapsulating the render states and responsible
  *  for execution control. All the entities in FireRender are created for a particular rendering context.
  *  Entities created for some context can't be used with other contexts. Possible error codes for this call are:
  *
  *     RPR_ERROR_COMPUTE_API_NOT_SUPPORTED
  *     RPR_ERROR_OUT_OF_SYSTEM_MEMORY
  *     RPR_ERROR_OUT_OF_VIDEO_MEMORY
  *     RPR_ERROR_INVALID_API_VERSION
  *     RPR_ERROR_INVALID_PARAMETER
  *
  *  @param api_version     Api version constant
  *	 @param context_type    Determines compute API to use, OPENCL only is supported for now
  *  @param creation_flags  Determines multi-gpu or cpu-gpu configuration
  *  @param props           Context properties, reserved for future use
  *  @param cache_path      Full path to kernel cache created by FireRender, NULL means to use current folder
  *  @param out_context		Pointer to context object
  *  @return                RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprCreateContext(rpr_int api_version, rpr_int * pluginIDs, size_t pluginCount, rpr_creation_flags creation_flags, rpr_context_properties const * props, rpr_char const * cache_path, rpr_context * out_context);

/** @breif Set active context plugin
*
*/
extern RPR_API_ENTRY rpr_int rprContextSetActivePlugin(rpr_context context, rpr_int pluginID);

/** @brief Query information about a context
 *
 *  The workflow is usually two-step: query with the data == NULL and size = 0 to get the required buffer size in size_ret,
 *  then query with size_ret == NULL to fill the buffer with the data.
 *   Possible error codes:
 *     RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  context         The context to query
 *  @param  context_info    The type of info to query
 *  @param  size            The size of the buffer pointed by data
 *  @param  data            The buffer to store queried info
 *  @param  size_ret        Returns the size in bytes of the data being queried
 *  @return                 RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextGetInfo(rpr_context context, rpr_context_info context_info, size_t size, void * data, size_t * size_ret);

/** @brief Query information about a context parameter
*
*  The workflow is usually two-step: query with the data == NULL and size = 0 to get the required buffer size in size_ret,
*  then query with size_ret == NULL to fill the buffer with the data
*   Possible error codes:
*     RPR_ERROR_INVALID_PARAMETER
*
*  @param  context         The context to query
*  @param  param_idx	   The index of the parameter
*  @param  parameter_info  The type of info to query
*  @param  size            The size of the buffer pointed by data
*  @param  data            The buffer to store queried info
*  @param  size_ret        Returns the size in bytes of the data being queried
*  @return                 RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprContextGetParameterInfo(rpr_context context, int param_idx, rpr_parameter_info parameter_info, size_t size, void * data, size_t * size_ret);

/** @brief Query the AOV
 *
 *  @param  context     The context to get a frame buffer from
 *  @param  out_fb		Pointer to framebuffer object
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextGetAOV(rpr_context context, rpr_aov aov, rpr_framebuffer * out_fb);

/** @brief Set AOV
 *
 *  @param  context         The context to set AOV
 *  @param  aov				AOV
 *  @param  frame_buffer    Frame buffer object to set
 *  @return                 RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextSetAOV(rpr_context context, rpr_aov aov, rpr_framebuffer frame_buffer);

/** @brief Set the scene
 *
 *  The scene is a collection of objects and lights
 *  along with all the data required to shade those. The scene is
 *  used by the context to render the image.
 *
 *  @param  context     The context to set the scene
 *  @param  scene       The scene to set
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextSetScene(rpr_context context, rpr_scene scene);

/** @brief Get the current scene
 *
 *  The scene is a collection of objects and lights
 *  along with all the data required to shade those. The scene is
 *  used by the context ro render the image.
 *
 *  @param  context     The context to get the scene from
 *  @param  out_scene   Pointer to scene object
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextGetScene(rpr_context arg0, rpr_scene * out_scene);

/** @brief Set context parameter
 *
 *  Parameters are used to control rendering modes, global sampling and AA settings, etc
 *
 *  @param  context                        The context to set the value to
 *  @param  name						   Param name, can be:

    *  aacellsize                          ft_float
    *  aasamples                           ft_float

    *  imagefilter.type					   rpr_aa_filter
    *  imagefilter.box.radius              ft_float
    *  imagefilter.gaussian.radius         ft_float
    *  imagefilter.triangle.radius         ft_float
    *  imagefilter.mitchell.radius         ft_float
    *  imagefilter.lanczos.radius          ft_float
    *  imagefilter.blackmanharris.radius   ft_float

	*  tonemapping.type                    rpr_tonemapping_operator
    *  tonemapping.linear.scale            ft_float
    *  tonemapping.photolinear.sensitivity ft_float
    *  tonemapping.photolinear.exposure    ft_float
    *  tonemapping.photolinear.fstop       ft_float
    *  tonemapping.reinhard02.prescale     ft_float
    *  tonemapping.reinhard02.postscale    ft_float
    *  tonemapping.reinhard02.burn         ft_float

 * @param x,y,z,w						   Parameter value

 * @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextSetParameter1u(rpr_context context, rpr_char const * name, rpr_uint x);
extern RPR_API_ENTRY rpr_int rprContextSetParameter1f(rpr_context context, rpr_char const * name, rpr_float x);
extern RPR_API_ENTRY rpr_int rprContextSetParameter3f(rpr_context context, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z);
extern RPR_API_ENTRY rpr_int rprContextSetParameter4f(rpr_context context, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z, rpr_float w);
extern RPR_API_ENTRY rpr_int rprContextSetParameterString(rpr_context context, rpr_char const * name, rpr_char const * value);

/** @brief Perform evaluation and accumulation of a single sample (or number of AA samples if AA is enabled)
 *
 *  The call is blocking and the image is ready when returned. The context accumulates the samples in order
 *  to progressively refine the image and enable interactive response. So each new call to Render refines the
 *  resultin image with 1 (or num aa samples) color samples. Call rprFramebufferClear if you want to start rendering new image
 *  instead of refining the previous one.
 *
 *  Possible error codes:
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_INTERNAL_ERROR
 *      RPR_ERROR_MATERIAL_STACK_OVERFLOW
 *
 *  if you have the RPR_ERROR_MATERIAL_STACK_OVERFLOW error, you have created a shader graph with too many nodes.
 *  you can check the nodes limit with rprContextGetInfo(,RPR_CONTEXT_MATERIAL_STACK_SIZE,)
 *
 *  @param  context     The context object
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextRender(rpr_context context);

/** @brief Perform evaluation and accumulation of a single sample (or number of AA samples if AA is enabled) on the part of the image
 *
 *  The call is blocking and the image is ready when returned. The context accumulates the samples in order
 *  to progressively refine the image and enable interactive response. So each new call to Render refines the
 *  resultin image with 1 (or num aa samples) color samples. Call rprFramebufferClear if you want to start rendering new image
 *  instead of refining the previous one. Possible error codes are:
 *
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_INTERNAL_ERROR
 *
 *  @param  context     The context to use for the rendering
 *  @param  xmin        X coordinate of the top left corner of a tile
 *  @param  xmax        X coordinate of the bottom right corner of a tile
 *  @param  ymin        Y coordinate of the top left corner of a tile
 *  @param  ymax        Y coordinate of the bottom right corner of a tile
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextRenderTile(rpr_context context, rpr_uint xmin, rpr_uint xmax, rpr_uint ymin, rpr_uint ymax);

/** @brief Clear all video memory used by the context
 *
 *  This function should be called after all context objects have been destroyed.
 *  It guarantees that all context memory is freed and returns the context into its initial state.
 *  Will be removed later. Possible error codes are:
 *
 *      RPR_ERROR_INTERNAL_ERROR
 *
 *  @param  context     The context to wipe out
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextClearMemory(rpr_context context);

/** @brief Create an image from memory data
 *
 *  Images are used as HDRI maps or inputs for various shading system nodes.
 *  Possible error codes are:
 *
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  context     The context to create image
 *  @param  format      Image format
 *  @param  image_desc  Image layout description
 *  @param  data        Image data in system memory, can be NULL in which case the memory is allocated
 *  @param  out_image   Pointer to image object
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextCreateImage(rpr_context context, rpr_image_format const format, rpr_image_desc const * image_desc, void const * data, rpr_image * out_image);

/** @brief Create an image from file
 *
 *   Images are used as HDRI maps or inputs for various shading system nodes.
 *
 *  The following image formats are supported:
 *      PNG, JPG, TGA, BMP, TIFF, TX(0-mip), HDR, EXR
 *
 *  Possible error codes are:
 *
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT
 *      RPR_ERROR_INVALID_PARAMETER
 *      RPR_ERROR_IO_ERROR
 *
 *  @param  context     The context to create image
 *  @param  path        NULL terminated path to an image file (can be relative)
 *  @param  out_image   Pointer to image object
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextCreateImageFromFile(rpr_context context, rpr_char const * path, rpr_image * out_image);

/** @brief Create a scene
 *
 *  Scene serves as a container for lights and objects.
 *
 *  Possible error codes are:
 *
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *
 *  @param  out_scene   Pointer to scene object
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextCreateScene(rpr_context context, rpr_scene * out_scene);

/** @brief Create an instance of an object
 *
 *  Possible error codes are:
 *
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  context  The context to create an instance for
 *  @param  shape    Parent shape for an instance
 *  @param  out_instance   Pointer to instance object
 *  @return RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextCreateInstance(rpr_context context, rpr_shape shape, rpr_shape * out_instance);

/** @brief Create a mesh
 *
 *  FireRender supports mixed meshes consisting of triangles and quads.
 *
 *  Possible error codes are:
 *
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  vertices            Pointer to position data (each position is described with 3 rpr_float numbers)
 *  @param  num_vertices        Number of entries in position array
 *  @param  vertex_stride       Number of bytes between the beginnings of two successive position entries
 *  @param  normals             Pointer to normal data (each normal is described with 3 rpr_float numbers), can be NULL
 *  @param  num_normals         Number of entries in normal array
 *  @param  normal_stride       Number of bytes between the beginnings of two successive normal entries
 *  @param  texcoord            Pointer to texcoord data (each texcoord is described with 2 rpr_float numbers), can be NULL
 *  @param  num_texcoords       Number of entries in texcoord array
 *  @param  texcoord_stride     Number of bytes between the beginnings of two successive texcoord entries
 *  @param  vertex_indices      Pointer to an array of vertex indices
 *  @param  vidx_stride         Number of bytes between the beginnings of two successive vertex index entries
 *  @param  normal_indices      Pointer to an array of normal indices
 *  @param  nidx_stride         Number of bytes between the beginnings of two successive normal index entries
 *  @param  texcoord_indices    Pointer to an array of texcoord indices
 *  @param  tidx_stride         Number of bytes between the beginnings of two successive texcoord index entries
 *  @param  num_face_vertices   Pointer to an array of num_faces numbers describing number of vertices for each face (can be 3(triangle) or 4(quad))
 *  @param  num_faces           Number of faces in the mesh
 *  @param  out_mesh            Pointer to mesh object
 *  @return                     RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextCreateMesh(rpr_context context, rpr_float const * vertices, size_t num_vertices, rpr_int vertex_stride, rpr_float const * normals, size_t num_normals, rpr_int normal_stride, rpr_float const * texcoords, size_t num_texcoords, rpr_int texcoord_stride, rpr_int const * vertex_indices, rpr_int vidx_stride, rpr_int const * normal_indices, rpr_int nidx_stride, rpr_int const * texcoord_indices, rpr_int tidx_stride, rpr_int const * num_face_vertices, size_t num_faces, rpr_shape * out_mesh);

/*  @brief Create a mesh
 *
 *  @return                     RPR_SUCCESS in case of success, error code otherwise	
	*/
extern RPR_API_ENTRY rpr_int rprContextCreateMeshEx(rpr_context context, rpr_float const * vertices, size_t num_vertices, rpr_int vertex_stride, rpr_float const * normals, size_t num_normals, rpr_int normal_stride, rpr_int const * perVertexFlag, size_t num_perVertexFlags, rpr_int perVertexFlag_stride, rpr_int numberOfTexCoordLayers, rpr_float const ** texcoords, size_t * num_texcoords, rpr_int * texcoord_stride, rpr_int const * vertex_indices, rpr_int vidx_stride, rpr_int const * normal_indices, rpr_int nidx_stride, rpr_int const ** texcoord_indices, rpr_int * tidx_stride, rpr_int const * num_face_vertices, size_t num_faces, rpr_shape * out_mesh);

/** @brief Create a camera
 *
 *  There are several camera types supported by a single rpr_camera type.
 *  Possible error codes are:
 *
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *
 *  @param  context The context to create a camera for
 *  @param  out_camera Pointer to camera object
 *  @return RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextCreateCamera(rpr_context context, rpr_camera * out_camera);

/** @brief Create framebuffer object
 *
 *  Framebuffer is used to store final rendering result.
 *
 *  Possible error codes are:
 *
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *
 *  @param  context  The context to create framebuffer
 *  @param  format   Framebuffer format
 *  @param  fb_desc  Framebuffer layout description
 *  @param  status   Pointer to framebuffer object
 *  @return          RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprContextCreateFrameBuffer(rpr_context context, rpr_framebuffer_format const format, rpr_framebuffer_desc const * fb_desc, rpr_framebuffer * out_fb);

/* rpr_camera */
/** @brief Query information about a camera
 *
 *  The workflow is usually two-step: query with the data == NULL to get the required buffer size,
 *  then query with size_ret == NULL to fill the buffer with the data.
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  camera      The camera to query
 *  @param  camera_info The type of info to query
 *  @param  size        The size of the buffer pointed by data
 *  @param  data        The buffer to store queried info
 *  @param  size_ret    Returns the size in bytes of the data being queried
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraGetInfo(rpr_camera camera, rpr_camera_info camera_info, size_t size, void * data, size_t * size_ret);

/** @brief Set camera focal length.
 *
 *  @param  camera  The camera to set focal length
 *  @param  flength Focal length in millimeters, default is 35mm
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraSetFocalLength(rpr_camera camera, rpr_float flength);

/** @brief Set camera focus distance
 *
 *  @param  camera  The camera to set focus distance
 *  @param  fdist   Focus distance in meters, default is 1m
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraSetFocusDistance(rpr_camera camera, rpr_float fdist);

/** @brief Set world transform for the camera
 *
 *  @param  camera      The camera to set transform for
 *  @param  transpose   Determines whether the basis vectors are in columns(false) or in rows(true) of the matrix
 *  @param  transform   Array of 16 rpr_float values (row-major form)
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraSetTransform(rpr_camera camera, rpr_bool transpose, rpr_float * transform);

/** @brief Set sensor size for the camera
 *
 *  Default sensor size is the one corresponding to full frame 36x24mm sensor
 *
 *  @param  camera  The camera to set transform for
 *  @param  width   Sensor width in millimeters
 *  @param  height  Sensor height in millimeters
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraSetSensorSize(rpr_camera camera, rpr_float width, rpr_float height);

/** @brief Set camera transform in lookat form
 *
 *  @param  camera  The camera to set transform for
 *  @param  posx    X component of the position
 *  @param  posy    Y component of the position
 *  @param  posz    Z component of the position
 *  @param  atx     X component of the center point
 *  @param  aty     Y component of the center point
 *  @param  atz     Z component of the center point
 *  @param  upx     X component of the up vector
 *  @param  upy     Y component of the up vector
 *  @param  upz     Z component of the up vector
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraLookAt(rpr_camera camera, rpr_float posx, rpr_float posy, rpr_float posz, rpr_float atx, rpr_float aty, rpr_float atz, rpr_float upx, rpr_float upy, rpr_float upz);

/** @brief Set f-stop for the camera
 *
 *  @param  camera  The camera to set f-stop for
 *  @param  fstop   f-stop value in mm^-1, default is FLT_MAX
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraSetFStop(rpr_camera camera, rpr_float fstop);

/** @brief Set the number of aperture blades
 *
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  camera      The camera to set aperture blades for
 *  @param  num_blades  Number of aperture blades 4 to 32
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraSetApertureBlades(rpr_camera camera, rpr_uint num_blades);

/** @brief Set the exposure of a camera
 *
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  camera    The camera to set aperture blades for
 *  @param  exposure  Exposure value 0.0 - 1.0
 *  @return           RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraSetExposure(rpr_camera camera, rpr_float exposure);

/** @brief Set camera mode
 *
 *  Camera modes include:
 *      RPR_CAMERA_MODE_PERSPECTIVE
 *      RPR_CAMERA_MODE_ORTHOGRAPHIC
 *      RPR_CAMERA_MODE_LATITUDE_LONGITUDE_360
 *      RPR_CAMERA_MODE_LATITUDE_LONGITUDE_STEREO
 *      RPR_CAMERA_MODE_CUBEMAP
 *      RPR_CAMERA_MODE_CUBEMAP_STEREO
 *
 *  @param  camera  The camera to set mode for
 *  @param  mode    Camera mode, default is RPR_CAMERA_MODE_PERSPECTIVE
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraSetMode(rpr_camera camera, rpr_camera_mode mode);

/** @brief Set orthographic view volume width
 *
 *  @param  camera  The camera to set volume width for
 *  @param  width   View volume width in meters, default is 1 meter
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprCameraSetOrthoWidth(rpr_camera camera, rpr_float width);
extern RPR_API_ENTRY rpr_int rprCameraSetFocalTilt(rpr_camera camera, rpr_float tilt);
extern RPR_API_ENTRY rpr_int rprCameraSetIPD(rpr_camera camera, rpr_float ipd);
extern RPR_API_ENTRY rpr_int rprCameraSetLensShift(rpr_camera camera, rpr_float shiftx, rpr_float shifty);

/** @brief Set orthographic view volume height
*
*  @param  camera  The camera to set volume height for
*  @param  width   View volume height in meters, default is 1 meter
*  @return         RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprCameraSetOrthoHeight(rpr_camera camera, rpr_float height);

/* rpr_image*/
/** @brief Query information about an image
 *
 *  The workflow is usually two-step: query with the data == NULL to get the required buffer size,
 *  then query with size_ret == NULL to fill the buffer with the data
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  image       An image object to query
 *  @param  image_info  The type of info to query
 *  @param  size        The size of the buffer pointed by data
 *  @param  data        The buffer to store queried info
 *  @param  size_ret    Returns the size in bytes of the data being queried
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprImageGetInfo(rpr_image image, rpr_image_info image_info, size_t size, void * data, size_t * size_ret);

/** @brief 
*
*
*  @param  image       The image to set wrap for
*  @param  type	   
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprImageSetWrap(rpr_image image, rpr_image_wrap_type type);

/* rpr_shape */
/** @brief Set shape world transform
 *
 *
 *  @param  shape       The shape to set transform for
 *  @param  transpose   Determines whether the basis vectors are in columns(false) or in rows(true) of the matrix
 *  @param  transform   Array of 16 rpr_float values (row-major form)
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprShapeSetTransform(rpr_shape shape, rpr_bool transpose, rpr_float const * transform);

/** @brief Set shape subdivision
*
*
*  @param  shape       The shape to set subdivision for
*  @param  factor	   Number of subdivision steps to do
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetSubdivisionFactor(rpr_shape shape, rpr_uint factor);

/** @brief 
*
*
*  @param  shape       The shape to set subdivision for
*  @param  factor	   
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetSubdivisionCreaseWeight(rpr_shape shape, rpr_float factor);

/** @brief 
*
*
*  @param  shape       The shape to set subdivision for
*  @param  type	   
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetSubdivisionBoundaryInterop(rpr_shape shape, rpr_subdiv_boundary_interfop_type type);

/** @brief Set displacement scale
*
*
*  @param  shape       The shape to set subdivision for
*  @param  scale	   The amount of displacement applied
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetDisplacementScale(rpr_shape shape, rpr_float minscale, rpr_float maxscale);

/** @brief Set object group ID (mainly for debugging).
*
*
*  @param  shape          The shape to set
*  @param  objectGroupID  The ID
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetObjectGroupID(rpr_shape shape, rpr_uint objectGroupID);

/** @brief Set displacement texture
*
*
*  @param  shape       The shape to set subdivision for
*  @param  image 	   Displacement texture (scalar displacement, only x component is used)
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetDisplacementImage(rpr_shape shape, rpr_image image);

/* rpr_shape */
/** @brief Set shape material
*
*/
extern RPR_API_ENTRY rpr_int rprShapeSetMaterial(rpr_shape shape, rpr_material_node node);

/** @brief Set shape material override
*
*/
extern RPR_API_ENTRY rpr_int rprShapeSetMaterialOverride(rpr_shape shape, rpr_material_node node);

/** @brief Set shape volume material
*
*/
extern RPR_API_ENTRY rpr_int rprShapeSetVolumeMaterial(rpr_shape shape, rpr_material_node node);

/** @brief Set shape linear motion
 *
 *  @param  shape       The shape to set linear motion for
 *  @param  x           X component of a motion vector
 *  @param  y           Y component of a motion vector
 *  @param  z           Z component of a motion vector
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprShapeSetLinearMotion(rpr_shape shape, rpr_float x, rpr_float y, rpr_float z);

/** @brief Set angular linear motion
 *
 *  @param  shape       The shape to set linear motion for
 *  @param  x           X component of the rotation axis
 *  @param  y           Y component of the rotation axis
 *  @param  z           Z component of the rotation axis
 *  @param  w           W rotation angle in radians
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprShapeSetAngularMotion(rpr_shape shape, rpr_float x, rpr_float y, rpr_float z, rpr_float w);

/** @brief Set visibility flag
 *
 *  @param  shape       The shape to set visibility for
 *  @param  visible     Determines if the shape is visible or not
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprShapeSetVisibility(rpr_shape shape, rpr_bool visible);

/** @brief Set visibility flag for primary rays
*
*  @param  shape       The shape to set visibility for
*  @param  visible     Determines if the shape is visible or not
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetVisibilityPrimaryOnly(rpr_shape shape, rpr_bool visible);

/** @brief Set visibility flag for specular refleacted\refracted rays
*
*  @param  shape       The shape to set visibility for
*  @param  visible     Determines if the shape is visible or not
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetVisibilityInSpecular(rpr_shape shape, rpr_bool visible);

/** @brief Set shadow catcher flag
*
*  @param  shape         The shape to set shadow catcher flag for
*  @param  shadowCatcher Determines if the shape behaves as shadow catcher
*  @return               RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetShadowCatcher(rpr_shape shape, rpr_bool shadowCatcher);

/** @brief Set shadow flag
*
*  @param  shape       The shape to set shadow flag for
*  @param  visible     Determines if the shape casts shadow
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprShapeSetShadow(rpr_shape shape, rpr_bool casts_shadow);

/** @brief Set light world transform
 *
 *
 *  @param  light       The light to set transform for
 *  @param  transpose   Determines whether the basis vectors are in columns(false) or in rows(true) of the matrix
 *  @param  transform   Array of 16 rpr_float values (row-major form)
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprLightSetTransform(rpr_light light, rpr_bool transpose, rpr_float const * transform);

/** @brief Query information about a shape
 *
 *  The workflow is usually two-step: query with the data == NULL to get the required buffer size,
 *  then query with size_ret == NULL to fill the buffer with the data
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  shape           The shape object to query
 *  @param  material_info   The type of info to query
 *  @param  size            The size of the buffer pointed by data
 *  @param  data            The buffer to store queried info
 *  @param  size_ret        Returns the size in bytes of the data being queried
 *  @return                 RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprShapeGetInfo(rpr_shape arg0, rpr_shape_info arg1, size_t arg2, void * arg3, size_t * arg4);

/* rpr_shape - mesh */
/** @brief Query information about a mesh
 *
 *  The workflow is usually two-step: query with the data == NULL to get the required buffer size,
 *  then query with size_ret == NULL to fill the buffer with the data
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  shape       The mesh to query
 *  @param  mesh_info   The type of info to query
 *  @param  size        The size of the buffer pointed by data
 *  @param  data        The buffer to store queried info
 *  @param  size_ret    Returns the size in bytes of the data being queried
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprMeshGetInfo(rpr_shape mesh, rpr_mesh_info mesh_info, size_t size, void * data, size_t * size_ret);

/** @brief Query information about a mesh polygon
 *
 *  The workflow is usually two-step: query with the data == NULL to get the required buffer size,
 *  then query with size_ret == NULL to fill the buffer with the data
 *
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  mesh        The mesh to query
 *  @param  polygon_index The index of a polygon
 *  @param  polygon_info The type of info to query
 *  @param  size        The size of the buffer pointed by data
 *  @param  data        The buffer to store queried info
 *  @param  size_ret    Returns the size in bytes of the data being queried
 *  @return             RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprMeshPolygonGetInfo(rpr_shape mesh, size_t polygon_index, rpr_mesh_polygon_info polygon_info, size_t size, void * data, size_t * size_ret);

/** @brief Get the parent shape for an instance
 *
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  shape    The shape to get a parent shape from
 *  @param  status   RPR_SUCCESS in case of success, error code otherwise
 *  @return          Shape object
 */
extern RPR_API_ENTRY rpr_int rprInstanceGetBaseShape(rpr_shape shape, rpr_shape * out_shape);

/* rpr_light - point */
/** @brief Create point light
 *
 *  Create analytic point light represented by a point in space.
  *  Possible error codes:
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *
 *  @param  context The context to create a light for
 *  @param  status  RPR_SUCCESS in case of success, error code otherwise
 *  @return         Light object
 */
extern RPR_API_ENTRY rpr_int rprContextCreatePointLight(rpr_context context, rpr_light * out_light);

/** @brief Set radiant power of a point light source
 *
 *  @param  r       R component of a radiant power vector
 *  @param  g       G component of a radiant power vector
 *  @param  b       B component of a radiant power vector
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprPointLightSetRadiantPower3f(rpr_light light, rpr_float r, rpr_float g, rpr_float b);

/* rpr_light - spot */
/** @brief Create spot light
 *
 *  Create analytic spot light
 *
 *  Possible error codes:
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *
 *  @param  context The context to create a light for
 *  @param  status  RPR_SUCCESS in case of success, error code otherwise
 *  @return         Light object
 */
extern RPR_API_ENTRY rpr_int rprContextCreateSpotLight(rpr_context context, rpr_light * light);

/** @brief Set radiant power of a spot light source
 *
 *  @param  r R component of a radiant power vector
 *  @param  g G component of a radiant power vector
 *  @param  b B component of a radiant power vector
 *  @return   RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprSpotLightSetRadiantPower3f(rpr_light light, rpr_float r, rpr_float g, rpr_float b);

/** @brief Set cone shape for a spot light
 *
 * Spot light produces smooth penumbra in a region between inner and outer circles,
 * the area inside the inner cicrle receives full power while the area outside the
 * outer one is fully in shadow.
 *
 *  @param  iangle Inner angle of a cone in radians
 *  @param  oangle Outer angle of a coner in radians, should be greater that or equal to inner angle
 *  @return status RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprSpotLightSetConeShape(rpr_light light, rpr_float iangle, rpr_float oangle);

/* rpr_light - directional */
/** @brief Create directional light
 *
 *  Create analytic directional light.
 *  Possible error codes:
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *
 *  @param  context The context to create a light for
 *  @param  status  RPR_SUCCESS in case of success, error code otherwise
 *  @return light id of a newly created light
 */
extern RPR_API_ENTRY rpr_int rprContextCreateDirectionalLight(rpr_context context, rpr_light * out_light);

/** @brief Set radiant power of a directional light source
 *
 *  @param  r R component of a radiant power vector
 *  @param  g G component of a radiant power vector
 *  @param  b B component of a radiant power vector
 *  @return   RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprDirectionalLightSetRadiantPower3f(rpr_light light, rpr_float r, rpr_float g, rpr_float b);

/** @brief Set softness of shadow produced by the light
 *
 *  @param  coeff value between [0;1]. 0.0 is sharp  
 *  @return   RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprDirectionalLightSetShadowSoftness(rpr_light light, rpr_float coeff);

/* rpr_light - environment */
/** @brief Create an environment light
 *
 *  Environment light is a light based on lightprobe.
 *  Possible error codes:
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *
 *  @param  context The context to create a light for
 *  @param  status  RPR_SUCCESS in case of success, error code otherwise
 *  @return         Light object
 */
extern RPR_API_ENTRY rpr_int rprContextCreateEnvironmentLight(rpr_context context, rpr_light * out_light);

/** @brief Set image for an environment light
 *
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *      RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT
 *
 *  @param  env_light Environment light
 *  @param  image     Image object to set
 *  @return           RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprEnvironmentLightSetImage(rpr_light env_light, rpr_image image);

/** @brief Set intensity scale or an env light
 *
 *  @param  env_light       Environment light
 *  @param  intensity_scale Intensity scale
 *  @return                 RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprEnvironmentLightSetIntensityScale(rpr_light env_light, rpr_float intensity_scale);

/** @brief Set portal for environment light to accelerate convergence of indoor scenes
*
*   Possible error codes:
*      RPR_ERROR_INVALID_PARAMETER
*
*  @param  env_light Environment light
*  @param  portal    Portal mesh, might have multiple components
*  @return           RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprEnvironmentLightAttachPortal(rpr_light env_light, rpr_shape portal);

/** @brief Remove portal for environment light.
*
*   Possible error codes:
*      RPR_ERROR_INVALID_PARAMETER
*
*  @param  env_light Environment light
*  @param  portal    Portal mesh, that have been added to light.
*  @return           RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprEnvironmentLightDetachPortal(rpr_light env_light, rpr_shape portal);

/* rpr_light - sky */
/** @brief Create sky light
*
*  Analytical sky model
*  Possible error codes:
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*
*  @param  context The context to create a light for
*  @param  status  RPR_SUCCESS in case of success, error code otherwise
*  @return         Light object
*/
extern RPR_API_ENTRY rpr_int rprContextCreateSkyLight(rpr_context context, rpr_light * out_light);

/** @brief Set turbidity of a sky light
*
*  @param  skylight        Sky light
*  @param  turbidity       Turbidity value
*  @return                 RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprSkyLightSetTurbidity(rpr_light skylight, rpr_float turbidity);

/** @brief Set albedo of a sky light
*
*  @param  skylight        Sky light
*  @param  albedo          Albedo value
*  @return                 RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprSkyLightSetAlbedo(rpr_light skylight, rpr_float albedo);

/** @brief Set scale of a sky light
*
*  @param  skylight        Sky light
*  @param  scale           Scale value
*  @return                 RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprSkyLightSetScale(rpr_light skylight, rpr_float scale);

/** @brief Set portal for sky light to accelerate convergence of indoor scenes
*
*   Possible error codes:
*      RPR_ERROR_INVALID_PARAMETER
*
*  @param  skylight  Sky light
*  @param  portal    Portal mesh, might have multiple components
*  @return           RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprSkyLightAttachPortal(rpr_light skylight, rpr_shape portal);

/** @brief Remove portal for Sky light.
*
*   Possible error codes:
*      RPR_ERROR_INVALID_PARAMETER
*
*  @param  env_light Sky light
*  @param  portal    Portal mesh, that have been added to light.
*  @return           RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprSkyLightDetachPortal(rpr_light skylight, rpr_shape portal);

/** @brief Create IES light
*
*  Create IES light
*
*  Possible error codes:
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*
*  @param  context The context to create a light for
*  @param  status  RPR_SUCCESS in case of success, error code otherwise
*  @return         Light object
*/
extern RPR_API_ENTRY rpr_int rprContextCreateIESLight(rpr_context context, rpr_light * light);

/** @brief Set radiant power of a IES light source
*
*  @param  r R component of a radiant power vector
*  @param  g G component of a radiant power vector
*  @param  b B component of a radiant power vector
*  @return   RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprIESLightSetRadiantPower3f(rpr_light light, rpr_float r, rpr_float g, rpr_float b);

/** @brief Set image for an IES light
*
*   Possible error codes:
*      RPR_ERROR_INVALID_PARAMETER
*      RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT : If the format of the IES file is not supported by Radeon ProRender.
*      RPR_ERROR_IO_ERROR : If the IES image path file doesn't exist.
*
*  @param  env_light     Environment light
*  @param  imagePath     Image path to set
*  @param  nx			  resolution X of the IES image
*  @param  ny            resolution Y of the IES image
*  @return               RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprIESLightSetImageFromFile(rpr_light env_light, rpr_char const * imagePath, rpr_int nx, rpr_int ny);

/** @brief Set image for an IES light
*
*   Possible error codes:
*      RPR_ERROR_INVALID_PARAMETER
*      RPR_ERROR_UNSUPPORTED_IMAGE_FORMAT : If the format of the IES data is not supported by Radeon ProRender.
*
*  @param  env_light     Environment light
*  @param  iesData       Image data string defining the IES. null terminated string. IES format.
*  @param  nx			  resolution X of the IES image
*  @param  ny            resolution Y of the IES image
*  @return               RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprIESLightSetImageFromIESdata(rpr_light env_light, rpr_char const * iesData, rpr_int nx, rpr_int ny);

/* rpr_light */
/** @brief Query information about a light
 *
 *  The workflow is usually two-step: query with the data == NULL to get the required buffer size,
 *  then query with size_ret == NULL to fill the buffer with the data
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  light    The light to query
 *  @param  light_info The type of info to query
 *  @param  size     The size of the buffer pointed by data
 *  @param  data     The buffer to store queried info
 *  @param  size_ret Returns the size in bytes of the data being queried
 *  @return          RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprLightGetInfo(rpr_light light, rpr_light_info info, size_t size, void * data, size_t * size_ret);

/* rpr_scene */
/** @brief Remove all objects from a scene
 *
 *  A scene is essentially a collection of shapes, lights and volume regions.
 *
 *  @param  scene   The scene to clear
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprSceneClear(rpr_scene scene);

/** @brief Attach a shape to the scene
 *
 *  A scene is essentially a collection of shapes, lights and volume regions.
 *
 *  @param  scene  The scene to attach
 *  @param  shape  The shape to attach
 *  @return        RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprSceneAttachShape(rpr_scene scene, rpr_shape shape);

/** @brief Detach a shape from the scene
 *
 *  A scene is essentially a collection of shapes, lights and volume regions.
 *
 *  @param  scene   The scene to dettach from
 *  @return         RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprSceneDetachShape(rpr_scene scene, rpr_shape shape);

/** @brief Attach a light to the scene
 *
 *  A scene is essentially a collection of shapes, lights and volume regions
 *
 *  @param  scene  The scene to attach
 *  @param  light  The light to attach
 *  @return        RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprSceneAttachLight(rpr_scene scene, rpr_light light);

/** @brief Detach a light from the scene
 *
 *  A scene is essentially a collection of shapes, lights and volume regions
 *
 *  @param  scene  The scene to dettach from
 *  @param  light  The light to detach
 *  @return        RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprSceneDetachLight(rpr_scene scene, rpr_light light);

/** @brief Query information about a scene
 *
 *  The workflow is usually two-step: query with the data == NULL to get the required buffer size,
 *  then query with size_ret == NULL to fill the buffer with the data
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  scene    The scene to query
 *  @param  info     The type of info to query
 *  @param  size     The size of the buffer pointed by data
 *  @param  data     The buffer to store queried info
 *  @param  size_ret Returns the size in bytes of the data being queried
 *  @return          RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprSceneGetInfo(rpr_scene scene, rpr_scene_info info, size_t size, void * data, size_t * size_ret);

/** @brief Get background override light 
*   Possible error codes:
*      RPR_ERROR_INVALID_PARAMETER
*
*  @param  scene       The scene to set background for
*  @param  overrride   overrride type
*  @param  out_light   light returned
*  @return        RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprSceneGetEnvironmentOverride(rpr_scene scene, rpr_environment_override overrride, rpr_light * out_light);

/** @brief Set background light for the scene which does not affect the scene lighting,
*    but gets shown as a background image
*   Possible error codes:
*      RPR_ERROR_INVALID_PARAMETER
*
*  @param  scene  The scene to set background for
*  @param  light  Background light
*  @return        RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprSceneSetEnvironmentOverride(rpr_scene scene, rpr_environment_override overrride, rpr_light light);

/** @brief Set background image for the scene which does not affect the scene lighting,
*    it is shown as view-independent rectangular background
*   Possible error codes:
*      RPR_ERROR_INVALID_PARAMETER
*
*  @param  scene  The scene to set background for
*  @param  image  Background image
*  @return        RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprSceneSetBackgroundImage(rpr_scene scene, rpr_image image);

/** @brief Get background image
*
*  @param  scene  The scene to get background image from
*  @param  status RPR_SUCCESS in case of success, error code otherwise
*  @return        Image object
*/
extern RPR_API_ENTRY rpr_int rprSceneGetBackgroundImage(rpr_scene scene, rpr_image * out_image);

/** @brief Set camera for the scene
 *
 *  This is the main camera which for rays generation for the scene.
 *
 *  @param  scene  The scene to set camera for
 *  @param  camera Camera
 *  @return        RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprSceneSetCamera(rpr_scene scene, rpr_camera camera);

/** @brief Get camera for the scene
 *
 *  @param  scene  The scene to get camera for
 *  @param  status RPR_SUCCESS in case of success, error code otherwise
 *  @return camera id for the camera if any, NULL otherwise
 */
extern RPR_API_ENTRY rpr_int rprSceneGetCamera(rpr_scene scene, rpr_camera * out_camera);

/* rpr_framebuffer*/
/** @brief Query information about a framebuffer
 *
 *  The workflow is usually two-step: query with the data == NULL to get the required buffer size,
 *  then query with size_ret == NULL to fill the buffer with the data
 *   Possible error codes:
 *      RPR_ERROR_INVALID_PARAMETER
 *
 *  @param  framebuffer  Framebuffer object to query
 *  @param  info         The type of info to query
 *  @param  size         The size of the buffer pointed by data
 *  @param  data         The buffer to store queried info
 *  @param  size_ret     Returns the size in bytes of the data being queried
 *  @return              RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprFrameBufferGetInfo(rpr_framebuffer framebuffer, rpr_framebuffer_info info, size_t size, void * data, size_t * size_ret);

/** @brief Clear contents of a framebuffer to zero
 *
 *   Possible error codes:
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *
 *  The call is blocking and the image is ready when returned
 *
 *  @param  frame_buffer  Framebuffer to clear
 *  @return              RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprFrameBufferClear(rpr_framebuffer frame_buffer);

/** @brief Save frame buffer to file
 *
 *   Possible error codes:
 *      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
 *      RPR_ERROR_OUT_OF_VIDEO_MEMORY
 *
 *  @param  frame_buffer Frame buffer to save
 *  @param  path         Path to file
 *  @return              RPR_SUCCESS in case of success, error code otherwise
 */
extern RPR_API_ENTRY rpr_int rprFrameBufferSaveToFile(rpr_framebuffer frame_buffer, rpr_char const * file_path);

/** @brief Resolve framebuffer
*
*   Resolve applies AA filters and tonemapping operators to the framebuffer data
*
*   Possible error codes:
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*/
extern RPR_API_ENTRY rpr_int rprContextResolveFrameBuffer(rpr_context context, rpr_framebuffer src_frame_buffer, rpr_framebuffer dst_frame_buffer, rpr_bool normalizeOnly = false);

/** @brief Create material system
*
*   Possible error codes:
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*
*/
extern RPR_API_ENTRY rpr_int rprContextCreateMaterialSystem(rpr_context in_context, rpr_material_system_type type, rpr_material_system * out_matsys);

/** @brief Create material node
*
*   Possible error codes:
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*
*/
extern RPR_API_ENTRY rpr_int rprMaterialSystemCreateNode(rpr_material_system in_matsys, rpr_material_node_type in_type, rpr_material_node * out_node);

/** @brief Connect nodes
*
*   Possible error codes:
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*
*/
extern RPR_API_ENTRY rpr_int rprMaterialNodeSetInputN(rpr_material_node in_node, rpr_char const * in_input, rpr_material_node in_input_node);

/** @brief Set float input value
*
*   Possible error codes:
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*
*/
extern RPR_API_ENTRY rpr_int rprMaterialNodeSetInputF(rpr_material_node in_node, rpr_char const * in_input, rpr_float in_value_x, rpr_float in_value_y, rpr_float in_value_z, rpr_float in_value_w);

/** @brief Set uint input value
*
*   Possible error codes:
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*
*/
extern RPR_API_ENTRY rpr_int rprMaterialNodeSetInputU(rpr_material_node in_node, rpr_char const * in_input, rpr_uint in_value);

/** @brief Set image input value
*
*   Possible error codes:
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*
*/
extern RPR_API_ENTRY rpr_int rprMaterialNodeSetInputImageData(rpr_material_node in_node, rpr_char const * in_input, rpr_image image);
extern RPR_API_ENTRY rpr_int rprMaterialNodeGetInfo(rpr_material_node in_node, rpr_material_node_info in_info, size_t in_size, void * in_data, size_t * out_size);
extern RPR_API_ENTRY rpr_int rprMaterialNodeGetInputInfo(rpr_material_node in_node, rpr_int in_input_idx, rpr_material_node_input_info in_info, size_t in_size, void * in_data, size_t * out_size);

/** @brief Delete object
*
*  rprObjectDelete(obj) deletes 'obj' from memory. 
*  User has to make sure that 'obj' will not be used anymore after this call.
*
*   Possible error codes:
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*
*/
extern RPR_API_ENTRY rpr_int rprObjectDelete(void * obj);

/** @brief Set material node name
*
*
*  @param  node        Node to set the name for
*  @param  name       NULL terminated string name
*  @return             RPR_SUCCESS in case of success, error code otherwise
*/
extern RPR_API_ENTRY rpr_int rprObjectSetName(void * node, rpr_char const * name);

/* rpr_post_effect */
/** @brief Create post effect
*
*  Create analytic point light represented by a point in space.
*  Possible error codes:
*      RPR_ERROR_OUT_OF_VIDEO_MEMORY
*      RPR_ERROR_OUT_OF_SYSTEM_MEMORY
*
*  @param  context The context to create a light for
*  @param  status  RPR_SUCCESS in case of success, error code otherwise
*  @return         Light object
*/
extern RPR_API_ENTRY rpr_int rprContextCreatePostEffect(rpr_context context, rpr_post_effect_type type, rpr_post_effect * out_effect);
extern RPR_API_ENTRY rpr_int rprContextAttachPostEffect(rpr_context context, rpr_post_effect effect);
extern RPR_API_ENTRY rpr_int rprContextDetachPostEffect(rpr_context context, rpr_post_effect effect);
extern RPR_API_ENTRY rpr_int rprPostEffectSetParameter1u(rpr_post_effect effect, rpr_char const * name, rpr_uint x);
extern RPR_API_ENTRY rpr_int rprPostEffectSetParameter1f(rpr_post_effect effect, rpr_char const * name, rpr_float x);
extern RPR_API_ENTRY rpr_int rprPostEffectSetParameter3f(rpr_post_effect effect, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z);
extern RPR_API_ENTRY rpr_int rprPostEffectSetParameter4f(rpr_post_effect effect, rpr_char const * name, rpr_float x, rpr_float y, rpr_float z, rpr_float w);
/***************compatibility part***************/
#define FR_API_VERSION 0x010000252 
#define FR_SUCCESS 0 
#define FR_ERROR_COMPUTE_API_NOT_SUPPORTED -1 
#define FR_ERROR_OUT_OF_SYSTEM_MEMORY -2 
#define FR_ERROR_OUT_OF_VIDEO_MEMORY -3 
#define FR_ERROR_INVALID_LIGHTPATH_EXPR -5 
#define FR_ERROR_INVALID_IMAGE -6 
#define FR_ERROR_INVALID_AA_METHOD -7 
#define FR_ERROR_UNSUPPORTED_IMAGE_FORMAT -8 
#define FR_ERROR_INVALID_GL_TEXTURE -9 
#define FR_ERROR_INVALID_CL_IMAGE -10 
#define FR_ERROR_INVALID_OBJECT -11 
#define FR_ERROR_INVALID_PARAMETER -12 
#define FR_ERROR_INVALID_TAG -13 
#define FR_ERROR_INVALID_LIGHT -14 
#define FR_ERROR_INVALID_CONTEXT -15 
#define FR_ERROR_UNIMPLEMENTED -16 
#define FR_ERROR_INVALID_API_VERSION -17 
#define FR_ERROR_INTERNAL_ERROR -18 
#define FR_ERROR_IO_ERROR -19 
#define FR_ERROR_UNSUPPORTED_SHADER_PARAMETER_TYPE -20 
#define FR_ERROR_MATERIAL_STACK_OVERFLOW -21 
#define FR_ERROR_INVALID_PARAMETER_TYPE -22 
#define FR_ERROR_UNSUPPORTED -23 
#define FR_ERROR_OPENCL_OUT_OF_HOST_MEMORY -24 
#define FR_PARAMETER_TYPE_FLOAT 0x1 
#define FR_PARAMETER_TYPE_FLOAT2 0x2 
#define FR_PARAMETER_TYPE_FLOAT3 0x3 
#define FR_PARAMETER_TYPE_FLOAT4 0x4 
#define FR_PARAMETER_TYPE_IMAGE 0x5 
#define FR_PARAMETER_TYPE_STRING 0x6 
#define FR_PARAMETER_TYPE_SHADER 0x7 
#define FR_PARAMETER_TYPE_UINT 0x8 
#define FR_IMAGE_TYPE_1D 0x1 
#define FR_IMAGE_TYPE_2D 0x2 
#define FR_IMAGE_TYPE_3D 0x3 
#define FR_CONTEXT_OPENCL (1 << 0) 
#define FR_CONTEXT_DIRECTCOMPUTE (1 << 1) 
#define FR_CONTEXT_REFERENCE (1 << 2) 
#define FR_CONTEXT_OPENGL (1 << 3) 
#define FR_CONTEXT_METAL (1 << 4) 
#define FR_CREATION_FLAGS_ENABLE_GPU0 (1 << 0) 
#define FR_CREATION_FLAGS_ENABLE_GPU1 (1 << 1) 
#define FR_CREATION_FLAGS_ENABLE_GPU2 (1 << 2) 
#define FR_CREATION_FLAGS_ENABLE_GPU3 (1 << 3) 
#define FR_CREATION_FLAGS_ENABLE_CPU (1 << 4) 
#define FR_CREATION_FLAGS_ENABLE_GL_INTEROP (1 << 5) 
#define FR_CREATION_FLAGS_ENABLE_GPU4 (1 << 6) 
#define FR_CREATION_FLAGS_ENABLE_GPU5 (1 << 7) 
#define FR_CREATION_FLAGS_ENABLE_GPU6 (1 << 8) 
#define FR_CREATION_FLAGS_ENABLE_GPU7 (1 << 9) 
#define FR_FILTER_BOX 0x1 
#define FR_FILTER_TRIANGLE 0x2 
#define FR_FILTER_GAUSSIAN 0x3 
#define FR_FILTER_MITCHELL 0x4 
#define FR_FILTER_LANCZOS 0x5 
#define FR_FILTER_BLACKMANHARRIS 0x6 
#define FR_SHAPE_TYPE_MESH 0x1 
#define FR_SHAPE_TYPE_INSTANCE 0x2 
#define FR_LIGHT_TYPE_POINT 0x1 
#define FR_LIGHT_TYPE_DIRECTIONAL 0x2 
#define FR_LIGHT_TYPE_SPOT 0x3 
#define FR_LIGHT_TYPE_ENVIRONMENT 0x4 
#define FR_LIGHT_TYPE_SKY 0x5 
#define FR_LIGHT_TYPE_IES 0x6 
#define FR_OBJECT_NAME 0x777777 
#define FR_CONTEXT_CREATION_FLAGS 0x102 
#define FR_CONTEXT_CACHE_PATH 0x103 
#define FR_CONTEXT_RENDER_STATUS 0x104 
#define FR_CONTEXT_RENDER_STATISTICS 0x105 
#define FR_CONTEXT_DEVICE_COUNT 0x106 
#define FR_CONTEXT_PARAMETER_COUNT 0x107 
#define FR_CONTEXT_ACTIVE_PLUGIN 0x108 
#define FR_CONTEXT_SCENE 0x109 
#define FR_CONTEXT_AA_CELL_SIZE 0x10A 
#define FR_CONTEXT_AA_SAMPLES 0x10B 
#define FR_CONTEXT_IMAGE_FILTER_TYPE 0x10C 
#define FR_CONTEXT_IMAGE_FILTER_BOX_RADIUS 0x10D 
#define FR_CONTEXT_IMAGE_FILTER_GAUSSIAN_RADIUS 0x10E 
#define FR_CONTEXT_IMAGE_FILTER_TRIANGLE_RADIUS 0x10F 
#define FR_CONTEXT_IMAGE_FILTER_MITCHELL_RADIUS 0x110 
#define FR_CONTEXT_IMAGE_FILTER_LANCZOS_RADIUS 0x111 
#define FR_CONTEXT_IMAGE_FILTER_BLACKMANHARRIS_RADIUS 0x112 
#define FR_CONTEXT_TONE_MAPPING_TYPE 0x113 
#define FR_CONTEXT_TONE_MAPPING_LINEAR_SCALE 0x114 
#define FR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_SENSITIVITY 0x115 
#define FR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_EXPOSURE 0x116 
#define FR_CONTEXT_TONE_MAPPING_PHOTO_LINEAR_FSTOP 0x117 
#define FR_CONTEXT_TONE_MAPPING_REINHARD02_PRE_SCALE 0x118 
#define FR_CONTEXT_TONE_MAPPING_REINHARD02_POST_SCALE 0x119 
#define FR_CONTEXT_TONE_MAPPING_REINHARD02_BURN 0x11A 
#define FR_CONTEXT_MAX_RECURSION 0x11B 
#define FR_CONTEXT_RAY_CAST_EPISLON 0x11C 
#define FR_CONTEXT_RADIANCE_CLAMP 0x11D 
#define FR_CONTEXT_X_FLIP 0x11E 
#define FR_CONTEXT_Y_FLIP 0x11F 
#define FR_CONTEXT_TEXTURE_GAMMA 0x120 
#define FR_CONTEXT_PDF_THRESHOLD 0x121 
#define FR_CONTEXT_RENDER_MODE 0x122 
#define FR_CONTEXT_ROUGHNESS_CAP 0x123 
#define FR_CONTEXT_DISPLAY_GAMMA 0x124 
#define FR_CONTEXT_MATERIAL_STACK_SIZE 0x125 
#define FR_CONTEXT_CLIPPING_PLANE 0x126 
#define FR_CONTEXT_GPU0_NAME 0x127 
#define FR_CONTEXT_GPU1_NAME 0x128 
#define FR_CONTEXT_GPU2_NAME 0x129 
#define FR_CONTEXT_GPU3_NAME 0x12A 
#define FR_CONTEXT_CPU_NAME 0x12B 
#define FR_CONTEXT_GPU4_NAME 0x12C 
#define FR_CONTEXT_GPU5_NAME 0x12D 
#define FR_CONTEXT_GPU6_NAME 0x12E 
#define FR_CONTEXT_GPU7_NAME 0x12F 
#define FR_CONTEXT_TONE_MAPPING_EXPONENTIAL_INTENSITY 0x130 
#define FR_CONTEXT_FRAMECOUNT 0x131 
#define FR_CONTEXT_MAX 0x132 
#define FR_CAMERA_TRANSFORM 0x201 
#define FR_CAMERA_FSTOP 0x202 
#define FR_CAMERA_APERTURE_BLADES 0x203 
#define FR_CAMERA_RESPONSE 0x204 
#define FR_CAMERA_EXPOSURE 0x205 
#define FR_CAMERA_FOCAL_LENGTH 0x206 
#define FR_CAMERA_SENSOR_SIZE 0x207 
#define FR_CAMERA_MODE 0x208 
#define FR_CAMERA_ORTHO_WIDTH 0x209 
#define FR_CAMERA_ORTHO_HEIGHT 0x20A 
#define FR_CAMERA_FOCUS_DISTANCE 0x20B 
#define FR_CAMERA_POSITION 0x20C 
#define FR_CAMERA_LOOKAT 0x20D 
#define FR_CAMERA_UP 0x20E 
#define FR_CAMERA_FOCAL_TILT 0x20F 
#define FR_CAMERA_LENS_SHIFT 0x210 
#define FR_CAMERA_IPD 0x211 
#define FR_IMAGE_FORMAT 0x301 
#define FR_IMAGE_DESC 0x302 
#define FR_IMAGE_DATA 0x303 
#define FR_IMAGE_DATA_SIZEBYTE 0x304 
#define FR_IMAGE_WRAP 0x305 
#define FR_SHAPE_TYPE 0x401 
#define FR_SHAPE_VIDMEM_USAGE 0x402 
#define FR_SHAPE_TRANSFORM 0x403 
#define FR_SHAPE_MATERIAL 0x404 
#define FR_SHAPE_LINEAR_MOTION 0x405 
#define FR_SHAPE_ANGULAR_MOTION 0x406 
#define FR_SHAPE_VISIBILITY_FLAG 0x407 
#define FR_SHAPE_SHADOW_FLAG 0x408 
#define FR_SHAPE_SUBDIVISION_FACTOR 0x409 
#define FR_SHAPE_DISPLACEMENT_SCALE 0x40A 
#define FR_SHAPE_DISPLACEMENT_IMAGE 0X40B 
#define FR_SHAPE_VISIBILITY_PRIMARY_ONLY_FLAG 0x40C 
#define FR_SHAPE_VISIBILITY_IN_SPECULAR_FLAG 0x40D 
#define FR_SHAPE_SHADOW_CATCHER_FLAG 0x40E 
#define FR_SHAPE_VOLUME_MATERIAL 0x40F 
#define FR_SHAPE_OBJECT_GROUP_ID 0x410 
#define FR_SHAPE_SUBDIVISION_CREASEWEIGHT 0x411 
#define FR_SHAPE_SUBDIVISION_BOUNDARYINTEROP 0x412 
#define FR_SHAPE_MATERIAL_OVERRIDE 0x413 
#define FR_MESH_POLYGON_COUNT 0x501 
#define FR_MESH_VERTEX_COUNT 0x502 
#define FR_MESH_NORMAL_COUNT 0x503 
#define FR_MESH_UV_COUNT 0x504 
#define FR_MESH_VERTEX_ARRAY 0x505 
#define FR_MESH_NORMAL_ARRAY 0x506 
#define FR_MESH_UV_ARRAY 0x507 
#define FR_MESH_VERTEX_INDEX_ARRAY 0x508 
#define FR_MESH_NORMAL_INDEX_ARRAY 0x509 
#define FR_MESH_UV_INDEX_ARRAY 0x50A 
#define FR_MESH_VERTEX_STRIDE 0x50C 
#define FR_MESH_NORMAL_STRIDE 0x50D 
#define FR_MESH_UV_STRIDE 0x50E 
#define FR_MESH_VERTEX_INDEX_STRIDE 0x50F 
#define FR_MESH_NORMAL_INDEX_STRIDE 0x510 
#define FR_MESH_UV_INDEX_STRIDE 0x511 
#define FR_MESH_NUM_FACE_VERTICES_ARRAY 0x512 
#define FR_MESH_UV2_COUNT 0x513 
#define FR_MESH_UV2_ARRAY 0x514 
#define FR_MESH_UV2_INDEX_ARRAY 0x515 
#define FR_MESH_UV2_STRIDE 0x516 
#define FR_MESH_UV2_INDEX_STRIDE 0x517 
#define FR_SCENE_SHAPE_COUNT 0x701 
#define FR_SCENE_LIGHT_COUNT 0x702 
#define FR_SCENE_SHAPE_LIST 0x704 
#define FR_SCENE_LIGHT_LIST 0x705 
#define FR_SCENE_CAMERA 0x706 
#define FR_SCENE_BACKGROUND_IMAGE 0x708 
#define FR_SCENE_ENVIRONMENT_OVERRIDE_REFLECTION 0x709 
#define FR_SCENE_ENVIRONMENT_OVERRIDE_REFRACTION 0x70A 
#define FR_SCENE_ENVIRONMENT_OVERRIDE_TRANSPARENCY 0x70B 
#define FR_SCENE_ENVIRONMENT_OVERRIDE_BACKGROUND 0x70C 
#define FR_SCENE_AXIS_ALIGNED_BOUNDING_BOX 0x70D
#define FR_LIGHT_TYPE 0x801 
#define FR_LIGHT_TRANSFORM 0x803 
#define FR_POINT_LIGHT_RADIANT_POWER 0x804 
#define FR_DIRECTIONAL_LIGHT_RADIANT_POWER 0x808 
#define FR_DIRECTIONAL_LIGHT_SHADOW_SOFTNESS 0x809 
#define FR_SPOT_LIGHT_RADIANT_POWER 0x80B 
#define FR_SPOT_LIGHT_CONE_SHAPE 0x80C 
#define FR_ENVIRONMENT_LIGHT_IMAGE 0x80F 
#define FR_ENVIRONMENT_LIGHT_INTENSITY_SCALE 0x810 
#define FR_ENVIRONMENT_LIGHT_PORTAL_LIST 0x818 
#define FR_ENVIRONMENT_LIGHT_PORTAL_COUNT 0x819 
#define FR_SKY_LIGHT_TURBIDITY 0x812 
#define FR_SKY_LIGHT_ALBEDO 0x813 
#define FR_SKY_LIGHT_SCALE 0x814 
#define FR_SKY_LIGHT_PORTAL_LIST 0x820 
#define FR_SKY_LIGHT_PORTAL_COUNT 0x821 
#define FR_IES_LIGHT_RADIANT_POWER 0x816 
#define FR_IES_LIGHT_IMAGE_DESC 0x817 
#define FR_PARAMETER_NAME 0x1201 
#define FR_PARAMETER_NAME_STRING 0x1202 
#define FR_PARAMETER_TYPE 0x1203 
#define FR_PARAMETER_DESCRIPTION 0x1204 
#define FR_PARAMETER_VALUE 0x1205 
#define FR_FRAMEBUFFER_FORMAT 0x1301 
#define FR_FRAMEBUFFER_DESC 0x1302 
#define FR_FRAMEBUFFER_DATA 0x1303 
#define FR_FRAMEBUFFER_GL_TARGET 0x1304 
#define FR_FRAMEBUFFER_GL_MIPLEVEL 0x1305 
#define FR_FRAMEBUFFER_GL_TEXTURE 0x1306 
#define FR_MESH_POLYGON_VERTEX_COUNT 0x1401 
#define FR_MESH_POLYGON_VERTEX_POSITION 0x1501 
#define FR_MESH_POLYGON_VERTEX_NORMAL 0x1502 
#define FR_MESH_POLYGON_VERTEX_TEXCOORD 0x1503 
#define FR_INSTANCE_PARENT_SHAPE 0x1601 
#define FR_COMPONENT_TYPE_UINT8 0x1 
#define FR_COMPONENT_TYPE_FLOAT16 0x2 
#define FR_COMPONENT_TYPE_FLOAT32 0x3 
#define FR_RENDER_MODE_GLOBAL_ILLUMINATION 0x1 
#define FR_RENDER_MODE_DIRECT_ILLUMINATION 0x2 
#define FR_RENDER_MODE_DIRECT_ILLUMINATION_NO_SHADOW 0x3 
#define FR_RENDER_MODE_WIREFRAME 0x4 
#define FR_RENDER_MODE_MATERIAL_INDEX 0x5 
#define FR_RENDER_MODE_POSITION 0x6 
#define FR_RENDER_MODE_NORMAL 0x7 
#define FR_RENDER_MODE_TEXCOORD 0x8 
#define FR_RENDER_MODE_AMBIENT_OCCLUSION 0x9 
#define FR_CAMERA_MODE_PERSPECTIVE 0x1 
#define FR_CAMERA_MODE_ORTHOGRAPHIC 0x2 
#define FR_CAMERA_MODE_LATITUDE_LONGITUDE_360 0x3 
#define FR_CAMERA_MODE_LATITUDE_LONGITUDE_STEREO 0x4 
#define FR_CAMERA_MODE_CUBEMAP 0x5 
#define FR_CAMERA_MODE_CUBEMAP_STEREO 0x6 
#define FR_TONEMAPPING_OPERATOR_NONE 0x0 
#define FR_TONEMAPPING_OPERATOR_LINEAR 0x1 
#define FR_TONEMAPPING_OPERATOR_PHOTOLINEAR 0x2 
#define FR_TONEMAPPING_OPERATOR_AUTOLINEAR 0x3 
#define FR_TONEMAPPING_OPERATOR_MAXWHITE 0x4 
#define FR_TONEMAPPING_OPERATOR_REINHARD02 0x5 
#define FR_TONEMAPPING_OPERATOR_EXPONENTIAL 0x6 
#define FR_VOLUME_TYPE_NONE 0xFFFF 
#define FR_VOLUME_TYPE_HOMOGENEOUS 0x0 
#define FR_VOLUME_TYPE_HETEROGENEOUS 0x1 
#define FR_MATERIAL_NODE_TYPE 0x1101 
#define FR_MATERIAL_NODE_SYSTEM 0x1102 
#define FR_MATERIAL_NODE_INPUT_COUNT 0x1103 
#define FR_MATERIAL_NODE_INPUT_NAME 0x1103 
#define FR_MATERIAL_NODE_INPUT_NAME_STRING 0x1104 
#define FR_MATERIAL_NODE_INPUT_DESCRIPTION 0x1105 
#define FR_MATERIAL_NODE_INPUT_VALUE 0x1106 
#define FR_MATERIAL_NODE_INPUT_TYPE 0x1107 
#define FR_MATERIAL_NODE_DIFFUSE 0x1 
#define FR_MATERIAL_NODE_MICROFACET 0x2 
#define FR_MATERIAL_NODE_REFLECTION 0x3 
#define FR_MATERIAL_NODE_REFRACTION 0x4 
#define FR_MATERIAL_NODE_MICROFACET_REFRACTION 0x5 
#define FR_MATERIAL_NODE_TRANSPARENT 0x6 
#define FR_MATERIAL_NODE_EMISSIVE 0x7 
#define FR_MATERIAL_NODE_WARD 0x8 
#define FR_MATERIAL_NODE_ADD 0x9 
#define FR_MATERIAL_NODE_BLEND 0xA 
#define FR_MATERIAL_NODE_ARITHMETIC 0xB 
#define FR_MATERIAL_NODE_FRESNEL 0xC 
#define FR_MATERIAL_NODE_NORMAL_MAP 0xD 
#define FR_MATERIAL_NODE_IMAGE_TEXTURE 0xE 
#define FR_MATERIAL_NODE_NOISE2D_TEXTURE 0xF 
#define FR_MATERIAL_NODE_DOT_TEXTURE 0x10 
#define FR_MATERIAL_NODE_GRADIENT_TEXTURE 0x11 
#define FR_MATERIAL_NODE_CHECKER_TEXTURE 0x12 
#define FR_MATERIAL_NODE_CONSTANT_TEXTURE 0x13 
#define FR_MATERIAL_NODE_INPUT_LOOKUP 0x14 
#define FR_MATERIAL_NODE_STANDARD 0x15 
#define FR_MATERIAL_NODE_BLEND_VALUE 0x16 
#define FR_MATERIAL_NODE_PASSTHROUGH 0x17 
#define FR_MATERIAL_NODE_ORENNAYAR 0x18 
#define FR_MATERIAL_NODE_FRESNEL_SCHLICK 0x19 
#define FR_MATERIAL_NODE_DIFFUSE_REFRACTION 0x1B 
#define FR_MATERIAL_NODE_BUMP_MAP 0x1C 
#define FR_MATERIAL_NODE_VOLUME 0x1D 
#define FR_MATERIAL_INPUT_COLOR 0x0 
#define FR_MATERIAL_INPUT_COLOR0 0x1 
#define FR_MATERIAL_INPUT_COLOR1 0x2 
#define FR_MATERIAL_INPUT_NORMAL 0x3 
#define FR_MATERIAL_INPUT_UV 0x4 
#define FR_MATERIAL_INPUT_DATA 0x5 
#define FR_MATERIAL_INPUT_ROUGHNESS 0x6 
#define FR_MATERIAL_INPUT_IOR 0x7 
#define FR_MATERIAL_INPUT_ROUGHNESS_X 0x8 
#define FR_MATERIAL_INPUT_ROUGHNESS_Y 0x9 
#define FR_MATERIAL_INPUT_ROTATION 0xA 
#define FR_MATERIAL_INPUT_WEIGHT 0xB 
#define FR_MATERIAL_INPUT_OP 0xC 
#define FR_MATERIAL_INPUT_INVEC 0xD 
#define FR_MATERIAL_INPUT_UV_SCALE 0xE 
#define FR_MATERIAL_INPUT_VALUE 0xF 
#define FR_MATERIAL_INPUT_REFLECTANCE 0x10 
#define FR_MATERIAL_INPUT_SCALE 0x11 
#define FR_MATERIAL_INPUT_SCATTERING 0x12 
#define FR_MATERIAL_INPUT_ABSORBTION 0x13 
#define FR_MATERIAL_INPUT_EMISSION 0x14 
#define FR_MATERIAL_INPUT_G 0x15 
#define FR_MATERIAL_INPUT_MULTISCATTER 0x16 
#define FR_MATERIAL_INPUT_COLOR2 0x17 
#define FR_MATERIAL_INPUT_COLOR3 0x18 
#define FR_MATERIAL_INPUT_MAX 0x19 
#define FR_MATERIAL_STANDARD_INPUT_DIFFUSE_COLOR 0x112 
#define FR_MATERIAL_STANDARD_INPUT_DIFFUSE_NORMAL 0x113 
#define FR_MATERIAL_STANDARD_INPUT_GLOSSY_COLOR 0x114 
#define FR_MATERIAL_STANDARD_INPUT_GLOSSY_NORMAL 0x115 
#define FR_MATERIAL_STANDARD_INPUT_GLOSSY_ROUGHNESS_X 0x116 
#define FR_MATERIAL_STANDARD_INPUT_CLEARCOAT_COLOR 0x117 
#define FR_MATERIAL_STANDARD_INPUT_CLEARCOAT_NORMAL 0x118 
#define FR_MATERIAL_STANDARD_INPUT_REFRACTION_COLOR 0x119 
#define FR_MATERIAL_STANDARD_INPUT_REFRACTION_NORMAL 0x11A 
#define FR_MATERIAL_STANDARD_INPUT_REFRACTION_IOR 0x11C 
#define FR_MATERIAL_STANDARD_INPUT_DIFFUSE_TO_REFRACTION_WEIGHT 0x11D 
#define FR_MATERIAL_STANDARD_INPUT_GLOSSY_TO_DIFFUSE_WEIGHT 0x11E 
#define FR_MATERIAL_STANDARD_INPUT_CLEARCOAT_TO_GLOSSY_WEIGHT 0x11F 
#define FR_MATERIAL_STANDARD_INPUT_TRANSPARENCY 0x120 
#define FR_MATERIAL_STANDARD_INPUT_TRANSPARENCY_COLOR 0x121 
#define FR_MATERIAL_STANDARD_INPUT_REFRACTION_ROUGHNESS 0x122 
#define FR_MATERIAL_STANDARD_INPUT_GLOSSY_ROUGHNESS_Y 0x123 
#define FR_MATERIAL_INPUT_RASTER_METALLIC 0x901 
#define FR_MATERIAL_INPUT_RASTER_ROUGHNESS 0x902 
#define FR_MATERIAL_INPUT_RASTER_SUBSURFACE 0x903 
#define FR_MATERIAL_INPUT_RASTER_ANISOTROPIC 0x904 
#define FR_MATERIAL_INPUT_RASTER_SPECULAR 0x905 
#define FR_MATERIAL_INPUT_RASTER_SPECULARTINT 0x906 
#define FR_MATERIAL_INPUT_RASTER_SHEEN 0x907 
#define FR_MATERIAL_INPUT_RASTER_SHEENTINT 0x908 
#define FR_MATERIAL_INPUT_RASTER_CLEARCOAT 0x90A 
#define FR_MATERIAL_INPUT_RASTER_CLEARCOATGLOSS 0x90B 
#define FR_MATERIAL_INPUT_RASTER_COLOR 0x90C 
#define FR_MATERIAL_INPUT_RASTER_NORMAL 0x90D 
#define FR_MATERIAL_NODE_OP_ADD 0x00 
#define FR_MATERIAL_NODE_OP_SUB 0x01 
#define FR_MATERIAL_NODE_OP_MUL 0x02 
#define FR_MATERIAL_NODE_OP_DIV 0x03 
#define FR_MATERIAL_NODE_OP_SIN 0x04 
#define FR_MATERIAL_NODE_OP_COS 0x05 
#define FR_MATERIAL_NODE_OP_TAN 0x06 
#define FR_MATERIAL_NODE_OP_SELECT_X 0x07 
#define FR_MATERIAL_NODE_OP_SELECT_Y 0x08 
#define FR_MATERIAL_NODE_OP_SELECT_Z 0x09 
#define FR_MATERIAL_NODE_OP_SELECT_W 0x0A 
#define FR_MATERIAL_NODE_OP_COMBINE 0x0B 
#define FR_MATERIAL_NODE_OP_DOT3 0x0C 
#define FR_MATERIAL_NODE_OP_DOT4 0x0D 
#define FR_MATERIAL_NODE_OP_CROSS3 0x0E 
#define FR_MATERIAL_NODE_OP_LENGTH3 0x0F 
#define FR_MATERIAL_NODE_OP_NORMALIZE3 0x10 
#define FR_MATERIAL_NODE_OP_POW 0x11 
#define FR_MATERIAL_NODE_OP_ACOS 0x12 
#define FR_MATERIAL_NODE_OP_ASIN 0x13 
#define FR_MATERIAL_NODE_OP_ATAN 0x14 
#define FR_MATERIAL_NODE_OP_AVERAGE_XYZ 0x15 
#define FR_MATERIAL_NODE_OP_AVERAGE 0x16 
#define FR_MATERIAL_NODE_OP_MIN 0x17 
#define FR_MATERIAL_NODE_OP_MAX 0x18 
#define FR_MATERIAL_NODE_OP_FLOOR 0x19 
#define FR_MATERIAL_NODE_OP_MOD 0x1A 
#define FR_MATERIAL_NODE_OP_ABS 0x1B 
#define FR_MATERIAL_NODE_OP_SHUFFLE_YZWX 0x1C 
#define FR_MATERIAL_NODE_OP_SHUFFLE_ZWXY 0x1D 
#define FR_MATERIAL_NODE_OP_SHUFFLE_WXYZ 0x1E 
#define FR_MATERIAL_NODE_OP_MAT_MUL 0x1F 
#define FR_MATERIAL_NODE_LOOKUP_UV 0x0 
#define FR_MATERIAL_NODE_LOOKUP_N 0x1 
#define FR_MATERIAL_NODE_LOOKUP_P 0x2 
#define FR_MATERIAL_NODE_LOOKUP_INVEC 0x3 
#define FR_MATERIAL_NODE_LOOKUP_OUTVEC 0x4 
#define FR_MATERIAL_NODE_LOOKUP_UV1 0x5 
#define FR_POST_EFFECT_TYPE 0x0 
#define FR_POST_EFFECT_PARAMETER_COUNT 0x1 
#define FR_POST_EFFECT_WHITE_BALANCE_COLOR_SPACE 0x1 
#define FR_POST_EFFECT_WHITE_BALANCE_COLOR_TEMPERATURE 0x2 
#define FR_POST_EFFECT_SIMPLE_TONEMAP_EXPOSURE 0x1 
#define FR_POST_EFFECT_SIMPLE_TONEMAP_CONTRAST 0x2 
#define FR_AOV_COLOR 0x0 
#define FR_AOV_OPACITY 0x1 
#define FR_AOV_WORLD_COORDINATE 0x2 
#define FR_AOV_UV 0x3 
#define FR_AOV_MATERIAL_IDX 0x4 
#define FR_AOV_GEOMETRIC_NORMAL 0x5 
#define FR_AOV_SHADING_NORMAL 0x6 
#define FR_AOV_DEPTH 0x7 
#define FR_AOV_OBJECT_ID 0x8 
#define FR_AOV_OBJECT_GROUP_ID 0x9 
#define FR_AOV_MAX 0xa 
#define FR_POST_EFFECT_TONE_MAP 0x0 
#define FR_POST_EFFECT_WHITE_BALANCE 0x1 
#define FR_POST_EFFECT_SIMPLE_TONEMAP 0x2 
#define FR_POST_EFFECT_NORMALIZATION 0x3 
#define FR_POST_EFFECT_GAMMA_CORRECTION 0x4 
#define FR_COLOR_SPACE_SRGB 0x1 
#define FR_COLOR_SPACE_ADOBE_RGB 0x2 
#define FR_COLOR_SPACE_REC2020 0x3 
#define FR_COLOR_SPACE_DCIP3 0x4 
#define FR_MATERIAL_NODE_INPUT_TYPE_FLOAT4 0x1 
#define FR_MATERIAL_NODE_INPUT_TYPE_UINT 0x2 
#define FR_MATERIAL_NODE_INPUT_TYPE_NODE 0x3 
#define FR_MATERIAL_NODE_INPUT_TYPE_IMAGE 0x4 
#define FR_RASTER_SHADOWS_FILTER_NONE 0x90E
#define FR_RASTER_SHADOWS_FILTER_PCF 0x90F
#define FR_RASTER_SHADOWS_FILTER_PCSS 0x910
#define FR_RASTER_SHADOWS_SAMPLING_BILINEAR 0x911
#define FR_RASTER_SHADOWS_SAMPLING_HAMMERSLEY 0x912
#define FR_RASTER_SHADOWS_SAMPLING_MULTIJITTERED 0x913
#define FR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_AND_CORNER 0x1 
#define FR_SUBDIV_BOUNDARY_INTERFOP_TYPE_EDGE_ONLY 0x2 
#define FR_IMAGE_WRAP_TYPE_REPEAT 0x1 
#define FR_IMAGE_WRAP_TYPE_MIRRORED_REPEAT 0x2 
#define FR_IMAGE_WRAP_TYPE_CLAMP_TO_EDGE 0x3 
#define FR_IMAGE_WRAP_TYPE_CLAMP_TO_BORDER 0x4 
#define FR_MAX_AA_SAMPLES 32 
#define FR_MAX_AA_GRID_SIZE 16 
#define FR_FALSE 0 
#define FR_TRUE 1 
typedef rpr_char fr_char;
typedef rpr_uchar fr_uchar;
typedef rpr_int fr_int;
typedef rpr_uint fr_uint;
typedef rpr_long fr_long;
typedef rpr_ulong fr_ulong;
typedef rpr_short fr_short;
typedef rpr_ushort fr_ushort;
typedef rpr_float fr_float;
typedef rpr_double fr_double;
typedef rpr_longlong fr_longlong;
typedef rpr_bool fr_bool;
typedef rpr_bitfield fr_bitfield;
typedef rpr_context fr_context;
typedef rpr_camera fr_camera;
typedef rpr_shape fr_shape;
typedef rpr_light fr_light;
typedef rpr_scene fr_scene;
typedef rpr_image fr_image;
typedef rpr_framebuffer fr_framebuffer;
typedef rpr_material_system fr_material_system;
typedef rpr_material_node fr_material_node;
typedef rpr_post_effect fr_post_effect;
typedef rpr_context_properties fr_context_properties;
typedef rpr_light_type fr_light_type;
typedef rpr_image_type fr_image_type;
typedef rpr_shape_type fr_shape_type;
typedef rpr_context_type fr_context_type;
typedef rpr_creation_flags fr_creation_flags;
typedef rpr_aa_filter fr_aa_filter;
typedef rpr_context_info fr_context_info;
typedef rpr_camera_info fr_camera_info;
typedef rpr_image_info fr_image_info;
typedef rpr_shape_info fr_shape_info;
typedef rpr_mesh_info fr_mesh_info;
typedef rpr_mesh_polygon_info fr_mesh_polygon_info;
typedef rpr_mesh_polygon_vertex_info fr_mesh_polygon_vertex_info;
typedef rpr_light_info fr_light_info;
typedef rpr_scene_info fr_scene_info;
typedef rpr_parameter_info fr_parameter_info;
typedef rpr_framebuffer_info fr_framebuffer_info;
typedef rpr_channel_order fr_channel_order;
typedef rpr_channel_type fr_channel_type;
typedef rpr_parameter_type fr_parameter_type;
typedef rpr_render_mode fr_render_mode;
typedef rpr_component_type fr_component_type;
typedef rpr_camera_mode fr_camera_mode;
typedef rpr_tonemapping_operator fr_tonemapping_operator;
typedef rpr_volume_type fr_volume_type;
typedef rpr_material_system_type fr_material_system_type;
typedef rpr_material_node_type fr_material_node_type;
typedef rpr_material_node_input fr_material_node_input;
typedef rpr_material_node_info fr_material_node_info;
typedef rpr_material_node_input_info fr_material_node_input_info;
typedef rpr_aov fr_aov;
typedef rpr_post_effect_type fr_post_effect_type;
typedef rpr_post_effect_info fr_post_effect_info;
typedef rpr_color_space fr_color_space;
typedef rpr_environment_override fr_environment_override;
typedef rpr_subdiv_boundary_interfop_type fr_subdiv_boundary_interfop_type;
typedef rpr_material_node_lookup_value fr_material_node_lookup_value;
typedef rpr_image_wrap_type fr_image_wrap_type;
typedef _rpr_image_desc _fr_image_desc;
typedef rpr_image_desc fr_image_desc;
typedef _rpr_framebuffer_desc _fr_framebuffer_desc;
typedef rpr_framebuffer_desc fr_framebuffer_desc;
typedef _rpr_render_statistics _fr_render_statistics;
typedef rpr_render_statistics fr_render_statistics;
typedef _rpr_image_format _fr_image_format;
typedef rpr_image_format fr_image_format;
typedef _rpr_ies_image_desc _fr_ies_image_desc;
typedef rpr_ies_image_desc fr_ies_image_desc;
typedef rpr_framebuffer_format fr_framebuffer_format;
extern RPR_API_ENTRY fr_int frRegisterPlugin(fr_char const * path);
extern RPR_API_ENTRY fr_int frCreateContext(fr_int api_version, fr_int * pluginIDs, size_t pluginCount, fr_creation_flags creation_flags, fr_context_properties const * props, fr_char const * cache_path, fr_context * out_context);
extern RPR_API_ENTRY fr_int frContextSetActivePlugin(fr_context context, fr_int pluginID);
extern RPR_API_ENTRY fr_int frContextGetInfo(fr_context context, fr_context_info context_info, size_t size, void * data, size_t * size_ret);
extern RPR_API_ENTRY fr_int frContextGetParameterInfo(fr_context context, int param_idx, fr_parameter_info parameter_info, size_t size, void * data, size_t * size_ret);
extern RPR_API_ENTRY fr_int frContextGetAOV(fr_context context, fr_aov aov, fr_framebuffer * out_fb);
extern RPR_API_ENTRY fr_int frContextSetAOV(fr_context context, fr_aov aov, fr_framebuffer frame_buffer);
extern RPR_API_ENTRY fr_int frContextSetScene(fr_context context, fr_scene scene);
extern RPR_API_ENTRY fr_int frContextGetScene(fr_context arg0, fr_scene * out_scene);
extern RPR_API_ENTRY fr_int frContextSetParameter1u(fr_context context, fr_char const * name, fr_uint x);
extern RPR_API_ENTRY fr_int frContextSetParameter1f(fr_context context, fr_char const * name, fr_float x);
extern RPR_API_ENTRY fr_int frContextSetParameter3f(fr_context context, fr_char const * name, fr_float x, fr_float y, fr_float z);
extern RPR_API_ENTRY fr_int frContextSetParameter4f(fr_context context, fr_char const * name, fr_float x, fr_float y, fr_float z, fr_float w);
extern RPR_API_ENTRY fr_int frContextSetParameterString(fr_context context, fr_char const * name, fr_char const * value);
extern RPR_API_ENTRY fr_int frContextRender(fr_context context);
extern RPR_API_ENTRY fr_int frContextRenderTile(fr_context context, fr_uint xmin, fr_uint xmax, fr_uint ymin, fr_uint ymax);
extern RPR_API_ENTRY fr_int frContextClearMemory(fr_context context);
extern RPR_API_ENTRY fr_int frContextCreateImage(fr_context context, fr_image_format const format, fr_image_desc const * image_desc, void const * data, fr_image * out_image);
extern RPR_API_ENTRY fr_int frContextCreateImageFromFile(fr_context context, fr_char const * path, fr_image * out_image);
extern RPR_API_ENTRY fr_int frContextCreateScene(fr_context context, fr_scene * out_scene);
extern RPR_API_ENTRY fr_int frContextCreateInstance(fr_context context, fr_shape shape, fr_shape * out_instance);
extern RPR_API_ENTRY fr_int frContextCreateMesh(fr_context context, fr_float const * vertices, size_t num_vertices, fr_int vertex_stride, fr_float const * normals, size_t num_normals, fr_int normal_stride, fr_float const * texcoords, size_t num_texcoords, fr_int texcoord_stride, fr_int const * vertex_indices, fr_int vidx_stride, fr_int const * normal_indices, fr_int nidx_stride, fr_int const * texcoord_indices, fr_int tidx_stride, fr_int const * num_face_vertices, size_t num_faces, fr_shape * out_mesh);
extern RPR_API_ENTRY fr_int frContextCreateMeshEx(fr_context context, fr_float const * vertices, size_t num_vertices, fr_int vertex_stride, fr_float const * normals, size_t num_normals, fr_int normal_stride, fr_int const * perVertexFlag, size_t num_perVertexFlags, fr_int perVertexFlag_stride, fr_int numberOfTexCoordLayers, fr_float const ** texcoords, size_t * num_texcoords, fr_int * texcoord_stride, fr_int const * vertex_indices, fr_int vidx_stride, fr_int const * normal_indices, fr_int nidx_stride, fr_int const ** texcoord_indices, fr_int * tidx_stride, fr_int const * num_face_vertices, size_t num_faces, fr_shape * out_mesh);
extern RPR_API_ENTRY fr_int frContextCreateCamera(fr_context context, fr_camera * out_camera);
extern RPR_API_ENTRY fr_int frContextCreateFrameBuffer(fr_context context, fr_framebuffer_format const format, fr_framebuffer_desc const * fb_desc, fr_framebuffer * out_fb);
extern RPR_API_ENTRY fr_int frCameraGetInfo(fr_camera camera, fr_camera_info camera_info, size_t size, void * data, size_t * size_ret);
extern RPR_API_ENTRY fr_int frCameraSetFocalLength(fr_camera camera, fr_float flength);
extern RPR_API_ENTRY fr_int frCameraSetFocusDistance(fr_camera camera, fr_float fdist);
extern RPR_API_ENTRY fr_int frCameraSetTransform(fr_camera camera, fr_bool transpose, fr_float * transform);
extern RPR_API_ENTRY fr_int frCameraSetSensorSize(fr_camera camera, fr_float width, fr_float height);
extern RPR_API_ENTRY fr_int frCameraLookAt(fr_camera camera, fr_float posx, fr_float posy, fr_float posz, fr_float atx, fr_float aty, fr_float atz, fr_float upx, fr_float upy, fr_float upz);
extern RPR_API_ENTRY fr_int frCameraSetFStop(fr_camera camera, fr_float fstop);
extern RPR_API_ENTRY fr_int frCameraSetApertureBlades(fr_camera camera, fr_uint num_blades);
extern RPR_API_ENTRY fr_int frCameraSetExposure(fr_camera camera, fr_float exposure);
extern RPR_API_ENTRY fr_int frCameraSetMode(fr_camera camera, fr_camera_mode mode);
extern RPR_API_ENTRY fr_int frCameraSetOrthoWidth(fr_camera camera, fr_float width);
extern RPR_API_ENTRY fr_int frCameraSetFocalTilt(fr_camera camera, fr_float tilt);
extern RPR_API_ENTRY fr_int frCameraSetIPD(fr_camera camera, fr_float ipd);
extern RPR_API_ENTRY fr_int frCameraSetLensShift(fr_camera camera, fr_float shiftx, fr_float shifty);
extern RPR_API_ENTRY fr_int frCameraSetOrthoHeight(fr_camera camera, fr_float height);
extern RPR_API_ENTRY fr_int frImageGetInfo(fr_image image, fr_image_info image_info, size_t size, void * data, size_t * size_ret);
extern RPR_API_ENTRY fr_int frImageSetWrap(fr_image image, fr_image_wrap_type type);
extern RPR_API_ENTRY fr_int frShapeSetTransform(fr_shape shape, fr_bool transpose, fr_float const * transform);
extern RPR_API_ENTRY fr_int frShapeSetSubdivisionFactor(fr_shape shape, fr_uint factor);
extern RPR_API_ENTRY fr_int frShapeSetSubdivisionCreaseWeight(fr_shape shape, fr_float factor);
extern RPR_API_ENTRY fr_int frShapeSetSubdivisionBoundaryInterop(fr_shape shape, fr_subdiv_boundary_interfop_type type);
extern RPR_API_ENTRY fr_int frShapeSetDisplacementScale(fr_shape shape, fr_float minscale, fr_float maxscale);
extern RPR_API_ENTRY fr_int frShapeSetObjectGroupID(fr_shape shape, fr_uint objectGroupID);
extern RPR_API_ENTRY fr_int frShapeSetDisplacementImage(fr_shape shape, fr_image image);
extern RPR_API_ENTRY fr_int frShapeSetMaterial(fr_shape shape, fr_material_node node);
extern RPR_API_ENTRY fr_int frShapeSetMaterialOverride(fr_shape shape, fr_material_node node);
extern RPR_API_ENTRY fr_int frShapeSetVolumeMaterial(fr_shape shape, fr_material_node node);
extern RPR_API_ENTRY fr_int frShapeSetLinearMotion(fr_shape shape, fr_float x, fr_float y, fr_float z);
extern RPR_API_ENTRY fr_int frShapeSetAngularMotion(fr_shape shape, fr_float x, fr_float y, fr_float z, fr_float w);
extern RPR_API_ENTRY fr_int frShapeSetVisibility(fr_shape shape, fr_bool visible);
extern RPR_API_ENTRY fr_int frShapeSetVisibilityPrimaryOnly(fr_shape shape, fr_bool visible);
extern RPR_API_ENTRY fr_int frShapeSetVisibilityInSpecular(fr_shape shape, fr_bool visible);
extern RPR_API_ENTRY fr_int frShapeSetShadowCatcher(fr_shape shape, fr_bool shadowCatcher);
extern RPR_API_ENTRY fr_int frShapeSetShadow(fr_shape shape, fr_bool casts_shadow);
extern RPR_API_ENTRY fr_int frLightSetTransform(fr_light light, fr_bool transpose, fr_float const * transform);
extern RPR_API_ENTRY fr_int frShapeGetInfo(fr_shape arg0, fr_shape_info arg1, size_t arg2, void * arg3, size_t * arg4);
extern RPR_API_ENTRY fr_int frMeshGetInfo(fr_shape mesh, fr_mesh_info mesh_info, size_t size, void * data, size_t * size_ret);
extern RPR_API_ENTRY fr_int frMeshPolygonGetInfo(fr_shape mesh, size_t polygon_index, fr_mesh_polygon_info polygon_info, size_t size, void * data, size_t * size_ret);
extern RPR_API_ENTRY fr_int frInstanceGetBaseShape(fr_shape shape, fr_shape * out_shape);
extern RPR_API_ENTRY fr_int frContextCreatePointLight(fr_context context, fr_light * out_light);
extern RPR_API_ENTRY fr_int frPointLightSetRadiantPower3f(fr_light light, fr_float r, fr_float g, fr_float b);
extern RPR_API_ENTRY fr_int frContextCreateSpotLight(fr_context context, fr_light * light);
extern RPR_API_ENTRY fr_int frSpotLightSetRadiantPower3f(fr_light light, fr_float r, fr_float g, fr_float b);
extern RPR_API_ENTRY fr_int frSpotLightSetConeShape(fr_light light, fr_float iangle, fr_float oangle);
extern RPR_API_ENTRY fr_int frContextCreateDirectionalLight(fr_context context, fr_light * out_light);
extern RPR_API_ENTRY fr_int frDirectionalLightSetRadiantPower3f(fr_light light, fr_float r, fr_float g, fr_float b);
extern RPR_API_ENTRY fr_int frDirectionalLightSetShadowSoftness(fr_light light, fr_float coeff);
extern RPR_API_ENTRY fr_int frContextCreateEnvironmentLight(fr_context context, fr_light * out_light);
extern RPR_API_ENTRY fr_int frEnvironmentLightSetImage(fr_light env_light, fr_image image);
extern RPR_API_ENTRY fr_int frEnvironmentLightSetIntensityScale(fr_light env_light, fr_float intensity_scale);
extern RPR_API_ENTRY fr_int frEnvironmentLightAttachPortal(fr_light env_light, fr_shape portal);
extern RPR_API_ENTRY fr_int frEnvironmentLightDetachPortal(fr_light env_light, fr_shape portal);
extern RPR_API_ENTRY fr_int frContextCreateSkyLight(fr_context context, fr_light * out_light);
extern RPR_API_ENTRY fr_int frSkyLightSetTurbidity(fr_light skylight, fr_float turbidity);
extern RPR_API_ENTRY fr_int frSkyLightSetAlbedo(fr_light skylight, fr_float albedo);
extern RPR_API_ENTRY fr_int frSkyLightSetScale(fr_light skylight, fr_float scale);
extern RPR_API_ENTRY fr_int frSkyLightAttachPortal(fr_light skylight, fr_shape portal);
extern RPR_API_ENTRY fr_int frSkyLightDetachPortal(fr_light skylight, fr_shape portal);
extern RPR_API_ENTRY fr_int frContextCreateIESLight(fr_context context, fr_light * light);
extern RPR_API_ENTRY fr_int frIESLightSetRadiantPower3f(fr_light light, fr_float r, fr_float g, fr_float b);
extern RPR_API_ENTRY fr_int frIESLightSetImageFromFile(fr_light env_light, fr_char const * imagePath, fr_int nx, fr_int ny);
extern RPR_API_ENTRY fr_int frIESLightSetImageFromIESdata(fr_light env_light, fr_char const * iesData, fr_int nx, fr_int ny);
extern RPR_API_ENTRY fr_int frLightGetInfo(fr_light light, fr_light_info info, size_t size, void * data, size_t * size_ret);
extern RPR_API_ENTRY fr_int frSceneClear(fr_scene scene);
extern RPR_API_ENTRY fr_int frSceneAttachShape(fr_scene scene, fr_shape shape);
extern RPR_API_ENTRY fr_int frSceneDetachShape(fr_scene scene, fr_shape shape);
extern RPR_API_ENTRY fr_int frSceneAttachLight(fr_scene scene, fr_light light);
extern RPR_API_ENTRY fr_int frSceneDetachLight(fr_scene scene, fr_light light);
extern RPR_API_ENTRY fr_int frSceneGetInfo(fr_scene scene, fr_scene_info info, size_t size, void * data, size_t * size_ret);
extern RPR_API_ENTRY fr_int frSceneGetEnvironmentOverride(fr_scene scene, fr_environment_override overrride, fr_light * out_light);
extern RPR_API_ENTRY fr_int frSceneSetEnvironmentOverride(fr_scene scene, fr_environment_override overrride, fr_light light);
extern RPR_API_ENTRY fr_int frSceneSetBackgroundImage(fr_scene scene, fr_image image);
extern RPR_API_ENTRY fr_int frSceneGetBackgroundImage(fr_scene scene, fr_image * out_image);
extern RPR_API_ENTRY fr_int frSceneSetCamera(fr_scene scene, fr_camera camera);
extern RPR_API_ENTRY fr_int frSceneGetCamera(fr_scene scene, fr_camera * out_camera);
extern RPR_API_ENTRY fr_int frFrameBufferGetInfo(fr_framebuffer framebuffer, fr_framebuffer_info info, size_t size, void * data, size_t * size_ret);
extern RPR_API_ENTRY fr_int frFrameBufferClear(fr_framebuffer frame_buffer);
extern RPR_API_ENTRY fr_int frFrameBufferSaveToFile(fr_framebuffer frame_buffer, fr_char const * file_path);
extern RPR_API_ENTRY fr_int frContextResolveFrameBuffer(fr_context context, fr_framebuffer src_frame_buffer, fr_framebuffer dst_frame_buffer, fr_bool normalizeOnly = false);
extern RPR_API_ENTRY fr_int frContextCreateMaterialSystem(fr_context in_context, fr_material_system_type type, fr_material_system * out_matsys);
extern RPR_API_ENTRY fr_int frMaterialSystemCreateNode(fr_material_system in_matsys, fr_material_node_type in_type, fr_material_node * out_node);
extern RPR_API_ENTRY fr_int frMaterialNodeSetInputN(fr_material_node in_node, fr_char const * in_input, fr_material_node in_input_node);
extern RPR_API_ENTRY fr_int frMaterialNodeSetInputF(fr_material_node in_node, fr_char const * in_input, fr_float in_value_x, fr_float in_value_y, fr_float in_value_z, fr_float in_value_w);
extern RPR_API_ENTRY fr_int frMaterialNodeSetInputU(fr_material_node in_node, fr_char const * in_input, fr_uint in_value);
extern RPR_API_ENTRY fr_int frMaterialNodeSetInputImageData(fr_material_node in_node, fr_char const * in_input, fr_image image);
extern RPR_API_ENTRY fr_int frMaterialNodeGetInfo(fr_material_node in_node, fr_material_node_info in_info, size_t in_size, void * in_data, size_t * out_size);
extern RPR_API_ENTRY fr_int frMaterialNodeGetInputInfo(fr_material_node in_node, fr_int in_input_idx, fr_material_node_input_info in_info, size_t in_size, void * in_data, size_t * out_size);
extern RPR_API_ENTRY fr_int frObjectDelete(void * obj);
extern RPR_API_ENTRY fr_int frObjectSetName(void * node, fr_char const * name);
extern RPR_API_ENTRY fr_int frContextCreatePostEffect(fr_context context, fr_post_effect_type type, fr_post_effect * out_effect);
extern RPR_API_ENTRY fr_int frContextAttachPostEffect(fr_context context, fr_post_effect effect);
extern RPR_API_ENTRY fr_int frContextDetachPostEffect(fr_context context, fr_post_effect effect);
extern RPR_API_ENTRY fr_int frPostEffectSetParameter1u(fr_post_effect effect, fr_char const * name, fr_uint x);
extern RPR_API_ENTRY fr_int frPostEffectSetParameter1f(fr_post_effect effect, fr_char const * name, fr_float x);
extern RPR_API_ENTRY fr_int frPostEffectSetParameter3f(fr_post_effect effect, fr_char const * name, fr_float x, fr_float y, fr_float z);
extern RPR_API_ENTRY fr_int frPostEffectSetParameter4f(fr_post_effect effect, fr_char const * name, fr_float x, fr_float y, fr_float z, fr_float w);

#ifdef __cplusplus
}
#endif

#endif  /*__RADEONPRORENDER_H  */
