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
#ifndef BXDF_CL
#define BXDF_CL

#include <../App/CL/utils.cl>
#include <../App/CL/texture.cl>
#include <../App/CL/payload.cl>

#define DENOM_EPS 0.0f
#define ROUGHNESS_EPS 0.0001f


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
float MicrofacetDistribution_Beckmann_D(float roughness, float3 m)
{
    float ndotm = fabs(m.y);

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
    float wodotm = fabs(dot(wo, m));

    if (wodotm <= 0.f)
        return 0.f;

    //
    float mpdf = MicrofacetDistribution_Beckmann_D(roughness, m) * fabs(m.y);
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
    float3 wh = make_float3(sintheta * cosphi, costheta, sintheta * sinphi);

    // Reflect wi around wh
    *wo = -wi + 2.f*fabs(dot(wi, wh)) * wh;

    // Calc pdf
    *pdf = MicrofacetDistribution_Beckmann_GetPdf(roughness, dg, wi, *wo, TEXTURE_ARGS);
}

// Sample the distribution
void MicrofacetDistribution_Beckmann_SampleNormal(// Roughness
    float roughness,
    // Geometry
    DifferentialGeometry const* dg,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wh
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

    // Reflect wi around wh
    *wh = make_float3(sintheta * cosphi, costheta, sintheta * sinphi);
}

float MicrofacetDistribution_Beckmann_G1(float roughness, float3 v, float3 m)
{
    float ndotv = fabs(v.y);
    float mdotv = fabs(dot(m, v));

    float sinnv = native_sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f));
    float tannv = sinnv / ndotv;
    float a = tannv > DENOM_EPS ? 1.f / (roughness * tannv) : 0.f;
    float a2 = a * a;

    if (a < 1.6f)
        return 1.f;

    return (3.535f * a + 2.181f * a2) / (1.f + 2.276f * a + 2.577f * a2);
}

// Shadowing function also depends on microfacet distribution
float MicrofacetDistribution_Beckmann_G(float roughness, float3 wi, float3 wo, float3 wh)
{
    return MicrofacetDistribution_Beckmann_G1(roughness, wi, wh) * MicrofacetDistribution_Beckmann_G1(roughness, wo, wh);
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
    float costhetao = fabs(wo.y);
    float costhetai = fabs(wi.y);

    // Calc halfway vector
    float3 wh = normalize(wi + wo);

    float F = dg->mat.fresnel;

    float denom = 4.f * costhetao * costhetai;

    // F(eta) * D * G * ks / (4 * cosa * cosi)
    return denom > DENOM_EPS ? F * ks * MicrofacetDistribution_Beckmann_G(roughness, wi, wo, wh) * MicrofacetDistribution_Beckmann_D(roughness, wh) / denom : 0.f;
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
    float ndotwi = fabs(wi.y);

    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));
    MicrofacetDistribution_Beckmann_Sample(roughness, dg, wi, TEXTURE_ARGS, sample, wo, pdf);
    return MicrofacetBeckmann_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
}

/*
 Microfacet GGX
 */
 // Distribution fucntion
float MicrofacetDistribution_GGX_D(float roughness, float3 m)
{
    float ndotm = fabs(m.y);
    float ndotm2 = ndotm * ndotm;
    float sinmn = native_sqrt(1.f - clamp(ndotm * ndotm, 0.f, 1.f));
    float tanmn = ndotm > DENOM_EPS ? sinmn / ndotm : 0.f;
    float a2 = roughness * roughness;
    float denom = (PI * ndotm2 * ndotm2 * (a2 + tanmn * tanmn) * (a2 + tanmn * tanmn));
    return denom > DENOM_EPS ? (a2 / denom) : 0.f;
}

// PDF of the given direction
float MicrofacetDistribution_GGX_GetPdf(
    // Halfway vector
    float3 m,
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
    float mpdf = MicrofacetDistribution_GGX_D(roughness, m) * fabs(m.y);
    // See Humphreys and Pharr for derivation
    float denom = (4.f * fabs(dot(wo, m)));

    return denom > DENOM_EPS ? mpdf / denom : 0.f;
}

// Sample the distribution
void MicrofacetDistribution_GGX_Sample(
    // Roughness
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
    float3 wh = make_float3(sintheta * cosphi, costheta, sintheta * sinphi);

    // Reflect wi around wh
    *wo = -wi + 2.f*fabs(dot(wi, wh)) * wh;

    // Calc pdf
    *pdf = MicrofacetDistribution_GGX_GetPdf(wh, roughness, dg, wi, *wo, TEXTURE_ARGS);
}

// Sample the distribution
void MicrofacetDistribution_GGX_SampleNormal(
    // Roughness
    float roughness,
    // Differential geometry
    DifferentialGeometry const* dg,
    // Texture args
    TEXTURE_ARG_LIST,
    // Sample
    float2 sample,
    // Outgoing  direction
    float3* wh
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
    *wh = make_float3(sintheta * cosphi, costheta, sintheta * sinphi);
}

//
float MicrofacetDistribution_GGX_G1(float roughness, float3 v, float3 m)
{
    float ndotv = fabs(v.y);
    float mdotv = fabs(dot(m, v));

    float sinnv = native_sqrt(1.f - clamp(ndotv * ndotv, 0.f, 1.f));
    float tannv = ndotv > 0.f ? sinnv / ndotv : 0.f;
    float a2 = roughness * roughness;
    return 2.f / (1.f + native_sqrt(1.f + a2 * tannv * tannv));
}

// Shadowing function also depends on microfacet distribution
float MicrofacetDistribution_GGX_G(float roughness, float3 wi, float3 wo, float3 wh)
{
    return MicrofacetDistribution_GGX_G1(roughness, wi, wh) * MicrofacetDistribution_GGX_G1(roughness, wo, wh);
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
    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));

    // Incident and reflected zenith angles
    float costhetao = fabs(wo.y);
    float costhetai = fabs(wi.y);

    // Calc halfway vector
    float3 wh = normalize(wi + wo);

    float F = dg->mat.fresnel;

    float denom = (4.f * costhetao * costhetai);

    // F(eta) * D * G * ks / (4 * cosa * cosi)
    return denom > 0.f ? F * ks * MicrofacetDistribution_GGX_G(roughness, wi, wo, wh) * MicrofacetDistribution_GGX_D(roughness, wh) / denom : 0.f;
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

    float3 wh = normalize(wo + wi);

    return MicrofacetDistribution_GGX_GetPdf(wh, roughness, dg, wi, wo, TEXTURE_ARGS);
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
    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));

    MicrofacetDistribution_GGX_Sample(roughness, dg, wi, TEXTURE_ARGS, sample, wo, pdf);

    return MicrofacetGGX_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
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
    return fabs(wo.y) / PI;
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

    *wo = Sample_MapToHemisphere(sample, make_float3(0.f, 1.f, 0.f) , 1.f);

    float F = dg->mat.fresnel;

    *pdf = fabs(wo->y) / PI;

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

    float ndotwi = wi.y;

    float3 n = ndotwi > DENOM_EPS ? make_float3(0.f, -1.f, 0.f) : make_float3(0.f, 1.f, 0.f);

    *wo = normalize(Sample_MapToHemisphere(sample, n, 1.f));

    *pdf = fabs(wo->y) / PI;

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

    float ndotwi = wi.y;
    float ndotwo = wo.y;

    if (ndotwi * ndotwo > 0.f)
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
    float ndotwi = wi.y;
    float ndotwo = wo.y;

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

    // Mirror reflect wi
    *wo = normalize(make_float3(-wi.x, wi.y, -wi.z));

    // PDF is infinite at that point, but deltas are going to cancel out while evaluating
    // so set it to 1.f
    *pdf = 1.f;

    float F = dg->mat.fresnel;

    float coswo = fabs(wo->y);

    // Return reflectance value
    return coswo > DENOM_EPS ? (F * ks * (1.f / coswo)) : 0.f;
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
    float cosi = wi.y;

    bool entering = cosi > 0.f;

    // Revert normal and eta if needed
    if (!entering)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
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

    *wo = normalize(make_float3(eta * -wi.x, entering ? -cost : cost, eta * -wi.z));

    *pdf = 1.f;

    return cost > DENOM_EPS ? (F * eta * eta * ks / cost) : 0.f;
}


float3 MicrofacetRefractionGGX_Evaluate(
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
    const float roughness = max(Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx)), ROUGHNESS_EPS);

    float ndotwi = wi.y;
    float ndotwo = wo.y;

    if (ndotwi * ndotwo >= 0.f)
    {
        return 0.f;
    }

    float etai = 1.f;
    float etat = dg->mat.ni;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }

    // Calc halfway vector
    float3 ht = -(etai * wi + etat * wo);
    float3 wh = normalize(ht);

    float widotwh = fabs(dot(wh, wi));
    float wodotwh = fabs(dot(wh, wo));

    float F = dg->mat.fresnel;

    float denom = dot(ht, ht);
    denom *= (fabs(ndotwi) * fabs(ndotwo));

    return denom > DENOM_EPS ? (F * ks * (widotwh * wodotwh)  * (etat)* (etat)*
        MicrofacetDistribution_GGX_G(roughness, wi, wo, wh) * MicrofacetDistribution_GGX_D(roughness, wh) / denom) : 0.f;
}



float MicrofacetRefractionGGX_GetPdf(
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
    const float roughness = max(Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx)), ROUGHNESS_EPS);
    float ndotwi = wi.y;
    float ndotwo = wo.y;

    float etai = 1.f;
    float etat = dg->mat.ni;

    if (ndotwi * ndotwo >= 0.f)
    {
        return 0.f;
    }

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }

    // Calc halfway vector
    float3 ht = -(etai * wi + etat * wo);

    float3 wh = normalize(ht);

    float wodotwh = fabs(dot(wo, wh));

    float whpdf = MicrofacetDistribution_GGX_D(roughness, wh) * fabs(wh.y);

    float whwo = wodotwh * etat * etat;

    float denom = dot(ht, ht);

    return denom > DENOM_EPS ? whpdf * whwo / denom : 0.f;
}

float3 MicrofacetRefractionGGX_Sample(
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
    const float roughness = max(Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx)), ROUGHNESS_EPS);

    float ndotwi = wi.y;

    if (ndotwi == 0.f)
    {
        *pdf = 0.f;
        return 0.f;
    }

    float etai = 1.f;
    float etat = dg->mat.ni;
    float s = 1.f;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
        s = -s;
    }

    float3 wh;
    MicrofacetDistribution_GGX_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh);

    float c = dot(wi, wh);
    float eta = etai / etat;

    float d = 1 + eta * (c * c - 1);

    if (d <= 0.f)
    {
        *pdf = 0.f;
        return 0.f;
    }

    *wo = normalize((eta * c - s * native_sqrt(d)) * wh - eta * wi);

    *pdf = MicrofacetRefractionGGX_GetPdf(dg, wi, *wo, TEXTURE_ARGS);

    return MicrofacetRefractionGGX_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
}



float3 MicrofacetRefractionBeckmann_Evaluate(
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
    const float roughness = max(Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx)), ROUGHNESS_EPS);

    float ndotwi = wi.y;
    float ndotwo = wo.y;

    float etai = 1.f;
    float etat = dg->mat.ni;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }

    // Calc halfway vector
    float3 ht = -(etai * wi + etat * wo);
    float3 wh = normalize(ht);

    float widotwh = fabs(dot(wh, wi));
    float wodotwh = fabs(dot(wh, wo));

    float F = dg->mat.fresnel;

    float denom = dot(ht, ht);
    denom *= (fabs(ndotwi) * fabs(ndotwo));

    return denom > DENOM_EPS ? (F * ks * (widotwh * wodotwh)  * (etat)* (etat)*
        MicrofacetDistribution_Beckmann_G(roughness, wi, wo, wh) * MicrofacetDistribution_Beckmann_D(roughness, wh) / denom) : 0.f;
}



float MicrofacetRefractionBeckmann_GetPdf(
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
    float ndotwi = wi.y;
    float ndotwo = wo.y;

    float etai = 1.f;
    float etat = dg->mat.ni;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
    }

    // Calc halfway vector
    float3 ht = -(etai * wi + etat * wo);

    float3 wh = normalize(ht);

    float wodotwh = fabs(dot(wo, wh));

    float whpdf = MicrofacetDistribution_Beckmann_D(roughness, wh) * fabs(wh.y);

    float whwo = wodotwh * etat * etat;

    float denom = dot(ht, ht);

    return denom > DENOM_EPS ? whpdf * whwo / denom : 0.f;
}

float3 MicrofacetRefractionBeckmann_Sample(
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
    const float roughness = Texture_GetValue1f(dg->mat.ns, dg->uv, TEXTURE_ARGS_IDX(dg->mat.nsmapidx));

    float ndotwi = wi.y;

    float etai = 1.f;
    float etat = dg->mat.ni;
    float s = 1.f;

    // Revert normal and eta if needed
    if (ndotwi < 0.f)
    {
        float tmp = etai;
        etai = etat;
        etat = tmp;
        s = -s;
    }

    float3 wh;
    MicrofacetDistribution_Beckmann_SampleNormal(roughness, dg, TEXTURE_ARGS, sample, &wh);

    float c = dot(wi, wh);
    float eta = etai / etat;

    float d = 1 + eta * (c * c - 1);

    if (d <= 0)
    {
        *pdf = 0.f;
        return 0.f;
    }

    *wo = normalize((eta * c - s * native_sqrt(d)) * wh - eta * wi);

    *pdf = MicrofacetRefractionBeckmann_GetPdf(dg, wi, *wo, TEXTURE_ARGS);

    return MicrofacetRefractionBeckmann_Evaluate(dg, wi, *wo, TEXTURE_ARGS);
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
    float coswo = fabs(wo->y);

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
    // Transform vectors into tangent space
    float3 wi_t = matrix_mul_vector3(dg->world_to_tangent, wi);
    float3 wo_t = matrix_mul_vector3(dg->world_to_tangent, wo);

    int mattype = dg->mat.type;
    switch (mattype)
    {
    case kLambert:
        return Lambert_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetGGX:
        return MicrofacetGGX_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetBeckmann:
        return MicrofacetBeckmann_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kIdealReflect:
        return IdealReflect_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kIdealRefract:
        return IdealRefract_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kTranslucent:
        return Translucent_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetRefractionGGX:
        return MicrofacetRefractionGGX_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetRefractionBeckmann:
        return MicrofacetRefractionBeckmann_Evaluate(dg, wi_t, wo_t, TEXTURE_ARGS);
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
    // Transform vectors into tangent space
    float3 wi_t = matrix_mul_vector3(dg->world_to_tangent, wi);
    float3 wo_t;

    float3 res = 0.f;

    int mattype = dg->mat.type;
    switch (mattype)
    {
    case kLambert:
        res = Lambert_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kMicrofacetGGX:
        res = MicrofacetGGX_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kMicrofacetBeckmann:
        res = MicrofacetBeckmann_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kIdealReflect:
        res = IdealReflect_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kIdealRefract:
        res = IdealRefract_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kTranslucent:
        res = Translucent_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kPassthrough:
        res = Passthrough_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kMicrofacetRefractionGGX:
        res = MicrofacetRefractionGGX_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    case kMicrofacetRefractionBeckmann:
        res = MicrofacetRefractionBeckmann_Sample(dg, wi_t, TEXTURE_ARGS, sample, &wo_t, pdf);
        break;
    default:
        *pdf = 0.f;
        break;
    }

    *wo = matrix_mul_vector3(dg->tangent_to_world, wo_t);

    return res;
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
    // Transform vectors into tangent space
    float3 wi_t = matrix_mul_vector3(dg->world_to_tangent, wi);
    float3 wo_t = matrix_mul_vector3(dg->world_to_tangent, wo);

    int mattype = dg->mat.type;
    switch (mattype)
    {
    case kLambert:
        return Lambert_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetGGX:
        return MicrofacetGGX_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetBeckmann:
        return MicrofacetBeckmann_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kIdealReflect:
        return IdealReflect_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kIdealRefract:
        return IdealRefract_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kTranslucent:
        return Translucent_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kPassthrough:
        return 0.f;
    case kMicrofacetRefractionGGX:
        return MicrofacetRefractionGGX_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
    case kMicrofacetRefractionBeckmann:
        return MicrofacetRefractionBeckmann_GetPdf(dg, wi_t, wo_t, TEXTURE_ARGS);
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
    return dg->mat.type == kIdealRefract || dg->mat.type == kPassthrough || dg->mat.type == kTranslucent ||
        dg->mat.type == kMicrofacetRefractionGGX || dg->mat.type == kMicrofacetRefractionBeckmann;
}

#endif // BXDF_CL
