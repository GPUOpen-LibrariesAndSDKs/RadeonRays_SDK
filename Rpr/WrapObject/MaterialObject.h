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

#include "WrapObject.h"
#include "RadeonProRender.h"
#include "math/float3.h"

#include <string>
#include <map>
#include <set>

namespace Baikal
{
    class Texture;
    class Material;
}

//represent rpr_material_node
class MaterialObject
    : public WrapObject
{
public:
    enum Type
    {
        //represent rpr_image:
        kImage, 
        //textures:
        kNormalMap = RPR_MATERIAL_NODE_NORMAL_MAP,
        kImageTexture = RPR_MATERIAL_NODE_IMAGE_TEXTURE,
        kNoise2dTexture = RPR_MATERIAL_NODE_NOISE2D_TEXTURE,
        kDotTexture = RPR_MATERIAL_NODE_DOT_TEXTURE,
        kGradientTexture = RPR_MATERIAL_NODE_GRADIENT_TEXTURE,
        kCheckerTexture = RPR_MATERIAL_NODE_CHECKER_TEXTURE,
        kConstantTexture = RPR_MATERIAL_NODE_CONSTANT_TEXTURE,
        kBumpMap = RPR_MATERIAL_NODE_BUMP_MAP,
        //materials:
        kDiffuse = RPR_MATERIAL_NODE_DIFFUSE,
        kDiffuseRefraction = RPR_MATERIAL_NODE_DIFFUSE_REFRACTION,
        kMicrofacet = RPR_MATERIAL_NODE_MICROFACET,
        kReflection = RPR_MATERIAL_NODE_REFLECTION,
        kRefraction = RPR_MATERIAL_NODE_REFRACTION,
        kMicrofacetRefraction = RPR_MATERIAL_NODE_MICROFACET_REFRACTION,
        kTransparent = RPR_MATERIAL_NODE_TRANSPARENT,
        kEmissive = RPR_MATERIAL_NODE_EMISSIVE,
        kWard = RPR_MATERIAL_NODE_WARD,
        kBlend = RPR_MATERIAL_NODE_BLEND,
        kFresnel = RPR_MATERIAL_NODE_FRESNEL,
        kFresnelShlick = RPR_MATERIAL_NODE_FRESNEL_SCHLICK,
        kStandard = RPR_MATERIAL_NODE_STANDARD,
        kPassthrough = RPR_MATERIAL_NODE_PASSTHROUGH,
        kOrennayar = RPR_MATERIAL_NODE_ORENNAYAR,
    };

    //initialize methods
    //represent rpr_image
    //Note: in_data will be copied. Baikal::Texture inside.
    MaterialObject(rpr_image_format const in_format, rpr_image_desc const * in_image_desc, void const * in_data);
    MaterialObject(const std::string& in_path);

    //Baikal::Material or Baikal::Texture inside
    MaterialObject(rpr_material_node_type in_type);

    virtual ~MaterialObject();

    bool IsImg() { return m_type == Type::kImage; }
    bool IsTexture() { return m_is_tex; }
    bool IsMaterial() { return !m_is_tex; }

    //inputs
    void SetInputMaterial(const std::string& input_name, MaterialObject* input);
    void SetInputValue(const std::string& input_name, const RadeonRays::float4& val);

    Type GetType() { return m_type; }
    Baikal::Texture* GetTexture() { return m_tex; }
    Baikal::Material* GetMaterial() { return m_mat; }
    //rprMaterialGetInfo:
    uint64_t GetInputCount();
    rpr_uint GetInputType(int i);
    void GetInput(int i, void* out, size_t* out_size);
    std::string GetInputName(int i);

    //rprImageGetInfo:
    rpr_image_desc GetTextureDesc() const;
    char const* GetTextureData() const;
    rpr_image_format GetTextureFormat() const;

private:
    void Clear();
    bool CheckInputMaterial();

    //handle input materials, it need for correct rprMaterialGet* methods.
    //Note: input_name is RPR input name
    void SetInput(MaterialObject* input_mat, const std::string& input_name);

    //add and remove output materials
    void AddOutput(MaterialObject* mat);
    void RemoveOutput(MaterialObject* mat);
    void Notify();
    void Update(MaterialObject* mat);

    //type - is type of input material
    std::string TranslatePropName(const std::string& in, Type type = Type::kDiffuse);

    Type m_type;
    bool m_is_tex;
    union
    {
        Baikal::Texture* m_tex;
        Baikal::Material* m_mat;
    };
    //output materials
    std::set<MaterialObject*> m_out_mats;

    //input material + RPR input name. Required for rprMaterialGet* methods.
    std::map<std::string, MaterialObject*> m_inputs;
};