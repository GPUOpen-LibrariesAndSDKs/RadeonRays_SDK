#include <../App/CL/utils.cl>
#include <../App/CL/random.cl>
#include <../App/CL/payload.cl>
#include <../App/CL/texture.cl>
#include <../App/CL/sampling.cl>
#include <../App/CL/normalmap.cl>
#include <../App/CL/bxdf.cl>
#include <../App/CL/light.cl>

#define NO_PATH_DATA
#include <../App/CL/camera.cl>
#include <../App/CL/scene.cl>
#include <../App/CL/material.cl>
#include <../App/CL/volumetrics.cl>
#include <../App/CL/path.cl>

#define CRAZY_LOW_THROUGHPUT 0.0f
#define CRAZY_HIGH_RADIANCE 10.f
#define CRAZY_HIGH_DISTANCE 10000.f
#define CRAZY_LOW_DISTANCE 0.001f
#define REASONABLE_RADIANCE(x) (clamp((x), 0.f, CRAZY_HIGH_RADIANCE))
#define NON_BLACK(x) (length(x) > 0.f)


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
    // RNG seed
    int rngseed,
    // Radius
    float radius,
    // Sampler states
    __global SobolSampler* samplers,
    // Sobol matrices
    __global uint const* sobolmat,
    // Shadow rays
    __global ray* shadowrays
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
        0,
        0,
        0,
        0,
        0,
        0
    };

    // Only applied to active rays after compaction
    if (globalid < *numhits)
    {
        // Fetch index
        int hitidx = hitindices[globalid];
        int pixelidx = pixelindices[globalid];
        Intersection isect = isects[hitidx];

        // Fetch incoming ray direction
        float3 wi = -normalize(rays[hitidx].d.xyz);
#ifdef SOBOL
        // Sample light
        __global SobolSampler* sampler = samplers + pixelidx;

        float2 sample0;
        sample0.x = SobolSampler_Sample1D(sampler->seq, GetSampleDim(0, kBrdf), sampler->s0, sobolmat);
        sample0.y = SobolSampler_Sample1D(sampler->seq, GetSampleDim(0, kLight), sampler->s0, sobolmat);

#else
        // Prepare RNG
        Rng rng;
        InitRng(rngseed + (globalid << 2) * 157 + 13, &rng);
        float2 sample0 = UniformSampler_Sample2D(&rng);
#endif

        // Fill surface data
        DifferentialGeometry diffgeo;
        FillDifferentialGeometry(&scene, &isect, &diffgeo);

        // Check if we are hitting from the inside
        float ndotwi = dot(diffgeo.ng, wi);
        if (ndotwi < 0.f)
        {
            // Reverse normal and tangents in this case
            // but not for BTDFs, since BTDFs rely
            // on normal direction in order to arrange
            // indices of refraction
            diffgeo.n = -diffgeo.n;
            diffgeo.dpdu = -diffgeo.dpdu;
            diffgeo.dpdv = -diffgeo.dpdv;
        }


        float3 wo = radius * Sample_MapToHemisphere(sample0, diffgeo.n, 1.f);
        // Generate shadow ray
        float shadow_ray_length = (1.f - 2.f * CRAZY_LOW_DISTANCE) * length(wo);
        float3 shadow_ray_dir = normalize(wo);
        float3 shadow_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * diffgeo.n;
        int shadow_ray_mask = 0xFFFFFFFF;

        Ray_Init(shadowrays + globalid, shadow_ray_o, shadow_ray_dir, shadow_ray_length, 0.f, shadow_ray_mask);
    }
}


///< Handle light samples and visibility info and add contribution to final buffer
__kernel void GatherOcclusion(
    // Pixel indices
    __global int const* pixelindices,
    // Number of rays
    __global int* numrays,
    // Shadow rays hits
    __global int const* shadowhits,
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
        output[pixelidx].w += 1;
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
    // Predicate
    __global int* predicate
)
{
    int globalid = get_global_id(0);

    // Handle only working subset
    if (globalid < *numitems)
    {
        int pixelidx = pixelindices[globalid];
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