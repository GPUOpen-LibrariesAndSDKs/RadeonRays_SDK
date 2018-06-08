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
#include "CLWImage2D.h"
#include "CLWExcept.h"


// Disable OCL 2.0 deprecations
#pragma warning(push)
#pragma warning(disable:4996)

CLWImage2D CLWImage2D::Create(cl_context context, cl_image_format const* imgFormat, size_t width, size_t height, size_t rowPitch)
{
    cl_int status = CL_SUCCESS;
    
    cl_image_desc desc;
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = width;
    desc.image_height = height;
    desc.image_row_pitch = rowPitch;
    desc.image_slice_pitch = 0;
    desc.num_mip_levels = 0;
    desc.num_samples = 0;
    desc.buffer = nullptr;
    
    cl_mem deviceImg = clCreateImage(context, CL_MEM_READ_WRITE, imgFormat, &desc, nullptr, &status);

    ThrowIf(status != CL_SUCCESS, status, "clCreateImage2D failed");

    CLWImage2D image(deviceImg);

    clReleaseMemObject(deviceImg);

    return image;
}


CLWImage2D CLWImage2D::CreateFromGLTexture(cl_context context, cl_GLint texture)
{
    cl_int status = CL_SUCCESS;

    // TODO: handle that gracefully: GL_TEXTURE_2D
    cl_mem deviceImg = clCreateFromGLTexture(context, CL_MEM_WRITE_ONLY, 0x0DE1, 0, texture, &status);
    
    ThrowIf(status != CL_SUCCESS, status, "clCreateFromGLTexture failed");
    
    CLWImage2D image(deviceImg);
    
    clReleaseMemObject(deviceImg);
    
    return image;
}

CLWImage2D::CLWImage2D(cl_mem image)
: ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>(image)
{
}

#pragma warning(pop)