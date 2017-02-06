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
#pragma once

#include "math/float3.h"
#include "math/matrix.h"
#include "math/ray.h"

#include "CLW.h"

#include <memory>

namespace Baikal
{
    class Hlbvh;

    class RadianceCache
    {
    public:
        struct RadianceProbeDesc;
        struct RadianceProbeData;

        // Constructor
        RadianceCache(CLWContext context, std::size_t max_probes);
        // Destructor
        virtual ~RadianceCache();

        // Add radiance probes into internal buffer
        void AttachProbes(CLWBuffer<RadianceProbeDesc> descs, 
            CLWBuffer<RadianceProbeData> probes, std::size_t num_probes);

        // Add radiance samples
        void AddRadianceSamples(CLWBuffer<RadeonRays::ray> rays,
            CLWBuffer<int> predicates,
            CLWBuffer<RadeonRays::float3> samples,
            int num_rays);
#ifdef RADIANCE_PROBE_DIRECT
        // Add radiance samples
        void AddDirectRadianceSamples(CLWBuffer<RadeonRays::ray> rays,
            CLWBuffer<int> predicates,
            CLWBuffer<RadeonRays::float3> samples,
            CLWBuffer<int> num_rays,
            int max_rays);
#endif
        void Refit();

        // Get acceleration structure buffer
        Hlbvh& GetAccel() const;
        CLWBuffer<RadianceProbeDesc> GetProbeDescs() const;
        CLWBuffer<RadianceProbeData> GetProbes() const;

    private:

        struct GpuData;
        std::unique_ptr<GpuData> m_gpudata;
        std::unique_ptr<Hlbvh> m_accel;

        CLWContext m_context;
    };

    struct RadianceCache::RadianceProbeDesc
    {
        RadeonRays::matrix world_to_tangent;
        RadeonRays::matrix tangent_to_world;
        RadeonRays::float3 position;
        float radius;
        int num_samples;
        int atomic_update;
        int padding1;
    };

    struct RadianceCache::RadianceProbeData
    {
        RadeonRays::float3 r0;
        RadeonRays::float3 r1;
        RadeonRays::float3 r2;

        RadeonRays::float3 g0;
        RadeonRays::float3 g1;
        RadeonRays::float3 g2;

        RadeonRays::float3 b0;
        RadeonRays::float3 b1;
        RadeonRays::float3 b2;

#ifdef RADIANCE_PROBE_DIRECT
        RadeonRays::float3 dr0;
        RadeonRays::float3 dr1;
        RadeonRays::float3 dr2;

        RadeonRays::float3 dg0;
        RadeonRays::float3 dg1;
        RadeonRays::float3 dg2;

        RadeonRays::float3 db0;
        RadeonRays::float3 db1;
        RadeonRays::float3 db2;
#endif
    };

}
