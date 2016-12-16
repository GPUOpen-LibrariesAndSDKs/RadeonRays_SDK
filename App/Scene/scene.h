/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/
#pragma once

#include "math/mathutils.h"
#include "perspective_camera.h"
#include <vector>
#include <string>
#include <memory>

namespace Baikal
{
    class Scene
    {
    public:
        // Load the scene from OBJ file
        static Scene* LoadFromObj(std::string const& filename, std::string const& basepath = "");

        Scene() : dirty_(kCamera)
        {
        }

        void SetEnvironment(std::string const& filename, std::string const& basepath = "", float envmapmul = 1.f);

        void SetBackground(std::string const& filename, std::string const& basepath = "");

        void AddDirectionalLight(RadeonRays::float3 const& d, RadeonRays::float3 const& e);
        
        void AddPointLight(RadeonRays::float3 const& p, RadeonRays::float3 const& e);
        
        void AddSpotLight(RadeonRays::float3 const& p, RadeonRays::float3 const& d, RadeonRays::float3 const& e, float ia, float oa);

        enum DirtyFlags
        {
            kNone = 0x0,
            kCamera = 0x1,
            kGeometry = 0x2,
            kGeometryTransform = 0x4,
            kMaterials = 0x8,
            kMaterialInputs = 0x16,
            kTextures = 0x32,
            kEnvironmentLight = 0x64,
        };

        enum Bxdf
        {
            kZero,
            kLambert,
            kIdealReflect,
            kIdealRefract,
            kMicrofacetBlinn,
            kMicrofacetBeckmann,
            kMicrofacetGGX,
            kLayered,
            kFresnelBlend,
            kMix,
            kEmissive,
            kPassthrough,
            kTranslucent,
            kMicrofacetRefractionGGX,
            kMicrofacetRefractionBeckmann
        };

        // Material description
        struct Material
        {
            RadeonRays::float3 kx;
            float ni;
            float ns;

            union
            {
                // Color map index
                int kxmapidx;
                //
                int brdftopidx;
            };

            union
            {
                // Normal map index
                int nmapidx;
                //
                int brdfbaseidx;
            };

            union
            {
                // Parameter map idx
                int nsmapidx;
            };

            union
            {
                float fresnel;
            };

            // BXDF type
            int type;
            int twosided;

            Material() :
                kx(0.7f, 0.7f, 0.7f, 1.f)
                , ni(25.f)
                , ns(0.05f)
                , kxmapidx(-1)
                , nmapidx(-1)
                , nsmapidx(-1)
                , fresnel(0)
                , type (kLambert)
                , twosided(0)
                {
                }
            };

            // Texture format
            enum TextureFormat
            {
                UNKNOWN,
                RGBA8,
                RGBA16,
                RGBA32
            };

            // Texture description
            struct Texture
            {
                // Texture width, height and depth
                int w;
                int h;
                int d;
                int dataoffset;
                int fmt;
                int size;
            };

            // Shape description
            struct Shape
            {
                // Shape starting index in indices_ array
                int startidx;
                // Number of primitives in the shape
                int numprims;
                // Start vertex
                int startvtx;
                // Number of vertices
                int numvertices;
                // Linear velocity
                RadeonRays::float3 linearvelocity;
                // Angular velocity
                RadeonRays::quaternion angularvelocity;
                // Transform
                RadeonRays::matrix m;
            };

            enum LightType
            {
                kPoint = 0x1,
                kDirectional,
                kSpot,
                kArea,
                kIbl
            };

            struct Light
            {
                int type;

                union
                {
                    // Area light
                    struct
                    {
                        int shapeidx;
                        int primidx;
                        int matidx;
                    };

                    // IBL
                    struct
                    {
                        int tex;
                        int texdiffuse;
                        float multiplier;
                    };
                    
                    // Spot
                    struct
                    {
                        float ia;
                        float oa;
                        float f;
                    };
                };

                RadeonRays::float3 p;
                RadeonRays::float3 d;
                RadeonRays::float3 intensity;
            };

            struct Volume
            {
                int type;
                int phase_func;
                int data;
                int extra;

                RadeonRays::float3 sigma_a;
                RadeonRays::float3 sigma_s;
                RadeonRays::float3 sigma_e;
            };

            void clear_dirty() const { dirty_ = kNone;  }
            std::uint32_t dirty() const { return dirty_; }
            void set_dirty(int dirty) const { dirty_ = dirty_ | dirty; }

            // Scene data
            // Vertices
            std::vector<RadeonRays::float3> vertices_;
            std::vector<RadeonRays::float3> normals_;
            std::vector<RadeonRays::float2> uvs_;
            // Primitive indices
            std::vector<int> indices_;
            // Shapes: index which points to indices array
            std::vector<Shape> shapes_;
            // Emissive primitive indices
            std::vector<Light> lights_;
            // Material indices per primitive
            std::vector<int> materialids_;
            // Material descriptions
            std::vector<Material> materials_;
            std::vector<std::string> material_names_;
            // Textures
            std::vector<Texture> textures_;
            // Texture data
            std::vector<std::unique_ptr<char[]>> texturedata_;
            // Camera
            std::unique_ptr<PerspectiveCamera> camera_;
            // Environment texture index
            int envidx_;
            // Environment low-res texture index
            int envlowresidx_;
            // Multiplier for envmap power
            float envmapmul_;
            // Background image
            int bgimgidx_;
            // Dirty flags
            mutable int dirty_;
        };
    }
