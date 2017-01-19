#include "material_io.h"

#include "Scene/scene1.h"
#include "Scene/iterator.h"
#include "Scene/shape.h"
#include "Scene/material.h"

#include "Scene/IO/image_io.h"
#include "Scene/Collector/Collector.h"


#include "XML/tinyxml2.h"

#include <sstream>
#include <map>
#include <stack>

namespace Baikal
{
    using namespace tinyxml2;
    
    // XML based material IO implememtation
    class MaterialIoXML : public MaterialIo
    {
    public:
        // Save materials to disk
        void SaveMaterials(std::string const& filename, Scene1 const& scene) override;
        
    private:
        // Write single material
        void WriteMaterial(ImageIo& io, XMLPrinter& printer, Material const* material);
        // Write single material input
        void WriteInput(ImageIo& io, XMLPrinter& printer, std::string const& name, Material::InputValue value);
        
        // Texture to name map
        std::map<Texture const*, std::string> m_textures;
    };
    
    MaterialIo* MaterialIo::CreateMaterialIoXML()
    {
        return new MaterialIoXML();
    }

    static std::string Float4ToString(RadeonRays::float3 const& v)
    {
        std::ostringstream oss;
        oss << v.x << " " <<  v.y << " " << v.z << " " << v.w;
        return oss.str();
    }
    
    void MaterialIoXML::WriteInput(ImageIo& io, XMLPrinter& printer, std::string const& name, Material::InputValue value)
    {
        printer.OpenElement("Input");
        
        printer.PushAttribute("name", name.c_str());
        
        if (value.type == Material::InputType::kFloat4)
        {
            printer.PushAttribute("type", "float4");
            printer.PushAttribute("value", Float4ToString(value.float_value).c_str());
        }
        else if (value.type == Material::InputType::kTexture)
        {
            printer.PushAttribute("type", "texture");
            
            auto iter = m_textures.find(value.tex_value);
            
            if (iter != m_textures.cend())
            {
                printer.PushAttribute("value", iter->second.c_str());
            }
            else
            {
                std::ostringstream oss;
                oss << (std::uint64_t)value.tex_value << ".jpg";
                
                io.SaveImage(oss.str(), value.tex_value);
                
                m_textures[value.tex_value] = oss.str();
                
                printer.PushAttribute("value", oss.str().c_str());
            }
        }
        else
        {
            printer.PushAttribute("type", "material");
            
            printer.PushAttribute("value", (int)(reinterpret_cast<std::uint64_t>(value.mat_value)));
        }
        
        printer.CloseElement();
    }
    
    void MaterialIoXML::WriteMaterial(ImageIo& io, XMLPrinter& printer, Material const* material)
    {
        printer.OpenElement("Material");
        
        printer.PushAttribute("name", material->GetName().c_str());
        
        printer.PushAttribute("id", (int)(reinterpret_cast<std::uint64_t>(material)));
        
        printer.PushAttribute("twosided", material->IsTwoSided());
        
        SingleBxdf const* bxdf = dynamic_cast<SingleBxdf const*>(material);
        
        if (bxdf)
        {
            printer.PushAttribute("type", "simple");
            
            SingleBxdf::BxdfType type = bxdf->GetBxdfType();
            
            printer.PushAttribute("bxdf", (int)type);
            
            SingleBxdf::InputValue albedo = bxdf->GetInputValue("albedo");
            
            WriteInput(io, printer,"albedo", albedo);
            
            SingleBxdf::InputValue normal = bxdf->GetInputValue("normal");
            
            if (normal.tex_value)
            {
                WriteInput(io, printer, "normal", normal);
            }
            
            SingleBxdf::InputValue ior = bxdf->GetInputValue("ior");
            
            WriteInput(io, printer,"ior", ior);
            
            SingleBxdf::InputValue fresnel = bxdf->GetInputValue("fresnel");
            
            WriteInput(io, printer,"fresnel", fresnel);
            
            if (type == SingleBxdf::BxdfType::kMicrofacetGGX ||
                type == SingleBxdf::BxdfType::kMicrofacetBeckmann ||
                type == SingleBxdf::BxdfType::kMicrofacetBlinn ||
                type == SingleBxdf::BxdfType::kMicrofacetRefractionGGX ||
                type == SingleBxdf::BxdfType::kMicrofacetRefractionBeckmann)
            {
                SingleBxdf::InputValue roughness = bxdf->GetInputValue("roughness");
                WriteInput(io, printer,"roughness", roughness);
            }
        }
        else
        {
            MultiBxdf const* blend = dynamic_cast<MultiBxdf const*>(material);
            
            printer.PushAttribute("type", "blend");
            
            MultiBxdf::Type type = blend->GetType();
            
            printer.PushAttribute("blend_type", (int)type);
            
            Material::InputValue base = material->GetInputValue("base_material");
            
            WriteInput(io, printer, "base_material", base);
            
            Material::InputValue top = material->GetInputValue("top_material");
            
            WriteInput(io, printer, "top_material", top);
            
            if (type == MultiBxdf::Type::kFresnelBlend)
            {
                Material::InputValue ior = material->GetInputValue("ior");
                
                WriteInput(io, printer, "ior", ior);
            }
            else
            {
                Material::InputValue weight = material->GetInputValue("weight");
                
                WriteInput(io, printer, "weight", weight);
            }
        }
        
        printer.CloseElement();
    }
    
    void MaterialIoXML::SaveMaterials(std::string const& filename, Scene1 const& scene)
    {
        XMLDocument doc;
        XMLPrinter printer;
        
        m_textures.clear();
        
        std::unique_ptr<ImageIo> image_io(ImageIo::CreateImageIo());
        
        std::unique_ptr<Iterator> shape_iter(scene.CreateShapeIterator());
        
        Collector mat_collector;
        // Collect materials from shapes first
        mat_collector.Collect(shape_iter.get(),
                              // This function adds all materials to resulting map
                              // recursively via Material dependency API
                              [](void const* item) -> std::set<void const*>
                              {
                                  // Resulting material set
                                  std::set<void const*> mats;
                                  // Material stack
                                  std::stack<Material const*> material_stack;
                                  
                                  // Get material from current shape
                                  auto shape = reinterpret_cast<Shape const*>(item);
                                  auto material = shape->GetMaterial();
                                  
                                  // Push to stack as an initializer
                                  material_stack.push(material);
                                  
                                  // Drain the stack
                                  while (!material_stack.empty())
                                  {
                                      // Get current material
                                      Material const* m = material_stack.top();
                                      material_stack.pop();
                                      
                                      // Emplace into the set
                                      mats.emplace(m);
                                      
                                      // Create dependency iterator
                                      std::unique_ptr<Iterator> mat_iter(m->CreateMaterialIterator());
                                      
                                      // Push all dependencies into the stack
                                      for (;mat_iter->IsValid(); mat_iter->Next())
                                      {
                                          material_stack.push(mat_iter->ItemAs<Material const>());
                                      }
                                  }
                                  
                                  // Return resulting set
                                  return mats;
                              });
        
        std::unique_ptr<Iterator> mat_iter(mat_collector.CreateIterator());
        for (;mat_iter->IsValid(); mat_iter->Next())
        {
            auto material = mat_iter->ItemAs<Material const>();
            
            if (material)
            {
                WriteMaterial(*image_io, printer, material);
            }
        }

        doc.Parse(printer.CStr());
        
        doc.SaveFile(filename.c_str());
    }

}
