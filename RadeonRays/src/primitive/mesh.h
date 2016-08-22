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
#ifndef MESH_H
#define MESH_H

#include <vector>
#include <memory>
#include <cassert>

#include "shapeimpl.h"
#include "math/bbox.h"
#include "math/float3.h"
#include "math/float2.h"

namespace RadeonRays
{
    ///< Transformable primitive implementation which represents
    ///< triangle mesh. Vertices, normals and uvs are indixed separately
    ///< using their own index buffers each.
    ///<
    class Mesh : public ShapeImpl
    {
    public:
        ///< Mesh might consist of multiple different face types
        enum FaceType
        {
            LINE = 0x1,
            TRIANGLE,
            QUAD
        };

        ///< Face can have up to 4 vertices
        struct Face
        {
            union
            {
                int idx[4];
                struct
                {
                    int i0, i1, i2, i3;
                };
            };
            
            FaceType type_;
        };

        //
        Mesh(float const* vertices, int vnum, int vstride,
            int const* vidx, int vistride,
            int const* nfaceverts,
            int nfaces);
        
        //
        ~Mesh();
        //
        int num_faces() const;
        //
        int num_vertices() const;
        // 
        void GetFaceBounds(int faceidx, bool objectspace, bbox& bounds) const;
        //
        float3 const* GetVertexData() const { return &vertices_[0]; }
        //
        Face const* GetFaceData() const { return &faces_[0]; }
        // True if the mesh consists of triangles only
        bool puretriangle() const { return puretriangle_;  }

    private:
        /// Disallow to copy meshes, too heavy
        Mesh(Mesh const& o);
        Mesh& operator = (Mesh const& o);

        // transforms face vertices, outverts but be at least 4 float3 in size, no of vertices in face returned
        int GetTransformedFace(int const faceidx, matrix const & transform, float3* outverts) const;

        /// Vertices
        std::vector<float3> vertices_;
        /// Primitives
        std::vector<Face> faces_;
        /// Pure triangle flag
        bool puretriangle_;
    };

    //
    inline int Mesh::num_faces() const
    {
        return (int)faces_.size();
    }

    //
    inline int Mesh::num_vertices() const
    {
        return (int)vertices_.size();
    }
}

#endif // MESH_H
