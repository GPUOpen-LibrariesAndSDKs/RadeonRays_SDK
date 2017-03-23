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
#include <map>

#include "App/Scene/texture.h"
#include "App/Scene/material.h"
#include "App/Scene/IO/image_io.h"
#include "WrapObject/MaterialObject.h"
#include "WrapObject/Exception.h"


using namespace RadeonRays;
using namespace Baikal;

namespace
{
    //contains pairs <rpr input name, baikal input name> of input names
    const std::map<std::string, std::string> kInputNamesDictionary = { { "color" , "albedo" },
                                                                { "normal" , "normal" },
																{ "roughness" , "roughness" },
																{ "roughness_x" , "roughness" },
                                                                { "weight" , "weight" }, 
                                                                { "ior" , "ior" },
                                                                { "color0", "base_material" },
                                                                { "color1", "top_material" }, };
	const std::set<std::string> kIgnoreInputnames = { "roughness_y",
														"rotation"};
	//these materials are unsupported now:
	const std::set<int> kUnsupportedMaterials = { RPR_MATERIAL_NODE_ADD,
													 RPR_MATERIAL_NODE_ARITHMETIC,
													 RPR_MATERIAL_NODE_BLEND_VALUE,
													 RPR_MATERIAL_NODE_VOLUME,
													 RPR_MATERIAL_NODE_INPUT_LOOKUP, };
}
MaterialObject::MaterialObject(rpr_material_node_type in_type)
{
    m_is_tex = false;
    switch (in_type)
    {
    case RPR_MATERIAL_NODE_DIFFUSE:
    case RPR_MATERIAL_NODE_ORENNAYAR:
    {
        SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
        m_mat = mat;
        break;
    }
	case RPR_MATERIAL_NODE_WARD:
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
	{
		//these materials are unsupported
		if (kUnsupportedMaterials.find(in_type) == kUnsupportedMaterials.end())
		{
			throw Exception(RPR_ERROR_INTERNAL_ERROR, "MaterialObject: need to update list of unsupported materials.");
		}
		SingleBxdf* mat = new SingleBxdf(SingleBxdf::BxdfType::kLambert);
		mat->SetInputValue("albedo", float3(1.f, 0.f, 0.f));
		m_mat = mat;
		break;
	}
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

 //   //TODO: fix only 4 components data supported
	//if (in_format.num_components != 4 && in_format.num_components != 3)
	//{
	//	throw Exception(RPR_ERROR_UNIMPLEMENTED, "TextureObject: only 3 and 4 component texture implemented.");
	//}

    //tex size
    int2 tex_size(in_image_desc->image_width, in_image_desc->image_height);

    //texture takes ownership of its data array
    //so need to copy input data
	int pixels_count = tex_size.x * tex_size.y;

	//bytes per pixel
	int pixel_bytes = in_format.num_components;
	int component_bytes = 1;
    Texture::Format data_format = Texture::Format::kRgba8;
    switch (in_format.type)
    {
    case RPR_COMPONENT_TYPE_UINT8:
        break;
    case RPR_COMPONENT_TYPE_FLOAT16:
		component_bytes = 2;
        data_format = Texture::Format::kRgba16;
        break;
    case RPR_COMPONENT_TYPE_FLOAT32:
		component_bytes = 4;
        data_format = Texture::Format::kRgba32;
        break;
    default:
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "TextureObject: invalid format type.");
    }
	pixel_bytes *= component_bytes;
	int data_size = 4 * component_bytes * pixels_count;//4 component baikal texture
    char* data = new char[data_size];
	if (in_format.num_components == 4)
	{
		//copy data
		memcpy(data, in_data, data_size);
	}
	else
	{
		//copy to 4component texture
		const char* in_data_cast = static_cast<const char*>(in_data);
		for (int i = 0; i < pixels_count; ++i)
		{
			//copy
			for (int comp_ind = 0; comp_ind < in_format.num_components; ++comp_ind)
			{
				int index = comp_ind * component_bytes;
				memcpy(&data[i * 4 * component_bytes + index], &in_data_cast[i * in_format.num_components * component_bytes + index], component_bytes);
			}
			//clean other colors
			for (int comp_ind = in_format.num_components; comp_ind < 4; ++comp_ind)
			{
				int index = comp_ind * component_bytes;
				memset(&data[i * 4 + comp_ind], 0, component_bytes);
			}
		}
	}
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

std::string MaterialObject::TranslatePropName(const std::string& in, Type type)
{
    std::string result;
    if (type == Type::kBumpMap)
    {
        return "bump";
    }
    else if (type == Type::kNormalMap)
    {
        return "normal";
    }

    auto it = kInputNamesDictionary.find(in);
    if (it != kInputNamesDictionary.end())
    {
        result = it->second;
    }
    else
    {
        throw Exception(RPR_ERROR_UNIMPLEMENTED, "MaterialObject: unimplemented input.");

    }

    return result;
}


void MaterialObject::SetInputN(const std::string& input_name, MaterialObject* input)
{
	//TODO: fix
	if (kUnsupportedMaterials.find(m_type) != kUnsupportedMaterials.end() ||
		kUnsupportedMaterials.find(input->GetType()) != kUnsupportedMaterials.end())
	{
		//this is unsupported material, so don't update anything
		return;
	}
	
	//TODO: 
	if (kIgnoreInputnames.find(input_name) != kIgnoreInputnames.end())
	{
		//ignore inputs
		return;
	}

	//TODO: fix
	//can't set material to color input
	if (input_name == "color" && input->IsMaterial())
	{
		return;
	}

    //convert input name
    if (m_type == Type::kImage)
    {
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "MaterialObject: trying SetInputN for rpr_image object.");
    }

    if (IsTexture())
    {
		//TODO: handle texture materials nodes
		return;
        //throw Exception(RPR_ERROR_UNIMPLEMENTED, "MaterialObject: SetInputN not implemented for Texture materials.");
    }

    //translate material name
    std::string name = TranslatePropName(input_name, input->GetType());

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
	//TODO:
	if (kUnsupportedMaterials.find(m_type) != kUnsupportedMaterials.end())
	{
		//this is unsupported material, so don't update anything
		return;
	}
	//TODO: 
	if (kIgnoreInputnames.find(input_name) != kIgnoreInputnames.end())
	{
		//ignore inputs
		return;
	}
    //convert input name
    if (m_type == Type::kImage)
    {
        throw Exception(RPR_ERROR_INVALID_PARAMETER, "MaterialObject: trying SetInputValue for rpr_image object.");
    }

    if (IsTexture())
    {
		//TODO: handle texture materials nodes
		return;
        //throw Exception(RPR_ERROR_UNIMPLEMENTED, "MaterialObject: SetInputValue not implemented for Texture materials.");
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
	if (kUnsupportedMaterials.find(m_type) != kUnsupportedMaterials.end())
	{
		//this is unsupported material, so don't update anything
		return;
	}

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