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
#include <fstream>
#include <iostream>
#include <vector>

#include "bvh.h"
#include "config.h"
#include "transform.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace
{
size_t GetStreamLength(std::istream& is)
{
    is.seekg(0, is.end);
    size_t length = is.tellg();
    is.seekg(0, is.beg);

    return length;
}
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cout << "Provide path to config as an application parameter" << std::endl;
        exit(1);
    }
    bvh::QualityStats stats;
    try
    {
        bvh::Config cfg(argv[1]);
        std::cout << cfg << std::endl;
        // try open bvh file
        std::ifstream in_bvh(cfg.binary_bvh_filename, std::ifstream::binary);
        if (!in_bvh.is_open())
        {
            throw std::runtime_error("Incorrect path to bvh file");
        }
        // read rays
        std::ifstream in_ray(cfg.binary_rays_filename.c_str(), std::ifstream::binary);
        if (!in_ray.is_open())
        {
            throw std::runtime_error("Incorrect path to rays file");
        }
        if (GetStreamLength(in_ray) < cfg.ray_width * cfg.ray_height * sizeof(bvh::Ray))
        {
            throw std::runtime_error("Rays file contains less elements than declared");
        }

        std::vector<bvh::Ray> rays(cfg.ray_width * cfg.ray_height);
        in_ray.read((char*)rays.data(), rays.size() * sizeof(bvh::Ray));
        std::vector<bvh::Hit> hits;

        if (cfg.type == bvh::BvhType::kVkBvh2)
        {
            std::vector<bvh::VkBvhNode> vk_bvh(cfg.internal_size + cfg.triangle_size);
            if (GetStreamLength(in_bvh) < cfg.internal_size + cfg.triangle_size * sizeof(bvh::VkBvhNode))
            {
                throw std::runtime_error("Bvh file contains less nodes than declared");
            }
            in_bvh.read((char*)vk_bvh.data(), vk_bvh.size() * sizeof(bvh::VkBvhNode));
            bvh::Bvh<2u> bvh2(cfg.internal_size, cfg.triangle_size);
            bvh2.TransformBvh(bvh::Transform2<bvh::VkBvhNode>, vk_bvh.data(), nullptr);
            stats = bvh2.CheckQuality(rays, cfg.ray_width, cfg.ray_height, bvh::QueryType::kClosestHit, hits);
        }
        in_bvh.close();
        in_ray.close();
    } catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        exit(1);
    }

    std::cout << stats << std::endl;

    return 0;
}