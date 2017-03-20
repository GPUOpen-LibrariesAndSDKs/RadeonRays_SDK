#include "../Rpr/RadeonProRender.h"
#include "../RadeonRays/include/math/matrix.h"
#include "../RadeonRays/include/math/mathutils.h"

#include <cassert>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>
#include "ObjViewer.h"

using namespace RadeonRays;

namespace
{
    //control how many frames to render in tests for resulting image
    int kRenderIterations = 100;
    //default structure for vertex data
    struct vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    rpr_shape CreateSphere(rpr_context context, std::uint32_t lat, std::uint32_t lon, float r, RadeonRays::float3 const& c)
    {
        auto num_verts = (lat - 2) * lon + 2;
        auto num_tris = (lat - 2) * (lon - 1) * 2;

        std::vector<RadeonRays::float3> vertices(num_verts);
        std::vector<RadeonRays::float3> normals(num_verts);
        std::vector<RadeonRays::float2> uvs(num_verts);
        std::vector<std::uint32_t> indices(num_tris * 3);


        auto t = 0U;
        for (auto j = 1U; j < lat - 1; j++)
            for (auto i = 0U; i < lon; i++)
            {
                float theta = float(j) / (lat - 1) * (float)M_PI;
                float phi = float(i) / (lon - 1) * (float)M_PI * 2;
                vertices[t].x = r * sinf(theta) * cosf(phi) + c.x;
                vertices[t].y = r * cosf(theta) + c.y;
                vertices[t].z = r * -sinf(theta) * sinf(phi) + c.z;
                normals[t].x = sinf(theta) * cosf(phi);
                normals[t].y = cosf(theta);
                normals[t].z = -sinf(theta) * sinf(phi);
                uvs[t].x = phi / (2 * (float)M_PI);
                uvs[t].y = theta / ((float)M_PI);
                ++t;
            }

        vertices[t].x = c.x; vertices[t].y = c.y + r; vertices[t].z = c.z;
        normals[t].x = 0; normals[t].y = 1; normals[t].z = 0;
        uvs[t].x = 0; uvs[t].y = 0;
        ++t;
        vertices[t].x = c.x; vertices[t].y = c.y - r; vertices[t].z = c.z;
        normals[t].x = 0; normals[t].y = -1; normals[t].z = 0;
        uvs[t].x = 1; uvs[t].y = 1;
        ++t;

        t = 0U;
        for (auto j = 0U; j < lat - 3; j++)
            for (auto i = 0U; i < lon - 1; i++)
            {
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
                indices[t++] = j * lon + i + 1;
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
            }

        for (auto i = 0U; i < lon - 1; i++)
        {
            indices[t++] = (lat - 2) * lon;
            indices[t++] = i;
            indices[t++] = i + 1;
            indices[t++] = (lat - 2) * lon + 1;
            indices[t++] = (lat - 3) * lon + i + 1;
            indices[t++] = (lat - 3) * lon + i;
        }

        std::vector<int> faces(indices.size() / 3, 3);
        rpr_int status = RPR_SUCCESS;
        rpr_shape mesh = NULL; status = rprContextCreateMesh(context,
            (rpr_float const*)vertices.data(), vertices.size(), sizeof(float3),
            (rpr_float const*)normals.data(), normals.size(), sizeof(float3),
            (rpr_float const*)uvs.data(), uvs.size(), sizeof(float2),
            (rpr_int const*)indices.data(), sizeof(rpr_int),
            (rpr_int const*)indices.data(), sizeof(rpr_int),
            (rpr_int const*)indices.data(), sizeof(rpr_int),
            faces.data(), faces.size(), &mesh);
        assert(status == RPR_SUCCESS);

        return mesh;
    }
}

#define FR_MACRO_CLEAN_SHAPE_RELEASE(mesh1__, scene1__)\
	status = rprShapeSetMaterial(mesh1__, NULL);\
	assert(status == RPR_SUCCESS);\
	status = rprSceneDetachShape(scene1__, mesh1__);\
	assert(status == RPR_SUCCESS);\
	status = rprObjectDelete(mesh1__); mesh1__ = NULL;\
	assert(status == RPR_SUCCESS);

#define FR_MACRO_SAFE_FRDELETE(a)    { status = rprObjectDelete(a); a = NULL;   assert(status == RPR_SUCCESS); }


void MeshCreationTest()
{
    struct vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    vertex quad[] =
    {
        { -1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 0.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 0.0f, 1.0f }
    };

    rpr_int indices[] =
    {
        3,1,0,
        2,1,3
    };

    rpr_int num_face_vertices[] =
    {
        3, 3
    };

    rpr_int status = RPR_SUCCESS;
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);

    rpr_shape quad_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&quad[0], 24, sizeof(vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 3), 24, sizeof(vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 6), 24, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &quad_mesh);
    assert(status == RPR_SUCCESS);

    status = rprObjectDelete(quad_mesh); quad_mesh = NULL;
    assert(status == RPR_SUCCESS);
}

void SimpleRenderTest()
{
    rpr_int status = RPR_SUCCESS;
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);
    rpr_material_system matsys = NULL;
    status = rprContextCreateMaterialSystem(context, 0, &matsys);
    assert(status == RPR_SUCCESS);

    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);

    vertex cube[] =
    {
        { -1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 0.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, -1.0f , 0.f, -1.f, 0.f, 0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f , 0.f, -1.f, 0.f, 1.0f, 0.0f },
        { 1.0f, -1.0f, 1.0f , 0.f, -1.f, 0.f, 1.0f, 1.0f },
        { -1.0f, -1.0f, 1.0f , 0.f, -1.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, 1.0f , -1.f, 0.f, 0.f, 0.0f, 0.0f },
        { -1.0f, -1.0f, -1.0f , -1.f, 0.f, 0.f, 1.0f, 0.0f },
        { -1.0f, 1.0f, -1.0f , -1.f, 0.f, 0.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , -1.f, 0.f, 0.f, 0.0f, 1.0f },

        { 1.0f, -1.0f, 1.0f ,  1.f, 0.f, 0.f, 0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f ,  1.f, 0.f, 0.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f ,  1.f, 0.f, 0.f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f ,  1.f, 0.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, -1.0f ,  0.f, 0.f, -1.f ,0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f ,  0.f, 0.f, -1.f ,1.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f ,  0.f, 0.f, -1.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, -1.0f ,  0.f, 0.f, -1.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, 1.0f , 0.f, 0.f, 1.f,0.0f, 0.0f },
        { 1.0f, -1.0f, 1.0f , 0.f, 0.f,  1.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f , 0.f, 0.f, 1.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , 0.f, 0.f, 1.f,0.0f, 1.0f },
    };

    vertex plane[] =
    {
        { -5.f, 0.f, -5.f, 0.f, 1.f, 0.f, 0.f, 0.f },
        { -5.f, 0.f,  5.f, 0.f, 1.f, 0.f, 0.f, 1.f },
        { 5.f, 0.f,  5.f, 0.f, 1.f, 0.f, 1.f, 1.f },
        { 5.f, 0.f, -5.f, 0.f, 1.f, 0.f, 1.f, 0.f },
    };

    rpr_int indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };


    rpr_int num_face_vertices[] =
    {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
    };

    //material
    rpr_material_node diffuse = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuse);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(diffuse, "color", 0.7f, 0.7f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);

    //sphere
    rpr_shape mesh = CreateSphere(context, 64, 32, 2.f, float3());
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh, diffuse);
    assert(status == RPR_SUCCESS);
    matrix m = translation(float3(0, 1, 0));
    status = rprShapeSetTransform(mesh, true, &m.m00);
    assert(status == RPR_SUCCESS);
    
    //plane mesh
    rpr_shape plane_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&plane[0], 4, sizeof(vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 3), 4, sizeof(vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 6), 4, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &plane_mesh);
    status = rprSceneAttachShape(scene, plane_mesh);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(plane_mesh, diffuse);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 3, 10, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocalLength(camera, 23.f);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFStop(camera, 5.4f);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreateSpotLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0, 16, 0)) * rotation_x(M_PI_2);
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprSpotLightSetConeShape(light, M_PI_4, M_PI * 2.f / 3.f);
    assert(status == RPR_SUCCESS);
    status = rprSpotLightSetRadiantPower3f(light, 350, 350, 350);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    //setup out
    rpr_framebuffer_desc desc;
    desc.fb_width = 800;
    desc.fb_height = 600;

    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);  assert(status == RPR_SUCCESS);

    //render
    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    status = rprFrameBufferSaveToFile(frame_buffer, "SimpleRenderTest_f1.jpg");
    assert(status == RPR_SUCCESS);

    //change light
    status = rprSpotLightSetConeShape(light, M_PI_4 * 0.5f, M_PI_4 * 0.5f);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);  assert(status == RPR_SUCCESS);
    assert(status == RPR_SUCCESS);
    
    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    status = rprFrameBufferSaveToFile(frame_buffer, "SimpleRenderTest_f2.jpg");
    assert(status == RPR_SUCCESS);
    rpr_render_statistics rs;
    status = rprContextGetInfo(context, RPR_CONTEXT_RENDER_STATISTICS, sizeof(rpr_render_statistics), &rs, NULL);
    assert(status == RPR_SUCCESS);

    //cleanup
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    rprObjectDelete(diffuse);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(matsys); matsys = NULL;
    assert(status == RPR_SUCCESS);
}

void ComplexRenderTest()
{
    rpr_int status = RPR_SUCCESS;

    //context, scene and mat. system
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);
    rpr_material_system matsys = NULL;
    status = rprContextCreateMaterialSystem(context, 0, &matsys);
    assert(status == RPR_SUCCESS);
    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);

    struct vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    vertex cube[] =
    {
        { -1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 0.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, -1.0f , 0.f, -1.f, 0.f, 0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f , 0.f, -1.f, 0.f, 1.0f, 0.0f },
        { 1.0f, -1.0f, 1.0f , 0.f, -1.f, 0.f, 1.0f, 1.0f },
        { -1.0f, -1.0f, 1.0f , 0.f, -1.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, 1.0f , -1.f, 0.f, 0.f, 0.0f, 0.0f },
        { -1.0f, -1.0f, -1.0f , -1.f, 0.f, 0.f, 1.0f, 0.0f },
        { -1.0f, 1.0f, -1.0f , -1.f, 0.f, 0.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , -1.f, 0.f, 0.f, 0.0f, 1.0f },

        { 1.0f, -1.0f, 1.0f ,  1.f, 0.f, 0.f, 0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f ,  1.f, 0.f, 0.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f ,  1.f, 0.f, 0.f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f ,  1.f, 0.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, -1.0f ,  0.f, 0.f, -1.f ,0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f ,  0.f, 0.f, -1.f ,1.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f ,  0.f, 0.f, -1.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, -1.0f ,  0.f, 0.f, -1.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, 1.0f , 0.f, 0.f, 1.f,0.0f, 0.0f },
        { 1.0f, -1.0f, 1.0f , 0.f, 0.f,  1.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f , 0.f, 0.f, 1.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , 0.f, 0.f, 1.f,0.0f, 1.0f },
    };

    vertex plane[] =
    {
        { -15.f, 0.f, -15.f, 0.f, 1.f, 0.f, 0.f, 0.f },
        { -15.f, 0.f,  15.f, 0.f, 1.f, 0.f, 0.f, 1.f },
        { 15.f, 0.f,  15.f, 0.f, 1.f, 0.f, 1.f, 1.f },
        { 15.f, 0.f, -15.f, 0.f, 1.f, 0.f, 1.f, 0.f },
    };

    rpr_int indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

    rpr_int num_face_vertices[] =
    {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
    };

    rpr_shape mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&cube[0], 24, sizeof(vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 3), 24, sizeof(vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 6), 24, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 12, &mesh);

    assert(status == RPR_SUCCESS);

    rpr_shape plane_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&plane[0], 4, sizeof(vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 3), 4, sizeof(vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 6), 4, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &plane_mesh);

    rpr_shape mesh1 = NULL; status = rprContextCreateInstance(context, mesh, &mesh1);
    assert(status == RPR_SUCCESS);

    //translate cubes
    matrix m = translation(float3(-2, 1, 0));
    status = rprShapeSetTransform(mesh1, true, &m.m00);
    assert(status == RPR_SUCCESS);
    matrix m1 = translation(float3(2, 1, 0)) * rotation_y(0.5);
    status = rprShapeSetTransform(mesh, true, &m1.m00);
    assert(status == RPR_SUCCESS);

    //attach shapes
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh1);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, plane_mesh);
    assert(status == RPR_SUCCESS);
    
    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 3, 10, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetSensorSize(camera, 22.5f, 15.f);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocalLength(camera, 35.f);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFStop(camera, 6.4f);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    //materials
    rpr_material_node diffuse = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuse);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(diffuse, "color", 0.9f, 0.9f, 0.f, 1.0f);
    assert(status == RPR_SUCCESS);
    rpr_image img = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/scratched.png", &img);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(diffuse, "color", img);
    assert(status == RPR_SUCCESS);
    rpr_material_node materialNodeTexture = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &materialNodeTexture);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(materialNodeTexture, "data", img);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(diffuse, "color", materialNodeTexture);
    assert(status == RPR_SUCCESS);

    status = rprShapeSetMaterial(mesh, diffuse);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh1, diffuse);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(plane_mesh, diffuse);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0, 6, 0)) * rotation_z(3.14f);
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprPointLightSetRadiantPower3f(light, 100, 100, 100);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    //result buffer
    rpr_framebuffer_desc desc;
    desc.fb_width = 800;
    desc.fb_height = 600;
    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    
    //render
    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    status = rprFrameBufferSaveToFile(frame_buffer, "ComplexRender.jpg");
    assert(status == RPR_SUCCESS);

    //cleanup
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh1, scene);
    FR_MACRO_CLEAN_SHAPE_RELEASE(plane_mesh, scene);
    FR_MACRO_SAFE_FRDELETE(img);
    FR_MACRO_SAFE_FRDELETE(materialNodeTexture);
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    rprObjectDelete(diffuse);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(matsys); matsys = NULL;
    assert(status == RPR_SUCCESS);
}

void EnvLightClearTest()
{
    rpr_int status = RPR_SUCCESS;

    //create context, scene and mat system
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);
    rpr_material_system matsys = NULL;
    status = rprContextCreateMaterialSystem(context, 0, &matsys);
    assert(status == RPR_SUCCESS);
    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);
    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    struct vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    vertex cube[] =
    {
        { -1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 0.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, -1.0f , 0.f, -1.f, 0.f, 0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f , 0.f, -1.f, 0.f, 1.0f, 0.0f },
        { 1.0f, -1.0f, 1.0f , 0.f, -1.f, 0.f, 1.0f, 1.0f },
        { -1.0f, -1.0f, 1.0f , 0.f, -1.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, 1.0f , -1.f, 0.f, 0.f, 0.0f, 0.0f },
        { -1.0f, -1.0f, -1.0f , -1.f, 0.f, 0.f, 1.0f, 0.0f },
        { -1.0f, 1.0f, -1.0f , -1.f, 0.f, 0.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , -1.f, 0.f, 0.f, 0.0f, 1.0f },

        { 1.0f, -1.0f, 1.0f ,  1.f, 0.f, 0.f, 0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f ,  1.f, 0.f, 0.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f ,  1.f, 0.f, 0.f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f ,  1.f, 0.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, -1.0f ,  0.f, 0.f, -1.f ,0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f ,  0.f, 0.f, -1.f ,1.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f ,  0.f, 0.f, -1.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, -1.0f ,  0.f, 0.f, -1.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, 1.0f , 0.f, 0.f, 1.f,0.0f, 0.0f },
        { 1.0f, -1.0f, 1.0f , 0.f, 0.f,  1.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f , 0.f, 0.f, 1.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , 0.f, 0.f, 1.f,0.0f, 1.0f },
    };

    vertex plane[] =
    {
        { -500.f, 0.f, -500.f, 0.f, 1.f, 0.f, 0.f, 0.f },
        { -500.f, 0.f,  500.f, 0.f, 1.f, 0.f, 0.f, 1.f },
        { 500.f, 0.f,  500.f, 0.f, 1.f, 0.f, 1.f, 1.f },
        { 500.f, 0.f, -500.f, 0.f, 1.f, 0.f, 1.f, 0.f },
    };

    rpr_int indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };


    rpr_int num_face_vertices[] =
    {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
    };

    //materials
    rpr_material_node diffuse = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuse);
    assert(status == RPR_SUCCESS);
    rpr_material_node specularSphere = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_REFLECTION, &specularSphere);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(diffuse, "color", 0.7f, 0.7f, 0.7f, 1.0f);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(specularSphere, "color", 1.0f, 1.0f, 1.0f, 1.0f);
    assert(status == RPR_SUCCESS);

    //sphere mesh
    rpr_shape mesh = CreateSphere(context, 64, 32, 2.f, float3());
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh, diffuse);
    assert(status == RPR_SUCCESS);
    matrix m = translation(float3(0, 1, 0)) * scale(float3(0.2, 0.2, 0.2));
    status = rprShapeSetTransform(mesh, true, &m.m00);
    assert(status == RPR_SUCCESS);

    //plane mesh
    rpr_shape plane_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&plane[0], 4, sizeof(vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 3), 4, sizeof(vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 6), 4, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &plane_mesh);
    status = rprSceneAttachShape(scene, plane_mesh);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(plane_mesh, diffuse);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 1, 10, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocalLength(camera, 23.f);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFStop(camera, 5.4f);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    //environment light
    rpr_light light = NULL; status = rprContextCreateEnvironmentLight(context, &light);
    assert(status == RPR_SUCCESS);
    rpr_image imageInput = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/Studio015.hdr", &imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light, imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetIntensityScale(light, 5.f);
    assert(status == RPR_SUCCESS);
    matrix lightm = rotation_y(M_PI);
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    //output
    rpr_framebuffer_desc desc;
    desc.fb_width = 800;
    desc.fb_height = 600;
    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    
    //render
    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "EnvLightClear_Stage1.png");

    //check clear frame buffer
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    rprFrameBufferSaveToFile(frame_buffer, "EnvLightClear_Stage2.png");

    //clear scene and reattach resources
    status = rprSceneClear(scene);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, plane_mesh);
    assert(status == RPR_SUCCESS);
    lightm = rotation_y(-M_PI_2);
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);

    //render
    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "EnvLightClear_Stage3.png");

    //cleanup
    status = rprObjectDelete(imageInput); imageInput = NULL;
    assert(status == RPR_SUCCESS);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    FR_MACRO_CLEAN_SHAPE_RELEASE(plane_mesh, scene);
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    rprObjectDelete(diffuse);
    rprObjectDelete(specularSphere);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(matsys); matsys = NULL;
    assert(status == RPR_SUCCESS);
}

void MemoryStatistics()
{
    rpr_render_statistics rs;
    rs.gpumem_usage = 0;
    rs.gpumem_total = 0;
    rs.gpumem_max_allocation = 0;

    //create context and check there is no used resources
    rpr_int status = RPR_SUCCESS;
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);
    status = rprContextGetInfo(context, RPR_CONTEXT_RENDER_STATISTICS, sizeof(rpr_render_statistics), &rs, NULL);
    assert(status == RPR_SUCCESS);

    assert(rs.gpumem_usage == 0);
    assert(rs.gpumem_total == 0);
    assert(rs.gpumem_max_allocation == 0);
}

void DefaultMaterialTest()
{
    rpr_int status = RPR_SUCCESS;
    //create context and scene
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);
    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);

    struct vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    vertex quad[] =
    {
        { -5.0f, -1.0f, -5.0f, 0.f, 1.f, 0.f, 0.f, 0.f },
        { 5.0f, -1.0f, -5.0f, 0.f, 1.f,  0.f, 1.f, 0.f },
        { 5.0f, -1.0f, 5.0f, 0.f, 1.f, 0.f,  1.f, 1.f },
        { -5.0f, -1.0f, 5.0f, 0.f, 1.f, 0.f, 0.f, 1.f },
    };


    rpr_int indices[] =
    {
        3,1,0,
        2,1,3,
    };

    rpr_int num_face_vertices[] =
    {
        3, 3
    };

    //plane mesh
    rpr_shape mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&quad[0], 4, sizeof(vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 3), 4, sizeof(vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 6), 4, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &mesh);
    assert(status == RPR_SUCCESS);
    matrix r = rotation_x(M_PI_2);
    status = rprShapeSetTransform(mesh, true, &r.m00);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 0, 10, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocalLength(camera, 20.f);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);
    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0, 0, 6));
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprPointLightSetRadiantPower3f(light, 200, 200, 200);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    //setup output
    rpr_framebuffer_desc desc;
    desc.fb_width = 800;
    desc.fb_height = 600;
    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);

    //render
    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "DefaultMaterialTest.png");

    //cleanup
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);
}

void NullShaderTest()
{
    rpr_int status = RPR_SUCCESS;

    //create context, material system and scene
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    rpr_material_system matsys = NULL;
    status = rprContextCreateMaterialSystem(context, 0, &matsys);
    assert(status == RPR_SUCCESS);
    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);
    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    struct vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    vertex quad[] =
    {
        { -5.0f, -1.0f, -5.0f, 0.f, 1.f, 0.f, 0.f, 0.f },
        { 5.0f, -1.0f, -5.0f, 0.f, 1.f,  0.f, 1.f, 0.f },
        { 5.0f, -1.0f, 5.0f, 0.f, 1.f, 0.f,  1.f, 1.f },
        { -5.0f, -1.0f, 5.0f, 0.f, 1.f, 0.f, 0.f, 1.f },
    };


    rpr_int indices[] =
    {
        3,1,0,
        2,1,3,
    };

    rpr_int num_face_vertices[] =
    {
        3, 3
    };

    //plane mesh
    rpr_shape mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&quad[0], 4, sizeof(vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 3), 4, sizeof(vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 6), 4, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &mesh);
    assert(status == RPR_SUCCESS);
    matrix r = rotation_x(M_PI_2);
    status = rprShapeSetTransform(mesh, true, &r.m00);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 0, 10, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocalLength(camera, 20.f);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    //materials
    rpr_material_node diffuse = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuse);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(diffuse, "color", 1, 0, 0, 1);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh, diffuse);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0, 0, 6));
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprPointLightSetRadiantPower3f(light, 200, 200, 200);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    //setup output
    rpr_framebuffer_desc desc;
    desc.fb_width = 800;
    desc.fb_height = 600;
    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    
    //render
    int render_iterations = 100;
    for (int i = 0; i < render_iterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "NullShader_Stage1.png");

    //try with NULL material
    status = rprShapeSetMaterial(mesh, NULL);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    for (int i = 0; i < render_iterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "NullShader_Stage2.png");


    //cleanup
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    rprObjectDelete(diffuse);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(matsys); matsys = NULL;
    assert(status == RPR_SUCCESS);
}

void TiledRender()
{
    rpr_int status = RPR_SUCCESS;

    //prepare
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);
    rpr_material_system matsys = NULL;
    status = rprContextCreateMaterialSystem(context, 0, &matsys);
    assert(status == RPR_SUCCESS);
    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);
    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    struct vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    vertex cube[] =
    {
        { -1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 0.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f, 0.f, 1.f, 0.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , 0.f, 1.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, -1.0f , 0.f, -1.f, 0.f, 0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f , 0.f, -1.f, 0.f, 1.0f, 0.0f },
        { 1.0f, -1.0f, 1.0f , 0.f, -1.f, 0.f, 1.0f, 1.0f },
        { -1.0f, -1.0f, 1.0f , 0.f, -1.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, 1.0f , -1.f, 0.f, 0.f, 0.0f, 0.0f },
        { -1.0f, -1.0f, -1.0f , -1.f, 0.f, 0.f, 1.0f, 0.0f },
        { -1.0f, 1.0f, -1.0f , -1.f, 0.f, 0.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , -1.f, 0.f, 0.f, 0.0f, 1.0f },

        { 1.0f, -1.0f, 1.0f ,  1.f, 0.f, 0.f, 0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f ,  1.f, 0.f, 0.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f ,  1.f, 0.f, 0.f, 1.0f, 1.0f },
        { 1.0f, 1.0f, 1.0f ,  1.f, 0.f, 0.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, -1.0f ,  0.f, 0.f, -1.f ,0.0f, 0.0f },
        { 1.0f, -1.0f, -1.0f ,  0.f, 0.f, -1.f ,1.0f, 0.0f },
        { 1.0f, 1.0f, -1.0f ,  0.f, 0.f, -1.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, -1.0f ,  0.f, 0.f, -1.f, 0.0f, 1.0f },

        { -1.0f, -1.0f, 1.0f , 0.f, 0.f, 1.f,0.0f, 0.0f },
        { 1.0f, -1.0f, 1.0f , 0.f, 0.f,  1.f, 1.0f, 0.0f },
        { 1.0f, 1.0f, 1.0f , 0.f, 0.f, 1.f, 1.0f, 1.0f },
        { -1.0f, 1.0f, 1.0f , 0.f, 0.f, 1.f,0.0f, 1.0f },
    };

    vertex plane[] =
    {
        { -500.f, 0.f, -500.f, 0.f, 1.f, 0.f, 0.f, 0.f },
        { -500.f, 0.f,  500.f, 0.f, 1.f, 0.f, 0.f, 1.f },
        { 500.f, 0.f,  500.f, 0.f, 1.f, 0.f, 1.f, 1.f },
        { 500.f, 0.f, -500.f, 0.f, 1.f, 0.f, 1.f, 0.f },
    };

    rpr_int indices[] =
    {
        3,1,0,
        2,1,3,

        6,4,5,
        7,4,6,

        11,9,8,
        10,9,11,

        14,12,13,
        15,12,14,

        19,17,16,
        18,17,19,

        22,20,21,
        23,20,22
    };

    rpr_int num_face_vertices[] =
    {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
    };

    //meshes
    rpr_shape mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&cube[0], 24, sizeof(vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 3), 24, sizeof(vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 6), 24, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 12, &mesh);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);
    matrix m = rotation_x(M_PI_4) * rotation_y(M_PI);
    status = rprShapeSetTransform(mesh, true, &m.m00);
    assert(status == RPR_SUCCESS);

    rpr_shape plane_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&plane[0], 4, sizeof(vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 3), 4, sizeof(vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 6), 4, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &plane_mesh);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, plane_mesh);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 3, 10, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocalLength(camera, 50.f);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFStop(camera, 5.4f);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    //material
    rpr_material_node diffuse = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuse);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(diffuse, "color", 0.7f, 0.7f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh, diffuse);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(plane_mesh, diffuse);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0, 6, 0));
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprPointLightSetRadiantPower3f(light, 60, 60, 60);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    //output
    rpr_framebuffer_desc desc;
    desc.fb_width = 800;
    desc.fb_height = 600;
    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);  assert(status == RPR_SUCCESS);
    assert(status == RPR_SUCCESS);

    //render
    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRenderTile(context, 10, 750, 10, 550);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "Tiled_Render.png");

    //clear
    FR_MACRO_CLEAN_SHAPE_RELEASE(plane_mesh, scene);
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    rprObjectDelete(diffuse);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);

    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(matsys); matsys = NULL;
    assert(status == RPR_SUCCESS);
}

void test_feature_cameraDOF()
{
    rpr_int status = RPR_SUCCESS;
    //create context, material system and scene
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    rpr_material_system matsys = NULL;
    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);
    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);




    float c = 0.5f;
    float d = -7.0f;
    float o = -7.0f;
    float x = 0.6f;

    vertex meshVertices[] =
    {
        { -1.0f*x,   (0.0f + 0.5f)*c,  0.0f + o,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f },//0
        { 1.0f*x,    (0.0f + 0.5f)*c,   0.0f + o,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f },//1
        { 1.0f*x,   (-0.5f + 0.5f)*c,  0.0f + o ,  0.0f, 0.0f, 1.0f,    1.0f, 1.0f },//2
        { -1.0f*x,    (-0.5f + 0.5f)*c, 0.0f + o , 0.0f, 0.0f, 1.0f,    0.0f, 1.0f },//3

        { -1.0f*x,   (0.0f + 0.8f)*c, 1.0f*d + o,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f },//4
        { 1.0f*x,   (0.0f + 0.8f)*c, 1.0f*d + o,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f },//5
        { 1.0f*x,    (-0.5f + 0.8f)*c, 1.0f*d + o ,  0.0f, 0.0f, 1.0f,    1.0f, 1.0f },//6
        { -1.0f*x,    (-0.5f + 0.8f)*c, 1.0f*d + o , 0.0f, 0.0f, 1.0f,    0.0f, 1.0f },//7

                                                                                       // in the DOF unit test, we will focus on those 4 vertices :
        { -1.0f*x,   (0.0f + 1.5f)*c, 2.0f*d + o,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f },//8
        { 1.0f*x,    (0.0f + 1.5f)*c, 2.0f*d + o,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f },//9
        { 1.0f*x,    (-0.5f + 1.5f)*c, 2.0f*d + o ,  0.0f, 0.0f, 1.0f,    1.0f, 1.0f },//10
        { -1.0f*x,    (-0.5f + 1.5f)*c, 2.0f*d + o , 0.0f, 0.0f, 1.0f,    0.0f, 1.0f },//11

        { -1.0f*x,   (0.0f + 2.5f)*c, 3.0f*d + o,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f },//12
        { 1.0f*x,    (0.0f + 2.5f)*c, 3.0f*d + o,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f },//13
        { 1.0f*x,    (-0.5f + 2.5f)*c, 3.0f*d + o ,  0.0f, 0.0f, 1.0f,    1.0f, 1.0f },//14
        { -1.0f*x,    (-0.5f + 2.5f)*c, 3.0f*d + o , 0.0f, 0.0f, 1.0f,    0.0f, 1.0f },//15

        { -1.0f*x,   (0.0f + 4.0f)*c, 4.0f*d + o,    0.0f, 0.0f, 1.0f,    0.0f, 0.0f },//16
        { 1.0f*x,    (0.0f + 4.0f)*c, 4.0f*d + o,    0.0f, 0.0f, 1.0f,    1.0f, 0.0f },//17
        { 1.0f*x,    (-0.5f + 4.0f)*c, 4.0f*d + o ,  0.0f, 0.0f, 1.0f,    1.0f, 1.0f },//18
        { -1.0f*x,    (-0.5f + 4.0f)*c, 4.0f*d + o , 0.0f, 0.0f, 1.0f,    0.0f, 1.0f },//19
    };


    rpr_int indices[] =
    {
        2,1,0,
        3,2,0,

        4 + 2,4 + 1,4 + 0,
        4 + 3,4 + 2,4 + 0,

        8 + 2,8 + 1,8 + 0,
        8 + 3,8 + 2,8 + 0,

        12 + 2,12 + 1,12 + 0,
        12 + 3,12 + 2,12 + 0,

        16 + 2,16 + 1,16 + 0,
        16 + 3,16 + 2,16 + 0,

    };

    rpr_int num_face_vertices[] =
    {
        3, 3,
        3, 3,
        3, 3,
        3, 3,
        3, 3,
    };

    //mesh
    rpr_shape mesh = 0; status = rprContextCreateMesh(context,
                                                    (rpr_float const*)&meshVertices[0], sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(vertex),
                                                    (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 3), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(vertex),
                                                    (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 6), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(vertex),
                                                    (rpr_int const*)indices, sizeof(rpr_int),
                                                    (rpr_int const*)indices, sizeof(rpr_int),
                                                    (rpr_int const*)indices, sizeof(rpr_int),
                                                    num_face_vertices, sizeof(num_face_vertices) / sizeof(num_face_vertices[0]), &mesh);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);


    //set camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0.0f, +0.6f, 4.0f, 0.0f, +0.6f, 0.0f, 0.0f, 1.0f, 0.0f);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocalLength(camera, 200.0f);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFStop(camera, 1.0f);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocusDistance(camera, 25.0f);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetMode(camera, RPR_CAMERA_MODE_PERSPECTIVE);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    //output
    rpr_framebuffer_desc desc;
    desc.fb_width = 800;
    desc.fb_height = 600;
    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);
    status = rprContextSetParameter1u(context, "rendermode", RPR_RENDER_MODE_GLOBAL_ILLUMINATION);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0.0f, +0.6f, 4.0f));
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprPointLightSetRadiantPower3f(light, 1000, 1000, 1000);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);

    //render
    for (rpr_uint i = 0; i< kRenderIterations; i++)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    rprFrameBufferSaveToFile(frame_buffer, "feature_cameraDOF.png");

    //cleanup
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);

    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);

}

int main(int argc, char* argv[])
{
    MeshCreationTest();
    SimpleRenderTest();
    ComplexRenderTest();
    EnvLightClearTest();
    MemoryStatistics();
    DefaultMaterialTest();
    NullShaderTest();
//    TiledRender();
    test_feature_cameraDOF();
//    RunObjViewer(argc, argv);

    return 0;
}