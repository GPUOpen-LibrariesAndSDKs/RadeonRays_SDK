/**********************************************************************
Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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
#pragma once

#include <map>
#include <string>
#include <vector>

#include "tiny_obj_loader.h"

struct MeshData
{
    MeshData(const std::string& fileName)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> objmaterials;

        std::string warn;
        std::string err;

        bool ret = tinyobj::LoadObj(
            &attrib, &shapes, &objmaterials, &warn, &err, fileName.c_str(), "");

        if (!err.empty())
        {
            throw std::runtime_error(err);
        }

        if (!ret)
        {
            throw std::runtime_error(err);
        }

        Init(shapes.begin(), shapes.end(), attrib);
    }

    MeshData(std::vector<tinyobj::shape_t>::const_iterator begin,
             std::vector<tinyobj::shape_t>::const_iterator end,
             tinyobj::attrib_t const& attrib) 
    {
        Init(begin, end, attrib);
    }

    void Init(std::vector<tinyobj::shape_t>::const_iterator begin,
              std::vector<tinyobj::shape_t>::const_iterator end,
              tinyobj::attrib_t const& attrib)
    {
        struct IndexLess
        {
            bool operator()(tinyobj::index_t const& lhs,
                            tinyobj::index_t const& rhs) const
            {
                return lhs.vertex_index != rhs.vertex_index
                           ? lhs.vertex_index < rhs.vertex_index
                           : lhs.normal_index != rhs.normal_index
                                 ? lhs.normal_index < rhs.normal_index
                                 : lhs.texcoord_index != rhs.texcoord_index
                                       ? lhs.texcoord_index < rhs.texcoord_index
                                       : false;
            }
        };

        std::map<tinyobj::index_t, uint32_t, IndexLess> indexCache;
        uint32_t shapeIndex = 0;
        for (auto it = begin; it < end; it++)
        {
            for (uint32_t i = 0u; i < it->mesh.indices.size(); ++i)
            {
                auto index = it->mesh.indices[i];
                auto iter = indexCache.find(index);
                if (iter != indexCache.cend())
                {
                    indices.push_back(iter->second);
                }
                else
                {
                    uint32_t vertexIndex = (uint32_t)positions.size() / 3;
                    indices.push_back(vertexIndex);
                    indexCache[index] = vertexIndex;

                    positions.push_back(
                        attrib.vertices[3 * index.vertex_index]);
                    positions.push_back(
                        attrib.vertices[3 * index.vertex_index + 1]);
                    positions.push_back(
                        attrib.vertices[3 * index.vertex_index + 2]);
                }
            }
            ++shapeIndex;
        }
    }

    std::vector<float> positions;
    std::vector<std::uint32_t> indices;
};

struct SceneData
{
    SceneData(const std::string& fileName)
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> objmaterials;

        std::string warn;
        std::string err;

        bool ret = tinyobj::LoadObj(
            &attrib, &shapes, &objmaterials, &warn, &err, fileName.c_str(), "");

        if (!err.empty())
        {
            throw std::runtime_error(err);
        }

        if (!ret)
        {
            throw std::runtime_error(err);
        }

        for (auto it = shapes.begin(); it < shapes.end(); it++)
        {
            meshes.emplace_back(MeshData(it, it + 1, attrib));
        }
    }

    std::vector<MeshData> meshes;
};