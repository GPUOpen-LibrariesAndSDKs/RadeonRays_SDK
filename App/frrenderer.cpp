#include "frrenderer.h"
#include "scene.h"

#include <numeric>
#include <chrono>
#include <cstddef>
#include <cstdlib>

#include "sobol.h"

#ifdef FR_EMBED_KERNELS
#include "./CL/cache/kernels.h"
#endif

using namespace FireRays;


static int const kMaxLightSamples = 1;

struct FrRenderer::QmcSampler
{
	unsigned int seq;
	unsigned int s0;
	unsigned int s1;
	unsigned int s2;
};

struct FrRenderer::PathState
{	
	float4 throughput;
	int volume;
	int flags;
	int extra0;
	int extra1;
};

struct FrRenderer::Volume
{
    int type;
    int phase_func;
    int data;
    int extra;
    
    float3 sigma_a;
    float3 sigma_s;
    float3 sigma_e;
};

struct FrRenderer::GpuData
{
    // OpenCL stuff
    CLWBuffer<ray> rays[2];
    CLWBuffer<int> hits;

    CLWBuffer<ray> shadowrays;
    CLWBuffer<int> shadowhits;

    CLWBuffer<Intersection> intersections;
    CLWBuffer<int> compacted_indices;
    CLWBuffer<int> pixelindices[2];
    CLWBuffer<int> iota;

    //CLWBuffer<int> iota;
    CLWBuffer<float3> radiance;
    CLWBuffer<float3> accumulator;
    CLWBuffer<float3> lightsamples;
    CLWBuffer<PathState> paths;
	CLWBuffer<QmcSampler> samplers;
	CLWBuffer<unsigned int> sobolmat;
    CLWBuffer<Volume> volumes;

    CLWBuffer<int> materialids;
    CLWBuffer<Scene::Emissive> emissives;
    int numemissive;
    CLWBuffer<Scene::Material> materials;

    CLWBuffer<float3> vertices;
    CLWBuffer<float3> normals;
    CLWBuffer<float2> uvs;
    CLWBuffer<int>    indices;
    CLWBuffer<Scene::Shape> shapes;

    CLWBuffer<Scene::Texture> textures;
    CLWBuffer<char> texturedata;
    CLWBuffer<int> hitcount[2];

	CLWBuffer<int> debug;

    CLWBuffer<PerspectiveCamera> camera;

    CLWProgram program;
    CLWParallelPrimitives pp;

    // FireRays stuff
    Buffer* fr_rays[2];
    Buffer* fr_shadowrays;
    Buffer* fr_shadowhits;
    Buffer* fr_hits;
    Buffer* fr_intersections;
    Buffer* fr_hitcount[2];

    GpuData()
        : fr_shadowrays(nullptr)
        , fr_shadowhits(nullptr)
        , fr_hits(nullptr)
        , fr_intersections(nullptr)
    {
        fr_rays[0] = nullptr;
        fr_rays[1] = nullptr;
    }
};



// Constructor
FrRenderer::FrRenderer(CLWContext context, int devidx)
: context_(context)
, output_(nullptr)
, gpudata_(new GpuData)
, nbounces_(0)
, vidmemusage_(0)
, vidmemws_(0)
, resetsampler_(1)
{
    // Get raw CL data out of CLW context
    cl_device_id id = context_.GetDevice(devidx).GetID();
    cl_command_queue queue = context_.GetCommandQueue(devidx);

#ifdef FR_OPENCL20
    cl_int status = CL_SUCCESS;
    cl_queue_properties prop[] = {
        CL_QUEUE_PROPERTIES, CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE | CL_QUEUE_ON_DEVICE | CL_QUEUE_ON_DEVICE_DEFAULT,
        CL_QUEUE_SIZE, 512, 0 };

    cl_command_queue devicequeue = clCreateCommandQueueWithProperties(context_, id, prop, &status);

    ThrowIf(status != CL_SUCCESS, status, "Can't create device queue\n");
#endif

    // Create intersection API
    api_ = IntersectionApiCL::CreateFromOpenClContext(context_, id, queue);

    // Do app specific settings
#ifdef __APPLE__
    // Apple runtime has known issue with stacked traversal
    api_->SetOption("acc.type", "bvh");
    api_->SetOption("bvh.builder", "sah");
#else
    api_->SetOption("acc.type", "fatbvh");
    api_->SetOption("bvh.builder", "sah");
#endif
    // Create parallel primitives
    gpudata_->pp = CLWParallelPrimitives(context_);

    // Load kernels
#ifndef FR_EMBED_KERNELS
    gpudata_->program = CLWProgram::CreateFromFile("../App/CL/integrator.cl", context_);
#else
    gpudata_->program = CLWProgram::CreateFromSource(cl_app, std::strlen(cl_integrator), context);
#endif

    // Create static buffers
    gpudata_->camera = context_.CreateBuffer<PerspectiveCamera>(1, CL_MEM_READ_ONLY);
    gpudata_->hitcount[0] = context_.CreateBuffer<int>(1, CL_MEM_READ_ONLY);
	gpudata_->hitcount[1] = context_.CreateBuffer<int>(1, CL_MEM_READ_ONLY);
    gpudata_->fr_hitcount[0] = api_->CreateFromOpenClBuffer(gpudata_->hitcount[0]);
	gpudata_->fr_hitcount[1] = api_->CreateFromOpenClBuffer(gpudata_->hitcount[1]);

    
	gpudata_->sobolmat = context_.CreateBuffer<unsigned int>(1024 * 52, CL_MEM_READ_ONLY, &g_SobolMatrices[0]);
    
    //Volume vol = {1, 0, 0, 0, {0.9f, 0.6f, 0.9f}, {5.1f, 1.8f, 5.1f}, {0.0f, 0.0f, 0.0f}};
	Volume vol = { 1, 0, 0, 0,{	1.2f, 0.4f, 1.2f },{ 5.1f, 4.8f, 5.1f },{ 0.0f, 0.0f, 0.0f } };

    
    gpudata_->volumes = context_.CreateBuffer<Volume>(1, CL_MEM_READ_ONLY, &vol);

	gpudata_->debug = context_.CreateBuffer<int>(1, CL_MEM_READ_WRITE);
}

FrRenderer::~FrRenderer()
{
    // Delete all shapes
    for (int i = 0; i < (int)shapes_.size(); ++i)
    {
        api_->DeleteShape(shapes_[i]);
    }

    // Delete API
    IntersectionApi::Delete(api_);
}

Output* FrRenderer::CreateOutput(int w, int h)
{
    return new FrOutput(w, h, context_);
}

void FrRenderer::DeleteOutput(Output* output)
{
    delete output;
}

void FrRenderer::Clear(FireRays::float3 const& val, Output& output)
{
    static_cast<FrOutput&>(output).Clear(val);
	resetsampler_ = 1;
}


void FrRenderer::Preprocess(Scene const& scene)
{
    // Release old data if any
    // TODO: implement me
	rand_init();

    // Enumerate all shapes in the scene
    for (int i = 0; i < (int)scene.shapes_.size(); ++i)
    {
        Shape* shape = nullptr;

        shape = api_->CreateMesh(
            // Vertices starting from the first one
            (float*)&scene.vertices_[scene.shapes_[i].startvtx],
            // Number of vertices
            scene.shapes_[i].numvertices,
            // Stride
            sizeof(float3),
            // TODO: make API signature const
            const_cast<int*>(&scene.indices_[scene.shapes_[i].startidx]),
            // Index stride
            0,
            // All triangles
            nullptr,
            // Number of primitives
            (int)scene.shapes_[i].numprims
            );

		int midx = scene.materialids_[scene.shapes_[i].startidx / 3];
		if (scene.material_names_[midx] == "glass" || scene.material_names_[midx] == "glasstranslucent")
		{
			//std::cout << "Setting glass mask\n";
			shape->SetMask(0xFFFF0000);
		}

        shape->SetLinearVelocity(scene.shapes_[i].linearvelocity);

        shape->SetAngularVelocity(scene.shapes_[i].angularvelocity);

        shape->SetTransform(scene.shapes_[i].m, inverse(scene.shapes_[i].m));

        api_->AttachShape(shape);

        shapes_.push_back(shape);
    }

    // Commit to intersector
    auto startime = std::chrono::high_resolution_clock::now();

    api_->Commit();

    auto delta = std::chrono::high_resolution_clock::now() - startime;

    std::cout << "Commited in " << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() / 1000.f << "s\n";

    // Duplicate geometry in OpenCL buffers
    CompileScene(scene);
}

// Set number of bounces
void FrRenderer::SetNumBounces(int nbounces)
{
    nbounces_ = nbounces;
}


// Render the scene into the output
void FrRenderer::Render(Scene const& scene)
{
    // Check output
    assert(output_);

    // Update camera data
    context_.WriteBuffer(0, gpudata_->camera, scene.camera_.get(), 1);

    // Number of rays to generate
    int maxrays = output_->width() * output_->height();

    // Generate primary 
    GeneratePrimaryRays();

    // Copy compacted indices to track reverse indices
    context_.CopyBuffer(0, gpudata_->iota, gpudata_->pixelindices[0], 0, 0, gpudata_->iota.GetElementCount());
    context_.CopyBuffer(0, gpudata_->iota, gpudata_->pixelindices[1], 0, 0, gpudata_->iota.GetElementCount());
    context_.FillBuffer(0, gpudata_->hitcount[0], maxrays, 1);
	context_.FillBuffer(0, gpudata_->hitcount[1], maxrays, 1);
    context_.FillBuffer(0, gpudata_->debug, 0, 1);

	//float3 clearval;
	//clearval.w = 1.f;
	//context_.FillBuffer(0, gpudata_->radiance, clearval, gpudata_->radiance.GetElementCount());

    // Initialize first pass
    for (int pass = 0; pass < nbounces_; ++pass)
    {
        // Clear ray hits buffer
        context_.FillBuffer(0, gpudata_->hits, 0, gpudata_->hits.GetElementCount());

        // Intersect ray batch
        api_->QueryIntersection(gpudata_->fr_rays[pass & 0x1], gpudata_->fr_hitcount[0], maxrays, gpudata_->fr_intersections, nullptr, nullptr);
        
		// Apply scattering
        EvaluateVolume(scene, pass);

        // Convert intersections to predicates
        FilterPathStream(pass);

        // Compact batch
        gpudata_->pp.Compact(0, gpudata_->hits, gpudata_->iota, gpudata_->compacted_indices, gpudata_->hitcount[0]);

		/*int cnt = 0;
		context_.ReadBuffer(0, gpudata_->hitcount[0], &cnt, 1).Wait();
		std::cout << "Pass " << pass << " Alive " << cnt << "\n";*/

        // Advance indices to keep pixel indices up to date
        RestorePixelIndices(pass);

		// Shade hits
		ShadeVolume(scene, pass);

		// Shade hits
		ShadeSurface(scene, pass);

		// Shade missing rays
		if (pass == 0) ShadeMiss(scene, pass);

        // Intersect shadow rays
        api_->QueryOcclusion(gpudata_->fr_shadowrays, gpudata_->fr_hitcount[0], maxrays, gpudata_->fr_shadowhits, nullptr, nullptr);

        // Gather light samples and account for visibility
        GatherLightSamples(scene, pass);
        
		//
        context_.Flush(0);
    }
}

// Render the scene into the output
void FrRenderer::RenderAmbientOcclusion(Scene const& scene, float radius)
{
    // Check output
    assert(output_);

    // Clear some buffers
    float3 clearval = float3(0, 0, 0);
    clearval.w = 1.f;
    // Clear radiance sample buffer
    context_.FillBuffer(0, gpudata_->radiance, clearval, gpudata_->radiance.GetElementCount());

    // Update camera data
    context_.WriteBuffer(0, gpudata_->camera, scene.camera_.get(), 1);

    // Number of rays to generate
    int numrays = output_->width() * output_->height();

    // Generate primary 
    GeneratePrimaryRays();

    // Copy compacted indices to track reverse indices
    context_.CopyBuffer(0, gpudata_->iota, gpudata_->pixelindices[0], 0, 0, gpudata_->iota.GetElementCount());
    context_.CopyBuffer(0, gpudata_->iota, gpudata_->pixelindices[1], 0, 0, gpudata_->iota.GetElementCount());
    context_.FillBuffer(0, gpudata_->hitcount[0], numrays, 1);
    context_.FillBuffer(0, gpudata_->hitcount[1], numrays, 1);

    // Intersect ray batch
    api_->QueryIntersection(gpudata_->fr_rays[0], gpudata_->fr_hitcount[0], numrays, gpudata_->fr_intersections, nullptr, nullptr);

    // Convert intersections to predicates
    FilterPathStream(0);

    //
    gpudata_->pp.Compact(0, gpudata_->hits, gpudata_->iota, gpudata_->compacted_indices, gpudata_->hitcount[0]).Wait();

    // Advance indices to keep pixel indices up to data
    RestorePixelIndices(0);

    // Shade hits
    SampleAmbientOcclusion(scene, radius);

    // Intersect shadow rays
    api_->QueryOcclusion(gpudata_->fr_shadowrays, gpudata_->fr_hitcount[0], numrays, gpudata_->fr_shadowhits, nullptr, nullptr);

    // Gather light samples and account for visibility
    GatherAmbientOcclusion(scene);
    
    //context_.Finish(0);
}

void FrRenderer::SampleAmbientOcclusion(Scene const& scene, float radius)
{
    // Fetch kernel
    CLWKernel shadekernel = gpudata_->program.GetKernel("SampleOcclusion");

    // Set kernel parameters
    int argc = 0;
    shadekernel.SetArg(argc++, gpudata_->rays[0]);
    shadekernel.SetArg(argc++, gpudata_->intersections);
    shadekernel.SetArg(argc++, gpudata_->compacted_indices);
    shadekernel.SetArg(argc++, gpudata_->pixelindices[0]);
    shadekernel.SetArg(argc++, gpudata_->hitcount[0]);
    shadekernel.SetArg(argc++, gpudata_->vertices);
    shadekernel.SetArg(argc++, gpudata_->normals);
    shadekernel.SetArg(argc++, gpudata_->uvs);
    shadekernel.SetArg(argc++, gpudata_->indices);
    shadekernel.SetArg(argc++, gpudata_->shapes);
    shadekernel.SetArg(argc++, gpudata_->materialids);
    shadekernel.SetArg(argc++, gpudata_->materials);
    shadekernel.SetArg(argc++, gpudata_->textures);
    shadekernel.SetArg(argc++, gpudata_->texturedata);
    shadekernel.SetArg(argc++, scene.envidx_);
    shadekernel.SetArg(argc++, radius);
    shadekernel.SetArg(argc++, gpudata_->emissives);
    shadekernel.SetArg(argc++, gpudata_->numemissive);
    shadekernel.SetArg(argc++, rand_uint());
    shadekernel.SetArg(argc++, gpudata_->shadowrays);
    shadekernel.SetArg(argc++, gpudata_->lightsamples);
    shadekernel.SetArg(argc++, gpudata_->paths);
    shadekernel.SetArg(argc++, gpudata_->rays[1]);


    // Run shading kernel
    {
        int globalsize = output_->width() * output_->height();
        context_.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
    }
}

void FrRenderer::GatherAmbientOcclusion(Scene const& scene)
{
    // Fetch kernel
    CLWKernel gatherkernel = gpudata_->program.GetKernel("GatherOcclusion");

    // Set kernel parameters
    int argc = 0;
    gatherkernel.SetArg(argc++, gpudata_->pixelindices[0]);
    gatherkernel.SetArg(argc++, gpudata_->hitcount[0]);
    gatherkernel.SetArg(argc++, gpudata_->shadowhits);
    gatherkernel.SetArg(argc++, gpudata_->lightsamples);
    gatherkernel.SetArg(argc++, gpudata_->paths);
    gatherkernel.SetArg(argc++, output_->data());

    // Run shading kernel
    {
        int globalsize = output_->width() * output_->height();
        context_.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gatherkernel);
    }
}

void FrRenderer::SetOutput(Output* output)
{
    if (!output_ || output_->width() < output->width() || output_->height() < output->height())
    {
        ResizeWorkingSet(*output);
    }

    output_ = static_cast<FrOutput*>(output);
}

void FrRenderer::ResizeWorkingSet(Output const& output)
{
    vidmemws_ = 0;

    // Create ray payloads
    gpudata_->rays[0] = context_.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(ray);

    gpudata_->rays[1] = context_.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(ray);

    gpudata_->rays[2] = context_.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(ray);

    gpudata_->rays[3] = context_.CreateBuffer<ray>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(ray);

    gpudata_->hits = context_.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(int);

    gpudata_->intersections = context_.CreateBuffer<Intersection>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(Intersection);

    gpudata_->shadowrays = context_.CreateBuffer<ray>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(ray)* kMaxLightSamples;

    gpudata_->shadowhits = context_.CreateBuffer<int>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(int)* kMaxLightSamples;

    gpudata_->lightsamples = context_.CreateBuffer<float3>(output.width() * output.height() * kMaxLightSamples, CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(float3)* kMaxLightSamples;

    gpudata_->radiance = context_.CreateBuffer<float3>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(float3) * kMaxLightSamples;

    gpudata_->accumulator = context_.CreateBuffer<float3>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(float3) * kMaxLightSamples;

    gpudata_->paths = context_.CreateBuffer<PathState>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(PathState);

	gpudata_->samplers = context_.CreateBuffer<QmcSampler>(output.width() * output.height(), CL_MEM_READ_WRITE);
	vidmemws_ += output.width() * output.height() * sizeof(QmcSampler);

    std::vector<int> initdata(output.width() * output.height());
    std::iota(initdata.begin(), initdata.end(), 0);
    gpudata_->iota = context_.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &initdata[0]);
    vidmemws_ += output.width() * output.height() * sizeof(int);

    gpudata_->compacted_indices = context_.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(int);

    gpudata_->pixelindices[0] = context_.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(int);

    gpudata_->pixelindices[1] = context_.CreateBuffer<int>(output.width() * output.height(), CL_MEM_READ_WRITE);
    vidmemws_ += output.width() * output.height() * sizeof(int);

    // Recreate FR buffers
    api_->DeleteBuffer(gpudata_->fr_rays[0]);
    api_->DeleteBuffer(gpudata_->fr_rays[1]);
    api_->DeleteBuffer(gpudata_->fr_shadowrays);
    api_->DeleteBuffer(gpudata_->fr_hits);
    api_->DeleteBuffer(gpudata_->fr_shadowhits);
    api_->DeleteBuffer(gpudata_->fr_intersections);

    gpudata_->fr_rays[0] = api_->CreateFromOpenClBuffer(gpudata_->rays[0]);
    gpudata_->fr_rays[1] = api_->CreateFromOpenClBuffer(gpudata_->rays[1]);
    gpudata_->fr_shadowrays = api_->CreateFromOpenClBuffer(gpudata_->shadowrays);
    gpudata_->fr_hits = api_->CreateFromOpenClBuffer(gpudata_->hits);
    gpudata_->fr_shadowhits = api_->CreateFromOpenClBuffer(gpudata_->shadowhits);
    gpudata_->fr_intersections = api_->CreateFromOpenClBuffer(gpudata_->intersections);

    std::cout << "Vidmem usage (working set): " << vidmemws_ / (1024 * 1024) << "Mb\n";
}

void FrRenderer::GeneratePrimaryRays()
{
    // Fetch kernel
    CLWKernel genkernel = gpudata_->program.GetKernel("PerspectiveCamera_GeneratePaths");

    // Set kernel parameters
    genkernel.SetArg(0, gpudata_->camera);
    genkernel.SetArg(1, output_->width());
    genkernel.SetArg(2, output_->height());
    genkernel.SetArg(3, (int)rand_uint());
    genkernel.SetArg(4, gpudata_->rays[0]);
	genkernel.SetArg(5, gpudata_->samplers);
	genkernel.SetArg(6, gpudata_->sobolmat);
	genkernel.SetArg(7, resetsampler_);
	genkernel.SetArg(8, gpudata_->paths);
	resetsampler_ = 0;

    // Run generation kernel
    {
        size_t gs[] = { static_cast<size_t>((output_->width() + 7) / 8 * 8), static_cast<size_t>((output_->height() + 7) / 8 * 8) };
        size_t ls[] = { 8, 8 };

        context_.Launch2D(0, gs, ls, genkernel);
    }
}

void FrRenderer::ShadeSurface(Scene const& scene, int pass)
{
    // Fetch kernel
    CLWKernel shadekernel = gpudata_->program.GetKernel("ShadeSurface");

    // Set kernel parameters
    int argc = 0;
    shadekernel.SetArg(argc++, gpudata_->rays[pass & 0x1]);
    shadekernel.SetArg(argc++, gpudata_->intersections);
    shadekernel.SetArg(argc++, gpudata_->compacted_indices);
    shadekernel.SetArg(argc++, gpudata_->pixelindices[pass & 0x1]);
    shadekernel.SetArg(argc++, gpudata_->hitcount[0]);
    shadekernel.SetArg(argc++, gpudata_->vertices);
    shadekernel.SetArg(argc++, gpudata_->normals);
    shadekernel.SetArg(argc++, gpudata_->uvs);
    shadekernel.SetArg(argc++, gpudata_->indices);
    shadekernel.SetArg(argc++, gpudata_->shapes);
    shadekernel.SetArg(argc++, gpudata_->materialids);
    shadekernel.SetArg(argc++, gpudata_->materials);
    shadekernel.SetArg(argc++, gpudata_->textures);
    shadekernel.SetArg(argc++, gpudata_->texturedata);
    shadekernel.SetArg(argc++, scene.envidx_);
    shadekernel.SetArg(argc++, scene.envmapmul_);
    shadekernel.SetArg(argc++, gpudata_->emissives);
    shadekernel.SetArg(argc++, gpudata_->numemissive);
    shadekernel.SetArg(argc++, rand_uint());
	shadekernel.SetArg(argc++, gpudata_->samplers);
	shadekernel.SetArg(argc++, gpudata_->sobolmat);
	shadekernel.SetArg(argc++, pass);
    shadekernel.SetArg(argc++, gpudata_->volumes);
    shadekernel.SetArg(argc++, gpudata_->shadowrays);
    shadekernel.SetArg(argc++, gpudata_->lightsamples);
    shadekernel.SetArg(argc++, gpudata_->paths);
    shadekernel.SetArg(argc++, gpudata_->rays[(pass + 1) & 0x1]);
	shadekernel.SetArg(argc++, output_->data());


    // Run shading kernel
    {
        int globalsize = output_->width() * output_->height();
        context_.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
    }
}

void FrRenderer::ShadeVolume(Scene const& scene, int pass)
{
	// Fetch kernel
	CLWKernel shadekernel = gpudata_->program.GetKernel("ShadeVolume");

	// Set kernel parameters
	int argc = 0;
	shadekernel.SetArg(argc++, gpudata_->rays[pass & 0x1]);
	shadekernel.SetArg(argc++, gpudata_->intersections);
	shadekernel.SetArg(argc++, gpudata_->compacted_indices);
	shadekernel.SetArg(argc++, gpudata_->pixelindices[pass & 0x1]);
	shadekernel.SetArg(argc++, gpudata_->hitcount[0]);
	shadekernel.SetArg(argc++, gpudata_->vertices);
	shadekernel.SetArg(argc++, gpudata_->normals);
	shadekernel.SetArg(argc++, gpudata_->uvs);
	shadekernel.SetArg(argc++, gpudata_->indices);
	shadekernel.SetArg(argc++, gpudata_->shapes);
	shadekernel.SetArg(argc++, gpudata_->materialids);
	shadekernel.SetArg(argc++, gpudata_->materials);
	shadekernel.SetArg(argc++, gpudata_->textures);
	shadekernel.SetArg(argc++, gpudata_->texturedata);
	shadekernel.SetArg(argc++, scene.envidx_);
	shadekernel.SetArg(argc++, scene.envmapmul_);
	shadekernel.SetArg(argc++, gpudata_->emissives);
	shadekernel.SetArg(argc++, gpudata_->numemissive);
	shadekernel.SetArg(argc++, rand_uint());
	shadekernel.SetArg(argc++, gpudata_->samplers);
	shadekernel.SetArg(argc++, gpudata_->sobolmat);
	shadekernel.SetArg(argc++, pass);
	shadekernel.SetArg(argc++, gpudata_->volumes);
	shadekernel.SetArg(argc++, gpudata_->shadowrays);
	shadekernel.SetArg(argc++, gpudata_->lightsamples);
	shadekernel.SetArg(argc++, gpudata_->paths);
	shadekernel.SetArg(argc++, gpudata_->rays[(pass + 1) & 0x1]);
	shadekernel.SetArg(argc++, output_->data());

	// Run shading kernel
	{
		int globalsize = output_->width() * output_->height();
		context_.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, shadekernel);
	}

}

void FrRenderer::EvaluateVolume(Scene const& scene, int pass)
{
    // Fetch kernel
    CLWKernel evalkernel = gpudata_->program.GetKernel("EvaluateVolume");

    // Set kernel parameters
    int argc = 0;
    evalkernel.SetArg(argc++, gpudata_->rays[pass & 0x1]);
    evalkernel.SetArg(argc++, gpudata_->pixelindices[(pass + 1) & 0x1]);
    evalkernel.SetArg(argc++, gpudata_->hitcount[0]);
    evalkernel.SetArg(argc++, gpudata_->volumes);
    evalkernel.SetArg(argc++, gpudata_->textures);
    evalkernel.SetArg(argc++, gpudata_->texturedata);
    evalkernel.SetArg(argc++, rand_uint());
    evalkernel.SetArg(argc++, gpudata_->samplers);
    evalkernel.SetArg(argc++, gpudata_->sobolmat);
    evalkernel.SetArg(argc++, pass);
    evalkernel.SetArg(argc++, gpudata_->intersections);
    evalkernel.SetArg(argc++, gpudata_->paths);
    evalkernel.SetArg(argc++, output_->data());
    
    // Run shading kernel
    {
        int globalsize = output_->width() * output_->height();
        context_.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, evalkernel);
    }
}

void FrRenderer::ShadeMiss(Scene const& scene, int pass)
{
    // Fetch kernel
    CLWKernel misskernel = gpudata_->program.GetKernel("ShadeMiss");

    int numrays = output_->width() * output_->height();

    // Set kernel parameters
    int argc = 0;
    misskernel.SetArg(argc++, gpudata_->rays[pass & 0x1]);
    misskernel.SetArg(argc++, gpudata_->intersections);
    misskernel.SetArg(argc++, gpudata_->pixelindices[(pass + 1) & 0x1]);
    misskernel.SetArg(argc++, numrays);
    misskernel.SetArg(argc++, gpudata_->textures);
    misskernel.SetArg(argc++, gpudata_->texturedata);
    misskernel.SetArg(argc++, scene.envidx_);
    misskernel.SetArg(argc++, gpudata_->paths);
    misskernel.SetArg(argc++, gpudata_->volumes);
    misskernel.SetArg(argc++, output_->data());

    // Run shading kernel
    {
        int globalsize = output_->width() * output_->height();
        context_.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, misskernel);
    }
}

void FrRenderer::GatherLightSamples(Scene const& scene, int pass)
{
    // Fetch kernel
    CLWKernel gatherkernel = gpudata_->program.GetKernel("GatherLightSamples");

    // Set kernel parameters
    int argc = 0;
    gatherkernel.SetArg(argc++, gpudata_->pixelindices[pass & 0x1]);
    gatherkernel.SetArg(argc++, gpudata_->hitcount[0]);
    gatherkernel.SetArg(argc++, gpudata_->shadowhits);
    gatherkernel.SetArg(argc++, gpudata_->lightsamples);
    gatherkernel.SetArg(argc++, gpudata_->paths);
    gatherkernel.SetArg(argc++, output_->data());

    // Run shading kernel
    {
        int globalsize = output_->width() * output_->height();
        context_.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, gatherkernel);
    }
}

void FrRenderer::RestorePixelIndices(int pass)
{
    // Fetch kernel
    CLWKernel restorekernel = gpudata_->program.GetKernel("RestorePixelIndices");

    // Set kernel parameters
    int argc = 0;
    restorekernel.SetArg(argc++, gpudata_->compacted_indices);
    restorekernel.SetArg(argc++, gpudata_->hitcount[0]);
    restorekernel.SetArg(argc++, gpudata_->pixelindices[(pass + 1) & 0x1]);
    restorekernel.SetArg(argc++, gpudata_->pixelindices[pass & 0x1]);

    // Run shading kernel
    {
        int globalsize = output_->width() * output_->height();
        context_.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, restorekernel);
    }
}

void FrRenderer::FilterPathStream(int pass)
{
    // Fetch kernel
    CLWKernel restorekernel = gpudata_->program.GetKernel("FilterPathStream");

    // Set kernel parameters
    int argc = 0;
    restorekernel.SetArg(argc++, gpudata_->intersections);
    restorekernel.SetArg(argc++, gpudata_->hitcount[0]);
	restorekernel.SetArg(argc++, gpudata_->pixelindices[(pass + 1) & 0x1]);
	restorekernel.SetArg(argc++, gpudata_->paths);
    restorekernel.SetArg(argc++, gpudata_->hits);
    restorekernel.SetArg(argc++, gpudata_->debug);

    // Run shading kernel
    {
        int globalsize = output_->width() * output_->height();
        context_.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, restorekernel);
    }
    
    //int cnt = 0;
    //context_.ReadBuffer(0, gpudata_->debug, &cnt, 1).Wait();
    //std::cout << "Pass " << pass << " killed " << cnt << "\n";
}

void FrRenderer::CompileScene(Scene const& scene)
{
    vidmemusage_ = 0;

    // Vertex, normal and uv data
    gpudata_->vertices = context_.CreateBuffer<float3>(scene.vertices_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.vertices_[0]);
    vidmemusage_ += scene.vertices_.size() * sizeof(float3);

    gpudata_->normals = context_.CreateBuffer<float3>(scene.normals_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.normals_[0]);
    vidmemusage_ += scene.normals_.size() * sizeof(float3);

    gpudata_->uvs = context_.CreateBuffer<float2>(scene.uvs_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.uvs_[0]);
    vidmemusage_ += scene.uvs_.size() * sizeof(float2);

    // Index data
    gpudata_->indices = context_.CreateBuffer<int>(scene.indices_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.indices_[0]);
    vidmemusage_ += scene.indices_.size() * sizeof(int);

    // Shapes
    gpudata_->shapes = context_.CreateBuffer<Scene::Shape>(scene.shapes_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.shapes_[0]);
    vidmemusage_ += scene.shapes_.size() * sizeof(Scene::Shape);

    // Material IDs
    gpudata_->materialids = context_.CreateBuffer<int>(scene.materialids_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.materialids_[0]);
    vidmemusage_ += scene.materialids_.size() * sizeof(int);

    // Material descriptions
    gpudata_->materials = context_.CreateBuffer<Scene::Material>(scene.materials_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.materials_[0]);
    vidmemusage_ += scene.materials_.size() * sizeof(Scene::Material);

    // Bake textures
    BakeTextures(scene);

    // Emissives
    if (scene.emissives_.size() > 0)
    {
        gpudata_->emissives = context_.CreateBuffer<Scene::Emissive>(scene.emissives_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.emissives_[0]);
        gpudata_->numemissive = scene.emissives_.size();
        vidmemusage_ += scene.emissives_.size() * sizeof(Scene::Emissive);
    }
    else
    {
        gpudata_->emissives = context_.CreateBuffer<Scene::Emissive>(1, CL_MEM_READ_ONLY);
        gpudata_->numemissive = 0;
        vidmemusage_ += sizeof(Scene::Emissive);
    }

    std::cout << "Vidmem usage (data): " << vidmemusage_ / (1024 * 1024) << "Mb\n";
    std::cout << "Polygon count " << scene.indices_.size() / 3 << "\n";
}

void FrRenderer::RunBenchmark(Scene const& scene)
{
    {
        // Update camera data
        context_.WriteBuffer(0, gpudata_->camera, scene.camera_.get(), 1);

        // Number of rays to generate
        int numrays = output_->width() * output_->height();

        // Generate primary
        GeneratePrimaryRays();

        // Copy compacted indices to track reverse indices
        context_.CopyBuffer(0, gpudata_->iota, gpudata_->pixelindices[0], 0, 0, gpudata_->iota.GetElementCount());
        context_.CopyBuffer(0, gpudata_->iota, gpudata_->pixelindices[1], 0, 0, gpudata_->iota.GetElementCount());

        // Initialize first pass
        int numhits = numrays;
        int kNumPasses = 100;
        float time = 0.f;

        std::cout << "Starting the benchmark\n";
        std::cout << "----------------------\n";
        std::cout << "Number of primary rays: " << numrays << "\n";


        for (int pass = 0; pass < nbounces_; ++pass)
        {
            std::cout << "Bounce number " << pass << "\n";
            std::cout << "Number of rays: " << numhits << "\n";

            // Clear ray hits buffer
            context_.FillBuffer(0, gpudata_->hits, 0, gpudata_->hits.GetElementCount()).Wait();

            // Intersect ray batch
            Event* event = nullptr;

            auto start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < kNumPasses; ++i)
            {
                api_->QueryIntersection(gpudata_->fr_rays[pass & 0x1], numhits, gpudata_->fr_intersections, nullptr, &event);
            }

            event->Wait();

            auto delta = std::chrono::high_resolution_clock::now() - start;

            time = (float)std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();

            std::cout << "Average trace time: " << time / kNumPasses << " ms\n";
            std::cout << "Average throuput: " << (float)numhits / (time / kNumPasses) / 1000 << " MRays/s\n";

            // Convert intersections to predicates
            FilterPathStream(pass);

            if (pass == 0)
            {
                // Shade missing rays
                ShadeMiss(scene, pass);
            }

            // Compact batch
            gpudata_->pp.Compact(0, gpudata_->hits, gpudata_->iota, gpudata_->compacted_indices, numhits).Wait();

            if (numhits == 0)
            {
                break;
            }

            // Advance indices to keep pixel indices up to data
            RestorePixelIndices(numhits);

            // Shade hits
            ShadeSurface(scene, pass);

            std::cout << "Number of shadow rays: " << kMaxLightSamples * numhits << "\n";
            // Intersect shadow rays

            start = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < kNumPasses; ++i)
            {
                api_->QueryOcclusion(gpudata_->fr_shadowrays, numhits * kMaxLightSamples, gpudata_->fr_shadowhits, nullptr, &event);
            }

            event->Wait();

            delta = std::chrono::high_resolution_clock::now() - start;

            time = (float)std::chrono::duration_cast<std::chrono::milliseconds>(delta).count();

            std::cout << "Average trace time: " << time / kNumPasses << " ms\n";
            std::cout << "Average throuput: " << (float)(kMaxLightSamples * numhits) / (time / kNumPasses) / 1000 << " MRays/s\n";

            // Gather light samples and account for visibility
            GatherLightSamples(scene, pass);

            std::cout << "----------------------\n";
        }
    }
}

void FrRenderer::BakeTextures(Scene const& scene)
{
    if (scene.textures_.size() > 0)
    {
        // Evaluate data size
        size_t datasize = 0;
        for (auto iter = scene.textures_.cbegin(); iter != scene.textures_.cend(); ++iter)
        {
            datasize += iter->size;
        }

        // Texture descriptors
        gpudata_->textures = context_.CreateBuffer<Scene::Texture>(scene.textures_.size(), CL_MEM_READ_ONLY);
        vidmemusage_ += scene.textures_.size() * sizeof(Scene::Texture);

        // Texture data
        gpudata_->texturedata = context_.CreateBuffer<char>(datasize, CL_MEM_READ_ONLY);

        // Map both buffers
        Scene::Texture* mappeddesc = nullptr;
        char* mappeddata = nullptr;
        Scene::Texture* mappeddesc_orig = nullptr;
        char* mappeddata_orig = nullptr;

        context_.MapBuffer(0, gpudata_->textures, CL_MAP_WRITE, &mappeddesc).Wait();
        context_.MapBuffer(0, gpudata_->texturedata, CL_MAP_WRITE, &mappeddata).Wait();

        // Save them for unmap
        mappeddesc_orig = mappeddesc;
        mappeddata_orig = mappeddata;

        // Write data into both buffers
        int current_offset = 0;
        for (auto iter = scene.textures_.cbegin(); iter != scene.textures_.cend(); ++iter)
        {
            // Copy texture desc
            Scene::Texture texture = *iter;

            // Write data into the buffer
            memcpy(mappeddata, scene.texturedata_[texture.dataoffset].get(), texture.size);
            vidmemusage_ += texture.size;

            // Adjust offset in the texture
            texture.dataoffset = current_offset;

            // Copy desc into the buffer
            *mappeddesc = texture;

            // Adjust offset
            current_offset += texture.size;

            // Adjust data pointer
            mappeddata += texture.size;

            // Adjust descriptor pointer
            ++mappeddesc;
        }

        context_.UnmapBuffer(0, gpudata_->textures, mappeddesc_orig).Wait();
        context_.UnmapBuffer(0, gpudata_->texturedata, mappeddata_orig).Wait();
    }
    else
    {
        // Create stub
        gpudata_->textures = context_.CreateBuffer<Scene::Texture>(1, CL_MEM_READ_ONLY);
        vidmemusage_ += sizeof(Scene::Texture);

        // Texture data
        gpudata_->texturedata = context_.CreateBuffer<char>(1, CL_MEM_READ_ONLY);
        vidmemusage_ += 1;
    }
}

CLWKernel FrRenderer::GetCopyKernel()
{
    return gpudata_->program.GetKernel("ApplyGammaAndCopyData");
}

CLWKernel FrRenderer::GetAccumulateKernel()
{
	return gpudata_->program.GetKernel("AccumulateData");
}

