#include "radiance_cache.h"
#include "hlbvh.h"

namespace Baikal
{


    struct RadianceCache::GpuData
    {
        CLWProgram program;

        CLWBuffer<RadianceProbeDesc> descs;
        CLWBuffer<RadianceProbeData> probes;
        std::size_t num_probes;
    };

    // Constructor
    RadianceCache::RadianceCache(CLWContext context, std::size_t max_probes)
        : m_gpudata(new GpuData)
        , m_accel(new Hlbvh(context))
        , m_context(context)
    {
        std::string buildopts;

        buildopts.append(" -cl-mad-enable -cl-fast-relaxed-math -cl-std=CL1.2 -I . ");

        buildopts.append(
#if defined(__APPLE__)
            "-D APPLE "
#elif defined(_WIN32) || defined (WIN32)
            "-D WIN32 "
#elif defined(__linux__)
            "-D __linux__ "
#else
            ""
#endif
        );

        // Load kernels
#ifndef RR_EMBED_KERNELS
        m_gpudata->program = CLWProgram::CreateFromFile("../App/CL/radiance_cache.cl", buildopts.c_str(), m_context);
#else
        m_gpudata->program = CLWProgram::CreateFromSource(cl_radiance_cache, std::strlen(cl_radiance_cache), buildopts.c_str(), context);
#endif

        m_gpudata->descs = context.CreateBuffer<RadianceProbeDesc>(max_probes, CL_MEM_READ_WRITE);
        m_gpudata->probes = context.CreateBuffer<RadianceProbeData>(max_probes, CL_MEM_READ_WRITE);
        m_gpudata->num_probes = 0;
    }

    // Destructor
    RadianceCache::~RadianceCache()
    {

    }

    // Add radiance probes into internal buffer
    void RadianceCache::AttachProbes(CLWBuffer<RadianceProbeDesc> descs,
        CLWBuffer<RadianceProbeData> probes, std::size_t num_probes)
    {
        if (m_gpudata->num_probes + num_probes > m_gpudata->descs.GetElementCount())
        {
            throw std::runtime_error("Maximum number of probes exceeded");
        }

        m_context.CopyBuffer(0, descs, m_gpudata->descs, 0, m_gpudata->num_probes, num_probes).Wait();
        m_context.CopyBuffer(0, probes, m_gpudata->probes, 0, m_gpudata->num_probes, num_probes).Wait();


        std::vector<RadianceProbeData> temp(num_probes);
        m_context.ReadBuffer(0, m_gpudata->probes, &temp[0], num_probes).Wait();

        m_accel->Build(m_gpudata->descs, num_probes);

        m_gpudata->num_probes += num_probes;
    }

    // Add radiance samples
    void RadianceCache::AddRadianceSamples(
        CLWBuffer<RadeonRays::ray> rays,
        CLWBuffer<int> predicates,
        CLWBuffer<RadeonRays::float3> samples,
        int num_rays)
    {
        auto kernel = m_gpudata->program.GetKernel("AddRadianceSamples");

        // Set kernel parameters
        int argc = 0;
        kernel.SetArg(argc++, rays);
        kernel.SetArg(argc++, predicates);
        kernel.SetArg(argc++, samples);
        kernel.SetArg(argc++, sizeof(int), &num_rays);
        kernel.SetArg(argc++, m_accel->GetGpuData().nodes);
        kernel.SetArg(argc++, m_accel->GetGpuData().sorted_bounds);
        kernel.SetArg(argc++, m_gpudata->descs);
        kernel.SetArg(argc++, m_gpudata->probes);

        // Run shading kernel
        {
            int globalsize = num_rays;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, kernel);
        }
    }

#ifdef RADIANCE_PROBE_DIRECT
    // Add radiance samples
    void RadianceCache::AddDirectRadianceSamples(
        CLWBuffer<RadeonRays::ray> rays,
        CLWBuffer<int> predicates,
        CLWBuffer<RadeonRays::float3> samples,
        CLWBuffer<int> num_rays,
        int max_rays)
    {
        auto kernel = m_gpudata->program.GetKernel("AddDirectRadianceSamples");

        // Set kernel parameters
        int argc = 0;
        kernel.SetArg(argc++, rays);
        kernel.SetArg(argc++, predicates);
        kernel.SetArg(argc++, samples);
        kernel.SetArg(argc++, num_rays);
        kernel.SetArg(argc++, m_accel->GetGpuData().nodes);
        kernel.SetArg(argc++, m_accel->GetGpuData().sorted_bounds);
        kernel.SetArg(argc++, m_gpudata->descs);
        kernel.SetArg(argc++, m_gpudata->probes);

        // Run shading kernel
        {
            int globalsize = max_rays;
            m_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, kernel);
        }
    }
#endif

    void RadianceCache::Refit()
    {
        m_accel->Build(m_gpudata->descs, m_gpudata->num_probes);
    }

    // Get acceleration structure buffer
    Hlbvh& RadianceCache::GetAccel() const
    {
        return *m_accel;
    }

    //
    CLWBuffer<RadianceCache::RadianceProbeDesc> RadianceCache::GetProbeDescs() const
    {
        return m_gpudata->descs;
    }

    //
    CLWBuffer<RadianceCache::RadianceProbeData> RadianceCache::GetProbes() const
    {
        return m_gpudata->probes;
    }
}