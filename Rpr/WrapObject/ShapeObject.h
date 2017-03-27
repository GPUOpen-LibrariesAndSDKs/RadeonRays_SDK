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

#include "RadeonProRender.h"
#include "App/Scene/shape.h"
#include "WrapObject/WrapObject.h"
#include "WrapObject/MaterialObject.h"

namespace Baikal {
    class Shape;
}

//this class represent rpr_scene
class ShapeObject
    : public WrapObject
{
private:

    ShapeObject(Baikal::Shape* shape, bool is_instance);
    virtual ~ShapeObject();

public:
    static ShapeObject* CreateMesh(rpr_float const * in_vertices, size_t in_num_vertices, rpr_int in_vertex_stride,
        rpr_float const * in_normals, size_t in_num_normals, rpr_int in_normal_stride,
        rpr_float const * in_texcoords, size_t in_num_texcoords, rpr_int in_texcoord_stride,
        rpr_int const * in_vertex_indices, rpr_int in_vidx_stride,
        rpr_int const * in_normal_indices, rpr_int in_nidx_stride,
        rpr_int const * in_texcoord_indices, rpr_int in_tidx_stride,
        rpr_int const * in_num_face_vertices, size_t in_num_faces);

    ShapeObject* CreateInstance();

    
    void SetTransform(const RadeonRays::matrix& m) { m_shape->SetTransform(m); };
    void SetMaterial(MaterialObject* mat);

    Baikal::Shape* GetShape() { return m_shape; }
private:
    Baikal::Shape* m_shape;
    bool m_is_instance;
};