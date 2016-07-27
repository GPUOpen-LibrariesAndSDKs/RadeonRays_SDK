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
#include "mesh.h"

#include "../except/except.h"

#include <algorithm>
#include <functional>

namespace RadeonRays
{
    Mesh::Mesh(float const* vertices, int vnum, int vstride,
        int const* vidx, int vistride,
        int* nfaceverts,
        int nfaces)
		: puretriangle_(true)
    {
        // Handle vertices
        // Allocate space in advance
        vertices_.resize(vnum);
        // Calculate vertex stride, assume dense packing if non passed
        vstride = (vstride == 0) ? (3 * sizeof(float)) : vstride;
        // Allocate space for faces
        faces_.resize(nfaces);

        // Load vertices
#pragma omp parallel for
        for (int i = 0; i < vnum; ++i)
        {
            float const* current = (float const*)((char*)vertices + i*vstride);

            float3 temp;
            temp.x = current[0];
            temp.y = current[1];
            temp.z = current[2];

            vertices_[i] = temp;
        }

        // If mesh consists of triangles only apply parallel loading
        if (nfaceverts == nullptr)
        {
			puretriangle_ = true;

            int istride = (vistride == 0) ? (3 * sizeof(int)) : vistride;

#pragma omp parallel for
            for (int i = 0; i < nfaces; ++i)
            {
                faces_[i].i0 = *((int const*)((char const*)vidx + i * istride));
                faces_[i].i1 = *((int const*)((char const*)vidx + i * istride + sizeof(int)));
                faces_[i].i2 = *((int const*)((char const*)vidx + i * istride + 2 * sizeof(int)));
                faces_[i].type_ = FaceType::TRIANGLE;
            }
        }
        // Otherwise execute serially
        else
        {
            char const* vidxptr = (char const*)vidx;

            for (int i = 0; i < nfaces; ++i)
            {
                // Triangle case
                if (nfaceverts[i] == 3)
                {
                    faces_[i].i0 = *((int const*)(vidxptr));
                    faces_[i].i1 = *((int const*)(vidxptr + sizeof(int)));
                    faces_[i].i2 = *((int const*)(vidxptr + 2 * sizeof(int)));
                    faces_[i].type_ = FaceType::TRIANGLE;

                    // Goto next primitive
                    vidxptr += (vistride == 0) ? (3 * sizeof(int)) : vistride;
                }
                // Quad case
                else if (nfaceverts[i] == 4)
                {
                    faces_[i].i0 = *((int const*)(vidxptr));
                    faces_[i].i1 = *((int const*)(vidxptr + sizeof(int)));
                    faces_[i].i2 = *((int const*)(vidxptr + 2 * sizeof(int)));
                    faces_[i].i3 = *((int const*)(vidxptr + 3 * sizeof(int)));
                    faces_[i].type_ = FaceType::QUAD;

					puretriangle_ = false;

                    // Goto next primitive
                    vidxptr += (vistride == 0) ? (4 * sizeof(int)) : vistride;
                }
                else
                {
                    throw ExceptionImpl("Wrong number of vertices per face");
                }
            }
        }
    }

    void Mesh::GetFaceBounds(int faceidx, bool objectspace, bbox& bounds) const
    {
        if (objectspace)
        {
            float3 p1 = vertices_[faces_[faceidx].i0];
            float3 p2 = vertices_[faces_[faceidx].i1];
            float3 p3 = vertices_[faces_[faceidx].i2];

            bounds = bbox(p1, p2);
            bounds.grow(p3);

            if (faces_[faceidx].type_ == FaceType::QUAD)
            {
                float3 p4 = vertices_[faces_[faceidx].i3];
                bounds.grow(p4);
            }
        }
        else
        {
            float3 p1 = transform_point(vertices_[faces_[faceidx].i0], worldmat_);
            float3 p2 = transform_point(vertices_[faces_[faceidx].i1], worldmat_);
            float3 p3 = transform_point(vertices_[faces_[faceidx].i2], worldmat_);

            bounds = bbox(p1, p2);
            bounds.grow(p3);

            if (faces_[faceidx].type_ == FaceType::QUAD)
            {
                float3 p4 = transform_point(vertices_[faces_[faceidx].i3], worldmat_);
                bounds.grow(p4);
            }
        }
    }

    Mesh::~Mesh()
    {
    }
}
