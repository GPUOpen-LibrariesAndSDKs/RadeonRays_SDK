
/**********************************************************************
Copyright �2015 Advanced Micro Devices, Inc. All rights reserved.

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
#include <../App/CL/utils.cl>
#include <../App/CL/random.cl>
#include <../App/CL/payload.cl>
#include <../App/CL/texture.cl>
#include <../App/CL/sampling.cl>
#include <../App/CL/normalmap.cl>
#include <../App/CL/bxdf.cl>
#include <../App/CL/light.cl>
#include <../App/CL/camera.cl>
#include <../App/CL/scene.cl>
#include <../App/CL/material.cl>



///< Sample environemnt light and produce light samples and shadow rays into separate buffer
__kernel void SampleEnvironment(
                                // Ray batch
                                __global ray const* rays,
                                // Intersection data
                                __global Intersection const* isects,
                                // Hit indices
                                __global int const* hitindices,
                                // Pixel indices
                                __global int const* pixelindices,
                                // Number of rays
                                __global int const* numhits,
                                // Vertices
                                __global float3 const* vertices,
                                // Normals
                                __global float3 const* normals,
                                // UVs
                                __global float2 const* uvs,
                                // Indices
                                __global int const* indices,
                                // Shapes
                                __global Shape const* shapes,
                                // Material IDs
                                __global int const* materialids,
                                // Materials
                                __global Material const* materials,
                                // Textures
                                TEXTURE_ARG_LIST,
                                // Environment texture index
                                int envmapidx,
                                // Envmap multiplier
                                float envmapmul,
                                // Emissives
                                __global Emissive const* emissives,
                                // Number of emissive objects
                                int numemissives,
                                // RNG seed
                                int rngseed,
                                // Shadow rays
                                __global ray* shadowrays,
                                // Light samples
                                __global float3* lightsamples,
                                // Path throughput
                                __global float3* throughput,
                                // Indirect rays
                                __global ray* indirectrays
                                )
{
    int globalid = get_global_id(0);
    
    Scene scene =
    {
        vertices,
        normals,
        uvs,
        indices,
        shapes,
        materialids,
        materials,
		emissives,
        envmapidx,
        envmapmul,
		numemissives
    };
    
    if (globalid < *numhits)
    {
        // Fetch index
        int hitidx = hitindices[globalid];
        int pixelidx = pixelindices[globalid];
        Intersection isect = isects[hitidx];

		/*if (length(throughput[pixelidx]) < 0.001f || rays[hitidx].extra.y == 0)
		{
			shadowrays[globalid].extra.y = 0;
			indirectrays[globalid].extra.y = 0;
			lightsamples[globalid] = 0.f;
			return;
		}*/
        
        // Fetch incoming ray
        float3 wi = -normalize(rays[hitidx].d.xyz);
        float time = rays[hitidx].d.w;
        
        // Fill surface data
        DifferentialGeometry diffgeo;
        FillDifferentialGeometry(&scene, &isect, &diffgeo);
        
        // Prepare RNG for sampling
        Rng rng;
        InitRng(rngseed + (globalid << 2) * 157 + 13, &rng);
        Material_Select(&scene, wi, TEXTURE_ARGS, &rng, &diffgeo);

		if (dot(diffgeo.n, wi) < 0.f && !Bxdf_IsBtdf(&diffgeo))
		{
			diffgeo.n = -diffgeo.n;
			diffgeo.dpdu = -diffgeo.dpdu;
			diffgeo.dpdv = -diffgeo.dpdv;
		}
        
        // Check if we need to apply normal map
        ApplyNormalMap(&diffgeo, TEXTURE_ARGS);
		//ApplyBumpMap(&diffgeo, TEXTURE_ARGS);
        float lightpdf = 0.f;
        float bxdflightpdf = 0.f;
        float bxdfpdf = 0.f;
        float lightbxdfpdf = 0.f;
        float3 lightwo;
        float3 bxdfwo;
        
        // Apply MIS
		int numlights = (scene.envmapidx == -1) ? scene.numemissives : (scene.numemissives + 1);
		int lightidx = RandUint(&rng) % numlights;
        
        // Sample light
        float3 le = Light_Sample(lightidx, &scene, &diffgeo, TEXTURE_ARGS, &rng, &lightwo, &lightpdf);
        lightbxdfpdf = Bxdf_GetPdf(&diffgeo, wi, normalize(lightwo), TEXTURE_ARGS);
        float lightweight = PowerHeuristic(1, lightpdf, 1, lightbxdfpdf);
        
        // Sample BxDF
		float3 bxdf = Bxdf_Sample(&diffgeo, wi, TEXTURE_ARGS, &rng, &bxdfwo, &bxdfpdf);
        bxdflightpdf = Light_GetPdf(lightidx, &scene, &diffgeo, bxdfwo, TEXTURE_ARGS);
        float bxdfweight = PowerHeuristic(1, bxdfpdf, 1, bxdflightpdf);
        
        float r = RandFloat(&rng);
        
        float3 radiance = 0.f;
		float3 wo;
        
		float prob = lightweight / (lightweight + bxdfweight);
		if ((r < prob) && lightpdf > 0.0f && length(le) > 0.f && !Bxdf_IsSingular(&diffgeo))
        {
            wo = lightwo;
			radiance = (le * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * throughput[pixelidx] * fabs(dot(diffgeo.n, normalize(wo))) / lightpdf);
        }
		else if (bxdfpdf > 0.0f)
        {
            wo = bxdfwo;
            le = Light_GetLe(lightidx, &scene, &diffgeo, &wo, TEXTURE_ARGS);
			radiance = le * bxdf * throughput[pixelidx] * fabs(dot(diffgeo.n, normalize(wo))) / bxdfpdf;
        }
        
        // Generate ray
        shadowrays[globalid].d.xyz = wo;
        shadowrays[globalid].o.xyz = diffgeo.p + 0.001f * normalize(wo);
		shadowrays[globalid].o.w = 0.99f;
        shadowrays[globalid].d.w = rays[hitidx].d.w;
		shadowrays[globalid].extra.x = Bxdf_IsSingular(&diffgeo) ? 0xFFFFFFFF : 0x0000FFFF;
		shadowrays[globalid].extra.y = 0xFFFFFFFF;
        // Perform some radiance clamping
		lightsamples[globalid] = clamp(radiance, 0.f, 10.f);

        
        // Generate indirect ray and update throughput
        bxdf = Bxdf_Sample(&diffgeo, wi, TEXTURE_ARGS, &rng, &bxdfwo, &bxdfpdf);
        
		if (bxdfpdf > 0.001f && fabs(dot(diffgeo.n, bxdfwo)) > 0.001f)
		{
			throughput[pixelidx] *= (bxdf * fabs(dot(diffgeo.n, bxdfwo)) / bxdfpdf);

			indirectrays[globalid].d.xyz = normalize(bxdfwo);
			indirectrays[globalid].o.xyz = diffgeo.p + 0.001f * normalize(bxdfwo);
			indirectrays[globalid].o.w = 100000000.f;
			indirectrays[globalid].d.w = rays[hitidx].d.w;
			indirectrays[globalid].extra.x = 0xFFFFFFFF;
			indirectrays[globalid].extra.y = 0xFFFFFFFF;
		}
		else
		{
			throughput[pixelidx] = 0;
			indirectrays[globalid].extra.y = 0;
		}
    }
}



///< Sample environemnt light and produce light samples and shadow rays into separate buffer
__kernel void SampleOcclusion(
                              // Ray batch
                              __global ray const* rays,
                              // Intersection data
                              __global Intersection const* isects,
                              // Hit indices
                              __global int const* hitindices,
                              // Pixel indices
                              __global int const* pixelindices,
                              // Number of rays
                              __global int* numhits,
                              // Vertices
                              __global float3 const* vertices,
                              // Normals
                              __global float3 const* normals,
                              // UVs
                              __global float2 const* uvs,
                              // Indices
                              __global int const* indices,
                              // Shapes
                              __global Shape const* shapes,
                              // Material IDs
                              __global int const* materialids,
                              // Materials
                              __global Material const* materials,
                              // Textures
                              __global Texture const* textures,
                              // Texture data
                              __global char const* texturedata,
                              // Environment texture index
                              int envmapidx,
                              // Envmap multiplier
                              float envmapmul,
                              // Emissives
                              __global Emissive const* emissives,
                              // Number of emissive objects
                              int numemissives,
                              // RNG seed
                              int rngseed,
                              // Shadow rays
                              __global ray* shadowrays,
                              // Light samples
                              __global float3* lightsamples,
                              // Path throughput
                              __global float3* throughput,
                              // Indirect rays
                              __global ray* indirectrays
                              )
{
    int globalid = get_global_id(0);
    
    if (globalid < *numhits)
    {
        // Fetch index
        int hitidx = hitindices[globalid];
        int pixelidx = pixelindices[globalid];
        
        // Fetch incoming ray
        float3 wi = -normalize(rays[hitidx].d.xyz);
        float time = rays[hitidx].d.w;
        
        // Determine shape and polygon
        int shapeid = isects[hitidx].shapeid - 1;
        int primid = isects[hitidx].primid;
        float2 uv = isects[hitidx].uvwt.xy;
        
        // Extract shape data
        Shape shape = shapes[shapeid];
        float3 linearvelocity = shape.linearvelocity;
        float4 angularvelocity = shape.angularvelocity;
        
        // Fetch indices starting from startidx and offset by primid
        int i0 = indices[shape.startidx + 3 * primid];
        int i1 = indices[shape.startidx + 3 * primid + 1];
        int i2 = indices[shape.startidx + 3 * primid + 2];
        
        // Fetch normals
        float3 n0 = normals[shape.startvtx + i0];
        float3 n1 = normals[shape.startvtx + i1];
        float3 n2 = normals[shape.startvtx + i2];
        
        // Fetch positions
        float3 v0 = vertices[shape.startvtx + i0];
        float3 v1 = vertices[shape.startvtx + i1];
        float3 v2 = vertices[shape.startvtx + i2];
        
        // Fetch UVs
        float2 uv0 = uvs[shape.startvtx + i0];
        float2 uv1 = uvs[shape.startvtx + i1];
        float2 uv2 = uvs[shape.startvtx + i2];
        
        // Calculate barycentric position and normal
        float3 n = normalize((1.f - uv.x - uv.y) * n0 + uv.x * n1 + uv.y * n2);
        float3 v = (1.f - uv.x - uv.y) * v0 + uv.x * v1 + uv.y * v2;
        //float2 t = (1.f - uv.x - uv.y) * uv0 + uv.x * uv1 + uv.y * uv2;
        
        // Bail out if opposite facing normal
        if (dot(wi, n) < 0.f)
        {
            //throughput[pixelidx] = 0;
            //return;
        }
        
        n = transform_vector(n, shape.m0, shape.m1, shape.m2, shape.m3);
        
        if (dot(wi, n) < 0.f)
        {
            n = -n;
        }
        
        // Prepare RNG for light sampling
        Rng rng;
        InitRng(rngseed + globalid * 157, &rng);
        
        // Generate AO ray
        float3 wo = SampleHemisphere(&rng, n, 1.f);
        shadowrays[globalid].d.xyz = wo;
        shadowrays[globalid].o.xyz = v + 0.001f * wo;
        shadowrays[globalid].o.w = envmapmul;
        shadowrays[globalid].d.w = rays[hitidx].d.w;
		shadowrays[globalid].extra.x = 0xFFFFFFFF;
		shadowrays[globalid].extra.y = 0xFFFFFFFF;
    }
}

///< Illuminate missing rays
__kernel void ShadeMiss(
                        // Ray batch
                        __global ray const* rays,
                        // Intersection data
                        __global Intersection const* isects,
                        // Pixel indices
                        __global int const* pixelindices,
                        // Number of rays
                        __global int* numrays,
                        // Textures
                        TEXTURE_ARG_LIST,
                        // Environment texture index
                        int envmapidx,
                        // Througput
                        __global float3 const* througput,
						// Inc sample count
						int inc,
                        // Output values
                        __global float4* output
                        )
{
    int globalid = get_global_id(0);
    
    if (globalid < *numrays)
    {
        int pixelidx = pixelindices[globalid];

        // In case of a miss
        if (isects[globalid].shapeid < 0)
        {
            // Multiply by througput
            output[pixelidx].xyz += Texture_SampleEnvMap(rays[globalid].d.xyz, TEXTURE_ARGS_IDX(envmapidx)) * througput[pixelidx];
        }
        
		if (inc)
			output[pixelidx].w += 1.f;
    }
}

///< Handle light samples and visibility info and add contribution to final buffer
__kernel void GatherLightSamples(
                                 // Pixel indices
                                 __global int const* pixelindices,
                                 // Number of rays
                                 __global int* numrays,
                                 // Shadow rays hits
                                 __global int const* shadowhits,
                                 // Light samples
                                 __global float3 const* lightsamples,
                                 // Througput
                                 __global float3 const* throughput,
                                 // Radiance sample buffer
                                 __global float4* output
                                 )
{
    int globalid = get_global_id(0);
    
    if (globalid < *numrays)
    {
        // Prepare accumulator variable
        float3 radiance = make_float3(0.f, 0.f, 0.f);
        
        // Get pixel id for this sample set
        int pixelidx = pixelindices[globalid];
        
        // Start collecting samples
        {
            // If shadow ray didn't hit anything and reached skydome
            if (shadowhits[globalid] == -1)
            {
                // Add its contribution to radiance accumulator
                radiance += lightsamples[globalid];
            }
        }
        
        // Divide by number of light samples (samples already have built-in throughput)
        output[pixelidx].xyz += radiance;
    }
}


///< Handle light samples and visibility info and add contribution to final buffer
__kernel void GatherOcclusion(
                              // Pixel indices
                              __global int const* pixelindices,
                              // Number of rays
                              __global int const* numrays,
                              // Shadow rays hits
                              __global int const* shadowhits,
                              // Light samples
                              __global float3 const* lightsamples,
                              // Througput
                              __global float3 const* throughput,
                              // Radiance sample buffer
                              __global float4* output
                              )
{
    int globalid = get_global_id(0);
    
    if (globalid < *numrays)
    {
        // Get pixel id for this sample set
        int pixelidx = pixelindices[globalid];
        
        float visibility = (shadowhits[globalid] == 1) ? 0.f : 1.f;
        
        output[pixelidx].xyz += visibility;
        output[pixelidx].w += 1.f;
    }
}

///< Restore pixel indices after compaction
__kernel void RestorePixelIndices(
                                  // Compacted indices
                                  __global int const* compacted_indices,
                                  // Number of compacted indices
                                  __global int* numitems,
                                  // Previous pixel indices
                                  __global int const* previndices,
                                  // New pixel indices
                                  __global int* newindices
                                  )
{
    int globalid = get_global_id(0);
    
    // Handle only working subset
    if (globalid < *numitems)
    {
        newindices[globalid] = previndices[compacted_indices[globalid]];
    }
}

///< Restore pixel indices after compaction
__kernel void ConvertToPredicate(
                                 // Intersections
                                 __global Intersection const* isects,
                                 // Number of compacted indices
                                 __global int* numitems,
                                 // Previous pixel indices
                                 __global int* predicate
                                 )
{
    int globalid = get_global_id(0);
    
    // Handle only working subset
    if (globalid < *numitems)
    {
        predicate[globalid] = isects[globalid].shapeid >= 0 ? 1 : 0;
    }
}

// Copy data to interop texture if supported
__kernel void AccumulateData(
	__global float4 const* srcdata,
	int numelems,
	__global float4* dstdata
	)
{
	int gid = get_global_id(0);

	if (gid < numelems)
	{
		float4 v = srcdata[gid];
		dstdata[gid] += v;
	}
}

// Copy data to interop texture if supported
__kernel void ApplyGammaAndCopyData(
                                    __global float4 const* data,
                                    int imgwidth,
                                    int imgheight,
                                    float gamma,
                                    write_only image2d_t img
                                    )
{
    int gid = get_global_id(0);
    
    int gidx = gid % imgwidth;
    int gidy = gid / imgwidth;
    
    if (gidx < imgwidth && gidy < imgheight)
    {
		float4 v = data[gid];
		float4 val = clamp(native_powr(v / v.w, 1.f / gamma), 0.f, 1.f);
        write_imagef(img, make_int2(gidx, gidy), val);
    }
}
