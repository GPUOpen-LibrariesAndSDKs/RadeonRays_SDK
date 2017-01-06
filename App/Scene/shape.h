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
 \file shape.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains declaration of various shape types supported by the renderer.
 */
#pragma once

#include "math/float3.h"
#include "math/float2.h"
#include <memory>
#include <string>

#include "scene_object.h"

namespace Baikal
{
    class Material;
    
    /**
     \brief Shape base interface.
     
     High-level interface all shape classes need to implement.
     */
    class Shape : public SceneObject
    {
    public:
        // Constructor
        Shape();
        // Destructor
        virtual ~Shape() = 0;
        
        // Get and set material
        virtual void SetMaterial(Material const* material);
        virtual Material const* GetMaterial() const;
        
        // Forbidden stuff
        Shape(Shape const&) = delete;
        Shape& operator = (Shape const&) = delete;
    
    private:
        Material const* m_material;
    };
    
    /**
     \brief Triangle mesh class.
     
     Triangle mesh is a collection of indexed triangle.
     */
    class Mesh : public Shape
    {
    public:
        Mesh();
        
        // Set and get index array
        virtual void SetIndices(std::uint32_t const* indices, std::size_t num_indices);
        virtual std::size_t GetNumIndices() const;
        virtual std::uint32_t const* GetIndices() const;
        
        // Set and get vertex array
        virtual void SetVertices(RadeonRays::float3 const* vertices, std::size_t num_vertices);
        virtual void SetVertices(float const* vertices, std::size_t num_vertices);
        virtual std::size_t GetNumVertices() const;
        virtual RadeonRays::float3 const* GetVertices() const;
        
        // Set and get normal array
        virtual void SetNormals(RadeonRays::float3 const* normals, std::size_t num_normals);
        virtual void SetNormals(float const* normals, std::size_t num_normals);
        virtual std::size_t GetNumNormals() const;
        virtual RadeonRays::float3 const* GetNormals() const;
        
        // Set and get UV array
        virtual void SetUVs(RadeonRays::float2 const* uvs, std::size_t num_uvs);
        virtual void SetUVs(float const* uvs, std::size_t num_uvs);
        virtual std::size_t GetNumUVs() const;
        virtual RadeonRays::float2 const* GetUVs() const;
        
    private:
        std::unique_ptr<RadeonRays::float3[]> m_vertices;
        std::unique_ptr<RadeonRays::float3[]> m_normals;
        std::unique_ptr<RadeonRays::float2[]> m_uvs;
        std::unique_ptr<std::uint32_t> m_indices;
        
        std::size_t m_num_vertices;
        std::size_t m_num_normals;
        std::size_t m_num_uvs;
        std::size_t m_num_indices;
    };
    
    inline Shape::~Shape()
    {
        
    }
    
    inline Shape::Shape() : m_material(nullptr)
    {
        
    }
    
    inline void Shape::SetMaterial(Material const* material)
    {
        m_material = material;
    }
    
    inline Material const* Shape::GetMaterial() const
    {
        return m_material;
    }
}

