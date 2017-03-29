#include "image_io.h"
#include "../texture.h"

#include "OpenImageIO/imageio.h"

namespace Baikal
{
    class Oiio : public ImageIo
    {
    public:
        Texture* LoadImage(std::string const& filename) const override;
        void SaveImage(std::string const& filename, Texture const* texture) const override;
    };
    
    static Texture::Format GetTextureForemat(OIIO_NAMESPACE::ImageSpec const& spec)
    {
        OIIO_NAMESPACE_USING
        
        if (spec.format.basetype == TypeDesc::UINT8)
            return Texture::Format::kRgba8;
        else if (spec.format.basetype == TypeDesc::HALF)
            return Texture::Format::kRgba16;
        else
            return Texture::Format::kRgba32;
    }
    
    static OIIO_NAMESPACE::TypeDesc GetTextureForemat(Texture::Format fmt)
    {
        OIIO_NAMESPACE_USING
        
        if (fmt == Texture::Format::kRgba8)
            return  TypeDesc::UINT8;
        else if (fmt == Texture::Format::kRgba16)
            return TypeDesc::HALF;
        else
            return TypeDesc::FLOAT;
    }
    
    Texture* Oiio::LoadImage(const std::string &filename) const
    {
        OIIO_NAMESPACE_USING
        
        ImageInput* input = ImageInput::open(filename);
        
        if (!input)
        {
            throw std::runtime_error("Can't load " + filename + " image");
        }
        
        ImageSpec const& spec = input->spec();
        
        auto fmt = GetTextureForemat(spec);
        char* texturedata = nullptr;

        if (fmt == Texture::Format::kRgba8)
        {
            auto size = spec.width * spec.height * spec.depth * 4;
            
            texturedata = new char[size];
            
            // Read data to storage
            input->read_image(TypeDesc::UINT8, texturedata, sizeof(char) * 4);
            
            // Close handle
            input->close();
        }
        else if (fmt == Texture::Format::kRgba16)
        {
            auto size = spec.width * spec.height * spec.depth * sizeof(float) * 2;
            
            // Resize storage
            texturedata = new char[size];
            
            // Read data to storage
            input->read_image(TypeDesc::HALF, texturedata, sizeof(float) * 2);
            
            // Close handle
            input->close();
        }
        else
        {
            auto size = spec.width * spec.height * spec.depth * sizeof(RadeonRays::float3);
            
            // Resize storage
            texturedata = new char[size];
            
            // Read data to storage
            input->read_image(TypeDesc::FLOAT, texturedata, sizeof(RadeonRays::float3));
            
            // Close handle
            input->close();
        }

        // Return new texture
        auto tex =  new Texture(texturedata, RadeonRays::int2(spec.width, spec.height), fmt);

        // Cleanup
        delete input;

        //
        return tex;
    }
    
    void Oiio::SaveImage(std::string const& filename, Texture const* texture) const
    {
        OIIO_NAMESPACE_USING;
        
        ImageOutput* out = ImageOutput::create(filename);
        
        if (!out)
        {
            throw std::runtime_error("Can't create image file on disk");
        }
        
        auto dim = texture->GetSize();
        auto fmt = GetTextureForemat(texture->GetFormat());
        
        ImageSpec spec(dim.x, dim.y, 4, fmt);
        
        out->open(filename, spec);
        
        out->write_image(fmt, texture->GetData());

        out->close();
        
    }
    
    ImageIo* ImageIo::CreateImageIo()
    {
        return new Oiio();
    }
}
