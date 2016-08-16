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
#ifndef INSTANCE_H
#define INSTANCE_H

#include <vector>
#include <memory>

#include "shapeimpl.h"
#include "math/float3.h"
#include "math/float2.h"


namespace RadeonRays
{
    ///< Instance represents a refernce to other primitive with different
    ///< world transform. Intended to lower memory requirements for
    ///< replicated geometry.
    ///<
    class Instance : public ShapeImpl
    {
    public:
        // Constructor
        Instance(Shape const* baseshape);

        // Get the shape this instance is based on
        Shape const* GetBaseShape() const;

        // Instance flag
        bool is_instance() const;

		// Test functions, fires a single ray into this shape via unoptimised CPU code
		bool TestOcclusion(const ray& r) const override;

		void TestIntersection(const ray& r, Intersection& isect) const override;

    private:
        /// Disallow to copy meshes, too heavy
        Instance(Instance const& o);
        Instance& operator = (Instance const& o);

        /// Base shape
        Shape const* shape_;
    };

    inline Instance::Instance(Shape const* baseshape)
        : shape_(baseshape)
    {
    }

    inline Shape const* Instance::GetBaseShape() const
    {
        return shape_;
    }

    inline bool Instance::is_instance() const
    {
        return true;
    }

	inline bool Instance::TestOcclusion(const ray& r) const
	{
		// note this won't pass the instance transform to known mesh base shape
		// this shouldn't be a problem at the moment, but possible in future...
		const auto mesh = dynamic_cast<const Mesh*>(GetBaseShape());
		if(mesh != nullptr )
		{
			return mesh->TestOcclusion(r, worldmat_);
		} else
		{
			//			GetBaseShape()->TestOcclusion(r, isect);
			return GetBaseShape()->TestOcclusion(r);
			
		}
	}
	inline void Instance::TestIntersection(const ray& r, Intersection& isect) const
    {
		// note this won't pass the instance transform to known mesh base shape
		// this shouldn't be a problem at the moment, but possible in future...
		const auto mesh = dynamic_cast<const Mesh*>(GetBaseShape());
		if (mesh != nullptr)
		{
			// This may cause a false positive if the inner mesh is used
			// both as a real mesh and an inner mesh. Don't do that for the Test call
			assert(isect.shapeid != mesh->GetId());

			mesh->TestIntersection(r, worldmat_, isect);
			// if we hit the mesh shape, pretend it hit the instance
			if(isect.shapeid == mesh->GetId())
			{
                isect.shapeid = GetId();
			}
		}
		else
		{
			if (GetBaseShape() != nullptr)
			{
				GetBaseShape()->TestIntersection(r, isect);
			}
		}
    }

}

#endif // MESH_H
