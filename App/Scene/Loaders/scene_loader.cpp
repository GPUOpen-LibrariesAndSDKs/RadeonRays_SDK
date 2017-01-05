#include "scene_loader.h"
#include "../scene1.h"
#include "../shape.h"
#include "../material.h"
#include "../light.h"

#include "tiny_obj_loader.h"

namespace Baikal
{
    Scene1* LoadFromObj(std::string const& filename, std::string const& basepath)
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
        Scene1* scene(new Scene1);
        
        // Texture map
        std::map<std::string, int> textures;
        std::map<int, int> matmap;
        
        // Enumerate and translate materials
        /*for (int i = 0; i < (int)objmaterials.size(); ++i)
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
                if (!objmaterials[i].specular_texname.empty())
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
                 }
                
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
        }*/
        
        Material* fakemat = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
        fakemat->SetTwoSided(false);
        
        // Enumerate all shapes in the scene
        for (int s = 0; s < (int)objshapes.size(); ++s)
        {
            Mesh* mesh = new Mesh();
            
            auto num_vertices = objshapes[s].mesh.positions.size() / 3;
            mesh->SetVertices(&objshapes[s].mesh.positions[0], num_vertices);
            
            auto num_normals = objshapes[s].mesh.normals.size() / 3;
            mesh->SetNormals(&objshapes[s].mesh.normals[0], num_normals);
            
            auto num_uvs = objshapes[s].mesh.texcoords.size() / 2;
            
            if (num_uvs)
            {
                mesh->SetUVs(&objshapes[s].mesh.texcoords[0], num_uvs);
            }
            else
            {
                std::vector<RadeonRays::float2> zero(num_vertices);
                mesh->SetUVs(&zero[0], num_vertices);
            }
            
            auto num_indices = objshapes[s].mesh.indices.size();
            mesh->SetIndices(reinterpret_cast<std::uint32_t const*>(&objshapes[s].mesh.indices[0]), num_indices);
            
            mesh->SetMaterial(fakemat);
            scene->AttachShape(mesh);
        }
        
        //ImageBasedLight* light = new ImageBasedLight();
        //g_scene->SetEnvironment(g_envmapname, "", g_envmapmul);
        
        PointLight* light = new PointLight();
        light->SetPosition(RadeonRays::float3(-0.3f, 1.5f, 2.f));
        light->SetEmittedRadiance(2.f * RadeonRays::float3(1.f, 1.f, 1.f));
        
        scene->AttachLight(light);
        return scene;
    }
}
