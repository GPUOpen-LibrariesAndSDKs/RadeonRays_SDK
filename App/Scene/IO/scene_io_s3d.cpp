#include "scene_io.h"
#include "image_io.h"

#include "../scene1.h"
#include "../shape.h"
#include "../material.h"
#include "../light.h"
#include "../texture.h"

#include "s3d.h"

#include <algorithm>
#include <iterator>

namespace Baikal
{
    // Obj scene loader
    class SceneIoS3d : public SceneIo
    {
    public:
        // Load scene from file
        Scene1* LoadScene(std::string const& filename, std::string const& basepath) const override;
    };

    SceneIo* SceneIo::CreateSceneIoS3d()
    {
        return new SceneIoS3d();
    }


    Scene1* SceneIoS3d::LoadScene(std::string const& filename, std::string const& basepath) const
    {
        auto image_io(ImageIo::CreateImageIo());

        lmS3D_SCENE_DATA data;
        lmRadeonRaysSaberLoad(data, filename.c_str());

        // Allocate scene
        Scene1* scene(new Scene1);

        Material* default_material = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
        default_material->SetInputValue("albedo", RadeonRays::float4(0.7f, 0.65f, 0.5f, 1.f));
        scene->AttachAutoreleaseObject(default_material);

        // Enumerate all shapes in the scene
        for (auto& obj : data.objects)
        {
            std::vector<RadeonRays::float3> vertices;
            std::vector<RadeonRays::float3> normals;
            std::vector<RadeonRays::float2> uvs;
            std::vector<std::uint32_t> indices;

            // Create empty mesh
            Mesh* mesh = new Mesh();

            for (auto& split : obj.splits)
            {
                std::copy(std::cbegin(split.dataTriangle), std::cend(split.dataTriangle), std::back_inserter(indices));
            }

            std::transform(std::cbegin(obj.points), std::cend(obj.points), std::back_inserter(vertices),
            [](S3D_FLOAT3 const& p) -> RadeonRays::float3 
            {
                return RadeonRays::float3(p.x, p.y, p.z);
            });

            std::transform(std::cbegin(obj.normals), std::cend(obj.normals), std::back_inserter(normals),
                [](S3D_FLOAT3 const& p) -> RadeonRays::float3
            {
                return RadeonRays::float3(p.x, p.y, p.z);
            });

            std::transform(std::cbegin(obj.diffUV), std::cend(obj.diffUV), std::back_inserter(uvs),
                [](S3D_FLOAT3 const& p) -> RadeonRays::float2
            {
                return RadeonRays::float2(p.x, p.y);
            });

            mesh->SetIndices(&indices[0], indices.size());
            mesh->SetVertices(&vertices[0], vertices.size());
            mesh->SetNormals(&normals[0], normals.size());
            mesh->SetUVs(&uvs[0], uvs.size());

            mesh->SetMaterial(default_material);

            // Attach to the scene
            scene->AttachShape(mesh);

            // Attach for autorelease
            scene->AttachAutoreleaseObject(mesh);
        }

        for (auto& light : data.lights)
        {
                RadeonRays::float3 color(light.color.x, light.color.y, light.color.z);
                RadeonRays::float3 pos(light.pos.x, light.pos.y, light.pos.z);
                RadeonRays::float3 dir(light.dir.x, light.dir.y, light.dir.z);

                Light* l = nullptr;
                if (light.type == lmS3D_LIGHT_DIR) 
                {
                    l = new DirectionalLight();
                    l->SetEmittedRadiance(color);
                    l->SetDirection(dir);
                }
                else if (light.type == lmS3D_LIGHT_POINT) 
                {
                    l = new PointLight();
                    l->SetEmittedRadiance(color);
                    l->SetPosition(pos);
                }
                else
                {
                    continue;
                }

                scene->AttachLight(l);
                scene->AttachAutoreleaseObject(l);
        }

        // TODO: temporary code, add IBL
        Texture* ibl_texture = image_io->LoadImage("../Resources/Textures/studio015.hdr");
        scene->AttachAutoreleaseObject(ibl_texture);

        ImageBasedLight* ibl = new ImageBasedLight();
        ibl->SetTexture(ibl_texture);
        ibl->SetMultiplier(1.f);
        scene->AttachAutoreleaseObject(ibl);

        // TODO: temporary code to add directional light
        /*DirectionalLight* light = new DirectionalLight();
        light->SetDirection(RadeonRays::float3(-0.3f, -1.f, -0.4f));
        light->SetEmittedRadiance(2.f * RadeonRays::float3(1.f, 1.f, 1.f));
        scene->AttachAutoreleaseObject(light);*/

        //scene->AttachLight(light);
        scene->AttachLight(ibl);

        return scene;
    }
}
