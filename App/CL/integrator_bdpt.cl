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

// Convert PDF of sampling point po from point p from solid angle measure to area measure
INLINE
float Pdf_ConvertSolidAngleToArea(float pdf, float3 po, float3 p, float3 n)
{
    float3 v = po - p;
    float dist = length(v);
    return pdf * fabs(dot(normalize(v), n)) / (dist * dist);
}

KERNEL void GenerateLightVertices(
    // Number of subpaths to generate
    int num_subpaths,
    // Vertices
    GLOBAL float3 const* restrict vertices,
    // Normals
    GLOBAL float3 const* restrict normals,
    // UVs
    GLOBAL float2 const* restrict uvs,
    // Indices
    GLOBAL int const* restrict indices,
    // Shapes
    GLOBAL Shape const* restrict shapes,
    // Material IDs
    GLOBAL int const* restrict material_ids,
    // Materials
    GLOBAL Material const* restrict materials,
    // Textures
    TEXTURE_ARG_LIST,
    // Environment texture index
    int env_light_idx,
    // Emissives
    GLOBAL Light const* restrict lights,
    // Number of emissive objects
    int num_lights,
    // RNG seed value
    uint rngseed,
    int frame,
    // RNG data
    GLOBAL uint const* restrict random,
    GLOBAL uint const* restrict sobolmat,
    // Output rays
    GLOBAL ray* restrict rays,
    // Light subpath
    GLOBAL PathVertex* restrict light_subpath,
    // Light subpath length
    GLOBAL int* restrict light_subpath_length,
    // Path buffer
    GLOBAL Path* restrict paths
)
{
    Scene scene =
    {
        vertices,
        normals,
        uvs,
        indices,
        shapes,
        material_ids,
        materials,
        lights,
        env_light_idx,
        num_lights
    };

    int global_id = get_global_id(0);

    // Check borders
    if (global_id < num_subpaths)
    {
        // Get pointer to ray to handle
        GLOBAL ray* my_ray = rays + global_id;
        GLOBAL PathVertex* my_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id;
        GLOBAL int* my_count = light_subpath_length + global_id;
        GLOBAL Path* my_path = paths + global_id;

        Sampler sampler;
#if SAMPLER == SOBOL
        uint scramble = random[global_id] * 0x1fe3434f;

        if (frame & 0xF)
        {
            random[global_id] = WangHash(scramble);
        }

        Sampler_Init(&sampler, frame, SAMPLE_DIM_CAMERA_OFFSET, scramble);
#elif SAMPLER == RANDOM
        uint scramble = global_id * rngseed;
        Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
        uint rnd = random[global_id];
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
        my_ray->d.xyz = normalize(wo);
        // Origin == camera position + nearz * d
        my_ray->o.xyz = p + CRAZY_LOW_DISTANCE * n;
        // Max T value = zfar - znear since we moved origin to znear
        my_ray->o.w = CRAZY_HIGH_DISTANCE;
        // Generate random time from 0 to 1
        my_ray->d.w = sample0.x;
        // Set ray max
        my_ray->extra.x = 0xFFFFFFFF;
        my_ray->extra.y = 0xFFFFFFFF;
        Ray_SetExtra(my_ray, 1.f);

        PathVertex v;
        PathVertex_Init(&v,
            p,
            n,
            n,
            0.f,
            selection_pdf * light_pdf,
            1.f,
            100.f,//ke * fabs(dot(n, my_ray->d.xyz)) / selection_pdf * light_pdf,
            kLight,
            -1);

        *my_count = 1;
        *my_vertex = v;

        // Initlize path data
        my_path->throughput = ke * fabs(dot(n, my_ray->d.xyz)) / selection_pdf * light_pdf;
        my_path->volume = -1;
        my_path->flags = 0;
        my_path->active = 0xFF;
    }
}

KERNEL void SampleSurface( 
    // Ray batch
    GLOBAL ray const* rays,
    // Intersection data
    GLOBAL Intersection const* intersections,
    // Hit indices
    GLOBAL int const* hit_indices,
    // Pixel indices
    GLOBAL int const* pixel_indices,
    // Number of rays
    GLOBAL int const* num_hits,
    // Vertices
    GLOBAL float3 const* vertices,
    // Normals
    GLOBAL float3 const* normals,
    // UVs
    GLOBAL float2 const* uvs,
    // Indices
    GLOBAL int const* indices,
    // Shapes
    GLOBAL Shape const* shapes,
    // Material IDs
    GLOBAL int const* material_ids,
    // Materials
    GLOBAL Material const* materials,
    // Textures
    TEXTURE_ARG_LIST,
    // Environment texture index
    int env_light_idx,
    // Emissives
    GLOBAL Light const* lights,
    // Number of emissive objects
    int num_lights,
    // RNG seed
    uint rngseed,
    // Sampler states
    GLOBAL uint* random,
    // Sobol matrices
    GLOBAL uint const* sobolmat,
    // Current bounce
    int bounce,
    // Frame
    int frame,
    // Radiance or importance
    int transfer_mode,
    // Path throughput
    GLOBAL Path* paths,
    // Indirect rays
    GLOBAL ray* extension_rays,
    // Vertices
    GLOBAL PathVertex* subpath,
    GLOBAL int* subpath_length
)
    {
        int global_id = get_global_id(0);

        Scene scene =
        {
            vertices,
            normals,
            uvs,
            indices,
            shapes,
            material_ids,
            materials,
            lights,
            env_light_idx,
            num_lights
        };

        // Only applied to active rays after compaction
        if (global_id < *num_hits)
        {
            // Fetch index
            int hit_idx = hit_indices[global_id];
            int pixel_idx = pixel_indices[global_id];
            Intersection isect = intersections[hit_idx];

            GLOBAL Path* path = paths + pixel_idx;

            // Fetch incoming ray direction
            float3 wi = -normalize(rays[hit_idx].d.xyz);

            Sampler sampler;
#if SAMPLER == SOBOL
            uint scramble = random[pixel_idx] * 0x1fe3434f;
            Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#elif SAMPLER == RANDOM
            uint scramble = pixel_idx * rngseed;
            Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
            uint rnd = random[pixel_idx];
            uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM));
            Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + bounce * SAMPLE_DIMS_PER_BOUNCE, scramble);
#endif

            // Fill surface data
            DifferentialGeometry diffgeo;
            Scene_FillDifferentialGeometry(&scene, &isect, &diffgeo);
            diffgeo.transfer_mode = transfer_mode; 

            // Check if we are hitting from the inside
            float backfacing = dot(diffgeo.ng, wi) < 0.f;
            int twosided = false; // diffgeo.mat.twosided;
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
            DifferentialGeometry_ApplyBumpNormalMap(&diffgeo, TEXTURE_ARGS);
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

            // Write path vertex
            GLOBAL int* my_counter = subpath_length + pixel_idx;
            int idx = atom_inc(my_counter);

            if (idx < BDPT_MAX_SUBPATH_LEN)
            {
                GLOBAL PathVertex* my_vertex = subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + idx;
                GLOBAL PathVertex* my_prev_vertex = subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + idx - 1;
                // Fill vertex
                // Calculate forward PDF of a current vertex
                float pdf_solid_angle = Ray_GetExtra(&rays[hit_idx]).x;
                float pdf_fwd = Pdf_ConvertSolidAngleToArea(pdf_solid_angle, rays[hit_idx].o.xyz, diffgeo.p, diffgeo.n);

                PathVertex v;
                PathVertex_Init(&v,
                    diffgeo.p,
                    diffgeo.n,
                    diffgeo.ng,
                    diffgeo.uv,
                    pdf_fwd,
                    0.f,
                    my_prev_vertex->flow * t / bxdfpdf,
                    kSurface,
                    diffgeo.material_index);

                *my_vertex = v;

                // TODO: wrong
                if (my_prev_vertex->type == kSurface)
                {
                    float pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&diffgeo, bxdfwo, wi, TEXTURE_ARGS), diffgeo.p, my_prev_vertex->position, my_prev_vertex->shading_normal);
                    my_prev_vertex->pdf_backward = pdf_bwd;
                }
            }
            else
            {
                atom_dec(my_counter);
                Path_Kill(path);
                Ray_SetInactive(extension_rays + global_id);
                return;
            }

            // Only continue if we have non-zero throughput & pdf
            if (NON_BLACK(t) && bxdfpdf > 0.f)
            {
                // Update the throughput
                Path_MulThroughput(path, t / bxdfpdf);

                // Generate ray
                float3 new_ray_dir = bxdfwo;
                float3 new_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * s * diffgeo.n;

                Ray_Init(extension_rays + global_id, new_ray_o, new_ray_dir, CRAZY_HIGH_DISTANCE, 0.f, 0xFFFFFFFF);
                Ray_SetExtra(extension_rays + global_id, make_float2(bxdfpdf, 0.f));
            }
            else
            {
                // Otherwise kill the path
                Path_Kill(path);
                Ray_SetInactive(extension_rays + global_id);
            }
        }
    }

    KERNEL void ConnectDirect(
        int eye_vertex_index,
        int num_rays,
        GLOBAL int const* pixel_indices,
        GLOBAL PathVertex const* restrict eye_subpath,
        GLOBAL int const* restrict eye_subpath_length,
        GLOBAL float3 const* restrict vertices,
        GLOBAL float3 const* restrict normals,
        GLOBAL float2 const* restrict uvs,
        GLOBAL int const* restrict indices,
        GLOBAL Shape const* restrict shapes,
        GLOBAL int const* restrict materialids,
        GLOBAL Material const* restrict materials,
        TEXTURE_ARG_LIST,
        int env_light_idx,
        GLOBAL Light const* restrict lights,
        int num_lights,
        uint rngseed,
        GLOBAL uint const* restrict random,
        GLOBAL uint const* restrict sobolmat,
        int frame,
        GLOBAL ray* restrict shadowrays,
        GLOBAL float3* restrict lightsamples
    )
    {
        int global_id = get_global_id(0);

        if (global_id < num_rays)
        {
            int pixel_idx = pixel_indices[global_id];
            int num_eye_vertices = *(eye_subpath_length + pixel_idx);

            GLOBAL PathVertex const* my_eye_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + eye_vertex_index;
            GLOBAL PathVertex const* my_prev_eye_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + eye_vertex_index - 1;

            DifferentialGeometry diffgeo;
            diffgeo.p = my_eye_vertex->position;
            diffgeo.n = my_eye_vertex->shading_normal;
            diffgeo.ng = my_eye_vertex->geometric_normal;
            diffgeo.uv = my_eye_vertex->uv;
            diffgeo.dpdu = GetOrthoVector(diffgeo.n);
            diffgeo.dpdv = cross(diffgeo.n, diffgeo.dpdu);
            diffgeo.mat = materials[my_eye_vertex->material_index];
            diffgeo.mat.fresnel = 1.f;
            DifferentialGeometry_CalculateTangentTransforms(&diffgeo);

            float3 wi = normalize(my_prev_eye_vertex->position - diffgeo.p);
            float3 throughput = my_prev_eye_vertex->flow;

            if (eye_vertex_index >= num_eye_vertices)
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
                env_light_idx,
                num_lights
            };

            Sampler sampler;
#if SAMPLER == SOBOL
            uint scramble = random[global_id] * 0x1fe3434f;
            Sampler_Init(&sampler, frame, SAMPLE_DIM_SURFACE_OFFSET + eye_vertex_index * SAMPLE_DIMS_PER_BOUNCE, scramble); 
#elif SAMPLER == RANDOM
            uint scramble = global_id * rngseed;
            Sampler_Init(&sampler, scramble);
#elif SAMPLER == CMJ
            uint rnd = random[global_id];
            uint scramble = rnd * 0x1fe3434f * ((frame + 331 * rnd) / (CMJ_DIM * CMJ_DIM));
            Sampler_Init(&sampler, frame % (CMJ_DIM * CMJ_DIM), SAMPLE_DIM_SURFACE_OFFSET + eye_vertex_index * SAMPLE_DIMS_PER_BOUNCE, scramble);
#endif

            float selection_pdf;
            int light_idx = Scene_SampleLight(&scene, Sampler_Sample1D(&sampler, SAMPLER_ARGS), &selection_pdf);

            // Sample light
            float lightpdf = 0.f;
            float lightbxdfpdf = 0.f;
            float bxdfpdf = 0.f;
            float bxdflightpdf = 0.f;
            float lightweight = 0.f;
            float bxdfweight = 0.f;


            float3 lightwo;
            float3 bxdfwo;
            float3 le = Light_Sample(light_idx, &scene, &diffgeo, TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &lightwo, &lightpdf);
            float3 bxdf = Bxdf_Sample(&diffgeo, normalize(wi), TEXTURE_ARGS, Sampler_Sample2D(&sampler, SAMPLER_ARGS), &bxdfwo, &bxdfpdf);
            lightbxdfpdf = Bxdf_GetPdf(&diffgeo, normalize(wi), normalize(lightwo), TEXTURE_ARGS);
            bxdflightpdf = Light_GetPdf(light_idx, &scene, &diffgeo, normalize(bxdfwo), TEXTURE_ARGS);

            bool singular_light = Light_IsSingular(&scene.lights[light_idx]);
            bool singular_bxdf = Bxdf_IsSingular(&diffgeo);
            lightweight = singular_light ? 1.f : BalanceHeuristic(1, lightpdf * selection_pdf, 1, lightbxdfpdf);
            bxdfweight = singular_bxdf ? 1.f : BalanceHeuristic(1, bxdfpdf, 1, bxdflightpdf * selection_pdf);

            float3 radiance = 0.f; 
            float3 wo;

            if (singular_light && singular_bxdf) 
            {
                // Otherwise save some intersector cycles
                Ray_SetInactive(shadowrays + global_id);  
                lightsamples[global_id] = 0; 
                return;
            }

            float split = Sampler_Sample1D(&sampler, SAMPLER_ARGS);

            if (((singular_bxdf && !singular_light) || split < 0.5f) && bxdfpdf > 0.f)
            {
                wo = CRAZY_HIGH_DISTANCE * bxdfwo; 
                float ndotwo = fabs(dot(diffgeo.n, normalize(wo)));
                le = Light_GetLe(light_idx, &scene, &diffgeo, &wo, TEXTURE_ARGS);
                radiance = 2.f * bxdfweight * le * throughput * bxdf * ndotwo / bxdfpdf;
            }

            if (((!singular_bxdf && singular_light) || split >= 0.5f) && lightpdf > 0.f) 
            {
                wo = lightwo;
                float ndotwo = fabs(dot(diffgeo.n, normalize(wo)));  
                radiance = 2.f * lightweight * le * Bxdf_Evaluate(&diffgeo, wi, normalize(wo), TEXTURE_ARGS) * throughput * ndotwo / lightpdf / selection_pdf;
            }

            // If we have some light here generate a shadow ray
            if (NON_BLACK(radiance)) 
            {
                // Generate shadow ray
                float3 shadow_ray_o = diffgeo.p + CRAZY_LOW_DISTANCE * diffgeo.n; 
                float3 temp = diffgeo.p + wo - shadow_ray_o;
                float3 shadow_ray_dir = normalize(temp);
                float shadow_ray_length = 0.999f * length(temp);
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
    }

    KERNEL void Connect(
        int num_rays,
        // Index of eye vertex we are trying to connect
        int eye_vertex_index,
        // Index of light vertex we are trying to connect
        int light_vertex_index,
        GLOBAL int const* pixel_indices,
        // Vertex arrays
        GLOBAL PathVertex const* restrict eye_subpath,
        GLOBAL int const* restrict eye_subpath_length,
        GLOBAL PathVertex const* restrict light_subpath,
        GLOBAL int const* restrict light_subpath_length,
        GLOBAL Material const* restrict materials,
        // Textures
        TEXTURE_ARG_LIST,
        GLOBAL ray* connection_rays,
        // Path contriburions to final image
        GLOBAL float4* contributions
    )
    {
        int global_id = get_global_id(0);


        if (global_id >= num_rays)
        {
            return;
        }

        int pixel_idx = pixel_indices[global_id];

        // Check if we our indices are within subpath index range
        if (eye_vertex_index >= eye_subpath_length[pixel_idx] ||
            light_vertex_index >= light_subpath_length[global_id])
        {
            contributions[global_id].xyz = 0.f;
            Ray_SetInactive(&connection_rays[global_id]);
            return;
        }

        // Prepare data pointers
        GLOBAL PathVertex const* my_eye_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + eye_vertex_index;
        GLOBAL PathVertex const* my_prev_eye_vertex = eye_subpath + BDPT_MAX_SUBPATH_LEN * pixel_idx + eye_vertex_index - 1;

        GLOBAL PathVertex const* my_light_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id + light_vertex_index;
        GLOBAL PathVertex const* my_prev_light_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id + light_vertex_index - 1;

        // Fill differential geometries for both eye and light vertices
        DifferentialGeometry eye_dg;
        eye_dg.p = my_eye_vertex->position;
        eye_dg.n = my_eye_vertex->shading_normal;
        eye_dg.ng = my_eye_vertex->geometric_normal;
        eye_dg.uv = my_eye_vertex->uv;
        eye_dg.dpdu = GetOrthoVector(eye_dg.n);
        eye_dg.dpdv = cross(eye_dg.n, eye_dg.dpdu);
        eye_dg.mat = materials[my_eye_vertex->material_index];
        eye_dg.mat.fresnel = 1.f;
        DifferentialGeometry_CalculateTangentTransforms(&eye_dg);

        DifferentialGeometry light_dg;
        light_dg.p = my_light_vertex->position;
        light_dg.n = my_light_vertex->shading_normal;
        light_dg.ng = my_light_vertex->geometric_normal;
        light_dg.uv = my_light_vertex->uv;
        light_dg.dpdu = GetOrthoVector(light_dg.n);
        light_dg.dpdv = cross(light_dg.dpdu, light_dg.n);
        light_dg.mat = materials[my_light_vertex->material_index];
        light_dg.mat.fresnel = 1.f;
        DifferentialGeometry_CalculateTangentTransforms(&light_dg);

        // Vector from eye subpath vertex to previous eye subpath vertex (incoming vector)
        float3 eye_wi = normalize(my_prev_eye_vertex->position - eye_dg.p);
        // Vector from camera subpath vertex to light subpath vertex (connection vector)
        float3 eye_wo = normalize(light_dg.p - eye_dg.p);
        // Distance between vertices we are connecting
        float dist = length(light_dg.p - eye_dg.p);
        // BXDF at eye subpath vertex
        float3 eye_bxdf = Bxdf_Evaluate(&eye_dg, eye_wi, eye_wo, TEXTURE_ARGS);
        // Eye subpath vertex cosine factor
        float eye_dotnl = fabs(dot(eye_dg.n, eye_wo));

        // Calculate througput of a segment eye->eyevetex0->...->eyevertexN->lightvertexM->lightvertexM-1...->light
        // First eye subpath throughput eye->eyevertex0->...eyevertexN
        float3 eye_contribution = my_prev_eye_vertex->flow;
        eye_contribution *= (eye_bxdf * eye_dotnl);

        // Vector from light subpath vertex to prev light subpath vertex (incoming light vector)
        float3 light_wi = normalize(my_prev_light_vertex->position - light_dg.p);
        // Outgoing light subpath vector
        float3 light_wo = -eye_wo;
        // BXDF at light subpath vertex
        float3 light_bxdf = Bxdf_Evaluate(&light_dg, light_wi, light_wo, TEXTURE_ARGS);
        // Light subpath vertex cosine factor
        float light_dotnl = fabs(dot(light_dg.n, light_wo));

        // Light subpath throughput lightvertexM->lightvertexM-1->light
        float3 light_contribution = my_prev_light_vertex->flow;
        light_contribution *= (light_bxdf * light_dotnl);

        // Now we need to figure out MIS weight for the path we are trying to build 
        // connecting these vertices.
        // MIS weight should account for all possible paths of the same length using 
        // same vertices, but generated using different subpath lengths, for example
        // Original: eye0 eye1 eye2 eye3 | connect | light3 light2 light1 light0
        // Variant 1: eye0 eye1 eye2 | connect | eye3 light3 light2 light1 light0
        // as if only eye0->eye1->eye2 was generated using eye walk, 
        // and eye3->light3->light2->light1->light0 was generated using light walk.

        // This is PDF of sampling eye subpath vertex from previous eye subpath vertex using eye walk strategy.
        float eye_vertex_pdf_fwd = my_eye_vertex->pdf_forward;
        // This is PDF of sampling light subpath vertex from previous light subpath vertex using light walk strategy.
        float light_vertex_pdf_fwd = my_light_vertex->pdf_forward;

        // Important: this is PDF of sampling eye subpath vertex from light subpath vertex using light walk strategy.
        float eye_vertex_pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&light_dg, light_wi, light_wo, TEXTURE_ARGS),
            eye_dg.p, light_dg.p, light_dg.n);
        // Important: this is PDF of sampling prev eye subpath vertex from eye subpath vertex using light walk strategy.
        float eye_prev_vertex_pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&eye_dg, eye_wi, eye_wo, TEXTURE_ARGS),
            my_prev_eye_vertex->position, eye_dg.p, eye_dg.n);

        // Important: this is PDF of sampling light subpath vertex from eye subpath vertex using eye walk strategy.
        float light_vertex_pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&eye_dg, eye_wo, eye_wi, TEXTURE_ARGS),
            light_dg.p, eye_dg.p, eye_dg.n);
        // Important: this is PDF of sampling prev light subpath vertex from light subpath vertex using eye walk strategy.
        float light_prev_vertex_pdf_bwd = Pdf_ConvertSolidAngleToArea(Bxdf_GetPdf(&light_dg, light_wo, light_wi, TEXTURE_ARGS),
            my_prev_light_vertex->position, light_dg.p, light_dg.n);

        // We need sum of all weights
        float weights_sum = (eye_vertex_pdf_bwd / eye_vertex_pdf_fwd);
        float weights_eye = (eye_vertex_pdf_bwd / eye_vertex_pdf_fwd) * (eye_prev_vertex_pdf_bwd / my_prev_eye_vertex->pdf_forward);
        weights_sum += weights_eye;

        weights_sum += (light_vertex_pdf_bwd / light_vertex_pdf_fwd);
        float weights_light = (light_vertex_pdf_bwd / light_vertex_pdf_fwd) * (light_prev_vertex_pdf_bwd / my_prev_light_vertex->pdf_forward);
        weights_sum += weights_light;

        // Iterate eye subpath
        // (we already accounted for prev vertex, so start from one before, that's why -2)
        for (int i = eye_vertex_index - 2; i >= 0; --i)
        {
            GLOBAL PathVertex const* v = &eye_subpath[BDPT_MAX_SUBPATH_LEN * pixel_idx + i];
            weights_eye *= (v->pdf_backward / v->pdf_forward);
            weights_sum += weights_eye;
        }

        // Iterate light subpath
        // (we already accounted for prev vertex, so start from one before, that's why -2)
        for (int i = light_vertex_index - 2; i >= 0; --i)
        {
            GLOBAL PathVertex const* v = &light_subpath[BDPT_MAX_SUBPATH_LEN * global_id + i];
            weights_light *= (v->pdf_backward / v->pdf_forward);
            weights_sum += weights_light; 
        }

        float mis_weight = 1.f / (1.f + weights_sum);
        contributions[global_id].xyz = REASONABLE_RADIANCE(mis_weight * (eye_contribution * light_contribution) / (dist * dist));


        float3 connect_ray_o = eye_dg.p + CRAZY_LOW_DISTANCE * eye_dg.n;
        float3 temp = light_dg.p - connect_ray_o;
        float  connect_ray_length = 0.999f * length(temp);
        float3 connect_ray_dir = eye_wo;

        Ray_Init(&connection_rays[global_id], connect_ray_o, connect_ray_dir, connect_ray_length, 0.f, 0xFFFFFFFF);
    }


// Generate pixel indices withing a tile
KERNEL void GenerateTileDomain(
    // Output buffer width
    int output_width,
    // Output buffer height
    int output_height,
    // Tile offset within an output buffer
    int tile_offset_x,
    int tile_offset_y,
    // Tile size
    int tile_width,
    int tile_height,
    // Subtile size (tile can be organized in subtiles)
    int subtile_width,
    int subtile_height,
    // Pixel indices
    GLOBAL int* restrict pixel_idx0,
    GLOBAL int* restrict pixel_idx1,
    // Pixel count
    GLOBAL int* restrict count
)
{
    int tile_x = get_global_id(0);
    int tile_y = get_global_id(1);
    int tile_start_idx = output_width * tile_offset_y + tile_offset_x;

    if (tile_x < tile_width && tile_y < tile_height)
    {
        // TODO: implement subtile support
        int idx = tile_start_idx + tile_y * output_width + tile_x;
        pixel_idx0[tile_y * tile_width + tile_x] = idx;
        pixel_idx1[tile_y * tile_width + tile_x] = idx; 
    }

    if (tile_x == 0 && tile_y == 0)
    {
        *count = tile_width * tile_height;
    }
}

///< Restore pixel indices after compaction
KERNEL void FilterPathStream(
    GLOBAL Intersection const* restrict intersections,
    // Number of compacted indices
    GLOBAL int const* restrict num_items,
    GLOBAL int const* restrict pixel_indices,
    GLOBAL Path* restrict paths,
    // 1 or 0 for each item (include or exclude)
    GLOBAL int* restrict predicate
)
{
    int global_id = get_global_id(0);

    // Handle only working subset
    if (global_id < *num_items)
    {
        int pixelidx = pixel_indices[global_id];

        GLOBAL Path* path = paths + pixelidx;

        if (Path_IsAlive(path))
        {
            bool kill = (length(Path_GetThroughput(path)) < CRAZY_LOW_THROUGHPUT);

            if (!kill)
            {
                predicate[global_id] = intersections[global_id].shapeid >= 0 ? 1 : 0;
            }
            else
            {
                Path_Kill(path);
                predicate[global_id] = 0;
            }
        }
        else
        {
            predicate[global_id] = 0;
        }
    }
}

KERNEL void IncrementSampleCounter(
    // Number of rays
    int num_rays,
    GLOBAL int const* restrict pixel_indices,
    // Radiance sample buffer
    GLOBAL float4* restrict output
)
{
    int global_id = get_global_id(0);

    if (global_id < num_rays)
    {
        int pixel_idx = pixel_indices[global_id];
        // Start collecting samples
        {
            output[pixel_idx].w += 1.f;
        }
    }
}

KERNEL void GatherContributions(
    // Number of rays
    int num_rays,
    GLOBAL int const* restrict pixel_indices,
    // Shadow rays hits
    GLOBAL int const* restrict shadow_hits,
    // Light samples
    GLOBAL float3 const* restrict contributions,
    // Radiance sample buffer
    GLOBAL float4* output
)
{
    int global_id = get_global_id(0);

    if (global_id < num_rays)
    {
        int pixel_idx = pixel_indices[global_id];
        // Prepare accumulator variable
        float3 radiance = make_float3(0.f, 0.f, 0.f);

        // Start collecting samples
        {
            // If shadow ray global_id't hit anything and reached skydome
            if (shadow_hits[global_id] == -1)
            {
                // Add its contribution to radiance accumulator
                radiance += contributions[global_id];
                output[pixel_idx].xyz += radiance;
            }
        }
    }
}

KERNEL void GatherCausticContributions(
    int num_items,
    int width,
    int height,
    GLOBAL int const* restrict shadow_hits,
    GLOBAL float3 const* restrict contributions,
    GLOBAL float2 const* restrict image_plane_positions,
    GLOBAL float4* restrict output
)
{
    int global_id = get_global_id(0);

    if (global_id < num_items)
    {
        // Start collecting samples
        {
            // If shadow ray didn't hit anything and reached skydome
            if (shadow_hits[global_id] == -1)
            {
                // Add its contribution to radiance accumulator
                float3 radiance = contributions[global_id];
                float2 uv = image_plane_positions[global_id];
                int x = clamp((int)(uv.x * width), 0, width - 1);
                int y = clamp((int)(uv.y * height), 0, height - 1);
                output[y * width + x].xyz += radiance;
            }
        }
    }
}

// Restore pixel indices after compaction
KERNEL void RestorePixelIndices(
    // Compacted indices
    GLOBAL int const* restrict compacted_indices,
    // Number of compacted indices
    GLOBAL int* restrict num_items,
    // Previous pixel indices
    GLOBAL int const* restrict prev_indices,
    // New pixel indices
    GLOBAL int* restrict new_indices
)
{
    int global_id = get_global_id(0);

    // Handle only working subset
    if (global_id < *num_items)
    {
        new_indices[global_id] = prev_indices[compacted_indices[global_id]];
    }
}

// Copy data to interop texture if supported
KERNEL void ApplyGammaAndCopyData(
    //
    GLOBAL float4 const* data,
    int width,
    int height,
    float gamma,
    write_only image2d_t img
)
{
    int global_id = get_global_id(0);
    int x = global_id % width;
    int y = global_id / width;

    if (x < width && y < height)
    {
        float4 v = data[global_id];
        float4 val = clamp(native_powr(v / v.w, 1.f / gamma), 0.f, 1.f);
        write_imagef(img, make_int2(x, y), val);
    }
}


INLINE void atomic_add_float(volatile __global float* addr, float val)
{
    union {
        unsigned int u32;
        float        f32;
    } next, expected, current;
    current.f32 = *addr;
    do {
        expected.f32 = current.f32;
        next.f32 = expected.f32 + val;
        current.u32 = atomic_cmpxchg((volatile __global unsigned int *)addr,
            expected.u32, next.u32);
    } while (current.u32 != expected.u32);
}

KERNEL void ConnectCaustics(
    int num_items,
    int eye_vertex_index,
    GLOBAL PathVertex const* restrict light_subpath,
    GLOBAL int const* restrict light_subpath_length,
    GLOBAL Camera const* restrict camera,
    GLOBAL Material const* restrict materials,
    TEXTURE_ARG_LIST,
    GLOBAL ray* restrict connection_rays,
    GLOBAL float4* restrict contributions,
    GLOBAL float2* restrict image_plane_positions
)
{
    int global_id = get_global_id(0);

    if (global_id >= num_items)
        return;

    if (eye_vertex_index >= light_subpath_length[global_id])
    {
        contributions[global_id].xyz = 0.f;
        Ray_SetInactive(connection_rays + global_id);
        return;
    }

    GLOBAL PathVertex const* my_light_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id + eye_vertex_index;
    GLOBAL PathVertex const* my_prev_light_vertex = light_subpath + BDPT_MAX_SUBPATH_LEN * global_id + eye_vertex_index - 1;

    DifferentialGeometry eye_dg;
    eye_dg.p = camera->p;
    eye_dg.n = camera->forward;
    eye_dg.ng = camera->forward;
    eye_dg.uv = 0.f;
    eye_dg.dpdu = GetOrthoVector(eye_dg.n);
    eye_dg.dpdv = cross(eye_dg.n, eye_dg.dpdu);
    DifferentialGeometry_CalculateTangentTransforms(&eye_dg);

    DifferentialGeometry light_dg;
    light_dg.p = my_light_vertex->position;
    light_dg.n = my_light_vertex->shading_normal;
    light_dg.ng = my_light_vertex->geometric_normal;
    light_dg.uv = my_light_vertex->uv;
    light_dg.dpdu = GetOrthoVector(light_dg.n);
    light_dg.dpdv = cross(light_dg.dpdu, light_dg.n);
    light_dg.mat = materials[my_light_vertex->material_index];
    light_dg.mat.fresnel = 1.f;
    DifferentialGeometry_CalculateTangentTransforms(&light_dg);

    float3 eye_wo = normalize(light_dg.p - eye_dg.p);
    float3 light_wi = normalize(my_prev_light_vertex->position - light_dg.p);
    float3 light_wo = -eye_wo;

    float3 light_bxdf = Bxdf_Evaluate(&light_dg, light_wi, light_wo, TEXTURE_ARGS);
    float light_pdf = Bxdf_GetPdf(&light_dg, light_wi, light_wo, TEXTURE_ARGS);
    float light_dotnl = max(dot(light_dg.n, light_wo), 0.f);

    float3 light_contribution = my_prev_light_vertex->flow * (light_bxdf) / my_light_vertex->pdf_forward;

    //float w0 = camera_pdf / (camera_pdf + light_pdf);
    //float w1 = light_pdf / (camera_pdf + light_pdf);

    float dist = length(light_dg.p - eye_dg.p);
    float3 val = (light_contribution) / (dist * dist);
    contributions[global_id].xyz = REASONABLE_RADIANCE(val);

    float  connect_ray_length = 0.99f * length(light_dg.p - eye_dg.p);
    float3 connect_ray_dir = eye_wo;
    float3 connect_ray_o = eye_dg.p;

    Ray_Init(connection_rays + global_id, connect_ray_o, connect_ray_dir, connect_ray_length, 0.f, 0xFFFFFFFF);

    ray r;
    r.o.xyz = eye_dg.p;
    r.o.w = CRAZY_HIGH_DISTANCE;
    r.d.xyz = eye_wo;

    float3 v0, v1, v2, v3;
    float2 ext = 0.5f * camera->dim;
    v0 = camera->p + camera->focal_length * camera->forward - ext.x * camera->right - ext.y * camera->up;
    v1 = camera->p + camera->focal_length * camera->forward + ext.x * camera->right - ext.y * camera->up;
    v2 = camera->p + camera->focal_length * camera->forward + ext.x * camera->right + ext.y * camera->up;
    v3 = camera->p + camera->focal_length * camera->forward - ext.x * camera->right + ext.y * camera->up;

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
    image_plane_positions[global_id] = impuv;
}



