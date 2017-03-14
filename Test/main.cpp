#include "../Rpr/RadeonProRender.h"
#include "../RadeonRays/include/math/matrix.h"
#include "../RadeonRays/include/math/mathutils.h"
#include <cassert>
#include <fstream>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <iostream>

using namespace RadeonRays;

#define FR_MACRO_CLEAN_SHAPE_RELEASE(mesh1__, scene1__)\
	status = rprShapeSetMaterial(mesh1__, NULL);\
	assert(status == RPR_SUCCESS);\
	status = rprSceneDetachShape(scene1__, mesh1__);\
	assert(status == RPR_SUCCESS);\
	status = rprObjectDelete(mesh1__); mesh1__ = NULL;\
	assert(status == RPR_SUCCESS);

#define FR_MACRO_SAFE_FRDELETE(a)    { status = rprObjectDelete(a); a = NULL;   assert(status == RPR_SUCCESS); }

void InitGl()
{
    //g_shader_manager.reset(new ShaderManager());
    //glClearColor(0.0, 0.0, 0.0, 0.0);

    //glCullFace(GL_FRONT);
    //glDisable(GL_DEPTH_TEST);
    //glEnable(GL_TEXTURE_2D);
    //glGenBuffers(1, &g_vertex_buffer);
    //glGenBuffers(1, &g_index_buffer);
    //// create Vertex buffer
    //glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer);
    //// fill data
    //glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertices), g_vertices, GL_STATIC_DRAW);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(g_indices), g_indices, GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER, 0);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    ////texture
    //glGenTextures(1, &g_texture);
    //glBindTexture(GL_TEXTURE_2D, g_texture);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_window_width, g_window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    //glBindTexture(GL_TEXTURE_2D, 0);

    // GLUT Window Initialization:

}

void Test1()
{
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
        status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CONTEXT_GPU0_NAME, NULL, NULL, &context);
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
}

void TestRender()
{
    //when reference image is validated, add this line :
    //referenceImageType = COMPARE_WITH_REFERENCE;

    rpr_int status = RPR_SUCCESS;
    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CONTEXT_GPU0_NAME, NULL, NULL, &context);
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

    rpr_material_node diffuse = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuse);
    assert(status == RPR_SUCCESS);

    status = rprMaterialNodeSetInputF(diffuse, "color", 0.7f, 0.7f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);

    size_t num_vertices, num_normals, num_texcoords, num_faces;
    rpr_int vertex_stride, normal_stride, texcoord_stride;
    std::ifstream fs("../Resources/Obj/sphere.bin", std::ios::in | std::ios::binary);
    assert(fs);

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
    int check = 0;
    for (int i : nfv)
        check += i;
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


    matrix m = translation(float3(0, 1, 0));

    //status = rprShapeSetTransform(mesh, true, &m.m00);
    //assert(status == RPR_SUCCESS);



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

    status = rprShapeSetMaterial(plane_mesh, diffuse);
    assert(status == RPR_SUCCESS);

    rpr_light light = NULL; status = rprContextCreateSpotLight(context, &light);
    assert(status == RPR_SUCCESS);
    //rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    //assert(status == RPR_SUCCESS);
    //status = rprPointLightSetRadiantPower3f(light, 350, 350, 350);
    //assert(status == RPR_SUCCESS);

    matrix lightm = translation(float3(0, 16, 0)) * rotation_x(M_PI_2);

    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);

    status = rprSpotLightSetConeShape(light, M_PI_4, M_PI * 2.f / 3.f);
    assert(status == RPR_SUCCESS);

    status = rprSpotLightSetRadiantPower3f(light, 350, 350, 350);
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

    status = rprFrameBufferClear(frame_buffer);  assert(status == RPR_SUCCESS);

    int m_maxIterationRendering = 100;
    for (int i = 0; i < m_maxIterationRendering; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    status = rprFrameBufferSaveToFile(frame_buffer, "1.jpg");
    assert(status == RPR_SUCCESS);

    status = rprSpotLightSetConeShape(light, M_PI_4 * 0.5f, M_PI_4 * 0.5f);
    assert(status == RPR_SUCCESS);

    status = rprFrameBufferClear(frame_buffer);  assert(status == RPR_SUCCESS);

    for (int i = 0; i < m_maxIterationRendering; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }
    status = rprFrameBufferSaveToFile(frame_buffer, "2.jpg");
    assert(status == RPR_SUCCESS);
    rpr_render_statistics rs;
    rprContextGetInfo(context, RPR_CONTEXT_RENDER_STATISTICS, sizeof(rpr_render_statistics), &rs, NULL);

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

void ComplexRender()
{
    rpr_int status = RPR_SUCCESS;

    rpr_context	context;
    status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CONTEXT_GPU0_NAME, NULL, NULL, &context);
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


    //status = rprShapeSetTransform(mesh, true, &m1.m00);
    //assert(status == RPR_SUCCESS);

    status = rprSceneAttachShape(scene, mesh);
    assert(status == RPR_SUCCESS);

    //status = rprSceneAttachShape(scene, mesh1);
    //assert(status == RPR_SUCCESS);

    status = rprSceneAttachShape(scene, plane_mesh);
    assert(status == RPR_SUCCESS);

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


    rpr_material_node diffuse = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_DIFFUSE, &diffuse);
    assert(status == RPR_SUCCESS);

    rpr_material_node mfc = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_MICROFACET, &mfc);
    assert(status == RPR_SUCCESS);

    status = rprMaterialNodeSetInputF(diffuse, "color", 0.9f, 0.9f, 0.9f, 1.0f);
    assert(status == RPR_SUCCESS);


    //rpr_image img = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/scratched.png", &img);
    //assert(status == RPR_SUCCESS);
    //status = rprMaterialNodeSetInputImageData(diffuse, "color", img);
    //assert(status == RPR_SUCCESS);
    //status = rprMaterialNodeSetInputImageData(mfc, "color", img);
    //assert(status == RPR_SUCCESS);
    rpr_image img = NULL; status = rprContextCreateImageFromFile(context, "../Resources/Textures/scratched.png", &img);
    assert(status == RPR_SUCCESS);
    rpr_material_node materialNodeTexture = NULL; status = rprMaterialSystemCreateNode(matsys, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &materialNodeTexture);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputImageData(materialNodeTexture, "data", img);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(diffuse, "color", materialNodeTexture);
    assert(status == RPR_SUCCESS);
    status = rprMaterialNodeSetInputN(mfc, "color", materialNodeTexture);
    assert(status == RPR_SUCCESS);




    status = rprMaterialNodeSetInputF(mfc, "ior", 1.3f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);

    status = rprMaterialNodeSetInputF(mfc, "roughness", 1.5f, 0.0f, 0.0f, 0.0f);
    assert(status == RPR_SUCCESS);

    status = rprShapeSetMaterial(mesh, diffuse);
    assert(status == RPR_SUCCESS);

    //status = rprShapeSetMaterial(mesh1, diffuse);
    //assert(status == RPR_SUCCESS);

    status = rprShapeSetMaterial(plane_mesh, diffuse);
    assert(status == RPR_SUCCESS);

    rpr_light light = NULL; status = rprContextCreatePointLight(context, &light);
    assert(status == RPR_SUCCESS);


    matrix lightm = translation(float3(0, 6, 0)) * rotation_z(3.14f);

    status = rprLightSetTransform(light, true, &lightm.m00);
    assert(status == RPR_SUCCESS);

    status = rprPointLightSetRadiantPower3f(light, 100, 100, 100);
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

    status = rprFrameBufferClear(frame_buffer);
    assert(status == RPR_SUCCESS);
    int m_maxIterationRendering = 1;
    for (int i = 0; i < m_maxIterationRendering; ++i)
    {
        status = rprContextRender(context);
        assert(status == RPR_SUCCESS);
    }

    status = rprFrameBufferSaveToFile(frame_buffer, "ComplexRender.jpg");
    assert(status == RPR_SUCCESS);

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
    rprObjectDelete(mfc);
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
    glutInit(&argc, (char**)argv);
    glutInitWindowSize(640, 480);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("Triangle");
#ifndef __APPLE__
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cout << "GLEW initialization failed\n";
        return -1;
    }
#endif

    //Test1();
    //TestRender();
    ComplexRender();

    return 0;
}