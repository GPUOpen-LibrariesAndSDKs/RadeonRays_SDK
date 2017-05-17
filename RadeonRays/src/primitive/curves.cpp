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

#include "curves.h"

#include "../except/except.h"

#include <algorithm>
#include <functional>

namespace RadeonRays
{

	Curves::Curves(float const* vertices, int numVerts, int vstride,
		           int const* segmentIndices, int numSegments) : ShapeImpl(SHAPE_CURVES)
	{
		vertices_.resize(numVerts);
		segmentIndices_.resize(2*numSegments);

		// Load vertices
#pragma omp parallel for
		for (int vi=0; vi<numVerts; ++vi)
		{
			float const* v = (float const*)((char*)vertices + vi*vstride);
			float4 temp;
			temp.x = v[0];
			temp.y = v[1];
			temp.z = v[2];
			temp.w = v[3];
			vertices_[vi] = temp;
		}

#pragma omp parallel for
		for (int i=0; i<2*numSegments; ++i)
		{
			segmentIndices_[i] = *((int const*)((char const*)segmentIndices +i*sizeof(int)));
		}
	}

	void Curves::GetSegmentData(int segmentidx, int& vStart, int& vEnd) const
	{
		vStart = segmentIndices_[2*segmentidx];
		vEnd   = segmentIndices_[2*segmentidx+1];
	}

	void Curves::GetSegmentBounds(int segmentidx, bool objectspace, bbox& bounds) const
	{
		int viStart = segmentIndices_[2 * segmentidx];
		int viEnd   = segmentIndices_[2 * segmentidx + 1];
		float4 vStaL = vertices_[viStart];
		float4 vEndL = vertices_[viEnd];

		// (NB, the .w component of the vectors, here containing radii, is not involved in this transform).
		float3 vStaW = transform_point(vStaL, objectspace ? matrix() : worldmat_);
		float3 vEndW = transform_point(vEndL, objectspace ? matrix() : worldmat_);

		float rStaL = vStaW.w;
		float rEndL = vEndL.w;

		// We cannot non-uniformly scale the capsule primitive of each segment, so we must
		// approximate by taking the maximum axis scale.
		float worldScale = std::max(std::max(worldmat_.m00, worldmat_.m11), worldmat_.m22);
		float rStaW = rStaL * worldScale;
		float rEndW = rEndL * worldScale;

		float3 segmentDir = (vEndW - vStaW);
		segmentDir.normalize();

		bbox segmentBounds(vStaW, vEndW);
		{
			float3 sta_disp = float3(rStaW, rStaW, rStaW);
			float3 end_disp = float3(rEndW, rEndW, rEndW);
			bbox sta_box(vStaW-sta_disp, vStaW+sta_disp);
			bbox end_box(vEndW-end_disp, vEndW+end_disp);
			segmentBounds.grow(sta_box);
			segmentBounds.grow(end_box);
		}
		bounds = segmentBounds;
	}

	Curves::~Curves()
	{

	}
}
