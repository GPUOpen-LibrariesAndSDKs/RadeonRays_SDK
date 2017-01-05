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
 \file scene.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains declaration of Baikal::Scene1 class, core interface representing representing the scene.
 */
#pragma once

#include <memory>

namespace Baikal
{
    class Light;
    class Shape;
    class Volume;
    class Camera;
    class Iterator;
    
    /**
     \brief Scene class.
     
     Scene represents a collection of objects such as ligths, meshes, volumes, etc. It also has a functionality
     to add, remove and change these objects.
     */
    class Scene1
    {
    public:
        
        using DirtyFlags = std::uint32_t;
        
        enum
        {
            kNone,
            kLights,
            kShapes,
            kShapeTransforms,
            kCamera
        };
        
        // Constructor
        Scene1();
        // Destructor
        ~Scene1();
        
        // Add or remove lights
        void AttachLight(Light const* light);
        void DetachLight(Light const* light);
        
        // TODO: make it iterable
        std::size_t GetNumLights() const;
        Iterator* CreateLightIterator() const;
        
        // Add or remove shapes
        void AttachShape(Shape const* shape);
        void DetachShape(Shape const* shape);
        
        // TODO: make it iterable
        std::size_t GetNumShapes() const;
        Iterator* CreateShapeIterator() const;
        
        // Set camera
        void SetCamera(Camera const* camera);
        Camera const* GetCamera() const;
        
        // Get state change since last clear
        DirtyFlags GetDirtyFlags() const;
        // Set specified flag in dirty state
        void SetDirtyFlag(DirtyFlags flag) const;
        // Clear all flags
        void ClearDirtyFlags() const;
        
        // Forbidden stuff
        Scene1(Scene1 const&) = delete;
        Scene1& operator = (Scene1 const&) = delete;
        
    private:
        struct SceneImpl;
        std::unique_ptr<SceneImpl> m_impl;
    };
    
}
