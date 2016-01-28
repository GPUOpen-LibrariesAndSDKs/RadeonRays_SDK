//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "scene.h"
#include "tiny_obj_loader.h"
#include "sh.h"
#include "shproject.h"
#include <OpenImageIO/imageio.h>

#include <algorithm>
#include <iterator>

using namespace FireRays;

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
        input->read_image(TypeDesc::UINT8, texturedata.get(), sizeof(char)* 4);

        // Close handle
        input->close();

        // Add to texture pool
        data.push_back(std::move(texturedata));
    }
    else if (texture.fmt == Scene::RGBA16)
    {
        texture.size = spec.width * spec.height * spec.depth * sizeof(float)* 2;

        // Resize storage
        std::unique_ptr<char[]> texturedata(new char[spec.width * spec.height * spec.depth * sizeof(float)* 2]);

        // Read data to storage
        input->read_image(TypeDesc::HALF, texturedata.get(), sizeof(float)* 2);

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
	if(res != "")
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
        if (objmaterials[i].name == "carpaint")
        {

            Material diffuse;
            diffuse.kx = float3(0.33f, 0.0f, 0.0f);
            diffuse.ni = 3.0f;
            diffuse.type = kLambert;

            Material specular;
            specular.kx = float3(0.4f, 0.4f, 0.4f);
            specular.ni = 3.0f;
            specular.ns = 0.005f;
            specular.type = kMicrofacetGGX;

			scene->materials_.push_back(diffuse);
			scene->material_names_.push_back(objmaterials[i].name);

            scene->materials_.push_back(specular);
			scene->material_names_.push_back(objmaterials[i].name);

            Material layered;
            layered.ni = 3.0f;
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
        else if (objmaterials[i].name == "rubber" || objmaterials[i].name == "inside")
        {
            Material specular;
            specular.kx = float3(0.4f, 0.4f, 0.4f);
            specular.ni = 1.1f;
            specular.ns = 0.1f;
            specular.type = kMicrofacetGGX;
			specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
			scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "glass" || objmaterials[i].name == "DoorNumber_phongE2SG" || objmaterials[i].name == "case" || objmaterials[i].name == "water")
        {

            Material refract;
            refract.kx = float3(1.f, 1.f, 1.f);
            refract.ni = 1.33f;
            refract.type = kIdealRefract;
			refract.fresnel = 1.f;

            scene->materials_.push_back(refract);
			scene->material_names_.push_back(objmaterials[i].name);

            Material specular;
            specular.kx = float3(1.f, 1.f, 1.f);
            specular.ni = 1.33f;
            specular.ns = 0.001f;
            specular.type = kIdealReflect;
			specular.fresnel = 0.f;

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
			specular.fresnel = 0.f;

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
		else if (objmaterials[i].name == "glasstranslucent")
		{

			Material refract;
			refract.kx = float3(1.f, 1.f, 1.f);
			refract.ni = 1.f;
			refract.type = kIdealRefract;

			scene->materials_.push_back(refract);
			scene->material_names_.push_back(objmaterials[i].name);

			Material specular;
			specular.kx = float3(1.f, 1.f, 1.f);
			specular.ni = 1.33f;
			specular.ns = 0.001f;
			specular.type = kIdealReflect;

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
        else if (objmaterials[i].name == "metal"  )
        {
            Material specular;
            specular.kx = float3(0.9f, 0.9f, 0.9f);
            specular.ni = 5.5f;
            specular.ns = 0.025f;
            specular.type = kMicrofacetGGX;
			specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
			scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "plastic")
        {
            Material specular;
            specular.kx = float3(0.4f, 0.4f, 0.4f);
            specular.ni = 1.3f;
            specular.ns = 0.1f;
            specular.type = kMicrofacetGGX;
			specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
			scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "chrome" || objmaterials[i].name == "wire_225143087")
        {
            Material specular;
            specular.kx = 1.3f * float3(0.58f, 0.3f, 0.1f);
            specular.ni = 580.5f;
            specular.ns = 0.0025f;
            specular.type = kMicrofacetGGX;
			//specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
			scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "brakes")
        {
            Material specular;
            specular.kx = float3(0.8f, 0.8f, 0.8f);
            specular.ni = 1.5f;
            specular.ns = 0.1f;
            specular.type = kIdealReflect;
			specular.fresnel = 1.f;

            scene->materials_.push_back(specular);
			scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
        else if (objmaterials[i].name == "asphalt" || objmaterials[i].name == "floor1")
        {
            Material specular;
            specular.kx = float3(0.5f, 0.5f, 0.5f);
            specular.ni = 1.5f;
            specular.ns = 0.1f;
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


            scene->materials_.push_back(specular);
			scene->material_names_.push_back(objmaterials[i].name);
            matmap[i] = scene->materials_.size() - 1;
            continue;
        }
		else if (objmaterials[i].name == "light" || objmaterials[i].name == "wire_088144226")
		{
			Material emissive;
			emissive.kx = float3(30.5f, 28.5f, 24.5f);
			emissive.type = kEmissive;
			emissive.fresnel = 1.f;

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
            if (!objmaterials[i].normal_texname.empty())
            {
                auto iter = textures.find(objmaterials[i].normal_texname);
                if (iter != textures.end())
                {
                    material.nmapidx = iter->second;
                }
                else
                {
                    Texture texture;

                    // Load texture
                    LoadTexture(basepath + "/" + objmaterials[i].normal_texname, texture, scene->texturedata_);

                    // Add texture desc
                    material.nmapidx = (int)scene->textures_.size();
                    scene->textures_.push_back(texture);

                    // Save in the map
                    textures[objmaterials[i].normal_texname] = material.nmapidx;
                }
            }


            scene->materials_.push_back(material);
			scene->material_names_.push_back(objmaterials[i].name);

			float3 spec = float3(0.6f, 0.6f, 0.6f);// objmaterials[i].specular[0], objmaterials[i].specular[1], objmaterials[i].specular[2]);
            if (spec.sqnorm() > 0.f)
            {
                Material specular;
                specular.kx = spec;
                specular.ni = 2.5f;//objmaterials[i].ior;
                specular.ns = 0.05f;//1.f - objmaterials[i].shininess;
                specular.type = kMicrofacetGGX;
                specular.nmapidx = scene->materials_.back().nmapidx;
				specular.fresnel = 1.f;

                scene->materials_.push_back(specular);
				scene->material_names_.push_back(objmaterials[i].name);

                Material layered;
                layered.ni = 2.5f;// objmaterials[i].ior;
                layered.type = kFresnelBlend;
                layered.brdftopidx = scene->materials_.size() - 1;
                layered.brdfbaseidx = scene->materials_.size() - 2;
				layered.fresnel = 1.f;

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
        shape.m = rotation_y(3.14);
        shape.linearvelocity = float3(0.0f, 0.f, 0.f);
        shape.angularvelocity = quaternion(0.f, 0.f, 0.f, 1.f);
        // Save last index to add to this shape indices
        // int baseidx = (int)scene->vertices_.size();

        // Enumerate and copy vertex data
        for (int i = 0; i < (int)objshapes[s].mesh.positions.size() / 3; ++i)
        {
            scene->vertices_.push_back(float3(objshapes[s].mesh.positions[3 * i], objshapes[s].mesh.positions[3 * i + 1], objshapes[s].mesh.positions[3 * i + 2]));
        }

        for (int i = 0; i < (int)objshapes[s].mesh.normals.size() / 3; ++i)
        {
            scene->normals_.push_back(float3(objshapes[s].mesh.normals[3 * i], objshapes[s].mesh.normals[3 * i + 1], objshapes[s].mesh.normals[3 * i + 2]));
        }

        for (int i = 0; i < (int)objshapes[s].mesh.texcoords.size() / 2; ++i)
        {
            scene->uvs_.push_back(float2(objshapes[s].mesh.texcoords[2 * i], objshapes[s].mesh.texcoords[2 * i + 1]));
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
				Emissive emissive;
				emissive.shapeidx = s;
				emissive.primidx = i;
				emissive.m = matidx;
                scene->emissives_.push_back(emissive);
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

    std::cout << "Loading complete\n";
    std::cout << "Number of objects: " << scene->shapes_.size() << "\n";
    std::cout << "Number of textures: " << scene->textures_.size() << "\n";
	std::cout << "Number of emissives: " << scene->emissives_.size() << "\n";

    return scene;
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
