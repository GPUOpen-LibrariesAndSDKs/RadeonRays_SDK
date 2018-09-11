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
#ifndef __CLW__CLWImage2D__
#define __CLW__CLWImage2D__

#include <iostream>
#include <memory>
#include <vector>
#include <cassert>

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#include <OpenGL/OpenGL.h>
#else
#include <CL/cl.h>
#include <CL/cl_gl.h>
#endif

#include "ReferenceCounter.h"
#include "ParameterHolder.h"
#include "CLWCommandQueue.h"
#include "CLWEvent.h"


class CLWImage2D : public ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>
{
public:
    static CLWImage2D Create(cl_context context, cl_image_format const* imgFormat, size_t width, size_t height, size_t rowPitch);
    static CLWImage2D CreateFromGLTexture(cl_context context, cl_GLint texture);

    CLWImage2D() = default;
    virtual ~CLWImage2D() = default;

    operator ParameterHolder() const
    {
        return ParameterHolder((cl_mem)*this);
    }
    
private:

    CLWImage2D(cl_mem image);
    friend class CLWContext;
};

//template <typename T> CLWBuffer<T> CLWBuffer<T>::Create(size_t elementCount, cl_context context)
//{
//    cl_int status = CL_SUCCESS;
//    cl_mem deviceBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, elementCount * sizeof(T), nullptr, &status);
//    
//    assert(status == CL_SUCCESS);
//    CLWBuffer<T> buffer(deviceBuffer, elementCount);
//    
//    clReleaseMemObject(deviceBuffer);
//    
//    return buffer;
//}
//
//template <typename T> CLWBuffer<T>::CLWBuffer(cl_mem buffer, size_t elementCount)
//: ReferenceCounter<cl_mem, clRetainMemObject, clReleaseMemObject>(buffer)
//, elementCount_(elementCount)
//{
//   
//}
//
//template <typename T> CLWBuffer<T>::~CLWBuffer()
//{
//}



#endif /* defined(__CLW__CLWBuffer__) */
