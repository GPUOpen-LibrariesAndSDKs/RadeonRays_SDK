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
 \file scene_controller.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains SceneController interface.
 */
#pragma once

#include <memory>
#include <map>

namespace Baikal
{
    class Scene1;
    class Collector;
    class Bundle;
    class Material;
    class Light;
    class Texture;
    
    
    /**
     \brief Tracks changes of a scene and serialized data if needed.
     
     SceneTracker class is intended to keep track of CPU side scene changes and update all
     necessary renderer buffers.
     */
    template <typename CompiledScene> class SceneController
    {
    public:
        // Constructor
        SceneController();
        // Destructor
        virtual ~SceneController() = default;
        
        // Given a scene this method produces (or loads from cache) corresponding GPU representation.
        CompiledScene& CompileScene(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector) const;
        
    protected:
        // Recompile the scene from scratch, i.e. not loading from cache.
        // All the buffers are recreated and reloaded.
        void RecompileFull(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, CompiledScene& out) const;
        
    private:
        // Update camera data only.
        virtual void UpdateCamera(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, CompiledScene& out) const = 0;
        // Update shape data only.
        virtual void UpdateShapes(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, CompiledScene& out) const = 0;
        // Update lights data only.
        virtual void UpdateLights(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, CompiledScene& out) const = 0;
        // Update material data.
        virtual void UpdateMaterials(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, CompiledScene& out) const = 0;
        // Update texture data only.
        virtual void UpdateTextures(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, CompiledScene& out) const = 0;
        // Default material
        virtual Material const* GetDefaultMaterial() const = 0;
        // If m_current_scene changes
        virtual void UpdateCurrentScene(Scene1 const& scene, CompiledScene& out) const = 0;
        
    private:
        mutable Scene1 const* m_current_scene;
        // Scene cache map (CPU scene -> GPU scene mapping)
        mutable std::map<Scene1 const*, CompiledScene> m_scene_cache;
    };
}

#include "scene_controller.inl"
