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
#include "Scene/scene.h"
#include "tiny_obj_loader.h"
#include "sh.h"
#include "shproject.h"
#include "OpenImageIO/imageio.h"
#include "Light/Ibl.h"

#include <algorithm>
#include <iterator>

namespace Baikal
{

using namespace RadeonRays;

static int GetTextureForemat(OIIO_NAMESPACE::ImageSpec const& spec)
{
    OIIO_NAMESPACE_USING

        if (spec.format.basetype == TypeDesc::UINT8)
            return Scene::RGBA8;
    if (spec.format.basetype == TypeDesc::HALF)
        return Scene::RGBA16;
    else
        return Scene::RGBA32;
}


static void LoadTexture(std::string const& filename, Scene::Texture& texture, std::vector<std::unique_ptr<char[]> >& data)
{
    OIIO_NAMESPACE_USING

    ImageInput* input = ImageInput::open(filename);

    if (!input)
    {
        throw std::runtime_error("Can't load " + filename + " image");
    }

    ImageSpec const& spec = input->spec();

    texture.w = spec.width;
    texture.h = spec.height;
    texture.d = spec.depth;
    texture.fmt = GetTextureForemat(spec);

    // Save old size for reading offset
    texture.dataoffset = (int)data.size();

    if (texture.fmt == Scene::RGBA8)
    {
        texture.size = spec.width * spec.height * spec.depth * 4;

        // Resize storage
        std::unique_ptr<char[]> texturedata(new char[spec.width * spec.height * spec.depth * 4]);

        // Read data to storage
        input->read_image(TypeDesc::UINT8, texturedata.get(), sizeof(char) * 4);

        // Close handle
        input->close();

        // Add to texture pool
        data.push_back(std::move(texturedata));
    }
    else if (texture.fmt == Scene::RGBA16)
    {
        texture.size = spec.width * spec.height * spec.depth * sizeof(float) * 2;

        // Resize storage
        std::unique_ptr<char[]> texturedata(new char[spec.width * spec.height * spec.depth * sizeof(float) * 2]);

        // Read data to storage
        input->read_image(TypeDesc::HALF, texturedata.get(), sizeof(float) * 2);

        // Close handle
        input->close();

        // Add to texture pool
        data.push_back(std::move(texturedata));
    }
    else
    {
        texture.size = spec.width * spec.height * spec.depth * sizeof(float3);

        // Resize storage
        std::unique_ptr<char[]> texturedata(new char[spec.width * spec.height * spec.depth * sizeof(float3)]);

        // Read data to storage
        input->read_image(TypeDesc::FLOAT, texturedata.get(), sizeof(float3));

        // Close handle
        input->close();

        // Add to texture pool
        data.push_back(std::move(texturedata));
    }

    // Cleanup
    delete input;
}

Scene* Scene::LoadFromObj(std::string const& filename, std::string const& basepath)
{
    using namespace tinyobj;

    // Loader data
    std::vector<shape_t> objshapes;
    std::vector<material_t> objmaterials;

    // Try loading file
    std::string res = LoadObj(objshapes, objmaterials, filename.c_str(), basepath.c_str());
    if (res != "")
    {
        throw std::runtime_error(res);
    }

    // Allocate scene
    Scene* scene(new Scene);

    // Texture map
    std::map<std::string, int> textures;
    std::map<int, int> matmap;

    // Enumerate and translate materials
    for (int i = 0; i < (int)objmaterials.size(); ++i)
    {
        if (objmaterials[i].name == "carpaint" || objmaterials[i].name == "inside1" || objmaterials[i].name == "CarShellNew")
        {
	    Material diffuse;
            diffuse.kx = float3(0.787f, 0.081f, 0.027f);
            diffuse.ni = 1.5f;
            diffuse.ns = 0.3f;
            diffuse.type = kMicrofacetBeckmann;

            Material specular;
            specular.kx = float3(0.99f, 0.99f, 0.99f);
            specular.ni = 1.5f;
            specular.ns = 0.01f;
            specular.type = kMicrofacetBeckmann;

            scene->materials_.push_back(diffuse);
            scene->material_names_.push_back(objmaterials[i].name);

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 1.5f;
            layered.type = kFresnelBlend;
            layered.brdfbaseidx = scene->materials_.size() - 2;
            layered.brdftopidx = scene->materials_.size() - 1;


            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);


            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "wire_088144225")
        {

            Material diffuse;
            diffuse.kx = float3(0.7f, 0.7f, 0.7f);
            diffuse.ni = 1.6f;
            diffuse.type = kLambert;

            Material specular;
            specular.kx = float3(0.9f, 0.9f, 0.9f);
            specular.ni = 1.6f;
            specular.ns = 0.005f;
            specular.type = kMicrofacetGGX;

            scene->materials_.push_back(diffuse);
            scene->material_names_.push_back(objmaterials[i].name);

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 1.6f;
            layered.type = kFresnelBlend;
            layered.brdfbaseidx = scene->materials_.size() - 2;
            layered.brdftopidx = scene->materials_.size() - 1;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "rubber" || objmaterials[i].name == "Rubber_Dark")
        {
            Material specular;
            specular.kx = float3(0.4f, 0.4f, 0.4f);
            specular.ni = 2.1f;
            specular.ns = 0.1f;
            specular.type = kMicrofacetGGX;
            specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "sss")
        {
            /*Material refract;
            refract.type = kIdealRefract;
            refract.kx = float3(0.1f, 0.5f, 0.1f);
            refract.ni = 1.3f;
            refract.fresnel = 1.f;

            scene->materials_.push_back(refract);
            scene->material_names_.push_back(objmaterials[i].name);*/

            Material specular;
            specular.kx = float3(0.7f, 0.7f, 0.7f);
            specular.ni = 1.3f;
            specular.ns = 0.1f;
            specular.type = kMicrofacetGGX;
            specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            /*Material layered;
            layered.ni = 1.3f;
            layered.type = kFresnelBlend;
            layered.brdftopidx = scene->materials_.size() - 1;
            layered.brdfbaseidx = scene->materials_.size() - 2;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);*/
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "glass"  || objmaterials[i].name == "water")
        {

            Material refract;
            refract.kx = float3(1.f, 1.f, 1.f);
            refract.ni = 1.33f;
            refract.type = kIdealRefract;
            refract.fresnel = 1.f;

            if (!objmaterials[i].normal_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].normal_texname);
                if (iter != textures.end())
                {
                    refract.nmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].normal_texname, texture, scene->texturedata_);

                    // Add texture desc
                    refract.nmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].normal_texname] = refract.nmapidx;
                }
            }

            scene->materials_.push_back(refract);
            scene->material_names_.push_back(objmaterials[i].name);

            Material specular;
            specular.kx = float3(1.f, 1.f, 1.f);
            specular.ni = 1.33f;
            specular.ns = 0.001f;
            specular.type = kIdealReflect;
            specular.fresnel = 1.f;
            specular.nmapidx = refract.nmapidx;



            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 1.33f;
            layered.type = kFresnelBlend;
            layered.brdftopidx = scene->materials_.size() - 1;
            layered.brdfbaseidx = scene->materials_.size() - 2;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "phong4SG" || objmaterials[i].name == "Material.007" || objmaterials[i].name == "glass_architectural")
        {

            Material refract;
            refract.kx = float3(0.83f, 1.f, 0.92f);
            refract.ns = 0.2f;
            refract.ni = 1.33f;
            refract.type = kMicrofacetRefractionGGX;
            refract.fresnel = 0.f;

            auto iter = textures.find("checker.png");
            if (iter != textures.end())
            {
                refract.nsmapidx = iter->second;
            }
            else
            {
                Texture texture;

                // Load texture
                LoadTexture(basepath + "/checker.png", texture, scene->texturedata_);

                // Add texture desc
                refract.nsmapidx = (int)scene->textures_.size();
                scene->textures_.push_back(texture);

                // Save in the map
                textures["checker.png"] = refract.nsmapidx;
            }


            scene->materials_.push_back(refract);
            scene->material_names_.push_back(objmaterials[i].name);

            Material specular;
            specular.kx = float3(0.83f, 1.f, 0.92f);
            specular.ni = 1.33f;
            specular.ns = 0.2f;
            specular.type = kMicrofacetGGX;
            specular.fresnel = 1.f;
            specular.nsmapidx = refract.nsmapidx;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 1.33f;
            layered.type = kFresnelBlend;
            layered.brdftopidx = scene->materials_.size() - 1;
            layered.brdfbaseidx = scene->materials_.size() - 2;
            //layered.twosided = 1;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;

            continue;
        }
        else if (objmaterials[i].name == "Test_Material.003")
        {
            Material specular;
            specular.kx = float3(1.f, 1.f, 1.f);
            specular.ni = 1.33f;
            specular.ns = 0.001f;
            specular.type = kIdealReflect;
            specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            Material refract;
            refract.kx = float3(1.f, 1.f, 1.f);
            refract.ni = 1.33f;
            refract.type = kIdealRefract;
            refract.fresnel = 1.f;

            scene->materials_.push_back(refract);
            scene->material_names_.push_back(objmaterials[i].name);

            Material diffuse;
            diffuse.kx = float3(0.f, 0.7f, 0.f);
            diffuse.ni = 1.33f;
            diffuse.ns = 0.001f;
            diffuse.type = kLambert;
            diffuse.fresnel = 1.f;

            scene->materials_.push_back(diffuse);
            scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 2.33f;
            layered.type = kFresnelBlend;
            layered.brdftopidx = scene->materials_.size() - 1;
            layered.brdfbaseidx = scene->materials_.size() - 2;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);

            layered.ni = 1.33f;
            layered.type = kFresnelBlend;
            layered.brdftopidx = scene->materials_.size() - 4;
            layered.brdfbaseidx = scene->materials_.size() - 1;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);

            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "red_glass" || objmaterials[i].name == "Tailight")
        {

            Material refract;
            refract.kx = float3(1.f, 0.3f, 0.3f);
            refract.ni = 1.33f;
            refract.type = kIdealRefract;
            refract.fresnel = 1.f;

            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    refract.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    refract.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = refract.kxmapidx;
                }
            }

            scene->materials_.push_back(refract);
            scene->material_names_.push_back(objmaterials[i].name);

            Material specular;
            specular.kx = float3(1.f, 0.3f, 0.3f);
            specular.ni = 1.33f;
            specular.ns = 0.001f;
            specular.type = kIdealReflect;
            specular.fresnel = 1.f;
            specular.kxmapidx = refract.kxmapidx;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 1.33f;
            layered.type = kFresnelBlend;
            layered.brdftopidx = scene->materials_.size() - 1;
            layered.brdfbaseidx = scene->materials_.size() - 2;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "wire_113135006")
        {

            Material refract;
            refract.kx = float3(1.f, 1.f, 1.f);
            refract.ni = 2.66f;
            refract.type = kIdealRefract;
            refract.fresnel = 0.f;

            scene->materials_.push_back(refract);
            scene->material_names_.push_back(objmaterials[i].name);

            Material specular;
            specular.kx = float3(1.f, 1.f, 1.f);
            specular.ni = 2.66f;
            specular.ns = 0.001f;
            specular.type = kIdealReflect;
            specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 2.66f;
            layered.type = kFresnelBlend;
            layered.brdftopidx = scene->materials_.size() - 1;
            layered.brdfbaseidx = scene->materials_.size() - 2;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "glasstranslucent" || objmaterials[i].name == "glass1" || objmaterials[i].name == "Glass" || objmaterials[i].name == "HeadlightGlass")
        {

            Material refract;
            refract.kx = float3(0.8f, 0.8f, 0.8f);
            refract.ni = 1.f;
            refract.type = kIdealRefract;

            scene->materials_.push_back(refract);
            scene->material_names_.push_back(objmaterials[i].name);

            Material specular;
            specular.kx = float3(0.8f, 0.8f, 0.8f);
            specular.ni = 1.2f;
            specular.type = kIdealReflect;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 1.2f;
            layered.type = kFresnelBlend;
            layered.twosided = 1;
            layered.brdftopidx = scene->materials_.size() - 1;
            layered.brdfbaseidx = scene->materials_.size() - 2;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "metal" || objmaterials[i].name == "Base_Motor_mat" || objmaterials[i].name == "Base_Motor_mat1"
            || objmaterials[i].name == "Base_Motor_mat2" || objmaterials[i].name == "Base_Motor_mat3" || objmaterials[i].name == "ChromeMatte")
        {
            Material specular;
            specular.kx = float3(0.9f, 0.9f, 0.9f);
            specular.ni = 5.5f;
            specular.ns = 0.15f;
            specular.type = kMicrofacetGGX;
            specular.fresnel = 1.f;

            if (!objmaterials[i].normal_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].normal_texname);
                if (iter != textures.end())
                {
                    specular.nmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].normal_texname, texture, scene->texturedata_);

                    // Add texture desc
                    specular.nmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].normal_texname] = specular.nmapidx;
                }
            }

            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    specular.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    specular.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = specular.kxmapidx;
                }
            }


                //auto iter = textures.find("rm.jpg");
                //if (iter != textures.end())
                //{
                //    specular.kxmapidx = iter->second;
                //}
                //else
                //{
                //    Texture texture;

                //    // Load texture
                //    LoadTexture(basepath + "/rm.jpg", texture, scene->texturedata_);

                //    // Add texture desc
                //    specular.nsmapidx= (int)scene->textures_.size();
                //    scene->textures_.push_back(texture);

                //    // Save in the map
                //    textures["rm.jpg"] = specular.nsmapidx;
                //}

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "plastic1" || objmaterials[i].name == "inside1" )
        {
            Material specular;
            specular.kx = float3(0.6f, 0.6f, 0.6f);
            specular.ni = 1.4f;
            specular.ns = 0.05f;
            specular.type = kMicrofacetGGX;
            specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "mirror")
        {
            Material specular;
            specular.kx = float3(1.f, 1.f, 1.f);
            specular.ni = 1.4f;
            specular.ns = 0.05f;
            specular.type = kIdealReflect;
            specular.fresnel = 0.f;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "fabric")
        {
            Material specular;
            specular.kx = float3(0.6f, 0.6f, 0.6f);
            specular.ni = 1.3f;
            specular.ns = 0.3f;
            specular.type = kMicrofacetGGX;
            specular.fresnel = 1.f;

            if (!objmaterials[i].normal_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].normal_texname);
                if (iter != textures.end())
                {
                    specular.nmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].normal_texname, texture, scene->texturedata_);

                    // Add texture desc
                    specular.nmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].normal_texname] = specular.nmapidx;
                }
            }

            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    specular.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    specular.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = specular.kxmapidx;
                }
            }

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "leather")
        {
            Material specular;
            specular.kx = float3(0.7f, 0.7f, 0.7f);
            specular.ni = 1.1f;
            specular.ns = 0.03f;
            specular.type = kMicrofacetGGX;
            specular.fresnel = 1.f;

            if (!objmaterials[i].normal_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].normal_texname);
                if (iter != textures.end())
                {
                    specular.nmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].normal_texname, texture, scene->texturedata_);

                    // Add texture desc
                    specular.nmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].normal_texname] = specular.nmapidx;
                }
            }
            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    specular.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    specular.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = specular.kxmapidx;
                }
            }

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "pillow")
        {
            Material specular;
            specular.kx = float3(0.95f, 0.95f, 0.95f);
            specular.ni = 1.3f;
            specular.ns = 0.3f;
            specular.type = kLambert;
            specular.fresnel = 0.f;

            if (!objmaterials[i].normal_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].normal_texname);
                if (iter != textures.end())
                {
                    specular.nmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].normal_texname, texture, scene->texturedata_);

                    // Add texture desc
                    specular.nmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].normal_texname] = specular.nmapidx;
                }
            }

            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    specular.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    specular.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = specular.kxmapidx;
                }
            }

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "chrome" || objmaterials[i].name == "wire_225143087" || objmaterials[i].name == "HeadlightChrome" || objmaterials[i].name == "Chrome")
        {
            Material specular;
            specular.kx = float3(0.99f, 0.99f, 0.99f);
            specular.ni = 10.5f;
            specular.ns = 0.025f;
            specular.type = kMicrofacetGGX;
            specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "brakes")
        {
            Material diffuse;
            diffuse.kx = float3(0.f, 0.f, 0.f);
            diffuse.ni = 1.3f;
            diffuse.type = kLambert;

            Material specular;
            specular.kx = float3(0.9f, 0.9f, 0.9f);
            specular.ni = 1.3f;
            specular.ns = 0.005f;
            specular.type = kMicrofacetGGX;

            scene->materials_.push_back(diffuse);
            scene->material_names_.push_back(objmaterials[i].name);

            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 1.3f;
            layered.type = kFresnelBlend;
            layered.brdfbaseidx = scene->materials_.size() - 2;
            layered.brdftopidx = scene->materials_.size() - 1;

            scene->materials_.push_back(layered);
            scene->material_names_.push_back(objmaterials[i].name);

            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "asphalt" || objmaterials[i].name == "Floor")
        {
            Material specular;
            specular.kx = 0.5f * float3(0.073f, 0.085f, 0.120f);
            specular.ni = 1.3f;
            specular.ns = 0.25f;
            specular.type = kMicrofacetGGX;
            specular.fresnel = 0.f;
            specular.twosided = 1;

            if (!objmaterials[i].normal_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].normal_texname);
                if (iter != textures.end())
                {
                    specular.nmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].normal_texname, texture, scene->texturedata_);

                    // Add texture desc
                    specular.nmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].normal_texname] = specular.nmapidx;
                }
            }


            scene->materials_.push_back(specular);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "light" || objmaterials[i].name == "Emit1" || objmaterials[i].name == "Light3" || objmaterials[i].name == "dayLight_portal")
        {
            Material emissive;
            emissive.kx = 10.f * float3(0.8f, 0.8f, 0.8f);
            emissive.type = kEmissive;

            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    emissive.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    emissive.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = emissive.kxmapidx;
                }
            }


            scene->materials_.push_back(emissive);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        } // 
        else if (objmaterials[i].name == "wire_138008110")
        {
            Material emissive;
            emissive.kx = 5.f * float3(0.8f, 0.8f, 0.7f);
            emissive.type = kEmissive;

            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    emissive.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    emissive.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = emissive.nmapidx;
                }
            }


            scene->materials_.push_back(emissive);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        } // wire_138008110
        else if (objmaterials[i].name == "HeadLightAngelEye")
        {
            Material emissive;
            emissive.kx = 50.f * float3(0.53f, 0.7f, 0.95f);
            emissive.type = kLambert;

            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    emissive.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    emissive.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = emissive.nmapidx;
                }
            }


            scene->materials_.push_back(emissive);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "HeadLightGlass")
        {
            Material emissive;
            emissive.kx = 50.f * float3(0.64f, 0.723f, 0.8f);
            emissive.type = kLambert;

            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    emissive.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    emissive.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = emissive.nmapidx;
                }
            }


            scene->materials_.push_back(emissive);
            scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else
        {
            Material material;

            material.kx = float3(objmaterials[i].diffuse[0], objmaterials[i].diffuse[1], objmaterials[i].diffuse[2]);
            material.ni = objmaterials[i].ior;
            material.type = kLambert;
            material.fresnel = 0.f;

            // Load diffuse texture if needed
            if (!objmaterials[i].diffuse_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].diffuse_texname);
                if (iter != textures.end())
                {
                    material.kxmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].diffuse_texname, texture, scene->texturedata_);

                    // Add texture desc
                    material.kxmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].diffuse_texname] = material.kxmapidx;
                }
            }

            // Load specular texture
            /*if (!objmaterials[i].specular_texname.empty())
             {
             auto iter = textures.find(objmaterials[i].specular_texname);
             if (iter != textures.end())
             {
             material.ksmapidx = iter->second;
             }
             else
             {
             Texture texture;

             // Load texture
             LoadTexture(basepath + "/" + objmaterials[i].specular_texname, texture, scene->texturedata_);

             // Add texture desc
             material.ksmapidx = (int)scene->textures_.size();
             scene->textures_.push_back(texture);

             // Save in the map
             textures[objmaterials[i].specular_texname] = material.ksmapidx;
             }
             }*/

             // Load normal map



            scene->materials_.push_back(material);
            scene->material_names_.push_back(objmaterials[i].name);

            float3 spec = float3(0.5f, 0.5f, 0.5f);// float3(objmaterials[i].specular[0], objmaterials[i].specular[1], objmaterials[i].specular[2]);
            if (spec.sqnorm() > 0.f)
            {
                Material specular;
                specular.kx = spec;
                specular.ni = 1.33f;//objmaterials[i].ior;
                specular.ns = 0.05f;//1.f - objmaterials[i].shininess;
                specular.type = kMicrofacetGGX;
                specular.nmapidx = -1;// scene->materials_.back().nmapidx;
                specular.fresnel = 1.f;

                if (!objmaterials[i].normal_texname.empty())
                {
                    auto iter = textures.find(objmaterials[i].normal_texname);
                    if (iter != textures.end())
                    {
                        specular.nmapidx = iter->second;
                    }
                    else
                    {
                        Texture texture;

                        // Load texture
                        LoadTexture(basepath + "/" + objmaterials[i].normal_texname, texture, scene->texturedata_);

                        // Add texture desc
                        specular.nmapidx = (int)scene->textures_.size();
                        scene->textures_.push_back(texture);

                        // Save in the map
                        textures[objmaterials[i].normal_texname] = specular.nmapidx;
                    }
                }

                scene->materials_.push_back(specular);
                scene->material_names_.push_back(objmaterials[i].name);

                Material layered;
                layered.ni = 1.9f;// objmaterials[i].ior;
                layered.type = kFresnelBlend;
                layered.brdftopidx = scene->materials_.size() - 1;
                layered.brdfbaseidx = scene->materials_.size() - 2;
                layered.fresnel = 1.f;
                layered.twosided = 1;

                scene->materials_.push_back(layered);
                scene->material_names_.push_back(objmaterials[i].name);
            }

            matmap[i] = scene->materials_.size() - 1;
        }
    }

    // Enumerate all shapes in the scene
    for (int s = 0; s < (int)objshapes.size(); ++s)
    {
        // Prepare shape
        Shape shape;
        shape.startidx = (int)scene->indices_.size();
        shape.numprims = (int)objshapes[s].mesh.indices.size() / 3;
        shape.startvtx = (int)scene->vertices_.size();
        shape.numvertices = (int)objshapes[s].mesh.positions.size() / 3;
        shape.m = matrix();
        shape.linearvelocity = float3(0.0f, 0.f, 0.f);
        shape.angularvelocity = quaternion(0.f, 0.f, 0.f, 1.f);
        // Save last index to add to this shape indices
        // int baseidx = (int)scene->vertices_.size();

        int pos_count = (int)objshapes[s].mesh.positions.size() / 3;
        // Enumerate and copy vertex data
        for (int i = 0; i < pos_count; ++i)
        {
            scene->vertices_.push_back(float3(objshapes[s].mesh.positions[3 * i], objshapes[s].mesh.positions[3 * i + 1], objshapes[s].mesh.positions[3 * i + 2]));
        }

        for (int i = 0; i < (int)objshapes[s].mesh.normals.size() / 3; ++i)
        {
            scene->normals_.push_back(float3(objshapes[s].mesh.normals[3 * i], objshapes[s].mesh.normals[3 * i + 1], objshapes[s].mesh.normals[3 * i + 2]));
        }

        //check UV
        int texcoords_count = objshapes[s].mesh.texcoords.size() / 2;
        if (texcoords_count == pos_count)
        {
            for (int i = 0; i < texcoords_count; ++i)
            {
                float2 uv = float2(objshapes[s].mesh.texcoords[2 * i], objshapes[s].mesh.texcoords[2 * i + 1]);
                scene->uvs_.push_back(uv);
            }
        }
        else
        {
            for (int i = 0; i < pos_count; ++i)
            {
                scene->uvs_.push_back(float2(0, 0));
            }
        }

        // Enumerate and copy indices (accounting for base index) and material indices
        for (int i = 0; i < (int)objshapes[s].mesh.indices.size() / 3; ++i)
        {
            scene->indices_.push_back(objshapes[s].mesh.indices[3 * i]);
            scene->indices_.push_back(objshapes[s].mesh.indices[3 * i + 1]);
            scene->indices_.push_back(objshapes[s].mesh.indices[3 * i + 2]);

            int matidx = matmap[objshapes[s].mesh.material_ids[i]];
            scene->materialids_.push_back(matidx);

            if (scene->materials_[matidx].type == kEmissive)
            {
                Light light;
                light.type = kArea;
                light.shapeidx = s;
                light.primidx = i;
                light.matidx = matidx;
                scene->lights_.push_back(light);
            }
        }

        scene->shapes_.push_back(shape);
    }

    // Check if there is no UVs
    if (scene->uvs_.empty())
    {
        scene->uvs_.resize(scene->normals_.size());
        std::fill(scene->uvs_.begin(), scene->uvs_.end(), float2(0, 0));
    }

    scene->envidx_ = -1;

    return scene;
}

void Scene::AddDirectionalLight(RadeonRays::float3 const& d, RadeonRays::float3 const& e)
{
    Light light;
    light.type = kDirectional;
    light.d = d;
    light.intensity = e;
    lights_.push_back(light);
}

void Scene::SetEnvironment(std::string const& filename, std::string const& basepath, float envmapmul)
{
    // Save multiplier
    envmapmul_ = envmapmul;

    Texture texture;

    // Load texture
    if (!basepath.empty())
    {
        LoadTexture(basepath + "/" + filename, texture, texturedata_);
    }
    else
    {
        LoadTexture(filename, texture, texturedata_);
    }

    //
    //Ibl* ibl = new Ibl((float3*)(texturedata_[texture.dataoffset].get()), texture.w, texture.h);
    //ibl->Simulate("pdf.png");


    // Save index
    envidx_ = (int)textures_.size();

    // Add texture desc
    textures_.push_back(texture);
}

void Scene::SetBackground(std::string const& filename, std::string const& basepath)
{
    Texture texture;

    // Load texture
    if (!basepath.empty())
    {
        LoadTexture(basepath + "/" + filename, texture, texturedata_);
    }
    else
    {
        LoadTexture(filename, texture, texturedata_);
    }

    // Save index
    bgimgidx_ = (int)textures_.size();

    // Add texture desc
    textures_.push_back(texture);
}
}
