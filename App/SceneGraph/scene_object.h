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
 \file scene_object.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains base interface for scene graph objects
 */
#pragma once

#include <string>

namespace Baikal
{
    class SceneObject
    {
    public:
        // Constructor
        SceneObject();
        // Destructor
        virtual ~SceneObject() = 0;
        
        // Check if the object has been changed since last reset
        bool IsDirty() const;
        // Set dirty flag
        void SetDirty(bool dirty) const;
        
        // Set & get name
        void SetName(std::string const& name);
        std::string GetName() const;
        
        SceneObject(SceneObject const&) = delete;
        SceneObject& operator = (SceneObject const&) = delete;
        
    private:
        mutable bool m_dirty;
        
        std::string m_name;
    };
    
    inline SceneObject::SceneObject()
    : m_dirty(false)
    {
    }
    
    inline SceneObject::~SceneObject()
    {
    }
    
    inline bool SceneObject::IsDirty() const
    {
        return m_dirty;
    }
    
    inline void SceneObject::SetDirty(bool dirty) const
    {
        m_dirty = dirty;
    }
    
    inline std::string SceneObject::GetName() const
    {
        return m_name;
    }
    
    inline void SceneObject::SetName(std::string const& name)
    {
        m_name = name;
    }
}
