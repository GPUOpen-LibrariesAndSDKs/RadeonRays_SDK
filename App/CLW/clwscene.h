#pragma once

#include "CLW.h"
#include "math/float3.h"
#include "SceneGraph/scene1.h"
#include "radeon_rays.h"
#include "SceneGraph/Collector/collector.h"


namespace Baikal
{
    using namespace RadeonRays;

    enum class CameraType
    {
        kDefault,
        kPhysical,
        kSpherical,
        kFisheye
    };

    struct ClwScene
    {
        #include "CL/payload.cl"

        CLWBuffer<RadeonRays::float3> vertices;
        CLWBuffer<RadeonRays::float3> normals;
        CLWBuffer<RadeonRays::float2> uvs;
        CLWBuffer<int> indices;

        CLWBuffer<Shape> shapes;

        CLWBuffer<Material> materials;
        CLWBuffer<Light> lights;
        CLWBuffer<int> materialids;
        CLWBuffer<Volume> volumes;
        CLWBuffer<Texture> textures;
        CLWBuffer<char> texturedata;

        CLWBuffer<Camera> camera;

        std::unique_ptr<Bundle> material_bundle;
        std::unique_ptr<Bundle> texture_bundle;

        int num_lights;
        int envmapidx;
        CameraType camera_type;

        std::vector<RadeonRays::Shape*> isect_shapes;
        std::vector<RadeonRays::Shape*> visible_shapes;
    };
}
