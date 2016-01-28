//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#ifndef PAYLOAD_CL
#define PAYLOAD_CL


/// Ray descriptor
typedef struct _ray
{
	/// xyz - origin, w - max range
	float4 o;
	/// xyz - direction, w - time
	float4 d;
	/// x - ray mask, y - activity flag
	int2 extra;
	int2 padding;
} ray;

/// Intersection data returned by FireRays
typedef struct _Intersection
{
	// uv - hit barycentrics, w - ray distance
	float4 uvwt;
	// id of a shape
	int shapeid;
	// Primitive index
	int primid;
} Intersection;

// Shape description
typedef struct _Shape
{
	// Shape starting index
	int startidx;
	// Number of primitives in the shape
	int numprims;
	// Start vertex
	int startvtx;
	// Number of vertices
	int numvertices;
	// Linear motion vector
	float3 linearvelocity;
	// Angular velocity
	float4 angularvelocity;
	// Transform in row major format
	float4 m0;
	float4 m1;
	float4 m2;
	float4 m3;
} Shape;

// Emissive object
typedef struct _Emissive
{
	// Shape index
	int shapeidx;
	// Polygon index
	int primidx;
	// Material index
	int m;
	//
	int padding;
} Emissive;


// Material description
typedef struct _Material
{
	// Color: can be diffuse, specular, whatever...
	float4 kx;
	// Refractive index
	float  ni;
	// Context dependent parameter: glossiness, etc
	float  ns;

	union
	{
		// Color map index
		int kxmapidx;
		int brdftopidx;
	};

	union
	{
		// Normal map index
		int nmapidx;
		int brdfbaseidx;
	};

	union
	{
		// Parameter map idx
		int nsmapidx;
	};

	union
	{
		// PDF
		float fresnel;
	};

	int type;
	float padding;

} Material;

typedef struct _Scene
{
	// Vertices
	__global float3 const* vertices;
	// Normals
	__global float3 const* normals;
	// UVs
	__global float2 const* uvs;
	// Indices
	__global int const* indices;
	// Shapes
	__global Shape const* shapes;
	// Material IDs
	__global int const* materialids;
	// Materials
	__global Material const* materials;
	// Emissive objects
	__global Emissive const* emissives;
	// Envmap idx
	int envmapidx;
	// Envmap multiplier
	float envmapmul;
	// Number of emissive objects
	int numemissives;
} Scene;

// Hit data
typedef struct _DifferentialGeometry
{
	// World space position
	float3 p;
	// Shading normal
	float3 n;
	// Geo normal
	float3 ng;
	// UVs
	float2 uv;
	// Derivatives
	float3 dpdu;
	float3 dpdv;
	// Material 
	Material mat;
} DifferentialGeometry;

#endif // PAYLOAD_CL