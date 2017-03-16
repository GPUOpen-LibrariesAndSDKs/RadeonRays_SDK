#include "../Rpr/RadeonProRender.h"
#include "../RadeonRays/include/math/matrix.h"
#include "../RadeonRays/include/math/mathutils.h"

#include <cassert>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <iostream>

using namespace RadeonRays;

namespace
{
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
    int render_iterations = 100;
    for (int i = 0; i < render_iterations; ++i)
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
    
    for (int i = 0; i < render_iterations; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    status = rprFrameBufferSaveToFile(frame_buffer, "SimpleRenderTest_f2.jpg");
    assert(status == RPR_SUCCESS);
    //rpr_render_statistics rs;
    //status = rprContextGetInfo(context, RPR_CONTEXT_RENDER_STATISTICS, sizeof(rpr_render_statistics), &rs, NULL);
    //assert(status == RPR_SUCCESS);

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

    //TODO add instance
    //rpr_shape mesh1 = NULL; status = rprContextCreateInstance(context, mesh, &mesh1);
    //assert(status == RPR_SUCCESS);

    matrix m = translation(float3(-2, 1, 0));

    //status = rprShapeSetTransform(mesh1, true, &m.m00);
    //assert(status == RPR_SUCCESS);

    matrix m1 = translation(float3(2, 1, 0)) * rotation_y(0.5);
    status = rprShapeSetTransform(mesh, true, &m1.m00);
    assert(status == RPR_SUCCESS);

    //attach shapes
    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);
    //status = rprSceneAttachShape(scene, mesh1);
    //assert(status == RPR_SUCCESS);
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
    //status = rprShapeSetMaterial(mesh1, diffuse);
    //assert(status == RPR_SUCCESS);
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
    int m_maxIterationRendering = 100;
    for (int i = 0; i < m_maxIterationRendering; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    status = rprFrameBufferSaveToFile(frame_buffer, "ComplexRender.jpg");
    assert(status == RPR_SUCCESS);

    //cleanup
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    //FR_MACRO_CLEAN_SHAPE_RELEASE(mesh1, scene);
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

    rpr_material_node diffuse = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuse);
    assert(status == RPR_SUCCESS);

    //rpr_material_node diffuseSphere = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuseSphere);
    //assert(status == RPR_SUCCESS);

    rpr_material_node specularSphere = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_REFLECTION, &specularSphere);
    assert(status == RPR_SUCCESS);

    status = rprMaterialNodeSetInputF(diffuse, "color", 0.7f, 0.7f, 0.7f, 1.0f);
    assert(status == RPR_SUCCESS);

    //status = rprMaterialNodeSetInputF(diffuseSphere, "color", 0.7f, 0.8f, 0.2f, 1.0f);
    //assert(status == RPR_SUCCESS);

    //status = rprMaterialNodeSetInputF(diffuseSphere, "roughness", 2.25f);
    //assert(status == RPR_SUCCESS);

    status = rprMaterialNodeSetInputF(specularSphere, "color", 1.0f, 1.0f, 1.0f, 1.0f);
    assert(status == RPR_SUCCESS);

    //rpr_material_node layered = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_BLEND, &layered);
    //assert(status == RPR_SUCCESS);

    //status = rprMaterialNodeSetInputN(layered, "color1", specularSphere);
    //assert(status == RPR_SUCCESS);

    // status = rprMaterialNodeSetInputN(layered, "color0", diffuseSphere);
    //assert(status == RPR_SUCCESS);

    //status = rprMaterialNodeSetInputF(layered, "baseior", 2.25f);
    //assert(status == RPR_SUCCESS);

    // status = rprMaterialNodeSetInputF(layered, "topior", 2.2f);
    //assert(status == RPR_SUCCESS);

    size_t num_vertices, num_normals, num_texcoords, num_faces;
    rpr_int vertex_stride, normal_stride, texcoord_stride;
    std::ifstream fs("../Resources/Obj/sphere.bin", std::ios::in | std::ios::binary);


    fs.read((char*)&num_vertices, sizeof(size_t));
    fs.read((char*)&num_normals, sizeof(size_t));
    fs.read((char*)&num_texcoords, sizeof(size_t));
    fs.read((char*)&vertex_stride, sizeof(rpr_int));
    fs.read((char*)&normal_stride, sizeof(rpr_int));
    fs.read((char*)&texcoord_stride, sizeof(rpr_int));

    std::vector<rpr_float> vertices(num_vertices * 3);
    std::vector<rpr_float> normals(num_normals * 3);
    std::vector<rpr_float> texcoords(num_texcoords * 2);

    fs.read((char*)&vertices[0], vertex_stride * num_vertices);
    fs.read((char*)&normals[0], normal_stride * num_normals);
    fs.read((char*)&texcoords[0], texcoord_stride * num_texcoords);

    fs.read((char*)&num_faces, sizeof(size_t));

    std::vector<rpr_int > nfv(num_faces);
    fs.read((char*)&nfv[0], num_faces * sizeof(rpr_int));

    std::vector<rpr_int> vertex_indices(4 * num_faces);
    fs.read((char*)&vertex_indices[0], 4 * num_faces * sizeof(rpr_int));
    fs.close();

    rpr_shape mesh = NULL; status = rprContextCreateMesh(context,
        (rpr_float const*)&vertices[0], num_vertices, vertex_stride,
        (rpr_float const*)&normals[0], num_normals, normal_stride,
        (rpr_float const*)&texcoords[0], num_texcoords, texcoord_stride,
        (rpr_int const*)&vertex_indices[0], sizeof(rpr_int),
        (rpr_int const*)&vertex_indices[0], sizeof(rpr_int),
        (rpr_int const*)&vertex_indices[0], sizeof(rpr_int),
        &nfv[0], num_faces, &mesh);

    assert(status == RPR_SUCCESS);

    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);

    status = rprShapeSetMaterial(mesh, diffuse);
    assert(status == RPR_SUCCESS);

    matrix m = translation(float3(0, 1, 0)) * scale(float3(0.2, 0.2, 0.2));

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

    status = rprSceneAttachShape(scene, plane_mesh);
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

    status = rprContextSetScene(context, scene);
    assert(status == RPR_SUCCESS);

    status = rprShapeSetMaterial(plane_mesh, diffuse);
    assert(status == RPR_SUCCESS);

    rpr_light light = NULL; status = rprContextCreateEnvironmentLight(context, &light);
    assert(status == RPR_SUCCESS);

    rpr_image imageInput = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/Studio015.hdr", &imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(light, imageInput);
    assert(status == RPR_SUCCESS);


    status = rprEnvironmentLightSetIntensityScale(light, 5.f);
    assert(status == RPR_SUCCESS);

    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    rpr_framebuffer_desc desc;
    desc.fb_width = 800;
    desc.fb_height = 600;

    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    rpr_framebuffer frame_buffer = NULL; status = rprContextCreateFrameBuffer(context, fmt, &desc, &frame_buffer);
    assert(status == RPR_SUCCESS);

    status = rprContextSetAOV(context, RPR_AOV_COLOR, frame_buffer);
    assert(status == RPR_SUCCESS);

    matrix lightm = rotation_y(M_PI);

    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);

    status = rprFrameBufferClear(frame_buffer);  assert(status == RPR_SUCCESS);

    int m_maxIterationRendering = 10;
    for (int i = 0; i < m_maxIterationRendering; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "EnvLightClear_Stage1.png");

    lightm = rotation_y(-M_PI_2);

    rprSceneClear(scene);

    status = rprFrameBufferClear(frame_buffer);  assert(status == RPR_SUCCESS);

    rprFrameBufferSaveToFile(frame_buffer, "EnvLightClear_Stage2.png");

    status = rprSceneAttachShape(scene, plane_mesh);
    assert(status == RPR_SUCCESS);

    status = rprFrameBufferClear(frame_buffer);  assert(status == RPR_SUCCESS);

    status = rprSceneAttachLight(scene, light);
    assert(status == RPR_SUCCESS);

    for (int i = 0; i < m_maxIterationRendering; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    rprFrameBufferSaveToFile(frame_buffer, "EnvLightClear_Stage3.png");

    status = rprObjectDelete(imageInput); imageInput = NULL;
    assert(status == RPR_SUCCESS);
    FR_MACRO_CLEAN_SHAPE_RELEASE(mesh, scene);
    FR_MACRO_CLEAN_SHAPE_RELEASE(plane_mesh, scene);
    status = rprSceneDetachLight(scene, light);
    assert(status == RPR_SUCCESS);
    status = rprObjectDelete(light); light = NULL;
    assert(status == RPR_SUCCESS);
    rprObjectDelete(diffuse);
    //rprObjectDelete(diffuseSphere);
    rprObjectDelete(specularSphere);
    //rprObjectDelete(layered);
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

    return 0;
}