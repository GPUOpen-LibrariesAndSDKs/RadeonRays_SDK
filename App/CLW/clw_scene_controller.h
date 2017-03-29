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
 \file scene_tracker.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains SceneTracker class implementation.
 */
#pragma once

#include "SceneGraph/scene_controller.h"
#include "CLW.h"

#include "CLW/clwscene.h"

#include "radeon_rays_cl.h"

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
    class ClwSceneController : public SceneController<ClwScene>
    {
    public:
        // Constructor
        ClwSceneController(CLWContext context, int devidx);
        // Destructor
        virtual ~ClwSceneController();
        
        // Get underlying intersection API.
        RadeonRays::IntersectionApi* GetIntersectionApi() { return  m_api; }

    protected:
        // Clear intersector and load meshes into it.
        void ReloadIntersector(Scene1 const& scene, ClwScene& inout) const;

    private:
        // Update camera data only.
        void UpdateCamera(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const override;
        // Update shape data only.
        void UpdateShapes(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const override;
        // Update lights data only.
        void UpdateLights(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const override;
        // Update material data.
        void UpdateMaterials(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const override;
        // Update texture data only.
        void UpdateTextures(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const override;
        // Get default material
        Material const* GetDefaultMaterial() const override;
        // If m_current_scene changes
        void UpdateCurrentScene(Scene1 const& scene, ClwScene& out) const override;
        
        // Update intersection API
        void UpdateIntersector(Scene1 const& scene, ClwScene& out) const;
        // Write out single material at data pointer.
        // Collectors are required to convert texture and material pointers into indices.
        void WriteMaterial(Material const* material, Collector& mat_collector, Collector& tex_collector, void* data) const;
        // Write out single light at data pointer.
        // Collector is required to convert texture pointers into indices.
        void WriteLight(Scene1 const& scene, Light const* light, Collector& tex_collector, void* data) const;
        // Write out single texture header at data pointer.
        // Header requires texture data offset, so it is passed in.
        void WriteTexture(Texture const* texture, std::size_t data_offset, void* data) const;
        // Write out texture data at data pointer.
        void WriteTextureData(Texture const* texture, void* data) const;

    private:
        // Context
        CLWContext m_context;
        // Intersection API
        RadeonRays::IntersectionApi* m_api;
        // Default material
        std::unique_ptr<Material> m_default_material;
    };
}
