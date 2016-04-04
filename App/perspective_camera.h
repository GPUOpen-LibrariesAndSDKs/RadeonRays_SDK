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