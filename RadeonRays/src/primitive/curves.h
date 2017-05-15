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
#ifndef CURVES_H
#define CURVES_H

#include <vector>
#include <memory>
#include <cassert>

#include "shapeimpl.h"
#include "math/bbox.h"
#include "math/float3.h"
#include "math/float2.h"

namespace RadeonRays
{

	///< Transformable primitive implementation which represents a set of 'curves', where each curve consists of a sequence of vertices connected by line segments.
	///< Additionally each vertex has an associated width, which then defines an intersection primitive associated with each segment 
	///> as a 'capsule' formed from the convex hull of spheres with radius equal to the given width at two endpoint vertices.
	///< The start and end vertex of each segment are specified, which allows for disjoint curves as well as curves which share one or more vertices.
	///< 
	class Curves : public ShapeImpl
	{
	public:

		// vertices is an array of numVerts vertices, with byte stride vstride between 4-float vertices,
		// where each 4-float vertex contains the vertex position as the .xyz components, and the curve width at that vertex as the .w component.
		// segmentIndices is an int array of length 2*numSegments, with the start and end vertex index of each segment.
		Curves(float const* vertices, int numVerts, int vstride, int const* segmentIndices, int numSegments);

		~Curves();

		int num_segments() const { return (int)segmentIndices_.size()/2; }
		int num_vertices() const { return (int)vertices_.size(); }

		// Returns an array of num_vertices float4
		const float4* GetVertexData() const { return &vertices_[0]; }

		// Return an array of 2*num_segments ints
		const int* GetSegmentData() const { return &segmentIndices_[0];  }

		// Returns the indices of an individual segment
		void GetSegmentData(int segmentidx, int& vStart, int& vEnd) const;

		void GetSegmentBounds(int segmentidx, bool objectspace, bbox& bounds) const;

	private:

		/// Disallow to copy curves, too heavy
		Curves(Curves const& o);
		Curves& operator = (Curves const& o);

		/// Data
		std::vector<float4> vertices_;
		std::vector<int> segmentIndices_;

	};

}

#endif // CURVES_H
