/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

?   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
?   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
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