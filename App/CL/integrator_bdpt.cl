
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
#include <../App/CL/vertex.cl>


inline 
float Pdf_ConvertSolidAngleToArea(float pdf, float3 po, float3 p, float3 n)
{
    float3 v = po - p;
    float dist = length(v);
    return pdf * fabs(dot(normalize(v), n)) / (dist * dist);
}

__kernel void GatherContributions(
    // Number of rays
   int numrays,
    // Shadow rays hits
    __global int const* shadowhits,
    // Light samples
    __global float3 const* contributions,
    // Radiance sample buffer
    __global float4* output
)
{
    int globalid = get_global_id(0);

    if (globalid < numrays)
    {
        // Prepare accumulator variable
        float3 radiance = make_float3(0.f, 0.f, 0.f);

        // Start collecting samples
        {
            // If shadow ray didn't hit anything and reached skydome
            if (shadowhits[globalid] == -1)
            {
                // Add its contribution to radiance accumulator
                radiance += contributions[globalid];
            }
        }

        // Divide by number of light samples (samples already have built-in throughput)
        output[globalid].xyz += radiance;
    }
}

__kernel void GatherCaustics(
    // Number of rays
    int numrays,
    int w,
    int h,
    // Shadow rays hits
    __global int const* shadowhits,
    // Light samples
    __global float3 const* contributions,
    __global float2 const* coords,
    // Radiance sample buffer
    __global float4* output
)
{
    int globalid = get_global_id(0);

    if (globalid < numrays)
    {
        // Prepare accumulator variable
        float3 radiance = make_float3(0.f, 0.f, 0.f);

        // Start collecting samples
        {
            // If shadow ray didn't hit anything and reached skydome
            if (shadowhits[globalid] == -1)
            {
                // Add its contribution to radiance accumulator
                radiance = contributions[globalid];

                float2 uv = coords[globalid];
                int x = (int)(uv.x * w);
                int y = (int)(uv.y * h);
                output[y * w + x].xyz += radiance;
            }
        }

        // Divide by number of light samples (samples already have built-in throughput)

    }
}

__kernel void IncrementSampleCounter(
    // Number of rays
    int numrays,
    // Radiance sample buffer
    __global float4* output
)
{
    int globalid = get_global_id(0);

    if (globalid < numrays)
    {
        // Start collecting samples
        {
            output[globalid].w += 1.f;
        }
    }
}



__kernel void DirectConnect(
    int s,
    __global int* camera_count,
    __global PathVertex const* camera_subpath,
    __global int* light_count,
    __global PathVertex const* light_subpath,
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
    // RNG seed value
    uint rngseed,
    __global uint* random,
    // Sobol matrices
    __global uint const* sobolmat,
    // Frame
    int frame,
    __global ray* shadowrays,
    __global float3* lightsamples
)
{
    int global_id = get_global_id(0);

    int num_camera_vertices = *(camera_count + global_id);
    int num_light_vertices = *(light_count + global_id);

    float weight = 1.f;// / ((num_camera_vertices - 1) * (num_light_vertices));

    __global PathVertex const* my_camera_vertex = camera_subpath + MAX_PATH_LEN * global_id + s;
    __global PathVertex const* my_prev_camera_vertex = camera_subpath + MAX_PATH_LEN * global_id + s - 1;


    DifferentialGeometry diffgeo;
    diffgeo.p = my_camera_vertex->p;
    diffgeo.n = my_camera_vertex->n;
    diffgeo.ng = my_camera_vertex->ng;
    diffgeo.uv = my_camera_vertex->uv;
    diffgeo.dpdu = GetOrthoVector(diffgeo.n);
    diffgeo.dpdv = cross(diffgeo.n, diffgeo.dpdu);
    diffgeo.mat = materials[my_camera_vertex->matidx];
    diffgeo.mat.fresnel = 1.f;
    DifferentialGeometry_CalculateTangentTransforms(&diffgeo);


    float3 wi = normalize(my_prev_camera_vertex->p - diffgeo.p);
    float3 throughput = my_prev_camera_vertex->flow;

    if (s >= num_camera_vertices)
    {
        lightsamples[global_id].xyz = 0.f;
        Ray_SetInactive(shadowrays + global_id);
        return;
    }

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

    Sampler sampler;
#if SAMPLER == SOBOL
    uint scramble = random[global_id] * 0x1fe3434f;
    Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#elif SAMPLER == RANDOM
    uint scramble = global_id * rngseed;
    Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
    uint rnd = random[global_id];
    uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM));
    Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#endif


    float selection_pdf;
    int light_idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf);

    // Sample light
    float lightpdf;
    float3 lightwo;
    float3 le = Light_Sample(light_idx, &scene, &diffgeo, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &lightwo, &lightpdf);
    float3 radiance = 0.f;
    float3 wo;

    // Apply MIS to account for both
    if (NON_BLACK(le) && lightpdf > 0.0f && !Bxdf_IsSingular(&diffgeo))
    {
        wo = lightwo;
        float ndotwo = fabs(dot(diffgeo.n, normalize(wo)));
        radiance = le * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * throughput * ndotwo / lightpdf / selection_pdf;
    }

    // If we have some light here generate a shadow ray
    if (NON_BLACK(radiance))
    {
        // Generate shadow ray
        float shadow_ray_length = 0.999f * (1.f - CRAZY_LOW_DISTANCE) * length(lightwo);
        float3 shadow_ray_dir = normalize(lightwo);
        float3 shadow_ray_o = diffgeo.p + normalize(diffgeo.n) * CRAZY_LOW_DISTANCE;
        int shadow_ray_mask = 0xFFFFFFFF;

        Ray_Init(shadowrays + global_id, shadow_ray_o, shadow_ray_dir, shadow_ray_length, 0.f, shadow_ray_mask);

        // And write the light sample
        lightsamples[global_id] = REASONABLE_RADIANCE(radiance);
    }
    else
    {
        // Otherwise save some intersector cycles
        Ray_SetInactive(shadowrays + global_id);
        lightsamples[global_id] = 0;
    }
}


__kernel void Connect(
    int s,
    int t,
    __global int* camera_count,
    __global PathVertex const* camera_subpath,
    __global int* light_count,
    __global PathVertex const* light_subpath,
    // Materials
    __global Material const* materials,
    // Textures
    TEXTURE_ARG_LIST,
    __global ray* connection_rays,
    __global float4* contributions
)
{
    int global_id = get_global_id(0);

    int num_camera_vertices = *(camera_count + global_id);
    int num_light_vertices = *(light_count + global_id);

    if (s >= num_camera_vertices ||
        t >= num_light_vertices)
    {
        contributions[global_id].xyz = 0.f;
        Ray_SetInactive(connection_rays + global_id);
        return;
    }

    float weight = 1.f / ((num_camera_vertices - 1) * (num_light_vertices - 1));

    __global PathVertex const* my_camera_vertex = camera_subpath + MAX_PATH_LEN * global_id + s;
    __global PathVertex const* my_prev_camera_vertex = camera_subpath + MAX_PATH_LEN * global_id + s - 1;

    __global PathVertex const* my_light_vertex = light_subpath + MAX_PATH_LEN * global_id + t;
    __global PathVertex const* my_prev_light_vertex = light_subpath + MAX_PATH_LEN * global_id + t - 1;

    DifferentialGeometry camera_dg;
    camera_dg.p = my_camera_vertex->p;
    camera_dg.n = my_camera_vertex->n;
    camera_dg.ng = my_camera_vertex->ng;
    camera_dg.uv = my_camera_vertex->uv;
    camera_dg.dpdu = GetOrthoVector(camera_dg.n);
    camera_dg.dpdv = cross(camera_dg.n, camera_dg.dpdu);
    camera_dg.mat = materials[my_camera_vertex->matidx];
    camera_dg.mat.fresnel = 1.f;
    DifferentialGeometry_CalculateTangentTransforms(&camera_dg);

    DifferentialGeometry light_dg;
    light_dg.p = my_light_vertex->p;
    light_dg.n = my_light_vertex->n;
    light_dg.ng = my_light_vertex->ng;
    light_dg.uv = my_light_vertex->uv;
    light_dg.dpdu = GetOrthoVector(light_dg.n);
    light_dg.dpdv = cross(light_dg.dpdu, light_dg.n);
    light_dg.mat = materials[my_light_vertex->matidx];
    light_dg.mat.fresnel = 1.f;
    DifferentialGeometry_CalculateTangentTransforms(&light_dg);

    float3 camera_wi = normalize(camera_dg.p - my_prev_camera_vertex->p);
    float3 camera_wo = normalize(light_dg.p - camera_dg.p);
    float dist = length(light_dg.p - camera_dg.p);
    float3 camera_bxdf = Bxdf_Evaluate(&camera_dg, camera_wi, camera_wo, TEXTURE_ARGS);
    float camera_pdf = Bxdf_GetPdf(&camera_dg, camera_wi, camera_wo, TEXTURE_ARGS);
    float camera_dotnl = fabs(dot(camera_dg.n, camera_wo));

    float3 camera_contribution = my_prev_camera_vertex->flow;
    camera_contribution *= (camera_bxdf * camera_dotnl);

    float3 light_wi = normalize(light_dg.p - my_prev_light_vertex->p);
    float3 light_wo = -camera_wo;
    float3 light_bxdf = Bxdf_Evaluate(&light_dg, light_wi, light_wo, TEXTURE_ARGS);
    float light_pdf = Bxdf_GetPdf(&light_dg, light_wi, light_wo, TEXTURE_ARGS);
    float light_dotnl = fabs(dot(light_dg.n, light_wo));

    float3 light_contribution = my_prev_light_vertex->flow;
    light_contribution *= (light_bxdf * light_dotnl);


    float3 val = weight * ( camera_contribution * light_contribution ) / (dist * dist);
    contributions[global_id].xyz = REASONABLE_RADIANCE(val);

    float  connect_ray_length = (1.f - 2.f * CRAZY_LOW_DISTANCE) * dist;
    float3 connect_ray_dir = camera_wo;
    float3 connect_ray_o = camera_dg.p + CRAZY_LOW_DISTANCE * camera_dg.n;

    Ray_Init(connection_rays + global_id, connect_ray_o, connect_ray_dir, connect_ray_length, 0.f, 0xFFFFFFFF);
}


__kernel void ConnectCaustic(
    int t,
    __global int* camera_count,
    __global PathVertex const* camera_subpath,
    __global int* light_count,
    __global PathVertex const* light_subpath,
    __global Camera const* camera,
    __global Material const* materials,
    TEXTURE_ARG_LIST,
    __global ray* connection_rays,
    __global float4* contributions,
    __global float2* coords
)
{
    int global_id = get_global_id(0);

    int num_light_vertices = *(light_count + global_id);
    int num_camera_vertices = *(camera_count + global_id);

    if (t >= num_light_vertices)
    {
        contributions[global_id].xyz = 0.f;
        Ray_SetInactive(connection_rays + global_id);
        return;
    }

    float weight = 1.f / ((num_camera_vertices) * (num_light_vertices));

    __global PathVertex const* my_camera_vertex = camera_subpath + MAX_PATH_LEN * global_id;
    __global PathVertex const* my_light_vertex = light_subpath + MAX_PATH_LEN * global_id + t;
    __global PathVertex const* my_prev_light_vertex = light_subpath + MAX_PATH_LEN * global_id + t - 1;

    DifferentialGeometry camera_dg;
    camera_dg.p = my_camera_vertex->p;
    camera_dg.n = my_camera_vertex->n;
    camera_dg.ng = my_camera_vertex->ng;
    camera_dg.uv = my_camera_vertex->uv;
    camera_dg.dpdu = GetOrthoVector(camera_dg.n);
    camera_dg.dpdv = cross(camera_dg.n, camera_dg.dpdu);
    camera_dg.mat = materials[my_camera_vertex->matidx];
    camera_dg.mat.fresnel = 1.f;
    DifferentialGeometry_CalculateTangentTransforms(&camera_dg);

    DifferentialGeometry light_dg;
    light_dg.p = my_light_vertex->p;
    light_dg.n = my_light_vertex->n;
    light_dg.ng = my_light_vertex->ng;
    light_dg.uv = my_light_vertex->uv;
    light_dg.dpdu = GetOrthoVector(light_dg.n);
    light_dg.dpdv = cross(light_dg.dpdu, light_dg.n);
    light_dg.mat = materials[my_light_vertex->matidx];
    light_dg.mat.fresnel = 1.f;
    DifferentialGeometry_CalculateTangentTransforms(&light_dg);

    float3 camera_wo = normalize(light_dg.p - camera_dg.p);

    float3 light_wi = normalize(light_dg.p - my_prev_light_vertex->p);
    float3 light_wo = -camera_wo;
    float3 light_bxdf = Bxdf_Evaluate(&light_dg, normalize(light_wi), normalize(light_wo), TEXTURE_ARGS);
    float light_pdf = Bxdf_GetPdf(&light_dg, light_wi, light_wo, TEXTURE_ARGS);
    float light_dotnl = fabs(dot(light_dg.n, light_wo));

    float3 light_contribution = my_prev_light_vertex->flow;
    light_contribution *= (light_bxdf * light_dotnl);

    //float w0 = camera_pdf / (camera_pdf + light_pdf);
    //float w1 = light_pdf / (camera_pdf + light_pdf);

    float3 val =  weight * (light_contribution);
    contributions[global_id].xyz = REASONABLE_RADIANCE(val);

    float  connect_ray_length = (1.f - 2.f * CRAZY_LOW_DISTANCE) * length(light_dg.p - camera_dg.p);
    float3 connect_ray_dir = camera_wo;
    float3 connect_ray_o = camera_dg.p;


    Ray_Init(connection_rays + global_id, connect_ray_o, connect_ray_dir, connect_ray_length, 0.f, 0xFFFFFFFF);

    ray r;
    r.o.xyz = light_dg.p;
    r.o.w = CRAZY_HIGH_DISTANCE;
    r.d.xyz = normalize(camera_dg.p - light_dg.p);

    float3 v0, v1, v2, v3;
    float2 ext = 0.5 * camera->dim;
    v0 = camera->focal_length * camera->forward - ext.x * camera->right - ext.y * camera->up;
    v1 = camera->focal_length * camera->forward + ext.x * camera->right - ext.y * camera->up;
    v2 = camera->focal_length * camera->forward + ext.x * camera->right + ext.y * camera->up;
    v3 = camera->focal_length * camera->forward - ext.x * camera->right + ext.y * camera->up;

    r.o.w = CRAZY_HIGH_DISTANCE;
    float a, b;
    float3 p;
    if (IntersectTriangle(&r, v0, v1, v2, &a, &b))
    {
        p = (1.f - a - b) * v0 + a * v1 + b * v2;
    }
    else if (IntersectTriangle(&r, v0, v2, v3, &a, &b))
    {
        p = (1.f - a - b) * v0 + a * v2 + b * v3;
    }
    else
    {
        contributions[global_id].xyz = 0.f;
        Ray_SetInactive(connection_rays + global_id);
        return;
    }

    float3 imgp = p - v0;
    float2 impuv;
    impuv.x = clamp(dot(imgp, v1 - v0) / dot(v1 - v0, v1 - v0), 0.f, 1.f);
    impuv.y = clamp(dot(imgp, v3 - v0) / dot(v3 - v0, v3 - v0), 0.f, 1.f);
    coords[global_id] = impuv;
}


__kernel void GenerateLightVertices(
    int num_vertices,
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
    // RNG seed value
    uint rngseed,
    // Output rays
    __global ray* rays,
    __global uint* random,
    __global uint const* sobolmat,
    int frame,
    __global PathVertex* path_vertices,
    __global int* path_vertex_counts
#ifndef NO_PATH_DATA
    , __global Path* paths
#endif
)
{
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

    int globalid = get_global_id(0);


    // Check borders
    if (globalid < num_vertices)
    {
        // Get pointer to ray to handle
        __global ray* myray = rays + globalid;
        __global PathVertex* my_vertex = path_vertices + MAX_PATH_LEN * globalid;
        __global int* my_count = path_vertex_counts + globalid;

#ifndef NO_PATH_DATA
        __global Path* mypath = paths + globalid;
#endif

        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[globalid] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[globalid] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = globalid * rngseed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[globalid];
        uint scramble = rnd * 0x1fe3434f * ((frame + 133 * rnd) / (CMJ_DIM * CMJ_DIM));
        Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_CAMERA_OFFSET, scramble);
#endif

        // Generate sample
        float2 sample0 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);
        float2 sample1 = Sampler_Sample2D(&sampler, SAMPLER_ARGS);

        float selection_pdf;
        int idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf);

        float3 p, n, wo;
        float light_pdf;
        float3 ke = Light_SampleVertex(idx, &scene, TEXTURE_ARGS, sample0, sample1, &p, &n, &wo, &light_pdf);

        // Calculate direction to image plane
        myray->d.xyz = normalize(wo);
        // Origin == camera position + nearz * d
        myray->o.xyz = p + CRAZY_LOW_DISTANCE * n;
        // Max T value = zfar - znear since we moved origin to znear
        myray->o.w = CRAZY_HIGH_DISTANCE;
        // Generate random time from 0 to 1
        myray->d.w = sample0.x;
        // Set ray max
        myray->extra.x = 0xFFFFFFFF;
        myray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(myray, 1.f);

#ifndef NO_PATH_DATA
        mypath->throughput = ke / (selection_pdf * light_pdf);
        mypath->volume = -1;
        mypath->flags = 0;
        mypath->active = 0xFF;
#endif

        PathVertex v;
        PathVertex_Init(&v,
            p,
            n,
            n,
            0.f,
            1.f,
            1.f,
            ke / (selection_pdf * light_pdf),
            kLight,
            -1);
        v.val = ke;
        *my_count = 1;
        *my_vertex = v;
    }
}



__kernel void SampleSurface(
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
    // Path throughput
    __global Path* paths,
    // Indirect rays
    __global ray* newrays,
    // Vertices
    __global PathVertex* path_vertices,
    __global int* path_vertex_counts
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
            Path_Kill(path);
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

        float3 throughput = Path_GetThroughput(path);

        // Sample bxdf
        float3 bxdf = Bxdf_Sample(&diffgeo, wi, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &bxdfwo, &bxdfpdf);

        bxdfwo = normalize(bxdfwo);
        float3 t = bxdf * fabs(dot(diffgeo.n, bxdfwo));

        // Only continue if we have non-zero throughput & pdf
        if (NON_BLACK(t) && bxdfpdf > 0.f)
        {
            // Update the throughput
            Path_MulThroughput(path, t / bxdfpdf);

            // Generate ray
            float3 new_ray_dir = bxdfwo;
            float3 new_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * s * diffgeo.n;

            Ray_Init(newrays + globalid, new_ray_o, new_ray_dir, CRAZY_HIGH_DISTANCE, 0.f, 0xFFFFFFFF);
            Ray_SetExtra(newrays + globalid, make_float2(bxdfpdf, 0.f));

            // Write path vertex
            __global int* my_counter = path_vertex_counts + pixelidx;
            int idx = atom_inc(my_counter);

            if (idx < MAX_PATH_LEN)
            {
                __global PathVertex* my_vertex = path_vertices + MAX_PATH_LEN * pixelidx + idx;
                __global PathVertex* my_prev_vertex = path_vertices + MAX_PATH_LEN * pixelidx + idx - 1;
                // Fill vertex

                // Calculate forward PDF of a current vertex
                float pdf_solid_angle = Ray_GetExtra(&rays[hitidx]).x;
                float pdf_fwd = Pdf_ConvertSolidAngleToArea(pdf_solid_angle, rays[hitidx].o.xyz, diffgeo.p, diffgeo.n);

                PathVertex v;
                PathVertex_Init(&v, 
                    diffgeo.p, 
                    diffgeo.n, 
                    diffgeo.ng, 
                    diffgeo.uv, 
                    pdf_fwd, 
                    0.f, 
                    throughput * t / bxdfpdf, 
                    kSurface,
                    diffgeo.matidx);

                *my_vertex = v;

                // TODO: wrong
                if (my_prev_vertex->type == kSurface)
                {
                    float pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&diffgeo, bxdfwo, wi, TEXTURE_ARGS), diffgeo.p, my_prev_vertex->p, my_prev_vertex->n);
                    my_prev_vertex->pdf_bwd = pdf_bwd;
                }
            }
            else
            {
                atom_dec(my_counter);
                Path_Kill(path);
                Ray_SetInactive(newrays + globalid);
            }
        }
        else
        {
            // Otherwise kill the path
            Path_Kill(path);
            Ray_SetInactive(newrays + globalid);
        }
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
