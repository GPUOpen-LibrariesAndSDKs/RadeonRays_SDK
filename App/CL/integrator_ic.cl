
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
#include <../App/CL/common.cl>
#include <../App/CL/ray.cl>
#include <../App/CL/isect.cl>
#include <../App/CL/utils.cl>
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
#include <../App/CL/hlbvh.cl>
#include <../App/CL/radiance_probe.cl>


inline
void RadianceCache_GatherRadiance(
    __global HlbvhNode const* restrict bvh, 
    __global bbox const* restrict bounds, 
    __global RadianceProbeDesc const* restrict descs,
    __global RadianceProbeData const* restrict probes,
    float3 p,
    float3 n,
    RadianceProbeData* out_probe
    )
{
    int stack[STACK_SIZE];
    int* ptr = stack;
    *ptr++ = -1;
    int idx = 0;

    HlbvhNode node;
    bbox lbox;
    bbox rbox;

    float weight = 0;

    while (idx > -1)
    {
        node = bvh[idx];

        if (LEAFNODE(node))
        {
            int probe_idx = STARTIDX(node);
            RadianceProbe_AddContribution(descs + probe_idx, probes + probe_idx, p, n, out_probe, &weight);
        }
        else
        {
            lbox = bounds[node.left];
            rbox = bounds[node.right];

            bool lhit = Bbox_ContainsPoint(lbox, p);
            bool rhit = Bbox_ContainsPoint(rbox, p);

            if (lhit && rhit)
            {

                idx = node.left;
                *ptr++ = node.right;
                continue;
            }
            else if (lhit)
            {
                idx = node.left;
                continue;
            }
            else if (rhit)
            {
                idx = node.right;
                continue;
            }
        }

        idx = *--ptr;
    }

    if (weight > 0.f)
    {
        float invw = 1.f / weight;
        out_probe->r0 *= invw;
        out_probe->r1 *= invw;
        out_probe->r2 *= invw;

        out_probe->g0 *= invw;
        out_probe->g1 *= invw;
        out_probe->g2 *= invw;

        out_probe->b0 *= invw;
        out_probe->b1 *= invw;
        out_probe->b2 *= invw;
    }
}

#ifdef RADIANCE_PROBE_DIRECT
inline
void RadianceCache_GatherDirectRadiance(
    __global HlbvhNode const* restrict bvh,
    __global bbox const* restrict bounds,
    __global RadianceProbeDesc const* restrict descs,
    __global RadianceProbeData const* restrict probes,
    float3 p,
    float3 n,
    RadianceProbeData* out_probe
)
{
    int stack[STACK_SIZE];
    int* ptr = stack;
    *ptr++ = -1;
    int idx = 0;

    HlbvhNode node;
    bbox lbox;
    bbox rbox;

    float weight = 0;

    while (idx > -1)
    {
        node = bvh[idx];

        if (LEAFNODE(node))
        {
            int probe_idx = STARTIDX(node);
            RadianceProbe_AddDirectContribution(descs + probe_idx, probes + probe_idx, p, n, out_probe, &weight);
        }
        else
        {
            lbox = bounds[node.left];
            rbox = bounds[node.right];

            bool lhit = Bbox_ContainsPoint(lbox, p);
            bool rhit = Bbox_ContainsPoint(rbox, p);

            if (lhit && rhit)
            {

                idx = node.left;
                *ptr++ = node.right;
                continue;
            }
            else if (lhit)
            {
                idx = node.left;
                continue;
            }
            else if (rhit)
            {
                idx = node.right;
                continue;
            }
        }

        idx = *--ptr;
    }

    if (weight > 0.f)
    {
        float invw = 1.f / weight;
        out_probe->dr0 *= invw;
        out_probe->dr1 *= invw;
        out_probe->dr2 *= invw;

        out_probe->dg0 *= invw;
        out_probe->dg1 *= invw;
        out_probe->dg2 *= invw;

        out_probe->db0 *= invw;
        out_probe->db1 *= invw;
        out_probe->db2 *= invw;
    }
}
#endif

inline
float3 RadianceCache_GatherIrradiance(
    __global HlbvhNode const* restrict bvh,
    __global bbox const* restrict bounds,
    __global RadianceProbeDesc const* restrict descs,
    __global RadianceProbeData const* restrict probes,
    float3 p,
    float3 n)
{
    RadianceProbeData probe = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
    RadianceCache_GatherRadiance(bvh, bounds, descs, probes, p, n, &probe);

    float3 r0 = probe.r0;
    float3 r1 = probe.r1;
    float3 r2 = probe.r2;
    float3 b0 = probe.b0;
    float3 b1 = probe.b1;
    float3 b2 = probe.b2;
    float3 g0 = probe.g0;
    float3 g1 = probe.g1;
    float3 g2 = probe.g2;

    float3 sh0, sh1, sh2;

    SH_Get2ndOrderCoeffs(make_float3(0.f, 1.f, 0.f), &sh0, &sh1, &sh2);

    sh0.x *= 0.8862268925f;
    sh0.y *= 0.0233267546f;
    sh0.z *= 0.4954159260f;
    sh1.x = 0.0000000000f;
    sh1.y *= -0.1107783690f;
    sh1.z = 0.0000000000f;

    sh2.x *= 0.0499271341f;
    sh2.y = 0.0000000000f;
    sh2.z *= -0.0285469331f;

    float3 l;
    l.x = 4 * PI * (dot(r0, sh0) + dot(r1, sh1) + dot(r2, sh2));
    l.y = 4 * PI * (dot(g0, sh0) + dot(g1, sh1) + dot(g2, sh2));
    l.z = 4 * PI * (dot(b0, sh0) + dot(b1, sh1) + dot(b2, sh2));

    return l;
}
//
//inline
//float3 RadianceCache_GatherRadiance(
//    __global HlbvhNode const* restrict bvh,
//    __global bbox const* restrict bounds,
//    __global RadianceProbeDesc const* restrict descs,
//    __global RadianceProbeData const* restrict probes,
//    float3 p,
//    float3 n,
//    float3 wo)
//{
//    RadianceProbeData probe = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
//    RadianceCache_GatherRadiance(bvh, bounds, descs, probes, p, , &probe);
//
//    float3 r0 = probe.r0;
//    float3 r1 = probe.r1;
//    float3 r2 = probe.r2;
//    float3 b0 = probe.b0;
//    float3 b1 = probe.b1;
//    float3 b2 = probe.b2;
//    float3 g0 = probe.g0;
//    float3 g1 = probe.g1;
//    float3 g2 = probe.g2;
//
//    float3 sh0, sh1, sh2;
//
//    SH_Get2ndOrderCoeffs(make_float3(0.f, 1.f, 0.f), &sh0, &sh1, &sh2);
//
//    sh0.x *= 0.8862268925f;
//    sh0.y *= 0.0233267546f;
//    sh0.z *= 0.4954159260f;
//    sh1.x = 0.0000000000f;
//    sh1.y *= -0.1107783690f;
//    sh1.z = 0.0000000000f;
//
//    sh2.x *= 0.0499271341f;
//    sh2.y = 0.0000000000f;
//    sh2.z *= -0.0285469331f;
//
//    float3 l;
//    l.x = 4 * PI * (dot(r0, sh0) + dot(r1, sh1) + dot(r2, sh2));
//    l.y = 4 * PI * (dot(g0, sh0) + dot(g1, sh1) + dot(g2, sh2));
//    l.z = 4 * PI * (dot(b0, sh0) + dot(b1, sh1) + dot(b2, sh2));
//
//    return l;
//}

#ifdef RADIANCE_PROBE_DIRECT
inline
float3 RadianceCache_GatherDirectIrradiance(
    __global HlbvhNode const* restrict bvh,
    __global bbox const* restrict bounds,
    __global RadianceProbeDesc const* restrict descs,
    __global RadianceProbeData const* restrict probes,
    float3 p,
    float3 n)
{
    RadianceProbeData probe = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
    RadianceCache_GatherDirectRadiance(bvh, bounds, descs, probes, p, n, &probe);

    float3 r0 = probe.dr0;
    float3 r1 = probe.dr1;
    float3 r2 = probe.dr2;
    float3 b0 = probe.db0;
    float3 b1 = probe.db1;
    float3 b2 = probe.db2;
    float3 g0 = probe.dg0;
    float3 g1 = probe.dg1;
    float3 g2 = probe.dg2;

    float3 sh0, sh1, sh2;

    SH_Get2ndOrderCoeffs(make_float3(0.f, 1.f, 0.f), &sh0, &sh1, &sh2);

    sh0.x *= 0.8862268925f;
    sh0.y *= 0.0233267546f;
    sh0.z *= 0.4954159260f;
    sh1.x = 0.0000000000f;
    sh1.y *= -0.1107783690f;
    sh1.z = 0.0000000000f;

    sh2.x *= 0.0499271341f;
    sh2.y = 0.0000000000f;
    sh2.z *= -0.0285469331f;

    float3 l;
    l.x = 4 * PI * (dot(r0, sh0) + dot(r1, sh1) + dot(r2, sh2));
    l.y = 4 * PI * (dot(g0, sh0) + dot(g1, sh1) + dot(g2, sh2));
    l.z = 4 * PI * (dot(b0, sh0) + dot(b1, sh1) + dot(b2, sh2));

    return l;
}
#endif



// Handle ray-surface interaction possibly generating path continuation.
// This is only applied to non-scattered paths.
__kernel void VisualizeCache(
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
    // Cache
    __global HlbvhNode const* bvh,
    // Bouds
    __global bbox const* bounds,
    // Radiance probe data
    __global RadianceProbeDesc const* descs,
    // Radiance probe data
    __global RadianceProbeData const* probes,
    // Radiance
    __global float4* output
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
        materials
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

        // Fill surface data
        DifferentialGeometry diffgeo;
        DifferentialGeometry_Fill(&scene, &isect, &diffgeo);

        // Check if we are hitting from the inside

        float backfacing = dot(diffgeo.ng, wi) < 0.f;
        int twosided = diffgeo.mat.twosided;
        if (twosided && backfacing)
        {
            // Reverse normal and tangents in this case
            // but not for BTDFs, since BTDFs rely
            // on normal direction in order to arrange
            // indices of refraction
            diffgeo.n = -diffgeo.n;
            diffgeo.dpdu = -diffgeo.dpdu;
            diffgeo.dpdv = -diffgeo.dpdv;
        }

        float ndotwi = dot(diffgeo.n, wi);

        // Select BxDF
        Sampler sampler;
        Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);

        int idx = Hlbvh_GetClosestItemIndex(bvh, bounds, diffgeo.p);
        int num_samples = descs[idx].num_samples;

        RadianceProbeData probe = { 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f };
        RadianceCache_GatherRadiance(bvh, bounds, descs, probes, diffgeo.p, diffgeo.n, &probe);

        float3 r0 = probe.r0;
        float3 r1 = probe.r1;
        float3 r2 = probe.r2;
        float3 b0 = probe.b0;
        float3 b1 = probe.b1;
        float3 b2 = probe.b2;
        float3 g0 = probe.g0;
        float3 g1 = probe.g1;
        float3 g2 = probe.g2;

        float3 sh0, sh1, sh2;

        SH_Get2ndOrderCoeffs(make_float3(0.f, 1.f, 0.f), &sh0, &sh1, &sh2);

        sh0.x *= 0.8862268925f;
        sh0.y *= 0.0233267546f;
        sh0.z *= 0.4954159260f;
        sh1.x = 0.0000000000f;
        sh1.y *= -0.1107783690;
        sh1.z = 0.0000000000f;

        sh2.x *= 0.0499271341f;
        sh2.y = 0.0000000000f;
        sh2.z *= -0.0285469331f;

        float3 l;
        l.x = 4 * PI * (dot(r0, sh0) + dot(r1, sh1) + dot(r2, sh2));
        l.y = 4 * PI * (dot(g0, sh0) + dot(g1, sh1) + dot(g2, sh2));
        l.z = 4 * PI * (dot(b0, sh0) + dot(b1, sh1) + dot(b2, sh2));

        output[pixelidx].xyz = l;
        output[pixelidx].w = 1.f;
    }
}

// Handle ray-surface interaction possibly generating path continuation.
// This is only applied to non-scattered paths.
__kernel void ShadeSurfaceAndCache(
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
    uint rngseed,
    // Sampler states
    __global uint* random,
    // Sobol matrices
    __global uint const* sobolmat,
    // Current bounce
    int bounce,
    int bounce_cached,
    // Frame
    int frame,
    // Volume data
    __global Volume const* volumes,
    // Shadow rays
    __global ray* shadowrays,
    // Light samples
    __global float3* lightsamples,
    // Light samples
    __global float3* indirect_lightsamples,
    // Path throughput
    __global Path* paths,
    // Indirect rays
    __global ray* indirectrays,
    // Radiance
    __global float3* output,
    // Indirect
    __global float3* indirect,
    // Indirect throughput
    __global float3* indirect_throughput,
    // Indirect rays
    __global ray* indirect_rays,
    __global int* indirect_predicate
#ifdef RADIANCE_PROBE_DIRECT
    ,
    // Light samples
    __global float3* direct_light_samples
#endif
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

        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[pixelidx] * 0x1fe3434f;
        Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#elif SAMPLER == RANDOM
        uint scramble = pixelidx * rngseed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[pixelidx];
        uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#endif

        // Fill surface data
        DifferentialGeometry diffgeo;
        DifferentialGeometry_Fill(&scene, &isect, &diffgeo);

        // Check if we are hitting from the inside

        float backfacing = dot(diffgeo.ng, wi) < 0.f;
        int twosided = diffgeo.mat.twosided;
        if (twosided && backfacing)
        {
            // Reverse normal and tangents in this case
            // but not for BTDFs, since BTDFs rely
            // on normal direction in order to arrange
            // indices of refraction
            diffgeo.n = -diffgeo.n;
            diffgeo.dpdu = -diffgeo.dpdu;
            diffgeo.dpdv = -diffgeo.dpdv;
        }

        float ndotwi = dot(diffgeo.n, wi);

        // Select BxDF
        Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);

        // Terminate if emissive
        if (Bxdf_IsEmissive(&diffgeo))
        {
            if (!backfacing)
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

                    if (bounce > bounce_cached - 1)
                        indirect[pixelidx] += indirect_throughput[pixelidx] * Emissive_GetLe(&diffgeo, TEXTURE_ARGS) * weight;
                }
            }

            Path_Kill(path);
            Ray_SetInactive(shadowrays + globalid);
            Ray_SetInactive(indirectrays + globalid);

            lightsamples[globalid] = 0.f;
            return;
        }


        float s = Bxdf_IsBtdf(&diffgeo) ? (-sign(ndotwi)) : 1.f;
        if (!twosided && backfacing && !Bxdf_IsBtdf(&diffgeo))
        {
            //Reverse normal and tangents in this case
            //but not for BTDFs, since BTDFs rely
            //on normal direction in order to arrange
            //indices of refraction
            diffgeo.n = -diffgeo.n;
            diffgeo.dpdu = -diffgeo.dpdu;
            diffgeo.dpdv = -diffgeo.dpdv;
        }

        // TODO: this is test code, need to
        // maintain proper volume stack here
        //if (Bxdf_IsBtdf(&diffgeo))
        //{
        //    // If we entering set the volume
        //    path->volume = !backfacing ? 0 : -1;
        //}

        // Check if we need to apply normal map
        //ApplyNormalMap(&diffgeo, TEXTURE_ARGS);
        DifferentialGeometry_ApplyBumpMap(&diffgeo, TEXTURE_ARGS);
        DifferentialGeometry_CalculateTangentTransforms(&diffgeo);

        float lightpdf = 0.f;
        float bxdflightpdf = 0.f;
        float bxdfpdf = 0.f;
        float lightbxdfpdf = 0.f;
        float selection_pdf = 0.f;
        float3 radiance = 0.f;
        float3 indirect_radiance = 0.f;
        float3 lightwo;
        float3 bxdfwo;
        float3 wo;
        float bxdfweight = 1.f;
        float lightweight = 1.f;

        int light_idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf);

        float3 throughput = Path_GetThroughput(path);

        // Sample bxdf
        float3 bxdf = Bxdf_Sample(&diffgeo, wi, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &bxdfwo, &bxdfpdf);

        // If we have light to sample we can hopefully do mis
        if (light_idx > -1)
        {
            // Sample light
            float3 le = Light_Sample(light_idx, &scene, &diffgeo, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &lightwo, &lightpdf);
            lightbxdfpdf = Bxdf_GetPdf(&diffgeo, wi, normalize(lightwo), TEXTURE_ARGS);
            lightweight = Light_IsSingular(&scene.lights[light_idx]) ? 1.f : BalanceHeuristic(1, lightpdf, 1, lightbxdfpdf);

            // Apply MIS to account for both
            if (NON_BLACK(le) && lightpdf > 0.0f && !Bxdf_IsSingular(&diffgeo))
            {
                wo = lightwo;
                float ndotwo = fabs(dot(diffgeo.n, normalize(wo)));
                    radiance = le * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * throughput * ndotwo * lightweight / lightpdf / selection_pdf;

                    if (bounce > bounce_cached - 1)
                        indirect_radiance = le * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * indirect_throughput[pixelidx] * ndotwo * lightweight / lightpdf / selection_pdf;
                    else
                        indirect_radiance = 0.f;

#ifdef RADIANCE_PROBE_DIRECT
                    direct_light_samples[globalid] = le * ndotwo * lightweight / lightpdf / selection_pdf;
#endif
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

            if (bounce > bounce_cached - 1)
                indirect_lightsamples[globalid] = REASONABLE_RADIANCE(indirect_radiance);
            else
                indirect_lightsamples[globalid] = 0.f;
        }
        else
        {
            // Otherwise save some intersector cycles
            Ray_SetInactive(shadowrays + globalid);
            lightsamples[globalid] = 0;
            indirect_lightsamples[globalid] = 0;
        }

        // Apply Russian roulette
        float q = max(min(0.5f,
            // Luminance
            0.2126f * throughput.x + 0.7152f * throughput.y + 0.0722f * throughput.z), 0.01f);
        // Only if it is 3+ bounce
        bool rr_apply = bounce > 3;
        bool rr_stop = Sampler_Sample1D(&sampler, SAMPLER_ARGS) > q && rr_apply;

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

            if (bounce > bounce_cached - 1)
            {
                indirect_throughput[pixelidx] *= t / bxdfpdf;
            }

            if (bounce == bounce_cached)
            {
                indirect_rays[pixelidx] = rays[hitidx];
                indirect_rays[pixelidx].o.w = isect.uvwt.w;
                indirect_predicate[pixelidx] = 1;
            }
        }
        else
        {
            // Otherwise kill the path
            Path_Kill(path);
            Ray_SetInactive(indirectrays + globalid);
        }
    }
}

///< Handle light samples and visibility info and add contribution to final buffer
__kernel void GatherLightSamplesIndirect(
    // Pixel indices
    __global int const* pixelindices,
    // Number of rays
    __global int* numrays,
    // Shadow rays hits
    __global int const* shadowhits,
    // Light samples
    __global float3 const* lightsamples,
    // Light samples
    __global float3 const* indirect_lightsamples,
    // throughput
    __global Path const* paths,
    // Radiance sample buffer
    __global float4* output,
    __global float4* indirect
)
{
    int globalid = get_global_id(0);

    if (globalid < *numrays)
    {
        // Get pixel id for this sample set
        int pixelidx = pixelindices[globalid];


        // Prepare accumulator variable
        float3 radiance = make_float3(0.f, 0.f, 0.f);
        float3 indirect_radiance = make_float3(0.f, 0.f, 0.f);

        // Start collecting samples
        {
            // If shadow ray didn't hit anything and reached skydome
            if (shadowhits[globalid] == -1)
            {
                // Add its contribution to radiance accumulator
                radiance += lightsamples[globalid];
                indirect_radiance += indirect_lightsamples[globalid];
            }
        }

        // Divide by number of light samples (samples already have built-in throughput)
        output[pixelidx].xyz += radiance;
        indirect[pixelidx].xyz += indirect_radiance;
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
    uint rngseed,
    // Sampler states
    __global uint* random,
    // Sobol matrices
    __global uint const* sobolmat,
    // Current bounce
    int bounce,
    // Frame
    int frame,
    // Volume data
    __global Volume const* volumes,
    //
    __global HlbvhNode const* restrict bvh,
    //
    __global bbox const* restrict bounds,
    //
    __global RadianceProbeDesc const* restrict descs,
    //
    __global RadianceProbeData const* restrict probes,
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

        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[pixelidx] * 0x1fe3434f;
        Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#elif SAMPLER == RANDOM
        uint scramble = pixelidx * rngseed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[pixelidx];
        uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#endif

        // Fill surface data
        DifferentialGeometry diffgeo;
        DifferentialGeometry_Fill(&scene, &isect, &diffgeo);

        // Check if we are hitting from the inside

        float backfacing = dot(diffgeo.ng, wi) < 0.f;
        int twosided = diffgeo.mat.twosided;
        if (twosided && backfacing)
        {
            // Reverse normal and tangents in this case
            // but not for BTDFs, since BTDFs rely
            // on normal direction in order to arrange
            // indices of refraction
            diffgeo.n = -diffgeo.n;
            diffgeo.dpdu = -diffgeo.dpdu;
            diffgeo.dpdv = -diffgeo.dpdv;
        }

        float ndotwi = dot(diffgeo.n, wi);

        // Select BxDF
        Material_Select(&scene, wi, &sampler, TEXTURE_ARGS, SAMPLER_ARGS, &diffgeo);

        // Terminate if emissive
        if (Bxdf_IsEmissive(&diffgeo))
        {
            if (!backfacing)
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
        if (!twosided && backfacing && !Bxdf_IsBtdf(&diffgeo))
        {
            //Reverse normal and tangents in this case
            //but not for BTDFs, since BTDFs rely
            //on normal direction in order to arrange
            //indices of refraction
            diffgeo.n = -diffgeo.n;
            diffgeo.dpdu = -diffgeo.dpdu;
            diffgeo.dpdv = -diffgeo.dpdv;
        }

        // TODO: this is test code, need to
        // maintain proper volume stack here
        //if (Bxdf_IsBtdf(&diffgeo))
        //{
        //    // If we entering set the volume
        //    path->volume = !backfacing ? 0 : -1;
        //}

        // Check if we need to apply normal map
        //ApplyNormalMap(&diffgeo, TEXTURE_ARGS);
        DifferentialGeometry_ApplyBumpMap(&diffgeo, TEXTURE_ARGS);
        DifferentialGeometry_CalculateTangentTransforms(&diffgeo);

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
        float3 bxdf = 0.f;

        int light_idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf);

        float3 throughput = Path_GetThroughput(path);

#ifdef RADIANCE_PROBE_DIRECT
        if (diffgeo.mat.type != kLambert)
        {
#endif
            // Sample bxdf
            bxdf = Bxdf_Sample(&diffgeo, wi, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &bxdfwo, &bxdfpdf);

            // If we have light to sample we can hopefully do mis
            if (light_idx > -1)
            {
                // Sample light
                float3 le = Light_Sample(light_idx, &scene, &diffgeo, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &lightwo, &lightpdf);
                lightbxdfpdf = Bxdf_GetPdf(&diffgeo, wi, normalize(lightwo), TEXTURE_ARGS);
                lightweight = Light_IsSingular(&scene.lights[light_idx]) ? 1.f : BalanceHeuristic(1, lightpdf, 1, lightbxdfpdf);


                // Apply MIS to account for both
                if (NON_BLACK(le) && lightpdf > 0.0f && !Bxdf_IsSingular(&diffgeo))
                {
                    wo = lightwo;
                    float ndotwo = fabs(dot(diffgeo.n, normalize(wo)));
                    radiance = le * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * throughput * ndotwo * lightweight / lightpdf / selection_pdf;
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
#ifdef RADIANCE_PROBE_DIRECT
        }
        else
        {
            float3 i = RadianceCache_GatherDirectIrradiance(bvh, bounds, descs, probes, diffgeo.p, diffgeo.n);
            output[pixelidx] += Path_GetThroughput(path) * i * diffgeo.mat.kx.xyz;

            // Otherwise save some intersector cycles
            Ray_SetInactive(shadowrays + globalid);
            lightsamples[globalid] = 0;
        }
#endif


        if (diffgeo.mat.type == kLambert)
        {
            float3 i = RadianceCache_GatherIrradiance(bvh, bounds, descs, probes, diffgeo.p, diffgeo.n);

            output[pixelidx] += Path_GetThroughput(path) * i * diffgeo.mat.kx.xyz;

            // Otherwise kill the path
            Path_Kill(path);
            Ray_SetInactive(indirectrays + globalid);
        }
        //else
        //{
        //    bxdfwo = normalize(bxdfwo);
        //    float3 t = bxdf * fabs(dot(diffgeo.n, bxdfwo));

        //    float3 r = RadianceCache_GatherRadiance(bvh, bounds, descs, probes, diffgeo.p, diffgeo.n, bxdfwo);

        //    if (NON_BLACK(t) && bxdfpdf > 0.f)
        //        output[pixelidx] += Path_GetThroughput(path) * bxdf * r / bxdfpdf;

        //    // Otherwise kill the path
        //    Path_Kill(path);
        //    Ray_SetInactive(indirectrays + globalid);
        //}


        // Apply Russian roulette
        float q = max(min(0.5f,
            // Luminance
            0.2126f * throughput.x + 0.7152f * throughput.y + 0.0722f * throughput.z), 0.01f);
        // Only if it is 3+ bounce
        bool rr_apply = bounce > 3;
        bool rr_stop = Sampler_Sample1D(&sampler, SAMPLER_ARGS) > q && rr_apply;

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