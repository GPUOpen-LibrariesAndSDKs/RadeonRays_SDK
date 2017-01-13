#include "Scene/scene_tracker.h"
#include "Scene/scene.h"
#include "CLW/clwscene.h"
#include "perspective_camera.h"

#include <chrono>

using namespace RadeonRays;


namespace Baikal
{
    SceneTracker::SceneTracker(CLWContext context, int devidx)
    : m_context(context)
    , m_vidmem_usage(0)
    {
        // Get raw CL data out of CLW context
        cl_device_id id = m_context.GetDevice(devidx).GetID();
        cl_command_queue queue = m_context.GetCommandQueue(devidx);

        // Create intersection API
        m_api = CreateFromOpenClContext(m_context, id, queue);

        // Do app specific settings
#ifdef __APPLE__
        // Apple runtime has known issue with stacked traversal
        m_api->SetOption("acc.type", "bvh");
        m_api->SetOption("bvh.builder", "sah");
#else
        m_api->SetOption("acc.type", "fatbvh");
        m_api->SetOption("bvh.builder", "sah");
#endif
    }

    SceneTracker::~SceneTracker()
    {
        //Flush();
        // Delete API
        IntersectionApi::Delete(m_api);
    }

    ClwScene& SceneTracker::CompileScene(Scene const& scene) const
    {
        auto iter = m_scene_cache.find(&scene);

        if (iter == m_scene_cache.cend())
        {
            auto res = m_scene_cache.emplace(std::make_pair(&scene, ClwScene()));

            RecompileFull(scene, res.first->second);

            ReloadIntersector(scene, res.first->second);

            m_current_scene = &scene;

            return res.first->second;
        }
        else
        {
            auto& out = iter->second;

            if (scene.dirty() & Scene::DirtyFlags::kCamera)
            {
                UpdateCamera(scene, out);
            }

            if (scene.dirty() & Scene::DirtyFlags::kGeometry)
            {
                UpdateGeometry(scene, out);
            }
            else if (scene.dirty() & Scene::DirtyFlags::kGeometryTransform)
            {
                // TODO: this is not yet supported in the renderer
            }

            if (scene.dirty() & Scene::DirtyFlags::kMaterials)
            {
                UpdateMaterials(scene, out);
            }
            else if (scene.dirty() & Scene::DirtyFlags::kMaterialInputs)
            {
                UpdateMaterialInputs(scene, out);
            }

            if (m_current_scene != &scene)
            {
                ReloadIntersector(scene, out);

                m_current_scene = &scene;
            }

            scene.clear_dirty();

            return out;
        }
    }

    void SceneTracker::UpdateCamera(Scene const& scene, ClwScene& out) const
    {
        // Update camere type
        out.camera_type = scene.camera_->GetAperture() > 0.f ? CameraType::kPhysical : CameraType::kDefault;

        // Update camera data
        m_context.WriteBuffer(0, out.camera, scene.camera_.get(), 1);
    }

    void SceneTracker::UpdateGeometry(Scene const& scene, ClwScene& out) const
    {
        out.vertices = m_context.CreateBuffer<float3>(scene.vertices_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.vertices_[0]);

        out.normals = m_context.CreateBuffer<float3>(scene.normals_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.normals_[0]);

        out.uvs = m_context.CreateBuffer<float2>(scene.uvs_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.uvs_[0]);

        out.indices = m_context.CreateBuffer<int>(scene.indices_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.indices_[0]);

        out.shapes = m_context.CreateBuffer<Scene::Shape>(scene.shapes_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.shapes_[0]);
    }

    void SceneTracker::UpdateMaterials(Scene const& scene, ClwScene& out) const
    {
        // Material IDs
        out.materialids = m_context.CreateBuffer<int>(scene.materialids_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.materialids_[0]);

        // Material descriptions
        out.materials = m_context.CreateBuffer<Scene::Material>(scene.materials_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.materials_[0]);
    }

    void SceneTracker::UpdateMaterialInputs(Scene const& scene, ClwScene& out) const
    {
        m_context.WriteBuffer(0, out.materials, &scene.materials_[0], out.materials.GetElementCount());
    }

    void SceneTracker::RecompileFull(Scene const& scene, ClwScene& out) const
    {
        // This usually unnecessary, but just in case we reuse out parameter here
        for (auto& s : out.isect_shapes)
        {
            m_api->DeleteShape(s);
        }

        out.isect_shapes.clear();

        m_vidmem_usage = 0;

        // Create static buffers
        out.camera = m_context.CreateBuffer<PerspectiveCamera>(1, CL_MEM_READ_ONLY |  CL_MEM_COPY_HOST_PTR, scene.camera_.get());

        // Vertex, normal and uv data
        out.vertices = m_context.CreateBuffer<float3>(scene.vertices_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.vertices_[0]);
        m_vidmem_usage += scene.vertices_.size() * sizeof(float3);

        out.normals = m_context.CreateBuffer<float3>(scene.normals_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.normals_[0]);
        m_vidmem_usage += scene.normals_.size() * sizeof(float3);

        out.uvs = m_context.CreateBuffer<float2>(scene.uvs_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.uvs_[0]);
        m_vidmem_usage += scene.uvs_.size() * sizeof(float2);

        // Index data
        out.indices = m_context.CreateBuffer<int>(scene.indices_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.indices_[0]);
        m_vidmem_usage += scene.indices_.size() * sizeof(int);

        // Shapes
        out.shapes = m_context.CreateBuffer<Scene::Shape>(scene.shapes_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.shapes_[0]);
        m_vidmem_usage += scene.shapes_.size() * sizeof(Scene::Shape);

        // Material IDs
        out.materialids = m_context.CreateBuffer<int>(scene.materialids_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.materialids_[0]);
        m_vidmem_usage += scene.materialids_.size() * sizeof(int);

        // Material descriptions
        out.materials = m_context.CreateBuffer<Scene::Material>(scene.materials_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.materials_[0]);
        m_vidmem_usage += scene.materials_.size() * sizeof(Scene::Material);

        // Bake textures
        BakeTextures(scene, out);

        // Emissives
        if (scene.lights_.size() > 0)
        {
            out.lights = m_context.CreateBuffer<Scene::Light>(scene.lights_.size(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, (void*)&scene.lights_[0]);
            out.num_lights = scene.lights_.size();
            m_vidmem_usage += scene.lights_.size() * sizeof(Scene::Light);
        }
        else
        {
            out.lights = m_context.CreateBuffer<Scene::Light>(1, CL_MEM_READ_ONLY);
            out.num_lights = 0;
            m_vidmem_usage += sizeof(Scene::Light);
        }

        //Volume vol = {1, 0, 0, 0, {0.9f, 0.6f, 0.9f}, {5.1f, 1.8f, 5.1f}, {0.0f, 0.0f, 0.0f}};
        Scene::Volume vol = { 1, 0, 0, 0,{    1.2f, 0.4f, 1.2f },{ 5.1f, 4.8f, 5.1f },{ 0.0f, 0.0f, 0.0f } };

        out.volumes = m_context.CreateBuffer<Scene::Volume>(1, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &vol);

        out.envmapmul = scene.envmapmul_;
        out.envmapidx = scene.envidx_;

        std::cout << "Vidmem usage (data): " << m_vidmem_usage / (1024 * 1024) << "Mb\n";
        std::cout << "Polygon count " << scene.indices_.size() / 3 << "\n";

        std::cout << "Number of objects: " << scene.shapes_.size() << "\n";
        std::cout << "Number of textures: " << scene.textures_.size() << "\n";
        std::cout << "Number of lights: " << scene.lights_.size() << "\n";

        // Enumerate all shapes in the scene
        for (int i = 0; i < (int)scene.shapes_.size(); ++i)
        {
            Shape* shape = nullptr;

            shape = m_api->CreateMesh(
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

            shape->SetLinearVelocity(scene.shapes_[i].linearvelocity);

            shape->SetAngularVelocity(scene.shapes_[i].angularvelocity);

            shape->SetTransform(scene.shapes_[i].m, inverse(scene.shapes_[i].m));

            shape->SetId(i + 1);

            out.isect_shapes.push_back(shape);
        }
    }

    void SceneTracker::ReloadIntersector(Scene const& scene, ClwScene& inout) const
    {
        m_api->DetachAll();

        for (auto& s : inout.isect_shapes)
        {
            m_api->AttachShape(s);
        }

        // Commit to intersector
        auto startime = std::chrono::high_resolution_clock::now();

        m_api->Commit();

        auto delta = std::chrono::high_resolution_clock::now() - startime;

        std::cout << "Commited in " << std::chrono::duration_cast<std::chrono::milliseconds>(delta).count() / 1000.f << "s\n";
    }

    void SceneTracker::BakeTextures(Scene const& scene, ClwScene& out) const
    {
        if (scene.textures_.size() > 0)
        {
            // Evaluate data size
            size_t datasize = 0;
            int alignment = 16;
            for (auto iter = scene.textures_.cbegin(); iter != scene.textures_.cend(); ++iter)
            {
                if (datasize % alignment != 0)
                    datasize += alignment - datasize % alignment;
                datasize += iter->size;
            }
            // Texture descriptors
            out.textures = m_context.CreateBuffer<Scene::Texture>(scene.textures_.size(), CL_MEM_READ_ONLY);
            m_vidmem_usage += scene.textures_.size() * sizeof(Scene::Texture);

            // Texture data
            out.texturedata = m_context.CreateBuffer<char>(datasize, CL_MEM_READ_ONLY);
            m_vidmem_usage += datasize;

            // Map both buffers
            Scene::Texture* mappeddesc = nullptr;
            char* mappeddata = nullptr;
            Scene::Texture* mappeddesc_orig = nullptr;
            char* mappeddata_orig = nullptr;

            m_context.MapBuffer(0, out.textures, CL_MAP_WRITE, &mappeddesc).Wait();
            m_context.MapBuffer(0, out.texturedata, CL_MAP_WRITE, &mappeddata).Wait();

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

                //alignment
                uintptr_t shift = alignment - (uintptr_t)mappeddata % alignment;
                if (shift != alignment)
                {
                    mappeddata += shift;
                    current_offset += shift;
                }
            }

            m_context.UnmapBuffer(0, out.textures, mappeddesc_orig).Wait();
            m_context.UnmapBuffer(0, out.texturedata, mappeddata_orig).Wait();
        }
        else
        {
            // Create stub
            out.textures = m_context.CreateBuffer<Scene::Texture>(1, CL_MEM_READ_ONLY);
            m_vidmem_usage += sizeof(Scene::Texture);

            // Texture data
            out.texturedata = m_context.CreateBuffer<char>(1, CL_MEM_READ_ONLY);
            m_vidmem_usage += 1;
        }
    }
}
