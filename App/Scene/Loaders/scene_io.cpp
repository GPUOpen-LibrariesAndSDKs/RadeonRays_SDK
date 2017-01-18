#include "scene_io.h"
#include "image_io.h"
#include "../scene1.h"
#include "../shape.h"
#include "../material.h"
#include "../light.h"
#include "../texture.h"



#include "tiny_obj_loader.h"

namespace Baikal
{
    // Obj scene loader
    class SceneIoObj : public SceneIo
    {
    public:
        // Load scene from file
        Scene1* LoadScene(std::string const& filename, std::string const& basepath) const override;
    };
    
    SceneIo* SceneIo::CreateSceneIoObj()
    {
        return new SceneIoObj();
    }
    
    Scene1* SceneIoObj::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        using namespace tinyobj;
        
        auto image_io(ImageIo::CreateImageIo());
        
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
        // Keep track of emissive subset
        std::set<Material*> emissives;
        std::vector<Material*> materials(objmaterials.size());
        for (int i = 0; i < (int)objmaterials.size(); ++i)
        {
            RadeonRays::float3 emission(objmaterials[i].emission[0], objmaterials[i].emission[1], objmaterials[i].emission[2]);
            
            // Check if this is emissive
            if (emission.sqnorm() > 0)
            {
                // If yes create emissive brdf
                materials[i] = new SingleBxdf(SingleBxdf::BxdfType::kEmissive);
                
                // Set albedo
                if (!objmaterials[i].diffuse_texname.empty())
                {
                    auto texture = image_io->LoadImage(basepath + "/" + objmaterials[i].diffuse_texname);
                    materials[i]->SetInputValue("albedo", texture);
                    scene->AttachAutoreleaseObject(texture);
                }
                else
                {
                    materials[i]->SetInputValue("albedo", emission);
                }
                
                // Insert into emissive set
                emissives.insert(materials[i]);
            }
            else
            {
                // Otherwise create lambert
                materials[i] = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
            
                // Set albedo
                if (!objmaterials[i].diffuse_texname.empty())
                {
                    auto texture = image_io->LoadImage(basepath + "/" + objmaterials[i].diffuse_texname);
                    materials[i]->SetInputValue("albedo", texture);
                    scene->AttachAutoreleaseObject(texture);
                }
                else
                {
                    materials[i]->SetInputValue("albedo", RadeonRays::float3(objmaterials[i].diffuse[0], objmaterials[i].diffuse[1], objmaterials[i].diffuse[2]));
                }
            }
            
            // Disable normal flip
            materials[i]->SetTwoSided(false);
            
            // Put into the pool of autoreleased objects
            scene->AttachAutoreleaseObject(materials[i]);
        }
        
        // Enumerate all shapes in the scene
        for (int s = 0; s < (int)objshapes.size(); ++s)
        {
            // Create empty mesh
            Mesh* mesh = new Mesh();
            
            // Set vertex and index data
            auto num_vertices = objshapes[s].mesh.positions.size() / 3;
            mesh->SetVertices(&objshapes[s].mesh.positions[0], num_vertices);
            
            auto num_normals = objshapes[s].mesh.normals.size() / 3;
            mesh->SetNormals(&objshapes[s].mesh.normals[0], num_normals);
            
            auto num_uvs = objshapes[s].mesh.texcoords.size() / 2;
            
            // If we do not have UVs, generate zeroes
            if (num_uvs)
            {
                mesh->SetUVs(&objshapes[s].mesh.texcoords[0], num_uvs);
            }
            else
            {
                std::vector<RadeonRays::float2> zero(num_vertices);
                mesh->SetUVs(&zero[0], num_vertices);
            }
            
            // Set indices
            auto num_indices = objshapes[s].mesh.indices.size();
            mesh->SetIndices(reinterpret_cast<std::uint32_t const*>(&objshapes[s].mesh.indices[0]), num_indices);
            
            // Set material
            auto idx = objshapes[s].mesh.material_ids[0];
            mesh->SetMaterial(materials[idx]);
            
            // Attach to the scene
            scene->AttachShape(mesh);
            
            // Attach for autorelease
            scene->AttachAutoreleaseObject(mesh);
            
            // If the mesh has emissive material we need to add area light for it
            if (emissives.find(materials[idx]) != emissives.cend())
            {
                // Add area light for each polygon of emissive mesh
                for (int l = 0; l < mesh->GetNumIndices() / 3 ;++l)
                {
                    AreaLight* light = new AreaLight(mesh, l);
                    scene->AttachLight(light);
                    scene->AttachAutoreleaseObject(light);
                }
                
            }
        }
        
        // TODO: temporary code, add IBL
        Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
        scene->AttachAutoreleaseObject(ibl_texture);
        
        ImageBasedLight* ibl = new ImageBasedLight();
        ibl->SetTexture(ibl_texture);
        ibl->SetMultiplier(1.f);
        scene->AttachAutoreleaseObject(ibl);
        
        
        // TODO: temporary code to add directional light
        DirectionalLight* light = new DirectionalLight();
        light->SetDirection(RadeonRays::float3(-0.3f, -1.f, -0.4f));
        light->SetEmittedRadiance(2.f * RadeonRays::float3(1.f, 1.f, 1.f));
        scene->AttachAutoreleaseObject(light);
        
        scene->AttachLight(light);
        scene->AttachLight(ibl);
        
        return scene;
    }
}
