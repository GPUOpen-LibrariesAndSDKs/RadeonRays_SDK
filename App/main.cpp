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
#include "OpenImageIO/imageio.h"

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
<<<<<<< HEAD
// #define GLFW_INCLUDE_GL3
// #define GLFW_NO_GLU
//#include "GLFW/glfw3.h"
=======
#define GLFW_INCLUDE_GLCOREARB
#define GLFW_NO_GLU
#include "GLFW/glfw3.h"
>>>>>>> github/baikal-next
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#else
#include <CL/cl.h>
#include <GL/glew.h>
#include <GL/glx.h>
#include "GLFW/glfw3.h"
#endif

#include "ImGUI/imgui.h"
#include "ImGUI/imgui_impl_glfw_gl3.h"

#include <memory>
#include <chrono>
#include <cassert>
#include <vector>
#include <stdexcept>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <numeric>
#include <thread>
#include <atomic>
#include <mutex>
#include <fstream>

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef RR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

#include "CLW.h"

#include "math/mathutils.h"

#include "Utils/tiny_obj_loader.h"
#include "SceneGraph/camera.h"
#include "Utils/shader_manager.h"
#include "SceneGraph/scene1.h"
#include "Renderers/PT/ptrenderer.h"
#include "Renderers/BDPT/bdptrenderer.h"
#include "CLW/clwoutput.h"
#include "Utils/config_manager.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/IO/scene_io.h"
#include "SceneGraph/IO/material_io.h"

#ifdef ENABLE_DENOISER
#include "PostEffects/bilateral_denoiser.h"
#endif

Baikal::Scene1 scene;

using namespace RadeonRays;

// Help message
char const* kHelpMessage =
"App [-p path_to_models][-f model_name][-b][-r][-ns number_of_shadow_rays][-ao ao_radius][-w window_width][-h window_height][-nb number_of_indirect_bounces]";
char const* g_path =
"../Resources/CornellBox";
char const* g_modelname = "orig.objm";
char const* g_envmapname = "../Resources/Textures/studio015.hdr";

std::unique_ptr<ShaderManager>    g_shader_manager;
std::unique_ptr<Baikal::PerspectiveCamera> g_camera;

GLuint g_vertex_buffer;
GLuint g_index_buffer;
GLuint g_texture;
GLuint g_vao;

int g_window_width = 512;
int g_window_height = 512;
int g_num_shadow_rays = 1;
int g_num_ao_rays = 1;
int g_ao_enabled = false;
int g_progressive = false;
int g_num_bounces = 5;
int g_num_samples = -1;
int g_samplecount = 0;
float g_ao_radius = 1.f;
float g_envmapmul = 1.f;
float g_cspeed = 10.25f;

float3 g_camera_pos = float3(0.f, 1.f, 3.f);
float3 g_camera_at = float3(0.f, 1.f, 0.f);
float3 g_camera_up = float3(0.f, 1.f, 0.f);

float2 g_camera_sensor_size = float2(0.036f, 0.024f);  // default full frame sensor 36x24 mm
float2 g_camera_zcap = float2(0.0f, 100000.f);
float g_camera_focal_length = 0.035f; // 35mm lens
float g_camera_focus_distance = 1.f;
float g_camera_aperture = 0.f;

bool g_recording_enabled = false;
int g_frame_count = 0;
bool g_benchmark = false;
bool g_interop = true;
bool g_gui_visible = true;
ConfigManager::Mode g_mode = ConfigManager::Mode::kUseSingleGpu;
Baikal::Renderer::OutputType g_ouput_type = Baikal::Renderer::OutputType::kColor;

using namespace tinyobj;

#define CHECK_GL_ERROR assert(glGetError() == 0)


struct OutputData
{
    Baikal::ClwOutput* output;

#ifdef ENABLE_DENOISER
    Baikal::ClwOutput* output_position;
    Baikal::ClwOutput* output_normal;
    Baikal::ClwOutput* output_albedo;
    Baikal::ClwOutput* output_denoised;
    Baikal::BilateralDenoiser* denoiser;
#endif

    std::vector<float3> fdata;
    std::vector<unsigned char> udata;
    CLWBuffer<float3> copybuffer;
};

struct ControlData
{
    std::atomic<int> clear;
    std::atomic<int> stop;
    std::atomic<int> newdata;
    std::mutex datamutex;
    int idx;
};

std::vector<ConfigManager::Config> g_cfgs;
std::vector<OutputData> g_outputs;
std::unique_ptr<ControlData[]> g_ctrl;
std::vector<std::thread> g_renderthreads;
int g_primary = -1;


std::unique_ptr<Baikal::Scene1> g_scene;


static bool     g_is_left_pressed = false;
static bool     g_is_right_pressed = false;
static bool     g_is_fwd_pressed = false;
static bool     g_is_back_pressed = false;
static bool     g_is_home_pressed = false;
static bool     g_is_end_pressed = false;
static bool     g_is_mouse_tracking = false;
static float2   g_mouse_pos = float2(0, 0);
static float2   g_mouse_delta = float2(0, 0);


// CLW stuff
CLWImage2D g_cl_interop_image;

char* GetCmdOption(char ** begin, char ** end, const std::string & option);
bool CmdOptionExists(char** begin, char** end, const std::string& option);
void ShowHelpAndDie();
void SaveFrameBuffer(std::string const& name, float3 const* data);

void Render(GLFWwindow* window)
{
    try
    {
        {
            int w, h;
            glfwGetFramebufferSize(window, &w, &h); CHECK_GL_ERROR;
            glDisable(GL_DEPTH_TEST); CHECK_GL_ERROR;
            glViewport(0, 0, w, h); CHECK_GL_ERROR;


            glClear(GL_COLOR_BUFFER_BIT); CHECK_GL_ERROR;
            glBindVertexArray(g_vao); CHECK_GL_ERROR;

            GLuint program = g_shader_manager->GetProgram("../App/GLSL/simple");
            glUseProgram(program); CHECK_GL_ERROR;

            GLuint texloc = glGetUniformLocation(program, "g_Texture");
            assert(texloc >= 0);

            glUniform1i(texloc, 0); CHECK_GL_ERROR;

            glActiveTexture(GL_TEXTURE0); CHECK_GL_ERROR;
            glBindTexture(GL_TEXTURE_2D, g_texture); CHECK_GL_ERROR;


            glEnableVertexAttribArray(0); CHECK_GL_ERROR;
            glEnableVertexAttribArray(1); CHECK_GL_ERROR;

            glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer); CHECK_GL_ERROR;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer); CHECK_GL_ERROR;

            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0); CHECK_GL_ERROR;
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3)); CHECK_GL_ERROR;
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

            glDisableVertexAttribArray(0); CHECK_GL_ERROR;
            glDisableVertexAttribArray(1); CHECK_GL_ERROR;
            glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;
            glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
            glUseProgram(0); CHECK_GL_ERROR;
            glBindVertexArray(0);

            glFinish(); CHECK_GL_ERROR;
        }
    }
    catch (std::runtime_error& e)
    {
        std::cout << e.what();
        exit(-1);
    }
}

void InitGraphics()
{
    g_shader_manager.reset(new ShaderManager());

    glGetError();
    glClearColor(0.0, 0.5, 0.0, 0.0); CHECK_GL_ERROR;
    glDisable(GL_DEPTH_TEST); CHECK_GL_ERROR;

    glGenBuffers(1, &g_vertex_buffer); CHECK_GL_ERROR;
    glGenBuffers(1, &g_index_buffer); CHECK_GL_ERROR;

    glGenVertexArrays(1, &g_vao); CHECK_GL_ERROR;

    // create Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer); CHECK_GL_ERROR;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer); CHECK_GL_ERROR;

    float quad_vdata[] =
    {
        -1, -1, 0.5, 0, 0,
        1, -1, 0.5, 1, 0,
        1, 1, 0.5, 1, 1,
        -1, 1, 0.5, 0, 1
    };

    GLshort quad_idata[] =
    {
        0, 1, 3,
        3, 1, 2
    };

    // fill data
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vdata), quad_vdata, GL_STATIC_DRAW); CHECK_GL_ERROR;
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_idata), quad_idata, GL_STATIC_DRAW); CHECK_GL_ERROR;

    glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_GL_ERROR;


    glGenTextures(1, &g_texture); CHECK_GL_ERROR;
    glBindTexture(GL_TEXTURE_2D, g_texture); CHECK_GL_ERROR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_GL_ERROR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_GL_ERROR;

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_window_width, g_window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); CHECK_GL_ERROR;

    glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;
}

void InitCl()
{
    bool force_disable_itnerop = false;

    try
    {
        ConfigManager::CreateConfigs(g_mode, g_interop, g_cfgs, g_num_bounces);
    }
    catch (CLWException &)
    {
        force_disable_itnerop = true;
        ConfigManager::CreateConfigs(g_mode, false, g_cfgs, g_num_bounces);
    }


    std::cout << "Running on devices: \n";

    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        std::cout << i << ": " << g_cfgs[i].context.GetDevice(g_cfgs[i].devidx).GetName() << "\n";
    }

    g_interop = false;

    g_outputs.resize(g_cfgs.size());
    g_ctrl.reset(new ControlData[g_cfgs.size()]);

    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        if (g_cfgs[i].type == ConfigManager::kPrimary)
        {
            g_primary = i;

            if (g_cfgs[i].caninterop)
            {
                g_cl_interop_image = g_cfgs[i].context.CreateImage2DFromGLTexture(g_texture);
                g_interop = true;
            }
        }

        g_ctrl[i].clear.store(1);
        g_ctrl[i].stop.store(0);
        g_ctrl[i].newdata.store(0);
        g_ctrl[i].idx = i;
    }

    if (force_disable_itnerop)
    {
        std::cout << "OpenGL interop is not supported, disabled, -interop flag is ignored\n";
    }
    else
    {
        if (g_interop)
        {
            std::cout << "OpenGL interop mode enabled\n";
        }
        else
        {
            std::cout << "OpenGL interop mode disabled\n";
        }
    }
}

void InitData()
{

    rand_init();

    // Load obj file
    std::string basepath = g_path;
    basepath += "/";
    std::string filename = basepath + g_modelname;

    {
        // Load OBJ scene
        std::unique_ptr<Baikal::SceneIo> scene_io(Baikal::SceneIo::CreateSceneIoObj());
        g_scene.reset(scene_io->LoadScene(filename, basepath));

        // Enable this to generate new materal mapping for a model
#if 0
        std::unique_ptr<Baikal::MaterialIo> material_io(Baikal::MaterialIo::CreateMaterialIoXML());
        material_io->SaveMaterialsFromScene(basepath + "materials.xml", *g_scene);
        material_io->SaveIdentityMapping(basepath + "mapping.xml", *g_scene);
#endif

        // Check it we have material remapping
        std::ifstream in_materials(basepath + "materials.xml");
        std::ifstream in_mapping(basepath + "mapping.xml");

        if (in_materials && in_mapping)
        {
            in_materials.close();
            in_mapping.close();

            std::unique_ptr<Baikal::MaterialIo> material_io(Baikal::MaterialIo::CreateMaterialIoXML());

            auto mats = material_io->LoadMaterials(basepath + "materials.xml");
            auto mapping = material_io->LoadMaterialMapping(basepath + "mapping.xml");

            material_io->ReplaceSceneMaterials(*g_scene, *mats, mapping);
        }
    }

    g_camera.reset(new Baikal::PerspectiveCamera(
        g_camera_pos
        , g_camera_at
        , g_camera_up));

    g_scene->SetCamera(g_camera.get());

    // Adjust sensor size based on current aspect ratio
    float aspect = (float)g_window_width / g_window_height;
    g_camera_sensor_size.y = g_camera_sensor_size.x / aspect;

    g_camera->SetSensorSize(g_camera_sensor_size);
    g_camera->SetDepthRange(g_camera_zcap);
    g_camera->SetFocalLength(g_camera_focal_length);
    g_camera->SetFocusDistance(g_camera_focus_distance);
    g_camera->SetAperture(g_camera_aperture);

    std::cout << "Camera type: " << (g_camera->GetAperture() > 0.f ? "Physical" : "Pinhole") << "\n";
    std::cout << "Lens focal length: " << g_camera->GetFocalLength() * 1000.f << "mm\n";
    std::cout << "Lens focus distance: " << g_camera->GetFocusDistance() << "m\n";
    std::cout << "F-Stop: " << 1.f / (g_camera->GetAperture() * 10.f) << "\n";
    std::cout << "Sensor size: " << g_camera_sensor_size.x * 1000.f << "x" << g_camera_sensor_size.y * 1000.f << "mm\n";


#pragma omp parallel for
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        g_outputs[i].output = (Baikal::ClwOutput*)g_cfgs[i].renderer->CreateOutput(g_window_width, g_window_height);

#ifdef ENABLE_DENOISER
        g_outputs[i].output_denoised = (Baikal::ClwOutput*)g_cfgs[i].renderer->CreateOutput(g_window_width, g_window_height);
        g_outputs[i].output_normal = (Baikal::ClwOutput*)g_cfgs[i].renderer->CreateOutput(g_window_width, g_window_height);
        g_outputs[i].output_position = (Baikal::ClwOutput*)g_cfgs[i].renderer->CreateOutput(g_window_width, g_window_height);
        g_outputs[i].output_albedo = (Baikal::ClwOutput*)g_cfgs[i].renderer->CreateOutput(g_window_width, g_window_height);
        g_outputs[i].denoiser = new Baikal::BilateralDenoiser(g_cfgs[i].context);
#endif

        g_cfgs[i].renderer->SetOutput(Baikal::Renderer::OutputType::kColor, g_outputs[i].output);

#ifdef ENABLE_DENOISER
        g_cfgs[i].renderer->SetOutput(Baikal::Renderer::OutputType::kWorldShadingNormal, g_outputs[i].output_normal);
        g_cfgs[i].renderer->SetOutput(Baikal::Renderer::OutputType::kWorldPosition, g_outputs[i].output_position);
        g_cfgs[i].renderer->SetOutput(Baikal::Renderer::OutputType::kAlbedo, g_outputs[i].output_albedo);
#endif

        g_outputs[i].fdata.resize(g_window_width * g_window_height);
        g_outputs[i].udata.resize(g_window_width * g_window_height * 4);

        if (g_cfgs[i].type == ConfigManager::kPrimary)
        {
            g_outputs[i].copybuffer = g_cfgs[i].context.CreateBuffer<float3>(g_window_width * g_window_height, CL_MEM_READ_WRITE);
        }
    }

    g_cfgs[g_primary].renderer->Clear(float3(0, 0, 0), *g_outputs[g_primary].output);
}

void OnMouseMove(GLFWwindow* window, double x, double y)
{
    if (g_is_mouse_tracking)
    {
        g_mouse_delta = float2((float)x, (float)y) - g_mouse_pos;
        g_mouse_pos = float2((float)x, (float)y);
    }
}

void OnMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            g_is_mouse_tracking = true;

            double x, y;
            glfwGetCursorPos(window, &x, &y);
            g_mouse_pos = float2((float)x, (float)y);
            g_mouse_delta = float2(0, 0);
        }
        else if (action == GLFW_RELEASE && g_is_mouse_tracking)
        {
            g_is_mouse_tracking = false;
            g_mouse_delta = float2(0, 0);
        }
    }
}

void OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    switch (key)
    {
    case GLFW_KEY_UP:
        g_is_fwd_pressed = action == GLFW_PRESS ? true : false;
        break;
    case GLFW_KEY_DOWN:
        g_is_back_pressed = action == GLFW_PRESS ? true : false;
        break;
    case GLFW_KEY_LEFT:
        g_is_left_pressed = action == GLFW_PRESS ? true : false;
        break;
    case GLFW_KEY_RIGHT:
        g_is_right_pressed = action == GLFW_PRESS ? true : false;
        break;
    case GLFW_KEY_HOME:
        g_is_home_pressed = action == GLFW_PRESS ? true : false;
        break;
    case GLFW_KEY_END:
        g_is_end_pressed = action == GLFW_PRESS ? true : false;
        break;
    case GLFW_KEY_F1:
        g_gui_visible = action == GLFW_PRESS ? !g_gui_visible : g_gui_visible;
        break;
    case GLFW_KEY_F3:
        g_benchmark = action == GLFW_PRESS ? true : g_benchmark;
        break;
    default:
        break;
    }
}

void Update(bool update_required)
{
    static auto prevtime = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(time - prevtime);
    prevtime = time;

    bool update = update_required;
    float camrotx = 0.f;
    float camroty = 0.f;

    const float kMouseSensitivity = 0.001125f;
    float2 delta = g_mouse_delta * float2(kMouseSensitivity, kMouseSensitivity);
    camrotx = -delta.x;
    camroty = -delta.y;

    if (std::abs(camroty) > 0.001f)
    {
        g_camera->Tilt(camroty);
        //g_camera->ArcballRotateVertically(float3(0, 0, 0), camroty);
        update = true;
    }

    if (std::abs(camrotx) > 0.001f)
    {

        g_camera->Rotate(camrotx);
        //g_camera->ArcballRotateHorizontally(float3(0, 0, 0), camrotx);
        update = true;
    }

    const float kMovementSpeed = g_cspeed;
    if (g_is_fwd_pressed)
    {
        g_camera->MoveForward((float)dt.count() * kMovementSpeed);
        update = true;
    }

    if (g_is_back_pressed)
    {
        g_camera->MoveForward(-(float)dt.count() * kMovementSpeed);
        update = true;
    }

    if (g_is_right_pressed)
    {
        g_camera->MoveRight((float)dt.count() * kMovementSpeed);
        update = true;
    }

    if (g_is_left_pressed)
    {
        g_camera->MoveRight(-(float)dt.count() * kMovementSpeed);
        update = true;
    }

    if (g_is_home_pressed)
    {
        g_camera->MoveUp((float)dt.count() * kMovementSpeed);
        update = true;
    }

    if (g_is_end_pressed)
    {
        g_camera->MoveUp(-(float)dt.count() * kMovementSpeed);
        update = true;
    }

    if (update)
    {
        //if (g_num_samples > -1)
        {
            g_samplecount = 0;
        }

        for (int i = 0; i < g_cfgs.size(); ++i)
        {
            if (i == g_primary)
            {
                g_cfgs[i].renderer->Clear(float3(0, 0, 0), *g_outputs[i].output);

#ifdef ENABLE_DENOISER
                g_cfgs[i].renderer->Clear(float3(0, 0, 0), *g_outputs[i].output_normal);
                g_cfgs[i].renderer->Clear(float3(0, 0, 0), *g_outputs[i].output_position);
                g_cfgs[i].renderer->Clear(float3(0, 0, 0), *g_outputs[i].output_albedo);
#endif

            }
            else
                g_ctrl[i].clear.store(true);
        }

        /*        numbnc = 1;
        for (int i = 0; i < g_cfgs.size(); ++i)
        {
        g_cfgs[i].renderer->SetNumBounces(numbnc);
        }*/
    }

    if (g_num_samples == -1 || g_samplecount < g_num_samples)
    {
        g_cfgs[g_primary].renderer->Render(*g_scene.get());

#ifdef ENABLE_DENOISER
        Baikal::PostEffect::InputSet input_set;
        input_set[Baikal::Renderer::OutputType::kColor] = g_outputs[g_primary].output;
        input_set[Baikal::Renderer::OutputType::kWorldShadingNormal] = g_outputs[g_primary].output_normal;
        input_set[Baikal::Renderer::OutputType::kWorldPosition] = g_outputs[g_primary].output_position;
        input_set[Baikal::Renderer::OutputType::kAlbedo] = g_outputs[g_primary].output_albedo;
        auto radius = 10U - RadeonRays::clamp((g_samplecount / 32), 1U, 9U);
        auto position_sensitivity = 5.f + 10.f * (radius / 10.f);
        auto normal_sensitivity = 0.1f + (radius / 10.f) * 0.15f;
        auto color_sensitivity = (radius / 10.f) * 5.f;
        auto albedo_sensitivity = 0.05f + (radius / 10.f) * 0.1f;
        g_outputs[g_primary].denoiser->SetParameter("radius", radius);
        g_outputs[g_primary].denoiser->SetParameter("color_sensitivity", color_sensitivity);
        g_outputs[g_primary].denoiser->SetParameter("normal_sensitivity", normal_sensitivity);
        g_outputs[g_primary].denoiser->SetParameter("position_sensitivity", position_sensitivity);
        g_outputs[g_primary].denoiser->SetParameter("albedo_sensitivity", albedo_sensitivity);
        g_outputs[g_primary].denoiser->Apply(input_set, *g_outputs[g_primary].output_denoised);
#endif

        ++g_samplecount;
    }
    else if (g_samplecount == g_num_samples)
    {
        std::cout << "Target sample count reached\n";
        ++g_samplecount;
    }

    //if (std::chrono::duration_cast<std::chrono::seconds>(time - updatetime).count() > 1)
    //{
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        if (g_cfgs[i].type == ConfigManager::kPrimary)
            continue;

        int desired = 1;
        if (std::atomic_compare_exchange_strong(&g_ctrl[i].newdata, &desired, 0))
        {
            {
                //std::unique_lock<std::mutex> lock(g_ctrl[i].datamutex);
                //std::cout << "Start updating acc buffer\n"; std::cout.flush();
                g_cfgs[g_primary].context.WriteBuffer(0, g_outputs[g_primary].copybuffer, &g_outputs[i].fdata[0], g_window_width * g_window_height);
                //std::cout << "Finished updating acc buffer\n"; std::cout.flush();
            }

            CLWKernel acckernel = g_cfgs[g_primary].renderer->GetAccumulateKernel();

            int argc = 0;
            acckernel.SetArg(argc++, g_outputs[g_primary].copybuffer);
            acckernel.SetArg(argc++, g_window_width * g_window_width);
            acckernel.SetArg(argc++, g_outputs[g_primary].output->data());

            int globalsize = g_window_width * g_window_height;
            g_cfgs[g_primary].context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, acckernel);
        }
    }

    //updatetime = time;
    //}

    if (!g_interop)
    {
#ifdef ENABLE_DENOISER
        g_outputs[g_primary].output_denoised->GetData(&g_outputs[g_primary].fdata[0]);
#else
        g_outputs[g_primary].output->GetData(&g_outputs[g_primary].fdata[0]);
#endif

        float gamma = 2.2f;
        for (int i = 0; i < (int)g_outputs[g_primary].fdata.size(); ++i)
        {
            g_outputs[g_primary].udata[4 * i] = (unsigned char)clamp(clamp(pow(g_outputs[g_primary].fdata[i].x / g_outputs[g_primary].fdata[i].w, 1.f / gamma), 0.f, 1.f) * 255, 0, 255);
            g_outputs[g_primary].udata[4 * i + 1] = (unsigned char)clamp(clamp(pow(g_outputs[g_primary].fdata[i].y / g_outputs[g_primary].fdata[i].w, 1.f / gamma), 0.f, 1.f) * 255, 0, 255);
            g_outputs[g_primary].udata[4 * i + 2] = (unsigned char)clamp(clamp(pow(g_outputs[g_primary].fdata[i].z / g_outputs[g_primary].fdata[i].w, 1.f / gamma), 0.f, 1.f) * 255, 0, 255);
            g_outputs[g_primary].udata[4 * i + 3] = 1;
        }


        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, g_texture);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_outputs[g_primary].output->width(), g_outputs[g_primary].output->height(), GL_RGBA, GL_UNSIGNED_BYTE, &g_outputs[g_primary].udata[0]);

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
        std::vector<cl_mem> objects;
        objects.push_back(g_cl_interop_image);
        g_cfgs[g_primary].context.AcquireGLObjects(0, objects);

        CLWKernel copykernel = g_cfgs[g_primary].renderer->GetCopyKernel();

#ifdef ENABLE_DENOISER
        auto output = g_outputs[g_primary].output_denoised;
#else
        auto output = g_outputs[g_primary].output;
#endif

        int argc = 0;
        copykernel.SetArg(argc++, output->data());
        copykernel.SetArg(argc++, output->width());
        copykernel.SetArg(argc++, output->height());
        copykernel.SetArg(argc++, 2.2f);
        copykernel.SetArg(argc++, g_cl_interop_image);

        int globalsize = output->width() * output->height();
        g_cfgs[g_primary].context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, copykernel);

        g_cfgs[g_primary].context.ReleaseGLObjects(0, objects);
        g_cfgs[g_primary].context.Finish(0);
    }


    if (g_benchmark)
    {
        auto const kNumBenchmarkPasses = 100U;

        Baikal::Renderer::BenchmarkStats stats;
        g_cfgs[g_primary].renderer->RunBenchmark(*g_scene.get(), kNumBenchmarkPasses, stats);

        auto numrays = stats.resolution.x * stats.resolution.y;
        std::cout << "Baikal renderer benchmark\n";
        std::cout << "Number of primary rays: " << numrays << "\n";
        std::cout << "Primary rays: " << (float)(numrays / (stats.primary_rays_time_in_ms * 0.001f) * 0.000001f) << "mrays/s ( " << stats.primary_rays_time_in_ms << "ms )\n";
        std::cout << "Secondary rays: " << (float)(numrays / (stats.secondary_rays_time_in_ms * 0.001f) * 0.000001f) << "mrays/s ( " << stats.secondary_rays_time_in_ms << "ms )\n";
        std::cout << "Shadow rays: " << (float)(numrays / (stats.shadow_rays_time_in_ms * 0.001f) * 0.000001f) << "mrays/s ( " << stats.shadow_rays_time_in_ms << "ms )\n";
        g_benchmark = false;
    }
}

void RenderThread(ControlData& cd)
{
    auto renderer = g_cfgs[cd.idx].renderer;
    auto output = g_outputs[cd.idx].output;

    auto updatetime = std::chrono::high_resolution_clock::now();

    while (!cd.stop.load())
    {
        int result = 1;
        bool update = false;

        if (std::atomic_compare_exchange_strong(&cd.clear, &result, 0))
        {
            renderer->Clear(float3(0, 0, 0), *output);
            update = true;
        }

        renderer->Render(*g_scene.get());

        auto now = std::chrono::high_resolution_clock::now();

        update = update || (std::chrono::duration_cast<std::chrono::seconds>(now - updatetime).count() > 1);

        if (update)
        {
            g_outputs[cd.idx].output->GetData(&g_outputs[cd.idx].fdata[0]);
            updatetime = now;
            cd.newdata.store(1);
        }

        g_cfgs[cd.idx].context.Finish(0);
    }
}

void StartRenderThreads()
{
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        if (i != g_primary)
        {
            g_renderthreads.push_back(std::thread(RenderThread, std::ref(g_ctrl[i])));
            g_renderthreads.back().detach();
        }
    }

    std::cout << g_cfgs.size() << " OpenCL submission threads started\n";
}


void OnError(int error, const char* description)
{
    std::cout << description << "\n";
}

int main(int argc, char * argv[])
{
    // Command line parsing
    char* path = GetCmdOption(argv, argv + argc, "-p");
    g_path = path ? path : g_path;

    char* modelname = GetCmdOption(argv, argv + argc, "-f");
    g_modelname = modelname ? modelname : g_modelname;

    char* envmapname = GetCmdOption(argv, argv + argc, "-e");
    g_envmapname = envmapname ? envmapname : g_envmapname;

    char* width = GetCmdOption(argv, argv + argc, "-w");
    g_window_width = width ? atoi(width) : g_window_width;

    char* height = GetCmdOption(argv, argv + argc, "-h");
    g_window_height = width ? atoi(height) : g_window_height;

    char* aorays = GetCmdOption(argv, argv + argc, "-ao");
    g_ao_radius = aorays ? (float)atof(aorays) : g_ao_radius;

    char* bounces = GetCmdOption(argv, argv + argc, "-nb");
    g_num_bounces = bounces ? atoi(bounces) : g_num_bounces;

    char* camposx = GetCmdOption(argv, argv + argc, "-cpx");
    g_camera_pos.x = camposx ? (float)atof(camposx) : g_camera_pos.x;

    char* camposy = GetCmdOption(argv, argv + argc, "-cpy");
    g_camera_pos.y = camposy ? (float)atof(camposy) : g_camera_pos.y;

    char* camposz = GetCmdOption(argv, argv + argc, "-cpz");
    g_camera_pos.z = camposz ? (float)atof(camposz) : g_camera_pos.z;

    char* camatx = GetCmdOption(argv, argv + argc, "-tpx");
    g_camera_at.x = camatx ? (float)atof(camatx) : g_camera_at.x;

    char* camaty = GetCmdOption(argv, argv + argc, "-tpy");
    g_camera_at.y = camaty ? (float)atof(camaty) : g_camera_at.y;

    char* camatz = GetCmdOption(argv, argv + argc, "-tpz");
    g_camera_at.z = camatz ? (float)atof(camatz) : g_camera_at.z;

    char* envmapmul = GetCmdOption(argv, argv + argc, "-em");
    g_envmapmul = envmapmul ? (float)atof(envmapmul) : g_envmapmul;

    char* numsamples = GetCmdOption(argv, argv + argc, "-ns");
    g_num_samples = numsamples ? atoi(numsamples) : g_num_samples;

    char* camera_aperture = GetCmdOption(argv, argv + argc, "-a");
    g_camera_aperture = camera_aperture ? (float)atof(camera_aperture) : g_camera_aperture;

    char* camera_dist = GetCmdOption(argv, argv + argc, "-fd");
    g_camera_focus_distance = camera_dist ? (float)atof(camera_dist) : g_camera_focus_distance;

    char* camera_focal_length = GetCmdOption(argv, argv + argc, "-fl");
    g_camera_focal_length = camera_focal_length ? (float)atof(camera_focal_length) : g_camera_focal_length;

    char* interop = GetCmdOption(argv, argv + argc, "-interop");
    g_interop = interop ? (atoi(interop) > 0) : g_interop;

    char* cspeed = GetCmdOption(argv, argv + argc, "-cs");
    g_cspeed = cspeed ? (float)atof(cspeed) : g_cspeed;


    char* cfg = GetCmdOption(argv, argv + argc, "-config");

    if (cfg)
    {
        if (strcmp(cfg, "cpu") == 0)
            g_mode = ConfigManager::Mode::kUseSingleCpu;
        else if (strcmp(cfg, "gpu") == 0)
            g_mode = ConfigManager::Mode::kUseSingleGpu;
        else if (strcmp(cfg, "mcpu") == 0)
            g_mode = ConfigManager::Mode::kUseCpus;
        else if (strcmp(cfg, "mgpu") == 0)
            g_mode = ConfigManager::Mode::kUseGpus;
        else if (strcmp(cfg, "all") == 0)
            g_mode = ConfigManager::Mode::kUseAll;
    }

    if (aorays)
    {
        g_num_ao_rays = atoi(aorays);
        g_ao_enabled = true;
    }

    if (CmdOptionExists(argv, argv + argc, "-r"))
    {
        g_progressive = true;
    }

    // Initialize GLFW
    {
        auto err = glfwInit();
        if (err != GLFW_TRUE)
        {
            std::cout << "GLFW initialization failed\n";
            return -1;
        }
    }
    // Setup window
    glfwSetErrorCallback(OnError);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // GLUT Window Initialization:
    GLFWwindow* window = glfwCreateWindow(g_window_width, g_window_height, "Baikal standalone demo", nullptr, nullptr);
    glfwMakeContextCurrent(window);

#ifndef __APPLE__
    {
        glewExperimental = GL_TRUE;
        GLenum err = glewInit();
        if (err != GLEW_OK)
        {
            std::cout << "GLEW initialization failed\n";
            return -1;
        }
    }
#endif

    ImGui_ImplGlfwGL3_Init(window, true);

    try
    {
        InitGraphics();
        InitCl();
        InitData();
        glfwSetMouseButtonCallback(window, OnMouseButton);
        glfwSetCursorPosCallback(window, OnMouseMove);
        glfwSetKeyCallback(window, OnKey);

        StartRenderThreads();

        // Collect some scene statistics
        auto num_triangles = 0U;
        auto num_instances = 0U;
        {
            std::unique_ptr<Baikal::Iterator> shape_iter(g_scene->CreateShapeIterator());

            for (; shape_iter->IsValid(); shape_iter->Next())
            {
                auto shape = shape_iter->ItemAs<Baikal::Shape const>();
                auto mesh = dynamic_cast<Baikal::Mesh const*>(shape);

                if (mesh)
                {
                    num_triangles += mesh->GetNumIndices() / 3;
                }
                else
                {
                    ++num_instances;
                }
            }
        }

        bool update = false;
        while (!glfwWindowShouldClose(window))
        {

            ImGui_ImplGlfwGL3_NewFrame();
            Update(update);
            Render(window);
            update = false;

            static float aperture = 0.0f;
            static float focal_length = 35.f;
            static float focus_distance = 1.f;
            static int num_bounces = 5;
            static char const* outputs =
                "Color\0"
                "World position\0"
                "Shading normal\0"
                "Geometric normal\0"
                "Texture coords\0"
                "Wire\0"
                "Albedo\0\0";

            static int output = 0;

            if (g_gui_visible)
            {
                ImGui::SetNextWindowSizeConstraints(ImVec2(380, 400), ImVec2(380, 400));
                ImGui::Begin("Baikal settings");
                ImGui::Text("Use arrow keys to move");
                ImGui::Text("PgUp/Down to climb/descent");
                ImGui::Text("Mouse+RMB to look around");
                ImGui::Text("F1 to hide/show GUI");
                ImGui::Separator();
                ImGui::Text("Device vendor: %s", g_cfgs[g_primary].context.GetDevice(0).GetVendor().c_str());
                ImGui::Text("Device name: %s", g_cfgs[g_primary].context.GetDevice(0).GetName().c_str());
                ImGui::Text("OpenCL: %s", g_cfgs[g_primary].context.GetDevice(0).GetVersion().c_str());
                ImGui::Separator();
                ImGui::Text("Resolution: %dx%d ", g_window_width, g_window_height);
                ImGui::Text("Scene: %s", g_modelname);
                ImGui::Text("Unique triangles: %d", num_triangles);
                ImGui::Text("Number of instances: %d", num_instances);
                ImGui::Separator();
                ImGui::SliderInt("GI bounces", &num_bounces, 1, 10);
                ImGui::SliderFloat("Aperture(mm)", &aperture, 0.0f, 100.0f);
                ImGui::SliderFloat("Focal length(mm)", &focal_length, 5.f, 200.0f);
                ImGui::SliderFloat("Focus distance(m)", &focus_distance, 0.05f, 20.f);

                if (aperture != g_camera_aperture * 1000.f)
                {
                    g_camera_aperture = aperture / 1000.f;
                    g_camera->SetAperture(g_camera_aperture);
                    update = true;
                }

                if (focus_distance != g_camera_focus_distance)
                {
                    g_camera_focus_distance = focus_distance;
                    g_camera->SetFocusDistance(g_camera_focus_distance);
                    update = true;
                }

                if (focal_length != g_camera_focal_length * 1000.f)
                {
                    g_camera_focal_length = focal_length / 1000.f;
                    g_camera->SetFocalLength(g_camera_focal_length);
                    update = true;
                }

                if (num_bounces != g_num_bounces)
                {
                    g_num_bounces = num_bounces;
                    for (int i = 0; i < g_cfgs.size(); ++i)
                    {
                        g_cfgs[i].renderer->SetNumBounces(g_num_bounces);
                    }
                    update = true;
                }

                if (static_cast<Baikal::Renderer::OutputType>(output) != g_ouput_type)
                {
                    auto tmp = static_cast<Baikal::Renderer::OutputType>(output);

                    for (int i = 0; i < g_cfgs.size(); ++i)
                    {
                        g_cfgs[i].renderer->SetOutput(g_ouput_type, nullptr);
                        g_cfgs[i].renderer->SetOutput(tmp, g_outputs[i].output);
                        g_ouput_type = tmp;
                    }

                    update = true;
                }

                ImGui::Combo("Output", &output, outputs);
                ImGui::Text(" ");
                ImGui::Text("Frame time %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
                ImGui::Text("Renderer performance %.3f Msamples/s", (ImGui::GetIO().Framerate * g_window_width * g_window_height) / 1000000.f);
                ImGui::End();
                ImGui::Render();
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }

        for (int i = 0; i < g_cfgs.size(); ++i)
        {
            if (i == g_primary)
                continue;

            g_ctrl[i].stop.store(true);
        }

        glfwDestroyWindow(window);
    }
    catch (std::runtime_error& err)
    {
        glfwDestroyWindow(window);
        std::cout << err.what();
        return -1;
    }

    return 0;
}

char* GetCmdOption(char ** begin, char ** end, const std::string & option)
{
    char ** itr = std::find(begin, end, option);
    if (itr != end && ++itr != end)
    {
        return *itr;
    }
    return 0;
}

bool CmdOptionExists(char** begin, char** end, const std::string& option)
{
    return std::find(begin, end, option) != end;
}

void ShowHelpAndDie()
{
    std::cout << kHelpMessage << "\n";
}

void SaveImage(std::string const& name, float3 const* data, int w, int h)
{


}

void SaveFrameBuffer(std::string const& name, float3 const* data)
{
    OIIO_NAMESPACE_USING;

    std::vector<float3> tempbuf(g_window_width * g_window_height);
    tempbuf.assign(data, data + g_window_width*g_window_height);

    for (auto y = 0; y < g_window_height; ++y)
        for (auto x = 0; x < g_window_width; ++x)
        {

            float3 val = data[(g_window_height - 1 - y) * g_window_width + x];
            tempbuf[y * g_window_width + x] = (1.f / val.w) * val;

            tempbuf[y * g_window_width + x].x = std::pow(tempbuf[y * g_window_width + x].x, 1.f / 2.2f);
            tempbuf[y * g_window_width + x].y = std::pow(tempbuf[y * g_window_width + x].y, 1.f / 2.2f);
            tempbuf[y * g_window_width + x].z = std::pow(tempbuf[y * g_window_width + x].z, 1.f / 2.2f);
        }

    ImageOutput* out = ImageOutput::create(name);

    if (!out)
    {
        throw std::runtime_error("Can't create image file on disk");
    }

    ImageSpec spec(g_window_width, g_window_height, 3, TypeDesc::FLOAT);

    out->open(name, spec);
    out->write_image(TypeDesc::FLOAT, &tempbuf[0], sizeof(float3));
    out->close();
}
