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
#ifndef PERSPECTIVE_CAMERA_H
#define PERSPECTIVE_CAMERA_H

#include "math/float3.h"
#include "math/float2.h"
#include "math/ray.h"

#include <iostream>


class PerspectiveCamera
{
public:
    // Pass camera position, camera aim, camera up vector, depth limits, vertical field of view
    // and image plane aspect ratio
    PerspectiveCamera(FireRays::float3 const& eye, FireRays::float3 const& at, FireRays::float3 const& up,
                       FireRays::float2 const& zcap, float fovy, float aspect);

    // Sample is a value in [0,1] square describing where to sample the image plane
    void GenerateRay(FireRays::float2 const& sample, FireRays::ray& r) const;

    // Rotate camera around world Z axis
    void Rotate(float angle);
    // Tilt camera
    void Tilt(float angle);
    // Move along camera Z direction
    void MoveForward(float distance);
	//
	void ArcballRotateHorizontally(FireRays::float3 c, float angle);
	//
	void ArcballRotateVertically(FireRays::float3 c, float angle);

private:
    // Rotate camera around world Z axis
    void Rotate(FireRays::float3, float angle);

    // Camera coordinate frame
    FireRays::float3 forward_;
    FireRays::float3 right_;
    FireRays::float3 up_;
    FireRays::float3 p_;

    // Image plane width & hight in scene units
    FireRays::float2 dim_;

    // Near and far Z
    FireRays::float2 zcap_;
    float  fovy_;
    float  aspect_;

    friend std::ostream& operator << (std::ostream& o, PerspectiveCamera const& p);
};

inline std::ostream& operator << (std::ostream& o, PerspectiveCamera const& p)
{
    o << "Position: " << p.p_.x << " " << p.p_.y << " " << p.p_.z << "\n";
    o << "At: " << p.p_.x + p.forward_.x << " " << p.p_.y + p.forward_.y << " " << p.p_.z + p.forward_.z << "\n";
    return o;
}

#endif // PERSPECTIVE_CAMERA_H