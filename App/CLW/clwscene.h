#pragma once

#include "CLW.h"
#include "math/float3.h"
#include "Scene/scene.h"
#include "radeon_rays.h"

namespace Baikal
{
    enum class CameraType
    {
        kDefault,
        kPhysical,
        kSpherical,
        kFisheye
    };

    struct ClwScene
    {
        CLWBuffer<RadeonRays::float3> vertices;
        CLWBuffer<RadeonRays::float3> normals;
        CLWBuffer<RadeonRays::float2> uvs;
        CLWBuffer<int> indices;

        CLWBuffer<Scene::Shape> shapes;

        CLWBuffer<Scene::Material> materials;
        CLWBuffer<Scene::Emissive> emissives;
        CLWBuffer<int> materialids;
        CLWBuffer<Scene::Volume> volumes;
        CLWBuffer<Scene::Texture> textures;
        CLWBuffer<char> texturedata;

        CLWBuffer<PerspectiveCamera> camera;

        int numemissive;
        int envmapidx;
        float envmapmul;
        CameraType camera_type;
        
        std::vector<RadeonRays::Shape*> isect_shapes;
    };
}
