#include "utils.h"
#include "RadeonRays/src/primitive/mesh.h"
#include "RadeonRays/src/primitive/instance.h"
#include <cfloat>

using namespace RadeonRays;

bool TestOcclusion(const TestShape& shape, const RadeonRays::ray& ray);
void TestIntersection(const TestShape& shape, const RadeonRays::ray& ray, RadeonRays::Intersection& results);

void TestOcclusions(const TestShape* shapes, int numshapes, RadeonRays::ray const * rays, int numrays, bool* hits)
{
    // not this is not optimised for speed!
#pragma omp parallel for
    for (int i = 0; i < numrays; ++i)
    {
        hits[i] = false;
        for (int sh_index = 0; sh_index < numshapes; ++sh_index)
        {
            hits[i] = TestOcclusion(shapes[sh_index], rays[i]);
            // any hit is enough to stop this rays cast
            if (hits[i] == true)
                break;
        }
    }
}

void TestIntersections(const TestShape* shapes, int numshapes, RadeonRays::ray const * rays, int numrays, RadeonRays::Intersection* results)
{
    // not this is not optimised for speed!
#pragma omp parallel for
    for (int i = 0; i < numrays; ++i)
    {
        results[i].uvwt.w = FLT_MAX;
        for (int sh_index = 0; sh_index < numshapes; ++sh_index)
        {
            TestIntersection(shapes[sh_index], rays[i], results[i]);
        }
    }
}
int GetTransformedFace(const TestShape& shape, int shape_index, float3* out_verts)
{
    const bool khave_faces = !shape.faceverts.empty();
    const int knum_verts = khave_faces ? shape.faceverts[shape_index] : 3; //triangles by default
    int index_pos = 0;
    if (khave_faces)
    {
        std::for_each(shape.faceverts.begin(), shape.faceverts.begin() + shape_index, [&index_pos](int vert_count) {index_pos += vert_count; });
    }
    else
        index_pos = shape_index * 3; //triangles by default

    matrix m, minv;
    shape.shape->GetTransform(m, minv);
    for (int i = 0; i < knum_verts; ++i)
    {
        int idx = shape.indices[index_pos + i];
        float3 pos;
        pos.x = shape.positions[3*idx];
        pos.y = shape.positions[3*idx + 1];
        pos.z = shape.positions[3*idx + 2];

        out_verts[i] = transform_point(pos, m);
    }
    return knum_verts;
}
bool TestOcclusion(const TestShape& shape, const RadeonRays::ray& r)
{
    const int kface_count = shape.faceverts.empty() ? (int)shape.indices.size() / 3 : (int)shape.faceverts.size();
    for (int i = 0; i < kface_count; ++i)
    {
        float3 tverts[4];
        const int knum_verts = GetTransformedFace(shape, i, tverts);

        for (int t = 0; t < knum_verts - 2; ++t)
        {
            // only tris and quad supported
            assert(t == 0 || t == 1);
            float3 v0 = tverts[0 + t];
            float3 e1 = tverts[1 + t] - v0;
            float3 e2 = tverts[2 + t] - v0;

            float3 s1 = cross(r.d, e2);
            float det = dot(s1, e1);

            float  invdet = 1.f / det;

            float3 d = r.o - v0;
            float  b1 = dot(d, s1) * invdet;

            if (b1 < 0.f || b1 > 1.f)
            {
                continue;
            }

            float3 s2 = cross(d, e1);
            float  b2 = dot(r.d, s2) * invdet;

            if (b2 < 0.f || b1 + b2 > 1.f)
            {
                continue;
            }

            float temp = dot(e2, s2) * invdet;

            if (temp > 0.f)
            {
                return true;
            }
        }

    }
    return false;
}

void TestIntersection(const TestShape& shape, const RadeonRays::ray& r, RadeonRays::Intersection& isect)
{
    const int kface_count = shape.faceverts.empty() ? (int)shape.indices.size() / 3 : (int)shape.faceverts.size();
    for (int i = 0; i < kface_count; ++i)
    {
        float3 tverts[4];
        const int knum_verts = GetTransformedFace(shape, i, tverts);

        for (int t = 0; t < knum_verts - 2; ++t)
        {
            // only tris and quad supported
            assert(t == 0 || t == 1);
            float3 v0 = tverts[0 + t];
            float3 e1 = tverts[1 + t] - v0;
            float3 e2 = tverts[2 + t] - v0;

            float3 s1 = cross(r.d, e2);
            float det = dot(s1, e1);

            float  invdet = 1.f / det;

            float3 d = r.o - v0;
            float  b1 = dot(d, s1) * invdet;

            if (b1 < 0.f || b1 > 1.f)
            {
                continue;
            }

            float3 s2 = cross(d, e1);
            float  b2 = dot(r.d, s2) * invdet;

            if (b2 < 0.f || b1 + b2 > 1.f)
            {
                continue;
            }

            float temp = dot(e2, s2) * invdet;

            if (temp > 0.f && temp < isect.uvwt.w)
            {
                isect.uvwt = float4(b1, b2, 0, temp);
                isect.shapeid = shape.shape->GetId();
                isect.primid = i + t;
            }
        }

    }
}