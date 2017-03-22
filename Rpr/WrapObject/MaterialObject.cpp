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
#include <OpenImageIO/imageio.h>
#include <math/int2.h>

#include "App/Scene/texture.h"
#include "App/Scene/material.h"
#include "App/Scene/IO/image_io.h"
#include "WrapObject/MaterialObject.h"
#include "WrapObject/Exception.h"


using namespace RadeonRays;
using namespace Baikal;

MaterialObject::MaterialObject(rpr_material_node_type in_type)
{
    m_is_tex = false;
    switch (in_type)
    {
    case RPR_MATERIAL_NODE_DIFFUSE:
    case RPR_MATERIAL_NODE_WARD:
    case RPR_MATERIAL_NODE_ORENNAYAR:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kLambert);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_MICROFACET:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetGGX);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_MICROFACET_REFRACTION:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kMicrofacetRefractionGGX);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_REFLECTION:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kIdealReflect);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_REFRACTION:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kIdealRefract);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_STANDARD:
    {
        //TODO: fix
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kLambert);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_DIFFUSE_REFRACTION:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kTranslucent);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_FRESNEL_SCHLICK:
    case RPR_MATERIAL_NODE_FRESNEL:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kTranslucent);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_EMISSIVE:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kEmissive);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_BLEND:
    {
        MultiBxdf* mat = new MultiBxdf(MultiBxdf::Type::kMix);

        mat->SetInputValue("weight", 0.5f);
        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_TRANSPARENT:
    case RPR_MATERIAL_NODE_PASSTHROUGH:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kPassthrough);

        m_mat = mat;
        break;
    }
    case RPR_MATERIAL_NODE_ADD:
    case RPR_MATERIAL_NODE_ARITHMETIC:
    case RPR_MATERIAL_NODE_BLEND_VALUE:
    case RPR_MATERIAL_NODE_VOLUME:
    case RPR_MATERIAL_NODE_INPUT_LOOKUP:
        throw Exception(RPR_ERROR_UNSUPPORTED, "MaterialObject: unsupported material");
        break;
    case RPR_MATERIAL_NODE_NORMAL_MAP:
    case RPR_MATERIAL_NODE_IMAGE_TEXTURE:
    case RPR_MATERIAL_NODE_NOISE2D_TEXTURE:
    case RPR_MATERIAL_NODE_DOT_TEXTURE:
    case RPR_MATERIAL_NODE_GRADIENT_TEXTURE:
    case RPR_MATERIAL_NODE_CHECKER_TEXTURE:
    case RPR_MATERIAL_NODE_CONSTANT_TEXTURE:
    case RPR_MATERIAL_NODE_BUMP_MAP:
        m_tex = new Texture();
        m_is_tex = true;
        break;
    default:
        throw Exception(RPR_ERROR_INVALID_PARAMETER_TYPE, "MaterialObject: unrecognized material type");
    }

    m_type = (Type)in_type;

}

MaterialObject::MaterialObject(rpr_image_format const in_format, rpr_image_desc const * in_image_desc, void const * in_data)
    : m_is_tex(true)
    , m_type(Type::kImage)
    , m_tex(nullptr)
{
    //Clean if material was already initialized
    Clear();

    //TODO: fix only 4 components data supported
    if (in_format.num_components != 4)
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "TextureObject: only 4 component texture implemented.");

    //tex size
    int2 tex_size(in_image_desc->image_width, in_image_desc->image_height);

    //texture takes ownership of its data array
    //so need to copy input data
    int data_size = in_format.num_components * tex_size.x * tex_size.y;
    Texture::Format data_format = Texture::Format::kRgba8;
    switch (in_format.type)
    {
    case RPR_COMPONENT_TYPE_UINT8:
        break;
    case RPR_COMPONENT_TYPE_FLOAT16:
        data_size *= 2;
        data_format = Texture::Format::kRgba16;
        break;
    case RPR_COMPONENT_TYPE_FLOAT32:
        data_size *= 4;
        data_format = Texture::Format::kRgba32;
        break;
    default:
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "TextureObject: invalid format type.");
    }

    char* data = new char[data_size];
    memcpy(data, in_data, data_size);
    m_tex = new Texture(data, tex_size, data_format);
}

MaterialObject::MaterialObject(const std::string& in_path)
    : m_is_tex(true)
    , m_type(Type::kImage)
    , m_tex(nullptr)
{
    //Clean if material was already initialized
    Clear();

    //load texture using oiio
    ImageIo* oiio = ImageIo::CreateImageIo();
    Texture* texture = nullptr;
    try
    {
        texture = oiio->LoadImage(in_path);
    }
    catch (...) {
        throw Exception(RPR_ERROR_IO_ERROR, "TextureObject: failed to load image.");
    }

    m_tex = texture;
    delete oiio;
}




MaterialObject::~MaterialObject()
{
    Clear();
}

void MaterialObject::Clear()
{
    if (IsMaterial())
    {
        delete m_mat;
        m_mat = nullptr;
    }
    else
    {
        delete m_tex;
        m_tex = nullptr;
    }
}

std::string MaterialObject::TranslatePropName(const std::string& in)
{
    std::string result;
    if (in == "color")
    {
        result = "albedo";
    }
    else if (in == "normal")
    {
        result = in;
    }
    else if (in == "ior")
    {
        result = in;
    }
    else if (in == "roughness")
    {
        result = in;
    }
    else if (in == "base")
    {
        result = "base_material";
    }
    else if (in == "top")
    {
        result = "top_material";
    }
    else if (in == "weight")
    {
        result = in;
    }
    else
    {
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "MaterialObject: unimplemented input.");
    }

    return result;
}


void MaterialObject::SetInputN(const std::string& input_name, MaterialObject* input)
{
    //TODO: handle RPR_MATERIAL_NODE_BLEND
    //convert input name
    if (m_type == Type::kImage)
    {
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "MaterialObject: trying SetInputN for rpr_image object.");
    }

    if (IsTexture())
    {
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "MaterialObject: SetInputN not implemented for Texture materials.");
    }

    //translate material name
    std::string name = TranslatePropName(input_name);
    if (input->IsTexture())
    {
        m_mat->SetInputValue(name, input->GetTexture());
        //handle blend material case
        if (m_type == kBlend && name == "weight")
        {
            MultiBxdf* blend_mat = dynamic_cast<MultiBxdf*>(m_mat);
            blend_mat->SetType(MultiBxdf::Type::kMix);
        }
    }
    else
    {
        //handle blend material case
        if (m_type == kBlend && name == "weight")
        {
            //expected only fresnel materials
            if (input->m_type != Type::kFresnel && input->m_type != Type::kFresnelShlick)
            {
                throw Exception(RPR_ERROR_INVALID_PARAMETER, "MaterialObject: expected only fresnel materials as weight of blend material.");
            }
            MultiBxdf* blend_mat = dynamic_cast<MultiBxdf*>(m_mat);
            blend_mat->SetType(MultiBxdf::Type::kFresnelBlend);
            m_mat->SetInputValue("ior", input->m_mat->GetInputValue("ior").float_value);
        }
        else
        {
            m_mat->SetInputValue(name, input->GetMaterial());
        }
    }

}

void MaterialObject::SetInputValue(const std::string& input_name, const RadeonRays::float4& val)
{
    //TODO: handle RPR_MATERIAL_NODE_BLEND
    //convert input name
    if (m_type == Type::kImage)
    {
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "MaterialObject: trying SetInputValue for rpr_image object.");
    }

    if (IsTexture())
    {
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "MaterialObject: SetInputValue not implemented for Texture materials.");
    }

    //translate material name
    std::string name = TranslatePropName(input_name);
    m_mat->SetInputValue(name, val);
    
    //handle blend material case
    if (m_type == kBlend && name == "weight")
    {
        MultiBxdf* blend_mat = dynamic_cast<MultiBxdf*>(m_mat);
        blend_mat->SetType(MultiBxdf::Type::kMix);
    }
}

void MaterialObject::SetInputImageData(const std::string& input_name, MaterialObject* input)
{
    if (!input->IsTexture())
    {
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "MaterialObject: input material isn't rpr_image.");
    }
    //ImageTexture case
    if (IsTexture())
    {
        if (input_name != "data")
        {
            throw Exception(RPR_ERROR_INVALID_TAG, "MaterialObject: inalid input tag");
        }
        //copy image data
        Baikal::Texture* tex = input->GetTexture();
        const char* data = tex->GetData();
        RadeonRays::int2 size = tex->GetSize();
        auto format = tex->GetFormat();
        char* tex_data = new char[tex->GetSizeInBytes()];
        memcpy(tex_data, data, tex->GetSizeInBytes());

        m_tex->SetData(tex_data, size, format);
    }
    else
    {
        SetInputN(input_name, input);
    }
}