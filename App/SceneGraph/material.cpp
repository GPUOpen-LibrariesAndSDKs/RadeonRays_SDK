#include "material.h"
#include "iterator.h"

#include <cassert>

namespace Baikal
{
    class Material::InputIterator : public Iterator
    {
    public:
        
        InputIterator(InputMap::const_iterator begin, InputMap::const_iterator end)
        : m_begin(begin)
        , m_end(end)
        {
            Reset();
        }
        
        // Check if we reached end
        bool IsValid() const override
        {
            return m_cur != m_end;
        }
        
        // Advance by 1
        void Next() override
        {
            ++m_cur;
        }
        
        // Set to starting iterator
        void Reset() override
        {
            m_cur = m_begin;
        }
        
        // Get underlying item
        void const* Item() const override
        {
            return &m_cur->second;
        }
        
    private:
        InputMap::const_iterator m_begin;
        InputMap::const_iterator m_end;
        InputMap::const_iterator m_cur;
    };
    
   
    
    
    Material::Material()
    : m_thin(false)
    {
    }
    
    void Material::RegisterInput(std::string const& name, std::string const& desc, std::set<InputType>&& supported_types)
    {
        Input input = {{name, desc, std::move(supported_types)}};
        
        assert(input.info.supported_types.size() > 0);
        
        input.value.type = *input.info.supported_types.begin();
        
        switch (input.value.type)
        {
            case InputType::kFloat4:
                input.value.float_value = RadeonRays::float4();
                break;
            case InputType::kTexture:
                input.value.tex_value = nullptr;
                break;
            case InputType::kMaterial:
                input.value.mat_value = nullptr;
                break;
            default:
                break;
        }
        
        m_inputs.emplace(std::make_pair(name, input));
    }
    
    void Material::ClearInputs()
    {
        m_inputs.clear();
    }

    
    // Iterator of dependent materials (plugged as inputs)
    Iterator* Material::CreateMaterialIterator() const
    {
        std::set<Material const*> materials;
        
        std::for_each(m_inputs.cbegin(), m_inputs.cend(),
                      [&materials](std::pair<std::string, Input> const& map_entry)
                      {
                          if (map_entry.second.value.type == InputType::kMaterial &&
                              map_entry.second.value.mat_value != nullptr)
                          {
                              materials.insert(map_entry.second.value.mat_value);
                          }
                      }
                      );
        
        return new ContainerIterator<std::set<Material const*>>(std::move(materials));
    }
    
    // Iterator of textures (plugged as inputs)
    Iterator* Material::CreateTextureIterator() const
    {
        std::set<Texture const*> textures;
        
        std::for_each(m_inputs.cbegin(), m_inputs.cend(),
                      [&textures](std::pair<std::string, Input> const& map_entry)
                      {
                          if (map_entry.second.value.type == InputType::kTexture &&
                              map_entry.second.value.tex_value != nullptr)
                          {
                              textures.insert(map_entry.second.value.tex_value);
                          }
                      }
                      );
        
        return new ContainerIterator<std::set<Texture const*>>(std::move(textures));
    }
    
    // Iterator of inputs
    Iterator* Material::CreateInputIterator() const
    {
        return new InputIterator(m_inputs.cbegin(), m_inputs.cend());
    }
    
    // Set input value
    // If specific data type is not supported throws std::runtime_error
    void Material::SetInputValue(std::string const& name, RadeonRays::float4 const& value)
    {
        auto input_iter = m_inputs.find(name);
        
        if (input_iter != m_inputs.cend())
        {
            auto& input = input_iter->second;
            
            if (input.info.supported_types.find(InputType::kFloat4) != input.info.supported_types.cend())
            {
                input.value.type = InputType::kFloat4;
                input.value.float_value = value;
                SetDirty(true);
            }
            else
            {
                throw std::runtime_error("Input type not supported");
            }
        }
        else
        {
            throw std::runtime_error("No such input");
        }
    }
    
    void Material::SetInputValue(std::string const& name, Texture const* texture)
    {
        auto input_iter = m_inputs.find(name);
        
        if (input_iter != m_inputs.cend())
        {
            auto& input = input_iter->second;
            
            if (input.info.supported_types.find(InputType::kTexture) != input.info.supported_types.cend())
            {
                input.value.type = InputType::kTexture;
                input.value.tex_value = texture;
                SetDirty(true);
            }
            else
            {
                throw std::runtime_error("Input type not supported");
            }
        }
        else
        {
            throw std::runtime_error("No such input");
        }
    }
    
    void Material::SetInputValue(std::string const& name, Material const* material)
    {
        auto input_iter = m_inputs.find(name);
        
        if (input_iter != m_inputs.cend())
        {
            auto& input = input_iter->second;
            
            if (input.info.supported_types.find(InputType::kMaterial) != input.info.supported_types.cend())
            {
                input.value.type = InputType::kMaterial;
                input.value.mat_value = material;
                SetDirty(true);
            }
            else
            {
                throw std::runtime_error("Input type not supported");
            }
        }
        else
        {
            throw std::runtime_error("No such input");
        }
    }
    
    Material::InputValue Material::GetInputValue(std::string const& name) const
    {
        auto input_iter = m_inputs.find(name);
        
        if (input_iter != m_inputs.cend())
        {
            auto& input = input_iter->second;
            
            return input.value;
        }
        else
        {
            throw std::runtime_error("No such input");
        }
    }

    bool Material::IsThin() const
    {
        return m_thin;
    }
    
    void Material::SetThin(bool thin)
    {
        m_thin = thin;
        SetDirty(true);
    }

    SingleBxdf::SingleBxdf(BxdfType type)
    : m_type(type)
    {
        RegisterInput("albedo", "Diffuse color", {InputType::kFloat4, InputType::kTexture});
        RegisterInput("normal", "Normal map", {InputType::kTexture});
        RegisterInput("bump", "Bump map", { InputType::kTexture });
        RegisterInput("ior", "Index of refraction", {InputType::kFloat4});
        RegisterInput("fresnel", "Fresnel flag", {InputType::kFloat4});
        RegisterInput("roughness", "Roughness", {InputType::kFloat4, InputType::kTexture});
        
        SetInputValue("albedo", RadeonRays::float4(0.7f, 0.7f, 0.7f, 1.f));
        SetInputValue("normal", static_cast<Texture const*>(nullptr));
        SetInputValue("bump", static_cast<Texture const*>(nullptr));
    }
    
    SingleBxdf::BxdfType SingleBxdf::GetBxdfType() const
    {
        return m_type;
    }
    
    void SingleBxdf::SetBxdfType(BxdfType type)
    {
        m_type = type;
        SetDirty(true);
    }

    bool SingleBxdf::HasEmission() const
    {
        return m_type == BxdfType::kEmissive;
    }
    
    MultiBxdf::MultiBxdf(Type type)
    : m_type(type)
    {
        RegisterInput("base_material", "Base material", {InputType::kMaterial});
        RegisterInput("top_material", "Top material", {InputType::kMaterial});
        RegisterInput("ior", "Index of refraction", {InputType::kFloat4});
        RegisterInput("weight", "Blend weight", {InputType::kFloat4, InputType::kTexture});
    }
    
    MultiBxdf::Type MultiBxdf::GetType() const
    {
        return m_type;
    }
    
    void MultiBxdf::SetType(Type type)
    {
        m_type = type;
        SetDirty(true);
    }

    bool MultiBxdf::HasEmission() const
    {
        InputValue base = GetInputValue("base_material");
        InputValue top = GetInputValue("base_material");

        if (base.mat_value && base.mat_value->HasEmission())
            return true;
        if (top.mat_value && top.mat_value->HasEmission())
            return true;

        return false;
    }


}
