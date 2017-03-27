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

#include "WrapObject/FramebufferObject.h"
#include "WrapObject/Exception.h"
#include "App/CLW/clwoutput.h"
#include "OpenImageIO/imageio.h"
#include "RadeonProRender.h"

FramebufferObject::FramebufferObject()
    : m_out(nullptr)
{

}

FramebufferObject::~FramebufferObject()
{
    delete m_out;
    m_out = nullptr;
}

int FramebufferObject::GetWidth()
{
    return m_out->width();
}

int FramebufferObject::GetHeight()
{
    return m_out->height();

}

void FramebufferObject::GetData(void* out_data)
{
    m_out->GetData(static_cast<RadeonRays::float3*>(out_data));
}

void FramebufferObject::Clear()
{
    Baikal::ClwOutput* output = dynamic_cast<Baikal::ClwOutput*>(m_out);
    output->Clear(RadeonRays::float3(0.f, 0.f, 0.f, 0.f));
}
void FramebufferObject::SaveToFile(const char* path)
{
    OIIO_NAMESPACE_USING;

    int width = m_out->width();
    int height = m_out->height();
    std::vector<RadeonRays::float3> tempbuf(width * height);
    m_out->GetData(tempbuf.data());
    std::vector<RadeonRays::float3> data(tempbuf);

    //convert pixels
    for (auto y = 0; y < height; ++y)
    {
        for (auto x = 0; x < width; ++x)
        {

            RadeonRays::float3 val = data[(height - 1 - y) * width + x];
            tempbuf[y * width + x] = (1.f / val.w) * val;

            tempbuf[y * width + x].x = std::pow(tempbuf[y * width + x].x, 1.f / 2.2f);
            tempbuf[y * width + x].y = std::pow(tempbuf[y * width + x].y, 1.f / 2.2f);
            tempbuf[y * width + x].z = std::pow(tempbuf[y * width + x].z, 1.f / 2.2f);
        }
    }

    //save results to file
    ImageOutput* out = ImageOutput::create(path);
    if (!out)
    {
        throw Exception(RPR_ERROR_IO_ERROR, "FramebufferObject: failed to create file.");
    }

    ImageSpec spec(width, height, 3, TypeDesc::FLOAT);
    out->open(path, spec);
    out->write_image(TypeDesc::FLOAT, &tempbuf[0], sizeof(RadeonRays::float3));
    out->close();
    delete out;
}