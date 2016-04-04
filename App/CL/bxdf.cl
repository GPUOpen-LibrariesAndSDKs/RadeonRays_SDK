/**********************************************************************
 Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

 ï   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 ï   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************/
#ifndef BXDF_CL
#define BXDF_CL

#include <../App/CL/utils.cl>
#include <../App/CL/random.cl>
#include <../App/CL/texture.cl>
#include <../App/CL/payload.cl>

enum Bxdf
{
	kZero,
    kLambert,
    kIdealReflect,
    kIdealRefract,
    kMicrofacetBlinn,
    kMicrofacetBeckmann,
    kMicrofacetGGX,
    kLayered,
    kFresnelBlend,
	kMix,
	kEmissive,
	kPassthrough,
	kTranslucent
};

enum BxdfFlags
{
    kReflection = (1 << 0),
    kTransmission = (1 << 1),
    kDiffuse = (1 << 2),
    kSpecular = (1 << 3),
    kGlossy = (1 << 4),
    kAllReflection = kReflection | kDiffuse | kSpecular | kGlossy,
    kAllTransmission = kTransmission | kDiffuse | kSpecular | kGlossy,
    kAll = kReflection | kTransmission | kDiffuse | kSpecular | kGlossy
};


/// Schlick's approximation of Fresnel equtions
float SchlickFresnel(float eta, float ndotw)
{
    const float f = ((1.f - eta) / (1.f + eta)) * ((1.f - eta) / (1.f + eta));
    const float m = 1.f - fabs(ndotw);
    const float m2 = m*m;
    return f + (1.f - f) * m2 * m2 * m;
}

/// Full Fresnel equations
float FresnelDielectric(float etai, float etat, float ndotwi, float ndotwt)
{
       // Parallel and perpendicular polarization
       float rparl = ((etat * ndotwi) - (etai * ndotwt)) / ((etat * ndotwi) + (etai * ndotwt));
       float rperp = ((etai * ndotwi) - (etat * ndotwt)) / ((etai * ndotwi) + (etat * ndotwt));
       return (rparl*rparl + rperp*rperp) * 0.5f;
}

/*
 Microfacet Beckmann
 */

// Distribution fucntion
float MicrofacetDistribution_Beckmann_D(float roughness, float3 m, float3 n)
{
    float ndotm = dot(m, n);

    if (ndotm <= 0.f)
        return 0.f;

    float ndotm2 = ndotm * ndotm;
    float sinmn = native_sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f));
    float tanmn = sinmn / ndotm;
    float a2 = roughness * roughness;

    return (1.f / (PI * a2 * ndotm2 * ndotm2)) * native_exp(-tanmn * tanmn / a2);
}

// PDF of the given direction
float MicrofacetDistribution_Beckmann_GetPdf(
    // Rougness
    float roughness,
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    // We need to convert pdf(wh)->pdf(wo)
    float3 m = normalize(wi + wo);
    float wodotm = dot(wo, m);

    if (wodotm <= 0.f)
        return 0.f;

    //
    float mpdf = MicrofacetDistribution_Beckmann_D(roughness, m, dg->n) * fabs(dot(dg->n, m));
    // See Humphreys and Pharr for derivation

    return mpdf / (4.f * wodotm);
}

// Sample the distribution
void MicrofacetDistribution_Beckmann_Sample(// Roughness
    float roughness,
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
	// Sample
	float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
    )
{
	float r1 = sample.x;
	float r2 = sample.y;

    // Sample halfway vector first, then reflect wi around that
    float temp = atan(native_sqrt(-roughness*roughness*native_log(1.f - r1*0.99f)));
    float theta = (float)((temp >= 0) ? temp : (temp + 2 * PI));

    float costheta = native_cos(theta);
    float sintheta = native_sqrt(1.f - clamp(costheta * costheta, 0.f, 1.f));

    // phi = 2*PI*ksi2
    float cosphi = native_cos(2.f*PI*r2);
    float sinphi = native_sqrt(1.f - clamp(cosphi * cosphi, 0.f, 1.f));

    // Calculate wh
    float3 wh = normalize(dg->dpdu * sintheta * cosphi + dg->dpdv * sintheta * sinphi + dg->n * costheta);

    // Reflect wi around wh
    *wo = -wi + 2.f*dot(wi, wh) * wh;

    // Calc pdf
    *pdf = MicrofacetDistribution_Beckmann_GetPdf(roughness, dg, wi, *wo, TEXTURE_ARGS);
}

float MicrofacetDistribution_Beckmann_G1(float roughness, float3 v, float3 m, float3 n)
{
    float ndotv = dot(n, v);
    float mdotv = dot(m, v);

    if (ndotv * mdotv <= 0.f)
        return 0.f;

    float sinnv = native_sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f));
    float tannv = sinnv / ndotv;
    float a = tannv > 0.f ? 1.f / (roughness * tannv) : 0.f;
    float a2 = a * a;

    if (mdotv * ndotv <= 0 || a < 1.6f)
        return 1.f;

    return (3.535f * a + 2.181f * a2) / (1.f + 2.276f * a + 2.577f * a2);
}

// Shadowing function also depends on microfacet distribution
float MicrofacetDistribution_Beckmann_G(float roughness, float3 wi, float3 wo, float3 wh, float3 n)
{
    return MicrofacetDistribution_Beckmann_G1(roughness, wi, wh, n) * MicrofacetDistribution_Beckmann_G1(roughness, wo, wh, n);
}


float3 MicrofacetBeckmann_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    const float3 ks = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));
    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));
    const float eta = dg->mat.ni;


    // Incident and reflected zenith angles
    float costhetao = dot(dg->n, wo);
    float costhetai = dot(dg->n, wi);

    if (costhetai == 0.f || costhetao == 0.f)
        return 0.f;

    // Calc halfway vector
    float3 wh = normalize(wi + wo);

	float F = dg->mat.fresnel;

    // F(eta) * D * G * ks / (4 * cosa * cosi)
    return F * ks * MicrofacetDistribution_Beckmann_G(roughness, wi, wo, wh, dg->n) * MicrofacetDistribution_Beckmann_D(roughness, wh, dg->n) / (4.f * costhetao * costhetai);
}


float MicrofacetBeckmann_GetPdf(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));
    return MicrofacetDistribution_Beckmann_GetPdf(roughness, dg, wi, wo, TEXTURE_ARGS);
}

float3 MicrofacetBeckmann_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
	// Sample
	float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
    )
{
	float ndotwi = dot(dg->n, wi);
    if ( ndotwi <= 0.f)
    {
        *pdf = 0.f;
        return 0.f;
    }

    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));
    MicrofacetDistribution_Beckmann_Sample(roughness, dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    return MicrofacetBeckmann_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
}


/*
 Microfacet GGX
 */
// Distribution fucntion
float MicrofacetDistribution_GGX_D(float roughness, float3 m, float3 n)
{
    float ndotm = dot(m, n);
    float ndotm2 = ndotm * ndotm;
    float sinmn = native_sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f));
    float tanmn = sinmn / ndotm;
    float a2 = roughness * roughness;

    return (a2 / (PI * ndotm2 * ndotm2 * (a2 + tanmn * tanmn) * (a2 + tanmn * tanmn)));
}

// PDF of the given direction
float MicrofacetDistribution_GGX_GetPdf(
    // Rougness
    float roughness,
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    // We need to convert pdf(wh)->pdf(wo)
    float3 m = normalize(wi + wo);
    //
    float mpdf = MicrofacetDistribution_GGX_D(roughness, m, dg->n) * dot(dg->n, m);
    // See Humphreys and Pharr for derivation
    return mpdf / (4.f * dot(wo, m));
}

// Sample the distribution
void MicrofacetDistribution_GGX_Sample(// Roughness
    float roughness,
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
	// Sample
	float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
    )
{
	float r1 = sample.x;
	float r2 = sample.y;

    // Sample halfway vector first, then reflect wi around that
    float temp = atan(roughness * native_sqrt(r1) / native_sqrt(1.f - r1));
    float theta = (float)((temp >= 0) ? temp : (temp + 2 * PI));

    float costheta = native_cos(theta);
    float sintheta = native_sqrt(1.f - clamp(costheta * costheta, 0.f, 1.f));

    // phi = 2*PI*ksi2
    float cosphi = native_cos(2.f*PI*r2);
    float sinphi = native_sqrt(1.f - clamp(cosphi * cosphi, 0.f, 1.f));

    // Calculate wh
    float3 wh = normalize(dg->dpdu * sintheta * cosphi + dg->dpdv * sintheta * sinphi + dg->n * costheta);

    // Reflect wi around wh
    *wo = -wi + 2.f*dot(wi, wh) * wh;

    // Calc pdf
    *pdf = MicrofacetDistribution_GGX_GetPdf(roughness, dg, wi, *wo, TEXTURE_ARGS);
}

//
float MicrofacetDistribution_GGX_G1(float roughness, float3 v, float3 m, float3 n)
{
    float ndotv = dot(n, v);
    float mdotv = dot(m, v);

    if (ndotv * mdotv <= 0.f)
        return 0.f;

    float sinnv = native_sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f));
    float tannv = sinnv / ndotv;
    float a2 = roughness * roughness;
    return 2.f / (1.f + native_sqrt(1.f + a2 * tannv * tannv));
}

// Shadowing function also depends on microfacet distribution
float MicrofacetDistribution_GGX_G(float roughness, float3 wi, float3 wo, float3 wh, float3 n)
{
    return MicrofacetDistribution_GGX_G1(roughness, wi, wh, n) * MicrofacetDistribution_GGX_G1(roughness, wo, wh, n);
}

float3 MicrofacetGGX_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    const float3 ks = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));
    const float eta = dg->mat.ni;
    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));

    // Incident and reflected zenith angles
    float costhetao = dot(dg->n, wo);
    float costhetai = dot(dg->n, wi);

    if (costhetai == 0.f || costhetao == 0.f)
        return 0.f;

    // Calc halfway vector
    float3 wh = normalize(wi + wo);

	float F = dg->mat.fresnel;

    // F(eta) * D * G * ks / (4 * cosa * cosi)
    return F * ks * MicrofacetDistribution_GGX_G(roughness, wi, wo, wh, dg->n) * MicrofacetDistribution_GGX_D(roughness, wh, dg->n) / (4.f * costhetao * costhetai);
}


float MicrofacetGGX_GetPdf(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));
    return MicrofacetDistribution_GGX_GetPdf(roughness, dg, wi, wo, TEXTURE_ARGS);
}

float3 MicrofacetGGX_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
	// Sample
	float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
    )
{
    if (dot(dg->n, wi) <= 0.f)
    {
        *pdf = 0.f;
        return 0.f;
    }

    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));
    MicrofacetDistribution_GGX_Sample(roughness, dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    return MicrofacetGGX_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
}


/*
 Microfacet Blinn
 */

// Distribution fucntion
float MicrofacetDistribution_Blinn_D(float shininess, float3 w, float3 n)
{
    float ndotw = fabs(dot(n, w));
    return (1.f / (2 * PI)) * (shininess + 2) * native_powr(ndotw, shininess);
}

// PDF of the given direction
float MicrofacetDistribution_Blinn_GetPdf(// Shininess
    float shininess,
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    // We need to convert pdf(wh)->pdf(wo)
    float3 wh = normalize(wi + wo);
    // costheta
    float ndotwh = dot(dg->n, wh);
    // See Humphreys and Pharr for derivation
    return ((shininess + 1.f) * native_powr(ndotwh, shininess)) / (2.f * PI * 4.f * dot(wo, wh));
}


// Sample the distribution
void MicrofacetDistribution_Blinn_Sample(// Shininess param
    float shininess,
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
	// Sample
	float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
    )
{
    //
	float r1 = sample.x;
	float r2 = sample.y;

    // Sample halfway vector first, then reflect wi around that
    float costheta = native_powr(r1, 1.f / (shininess + 1.f));
    float sintheta = native_sqrt(1.f - costheta * costheta);

    // phi = 2*PI*ksi2
    float cosphi = native_cos(2.f*PI*r2);
    float sinphi = native_sqrt(1.f - cosphi * cosphi);

    // Calculate wh
    float3 wh = normalize(dg->dpdu * sintheta * cosphi + dg->dpdv * sintheta * sinphi + dg->n * costheta);

    // Reflect wi around wh
    *wo = -wi + 2.f*dot(wi, wh) * wh;

    // Calc pdf
    *pdf = MicrofacetDistribution_Blinn_GetPdf(shininess, dg, wi, *wo, TEXTURE_ARGS);
}


// Shadowing function also depends on microfacet distribution
float MicrofacetDistribution_Blinn_G(float3 wi, float3 wo, float3 wh, float3 n)
{
    float ndotwh = fabs(dot(n, wh));
    float ndotwo = fabs(dot(n, wo));
    float ndotwi = fabs(dot(n, wi));
    float wodotwh = fabs(dot(wo, wh));

    return min(1.f, min(2.f * ndotwh * ndotwo / wodotwh, 2.f * ndotwh * ndotwi / wodotwh));
}

/// Lambert BRDF evaluation
float3 MicrofacetBlinn_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    const float3 ks = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));
    const float eta = dg->mat.ni;
    const float shininess = dg->mat.ns;

    // Incident and reflected zenith angles
    float costhetao = dot(dg->n, wo);
    float costhetai = dot(dg->n, wi);

    if (costhetai == 0.f || costhetao == 0.f)
        return 0.f;

    // Calc halfway vector
    float3 wh = normalize(wi + wo);

	float F = dg->mat.fresnel;

    // F(eta) * D * G * ks / (4 * cosa * cosi)
    return F * ks * MicrofacetDistribution_Blinn_G(wi, wo, wh, dg->n) * MicrofacetDistribution_Blinn_D(shininess, wh, dg->n) / (4.f * costhetao * costhetai);
}

/// Lambert BRDF PDF
float MicrofacetBlinn_GetPdf(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    const float shininess = dg->mat.ns;
    return MicrofacetDistribution_Blinn_GetPdf(shininess, dg, wi, wo, TEXTURE_ARGS);
}

/// Lambert BRDF sampling
float3 MicrofacetBlinn_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
    )
{
    if (dot(dg->n, wi) <= 0.f)
    {
        *pdf = 0.f;
        return 0.f;
    }

    const float shininess = dg->mat.ns;
    MicrofacetDistribution_Blinn_Sample(shininess, dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    return MicrofacetBlinn_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
}

/*
 Lambert BRDF
 */
/// Lambert BRDF evaluation
float3 Lambert_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    const float3 kd = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));

	float F = dg->mat.fresnel;

    return F * kd / PI;
}

/// Lambert BRDF PDF
float Lambert_GetPdf(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    return fabs(dot(dg->n, wo)) / PI;
}

/// Lambert BRDF sampling
float3 Lambert_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
    )
{
    const float3 kd = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));

    *wo = Sample_MapToHemisphere(sample, dg->n, 1.f);

	float F = dg->mat.fresnel;

    *pdf = fabs(dot(dg->n, *wo)) / PI;

    return F * kd / PI;
}

/*
 Ideal reflection BRDF
 */
float3 IdealReflect_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    return 0.f;
}

/// Lambert BRDF sampling
float3 Translucent_Sample(
	// Geometry
	DifferentialGeometry const* dg,
	// Incoming direction
	float3 wi,
	// Texture args
	TEXTURE_ARG_LIST,
	// Sample
	float2 sample,
	// Outgoing  direction
	float3* wo,
	// PDF at wo
	float* pdf
	)
{
	const float3 kd = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));

	float ndotwi = dot(dg->n, wi);

	float3 n = ndotwi > 0.f ? -dg->n : dg->n;

	*wo = normalize(Sample_MapToHemisphere(sample, n, 1.f));

	*pdf = fabs(dot(n, *wo)) / PI;

	return kd / PI;
}

// Lambert BRDF evaluation
float3 Translucent_Evaluate(
	// Geometry
	DifferentialGeometry const* dg,
	// Incoming direction
	float3 wi,
	// Outgoing direction
	float3 wo,
	// Texture args
	TEXTURE_ARG_LIST
	)
{
	const float3 kd = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));

	float ndotwi = dot(dg->n, wi);
	float ndotwo = dot(dg->n, wo);

	if (ndotwi * ndotwo > 0)
		return 0.f;

	return kd / PI;
}

/// Lambert BRDF PDF
float Translucent_GetPdf(
	// Geometry
	DifferentialGeometry const* dg,
	// Incoming direction
	float3 wi,
	// Outgoing direction
	float3 wo,
	// Texture args
	TEXTURE_ARG_LIST
	)
{
	float ndotwi = dot(dg->n, wi);
	float ndotwo = dot(dg->n, wo);

	if (ndotwi * ndotwo > 0)
		return 0.f;

	return fabs(ndotwo) / PI;
}

float IdealReflect_GetPdf(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    return 0.f;
}

float3 IdealReflect_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
    )
{
    const float3 ks = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));
    const float eta = dg->mat.ni;

    float ndotwi = dot(dg->n, wi);

    // Mirror reflect wi
    *wo = normalize(2.f * ndotwi * dg->n - wi);

    // PDF is infinite at that point, but deltas are going to cancel out while evaluating
    // so set it to 1.f
    *pdf = 1.f;

	float F = dg->mat.fresnel;

	float coswo = fabs(dot(dg->n, *wo));

    // Return reflectance value
    return coswo > 0.0001f ? (F * ks * (1.f / coswo)) : 0.f;
}

/*
 Ideal refraction BTDF
 */

float3 IdealRefract_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    return 0.f;
}

float IdealRefract_GetPdf(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    return 0.f;
}

float3 IdealRefract_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at wo
    float* pdf
    )
{
    const float3 ks = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));
	
	float etai = 1.f;
	float etat = dg->mat.ni;
    float cosi = dot(dg->n, wi);
 
	bool entering = cosi > 0.f;
	float3 n = dg->n;

    // Revert normal and eta if needed
    if (!entering)
    {
		float tmp = etai;
		etai = etat;
		etat = tmp;
		n = -dg->n;
		cosi = -cosi;
    }

	float eta = etai / etat;
	float sini2 = 1.f - cosi * cosi;
	
	float sint2 = eta * eta * sini2;

	if (sint2 >= 1.f)
	{
		*pdf = 0.f;
		return 0.f;
	}

	float cost = native_sqrt(max(0.f, 1.f - sint2));

    // Transmitted ray
	float F = dg->mat.fresnel;

	*wo = normalize(-n * cost + normalize(n * cosi - wi) * native_sqrt(max(sint2, 0.f)));

    // PDF is infinite at that point, but deltas are going to cancel out while evaluating
    // so set it to 1.f
    *pdf = 1.f;

	return cost > 0.0001f ? F * (((etai * etai) / (etat * etat)) * ks / cost) : 0.f;
}

float3 Passthrough_Sample(
	// Geometry
	DifferentialGeometry const* dg,
	// Incoming direction
	float3 wi,
	// Texture args
	TEXTURE_ARG_LIST,
	// Sample
	float2 sample,
	// Outgoing  direction
	float3* wo,
	// PDF at wo
	float* pdf
	)
{

	*wo = -wi;
	float coswo = fabs(dot(dg->n, *wo));

	// PDF is infinite at that point, but deltas are going to cancel out while evaluating
	// so set it to 1.f
	*pdf = 1.f;

	// 
	return coswo > 0.0001f ? (1.f / coswo) : 0.f;
}

/*
 Dispatch functions
 */
float3 Bxdf_Evaluate(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    int mattype = dg->mat.type;
    switch (mattype)
    {
    case kLambert:
        return Lambert_Evaluate(dg, wi, wo, TEXTURE_ARGS);
    case kMicrofacetBlinn:
        return MicrofacetBlinn_Evaluate(dg, wi, wo, TEXTURE_ARGS);
    case kMicrofacetGGX:
        return MicrofacetGGX_Evaluate(dg, wi, wo, TEXTURE_ARGS);
    case kMicrofacetBeckmann:
        return MicrofacetBeckmann_Evaluate(dg, wi, wo, TEXTURE_ARGS);
    case kIdealReflect:
        return IdealReflect_Evaluate(dg, wi, wo, TEXTURE_ARGS);
    case kIdealRefract:
        return IdealRefract_Evaluate(dg, wi, wo, TEXTURE_ARGS);
	case kTranslucent:
		return Translucent_Evaluate(dg, wi, wo, TEXTURE_ARGS);
    }

    return 0.f;
}

float3 Bxdf_Sample(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Texture args
    TEXTURE_ARG_LIST,
    // RNG
    float2 sample,
    // Outgoing  direction
    float3* wo,
    // PDF at w
    float* pdf
    )
{
    int mattype = dg->mat.type;
    switch (mattype)
    {
    case kLambert:
        return Lambert_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    case kMicrofacetBlinn:
        return MicrofacetBlinn_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    case kMicrofacetGGX:
        return MicrofacetGGX_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    case kMicrofacetBeckmann:
        return MicrofacetBeckmann_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    case kIdealReflect:
        return IdealReflect_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    case kIdealRefract:
        return IdealRefract_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
	case kTranslucent:
		return Translucent_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
	case kPassthrough:
		return Passthrough_Sample(dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    
    }

    *pdf = 0.f;
    return make_float3(0.f, 0.f, 0.f);
}

float Bxdf_GetPdf(
    // Geometry
    DifferentialGeometry const* dg,
    // Incoming direction
    float3 wi,
    // Outgoing direction
    float3 wo,
    // Texture args
    TEXTURE_ARG_LIST
    )
{
    int mattype = dg->mat.type;
    switch (mattype)
    {
    case kLambert:
        return Lambert_GetPdf(dg, wi, wo, TEXTURE_ARGS);
    case kMicrofacetBlinn:
        return MicrofacetBlinn_GetPdf(dg, wi, wo, TEXTURE_ARGS);
    case kMicrofacetGGX:
        return MicrofacetGGX_GetPdf(dg, wi, wo, TEXTURE_ARGS);
    case kMicrofacetBeckmann:
        return MicrofacetBeckmann_GetPdf(dg, wi, wo, TEXTURE_ARGS);
    case kIdealReflect:
        return IdealReflect_GetPdf(dg, wi, wo, TEXTURE_ARGS);
    case kIdealRefract:
        return IdealRefract_GetPdf(dg, wi, wo, TEXTURE_ARGS);
	case kTranslucent:
		return Translucent_GetPdf(dg, wi, wo, TEXTURE_ARGS);
	case kPassthrough:
		return 0.f;
    }

	return 0.f;
}

/// Emissive BRDF sampling
float3 Emissive_GetLe(
	// Geometry
	DifferentialGeometry const* dg,
	// Texture args
	TEXTURE_ARG_LIST)
{
	const float3 kd = Texture_GetValue3f(dg->mat.kx.xyz, dg->uv, TEXTURE_ARGS_IDX(dg->mat.kxmapidx));
	return kd;
}


/// BxDF singularity check
bool Bxdf_IsSingular(DifferentialGeometry const* dg)
{
    return dg->mat.type == kIdealReflect || dg->mat.type == kIdealRefract || dg->mat.type == kPassthrough;
}

/// BxDF emission check
bool Bxdf_IsEmissive(DifferentialGeometry const* dg)
{
	return dg->mat.type == kEmissive;
}

/// BxDF singularity check
bool Bxdf_IsBtdf(DifferentialGeometry const* dg)
{
	return dg->mat.type == kIdealRefract || dg->mat.type == kPassthrough || dg->mat.type == kTranslucent;
}

#endif // BXDF_CL
