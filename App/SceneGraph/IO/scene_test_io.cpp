#include "scene_io.h"
#include "image_io.h"
#include "../scene1.h"
#include "../shape.h"
#include "../material.h"
#include "../light.h"
#include "../texture.h"
#include "math/mathutils.h"

#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

namespace Baikal
{
    // Create fake test IO
    class SceneIoTest : public SceneIo
    {
    public:
        // Load scene (this class uses filename to determine what scene to generate)
        Scene1* LoadScene(std::string const& filename, std::string const& basepath) const override;
    };
    
    // Create test IO
    SceneIo* SceneIo::CreateSceneIoTest()
    {
        return new SceneIoTest();
    }
    
    
    // Create spehere mesh
    Mesh* CreateSphere(std::uint32_t lat, std::uint32_t lon, float r, RadeonRays::float3 const& c)
    {
        auto num_verts = (lat - 2) * lon + 2;
        auto num_tris = (lat - 2) * (lon - 1 ) * 2;
        
        std::vector<RadeonRays::float3> vertices(num_verts);
        std::vector<RadeonRays::float3> normals(num_verts);
        std::vector<RadeonRays::float2> uvs(num_verts);
        std::vector<std::uint32_t> indices (num_tris * 3);

        
        auto t = 0U;
        for(auto j = 1U; j < lat - 1; j++)
            for(auto i = 0U; i < lon; i++)
            {
                float theta = float(j) / (lat - 1) * (float)M_PI; 
                float phi   = float(i) / (lon - 1 ) * (float)M_PI * 2;
                vertices[t].x =  r * sinf(theta) * cosf(phi) + c.x;
                vertices[t].y =  r * cosf(theta) + c.y;
                vertices[t].z = r * -sinf(theta) * sinf(phi) + c.z;
                normals[t].x = sinf(theta) * cosf(phi);
                normals[t].y = cosf(theta);
                normals[t].z = -sinf(theta) * sinf(phi);
                uvs[t].x = float(j) / (lat - 2);
                uvs[t].y = float(i) / (lon - 1);
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

        t = 0U;
        for(auto j = 0U; j < lat - 3; j++)
            for(auto i = 0U; i < lon - 1; i++)
            {
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
                indices[t++] = j * lon + i + 1;
                indices[t++] = j * lon + i;
                indices[t++] = (j + 1) * lon + i;
                indices[t++] = (j + 1) * lon + i + 1;
            }
        
        for(auto i = 0U; i < lon - 1; i++)
        {
            indices[t++] = (lat - 2) * lon;
            indices[t++] = i;
            indices[t++] = i + 1;
            indices[t++] = (lat - 2) * lon + 1;
            indices[t++] = (lat - 3) * lon + i + 1;
            indices[t++] = (lat - 3) * lon + i;
        }
        
        auto mesh = new Mesh();
        mesh->SetVertices(&vertices[0], vertices.size());
        mesh->SetNormals(&normals[0], normals.size());
        mesh->SetUVs(&uvs[0], uvs.size());
        mesh->SetIndices(&indices[0], indices.size());

        return mesh;
    }
    
    
    // Create quad
    Mesh* CreateQuad(std::vector<RadeonRays::float3> const& vertices, bool flip)
    {
        using namespace RadeonRays;
        
        auto u1 = normalize(vertices[1] - vertices[0]);
        auto u2 = normalize(vertices[3] - vertices[0]);
        auto n = -cross(u1, u2);
        
        if (flip)
        {
            n = -n;
        }
        
        float3 normals[] = { n, n, n, n };
        
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
        mesh->SetVertices(&vertices[0], 4);
        mesh->SetNormals(normals, 4);
        mesh->SetUVs(uvs, 4);
        mesh->SetIndices(indices, 6);
     
        return mesh;
    }
    
    Scene1* SceneIoTest::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        using namespace RadeonRays;
        
        Scene1* scene(new Scene1);
        
        auto image_io(ImageIo::CreateImageIo());
        
        if (filename == "quad+ibl")
        {
            Mesh* quad = CreateQuad(
            {
                RadeonRays::float3(-5, 0, -5),
                RadeonRays::float3(5, 0, -5),
                RadeonRays::float3(5, 0, 5),
                RadeonRays::float3(-5, 0, 5),
            }
            , false);
            
            scene->AttachShape(quad);
            scene->AttachAutoreleaseObject(quad);
            
            Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
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
            
            quad->SetMaterial(mix);
            
            scene->AttachAutoreleaseObject(green);
            scene->AttachAutoreleaseObject(spec);
            scene->AttachAutoreleaseObject(mix);
        }
        else if (filename == "sphere+ibl")
        {
            auto mesh = CreateSphere(64, 32, 2.f, float3());
            scene->AttachShape(mesh);
            scene->AttachAutoreleaseObject(mesh);
            
            Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
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
        else if (filename == "sphere+plane+area")
        {
            auto mesh = CreateSphere(64, 32, 2.f, float3(0.f, 2.5f, 0.f));
            scene->AttachShape(mesh);
            scene->AttachAutoreleaseObject(mesh);

            SingleBxdf* grey = new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX);
            grey->SetInputValue("albedo", float4(0.99f, 0.99f, 0.99f, 1.f));
            grey->SetInputValue("roughness", float4(0.025f, 0.025f, 0.025f, 1.f));

            Mesh* floor = CreateQuad(
                                    {
                                        RadeonRays::float3(-8, 0, -8),
                                        RadeonRays::float3(8, 0, -8),
                                        RadeonRays::float3(8, 0, 8),
                                        RadeonRays::float3(-8, 0, 8),
                                    }
                                    , false);
            scene->AttachShape(floor);
            scene->AttachAutoreleaseObject(floor);
            
            floor->SetMaterial(grey);
            mesh->SetMaterial(grey);

            SingleBxdf* emissive = new SingleBxdf(SingleBxdf::BxdfType::kEmissive);
            emissive->SetInputValue("albedo", 1.f * float4(3.1f, 3.f, 2.8f, 1.f));
            
            Mesh* light = CreateQuad(
                                     {
                                         RadeonRays::float3(-2, 6, -2),
                                         RadeonRays::float3(2, 6, -2),
                                         RadeonRays::float3(2, 6, 2),
                                         RadeonRays::float3(-2, 6, 2),
                                     }
                                     , true);
            scene->AttachShape(light);
            scene->AttachAutoreleaseObject(light);
            
            light->SetMaterial(emissive);

            AreaLight* l1 = new AreaLight(light, 0);
            AreaLight* l2 = new AreaLight(light, 1);

            scene->AttachLight(l1);
            scene->AttachLight(l2);
            scene->AttachAutoreleaseObject(l1);
            scene->AttachAutoreleaseObject(l2);

            scene->AttachAutoreleaseObject(emissive);
            scene->AttachAutoreleaseObject(grey);
        }
        else if (filename == "sphere+plane+ibl")
        {
            auto mesh = CreateSphere(64, 32, 2.f, float3(0.f, 2.2f, 0.f));
            scene->AttachShape(mesh);
            scene->AttachAutoreleaseObject(mesh);

            auto instance = new Instance(mesh);
            scene->AttachShape(instance);
            scene->AttachAutoreleaseObject(instance);

            matrix t = RadeonRays::translation(float3(5.f, 0.f, 5.f));
            instance->SetTransform(t);

            SingleBxdf* green = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
            green->SetInputValue("albedo", 2.f * float4(0.1f, 0.2f, 0.1f, 1.f));

            SingleBxdf* grey = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
            green->SetInputValue("albedo", 2.f * float4(0.2f, 0.2f, 0.2f, 0.2f));

            SingleBxdf* spec = new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX);
            spec->SetInputValue("albedo", float4(0.9f, 0.9f, 0.9f, 1.f));
            spec->SetInputValue("roughness", float4(0.2f, 0.2f, 0.2f, 1.f));

            auto normal_map = image_io->LoadImage("../Resources/Textures/test_normal.jpg");
            auto bump_map = image_io->LoadImage("../Resources/Textures/test_bump.jpg");
            green->SetInputValue("normal", normal_map);
            spec->SetInputValue("normal", normal_map);
            grey->SetInputValue("bump", bump_map);
            instance->SetMaterial(grey);

            MultiBxdf* mix = new MultiBxdf(MultiBxdf::Type::kFresnelBlend);
            mix->SetInputValue("base_material", green);
            mix->SetInputValue("top_material", spec);
            mix->SetInputValue("ior", float4(3.33f, 3.33f, 3.33f, 3.33f));

            mesh->SetMaterial(mix);

            Mesh* floor = CreateQuad(
                                     {
                                         RadeonRays::float3(-8, 0, -8),
                                         RadeonRays::float3(8, 0, -8),
                                         RadeonRays::float3(8, 0, 8),
                                         RadeonRays::float3(-8, 0, 8),
                                     }
                                     , false);
            scene->AttachShape(floor);
            scene->AttachAutoreleaseObject(floor);
            
            floor->SetMaterial(green);
            
            Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
            scene->AttachAutoreleaseObject(ibl_texture);
            
            ImageBasedLight* ibl = new ImageBasedLight();
            ibl->SetTexture(ibl_texture);
            ibl->SetMultiplier(1.f);
            scene->AttachLight(ibl);
            scene->AttachAutoreleaseObject(ibl);
            
            scene->AttachAutoreleaseObject(green);
            scene->AttachAutoreleaseObject(spec);
            scene->AttachAutoreleaseObject(mix);
            scene->AttachAutoreleaseObject(grey);
            scene->AttachAutoreleaseObject(normal_map);
            scene->AttachAutoreleaseObject(bump_map);
            
        }
        
        return scene;
    }
}

