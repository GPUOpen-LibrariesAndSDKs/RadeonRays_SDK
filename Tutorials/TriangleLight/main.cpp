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
#include "radeon_rays.h"
#include <GL/glew.h>
#include <GLUT/GLUT.h>
#include <cassert>
#include <iostream>
#include <memory>
#include "../Tools/shader_manager.h"

using namespace RadeonRays;

namespace {
    float const g_vertices[] = {
        -1.f,-1.f,0.f,
        1.f,-1.f,0.f,
        0.f,1.f,0.f,
    };
    float const g_normals[] = {
        0.f,0.f,1.f,
        0.f,0.f,1.f,
        0.f,0.f,1.f,
    };
    int const g_indices[] = { 0, 1, 2 };
    const int g_numfaceverts[] = { 3 };
    const float3 g_color = { 1.f, 0.f, 0.f, 1.f };
    GLuint g_vertex_buffer, g_index_buffer;
    GLuint g_texture;
    int g_window_width = 640;
    int g_window_height = 480;
    std::unique_ptr<ShaderManager> g_shader_manager;
}

float3 ConvertFromBarycentric(const float* vec, const int* ind, int prim_id, const float4& uvwt)
{
    float3 a = { vec[ind[prim_id * 3] * 3],
        vec[ind[prim_id * 3] * 3 + 1],
        vec[ind[prim_id * 3] * 3 + 2], };

    float3 b = { vec[ind[prim_id * 3 + 1] * 3],
        vec[ind[prim_id * 3 + 1] * 3 + 1],
        vec[ind[prim_id * 3 + 1] * 3 + 2], };

    float3 c = { vec[ind[prim_id * 3 + 2] * 3],
        vec[ind[prim_id * 3 + 2] * 3 + 1],
        vec[ind[prim_id * 3 + 2] * 3 + 2], };
    return a * (1 - uvwt.x - uvwt.y) + b * uvwt.x + c * uvwt.y;
}

void InitGl()
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

    //texture
    glGenTextures(1, &g_texture);
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, g_window_width, g_window_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void DrawScene()
{
    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, g_window_width, g_window_height);
    glClear(GL_COLOR_BUFFER_BIT);

    glBindBuffer(GL_ARRAY_BUFFER, g_vertex_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_index_buffer);

    //shader data
    GLuint program = g_shader_manager->GetProgram("simple");
    glUseProgram(program);
    GLuint texloc = glGetUniformLocation(program, "g_Texture");
    assert(texloc >= 0);

    glUniform1i(texloc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_texture);

    GLuint position_attr = glGetAttribLocation(program, "inPosition");
    GLuint texcoord_attr = glGetAttribLocation(program, "inTexcoord");
    glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0);
    glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3));
    glEnableVertexAttribArray(position_attr);
    glEnableVertexAttribArray(texcoord_attr);

    //draw rectanle
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);

    glDisableVertexAttribArray(texcoord_attr);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);

    glFinish();
    glutSwapBuffers();
}

int main(int argc, char* argv[])
{
    // GLUT Window Initialization:
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
    // Prepare rectangle for drawing texture
    // rendered using intersection results
    InitGl();

    // Choose device
    int nativeidx = -1;
    // Always use OpenCL
    IntersectionApi::SetPlatform(DeviceInfo::kOpenCL);

    for (auto idx = 0U; idx < IntersectionApi::GetDeviceCount(); ++idx)
    {
        DeviceInfo devinfo;
        IntersectionApi::GetDeviceInfo(idx, devinfo);

        if (devinfo.type == DeviceInfo::kGpu && nativeidx == -1)
        {
            nativeidx = idx;
        }
    }
    assert(nativeidx != -1);
    IntersectionApi* api = IntersectionApi::Create(nativeidx);

    // Adding triangle to tracing scene
    Shape* shape = api->CreateMesh(g_vertices, 3, 3 * sizeof(float), g_indices, 0, g_numfaceverts, 1);
    assert(shape != nullptr);
    api->AttachShape(shape);
    // Commit scene changes
    api->Commit();

    const int k_raypack_size = g_window_height * g_window_width;

    // Prepare rays. One for each texture pixel.
    std::vector<ray> rays(k_raypack_size);
    for (int i = 0; i < g_window_height; ++i)
        for (int j = 0; j < g_window_width; ++j)
        {
            const float xstep = 2.f / (float)g_window_width;
            const float ystep = 2.f / (float)g_window_height;
            float x = -1.f + xstep * (float)j;
            float y = -1.f + ystep * (float)i;
            float z = 1.f;
            rays[i * g_window_width + j].o = float3(x, y, z, 1000.f);
            rays[i * g_window_width + j].d = float3{0.f, 0.f, -1.f};
        }
    Buffer* ray_buffer = api->CreateBuffer(rays.size() * sizeof(ray), rays.data());

    // Intersection data
    std::vector<Intersection> isect(k_raypack_size);
    Buffer* isect_buffer = api->CreateBuffer(isect.size() * sizeof(Intersection), nullptr);

    // Intersection
    api->QueryIntersection(ray_buffer, k_raypack_size, isect_buffer, nullptr, nullptr);
    
    // Get results
    Event* e = nullptr;
    Intersection* tmp = nullptr;
    api->MapBuffer(isect_buffer, kMapRead, 0, isect.size() * sizeof(Intersection), (void**)&tmp, &e);
    // RadeonRays calls are asynchronous, so need to wait for calculation to complete.
    e->Wait();
    api->DeleteEvent(e);
    e = nullptr;

    // Copy results
    for (int i = 0; i < k_raypack_size; ++i)
    {
        isect[i] = tmp[i];
    }

    // Point light position
    float3 light = { 0.f, 0.f, 0.25f };

    // Render triangle and lightning
    std::vector<unsigned char> tex_data(k_raypack_size * 4);
    for (int i = 0; i < k_raypack_size; ++i)
    {
        int shape_id = isect[i].shapeid;
        int prim_id = isect[i].primid;

        if (shape_id != kNullId)
        {
            // Calculate position and normal of the intersection point
            float3 pos = ConvertFromBarycentric(g_vertices, g_indices, prim_id, isect[i].uvwt);
            float3 norm = ConvertFromBarycentric(g_normals, g_indices, prim_id, isect[i].uvwt);
            norm.normalize();

            // Calculate lighting
            float3 col = { 0.f, 0.f, 0.f };
            float3 light_dir = light - pos;
            light_dir.normalize();
            float dot_prod = dot(norm, light_dir);
            if (dot_prod > 0)
                col += dot_prod * g_color;

            tex_data[i * 4] = col[0] * 255;
            tex_data[i * 4 + 1] = col[1] * 255;
            tex_data[i * 4 + 2] = col[2] * 255;
            tex_data[i * 4 + 3] = 255;
        }
        else
        {
            // Draw white pixels for misses
            tex_data[i * 4] = 255;
            tex_data[i * 4 + 1] = 255;
            tex_data[i * 4 + 2] = 255;
            tex_data[i * 4 + 3] = 255;
        }
    }

    // Update texture data
    glBindTexture(GL_TEXTURE_2D, g_texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, g_window_width, g_window_height, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
    glBindTexture(GL_TEXTURE_2D, NULL);

    // Start main loop.
    glutDisplayFunc(DrawScene);
    glutMainLoop(); 

    // Cleanup
    IntersectionApi::Delete(api);

    return 0;
}
