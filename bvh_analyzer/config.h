/**********************************************************************
Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.

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
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

namespace bvh
{
enum class BvhType
{
    kBvh2,
    kVkBvh2,
    kDx12Bvh2
};

namespace
{
static std::map<std::string, BvhType> s_str_to_type = {{"bvh2", BvhType::kBvh2},
                                                       {"vkbvh2", BvhType::kVkBvh2},
                                                       {"dx12bvh2", BvhType::kDx12Bvh2}};
}

struct Config
{
    Config(char const* filename)
    {
        try
        {
            std::ifstream input(filename, std::ifstream::in);
            std::getline(input, binary_bvh_filename);
            std::string type_str, internal, tris, w, h;
            std::getline(input, type_str);
            type = s_str_to_type.at(type_str);
            std::getline(input, internal);
            internal_size = std::stoul(internal);
            std::getline(input, tris);
            triangle_size = std::stoul(tris);
            std::getline(input, binary_rays_filename);
            std::getline(input, w);
            ray_width = std::stoul(w);
            std::getline(input, h);
            ray_height = std::stoul(h);
        } catch (...)
        {
            throw std::runtime_error(
                "Incorrect config format.\nCorrect format "
                "is:\nbvh_path\nbvh_type_str\nnum_internal_nodes\nnum_triangles\nrays_path\nray_width\nray_height");
        }
    }
    std::string binary_bvh_filename;
    std::string binary_rays_filename;
    BvhType     type;
    uint32_t    internal_size;
    uint32_t    triangle_size;
    uint32_t    ray_width;
    uint32_t    ray_height;
};

inline std::ostream& operator<<(std::ostream& oss, const Config& cfg)
{
    oss << "Bvh file: " << cfg.binary_bvh_filename << std::endl;
    oss << "Internal nodes: " << cfg.internal_size << std::endl;
    oss << "Triangles: " << cfg.triangle_size << std::endl;
    oss << "Rays file: " << cfg.binary_rays_filename << std::endl;
    oss << "Ray count: " << cfg.ray_width * cfg.ray_height << std::endl;

    return oss;
}

}  // namespace bvh