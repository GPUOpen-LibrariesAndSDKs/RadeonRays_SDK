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
        
        // Enumerate and translate materials
        std::vector<Material*> materials(objmaterials.size());
        for (int i = 0; i < (int)objmaterials.size(); ++i)
        {
            materials[i] = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
            materials[i]->SetInputValue("albedo", RadeonRays::float3(objmaterials[i].diffuse[0], objmaterials[i].diffuse[1], objmaterials[i].diffuse[2]));
            materials[i]->SetTwoSided(false);
        }
        
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
            
            auto idx = objshapes[s].mesh.material_ids[0];
            mesh->SetMaterial(materials[idx]);
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
