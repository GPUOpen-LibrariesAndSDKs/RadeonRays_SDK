#include "perspective_camera.h"

#include <cmath>
#include <cassert>

#include "math/quaternion.h"
#include "math/matrix.h"
#include "math/mathutils.h"


using namespace FireRays;

PerspectiveCamera::PerspectiveCamera(float3 const& eye, float3 const& at, float3 const& up,
                       float2 const& zcap, float fovy, float aspect) 
                       : p_(eye)
                       , zcap_(zcap)
                       , fovy_(fovy)
                       , aspect_(aspect)
{
    // Construct camera frame
    forward_ = normalize(at - eye);
    right_   = cross(forward_, normalize(up));
    up_      = cross(right_, forward_);

    // Image plane parameters
    // tan(fovy/2) = (height / 2) / znear
    // height = 2 * tan(fovy/2) * znear
    // width = aspect * height
    dim_.x = 2 * tanf(fovy * 0.5f) * zcap_.x;
    dim_.y = dim_.x / aspect_;
}

void PerspectiveCamera::GenerateRay(float2 const& sample, ray& r) const
{
    // Transform into [-0.5,0.5]
    float2 hsample = sample - float2(0.5f, 0.5f);
    // Transform into [-dim/2, dim/2]
    float2 csample = hsample * dim_;
    // Direction to image plane
    r.d = normalize(zcap_.x * forward_ + csample.x * right_ + csample.y * up_);
    // Origin == camera position
    r.o = p_ + zcap_.x * r.d;
    // Zcap == (znear,zfar)
    r.o.w = zcap_.y;
}

// Rotate camera around world Z axis, use for FPS camera
void PerspectiveCamera::Rotate(float angle)
{
    Rotate(float3(0,1,0), angle);
}

void PerspectiveCamera::Rotate(float3 v, float angle)
{
    /// matrix should have basis vectors in rows
    /// to be used for quaternion construction
    /// would be good to add several options
    /// to quaternion class
    matrix cam_matrix = matrix(
                               up_.x, up_.y, up_.z, 0,
                               right_.x, right_.y, right_.z, 0,
                               forward_.x, forward_.y, forward_.z, 0,
                                 0,   0,   0, 1);

	// Create camera orientation quaternion
    quaternion q = normalize(quaternion(cam_matrix));

	// Rotate camera frame around v
    q = q * rotation_quaternion(v, -angle);

	// Uncomress back to lookat
    q.to_matrix(cam_matrix);

    up_ = normalize(float3(cam_matrix.m00, cam_matrix.m01, cam_matrix.m02));
    right_ = normalize(float3(cam_matrix.m10, cam_matrix.m11, cam_matrix.m12));
    forward_ = normalize(float3(cam_matrix.m20, cam_matrix.m21, cam_matrix.m22));
}

// Tilt camera
void PerspectiveCamera::Tilt(float angle)
{
    Rotate(right_, angle);
}

// Move along camera Z direction
void PerspectiveCamera::MoveForward(float distance)
{
    p_ += distance * forward_;
}

// Arcball rotation
void PerspectiveCamera::ArcballRotateHorizontally(float3 c, float angle)
{
	// Build camera matrix
	matrix cam_matrix = matrix(
		up_.x, up_.y, up_.z, 0,
		right_.x, right_.y, right_.z, 0,
		forward_.x, forward_.y, forward_.z, 0,
		0, 0, 0, 1);

	// Create camera orientation quaternion
	quaternion q = normalize(quaternion(cam_matrix));

	// Rotation of camera around the center of interest by a == rotation of the frame by -a and rotation of the position by a
	q = q * rotation_quaternion(float3(0.f, 1.f, 0.f), -angle);

	// Rotate the frame
	q.to_matrix(cam_matrix);

	up_ = normalize(float3(cam_matrix.m00, cam_matrix.m01, cam_matrix.m02));
	right_ = normalize(float3(cam_matrix.m10, cam_matrix.m11, cam_matrix.m12));
	forward_ = normalize(float3(cam_matrix.m20, cam_matrix.m21, cam_matrix.m22));

	// Rotate the position
	float3 dir = c - p_;
	dir = rotate_vector(dir, rotation_quaternion(float3(0.f, 1.f, 0.f), angle));
	
	p_ = c - dir;
}

void PerspectiveCamera::ArcballRotateVertically(float3 c, float angle)
{
	// Build camera matrix
	matrix cam_matrix = matrix(
		up_.x, up_.y, up_.z, 0,
		right_.x, right_.y, right_.z, 0,
		forward_.x, forward_.y, forward_.z, 0,
		0, 0, 0, 1);

	// Create camera orientation quaternion
	quaternion q = normalize(quaternion(cam_matrix));

	// Rotation of camera around the center of interest by a == rotation of the frame by -a and rotation of the position by a
	q = q * rotation_quaternion(right_, -angle);

	// Rotate the frame
	q.to_matrix(cam_matrix);

	up_ = normalize(float3(cam_matrix.m00, cam_matrix.m01, cam_matrix.m02));
	right_ = normalize(float3(cam_matrix.m10, cam_matrix.m11, cam_matrix.m12));
	forward_ = normalize(float3(cam_matrix.m20, cam_matrix.m21, cam_matrix.m22));

	// Rotate the position
	float3 dir = c - p_;
	dir = rotate_vector(dir, rotation_quaternion(right_, angle));

	p_ = c - dir;
}