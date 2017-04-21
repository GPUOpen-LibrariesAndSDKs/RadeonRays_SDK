#include "../Rpr/RadeonProRender.h"
#include "../RadeonRays/include/math/matrix.h"
#include "../RadeonRays/include/math/mathutils.h"
#include "../RprLoadStore/RprLoadStore.h"

#include <map>
#include <cassert>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

using namespace RadeonRays;

namespace
{
    //control how many frames to render in tests for resulting image
    int kRenderIterations = 256;
    //default structure for Vertex data
    struct Vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    //multi texture Vertex
    struct VertexMT
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex0[2];
        rpr_float tex1[2];
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
    struct Vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    Vertex quad[] =
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
    rpr_context	context = nullptr;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);

    rpr_shape quad_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&quad[0], 24, sizeof(Vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 3), 24, sizeof(Vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 6), 24, sizeof(Vertex),
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

    Vertex cube[] =
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

    Vertex plane[] =
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
    status = rprShapeSetTransform(mesh, false, &m.m00);
    assert(status == RPR_SUCCESS);
    
    //plane mesh
    rpr_shape plane_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&plane[0], 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 3), 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 6), 4, sizeof(Vertex),
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

    status = rprFrameBufferSaveToFile(frame_buffer, "Output/SimpleRenderTest_f1.jpg");
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
    status = rprFrameBufferSaveToFile(frame_buffer, "Output/SimpleRenderTest_f2.jpg");
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

    struct Vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    Vertex cube[] =
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

    Vertex plane[] =
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
        (rpr_float const*)&cube[0], 24, sizeof(Vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 3), 24, sizeof(Vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 6), 24, sizeof(Vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 12, &mesh);

    assert(status == RPR_SUCCESS);

    rpr_shape plane_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&plane[0], 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 3), 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 6), 4, sizeof(Vertex),
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
    rpr_image img = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_diffuse.jpg", &img);
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
    status = rprFrameBufferSaveToFile(frame_buffer, "Output/ComplexRender.jpg");
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

    struct Vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    Vertex cube[] =
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

    Vertex plane[] =
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
    status = rprShapeSetTransform(mesh, false, &m.m00);
    assert(status == RPR_SUCCESS);

    //plane mesh
    rpr_shape plane_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&plane[0], 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 3), 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 6), 4, sizeof(Vertex),
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
    rpr_image imageInput = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/studio015.hdr", &imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light, imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetIntensityScale(light, 5.f);
    assert(status == RPR_SUCCESS);
    matrix lightm = rotation_y(M_PI);
    status = rprLightSetTransform(light, false, &lightm.m00);
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

    rprFrameBufferSaveToFile(frame_buffer, "Output/EnvLightClear_Stage1.png");

    //check clear frame buffer
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    rprFrameBufferSaveToFile(frame_buffer, "Output/EnvLightClear_Stage2.png");

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

    rprFrameBufferSaveToFile(frame_buffer, "Output/EnvLightClear_Stage3.png");

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

    struct Vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    Vertex quad[] =
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
        (rpr_float const*)&quad[0], 4, sizeof(Vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 3), 4, sizeof(Vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 6), 4, sizeof(Vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &mesh);
    assert(status == RPR_SUCCESS);
    matrix r = rotation_x(M_PI_2);
    status = rprShapeSetTransform(mesh, false, &r.m00);
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

    rprFrameBufferSaveToFile(frame_buffer, "Output/DefaultMaterialTest.png");

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

    struct Vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    Vertex quad[] =
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
        (rpr_float const*)&quad[0], 4, sizeof(Vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 3), 4, sizeof(Vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 6), 4, sizeof(Vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &mesh);
    assert(status == RPR_SUCCESS);
    matrix r = rotation_x(M_PI_2);
    status = rprShapeSetTransform(mesh, false, &r.m00);
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

    rprFrameBufferSaveToFile(frame_buffer, "Output/NullShader_Stage1.png");

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

    rprFrameBufferSaveToFile(frame_buffer, "Output/NullShader_Stage2.png");


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

    struct Vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    Vertex cube[] =
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

    Vertex plane[] =
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
        (rpr_float const*)&cube[0], 24, sizeof(Vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 3), 24, sizeof(Vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 6), 24, sizeof(Vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 12, &mesh);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);
    matrix m = rotation_x(M_PI_4) * rotation_y(M_PI);
    status = rprShapeSetTransform(mesh, false, &m.m00);
    assert(status == RPR_SUCCESS);

    rpr_shape plane_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&plane[0], 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 3), 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 6), 4, sizeof(Vertex),
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
        status = rprContextRenderTile(context, 10, desc.fb_width/2, 10, desc.fb_height/2);
        //status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "Output/Tiled_Render.png");

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

void AOVTest()
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

    struct Vertex
    {
        rpr_float pos[3];
        rpr_float norm[3];
        rpr_float tex[2];
    };

    Vertex cube[] =
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

    Vertex plane[] =
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
        (rpr_float const*)&cube[0], 24, sizeof(Vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 3), 24, sizeof(Vertex),
        (rpr_float const*)((char*)&cube[0] + sizeof(rpr_float) * 6), 24, sizeof(Vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 12, &mesh);

    assert(status == RPR_SUCCESS);

    rpr_shape plane_mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&plane[0], 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 3), 4, sizeof(Vertex),
        (rpr_float const*)((char*)&plane[0] + sizeof(rpr_float) * 6), 4, sizeof(Vertex),
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
    rpr_image img = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_diffuse.jpg", &img);
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

    const std::map<int, std::string> kAovs = { {RPR_AOV_COLOR, "RPR_AOV_COLOR"},
                                        { RPR_AOV_OPACITY, "RPR_AOV_OPACITY" },
                                        { RPR_AOV_WORLD_COORDINATE, "RPR_AOV_WORLD_COORDINATE"},
                                        { RPR_AOV_UV, "RPR_AOV_UV"},
                                        { RPR_AOV_MATERIAL_IDX, "RPR_AOV_MATERIAL_IDX"},
                                        { RPR_AOV_GEOMETRIC_NORMAL, "RPR_AOV_GEOMETRIC_NORMAL"},
                                        { RPR_AOV_SHADING_NORMAL, "RPR_AOV_SHADING_NORMAL"},
                                        { RPR_AOV_DEPTH, "RPR_AOV_DEPTH"},
                                        { RPR_AOV_OBJECT_ID, "RPR_AOV_OBJECT_ID"},
                                        { RPR_AOV_OBJECT_GROUP_ID, "RPR_AOV_OBJECT_GROUP_ID"},};
    for (auto aov : kAovs)
    {
        rpr_framebuffer buf = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &buf);
        assert(status == RPR_SUCCESS);
        status = rprContextSetAOV(context, aov.first, buf);
        //some AOVs not supported now
        if (status == FR_ERROR_UNIMPLEMENTED)
        {
            std::cout << "AOVTest():Warning " << aov.second << " is unsupported" << std::endl;
            status = rprObjectDelete(buf); buf = NULL;
            continue;
        }
        status = rprFrameBufferClear(buf);
        assert(status == RPR_SUCCESS);
    }
    
    //render
    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    for (auto aov : kAovs)
    {
        rpr_framebuffer buf = NULL; 
        status = rprContextGetAOV(context, aov.first, &buf);
        //some AOVs not supported now
        if (status == FR_ERROR_UNIMPLEMENTED)
        {
            continue;
        }
        const std::string name = "Output/AOVTest_" + aov.second + ".jpg";
        status = rprFrameBufferSaveToFile(buf, name.c_str());
        assert(status == RPR_SUCCESS);
    }


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

    for (auto aov : kAovs)
    {
        rpr_framebuffer buf = NULL;
        status = rprContextGetAOV(context, aov.first, &buf);
        //some AOVs not supported now
        if (status == FR_ERROR_UNIMPLEMENTED)
        {
            continue;
        }
        status = rprObjectDelete(buf); buf = NULL;
        assert(status == RPR_SUCCESS);
    }

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

    Vertex meshVertices[] =
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
                                                    (rpr_float const*)&meshVertices[0], sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
                                                    (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 3), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
                                                    (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 6), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
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
    rprFrameBufferSaveToFile(frame_buffer, "Output/feature_cameraDOF.png");

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

void test_feature_ContextImageFromData()
{
    rpr_int status = RPR_SUCCESS;
    
    //create context and scene
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    rpr_material_system matsys = NULL;
    status = rprContextCreateMaterialSystem(context, 0, &matsys);
    assert(status == RPR_SUCCESS);
    rpr_scene scene = NULL;
    status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);
    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    //texture
    rpr_image_format imageFormat;
    memset(&imageFormat, 0, sizeof(imageFormat));
    imageFormat.num_components = 4;
    imageFormat.type = RPR_COMPONENT_TYPE_FLOAT32;
    rpr_image_desc imageDesc;
    memset(&imageDesc, 0, sizeof(imageDesc));
    imageDesc.image_depth = 1;
    imageDesc.image_width = 32;
    imageDesc.image_height = 32;
    imageDesc.image_row_pitch = imageDesc.image_width * sizeof(float) * 4;
    imageDesc.image_slice_pitch = imageDesc.image_width * imageDesc.image_height * sizeof(float) * 4;
    std::vector<float> dataImage(imageDesc.image_width  *  imageDesc.image_height * 4);

    //first : fill texture with red
    for (rpr_uint y = 0; y<imageDesc.image_height; y++)
    {
        for (rpr_uint x = 0; x<imageDesc.image_width; x++)
        {
            dataImage[x * 4 + y*imageDesc.image_width * 4 + 0] = 1.0f; // R
            dataImage[x * 4 + y*imageDesc.image_width * 4 + 1] = 0.0f; // G
            dataImage[x * 4 + y*imageDesc.image_width * 4 + 2] = 0.0f; // B
            dataImage[x * 4 + y*imageDesc.image_width * 4 + 3] = 1.0f; // A
        }
    }

    //second step : draw a square
    for (rpr_uint y = 10; y<imageDesc.image_height; y++)
    {
        for (rpr_uint x = 10; x<imageDesc.image_width; x++)
        {
            dataImage[x * 4 + y*imageDesc.image_width * 4 + 0] = 0.0f; // R
            dataImage[x * 4 + y*imageDesc.image_width * 4 + 1] = 0.7f; // G
            dataImage[x * 4 + y*imageDesc.image_width * 4 + 2] = 0.2f; // B
            dataImage[x * 4 + y*imageDesc.image_width * 4 + 3] = 1.0f; // A
        }
    }

    //first point is blue
    dataImage[0] = 0.0f;
    dataImage[1] = 0.0f;
    dataImage[2] = 1.0f;
    dataImage[3] = 1.0f;

    //last point is yellow
    dataImage[imageDesc.image_width * imageDesc.image_height * 4 - 4] = 1.0f;
    dataImage[imageDesc.image_width * imageDesc.image_height * 4 - 3] = 1.0f;
    dataImage[imageDesc.image_width * imageDesc.image_height * 4 - 2] = 0.0f;
    dataImage[imageDesc.image_width * imageDesc.image_height * 4 - 1] = 1.0f;

    rpr_image imageBuilt = NULL; status = rprContextCreateImage(context, imageFormat, &imageDesc, dataImage.data(), &imageBuilt);
    assert(status == RPR_SUCCESS);
    //free memory
    std::vector<float>().swap(dataImage);

    //mesh
    Vertex meshVertices[] = { { -2.0f, 2.0f, 0.0f, 0.0f, 0.0f, +1.0f, 0.0f, 0.0f },
    { 2.0f, 2.0f, 0.0f, 0.0f, 0.0f, +1.0f, 1.0f, 0.0f },
    { 2.0f, -2.0f, 0.0f, 0.0f, 0.0f, +1.0f, 1.0f, 1.0f },
    { -2.0f, -2.0f, 0.0f, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f }, };
    rpr_int indices[] = { 3, 2, 1, 0, };
    rpr_int num_face_vertices[] = { 4, };

    rpr_shape mesh = 0;
    status = rprContextCreateMesh(context,
        (rpr_float const*)&meshVertices[0], 4, sizeof(Vertex),
        (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 3), 4, sizeof(Vertex),
        (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 6), 4, sizeof(Vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 1, &mesh);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);

    //materials
    rpr_material_node textureNormalMap = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &textureNormalMap);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(textureNormalMap, "data", imageBuilt);
    assert(status == RPR_SUCCESS);
    rpr_material_node diffuseSphere = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuseSphere);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(diffuseSphere, "color", textureNormalMap);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh, diffuseSphere);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 0, 7, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0, 0, 4)) * rotation_z(3.14f);
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprPointLightSetRadiantPower3f(light, 40, 40, 40);
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
    for (rpr_uint i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    rprFrameBufferSaveToFile(frame_buffer, "Output/feature_ContextImageFromData.png");

    //cleanup
    FR_MACRO_SAFE_FRDELETE(textureNormalMap);
    status = rprObjectDelete(imageBuilt);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    status = rprObjectDelete(diffuseSphere);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(matsys); matsys = NULL;
    assert(status == RPR_SUCCESS);
}

//test RPR_MATERIAL_NODE_INPUT_LOOKUP and rprContextCreateMeshEx unsupported
void test_feature_multiUV()
{
    rpr_int status = RPR_SUCCESS;

    //create context and scene
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

    
    rpr_material_node uv_node = NULL;
    status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_INPUT_LOOKUP, &uv_node);
    assert(status == RPR_ERROR_UNSUPPORTED);
      
    VertexMT meshVertices[] =
    {
        { -2.0f, 2.0f, 0.0f,    0.0f, 0.0f, +1.0f,    0.0f, 0.0f ,    1.0f, 1.0f },
        { 2.0f, 2.0f, 0.0f,    0.0f, 0.0f, +1.0f,    1.0f, 0.0f ,    0.0f, 1.0f },
        { 2.0f, -2.0f, 0.0f ,  0.0f, 0.0f, +1.0f,    1.0f, 1.0f ,    0.0f, 0.0f },
        { -2.0f, -2.0f, 0.0f , 0.0f, 0.0f, +1.0f,    0.0f, 1.0f ,    1.0f, 0.0f },
    };
    rpr_int indices[] =
    {
        3,2,1,0,
    };
    rpr_int num_face_vertices[] =
    {
        4,
    };

    //TODO: codestyle
    unsigned int meshVertices_nbOfElement = sizeof(meshVertices) / sizeof(meshVertices[0]);
    unsigned int num_face_vertices_nbOfElement = sizeof(num_face_vertices) / sizeof(num_face_vertices[0]);
    rpr_shape mesh = 0;

    const rpr_float* texcoords[] = { (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 6), (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 8) };
    size_t num_texcoords[] = { meshVertices_nbOfElement,  meshVertices_nbOfElement };
    rpr_int texcoord_stride[] = { sizeof(VertexMT)   ,   sizeof(VertexMT) };
    const rpr_int*   texcoord_indices_[] = { indices,   indices };
    rpr_int tidx_stride_[] = { sizeof(rpr_int)   ,sizeof(rpr_int) };

    status = rprContextCreateMeshEx(context,
        (rpr_float const*)&meshVertices[0], meshVertices_nbOfElement, sizeof(VertexMT),
        (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 3), meshVertices_nbOfElement, sizeof(VertexMT),
        nullptr, 0, 0,
        2,
        texcoords, num_texcoords, texcoord_stride,
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),

        texcoord_indices_, tidx_stride_,

        num_face_vertices, num_face_vertices_nbOfElement, &mesh);
    assert(status == RPR_ERROR_UNSUPPORTED);

    //cleanup
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(matsys); matsys = NULL;
    assert(status == RPR_SUCCESS);
}

void test_apiMecha_Light()
{
    rpr_int status = RPR_SUCCESS;
    //create context and scene
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);
    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);

    for (rpr_int iter = 0; iter<3; iter++)//add/destroy several times
    {

        rpr_light lightDir = NULL; status = rprContextCreateDirectionalLight(context, &lightDir);
        assert(status == RPR_SUCCESS);
        rpr_light lightEnv = NULL; status = rprContextCreateEnvironmentLight(context, &lightEnv);
        assert(status == RPR_SUCCESS);
        rpr_light lightPoint = NULL; status = rprContextCreatePointLight(context, &lightPoint);
        assert(status == RPR_SUCCESS);
        rpr_light lightSpot = NULL; status = rprContextCreateSpotLight(context, &lightSpot);
        assert(status == RPR_SUCCESS);

        matrix transform = translation(float3(-0.7f, 0.5f, 1.5f));

        status = rprLightSetTransform(lightDir, true, &transform.m00);
        assert(status == RPR_SUCCESS);
        status = rprLightSetTransform(lightEnv, true, &transform.m00);
        assert(status == RPR_SUCCESS);
        status = rprLightSetTransform(lightPoint, true, &transform.m00);
        assert(status == RPR_SUCCESS);
        status = rprLightSetTransform(lightSpot, true, &transform.m00);
        assert(status == RPR_SUCCESS);

        size_t querrySize = 0;
        char* querryData = (char*)malloc(1);

        for (rpr_int i = 0; i<4; i++)
        {
            rpr_light pLightTest = NULL;
            rpr_light_type LightTypeShouldBe = 0;
            if (i == 0) { pLightTest = lightDir;    LightTypeShouldBe = RPR_LIGHT_TYPE_DIRECTIONAL; }
            else if (i == 1) { pLightTest = lightEnv;    LightTypeShouldBe = RPR_LIGHT_TYPE_ENVIRONMENT; }
            else if (i == 2) { pLightTest = lightPoint;  LightTypeShouldBe = RPR_LIGHT_TYPE_POINT; }
            else if (i == 3) { pLightTest = lightSpot;   LightTypeShouldBe = RPR_LIGHT_TYPE_SPOT; }
            else { assert(false); }

            status = rprLightGetInfo(pLightTest, RPR_LIGHT_TRANSFORM, 0, NULL, &querrySize);
            assert(status == RPR_SUCCESS);
            querryData = (char*)realloc(querryData, querrySize);
            status = rprLightGetInfo(pLightTest, RPR_LIGHT_TRANSFORM, querrySize, querryData, NULL);
            assert(status == RPR_SUCCESS);
            if (querrySize != sizeof(matrix)) { assert(false); }
            if (memcmp(querryData, &transform.m00, querrySize) != 0) { assert(false); }

            status = rprLightGetInfo(pLightTest, RPR_LIGHT_TYPE, 0, NULL, &querrySize);
            assert(status == RPR_SUCCESS);
            querryData = (char*)realloc(querryData, querrySize);
            status = rprLightGetInfo(pLightTest, RPR_LIGHT_TYPE, querrySize, querryData, NULL);
            assert(status == RPR_SUCCESS);
            if (querrySize != sizeof(rpr_light_type)) { assert(false); }
            if (memcmp(querryData, &LightTypeShouldBe, querrySize) != 0) { assert(false); }
        }

        free(querryData); querryData = NULL;

        //attach
        status = rprSceneAttachLight(scene, lightDir);
        assert(status == RPR_SUCCESS);
        status = rprSceneAttachLight(scene, lightEnv);
        assert(status == RPR_SUCCESS);
        status = rprSceneAttachLight(scene, lightPoint);
        assert(status == RPR_SUCCESS);
        status = rprSceneAttachLight(scene, lightSpot);
        assert(status == RPR_SUCCESS);

        //detach
        status = rprSceneDetachLight(scene, lightDir);
        assert(status == RPR_SUCCESS);
        status = rprSceneDetachLight(scene, lightEnv);
        assert(status == RPR_SUCCESS);
        status = rprSceneDetachLight(scene, lightPoint);
        assert(status == RPR_SUCCESS);
        status = rprSceneDetachLight(scene, lightSpot);
        assert(status == RPR_SUCCESS);

        //cleanup
        status = rprObjectDelete(lightDir); lightDir = NULL;
        assert(status == RPR_SUCCESS);
        status = rprObjectDelete(lightEnv); lightEnv = NULL;
        assert(status == RPR_SUCCESS);
        status = rprObjectDelete(lightPoint); lightPoint = NULL;
        assert(status == RPR_SUCCESS);
        status = rprObjectDelete(lightSpot); lightSpot = NULL;
        assert(status == RPR_SUCCESS);
    }

    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
}

void test_feature_LightDirectional()
{
    rpr_int status = RPR_SUCCESS;

    //create context and scene
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

    //vertex data
    float c = 0.5f;
    float d = -7.0f;
    float o = -7.0f;
    float x = 0.6f;

    Vertex meshVertices[] =
    {
        { -2.0f, 2.0f, 0.0f,    0.0f, 0.0f, +1.0f,    0.0f, 0.0f },
        {  2.0f, 2.0f, 0.0f,    0.0f, 0.0f, +1.0f,    1.0f, 0.0f  },
        {  2.0f, -2.0f, 0.0f ,  0.0f, 0.0f, +1.0f,    1.0f, 1.0f  },
        {  -2.0f, -2.0f, 0.0f , 0.0f, 0.0f, +1.0f,    0.0f, 1.0f },
    };
    rpr_int indices[] =
    {
        3,2,1,0,
    };
    rpr_int num_face_vertices[] =
    {
        4,
    };

    //mesh
    rpr_shape mesh = 0; status = rprContextCreateMesh(context,
        (rpr_float const*)&meshVertices[0], sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
        (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 3), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
        (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 6), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, sizeof(num_face_vertices) / sizeof(num_face_vertices[0]), &mesh);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);
    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 0, 7, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);


    rpr_framebuffer_desc desc;
    desc.fb_width = 800;
    desc.fb_height = 600;

    //shader
    rpr_material_node shader_lambert = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &shader_lambert);
    status = rprMaterialNodeSetInputF(shader_lambert, "color", 0.0f, 0.5f, 1.0f, 1.0f);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh, shader_lambert);
    assert(status == RPR_SUCCESS);


    //light
    rpr_light light = NULL; status = rprContextCreateDirectionalLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(1.0f, 0.0f, 1.0f)) * rotation_y(0.5f*3.14f / 2.0f);
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprDirectionalLightSetRadiantPower3f(light, 1.0f, 1.0f, 1.0f);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);


    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);

    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);

    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);

    //render
    for (rpr_uint i = 0; i<kRenderIterations; i++)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    rprFrameBufferSaveToFile(frame_buffer, "Output/feature_LightDirectional.png");

    //dynamic:
    matrix lightm2 = translation(float3(0.0f, 0.0f, 1.0f)) * rotation_y(0.0f*3.14f / 2.0f);
    status = rprLightSetTransform(light, false, &lightm2.m00);
    assert(status == RPR_SUCCESS);
    status = rprDirectionalLightSetRadiantPower3f(light, 8.0f, 8.0f, 0.0f);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);

    for (rpr_uint i = 0; i<kRenderIterations; i++)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    rprFrameBufferSaveToFile(frame_buffer, "Output/feature_LightDirectional_dynamic.png");

    //cleanup
    status = rprObjectDelete(shader_lambert);
    assert(status == RPR_SUCCESS);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
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

void InstancingTest()
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

    //material
    rpr_material_node diffuse = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuse);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(diffuse, "color", 0.7f, 0.7f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);

    //sphere
    rpr_shape mesh = CreateSphere(context, 64, 32, 1.f, float3());
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh, diffuse);
    assert(status == RPR_SUCCESS);

    std::vector<rpr_shape> instances;
    std::vector<rpr_material_node> materials;

    for (auto x = -20; x <= 20; x+= 2)
        for (auto y = -20; y <= 20; y += 2)
        {
            if (x == 0 && y == 0) continue;
            rpr_shape instance = nullptr;
            status = rprContextCreateInstance(context, mesh, &instance);
            assert(status == RPR_SUCCESS);

            float s = 0.75f + rand_float();
            matrix m = translation(float3(x, y, rand_float() * 10.f - 5.f)) * scale(float3(s, s, s));
            status = rprShapeSetTransform(instance, true, &m.m00);
            assert(status == RPR_SUCCESS);

            status = rprSceneAttachShape(scene, instance);
            assert(status == RPR_SUCCESS);

            if (rand_float() < 0.1)
            {
                rpr_material_node mat = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &mat);
                assert(status == RPR_SUCCESS);
                status = rprMaterialNodeSetInputF(mat, "color", rand_float(), rand_float(), rand_float(), 1.f);
                assert(status == RPR_SUCCESS);
                status = rprShapeSetMaterial(instance, mat);
                assert(status == RPR_SUCCESS);
                materials.push_back(mat);
            }
            else if (rand_float() < 0.5)
            {
                rpr_material_node spec = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_MICROFACET, &spec);
                assert(status == RPR_SUCCESS);
                status = rprMaterialNodeSetInputF(spec, "color", rand_float(), rand_float(), rand_float(), 1.f);
                status = rprMaterialNodeSetInputF(spec, "roughness", 0.5f * rand_float(), 0.5f * rand_float(), 0.5f * rand_float(), 1.f);
                assert(status == RPR_SUCCESS);
                status = rprShapeSetMaterial(instance, spec);
                assert(status == RPR_SUCCESS);
                materials.push_back(spec);
            }
            else
            {
                rpr_material_node mat = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &mat);
                assert(status == RPR_SUCCESS);
                status = rprMaterialNodeSetInputF(mat, "color", rand_float(), rand_float(), rand_float(), 1.f);
                assert(status == RPR_SUCCESS);
                materials.push_back(mat);
                rpr_material_node spec = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_MICROFACET, &spec);
                assert(status == RPR_SUCCESS);
                status = rprMaterialNodeSetInputF(spec, "color", rand_float(), rand_float(), rand_float(), 1.f);
                status = rprMaterialNodeSetInputF(spec, "roughness", 0.1f * rand_float(), 0.1f * rand_float(), 0.1f * rand_float(), 1.f);
                assert(status == RPR_SUCCESS);
                materials.push_back(spec);
                rpr_material_node layered = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_BLEND, &layered);
                assert(status == RPR_SUCCESS);
                status = rprMaterialNodeSetInputN(layered, "color0", mat);
                status = rprMaterialNodeSetInputN(layered, "color1", spec);
                assert(status == RPR_SUCCESS);
                rpr_material_node fresnel = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_FRESNEL, &fresnel);
                assert(status == RPR_SUCCESS);

                auto ior = 1.f + rand_float() * 2.f;
                status = rprMaterialNodeSetInputF(fresnel, "ior", ior, ior, ior, ior);
                assert(status == RPR_SUCCESS);
                materials.push_back(fresnel);
                status = rprMaterialNodeSetInputN(layered, "weight", fresnel);
                assert(status == RPR_SUCCESS);
                status = rprShapeSetMaterial(instance, layered);
                assert(status == RPR_SUCCESS);
                materials.push_back(layered);
            }

            instances.push_back(instance);
        }

    rpr_light light = NULL; status = rprContextCreateEnvironmentLight(context, &light);
    assert(status == RPR_SUCCESS);
    rpr_image imageInput = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/studio015.hdr", &imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light, imageInput);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 0, 20, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprCameraSetFocalLength(camera, 23.f);
    assert(status == RPR_SUCCESS);
    //status = rprCameraSetFStop(camera, 5.4f);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    //light

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

    //change light
    status = rprFrameBufferClear(frame_buffer);  assert(status == RPR_SUCCESS);
    assert(status == RPR_SUCCESS);

    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    status = rprFrameBufferSaveToFile(frame_buffer, "Output/ManySpheresTest.jpg");
    assert(status == RPR_SUCCESS);
    rpr_render_statistics rs;
    status = rprContextGetInfo(context, RPR_CONTEXT_RENDER_STATISTICS, sizeof(rpr_render_statistics), &rs, NULL);
    assert(status == RPR_SUCCESS);


    //cleanup
    for (auto& i : instances)
    {
        status = rprObjectDelete(i);
        assert(status == RPR_SUCCESS);
    }

    for (auto& i : materials)
    {
        status = rprObjectDelete(i);
        assert(status == RPR_SUCCESS);
    }

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

void BumpmapTest()
{
    rpr_int status = RPR_SUCCESS;
    //context and scene
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

    rpr_shape mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&quad[0], 4, sizeof(vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 3), 4, sizeof(vertex),
        (rpr_float const*)((char*)&quad[0] + sizeof(rpr_float) * 6), 4, sizeof(vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, 2, &mesh);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);

    //materials
    rpr_image img = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_diffuse.jpg", &img);
    assert(status == RPR_SUCCESS);
    rpr_material_node materialNodeTextureA = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &materialNodeTextureA);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(materialNodeTextureA, "data", img);
    assert(status == RPR_SUCCESS);

    rpr_image imgBump = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_normal.jpg", &imgBump);
    assert(status == RPR_SUCCESS);
    rpr_material_node materialNodeTextureB = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_NORMAL_MAP, &materialNodeTextureB);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(materialNodeTextureB, "data", imgBump);
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
    status = rprMaterialNodeSetInputF(diffuse, "color", 1, 1, 1, 1);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(diffuse, "color", materialNodeTextureA);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(diffuse, "normal", materialNodeTextureB);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh, diffuse);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0, 6, 0));
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprPointLightSetRadiantPower3f(light, 200, 200, 200);
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

    //unsupported:
    //status = rprContextSetParameter1u(context, "rendermode", RPR_RENDER_MODE_NORMAL);
    //assert(status == RPR_SUCCESS);
    //status = rprContextSetParameter1u(context, "xflip", 0);
    //assert(status == RPR_SUCCESS);
    //status = rprContextSetParameter1u(context, "yflip", 1);
    //assert(status == RPR_SUCCESS);

    //render
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    for (int i = 0; i < 100; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    rprFrameBufferSaveToFile(frame_buffer, "Output/BumpMap_Stage1.png");

    //cleanup
    FR_MACRO_SAFE_FRDELETE(materialNodeTextureA);
    FR_MACRO_SAFE_FRDELETE(materialNodeTextureB);
    FR_MACRO_SAFE_FRDELETE(img);
    FR_MACRO_SAFE_FRDELETE(imgBump);
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

void test_feature_shaderBumpmap()
{
    rpr_int status = RPR_SUCCESS;
    //create scene and context
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

    //materials
    rpr_material_node shaderMicrofacet = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_MICROFACET, &shaderMicrofacet);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(shaderMicrofacet, "color", 0.3f, 0.7f, 0.9f, 1.0f);
    assert(status == RPR_SUCCESS);

    rpr_image imageInputA = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_bump.jpg", &imageInputA);
    assert(status == RPR_SUCCESS);
    rpr_material_node textureNormalMap = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_BUMP_MAP, &textureNormalMap);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(textureNormalMap, "data", imageInputA);
    assert(status == RPR_SUCCESS);

    status = rprMaterialNodeSetInputN(shaderMicrofacet, "normal", textureNormalMap);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(shaderMicrofacet, "ior", 1.3f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(shaderMicrofacet, "roughness", 0.5f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);

    Vertex meshVertices[] =
    {
        { -2.0f, 2.0f, 0.0f,    0.0f, 0.0f, +1.0f,    0.0f, 0.0f },
        { 2.0f, 2.0f, 0.0f,    0.0f, 0.0f, +1.0f,    1.0f, 0.0f },
        { 2.0f, -2.0f, 0.0f ,  0.0f, 0.0f, +1.0f,    1.0f, 1.0f },
        { -2.0f, -2.0f, 0.0f , 0.0f, 0.0f, +1.0f,    0.0f, 1.0f },
    };
    rpr_int indices[] =
    {
        3,2,1,0,
    };
    rpr_int num_face_vertices[] =
    {
        4,
    };

    //mesh
    rpr_shape mesh = 0; status = rprContextCreateMesh(context,
        (rpr_float const*)&meshVertices[0], sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
        (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 3), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
        (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 6), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, sizeof(num_face_vertices) / sizeof(num_face_vertices[0]), &mesh);

    status = rprShapeSetMaterial(mesh, shaderMicrofacet);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 0, 7, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0, 0, 4)) * rotation_z(3.14f);
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprPointLightSetRadiantPower3f(light, 40, 40, 40);
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
    for (rpr_uint i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    rprFrameBufferSaveToFile(frame_buffer, "Output/feature_shaderBumpmap_0.png");

    //dynamic : change parameters
    status = rprMaterialNodeSetInputF(shaderMicrofacet, "color", 0.9f, 0.7f, 0.3f, 1.0f);
    assert(status == RPR_SUCCESS);

    rpr_image imageInputB = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_bump.jpg", &imageInputB);
    assert(status == RPR_SUCCESS);
    rpr_material_node textureNormalMap2 = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_BUMP_MAP, &textureNormalMap2);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(textureNormalMap2, "data", imageInputB);
    assert(status == RPR_SUCCESS);

    status = rprMaterialNodeSetInputN(shaderMicrofacet, "normal", textureNormalMap2);
    assert(status == RPR_SUCCESS);

    status = rprMaterialNodeSetInputF(shaderMicrofacet, "ior", 1.3f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    for (rpr_uint i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    rprFrameBufferSaveToFile(frame_buffer, "Output/feature_shaderBumpmap_1.png");


    //dynamic : change parameters
    //unsupported:
    //status = rprMaterialNodeSetInputF(textureNormalMap2, "bumpscale", 2.0f, 1.0f, 1.0f, 1.0f);
    //assert(status == RPR_SUCCESS);

    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    for (rpr_uint i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    rprFrameBufferSaveToFile(frame_buffer, "Output/feature_shaderBumpmap_2.png");


    status = rprObjectDelete(imageInputA); imageInputA = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(imageInputB); imageInputB = NULL;
    assert(status == RPR_SUCCESS);
    FR_MACRO_SAFE_FRDELETE(textureNormalMap);
    FR_MACRO_SAFE_FRDELETE(textureNormalMap2);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(shaderMicrofacet);
    assert(status == RPR_SUCCESS);
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

void test_feature_shaderTypeLayered()
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
    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    //materials
    rpr_material_node diffuseSphere = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuseSphere);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(diffuseSphere, "color", 0.0f, 0.0f, 1.0f, 1.0f);
    assert(status == RPR_SUCCESS);
    rpr_material_node specularSphere = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_MICROFACET, &specularSphere);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(specularSphere, "color", 0.1f, 0.9f, 0.2f, 1.0f);
    assert(status == RPR_SUCCESS);
    rpr_image imageInputA = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_normal.jpg", &imageInputA);
    assert(status == RPR_SUCCESS);
    rpr_material_node textureNormalMap = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_NORMAL_MAP, &textureNormalMap);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(textureNormalMap, "data", imageInputA);
    assert(status == RPR_SUCCESS);

    status = rprMaterialNodeSetInputN(specularSphere, "normal", textureNormalMap);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(specularSphere, "ior", 1.3f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(specularSphere, "roughness", 0.5f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);

    rpr_material_node layered = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_BLEND, &layered);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(layered, "color1", specularSphere);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(layered, "color0", diffuseSphere);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(layered, "weight", 0.4f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);

    Vertex meshVertices[] =
    {
        { -2.0f, 2.0f, 0.0f,    0.0f, 0.0f, +1.0f,    0.0f, 0.0f },
        { 2.0f, 2.0f, 0.0f,    0.0f, 0.0f, +1.0f,    1.0f, 0.0f },
        { 2.0f, -2.0f, 0.0f ,  0.0f, 0.0f, +1.0f,    1.0f, 1.0f },
        { -2.0f, -2.0f, 0.0f , 0.0f, 0.0f, +1.0f,    0.0f, 1.0f },
    };
    rpr_int indices[] =
    {
        3,2,1,0,
    };
    rpr_int num_face_vertices[] =
    {
        4,
    };

    //mesh
    rpr_shape mesh = 0; status = rprContextCreateMesh(context,
        (rpr_float const*)&meshVertices[0], sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
        (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 3), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
        (rpr_float const*)((char*)&meshVertices[0] + sizeof(rpr_float) * 6), sizeof(meshVertices) / sizeof(meshVertices[0]), sizeof(Vertex),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        (rpr_int const*)indices, sizeof(rpr_int),
        num_face_vertices, sizeof(num_face_vertices) / sizeof(num_face_vertices[0]), &mesh);

    status = rprShapeSetMaterial(mesh, layered);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 0, 7, 0, 0, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, camera);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);
    matrix lightm = translation(float3(0, 0, 4)) * rotation_z(3.14f);
    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);
    status = rprPointLightSetRadiantPower3f(light, 40, 40, 40);
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
    for (rpr_uint i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "Output/feature_shaderTypeLayered.png");


    //dynamic : change parameters :
    rpr_image imageInputB = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/test_normal.jpg", &imageInputB);
    assert(status == RPR_SUCCESS);
    rpr_material_node textureNormalMap2 = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_NORMAL_MAP, &textureNormalMap2);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(textureNormalMap2, "data", imageInputB);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(specularSphere, "normal", textureNormalMap2);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(specularSphere, "roughness", 0.2f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(layered, "weight", 0.6f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    for (rpr_uint i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    rprFrameBufferSaveToFile(frame_buffer, "Output/feature_shaderTypeLayered_dynamic.png");

    //cleanup
    FR_MACRO_SAFE_FRDELETE(textureNormalMap);
    FR_MACRO_SAFE_FRDELETE(textureNormalMap2);
    status = rprObjectDelete(imageInputA); imageInputA = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(imageInputB); imageInputB = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
    assert(status == RPR_SUCCESS);
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    status = rprSceneSetCamera(scene, NULL);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(camera); camera = NULL;
    assert(status == RPR_SUCCESS);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    status = rprObjectDelete(layered);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(specularSphere);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(diffuseSphere);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(scene); scene = NULL;
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(matsys); matsys = NULL;
    assert(status == RPR_SUCCESS);
}


void LoadFrs(const std::string& file)
{
	rpr_int status = RPR_SUCCESS;

	//create context and scene
	rpr_context	context;
	status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
	rpr_scene scene = NULL;
	rpr_material_system matsys = NULL;
	status = rprContextCreateMaterialSystem(context, 0, &matsys);
	assert(status == RPR_SUCCESS);
	
	//load frs file
	status = rprsImport(file.c_str(), context, matsys, &scene, false);
	assert(status == RPR_SUCCESS);

	////add env light
	//rpr_light light = nullptr;
	//status = rprContextCreateEnvironmentLight(context, &light);
	//assert(status == RPR_SUCCESS);
	//rpr_image image_input = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/Studio015.hdr", &image_input);
	//assert(status == RPR_SUCCESS);
	//status = rprEnvironmentLightSetImage(light, image_input);
	//assert(status == RPR_SUCCESS);
	//status = rprEnvironmentLightSetIntensityScale(light, 1.f);
	//assert(status == RPR_SUCCESS);
	//status = rprSceneAttachLight(scene, light);
	//assert(status == RPR_SUCCESS);


	size_t shapeCount = 0;
	status = rprSceneGetInfo(scene, RPR_SCENE_SHAPE_COUNT, sizeof(shapeCount), &shapeCount, NULL);
	assert(status == RPR_SUCCESS);

	std::vector<rpr_shape> shapes(shapeCount);
	status = rprSceneGetInfo(scene, RPR_SCENE_SHAPE_LIST, shapeCount * sizeof(rpr_shape), shapes.data(), NULL);
	assert(status == RPR_SUCCESS);

	//rpr_material_node grey = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &grey);
	//assert(status == RPR_SUCCESS);
	//status = rprMaterialNodeSetInputF(grey, "color", 0.5f, 0.5f, 0.5f, 1.0f);
	//assert(status == RPR_SUCCESS);
	//// Iterate over shapes and replace materials.
	//for (size_t i = 0; i < shapeCount; i++)
	//{
	//	// Retrieve the shape's material node.
	//	rpr_material_node mat = 0;
	//	status = rprShapeSetMaterial(shapes[i], grey);
	//	assert(status == RPR_SUCCESS);
	//}

	rpr_camera cam;
	status = rprSceneGetInfo(scene, RPR_SCENE_CAMERA, sizeof(rpr_camera), &cam, NULL);
	assert(status == RPR_SUCCESS);
	status = rprCameraSetFStop(cam, 0.f);
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
	for (rpr_uint i = 0; i < kRenderIterations; ++i)
	{
		status = rprContextRender(context);
		assert(status == RPR_SUCCESS);
	}

	rprFrameBufferSaveToFile(frame_buffer, "Output/LoadFrs.png");

	//cleanup
	status = rprObjectDelete(frame_buffer); frame_buffer = NULL;
 	assert(status == RPR_SUCCESS);
	status = rprObjectDelete(scene); scene = NULL;
	assert(status == RPR_SUCCESS);
	status = rprObjectDelete(matsys); matsys = NULL;
	assert(status == RPR_SUCCESS);

}

void UpdateMaterial()
{
    //check material properties updated properly
    //on blend material
    rpr_int status = RPR_SUCCESS;
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &context);
    assert(status == RPR_SUCCESS);
    rpr_material_system matsys = NULL;
    status = rprContextCreateMaterialSystem(context, 0, &matsys);
    assert(status == RPR_SUCCESS);

    rpr_scene scene = NULL; status = rprContextCreateScene(context, &scene);
    assert(status == RPR_SUCCESS);

    //material
    rpr_material_node red = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &red);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(red, "color", 1.f, 0.f, 0.f, 1.f);
    assert(status == RPR_SUCCESS);
    rpr_material_node green = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &green);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputF(green, "color", 0.f, 1.f, 0.f, 1.f);
    assert(status == RPR_SUCCESS);
    rpr_material_node layered = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_BLEND, &layered);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(layered, "color0", red);
    status = rprMaterialNodeSetInputN(layered, "color1", green);
    assert(status == RPR_SUCCESS);
    rpr_material_node fresnel = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_FRESNEL, &fresnel);
    assert(status == RPR_SUCCESS);
    auto ior = 0.f;
    status = rprMaterialNodeSetInputF(fresnel, "ior", ior, ior, ior, ior);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(layered, "weight", fresnel);
    assert(status == RPR_SUCCESS);

    //sphere
    rpr_shape mesh = CreateSphere(context, 64, 32, 1.f, float3());
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);
    status = rprShapeSetMaterial(mesh, layered);
    assert(status == RPR_SUCCESS);

    //light
    rpr_light light = NULL; status = rprContextCreateEnvironmentLight(context, &light);
    assert(status == RPR_SUCCESS);
    rpr_image imageInput = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/studio015.hdr", &imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light, imageInput);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    //camera
    rpr_camera camera = NULL; status = rprContextCreateCamera(context, &camera);
    assert(status == RPR_SUCCESS);
    status = rprCameraLookAt(camera, 0, 0, 3, 0, 0, 0, 0, 1, 0);
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

    //setup out
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
    status = rprFrameBufferSaveToFile(frame_buffer, "Output/UpdateMaterial_1.jpg");

    //update ior
    ior = 1.f;
    status = rprMaterialNodeSetInputF(fresnel, "ior", ior, ior, ior, ior);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    for (int i = 0; i < kRenderIterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    status = rprFrameBufferSaveToFile(frame_buffer, "Output/UpdateMaterial_2.jpg");

    assert(status == RPR_SUCCESS);
    rpr_render_statistics rs;
    status = rprContextGetInfo(context, RPR_CONTEXT_RENDER_STATISTICS, sizeof(rpr_render_statistics), &rs, NULL);
    assert(status == RPR_SUCCESS);



    //cleanup
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
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
int main(int argc, char* argv[])
{
	MeshCreationTest();
    SimpleRenderTest();
    ComplexRenderTest();
    EnvLightClearTest();
    MemoryStatistics();
    DefaultMaterialTest();
    NullShaderTest();
	TiledRender();
    AOVTest();
    test_feature_cameraDOF();
    test_feature_ContextImageFromData();
//    test_feature_multiUV();
//    test_apiMecha_Light();
    test_feature_LightDirectional();
    InstancingTest();
    BumpmapTest();
    test_feature_shaderBumpmap();
    test_feature_shaderTypeLayered();
    UpdateMaterial();
	//LoadFrs("../Resources/frs/bath_new.frs");
    
    return 0;
}