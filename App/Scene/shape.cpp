#include "shape.h"
#include <cassert>

namespace Baikal
{
    Mesh::Mesh()
    : m_num_indices(0)
    , m_num_vertices(0)
    , m_num_normals(0)
    , m_num_uvs(0)
    {
    }
    
    void Mesh::SetIndices(std::uint32_t const* indices, std::size_t num_indices)
    {
        assert(indices);
        assert(num_indices != 0);
        
        // Resize internal array and copy data
        m_indices.reset(new std::uint32_t[num_indices]);
        
        std::copy(indices, indices + num_indices, m_indices.get());
        
        m_num_indices = num_indices;
        
        SetDirty(true);
    }
    
    std::size_t Mesh::GetNumIndices() const
    {
        return m_num_indices;
        
    }
    std::uint32_t const* Mesh::GetIndices() const
    {
        return m_indices.get();
    }
    
    void Mesh::SetVertices(RadeonRays::float3 const* vertices, std::size_t num_vertices)
    {
        assert(vertices);
        assert(num_vertices != 0);
        
        // Resize internal array and copy data
        m_vertices.reset(new RadeonRays::float3[num_vertices]);
        
        std::copy(vertices, vertices + num_vertices, m_vertices.get());
        
        m_num_vertices = num_vertices;
        
        SetDirty(true);
    }
    
    void Mesh::SetVertices(float const* vertices, std::size_t num_vertices)
    {
        assert(vertices);
        assert(num_vertices != 0);
        
        // Resize internal array and copy data
        m_vertices.reset(new RadeonRays::float3[num_vertices]);
        
        for (std::size_t i = 0; i < num_vertices; ++i)
        {
            m_vertices[i].x = vertices[3 * i];
            m_vertices[i].y = vertices[3 * i + 1];
            m_vertices[i].z = vertices[3 * i + 2];
            m_vertices[i].w = 1;
        }
        
        m_num_vertices = num_vertices;
        
        SetDirty(true);
    }
    
    std::size_t Mesh::GetNumVertices() const
    {
        return m_num_vertices;
    }
    
    RadeonRays::float3 const* Mesh::GetVertices() const
    {
        return m_vertices.get();
    }
    
    void Mesh::SetNormals(RadeonRays::float3 const* normals, std::size_t num_normals)
    {
        assert(normals);
        assert(num_normals != 0);
        
        // Resize internal array and copy data
        m_normals.reset(new RadeonRays::float3[num_normals]);
        
        std::copy(normals, normals + num_normals, m_normals.get());

        m_num_normals = num_normals;

        SetDirty(true);
    }
    
    void Mesh::SetNormals(float const* normals, std::size_t num_normals)
    {
        assert(normals);
        assert(num_normals != 0);
        
        // Resize internal array and copy data
        m_normals.reset(new RadeonRays::float3[num_normals]);
        
        for (std::size_t i = 0; i < num_normals; ++i)
        {
            m_normals[i].x = normals[3 * i];
            m_normals[i].y = normals[3 * i + 1];
            m_normals[i].z = normals[3 * i + 2];
            m_normals[i].w = 0;
        }
        
        m_num_normals = num_normals;
        
        SetDirty(true);
    }
    
    std::size_t Mesh::GetNumNormals() const
    {
        return m_num_normals;
    }
    
    RadeonRays::float3 const* Mesh::GetNormals() const
    {
        return m_normals.get();
    }
    
    void Mesh::SetUVs(RadeonRays::float2 const* uvs, std::size_t num_uvs)
    {
        assert(uvs);
        assert(num_uvs != 0);
        
        // Resize internal array and copy data
        m_uvs.reset(new RadeonRays::float2[num_uvs]);
        
        std::copy(uvs, uvs + num_uvs, m_uvs.get());
        
        m_num_uvs = num_uvs;
        
        SetDirty(true);
    }
    
    void Mesh::SetUVs(float const* uvs, std::size_t num_uvs)
    {
        assert(uvs);
        assert(num_uvs != 0);
        
        // Resize internal array and copy data
        m_uvs.reset(new RadeonRays::float2[num_uvs]);
        
        for (std::size_t i = 0; i < num_uvs; ++i)
        {
            m_uvs[i].x = uvs[2 * i];
            m_uvs[i].y = uvs[2 * i + 1];
        }
        
        m_num_uvs = num_uvs;
        
        SetDirty(true);
    }
    
    std::size_t Mesh::GetNumUVs() const
    {
        return m_num_uvs;
    }
    
    RadeonRays::float2 const* Mesh::GetUVs() const
    {
        return m_uvs.get();
    }
}
