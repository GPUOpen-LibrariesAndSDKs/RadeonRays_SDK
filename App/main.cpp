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
#include <OpenGL/OpenGL.h>
#elif WIN32
#define NOMINMAX
#include <Windows.h>
#include "GL/glew.h"
#include "GLUT/GLUT.h"
#else
#include <CL/cl.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glx.h>
#endif


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

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef FR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

#include "CLW.h"

#include "math/mathutils.h"

#include "tiny_obj_loader.h"
#include "perspective_camera.h"
#include "shader_manager.h"
#include "Scene/scene.h"
#include "PT/ptrenderer.h"
#include "AO/aorenderer.h"
#include "CLW/clwoutput.h"
#include "config_manager.h"

using namespace FireRays;

// Help message
char const* kHelpMessage =
"App [-p path_to_models][-f model_name][-b][-r][-ns number_of_shadow_rays][-ao ao_radius][-w window_width][-h window_height][-nb number_of_indirect_bounces]";
char const* g_path =
"../Resources/bmw";
char const* g_modelname = "i8.obj";

std::unique_ptr<ShaderManager>	g_shader_manager;

GLuint g_vertex_buffer;
GLuint g_index_buffer;
GLuint g_texture;

int g_window_width = 800;
int g_window_height = 600;
int g_num_shadow_rays = 1;
int g_num_ao_rays = 1;
int g_ao_enabled = false;
int g_progressive = false;
int g_num_bounces = 5;
int g_num_samples = -1;
int g_samplecount = 0;
float g_ao_radius = 1.f;
float g_envmapmul = 1.f;
float g_cspeed = 100.25f;

float3 g_camera_pos = float3(0.f, 1.f, 4.f);
float3 g_camera_at = float3(0.f, 1.f, 0.f);
float3 g_camera_up = float3(0.f, 1.f, 0.f);

float2 g_camera_sensor_size = float2(0.036f, 0.024f);  // default full frame sensor 36x24 mm
float2 g_camera_zcap = float2(0.0f, 100000.f);
float g_camera_focal_length = 0.035f; // 35mm lens
float g_camera_focus_distance = 0.f;
float g_camera_aperture = 0.f;


bool g_recording_enabled = false;
int g_frame_count = 0;
bool g_benchmark = false;
bool g_interop = true;
ConfigManager::Mode g_mode = ConfigManager::Mode::kUseSingleGpu;

using namespace tinyobj;


struct OutputData
{
    Baikal::ClwOutput* output;
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


std::unique_ptr<Baikal::Scene> g_scene;


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

void Render()
{
    try
    {
        {
            glDisable(GL_DEPTH_TEST);
            glViewport(0, 0, g_window_width, g_window_height);

            glClear(GL_COLOR_BUFFER_BIT);

            glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer);

            GLuint program = g_shader_manager->GetProgram("../App/simple");
            glUseProgram(program);

            GLuint texloc = glGetUniformLocation(program, "g_Texture");
            assert(texloc >= 0);

            glUniform1i(texloc, 0);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, g_texture);

            GLuint position_attr = glGetAttribLocation(program, "inPosition");
            GLuint texcoord_attr = glGetAttribLocation(program, "inTexcoord");

            glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, sizeof(float)* 5, 0);
            glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 5, (void*)(sizeof(float)* 3));

            glEnableVertexAttribArray(position_attr);
            glEnableVertexAttribArray(texcoord_attr);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

            glDisableVertexAttribArray(texcoord_attr);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glUseProgram(0);

            glFinish();
        }

        glutSwapBuffers();
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

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glCullFace(GL_NONE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glGenBuffers(1, &g_vertex_buffer);
    glGenBuffers(1, &g_index_buffer);

    // create Vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer);

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
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vdata), quad_vdata, GL_STATIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_idata), quad_idata, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_window_width, g_window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void InitCl()
{
    bool forceDisableInterop = false;

    try {
        ConfigManager::CreateConfigs(g_mode, g_interop, g_cfgs, g_num_bounces);
    } catch(CLWException & exc) {
        forceDisableInterop = true;
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

    if(forceDisableInterop)
    {
        std::cout << "OpenGL interop is not supported, disabled, -interop flag is ignored\n";
    } else {
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

    g_scene.reset(Baikal::Scene::LoadFromObj(filename, basepath));

    g_scene->camera_.reset(new PerspectiveCamera(
    g_camera_pos
    , g_camera_at
    , g_camera_up));

    // Adjust sensor size based on current aspect ratio
    float aspect = (float)g_window_width / g_window_height;
    g_camera_sensor_size.y = g_camera_sensor_size.x / aspect;

    g_scene->camera_->SetSensorSize(g_camera_sensor_size);
    g_scene->camera_->SetDepthRange(g_camera_zcap);
    g_scene->camera_->SetFocalLength(g_camera_focal_length);
    g_scene->camera_->SetFocusDistance(g_camera_focus_distance);
    g_scene->camera_->SetAperture(g_camera_aperture);

    std::cout << "Camera type: " << (g_scene->camera_->GetAperture() > 0.f ? "Physical" : "Pinhole") << "\n";
    std::cout << "Lens focal length: " << g_scene->camera_->GetFocalLength() * 1000.f << "mm\n";
    std::cout << "Lens focus distance: " << g_scene->camera_->GetFocusDistance() << "m\n";
    std::cout << "F-Stop: " << 1.f / (g_scene->camera_->GetAperture() * 10.f)  << "\n";
    std::cout << "Sensor size: " << g_camera_sensor_size.x * 1000.f << "x"  << g_camera_sensor_size.y * 1000.f << "mm\n";

    g_scene->SetEnvironment("../Resources/Textures/studio015.hdr", "", g_envmapmul);

    #pragma omp parallel for
    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        //g_cfgs[i].renderer->SetNumBounces(g_num_bounces);
        g_cfgs[i].renderer->Preprocess(*g_scene);

        g_outputs[i].output = (Baikal::ClwOutput*)g_cfgs[i].renderer->CreateOutput(g_window_width, g_window_height);

        g_cfgs[i].renderer->SetOutput(g_outputs[i].output);

        g_outputs[i].fdata.resize(g_window_width * g_window_height);
        g_outputs[i].udata.resize(g_window_width * g_window_height * 4);

        if (g_cfgs[i].type == ConfigManager::kPrimary)
        {
            g_outputs[i].copybuffer = g_cfgs[i].context.CreateBuffer<float3>(g_window_width * g_window_height, CL_MEM_READ_WRITE);
        }
    }

    g_cfgs[g_primary].renderer->Clear(float3(0, 0, 0), *g_outputs[g_primary].output);
}

void Reshape(GLint w, GLint h)
{
    // Disable window resize
    glutReshapeWindow(g_window_width, g_window_height);
}

void OnMouseMove(int x, int y)
{
    if (g_is_mouse_tracking)
    {
        g_mouse_delta = float2((float)x, (float)y) - g_mouse_pos;
        g_mouse_pos = float2((float)x, (float)y);
    }
}

void OnMouseButton(int btn, int state, int x, int y)
{
    if (btn == GLUT_LEFT_BUTTON)
    {
        if (state == GLUT_DOWN)
        {
            g_is_mouse_tracking = true;
            g_mouse_pos = float2((float)x, (float)y);
            g_mouse_delta = float2(0, 0);
        }
        else if (state == GLUT_UP && g_is_mouse_tracking)
        {
            g_is_mouse_tracking = true;
            g_mouse_delta = float2(0, 0);
        }
    }
}

void OnKey(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_UP:
        g_is_fwd_pressed = true;
        break;
    case GLUT_KEY_DOWN:
        g_is_back_pressed = true;
        break;
    case GLUT_KEY_LEFT:
        g_is_left_pressed = true;
        break;
    case GLUT_KEY_RIGHT:
        g_is_right_pressed = true;
		break;
	case GLUT_KEY_HOME:
		g_is_home_pressed = true;
		break;
	case GLUT_KEY_END:
		g_is_end_pressed = true;
		break;
    case GLUT_KEY_F1:
        g_mouse_delta = float2(0, 0);
        break;
    case GLUT_KEY_F3:
        g_benchmark = true;
        break;
    case GLUT_KEY_F4:
        if (!g_interop)
        {
            std::ostringstream oss;
            oss << "aov_color_" << g_frame_count << ".hdr";
            SaveFrameBuffer(oss.str(), &g_outputs[g_primary].fdata[0]);
            break;
        }
    default:
        break;
    }
}

void OnKeyUp(int key, int x, int y)
{
    switch (key)
    {
    case GLUT_KEY_UP:
        g_is_fwd_pressed = false;
        break;
    case GLUT_KEY_DOWN:
        g_is_back_pressed = false;
        break;
    case GLUT_KEY_LEFT:
        g_is_left_pressed = false;
        break;
    case GLUT_KEY_RIGHT:
        g_is_right_pressed = false;
        break;
	case GLUT_KEY_HOME:
		g_is_home_pressed = false;
		break;
	case GLUT_KEY_END:
		g_is_end_pressed = false;
		break;
    case GLUT_KEY_PAGE_DOWN:
        {
            ++g_num_bounces;
            for (int i = 0; i < g_cfgs.size(); ++i)
            {
                g_cfgs[i].renderer->SetNumBounces(g_num_bounces);
                g_cfgs[i].renderer->Clear(float3(0, 0, 0), *g_outputs[i].output);
            }
            g_samplecount = 0;
            break;
        }
    case GLUT_KEY_PAGE_UP:
        {
            if (g_num_bounces > 1)
            {
                --g_num_bounces;
                for (int i = 0; i < g_cfgs.size(); ++i)
                {
                    g_cfgs[i].renderer->SetNumBounces(g_num_bounces);
                    g_cfgs[i].renderer->Clear(float3(0, 0, 0), *g_outputs[i].output);
                }
                g_samplecount = 0;
            }
            break;
        }
    default:
        break;
    }
}

void Update()
{
    static auto prevtime = std::chrono::high_resolution_clock::now();
    static auto updatetime = std::chrono::high_resolution_clock::now();

    static int numbnc = 1;
    static int framesperbnc = 2;
    auto time = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(time - prevtime);
    prevtime = time;

    bool update = false;
    float camrotx = 0.f;
    float camroty = 0.f;

    const float kMouseSensitivity = 0.001125f;
    float2 delta = g_mouse_delta * float2(kMouseSensitivity, kMouseSensitivity);
    camrotx = -delta.x;
    camroty = -delta.y;

    if (std::abs(camroty) > 0.001f)
    {
        g_scene->camera_->Tilt(camroty);
        //g_scene->camera_->ArcballRotateVertically(float3(0, 0, 0), camroty);
        update = true;
    }

    if (std::abs(camrotx) > 0.001f)
    {

        g_scene->camera_->Rotate(camrotx);
        //g_scene->camera_->ArcballRotateHorizontally(float3(0, 0, 0), camrotx);
        update = true;
    }

    const float kMovementSpeed = g_cspeed;
    if (g_is_fwd_pressed)
    {
        g_scene->camera_->MoveForward((float)dt.count() * kMovementSpeed);
        update = true;
    }

    if (g_is_back_pressed)
    {
        g_scene->camera_->MoveForward(-(float)dt.count() * kMovementSpeed);
        update = true;
    }

	if (g_is_right_pressed)
	{
		g_scene->camera_->MoveRight((float)dt.count() * kMovementSpeed);
		update = true;
	}

	if (g_is_left_pressed)
	{
		g_scene->camera_->MoveRight(-(float)dt.count() * kMovementSpeed);
		update = true;
	}

	if (g_is_home_pressed)
	{
		g_scene->camera_->MoveUp((float)dt.count() * kMovementSpeed);
		update = true;
	}

	if (g_is_end_pressed)
	{
		g_scene->camera_->MoveUp(-(float)dt.count() * kMovementSpeed);
		update = true;
	}

    if (update)
    {
        g_scene->set_dirty(Baikal::Scene::kCamera);

        if (g_num_samples > -1)
        {
            g_samplecount = 0;
        }

        for (int i = 0; i < g_cfgs.size(); ++i)
        {
            if (i == g_primary)
                g_cfgs[i].renderer->Clear(float3(0, 0, 0), *g_outputs[i].output);
            else
                g_ctrl[i].clear.store(true);
        }

        /*        numbnc = 1;
        for (int i = 0; i < g_cfgs.size(); ++i)
        {
        g_cfgs[i].renderer->SetNumBounces(numbnc);
    }*/
}

if (g_num_samples == -1 || g_samplecount++ < g_num_samples)
{
    g_cfgs[g_primary].renderer->Render(*g_scene.get());
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
    g_outputs[g_primary].output->GetData(&g_outputs[g_primary].fdata[0]);

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

    int argc = 0;
    copykernel.SetArg(argc++, g_outputs[g_primary].output->data());
    copykernel.SetArg(argc++, g_outputs[g_primary].output->width());
    copykernel.SetArg(argc++, g_outputs[g_primary].output->height());
    copykernel.SetArg(argc++, 2.2f);
    copykernel.SetArg(argc++, g_cl_interop_image);

    int globalsize = g_outputs[g_primary].output->width() * g_outputs[g_primary].output->height();
    g_cfgs[g_primary].context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, copykernel);

    g_cfgs[g_primary].context.ReleaseGLObjects(0, objects);
    g_cfgs[g_primary].context.Finish(0);
}
//}

glutPostRedisplay();
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

int main(int argc, char * argv[])
{
    // Command line parsing
    char* path = GetCmdOption(argv, argv + argc, "-p");
    g_path = path ? path : g_path;

    char* modelname = GetCmdOption(argv, argv + argc, "-f");
    g_modelname = modelname ? modelname : g_modelname;

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
    g_camera_aperture = camera_aperture ? atof(camera_aperture) : g_camera_aperture;

    char* camera_dist = GetCmdOption(argv, argv + argc, "-fd");
    g_camera_focus_distance = camera_dist ? atof(camera_dist) : g_camera_focus_distance;

    char* camera_focal_length = GetCmdOption(argv, argv + argc, "-fl");
    g_camera_focal_length = camera_focal_length ? atof(camera_focal_length) : g_camera_focal_length;

    char* interop = GetCmdOption(argv, argv + argc, "-interop");
    g_interop = interop ? (atoi(interop) > 0) : g_interop;

    char* cspeed = GetCmdOption(argv, argv + argc, "-cs");
    g_cspeed = cspeed ? (atof(cspeed) > 0) : g_cspeed;


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

    // GLUT Window Initialization:
    glutInit(&argc, (char**)argv);
    glutInitWindowSize(g_window_width, g_window_height);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glutCreateWindow("App");

    #ifndef __APPLE__
    GLenum err = glewInit();
    if (err != GLEW_OK)
    {
        std::cout << "GLEW initialization failed\n";
        return -1;
    }
    #endif

    try
    {
        InitGraphics();
        InitCl();
        InitData();

        // Register callbacks:
        glutDisplayFunc(Render);
        glutReshapeFunc(Reshape);

        glutSpecialFunc(OnKey);
        glutSpecialUpFunc(OnKeyUp);
        glutMouseFunc(OnMouseButton);
        glutMotionFunc(OnMouseMove);
        glutIdleFunc(Update);

        StartRenderThreads();

        glutMainLoop();

        for (int i = 0; i < g_cfgs.size(); ++i)
        {
            if (i == g_primary)
            continue;

            g_ctrl[i].stop.store(true);
        }
    }
    catch (std::runtime_error& err)
    {
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
