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

#define NOMINMAX


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

#ifdef RR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

#include "CLW.h"

#include "math/mathutils.h"

#include "Utils/tiny_obj_loader.h"
#include "SceneGraph/camera.h"
#include "SceneGraph/scene1.h"
#include "Renderers/PT/ptrenderer.h"
#include "Renderers/AO/aorenderer.h"
#include "CLW/clwoutput.h"
#include "Utils/config_manager.h"
#include "SceneGraph/IO/scene_io.h"

using namespace RadeonRays;

// Help message
char const* kHelpMessage =
"App [-p path_to_models][-f model_name][-b][-r][-ns number_of_shadow_rays][-ao ao_radius][-w window_width][-h window_height][-nb number_of_indirect_bounces]";
char const* g_path =
"../Resources/bmw";
//"../Resources/CornellBox";

char const* g_modelname = 
"i8.obj";
//"orig.objm";

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
int g_pass_count = 200;

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


std::unique_ptr<Baikal::Scene1> g_scene;
std::unique_ptr<Baikal::PerspectiveCamera> g_camera;

// CLW stuff
CLWImage2D g_cl_interop_image;

char* GetCmdOption(char ** begin, char ** end, const std::string & option);
bool CmdOptionExists(char** begin, char** end, const std::string& option);
void ShowHelpAndDie();
void SaveFrameBuffer(std::string const& name, float3 const* data);

void InitCl()
{
    ConfigManager::CreateConfigs(g_mode, false, g_cfgs, g_num_bounces);


    std::cout << "Running on devices: \n";

    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        std::cout << i << ": " << g_cfgs[i].context.GetDevice(g_cfgs[i].devidx).GetName() << "\n";
    }

    g_outputs.resize(g_cfgs.size());
    g_ctrl.reset(new ControlData[g_cfgs.size()]);

    for (int i = 0; i < g_cfgs.size(); ++i)
    {
        if (g_cfgs[i].type == ConfigManager::kPrimary)
        {
            g_primary = i;
        }

        g_ctrl[i].clear.store(1);
        g_ctrl[i].stop.store(0);
        g_ctrl[i].newdata.store(0);
        g_ctrl[i].idx = i;
    }
}

void InitData()
{

    rand_init();

    // Load obj file
    std::string basepath = g_path;
    basepath += "/";
    std::string filename = basepath + g_modelname;

    std::unique_ptr<Baikal::SceneIo> scene_io(Baikal::SceneIo::CreateSceneIoObj());
    g_scene.reset(scene_io->LoadScene(filename, basepath));

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

    //g_scene->SetEnvironment("../Resources/Textures/studio015.hdr", "", g_envmapmul);

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

Baikal::Renderer::BenchmarkStats Update()
{
    static auto prevtime = std::chrono::high_resolution_clock::now();
    static auto updatetime = std::chrono::high_resolution_clock::now();

    static int numbnc = 1;
    static int framesperbnc = 2;
    auto time = std::chrono::high_resolution_clock::now();
    auto dt = std::chrono::duration_cast<std::chrono::duration<double>>(time - prevtime);
    prevtime = time;


    if (g_num_samples == -1 || g_samplecount++ < g_num_samples)
    {
        g_cfgs[g_primary].renderer->Render(*g_scene.get());
    }

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

    {
        Baikal::Renderer::BenchmarkStats stats;
        g_cfgs[g_primary].renderer->RunBenchmark(*g_scene.get(), g_pass_count, stats);

        return stats;
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
    g_camera_aperture = camera_aperture ? (float)atof(camera_aperture) : g_camera_aperture;

    char* camera_dist = GetCmdOption(argv, argv + argc, "-fd");
    g_camera_focus_distance = camera_dist ? (float)atof(camera_dist) : g_camera_focus_distance;

    char* camera_focal_length = GetCmdOption(argv, argv + argc, "-fl");
    g_camera_focal_length = camera_focal_length ? (float)atof(camera_focal_length) : g_camera_focal_length;

    char* cspeed = GetCmdOption(argv, argv + argc, "-cs");
    g_cspeed = cspeed ? (atof(cspeed) > 0) : g_cspeed;


    char* pass_count = GetCmdOption(argv, argv + argc, "-fc");
    g_pass_count = pass_count ? (atoi(pass_count)) : g_pass_count;

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


    try
    {
        InitCl();
        InitData();

        StartRenderThreads();

        Baikal::Renderer::BenchmarkStats stats = Update();

        auto numrays = stats.resolution.x * stats.resolution.y;
        std::cout << "Baikal renderer benchmark\n";
        std::cout << "Iterations: " << g_pass_count << std::endl;
        std::cout << "Number of primary rays: " << numrays << std::endl;
        std::cout << "Primary rays: " << (float)(numrays / (stats.primary_rays_time_in_ms * 0.001f) * 0.000001f) << "mrays/s ( " << stats.primary_rays_time_in_ms << "ms )\n";
        std::cout << "Secondary rays: " << (float)(numrays / (stats.secondary_rays_time_in_ms * 0.001f) * 0.000001f) << "mrays/s ( " << stats.secondary_rays_time_in_ms << "ms )\n";
        std::cout << "Shadow rays: " << (float)(numrays / (stats.shadow_rays_time_in_ms * 0.001f) * 0.000001f) << "mrays/s ( " << stats.shadow_rays_time_in_ms << "ms )\n";
        std::cout << "Samples: " << stats.samples_pes_sec << "msamples/s\n";
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