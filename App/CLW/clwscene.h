#pragma once

#include "CLW.h"
#include "math/float3.h"
#include "Scene/scene.h"
#include "firerays.h"

namespace Baikal
{
    struct ClwScene
    {
        CLWBuffer<FireRays::float3> vertices;
        CLWBuffer<FireRays::float3> normals;
        CLWBuffer<FireRays::float2> uvs;
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
        
        std::vector<FireRays::Shape*> isect_shapes;
    };
}
