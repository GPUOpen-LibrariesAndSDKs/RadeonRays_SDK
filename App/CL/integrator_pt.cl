
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
#include <../App/CL/volumetrics.cl>
#include <../App/CL/path.cl>

#define CRAZY_LOW_THROUGHPUT 0.0f
#define CRAZY_HIGH_RADIANCE 3.f
#define CRAZY_HIGH_DISTANCE 1000000.f
#define CRAZY_LOW_DISTANCE 0.001f
#define REASONABLE_RADIANCE(x) (clamp((x), 0.f, CRAZY_HIGH_RADIANCE))
#define NON_BLACK(x) (length(x) > 0.f)

// This kernel only handles scattered paths.
// It applies direct illumination and generates
// path continuation if multiscattering is enabled.
__kernel void ShadeVolume(
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
    __global Light const* lights,
    // Number of emissive objects
    int num_lights,
    // RNG seed
    int rngseed,
    // Sampler state
    __global SobolSampler* samplers,
    // Sobol matrices
    __global uint const* sobolmat,
    // Current bounce
    int bounce,
    // Volume data
    __global Volume const* volumes,
    // Shadow rays
    __global ray* shadowrays,
    // Light samples
    __global float3* lightsamples,
    // Path throughput
    __global Path* paths,
    // Indirect rays (next path segment)
    __global ray* indirectrays,
    // Radiance
    __global float3* output
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
        lights,
        envmapidx,
        envmapmul,
        num_lights
    };

    if (globalid < *numhits)
    {
        // Fetch index
        int hitidx = hitindices[globalid];
        int pixelidx = pixelindices[globalid];
        Intersection isect = isects[hitidx];

        __global Path* path = paths + pixelidx;

        // Only apply to scattered paths
        if (!Path_IsScattered(path))
        {
            return;
        }

        // Fetch incoming ray
        float3 o = rays[hitidx].o.xyz;
        float3 wi = rays[hitidx].d.xyz;

#ifdef SOBOL
        __global SobolSampler* sampler = samplers + pixelidx;
        float sample0 = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kVolumeLight), sampler->s0, sobolmat);
        float2 sample1;
        sample1.x = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kVolumeLightU), sampler->s0, sobolmat);
        sample1.y = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kVolumeLightV), sampler->s0, sobolmat);
#ifdef MULTISCATTER
        float2 sample2;
        sample2.x = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kVolumeIndirectU), sampler->s0, sobolmat);
        sample2.y = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kVolumeIndirectV), sampler->s0, sobolmat);
#endif
#else
        // Prepare RNG for sampling
        Rng rng;
        InitRng(rngseed + (globalid << 2) * 157 + 13, &rng);
        float sample0 = UniformSampler_Sample2D(&rng).x;
        float2 sample1 = UniformSampler_Sample2D(&rng);
#ifdef MULTISCATTER
        float2 sample2 = UniformSampler_Sample2D(&rng);
#endif
#endif
        // Here we know that volidx != -1 since this is a precondition
        // for scattering event
        int volidx = Path_GetVolumeIdx(path);

        // Sample light source
        float pdf = 0.f;
        float selection_pdf = 0.f;
        float3 wo;

        int light_idx = Scene_SampleLight(&scene, sample0, &selection_pdf);

        // Here we need fake differential geometry for light sampling procedure
        DifferentialGeometry dg;
        // put scattering position in there (it is along the current ray at isect.distance
        // since EvaluateVolume has put it there
        dg.p = o + wi * Intersection_GetDistance(isects + hitidx);
        // Get light sample intencity
        float3 le = Light_Sample(light_idx, &scene, &dg, TEXTURE_ARGS, sample1, &wo, &pdf);

        // Generate shadow ray
        float shadow_ray_length = 0.999f * length(wo);
        Ray_Init(shadowrays + globalid, dg.p, normalize(wo), shadow_ray_length, 0.f, 0xFFFFFFFF);

        // Evaluate volume transmittion along the shadow ray (it is incorrect if the light source is outside of the
        // current volume, but in this case it will be discarded anyway since the intersection at the outer bound
        // of a current volume), so the result is fully correct.
        float3 tr = Volume_Transmittance(&volumes[volidx], &shadowrays[globalid], shadow_ray_length);

        // Volume emission is applied only if the light source is in the current volume(this is incorrect since the light source might be
        // outside of a volume and we have to compute fraction of ray in this case, but need to figure out how)
        float3 r = Volume_Emission(&volumes[volidx], &shadowrays[globalid], shadow_ray_length);

        // This is the estimate coming from a light source
        // TODO: remove hardcoded phase func and sigma
        r += tr * le * volumes[volidx].sigma_s * PhaseFunction_Uniform(wi, normalize(wo)) / pdf / selection_pdf;

        // Only if we have some radiance compute the visibility ray
        if (NON_BLACK(tr) && NON_BLACK(r) && pdf > 0.f)
        {
            // Put lightsample result
            lightsamples[globalid] = REASONABLE_RADIANCE(r * Path_GetThroughput(path));
        }
        else
        {
            // Nothing to compute
            lightsamples[globalid] = 0.f;
            // Otherwise make it incative to save intersector cycles (hopefully)
            Ray_SetInactive(shadowrays + globalid);
        }

#ifdef MULTISCATTER
        // This is highly brute-force
        // TODO: investigate importance sampling techniques here
        wo = Sample_MapToSphere(sample2);
        pdf = 1.f / (4.f * PI);

        // Generate new path segment
        Ray_Init(indirectrays + globalid, dg.p, normalize(wo), CRAZY_HIGH_DISTANCE, 0.f, 0xFFFFFFFF);

        // Update path throughput multiplying by phase function.
        Path_MulThroughput(path, volumes[volidx].sigma_s * PhaseFunction_Uniform(wi, normalize(wo)) / pdf);
#else
        // Single-scattering mode only,
        // kill the path and compact away on next iteration
        Path_Kill(path);
        Ray_SetInactive(indirectrays + globalid);
#endif
    }
}

// Handle ray-surface interaction possibly generating path continuation.
// This is only applied to non-scattered paths.
__kernel void ShadeSurface(
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
    __global Light const* lights,
    // Number of emissive objects
    int num_lights,
    // RNG seed
    int rngseed,
    // Sampler states
    __global SobolSampler* samplers,
    // Sobol matrices
    __global uint const* sobolmat,
    // Current bounce
    int bounce,
    // Volume data
    __global Volume const* volumes,
    // Shadow rays
    __global ray* shadowrays,
    // Light samples
    __global float3* lightsamples,
    // Path throughput
    __global Path* paths,
    // Indirect rays
    __global ray* indirectrays,
    // Radiance
    __global float3* output
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
        lights,
        envmapidx,
        envmapmul,
        num_lights
    };

    // Only applied to active rays after compaction
    if (globalid < *numhits)
    {
        // Fetch index
        int hitidx = hitindices[globalid];
        int pixelidx = pixelindices[globalid];
        Intersection isect = isects[hitidx];

        __global Path* path = paths + pixelidx;

        // Early exit for scattered paths
        if (Path_IsScattered(path))
        {
            return;
        }

        // Fetch incoming ray direction
        float3 wi = -normalize(rays[hitidx].d.xyz);
#ifdef SOBOL
        // Sample light
        __global SobolSampler* sampler = samplers + pixelidx;

        float2 sample0;
        sample0.x = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kBrdf), sampler->s0, sobolmat);
        sample0.y = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kLight), sampler->s0, sobolmat);

        float2 sample1;
        sample1.x = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kLightU), sampler->s0, sobolmat);
        sample1.y = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kLightV), sampler->s0, sobolmat);

        float2 sample2;
        sample2.x = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kBrdfU), sampler->s0, sobolmat);
        sample2.y = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kBrdfV), sampler->s0, sobolmat);

        float2 sample3;
        sample3.x = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kIndirectU), sampler->s0, sobolmat);
        sample3.y = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kIndirectV), sampler->s0, sobolmat);

        float sample4 = SobolSampler_Sample1D(sampler->seq, GetSampleDim(bounce, kRR), sampler->s0, sobolmat);
#else
        // Prepare RNG
        Rng rng;
        InitRng(rngseed + (globalid << 2) * 157 + 13, &rng);
        float2 sample0 = UniformSampler_Sample2D(&rng);
        float2 sample1 = UniformSampler_Sample2D(&rng);
        float2 sample2 = UniformSampler_Sample2D(&rng);
        float2 sample3 = UniformSampler_Sample2D(&rng);
        float  sample4 = UniformSampler_Sample2D(&rng).x;
#endif

        // Fill surface data
        DifferentialGeometry diffgeo;
        FillDifferentialGeometry(&scene, &isect, &diffgeo);

        // Check if we are hitting from the inside
        float ndotwi = dot(diffgeo.n, wi);
        int twosided = diffgeo.mat.twosided;
        if (twosided && ndotwi < 0.f)
        {
            // Reverse normal and tangents in this case
            // but not for BTDFs, since BTDFs rely
            // on normal direction in order to arrange
            // indices of refraction
            diffgeo.n = -diffgeo.n;
            diffgeo.dpdu = -diffgeo.dpdu;
            diffgeo.dpdv = -diffgeo.dpdv;
        }

        // Select BxDF
        Material_Select(
            &scene, wi, TEXTURE_ARGS,
#ifdef SOBOL
            sampler, sobolmat, bounce,
#else
            &rng,
#endif
            &diffgeo
        );

        ndotwi = dot(diffgeo.n, wi);

        // Terminate if emissive
        if (Bxdf_IsEmissive(&diffgeo))
        {
            if (ndotwi > 0.f)
            {
                float weight = 1.f;

                if (bounce > 0 && !Path_IsSpecular(path))
                {
                    float2 extra = Ray_GetExtra(&rays[hitidx]);
                    float ld = isect.uvwt.w;
                    float denom = extra.y * diffgeo.area;
                    // TODO: num_lights should be num_emissies instead, presence of analytical lights breaks this code
                    float bxdflightpdf = denom > 0.f ? (ld * ld / denom / num_lights) : 0.f;
                    weight = BalanceHeuristic(1, extra.x, 1, bxdflightpdf);
                }
                
                {
                    // In this case we hit after an application of MIS process at previous step.
                    // That means BRDF weight has been already applied.
                    output[pixelidx] += Path_GetThroughput(path) * Emissive_GetLe(&diffgeo, TEXTURE_ARGS) * weight;
                }
            }

            Path_Kill(path);
            Ray_SetInactive(shadowrays + globalid);
            Ray_SetInactive(indirectrays + globalid);

            lightsamples[globalid] = 0.f;
            return;
        }


        float s = Bxdf_IsBtdf(&diffgeo) ? (-sign(ndotwi)) : 1.f;
        if (!twosided && ndotwi < 0.f && !Bxdf_IsBtdf(&diffgeo))
        {
            // Reverse normal and tangents in this case
            // but not for BTDFs, since BTDFs rely
            // on normal direction in order to arrange
            // indices of refraction
            diffgeo.n = -diffgeo.n;
            diffgeo.dpdu = -diffgeo.dpdu;
            diffgeo.dpdv = -diffgeo.dpdv;
        }

        // TODO: this is test code, need to
        // maintain proper volume stack here
        //if (Bxdf_IsBtdf(&diffgeo))
        //{
            // If we entering set the volume
            //path->volume = ndotwi > 0.f ? 0 : -1;
        //}

        // Check if we need to apply normal map
        //ApplyNormalMap(&diffgeo, TEXTURE_ARGS);
        ApplyBumpMap(&diffgeo, TEXTURE_ARGS);
        float lightpdf = 0.f;
        float bxdflightpdf = 0.f;
        float bxdfpdf = 0.f;
        float lightbxdfpdf = 0.f;
        float selection_pdf = 0.f;
        float3 radiance = 0.f;
        float3 lightwo;
        float3 bxdfwo;
        float3 wo;
        float bxdfweight = 1.f;
        float lightweight = 1.f;

        int light_idx = num_lights > 0 ? Scene_SampleLight(&scene, sample0.y, &selection_pdf) : -1;

        float3 throughput = Path_GetThroughput(path);

        // Sample bxdf
        float3 bxdf = Bxdf_Sample(&diffgeo, wi, TEXTURE_ARGS, sample2, &bxdfwo, &bxdfpdf);

        // If we have light to sample we can hopefully do mis
        if (light_idx > -1)
        {
            // Sample light
            float3 le = Light_Sample(light_idx, &scene, &diffgeo, TEXTURE_ARGS, sample1, &lightwo, &lightpdf);
            lightbxdfpdf = Bxdf_GetPdf(&diffgeo, wi, normalize(lightwo), TEXTURE_ARGS);
            lightweight = Light_IsSingular(&scene.lights[light_idx]) ? 1.f : BalanceHeuristic(1, lightpdf, 1, lightbxdfpdf);


            // Apply MIS to account for both
            if (NON_BLACK(le) && lightpdf > 0.0f && !Bxdf_IsSingular(&diffgeo))
            {
                wo = lightwo;
                float ndotwo = fabs(dot(diffgeo.n, normalize(wo)));
                radiance = le * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * throughput *
                    ndotwo * lightweight / lightpdf / selection_pdf;
            }
        }

        // If we have some light here generate a shadow ray
        if (NON_BLACK(radiance))
        {
            // Generate shadow ray
            float shadow_ray_length = (1.f - 2.f * CRAZY_LOW_DISTANCE) * length(wo);
            float3 shadow_ray_dir = normalize(wo);
            float3 shadow_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * s * diffgeo.n;
            int shadow_ray_mask = Bxdf_IsSingular(&diffgeo) ? 0xFFFFFFFF : 0x0000FFFF;

            Ray_Init(shadowrays + globalid, shadow_ray_o, shadow_ray_dir, shadow_ray_length, 0.f, shadow_ray_mask);

            // Apply the volume to shadow ray if needed
            int volidx = Path_GetVolumeIdx(path);
            if (volidx != -1)
            {
                radiance *= Volume_Transmittance(&volumes[volidx], &shadowrays[globalid], shadow_ray_length);
                radiance += Volume_Emission(&volumes[volidx], &shadowrays[globalid], shadow_ray_length) * throughput;
            }

            // And write the light sample
            lightsamples[globalid] = REASONABLE_RADIANCE(radiance);
        }
        else
        {
            // Otherwise save some intersector cycles
            Ray_SetInactive(shadowrays + globalid);
            lightsamples[globalid] = 0;
        }

        // Apply Russian roulette
        float q = max(min(0.5f,
            // Luminance
            0.2126f * throughput.x + 0.7152f * throughput.y + 0.0722f * throughput.z), 0.01f);
        // Only if it is 3+ bounce
        bool rr_apply = bounce > 3;
        bool rr_stop = sample4 > q && rr_apply;

        if (rr_apply)
        {
            Path_MulThroughput(path, 1.f / q);
        }

        if (Bxdf_IsSingular(&diffgeo))
        {
            Path_SetSpecularFlag(path);
        }

        bxdfwo = normalize(bxdfwo);
        float3 t = bxdf * fabs(dot(diffgeo.n, bxdfwo));

        // Only continue if we have non-zero throughput & pdf
        if (NON_BLACK(t) && bxdfpdf > 0.f && !rr_stop)
        {
            // Update the throughput
            Path_MulThroughput(path, t / bxdfpdf);

            // Generate ray
            float3 indirect_ray_dir = bxdfwo;
            float3 indirect_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * s * diffgeo.n;

            Ray_Init(indirectrays + globalid, indirect_ray_o, indirect_ray_dir, CRAZY_HIGH_DISTANCE, 0.f, 0xFFFFFFFF);
            Ray_SetExtra(indirectrays + globalid, make_float2(bxdfpdf, fabs(dot(diffgeo.n, bxdfwo))));
        }
        else
        {
            // Otherwise kill the path
            Path_Kill(path);
            Ray_SetInactive(indirectrays + globalid);
        }
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
    int numrays,
    // Textures
    TEXTURE_ARG_LIST,
    // Environment texture index
    int envmapidx,
    //
    __global Path const* paths,
    __global Volume const* volumes,
    // Output values
    __global float4* output
)
{
    int globalid = get_global_id(0);

    if (globalid < numrays)
    {
        int pixelidx = pixelindices[globalid];

        // In case of a miss
        if (isects[globalid].shapeid < 0)
        {
            // Multiply by throughput
            int volidx = paths[pixelidx].volume;

            if (volidx == -1)
                output[pixelidx].xyz += Texture_SampleEnvMap(rays[globalid].d.xyz, TEXTURE_ARGS_IDX(envmapidx));
            else
            {
                output[pixelidx].xyz += Texture_SampleEnvMap(rays[globalid].d.xyz, TEXTURE_ARGS_IDX(envmapidx)) *
                    Volume_Transmittance(&volumes[volidx], &rays[globalid], rays[globalid].o.w);

                output[pixelidx].xyz += Volume_Emission(&volumes[volidx], &rays[globalid], rays[globalid].o.w);
            }
        }

        if (isnan(output[pixelidx].x) || isnan(output[pixelidx].y) || isnan(output[pixelidx].z))
        {
            output[pixelidx] = make_float4(100.f, 0.f, 0.f, 1.f);
        }

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
    // throughput
    __global Path const* paths,
    // Radiance sample buffer
    __global float4* output
)
{
    int globalid = get_global_id(0);

    if (globalid < *numrays)
    {
        // Get pixel id for this sample set
        int pixelidx = pixelindices[globalid];


        // Prepare accumulator variable
        float3 radiance = make_float3(0.f, 0.f, 0.f);

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
    // throughput
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
__kernel void FilterPathStream(
    // Intersections
    __global Intersection const* isects,
    // Number of compacted indices
    __global int const* numitems,
    // Pixel indices
    __global int const* pixelindices,
    // Paths
    __global Path* paths,
    // Predicate
    __global int* predicate
)
{
    int globalid = get_global_id(0);

    // Handle only working subset
    if (globalid < *numitems)
    {
        int pixelidx = pixelindices[globalid];

        __global Path* path = paths + pixelidx;

        if (Path_IsAlive(path))
        {
            bool kill = (length(Path_GetThroughput(path)) < CRAZY_LOW_THROUGHPUT);

            if (!kill)
            {
                predicate[globalid] = isects[globalid].shapeid >= 0 ? 1 : 0;
            }
            else
            {
                Path_Kill(path);
                predicate[globalid] = 0;
            }
        }
        else
        {
            predicate[globalid] = 0;
        }
    }
}

///< Illuminate missing rays
__kernel void ShadeBackground(
    // Ray batch
    __global ray const* rays,
    // Intersection data
    __global Intersection const* isects,
    // Pixel indices
    __global int const* pixelindices,
    // Number of rays
    __global int const* numrays,
    // Textures
    TEXTURE_ARG_LIST,
    // Environment texture index
    int envmapidx,
    float envmapmul,
    //
    int num_lights,
    __global Path const* paths,
    __global Volume const* volumes,
    // Output values
    __global float4* output
)
{
    int globalid = get_global_id(0);

    if (globalid < *numrays)
    {
        int pixelidx = pixelindices[globalid];

        __global Path const* path = paths + pixelidx;

        // In case of a miss
        if (isects[globalid].shapeid < 0 && Path_IsAlive(path))
        {
            float3 t = Path_GetThroughput(path);
            output[pixelidx].xyz += REASONABLE_RADIANCE(envmapmul * Texture_SampleEnvMap(rays[globalid].d.xyz, TEXTURE_ARGS_IDX(envmapidx)) * t);
        }
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
