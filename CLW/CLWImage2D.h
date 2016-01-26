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

    CLWImage2D(){}
    virtual ~CLWImage2D();

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
