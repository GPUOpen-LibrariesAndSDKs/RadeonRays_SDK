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

/**
\file gl_scene_controller.h
\author Dmitry Kozlov
\version 1.0
\brief Contains scene controller implementation for realtime OpenGL renderer.
*/
#pragma once

#include "SceneGraph/scene_controller.h"
#include "radeon_rays_cl.h"

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

#include <map>
#include <vector>

namespace Baikal
{
    class Scene1;
    class Collector;
    class Bundle;
    class Material;
    class Light;
    class Texture;

    /**
    \brief Tracks changes of a scene and serialized data into GPU memory when needed.

    ClwSceneController class is intended to keep track of CPU side scene changes and update all
    necessary GPU buffers. It essentially establishes a mapping between Scene class and
    corresponding ClwScene class. It also pre-caches ClwScenes and speeds up loading for
    already compiled scenes.
    */
    struct GlShapeData
    {
        GLuint index_buffer;
        GLuint vertex_buffer;
        GLuint num_triangles;
        GLint material_idx;
        RadeonRays::matrix transform;

        GlShapeData()
            : index_buffer(0)
            , vertex_buffer(0)
            , num_triangles(0)
            , material_idx(-1)
        {
        }
    };

    struct GlMaterialData
    {
        GLint diffuse_texture_idx;
        GLint gloss_texture_idx;
        RadeonRays::float3 diffuse_color;
        RadeonRays::float3 gloss_color;

        GlMaterialData()
            : diffuse_texture_idx(-1)
            , gloss_texture_idx(-1)
        {
        }
    };

    struct GlTexureData
    {
        GLuint texture;

        GlTexureData()
            : texture(0)
        {
        }
    };

    struct GlVertex
    {
        RadeonRays::float3 p;
        RadeonRays::float3 n;
        RadeonRays::float2 uv;
    };

    struct GlScene
    {
        RadeonRays::matrix view_transform;
        RadeonRays::matrix proj_transform;

        std::map<Shape const*, GlShapeData> shapes;
        std::vector<GlMaterialData> materials;
        std::vector<GlTexureData> textures;

        std::unique_ptr<Bundle> material_bundle;
        std::unique_ptr<Bundle> texture_bundle;
    };

    class GlSceneController : public SceneController<GlScene>
    {
    public:
        // Constructor
        GlSceneController();
        // Destructor
        virtual ~GlSceneController();

    private:
        // Update camera data only.
        void UpdateCamera(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const override;
        // Update shape data only.
        void UpdateShapes(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const override;
        // Update lights data only.
        void UpdateLights(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const override;
        // Update material data.
        void UpdateMaterials(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const override;
        // Update texture data only.
        void UpdateTextures(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const override;
        // Get default material
        Material const* GetDefaultMaterial() const override;
        // If m_current_scene changes
        void UpdateCurrentScene(Scene1 const& scene, GlScene& out) const override;

    private:
        std::unique_ptr<Material> m_default_material;
    };
}
