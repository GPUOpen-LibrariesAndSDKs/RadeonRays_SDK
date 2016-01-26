/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
#ifndef SCENE_H
#define SCENE_H


#include "math/mathutils.h"
#include "perspective_camera.h"
#include <vector>
#include <string>
#include <memory>


class Scene
{
public:
    // Load the scene from OBJ file
    static Scene* LoadFromObj(std::string const& filename, std::string const& basepath = "");

    void SetEnvironment(std::string const& filename, std::string const& basepath = "", float envmapmul = 1.f);

    void SetBackground(std::string const& filename, std::string const& basepath = "");
    
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
		kEmissive
    };

    // Material description
    struct Material
    {
        FireRays::float3 kx;
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
        float padding;

        Material() :
          kx(0.7f, 0.7f, 0.7f, 1.f)
        , ni(25.f)
        , ns(0.05f)
        , kxmapidx(-1)
        , nmapidx(-1)
        , nsmapidx(-1)
        , fresnel(0)
        , type (kLambert)
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
        FireRays::float3 linearvelocity;
        // Angular velocity
        FireRays::quaternion angularvelocity;
        // Transform
        FireRays::matrix m;
    };

    // Emissive object
    struct Emissive
    {
        // Shape index
        int shapeidx;
        // Polygon index
        int primidx;
		// Material index
		int m;

		int padding;
    };

    // Scene data
    // Vertices
    std::vector<FireRays::float3> vertices_;
    std::vector<FireRays::float3> normals_;
    std::vector<FireRays::float2> uvs_;
    // Primitive indices
    std::vector<int> indices_;
    // Shapes: index which points to indices array
    std::vector<Shape> shapes_;
    // Emissive primitive indices
    std::vector<Emissive> emissives_;
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
};


#endif // SCENE_H
