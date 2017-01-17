#include "scene_io.h"
#include "image_io.h"
#include "../scene1.h"
#include "../shape.h"
#include "../material.h"
#include "../light.h"
#include "../texture.h"
#include "../scene_object.h"


#include "tiny_obj_loader.h"

namespace Baikal
{
    class SceneIoObj : public SceneIo
    {
    public:
        Scene1* LoadScene(std::string const& filename, std::string const& basepath) const override;
    };
    
    class SceneIoTest : public SceneIo
    {
    public:
        Scene1* LoadScene(std::string const& filename, std::string const& basepath) const override;
    };

    
    SceneIo* SceneIo::CreateSceneIoObj()
    {
        return new SceneIoObj();
    }
    
    SceneIo* SceneIo::CreateSceneIoTest()
    {
        return new SceneIoTest();
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
        std::vector<Material*> materials(objmaterials.size());
        for (int i = 0; i < (int)objmaterials.size(); ++i)
        {
            materials[i] = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
            
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
            
            materials[i]->SetTwoSided(false);
            
            scene->AttachAutoreleaseObject(materials[i]);
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
            
            scene->AttachAutoreleaseObject(mesh);
        }
        
        Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
        scene->AttachAutoreleaseObject(ibl_texture);
        
        ImageBasedLight* ibl = new ImageBasedLight();
        ibl->SetTexture(ibl_texture);
        ibl->SetMultiplier(1.f);
        scene->AttachAutoreleaseObject(ibl);
        
        
        DirectionalLight* light = new DirectionalLight();
        light->SetDirection(RadeonRays::float3(-0.3f, -1.f, -0.4f));
        light->SetEmittedRadiance(2.f * RadeonRays::float3(1.f, 1.f, 1.f));
        scene->AttachAutoreleaseObject(light);
        
        scene->AttachLight(light);
        scene->AttachLight(ibl);
        
        return scene;
    }
    
    static void CreateSphere(std::uint32_t lat, std::uint32_t lon, float r, RadeonRays::float3 const& c,
                             std::vector<RadeonRays::float3>& vertices,
                             std::vector<RadeonRays::float3>& normals,
                             std::vector<RadeonRays::float2>& uvs,
                             std::vector<std::uint32_t>& indices
                             )
    {
        
        float theta, phi;
        int i, j, t, ntri, nvec;
        
        nvec = (lat-2)* lon+2;
        ntri = (lat-2)*(lon-1)*2;
        
        vertices.resize( nvec );
        normals.resize( nvec );
        uvs.resize( nvec );
        indices.resize( ntri * 3 );
        
        for( t=0, j=1; j<lat-1; j++ )
            for(      i=0; i<lon; i++ )
            {
                theta = float(j)/(lat-1) * M_PI;
                phi   = float(i)/(lon-1 ) * M_PI * 2;
                vertices[t].x =  r * sinf(theta) * cosf(phi) + c.x;
                vertices[t].y =  r * cosf(theta) + c.y;
                vertices[t].z = r * -sinf(theta) * sinf(phi) + c.z;
                normals[t].x = sinf(theta) * cosf(phi);
                normals[t].y = cosf(theta);
                normals[t].z = -sinf(theta) * sinf(phi);
                uvs[t].x = phi / (2 * M_PI);
                uvs[t].y = theta / (M_PI);
                ++t;
            }
        
        vertices[t].x=c.x; vertices[t].y = c.y + r; vertices[t].z = c.z;
        normals[t].x=0; normals[t].y = 1; normals[t].z = 0;
        uvs[t].x=0; uvs[t].y = 0;
        ++t;
        vertices[t].x=c.x; vertices[t].y = c.y-r; vertices[t].z = c.z;
        normals[t].x=0; normals[t].y = -1; normals[t].z = 0;
        uvs[t].x=1; uvs[t].y = 1;
        ++t;
        
        for( t=0, j=0; j<lat-3; j++ )
            for(      i=0; i<lon-1; i++ )
            {
                indices[t++] = (j  )*lon + i  ;
                indices[t++] = (j+1)*lon + i+1;
                indices[t++] = (j  )*lon + i+1;
                indices[t++] = (j  )*lon + i  ;
                indices[t++] = (j+1)*lon + i  ;
                indices[t++] = (j+1)*lon + i+1;
            }
        for( i=0; i<lon-1; i++ )
        {
            indices[t++] = (lat-2)*lon;
            indices[t++] = i;
            indices[t++] = i+1;
            indices[t++] = (lat-2)*lon+1;
            indices[t++] = (lat-3)*lon + i+1;
            indices[t++] = (lat-3)*lon + i;
        }
    }
    
    Scene1* SceneIoTest::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        using namespace RadeonRays;
        
        Scene1* scene(new Scene1);
        
        auto image_io(ImageIo::CreateImageIo());
        
        if (filename == "plane+ibl")
        {
            float3 vertices[] =
            {
                float3(-5, 0, -5),
                float3(5, 0, -5),
                float3(5, 0, 5),
                float3(-5, 0, 5)
            };
            
            float3 vertices1[] =
            {
                float3(-5, 1, -6),
                float3(5, 1, -6),
                float3(5, 5, -6),
                float3(-5, 5, -6)
            };
            
            float3 normals[] =
            {
                float3(0, 1, 0),
                float3(0, 1, 0),
                float3(0, 1, 0),
                float3(0, 1, 0)
            };
            
            float3 normals1[] =
            {
                float3(0, 0, 1),
                float3(0, 0, 1),
                float3(0, 0, 1),
                float3(0, 0, 1)
            };

            
            float2 uvs[] =
            {
                float2(0, 0),
                float2(1, 0),
                float2(1, 1),
                float2(0, 1)
            };
            
            std::uint32_t indices[] =
            {
                0, 1, 2,
                0, 2, 3
            };
        
            auto mesh = new Mesh();
            mesh->SetVertices(vertices, 4);
            mesh->SetNormals(normals, 4);
            mesh->SetUVs(uvs, 4);
            mesh->SetIndices(indices, 6);
            scene->AttachShape(mesh);
            scene->AttachAutoreleaseObject(mesh);
            
            auto mesh1 = new Mesh();
            mesh1->SetVertices(vertices1, 4);
            mesh1->SetNormals(normals1, 4);
            mesh1->SetUVs(uvs, 4);
            mesh1->SetIndices(indices, 6);
            scene->AttachShape(mesh1);
            scene->AttachAutoreleaseObject(mesh1);
            
            Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/parking.hdr");
            scene->AttachAutoreleaseObject(ibl_texture);
            
            ImageBasedLight* ibl = new ImageBasedLight();
            ibl->SetTexture(ibl_texture);
            ibl->SetMultiplier(1.f);
            scene->AttachLight(ibl);
            scene->AttachAutoreleaseObject(ibl);
            
            SingleBxdf* green = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
            green->SetInputValue("albedo", float4(0.1f, 0.2f, 0.1f, 1.f));
            
            SingleBxdf* spec = new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX);
            spec->SetInputValue("albedo", float4(0.9f, 0.9f, 0.9f, 1.f));
            spec->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));
            
            MultiBxdf* mix = new MultiBxdf(MultiBxdf::Type::kFresnelBlend);
            mix->SetInputValue("base_material", green);
            mix->SetInputValue("top_material", spec);
            mix->SetInputValue("ior", float4(1.33f, 1.33f, 1.33f, 1.33f));
            
            
            mesh->SetMaterial(mix);
            mesh1->SetMaterial(spec);
            
            scene->AttachAutoreleaseObject(green);
            scene->AttachAutoreleaseObject(spec);
            scene->AttachAutoreleaseObject(mix);
        }
        else if (filename == "sphere+ibl")
        {
            std::vector<float3> vertices;
            std::vector<float3> normals;
            std::vector<float2> uvs;
            std::vector<std::uint32_t> indices;
            
            CreateSphere(64, 32, 2.f, float3(),
                         vertices,
                         normals,
                         uvs,
                         indices);
            
            auto mesh = new Mesh();
            mesh->SetVertices(&vertices[0], vertices.size());
            mesh->SetNormals(&normals[0], normals.size());
            mesh->SetUVs(&uvs[0], uvs.size());
            mesh->SetIndices(&indices[0], indices.size());
            scene->AttachShape(mesh);
            scene->AttachAutoreleaseObject(mesh);
            
            Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/parking.hdr");
            scene->AttachAutoreleaseObject(ibl_texture);
            
            ImageBasedLight* ibl = new ImageBasedLight();
            ibl->SetTexture(ibl_texture);
            ibl->SetMultiplier(1.f);
            scene->AttachLight(ibl);
            scene->AttachAutoreleaseObject(ibl);
            
            SingleBxdf* green = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
            green->SetInputValue("albedo", float4(0.1f, 0.2f, 0.1f, 1.f));
            
            SingleBxdf* spec = new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX);
            spec->SetInputValue("albedo", float4(0.9f, 0.9f, 0.9f, 1.f));
            spec->SetInputValue("roughness", float4(0.002f, 0.002f, 0.002f, 1.f));
            
            MultiBxdf* mix = new MultiBxdf(MultiBxdf::Type::kFresnelBlend);
            mix->SetInputValue("base_material", green);
            mix->SetInputValue("top_material", spec);
            mix->SetInputValue("ior", float4(1.33f, 1.33f, 1.33f, 1.33f));
            
            
            mesh->SetMaterial(mix);
            
            scene->AttachAutoreleaseObject(green);
            scene->AttachAutoreleaseObject(spec);
            scene->AttachAutoreleaseObject(mix);
            
        }
        
        return scene;
    }
}
