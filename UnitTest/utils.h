#pragma once
#include "radeon_rays.h"

struct TestShape
{
    std::vector<float> positions;
    std::vector<int> indices;
    std::vector<int> faceverts;
    RadeonRays::Shape* shape; //scene id
    
    TestShape(float const * vertices, int vnum,
        // Index data for vertices
        int const * index, int innum,
        // Numbers of vertices per face
        int const * numfacevertices,
        // Number of faces
        int  numface)
        : shape(nullptr)
    {
        positions.assign(vertices, vertices + 3*vnum);
        indices.assign(index, index + innum);
        if (numfacevertices)
            faceverts.assign(numfacevertices, numfacevertices + numface);
    }
    TestShape() : shape(nullptr) {};
};

// test functions, perform tests using unoptimised CPU for validating the fast paths
void TestOcclusions(const TestShape* shapes, int numshapes, RadeonRays::ray const * rays, int numrays, bool* hits);
void TestIntersections(const TestShape* shapes, int numshapes, RadeonRays::ray const * rays, int numrays, RadeonRays::Intersection* results);
