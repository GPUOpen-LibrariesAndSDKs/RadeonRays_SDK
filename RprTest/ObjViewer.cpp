#include <stdio.h>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <cassert>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/GLUT.h>
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include "GL/glew.h"
#include "GLUT/GLUT.h"
#else
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glx.h>
#endif
//#include <GL/glfw3.h>
//#include <glm/glm.hpp>
//#include <glm/ext.hpp>
#include "../Rpr/RadeonProRender.h"
#include "../RadeonRays/include/math/matrix.h"
#include "../RadeonRays/include/math/mathutils.h"

//#include <Rpr/RadeonProRender_GL.h>
//#include <Rpr/Math/mathutils.h>
#include "../App/tiny_obj_loader.h"

#include "../RprLoadStore/RprLoadStore.h"
#include <random>
#include <algorithm>

using namespace RadeonRays;

#define FRAMEBUFFER_WIDTH   1024
#define FRAMEBUFFER_HEIGHT  1024

const char* VERTEX_SHADER = "\
    #version 450\n\
    layout (location = 0) in vec3 in_Position;\n\
    \n\
    out VS_OUT\n\
    {\n\
        vec2 Texcoord;\n\
    } vs_out;\n\
    \n\
    void main()\n\
    {\n\
        vs_out.Texcoord = in_Position.xy * 0.5 + 0.5;\n\
        gl_Position = vec4(in_Position, 1.0);\n\
    }\n\
    ";

const char *FRAGMENT_SHADER = "\
    #version 450\n\
    layout (binding = 0) uniform sampler2D tex;\n\
    layout (location = 1) uniform int layer;\n\
    layout (location = 2) uniform int mipLevel;\n\
    layout (location = 0) out vec4 FragColor;\n\
    in VS_OUT\n\
    {\n\
        vec2 Texcoord;\n\
    } ps_in;\n\
    \n\
    vec3 ToneMapping(vec3 color)\n\
    {\n\
        color = max(vec3(0.), color - vec3(0.004));\n\
        color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);\n\
        return color;\n\
    }\n\
    \n\
    void main()\n\
    {\n\
        vec3 color = texture(tex, vec2(ps_in.Texcoord.x, 1.0f - ps_in.Texcoord.y)).xyz; \n\
        FragColor = vec4(color, 1.0f);\n\
    }\n\
    ";

rpr_context              g_context;
rpr_material_system      g_matSys;
rpr_scene                g_scene;
rpr_light                g_envLight;
rpr_framebuffer          g_frameBuffer;
rpr_camera               g_camera;

GLuint g_program;
GLuint g_vao;
GLuint g_texture;

int g_width = 640;
int g_height = 480;

matrix g_rotationMatrix;

struct
{
    bool left_button_pressed;
    bool right_button_pressed;
    int x;
    int y;
} g_mouse;
struct SphericalCamControls
{
	SphericalCamControls()
		:	m_rotX		(1.8f), 
			m_rotY		(1.8f),
			m_magnitude	(5.f),
			m_posX		(0.f),
			m_posY		(0.f),
			m_posZ		(0.f)
	{
	}

	float m_rotX;
	float m_rotY;
	float m_magnitude;

	float m_posX;
	float m_posY;
	float m_posZ;
};

SphericalCamControls g_cameraParams;

// Structure for override material
struct RasterMaterialProperty
{
    std::string name;
    std::string type;
    float x, y, z;
    std::string value;
};

struct RasterMaterial
{
    std::vector<RasterMaterialProperty> properties;
};


std::map<std::string, RasterMaterial> LoadOverrideMaterials(const char* filename)
{
    // Open file.
    std::ifstream file(filename);

    // Ignore header.
    int materialCount, paramCount;
    file >> materialCount >> paramCount;

    // Read in all materials.
    std::map<std::string, RasterMaterial> result;
    for (int i = 0; i < materialCount; ++i)
    {
        RasterMaterial mat;

        std::string name;
        std::getline(file, name);
        std::getline(file, name);

        for (int j = 0; j < paramCount; ++j)
        {
            RasterMaterialProperty prop;
            file >> prop.name >> prop.type;

            prop.name = "raster." + prop.name;

            if (prop.type == "float")
                file >> prop.x >> prop.y >> prop.z;
            else if (prop.type == "texture")
                std::getline(file, prop.value);

            mat.properties.push_back(prop);
        }

        result[name] = mat;
    }

    return result;
}


void InitGraphics(int argc, char** argv)
{
    int status;

    // Compile GLSL shaders.
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar**)&VERTEX_SHADER, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        int length;
        char log[1024];
        glGetShaderInfoLog(vertexShader, 1024, &length, log);
        std::cerr << "Error compiling vertexShader\n" << log << "\n";
        system("pause");
        exit(1);
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar**)&FRAGMENT_SHADER, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        int length;
        char log[1024];
        glGetShaderInfoLog(fragmentShader, 1024, &length, log);
        std::cerr << "Error compiling fragmentShader\n" << log << "\n";
        system("pause");
        exit(1);
    }

    g_program = glCreateProgram();
    glAttachShader(g_program, vertexShader);
    glAttachShader(g_program, fragmentShader);
    glLinkProgram(g_program);
    glGetProgramiv(g_program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        int length;
        char log[1024];
        glGetProgramInfoLog(g_program, 1024, &length, log);
        std::cerr << "Error linking program:\n" << log << "\n";
        system("pause");
        exit(1);
    }

    // Create vao for fullscreen quad.
    float vertices[] =
    {
        -1, 1, 0,
        -1, -1, 0,
        1, 1, 0,
        1, -1, 0
    };

#ifdef __APPLE__
    glGenVertexArraysAPPLE(1, &g_vao);
    glBindVertexArrayAPPLE(g_vao);
#else
    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);
#endif

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 3 * 4, vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

#ifdef __APPLE__
    glBindVertexArrayAPPLE(0);
#else
    glBindVertexArray(0);
#endif
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Allocate a texture to be used as framebuffer target.
    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glGetError();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    //glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA32F, FRAMEBUFFER_WIDTH, FRAMEBUFFER_HEIGHT);
    GLenum error = glGetError();
    assert(error == GL_NO_ERROR);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Set default state.
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}

void InitFR(int argc, char** argv)
{
    // Register plugins with FireRender.
    //TODO: plugins not implemented
//    std::vector<rpr_int> plugins;
//#ifndef _DEBUG
//    plugins.push_back(frRegisterPlugin("Raster64.dll"));
//#else
//    plugins.push_back(rprRegisterPlugin("Raster64D.dll"));
//#endif
//    for (int i = 0; i < plugins.size(); ++i)
//    {
//        if (plugins[i] == -1)
//        {
//            std::cout << "Error registering plugin index " << i << "\n";
//            exit(-1);
//        }
//    }

    // Create FireRender context.
    //rpr_int status = rprCreateContext(RPR_API_VERSION, &plugins[0], plugins.size(), RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &g_context);
    //assert(status == RPR_SUCCESS);

    //TODO: plugins not implemented
    // Set active plugin.
    //status = rprContextSetActivePlugin(g_context, plugins[0]);
    //assert(status == RPR_SUCCESS);

    rpr_int status = rprCreateContext(RPR_API_VERSION, nullptr, 0, RPR_CREATION_FLAGS_ENABLE_GPU0, NULL, NULL, &g_context);
    assert(status == RPR_SUCCESS);
    // Create material system.
    status = rprContextCreateMaterialSystem(g_context, 0, &g_matSys);
    assert(status == RPR_SUCCESS);

    // Create a scene.
    status = rprContextCreateScene(g_context, &g_scene);
    assert(status == RPR_SUCCESS);

    status = rprContextSetScene(g_context, g_scene);
    assert(status == RPR_SUCCESS);

    // Create a camera.    
    status = rprContextCreateCamera(g_context, &g_camera);
    assert(status == RPR_SUCCESS);

    // Position camera in world space.    
    status = rprCameraLookAt(g_camera, 0, 1, 5, 0, 1, 0, 0, 1, 0);
    assert(status == RPR_SUCCESS);

    // Attach camera to scene.
    status = rprSceneSetCamera(g_scene, g_camera);
    assert(status == RPR_SUCCESS);

    // Create image-based environment light.
   /* rpr_light ibl;
    status = frContextCreateEnvironmentLight(g_context, &ibl);
    assert(status == RPR_SUCCESS);*/

    //// Set an image for the light to take the radiance values from
    //rpr_image img;
    //status = frContextCreateImageFromFile(g_context, "ibl.jpg", &img);
    //assert(status == RPR_SUCCESS);

    //status = frEnvironmentLightSetImage(ibl, img);
    //assert(status == RPR_SUCCESS);

    //// Set IBL as a background for the scene
    //status = frSceneSetEnvironmentLight(g_scene, ibl);
    //assert(status == RPR_SUCCESS);

    // Create framebuffer target.
    //TODO: creation from gl buffer not implemented
    //status = rprContextCreateFramebufferFromGLTexture2D(g_context, GL_TEXTURE_2D, 0, g_texture, &g_frameBuffer);
    rpr_framebuffer_desc desc;
    desc.fb_width = g_width;
    desc.fb_height = g_height;
    rpr_framebuffer_format fmt = { 4, RPR_COMPONENT_TYPE_FLOAT32 };
    status = rprContextCreateFrameBuffer(g_context, fmt, &desc, &g_frameBuffer);
    assert(status == RPR_SUCCESS);

    status = rprFrameBufferClear(g_frameBuffer);
    assert(status == RPR_SUCCESS);

    status = rprContextSetAOV(g_context, RPR_AOV_COLOR, g_frameBuffer);
    assert(status == RPR_SUCCESS);

    // Set context properties.
   // frContextSetParameter1u(g_context, "rendermode", RPR_RENDER_MODE_NORMAL);
    /*frContextSetParameter1f(g_context, "aacellsize", 8.0f);
    frContextSetParameter1f(g_context, "aasamples", 8.0f);*/
    //frContextSetParameter1u(g_context, "raster.shadows.filter", RPR_RASTER_SHADOW_MAPS_PCF);

#if 0
    // Create image-based environment light
    rpr_light ibl;
    status = frContextCreateEnvironmentLight(g_context, &ibl);
    assert(status == RPR_SUCCESS);

    // Set an image for the light to take the radiance values from
    rpr_image img2;
    status = frContextCreateImageFromFile(g_context, "ibl.png", &img2);
    assert(status == RPR_SUCCESS);

    status = frEnvironmentLightSetImage(ibl, img2);
    assert(status == RPR_SUCCESS);

    // Set IBL as a background for the scene
    //status = frSceneSetEnvironmentLight(scene, ibl);
    status = frSceneAttachLight(g_scene, ibl);
    assert(status == RPR_SUCCESS);
#endif

    float identity[16] = {
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1,
    };
    memcpy(&g_rotationMatrix.m[0][0], identity, sizeof(float) * 16);
}

void LoadSceneFromFile(int argc, char** argv)
{
	std::string fileToLoad = //"C:/Users/Sean/Downloads/Synergy_test_01b.frs";
	   // "../Resources/Obj/bumpMapTest.obj";
		"../Resources/frs/bath_new.frs";
    if (argc == 2)
    {
        fileToLoad = argv[1];
    }

    if (fileToLoad.find(".frs") != std::string::npos)
    {
        //Load .frs file

        auto iter = fileToLoad.find(".frs");
        std::string fileWithoutExt = fileToLoad;
        fileWithoutExt.replace(iter, iter + 4, "");

        std::string fileMaterials = fileWithoutExt;
        fileMaterials.append(".frasm");

        // Create a scene
        rpr_scene scene = nullptr;
        // Load the materials override file.
        auto rasterMaterials = LoadOverrideMaterials(fileMaterials.c_str());

        // Load the serialized scene.
        int result = rprsImport(fileToLoad.c_str(), g_context, g_matSys, &scene, false);
        assert(result == 0);

        // Get the scene camera.
        rprSceneGetCamera(scene, &g_camera);

        // Enumerate all shapes in the scene.
        size_t shapeCount = 0;
        rpr_int status = rprSceneGetInfo(scene, RPR_SCENE_SHAPE_COUNT, sizeof(shapeCount), &shapeCount, NULL);
        assert(status == RPR_SUCCESS);

        rpr_shape* shapes = new rpr_shape[shapeCount];
        status = rprSceneGetInfo(scene, RPR_SCENE_SHAPE_LIST, shapeCount * sizeof(rpr_shape), shapes, NULL);
        assert(status == RPR_SUCCESS);

        // Iterate over shapes and replace materials.
        for (size_t i = 0; i < shapeCount; i++)
        {
            // Get the shape type.
            rpr_shape_type type;
            status = rprShapeGetInfo(shapes[i], RPR_SHAPE_TYPE, sizeof(rpr_shape_type), &type, NULL);
            assert(status == RPR_SUCCESS);

            // Only process meshes.
            if (type == RPR_SHAPE_TYPE_MESH)
            {
                // Retrieve the shape's material node.
                rpr_material_node mat = 0;
                status = rprShapeGetInfo(shapes[i], RPR_SHAPE_MATERIAL, sizeof(mat), &mat, NULL);
                assert(status == RPR_SUCCESS);

                // Get the material's name.
                std::string matName(1024, '\0');
                status = rprMaterialNodeGetInfo(mat, RPR_OBJECT_NAME, 1024, &matName[0], nullptr);
                assert(status == RPR_SUCCESS);

                // Look for name in raster materials.
                auto itr = rasterMaterials.find(matName.c_str());
                if (itr != rasterMaterials.end())
                {
                    printf("Found %s\n", matName.c_str());
                    RasterMaterial rm = itr->second;
                    for (auto& prop : rm.properties)
                    {
                        if (prop.type == "float")
                        {
                            printf("\t%s = (%f, %f, %f)\n", prop.name.c_str(), prop.x, prop.y, prop.z);
                            status = rprMaterialNodeSetInputF(mat, prop.name.c_str(), prop.x, prop.y, prop.z, 0.0f);
                            assert(status == RPR_SUCCESS);
                        }
                        else if (prop.type == "texture")
                        {
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Load the obj file from disk.
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string result = tinyobj::LoadObj(shapes, materials, fileToLoad.c_str(), "../Resources/Obj/");
        if (result != "")
        {
            std::cout << result;
            exit(-1);
        }

        std::random_device r;
        std::default_random_engine randomEngine(r());
        std::uniform_real_distribution<float> uniDistFloat(0.f, 1.f);

        // Loop over shapes from OBJ file.
        for (tinyobj::shape_t& shape : shapes)
        {
            if (shape.name.find("lightProbe") != std::string::npos) continue;
            // if (shape.name.find("light") != std::string::npos) continue;

            size_t numVertices = shape.mesh.positions.size() / 3;
            size_t numNormals = shape.mesh.normals.size() / 3;
            size_t numTexCoords = shape.mesh.texcoords.size() / 2;
            size_t numFaces = shape.mesh.indices.size() / 3;

            rpr_float* vertices = &shape.mesh.positions[0];
            rpr_float* normals = (numNormals > 0) ? &shape.mesh.normals[0] : nullptr;
            rpr_float* texCoords = (numTexCoords > 0) ? &shape.mesh.texcoords[0] : nullptr;

            size_t verticesStride = sizeof(float) * 3;
            size_t normalsStride = (numNormals > 0) ? sizeof(float) * 3 : 0;
            size_t texCoordsStride = (numTexCoords > 0) ? sizeof(float) * 2 : 0;

            std::vector<rpr_int> faces;
            faces.resize(numFaces, 3);

            // Create FireRender shape from mesh.
            rpr_shape out_shape;
            rpr_int status = rprContextCreateMesh(g_context,
                vertices, numVertices, verticesStride,
                normals, numNormals, normalsStride,
                texCoords, numTexCoords, texCoordsStride,
                (const rpr_int*)&shape.mesh.indices[0], sizeof(rpr_int),
                (numNormals > 0) ? (const rpr_int*)&shape.mesh.indices[0] : nullptr, sizeof(rpr_int),
                (numTexCoords > 0) ? (const rpr_int*)&shape.mesh.indices[0] : nullptr, sizeof(rpr_int),
                &faces[0], numFaces, &out_shape
            );
            assert(status == RPR_SUCCESS);

            // Get shape's material.
            tinyobj::material_t material = materials[shape.mesh.material_ids[0]];

            // Create FireRender material.
            rpr_material_node materialNode = nullptr;
            if (material.name == "light")
            {
                // Create FireRender material.            
                status = rprMaterialSystemCreateNode(g_matSys, RPR_MATERIAL_NODE_EMISSIVE, &materialNode);
                assert(status == RPR_SUCCESS);

                // Apply shape's material properties.
                status = rprMaterialNodeSetInputF(materialNode, "color", 3, 3, 3, 1);// material.emission[0], material.emission[1], material.emission[2], 1.0f);
                assert(status == RPR_SUCCESS);
                //status = frShapeSetVisibility(out_shape, RPR_FALSE);
                //assert(status == RPR_SUCCESS);
            }
            else
            {
                // Create FireRender material.            
                status = rprMaterialSystemCreateNode(g_matSys, RPR_MATERIAL_NODE_STANDARD, &materialNode);
                assert(status == RPR_SUCCESS);

                // Apply shape's material properties.
                //status = rprMaterialNodeSetInputF(materialNode, "diffuse.color", material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f);
                status = rprMaterialNodeSetInputF(materialNode, "color", material.diffuse[0], material.diffuse[1], material.diffuse[2], 1.0f);
                assert(status == RPR_SUCCESS);

                float ranNum = uniDistFloat(randomEngine);
                float invRan = uniDistFloat(randomEngine);

                // Apply raster material properties.
                //status = rprMaterialNodeSetInputF(materialNode, "glossy.color", 0.f, 1.f, 0.f, 1.f);
                //assert(status == RPR_SUCCESS);
                //status = rprMaterialNodeSetInputF(materialNode, "glossy.roughness_x", invRan, invRan, invRan, invRan);
                //assert(status == RPR_SUCCESS);
                status = rprMaterialNodeSetInputF(materialNode, "roughness", invRan, invRan, invRan, invRan);
                assert(status == RPR_SUCCESS);


                // Create FireRender material.
                rpr_material_node fresnel = NULL;
                status = rprMaterialSystemCreateNode(g_matSys, RPR_MATERIAL_NODE_FRESNEL, &fresnel);
                assert(status == RPR_SUCCESS);
                status = rprMaterialNodeSetInputF(fresnel, "ior", 1.33f, 1.33f, 1.33f, 1.33f);
                assert(status == RPR_SUCCESS);

                //status = frMaterialNodeSetInputN(materialNode, "weights.glossy2diffuse", fresnel);
                //status = rprMaterialNodeSetInputF(materialNode, "weights.glossy2diffuse", 0.1f, 0.1f, 0.1f, 1.f);
                assert(status == RPR_SUCCESS);

                if (!material.specular_texname.empty())
                {
                    rpr_image img = NULL;
                    std::string path = "../Resources/Textures/";
                    path.append(material.specular_texname);
                    status = rprContextCreateImageFromFile(g_context, path.c_str(), &img);
                    assert(status == RPR_SUCCESS);

                    rpr_material_node texture = NULL;
                    status = rprMaterialSystemCreateNode(g_matSys, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &texture);
                    assert(status == RPR_SUCCESS);

                    status = rprMaterialNodeSetInputImageData(texture, "data", img);
                    assert(status == RPR_SUCCESS);

                    status = rprMaterialNodeSetInputN(materialNode, "roughness", texture);
                    assert(status == RPR_SUCCESS);
                }

                if (!material.normal_texname.empty())
                {
                    rpr_image img = NULL;
                    std::string path = "../Resources/Textures/";
                    path.append(material.normal_texname);
                    status = rprContextCreateImageFromFile(g_context, path.c_str(), &img);
                    assert(status == RPR_SUCCESS);

                    rpr_material_node texture = NULL;
                    status = rprMaterialSystemCreateNode(g_matSys, RPR_MATERIAL_NODE_NORMAL_MAP, &texture);
                    assert(status == RPR_SUCCESS);

                    status = rprMaterialNodeSetInputImageData(texture, "data", img);
                    assert(status == RPR_SUCCESS);

                    status = rprMaterialNodeSetInputN(materialNode, "normal", texture);
                    assert(status == RPR_SUCCESS);
                }

                if (!material.diffuse_texname.empty())
                {
                    rpr_image img = NULL;
                    std::string path = "../Resources/Textures/";
                    path.append(material.diffuse_texname);
                    status = rprContextCreateImageFromFile(g_context, path.c_str(), &img);
                    assert(status == RPR_SUCCESS);

                    rpr_material_node texture = NULL;
                    status = rprMaterialSystemCreateNode(g_matSys, RPR_MATERIAL_NODE_IMAGE_TEXTURE, &texture);
                    assert(status == RPR_SUCCESS);

                    status = rprMaterialNodeSetInputImageData(texture, "data", img);
                    assert(status == RPR_SUCCESS);

                    status = rprMaterialNodeSetInputN(materialNode, "color", texture);
                    assert(status == RPR_SUCCESS);
                }

                //status = frShapeSetVisibility(out_shape, RPR_FALSE);
                //assert(status == RPR_SUCCESS);
            }

            // Set shape's material.
            status = rprShapeSetMaterial(out_shape, materialNode);
            assert(status == RPR_SUCCESS);

            // Attach shape to scene.
            status = rprSceneAttachShape(g_scene, out_shape);
            assert(status == RPR_SUCCESS);
        }

#if 0
        {
            glm::mat4 lightTransform = glm::lookAt(glm::vec3(0.0f, 10.5f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            rpr_light light;
            frContextCreateSpotLight(g_context, &light);
            frLightSetTransform(light, RPR_FALSE, glm::value_ptr(lightTransform));
            frSpotLightSetRadiantPower3f(light, 3, 3, 3);
            frSpotLightSetConeShape(light, 45 * 3.14159265 / 180.0, 120 * 3.14159265 / 180.0);
            frSceneAttachLight(g_scene, light);
    }
#endif

#if 0
        {
            glm::mat4 lightTransform = glm::lookAt(glm::vec3(0.0f, 10.5f, 5.0f), glm::vec3(0.01f, 0.0f, 0.01f), glm::vec3(1.0f, 0.0f, 0.0f));
            rpr_light light;
            frContextCreateDirectionalLight(g_context, &light);
            frLightSetTransform(light, RPR_FALSE, glm::value_ptr(lightTransform));
            frDirectionalLightSetRadiantPower3f(light, 3, 3, 3);
            frSceneAttachLight(g_scene, light);
        }
#endif

#if 0
        {
            glm::mat4 lightTransform = glm::translate(-glm::vec3(4.f, 8.f, 4.5f));
            rpr_light light;
            frContextCreatePointLight(g_context, &light);
            frLightSetTransform(light, RPR_FALSE, glm::value_ptr(lightTransform));
            frPointLightSetRadiantPower3f(light, 2, 2, 2);
            frSceneAttachLight(g_scene, light);
        }
#endif
    }
    rpr_int status = RPR_SUCCESS;
    status = rprContextCreateEnvironmentLight(g_context, &g_envLight);
    assert(status == RPR_SUCCESS);
    rpr_image imageInput = NULL; status = rprContextCreateImageFromFile(g_context, "../Resources/Textures/Studio015.hdr", &imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetImage(g_envLight, imageInput);
    assert(status == RPR_SUCCESS);
    status = rprEnvironmentLightSetIntensityScale(g_envLight, 5.f);
    assert(status == RPR_SUCCESS);
    status = rprSceneAttachLight(g_scene, g_envLight);
    assert(status == RPR_SUCCESS);
}

void SphericalToCartesianCoordinates(float _magnitude, float _rotX, float _rotY, float _posX, float _posY, float _posZ, float& _outX, float& _outY, float& _outZ) 
{
#define HALFPI 1.57079632679f


	_outX	= _magnitude * std::sin(_rotY) * std::cos(_rotX );
	_outY	= _magnitude * std::cos(_rotY); 
	_outZ	= _magnitude * std::sin(_rotY) * std::sin(_rotX);

	_outX	+= _posX;
	_outY	+= _posY;
	_outZ	+= _posZ;
}

float3 GetGlobalCameraPosition()
{
	float x, y, z;
	SphericalToCartesianCoordinates( g_cameraParams.m_magnitude	, g_cameraParams.m_rotX, g_cameraParams.m_rotY, 
									 g_cameraParams.m_posX		, g_cameraParams.m_posY, g_cameraParams.m_posZ,
									 x,y,z);

	return float3(x, y, z);

}

void mouse_click(int button, int state, int x, int y)
{
    bool pressed = true;
    if (state == GLUT_DOWN)
    {
        g_mouse.x = x;
        g_mouse.y = y;
        pressed = true;
    }

    if (state == GLUT_UP)
    {
        pressed = false;
    }

    if (button == GLUT_LEFT_BUTTON)
    {
        g_mouse.left_button_pressed = pressed;
    }
    if (button == GLUT_RIGHT_BUTTON)
    {
        g_mouse.right_button_pressed = pressed;
    }
}
void mouse_move(int x, int y)
{
    int dx = x - g_mouse.x;
    int dy = y - g_mouse.y;
    g_mouse.x = x;
    g_mouse.y = y;

    if (g_mouse.left_button_pressed)
    {
        g_cameraParams.m_rotX += dx * 0.01f;
        g_cameraParams.m_rotY -= dy * 0.01f;
    }
    else if (g_mouse.right_button_pressed)
    {
        g_cameraParams.m_magnitude += dy * 0.1f;
    }
    // Position camera in world space.    

    float3 cameraPos = GetGlobalCameraPosition();
    float3 cameraCentre = float3(g_cameraParams.m_posX, g_cameraParams.m_posY, g_cameraParams.m_posZ);

    rpr_int status = rprCameraLookAt(g_camera, cameraPos.x, cameraPos.y, cameraPos.z,
        g_cameraParams.m_posX, g_cameraParams.m_posY, g_cameraParams.m_posZ,
        0, 1, 0);
    assert(status == RPR_SUCCESS);
    status = rprFrameBufferClear(g_frameBuffer);
    assert(status == RPR_SUCCESS);
}
void keyCallback(unsigned char key, int x, int y)
{
	float speed = 0.01f;

    {
        //TODO: rprContextSetParameter1u not implemented
        if (key == '1') rprContextSetParameter1u(g_context, "raster.shadows.filter", RPR_RASTER_SHADOWS_FILTER_NONE);
        else if (key == '2') rprContextSetParameter1u(g_context, "raster.shadows.filter", RPR_RASTER_SHADOWS_FILTER_PCF);
        else if (key == '3') rprContextSetParameter1u(g_context, "raster.shadows.filter", RPR_RASTER_SHADOWS_FILTER_PCSS);
        else if (key == '4') rprContextSetParameter1u(g_context, "raster.shadows.resolution", 32U);
        else if (key == '5') rprContextSetParameter1u(g_context, "raster.shadows.resolution", 128U);
        else if (key == '6') rprContextSetParameter1u(g_context, "raster.shadows.resolution", 256U);
		else if (key == 'A') g_cameraParams.m_rotX -= speed;
		else if (key == 'D') g_cameraParams.m_rotX += speed;
		else if (key == 'W') g_cameraParams.m_rotY -= speed;
		else if (key == 'S') g_cameraParams.m_rotY += speed;
		else if (key == 'Q') g_cameraParams.m_magnitude -= 10.f * speed;
		else if (key == 'E') g_cameraParams.m_magnitude += 10.f * speed;
    }
}

void Render()
{
    // Render scene.
    rprContextRender(g_context);

    // Clear backbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, g_width, g_height);
    glClearColor(1, 1, 1, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Bind and setup program.
    glUseProgram(g_program);

    // Bind input texture.
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    

    //copy data
    rpr_int status = RPR_SUCCESS;
    size_t frame_buffer_dataSize = 0;
    status = rprFrameBufferGetInfo(g_frameBuffer, RPR_FRAMEBUFFER_DATA, 0 , NULL , &frame_buffer_dataSize );
    assert(status == RPR_SUCCESS);
    std::vector<float3> buf_data(frame_buffer_dataSize/sizeof(float3));
    std::vector<unsigned char> tex_data(buf_data.size() * 4);

    status = rprFrameBufferGetInfo(g_frameBuffer, RPR_FRAMEBUFFER_DATA, frame_buffer_dataSize , buf_data.data(), NULL );
    assert(status == RPR_SUCCESS);
    float gamma = 2.2f;
    for (int i = 0; i < (int)buf_data.size(); ++i)
    {
        tex_data[4 * i] = (unsigned char)clamp(clamp(pow(buf_data[i].x / buf_data[i].w, 1.f / gamma), 0.f, 1.f) * 255, 0, 255);
        tex_data[4 * i + 1] = (unsigned char)clamp(clamp(pow(buf_data[i].y / buf_data[i].w, 1.f / gamma), 0.f, 1.f) * 255, 0, 255);
        tex_data[4 * i + 2] = (unsigned char)clamp(clamp(pow(buf_data[i].z / buf_data[i].w, 1.f / gamma), 0.f, 1.f) * 255, 0, 255);
        tex_data[4 * i + 3] = 255;
    }
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_width, g_height, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
    assert(glGetError() == GL_NO_ERROR);

    // Render full screen quad.
#ifdef __APPLE__
    glBindVertexArrayAPPLE(g_vao);
#else
    glBindVertexArray(g_vao);
#endif
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindTexture(GL_TEXTURE_2D, 0);
    glFinish();
    glutSwapBuffers();
}

void RunObjViewer(int argc, char** argv)
{
    // GLUT Window Initialization:
    glutInit(&argc, (char**)argv);
    glutInitWindowSize(640, 480);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("ObjViewer");
#ifndef __APPLE__
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cout << "GLEW initialization failed\n";
        return;
    }
#endif
    glutKeyboardFunc(keyCallback);
    glutMouseFunc(mouse_click);
    glutMotionFunc(mouse_move);

    // Initialize OpenGL state and objects.
    InitGraphics(argc, argv);

    // Initialize FireRender.
    InitFR(argc, argv);

    // Load obj file from disk.
    LoadSceneFromFile(argc, argv);

    // Loop until window is closed.
    glutIdleFunc(Render);
    glutMainLoop();
}
