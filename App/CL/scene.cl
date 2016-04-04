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
#ifndef SCENE_CL
#define SCENE_CL

#include "../App/CL/utils.cl"
#include "../App/CL/payload.cl"

/// Fill DifferentialGeometry structure based on intersection info from FireRays
void FillDifferentialGeometry(// Scene
                              Scene const* scene,
                              // FireRays intersection
                              Intersection const* isect,
                              // Differential geometry
                              DifferentialGeometry* diffgeo
                              )
{
    // Determine shape and polygon
    int shapeid = isect->shapeid - 1;
    int primid = isect->primid;
    
    // Get barycentrics
    float2 uv = isect->uvwt.xy;
    
    // Extract shape data
    Shape shape = scene->shapes[shapeid];
    //float3 linearvelocity = shape.linearvelocity;
    //float4 angularvelocity = shape.angularvelocity;
    
    // Fetch indices starting from startidx and offset by primid
    int i0 = scene->indices[shape.startidx + 3 * primid];
    int i1 = scene->indices[shape.startidx + 3 * primid + 1];
    int i2 = scene->indices[shape.startidx + 3 * primid + 2];
    
    // Fetch normals
    float3 n0 = scene->normals[shape.startvtx + i0];
    float3 n1 = scene->normals[shape.startvtx + i1];
    float3 n2 = scene->normals[shape.startvtx + i2];
    
    // Fetch positions
    float3 v0 = scene->vertices[shape.startvtx + i0];
    float3 v1 = scene->vertices[shape.startvtx + i1];
    float3 v2 = scene->vertices[shape.startvtx + i2];
    
    // Fetch UVs
    float2 uv0 = scene->uvs[shape.startvtx + i0];
    float2 uv1 = scene->uvs[shape.startvtx + i1];
    float2 uv2 = scene->uvs[shape.startvtx + i2];
    
    // Calculate barycentric position and normal
    diffgeo->n = normalize(transform_vector((1.f - uv.x - uv.y) * n0 + uv.x * n1 + uv.y * n2, shape.m0, shape.m1, shape.m2, shape.m3));
    diffgeo->p = transform_point((1.f - uv.x - uv.y) * v0 + uv.x * v1 + uv.y * v2, shape.m0, shape.m1, shape.m2, shape.m3);
    diffgeo->uv = (1.f - uv.x - uv.y) * uv0 + uv.x * uv1 + uv.y * uv2;
    
    // Get material at shading point
    int matidx = scene->materialids[shape.startidx / 3 + primid];
    diffgeo->mat = scene->materials[matidx];
    
    
    /// From PBRT book
    /// Construct tangent basis on the fly and apply normal map
    float du1 = uv0.x - uv2.x;
    float du2 = uv1.x - uv2.x;
    float dv1 = uv0.y - uv2.y;
    float dv2 = uv1.y - uv2.y;
    
    float3 dp1 = v0 - v2;
    float3 dp2 = v1 - v2;
    
    float det = du1 * dv2 - dv1 * du2;
    
    if (0 && det != 0.f)
    {
        float invdet = 1.f / det;
        diffgeo->dpdu = normalize( (dv2 * dp1 - dv1 * dp2) * invdet );
        diffgeo->dpdv = normalize( (-du2 * dp1 + du1 * dp2) * invdet );
    }
    else
    {
        diffgeo->dpdu = normalize(GetOrthoVector(diffgeo->n));
        diffgeo->dpdv = normalize(cross(diffgeo->n, diffgeo->dpdu));
    }
    
    diffgeo->ng = normalize(cross(diffgeo->dpdv, diffgeo->dpdu));
    // Fix all to be orthogonal
    diffgeo->dpdv = normalize(cross(diffgeo->ng, diffgeo->dpdu));
    
    // Apply transform & linear motion blur
    //v += (linearvelocity * time);
    // MT^-1 should be used if scale is present
    //n = rotate_vector(n, angularvelocity);
}

int Scene_SampleLight(Scene const* scene, float sample, float* pdf)
{
	int numlights = (scene->envmapidx == -1) ? scene->numemissives : (scene->numemissives + 1);
	int lightidx = clamp((int)(sample * numlights), 0, numlights - 1);
	*pdf = 1.f / numlights;
	return lightidx;
}


#endif
