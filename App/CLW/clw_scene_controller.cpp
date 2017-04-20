#include "CLW/clw_scene_controller.h"
#include "SceneGraph/scene1.h"
#include "SceneGraph/camera.h"
#include "SceneGraph/light.h"
#include "SceneGraph/shape.h"
#include "SceneGraph/material.h"
#include "SceneGraph/texture.h"
#include "SceneGraph/Collector/collector.h"
#include "SceneGraph/iterator.h"

#include <chrono>
#include <memory>
#include <stack>
#include <vector>
#include <array>

using namespace RadeonRays;

namespace Baikal
{
    ClwSceneController::ClwSceneController(CLWContext context, int devidx)
        : m_default_material(new SingleBxdf(SingleBxdf::BxdfType::kLambert)),
        m_context(context)
    {
        // Get raw CL data out of CLW context
        cl_device_id id = m_context.GetDevice(devidx).GetID();
        cl_command_queue queue = m_context.GetCommandQueue(devidx);

        // Create intersection API
        m_api = CreateFromOpenClContext(m_context, id, queue);

        m_api->SetOption("acc.type", "fatbvh");
        m_api->SetOption("bvh.builder", "sah");
        m_api->SetOption("bvh.sah.num_bins", 64.f);
    }
    
    Material const* ClwSceneController::GetDefaultMaterial() const
    {
        return m_default_material.get();
    }

    ClwSceneController::~ClwSceneController()
    {
        // Delete API
        IntersectionApi::Delete(m_api);
    }

    static void SplitMeshesAndInstances(Iterator* shape_iter, std::set<Mesh const*>& meshes, std::set<Instance const*>& instances, std::set<Mesh const*>& excluded_meshes)
    {
        // Clear all sets
        meshes.clear();
        instances.clear();
        excluded_meshes.clear();

        // Prepare instance check lambda
        auto is_instance = [](Shape const* shape)
        {
            if (dynamic_cast<Instance const*>(shape))
            {
                return true;
            }
            else
            {
                return false;
            }
        };

        for (; shape_iter->IsValid(); shape_iter->Next())
        {
            auto shape = shape_iter->ItemAs<Shape const>();

            if (!is_instance(shape))
            {
                meshes.emplace(static_cast<Mesh const*>(shape));
            }
            else
            {
                instances.emplace(static_cast<Instance const*>(shape));
            }
        }

        for (auto& i : instances)
        {
            auto base_mesh = static_cast<Mesh const*>(i->GetBaseShape());
            if (meshes.find(base_mesh) == meshes.cend())
            {
                excluded_meshes.emplace(base_mesh);
            }
        }
    }

    static std::size_t GetShapeIdx(Iterator* shape_iter, Shape const* shape)
    {
        std::set<Mesh const*> meshes;
        std::set<Mesh const*> excluded_meshes;
        std::set<Instance const*> instances;
        SplitMeshesAndInstances(shape_iter, meshes, instances, excluded_meshes);

        std::size_t idx = 0;
        for (auto& i : meshes)
        {
            if (i == shape)
            {
                return idx;
            }

            ++idx;
        }

        for (auto& i : excluded_meshes)
        {
            if (i == shape)
            {
                return idx;
            }

            ++idx;
        }

        for (auto& i : instances)
        {
            if (i == shape)
            {
                return idx;
            }

            ++idx;
        }

        return -1;
    }

    void ClwSceneController::UpdateIntersector(Scene1 const& scene, ClwScene& out) const
    {
        // Detach and delete all shapes
        for (auto& shape : out.isect_shapes)
        {
            m_api->DetachShape(shape);
            m_api->DeleteShape(shape);
        }

        // Clear shapes cache
        out.isect_shapes.clear();
        // Only visible shapes are attached to the API.
        // So excluded meshes are pushed into isect_shapes, but
        // not to visible_shapes.
        out.visible_shapes.clear();

        // Create new shapes
        std::unique_ptr<Iterator> shape_iter(scene.CreateShapeIterator());

        if (!shape_iter->IsValid())
        {
            throw std::runtime_error("No shapes in the scene");
        }

        // Split all shapes into meshes and instances sets.
        std::set<Mesh const*> meshes;
        // Excluded shapes are shapes which are not in the scene,
        // but references by at least one instance.
        std::set<Mesh const*> excluded_meshes;
        std::set<Instance const*> instances;
        SplitMeshesAndInstances(shape_iter.get(), meshes, instances, excluded_meshes);

        // Keep shape->rr shape association for 
        // instance base shape lookup.
        std::map<Shape const*, RadeonRays::Shape*> rr_shapes;

        // Start from ID 1
        // Handle meshes
        int id = 1;
        for (auto& iter : meshes)
        {
            auto mesh = iter;

            auto shape = m_api->CreateMesh(
                // Vertices starting from the first one
                (float*)mesh->GetVertices(),
                // Number of vertices
                static_cast<int>(mesh->GetNumVertices()),
                // Stride
                sizeof(float3),
                // TODO: make API signature const
                reinterpret_cast<int const*>(mesh->GetIndices()),
                // Index stride
                0,
                // All triangles
                nullptr,
                // Number of primitives
                static_cast<int>(mesh->GetNumIndices() / 3)
            );

            auto transform = mesh->GetTransform();
            shape->SetTransform(transform, inverse(transform));
            shape->SetId(id++);
            out.isect_shapes.push_back(shape);
            out.visible_shapes.push_back(shape);
            rr_shapes[mesh] = shape;
        }

        // Handle excluded meshes
        for (auto& iter : excluded_meshes)
        {
            auto mesh = iter;

            auto shape = m_api->CreateMesh(
                // Vertices starting from the first one
                (float*)mesh->GetVertices(),
                // Number of vertices
                static_cast<int>(mesh->GetNumVertices()),
                // Stride
                sizeof(float3),
                // TODO: make API signature const
                reinterpret_cast<int const*>(mesh->GetIndices()),
                // Index stride
                0,
                // All triangles
                nullptr,
                // Number of primitives
                static_cast<int>(mesh->GetNumIndices() / 3)
            );

            auto transform = mesh->GetTransform();
            shape->SetTransform(transform, inverse(transform));
            shape->SetId(id++);
            out.isect_shapes.push_back(shape);
            rr_shapes[mesh] = shape;
        }

        // Handle instances
        for (auto& iter: instances)
        {
            auto instance = iter;
            auto rr_mesh = rr_shapes[instance->GetBaseShape()];
            auto shape = m_api->CreateInstance(rr_mesh);

            auto transform = instance->GetTransform();
            shape->SetTransform(transform, inverse(transform));
            shape->SetId(id++);
            out.isect_shapes.push_back(shape);
            out.visible_shapes.push_back(shape);
        }
    }

    void ClwSceneController::UpdateCamera(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const
    {
        // TODO: support different camera types here
        auto camera = static_cast<PerspectiveCamera const*>(scene.GetCamera());
        
        // Create light buffer if needed
        if (out.camera.GetElementCount() == 0)
        {
            out.camera = m_context.CreateBuffer<ClwScene::Camera>(1, CL_MEM_READ_ONLY);
        }
        
        // TODO: remove this
        out.camera_type = camera->GetAperture() > 0.f ? CameraType::kPhysical : CameraType::kDefault;

        // Update camera data
        ClwScene::Camera* data = nullptr;

        // Map GPU camera buffer
        m_context.MapBuffer(0, out.camera, CL_MAP_WRITE, &data).Wait();

        // Copy camera parameters
        data->forward = camera->GetForwardVector();
        data->up = camera->GetUpVector();
        data->right = camera->GetRightVector();
        data->p = camera->GetPosition();
        data->aperture = camera->GetAperture();
        data->aspect_ratio = camera->GetAspectRatio();
        data->dim = camera->GetSensorSize();
        data->focal_length = camera->GetFocalLength();
        data->focus_distance = camera->GetFocusDistance();
        data->zcap = camera->GetDepthRange();

        // Unmap camera buffer
        m_context.UnmapBuffer(0, out.camera, data);

        // Drop camera dirty flag
        camera->SetDirty(false);
    }

    void ClwSceneController::UpdateShapes(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const
    {
        std::size_t num_vertices = 0;
        std::size_t num_normals = 0;
        std::size_t num_uvs = 0;
        std::size_t num_indices = 0;
        std::size_t num_material_ids = 0;

        std::size_t num_vertices_written = 0;
        std::size_t num_normals_written = 0;
        std::size_t num_uvs_written = 0;
        std::size_t num_indices_written = 0;
        std::size_t num_matids_written = 0;
        std::size_t num_shapes_written = 0;

        std::unique_ptr<Iterator> shape_iter(scene.CreateShapeIterator());

        // Sort shapes into meshes and instances sets.
        std::set<Mesh const*> meshes;
        // Excluded meshes are meshes which are not in the scene, 
        // but are references by at least one instance.
        std::set<Mesh const*> excluded_meshes;
        std::set<Instance const*> instances;
        SplitMeshesAndInstances(shape_iter.get(), meshes, instances, excluded_meshes);

        // Calculate GPU array sizes. Do that only for meshes,
        // since instances do not occupy space in vertex buffers.
        // However instances still have their own material ids.
        for (auto& iter : meshes)
        {
            auto mesh = iter;

            num_vertices += mesh->GetNumVertices();
            num_normals += mesh->GetNumNormals();
            num_uvs += mesh->GetNumUVs();
            num_indices += mesh->GetNumIndices();
            num_material_ids += mesh->GetNumIndices() / 3;
        }

        // Excluded meshes still occupy space in vertex buffers.
        for (auto& iter : excluded_meshes)
        {
            auto mesh = iter;

            num_vertices += mesh->GetNumVertices();
            num_normals += mesh->GetNumNormals();
            num_uvs += mesh->GetNumUVs();
            num_indices += mesh->GetNumIndices();
            num_material_ids += mesh->GetNumIndices() / 3;
        }

        // Instances only occupy material IDs space.
        for (auto& iter : instances)
        {
            auto instance = iter;
            auto mesh = static_cast<Mesh const*>(instance->GetBaseShape());
            num_material_ids += mesh->GetNumIndices() / 3;
        }

        // Create CL arrays
        out.vertices = m_context.CreateBuffer<float3>(num_vertices, CL_MEM_READ_ONLY);
        out.normals = m_context.CreateBuffer<float3>(num_normals, CL_MEM_READ_ONLY);
        out.uvs = m_context.CreateBuffer<float2>(num_uvs, CL_MEM_READ_ONLY);
        out.indices = m_context.CreateBuffer<int>(num_indices, CL_MEM_READ_ONLY);

        // Total number of entries in shapes GPU array
        auto num_shapes = meshes.size() + excluded_meshes.size() + instances.size();
        out.shapes = m_context.CreateBuffer<ClwScene::Shape>(num_shapes, CL_MEM_READ_ONLY);
        out.materialids = m_context.CreateBuffer<int>(num_material_ids, CL_MEM_READ_ONLY);

        float3* vertices = nullptr;
        float3* normals = nullptr;
        float2* uvs = nullptr;
        int* indices = nullptr;
        int* matids = nullptr;
        ClwScene::Shape* shapes = nullptr;

        // Map arrays and prepare to write data
        m_context.MapBuffer(0, out.vertices, CL_MAP_WRITE, &vertices);
        m_context.MapBuffer(0, out.normals, CL_MAP_WRITE, &normals);
        m_context.MapBuffer(0, out.uvs, CL_MAP_WRITE, &uvs);
        m_context.MapBuffer(0, out.indices, CL_MAP_WRITE, &indices);
        m_context.MapBuffer(0, out.materialids, CL_MAP_WRITE, &matids);
        m_context.MapBuffer(0, out.shapes, CL_MAP_WRITE, &shapes).Wait();

        // Keep associated shapes data for instance look up.
        // We retrieve data from here while serializing instances,
        // using base shape lookup.
        std::map<Mesh const*, ClwScene::Shape> shape_data;
        // Handle meshes
        for (auto& iter : meshes)
        {
            auto mesh = iter;

            // Get pointers data
            auto mesh_vertex_array = mesh->GetVertices();
            auto mesh_num_vertices = mesh->GetNumVertices();

            auto mesh_normal_array = mesh->GetNormals();
            auto mesh_num_normals = mesh->GetNumNormals();

            auto mesh_uv_array = mesh->GetUVs();
            auto mesh_num_uvs = mesh->GetNumUVs();

            auto mesh_index_array = mesh->GetIndices();
            auto mesh_num_indices = mesh->GetNumIndices();

            // Prepare shape descriptor
            ClwScene::Shape shape;
            shape.numprims = static_cast<int>(mesh_num_indices / 3);
            shape.startvtx = static_cast<int>(num_vertices_written);
            shape.startidx = static_cast<int>(num_indices_written);
            shape.start_material_idx = static_cast<int>(num_matids_written);

            auto transform = mesh->GetTransform();
            shape.transform.m0 = { transform.m00, transform.m01, transform.m02, transform.m03 };
            shape.transform.m1 = { transform.m10, transform.m11, transform.m12, transform.m13 };
            shape.transform.m2 = { transform.m20, transform.m21, transform.m22, transform.m23 };
            shape.transform.m3 = { transform.m30, transform.m31, transform.m32, transform.m33 };

            shape.linearvelocity = float3(0.0f, 0.f, 0.f);
            shape.angularvelocity = float3(0.f, 0.f, 0.f, 1.f);

            shape_data[mesh] = shape;

            std::copy(mesh_vertex_array, mesh_vertex_array + mesh_num_vertices, vertices + num_vertices_written);
            num_vertices_written += mesh_num_vertices;

            std::copy(mesh_normal_array, mesh_normal_array + mesh_num_normals, normals + num_normals_written);
            num_normals_written += mesh_num_normals;

            std::copy(mesh_uv_array, mesh_uv_array + mesh_num_uvs, uvs + num_uvs_written);
            num_uvs_written += mesh_num_uvs;

            std::copy(mesh_index_array, mesh_index_array + mesh_num_indices, indices + num_indices_written);
            num_indices_written += mesh_num_indices;

            shapes[num_shapes_written++] = shape;

            // Check if mesh has a material and use default if not
            auto material = mesh->GetMaterial();
            if (!material)
            {
                material = m_default_material.get();
            }

            auto matidx = mat_collector.GetItemIndex(material);
            std::fill(matids + num_matids_written, matids + num_matids_written + mesh_num_indices / 3, matidx);

            num_matids_written += mesh_num_indices / 3;

            // Drop dirty flag
            mesh->SetDirty(false);
        }

        // Excluded shapes are handled in almost the same way
        // except materials.
        for (auto& iter : excluded_meshes)
        {
            auto mesh = iter;

            // Get pointers data
            auto mesh_vertex_array = mesh->GetVertices();
            auto mesh_num_vertices = mesh->GetNumVertices();

            auto mesh_normal_array = mesh->GetNormals();
            auto mesh_num_normals = mesh->GetNumNormals();

            auto mesh_uv_array = mesh->GetUVs();
            auto mesh_num_uvs = mesh->GetNumUVs();

            auto mesh_index_array = mesh->GetIndices();
            auto mesh_num_indices = mesh->GetNumIndices();

            // Prepare shape descriptor
            ClwScene::Shape shape;
            shape.numprims = static_cast<int>(mesh_num_indices / 3);
            shape.startvtx = static_cast<int>(num_vertices_written);
            shape.startidx = static_cast<int>(num_indices_written);
            shape.start_material_idx = static_cast<int>(num_matids_written);

            auto transform = mesh->GetTransform();
            shape.transform.m0 = { transform.m00, transform.m01, transform.m02, transform.m03 };
            shape.transform.m1 = { transform.m10, transform.m11, transform.m12, transform.m13 };
            shape.transform.m2 = { transform.m20, transform.m21, transform.m22, transform.m23 };
            shape.transform.m3 = { transform.m30, transform.m31, transform.m32, transform.m33 };

            shape.linearvelocity = float3(0.0f, 0.f, 0.f);
            shape.angularvelocity = float3(0.f, 0.f, 0.f, 1.f);

            shape_data[mesh] = shape;

            std::copy(mesh_vertex_array, mesh_vertex_array + mesh_num_vertices, vertices + num_vertices_written);
            num_vertices_written += mesh_num_vertices;

            std::copy(mesh_normal_array, mesh_normal_array + mesh_num_normals, normals + num_normals_written);
            num_normals_written += mesh_num_normals;

            std::copy(mesh_uv_array, mesh_uv_array + mesh_num_uvs, uvs + num_uvs_written);
            num_uvs_written += mesh_num_uvs;

            std::copy(mesh_index_array, mesh_index_array + mesh_num_indices, indices + num_indices_written);
            num_indices_written += mesh_num_indices;

            shapes[num_shapes_written++] = shape;

            // We do not need materials for excluded shapes, we never shade them.
            std::fill(matids + num_matids_written, matids + num_matids_written + mesh_num_indices / 3, -1);

            num_matids_written += mesh_num_indices / 3;

            // Drop dirty flag
            mesh->SetDirty(false);
        }

        // Handle instances
        for (auto& iter : instances)
        {
            auto instance = iter;
            auto base_shape = static_cast<Mesh const*>(instance->GetBaseShape());
            auto material = instance->GetMaterial();
            auto transform = instance->GetTransform();
            auto mesh_num_indices = base_shape->GetNumIndices();

            // Here shape_data is guaranteed to contain
            // info for base_shape since we have serialized it
            // above in a different pass.
            ClwScene::Shape shape = shape_data[base_shape];
            // Instance has its own material part.
            shape.start_material_idx = static_cast<int>(num_matids_written);

            // Instance has its own transform.
            shape.transform.m0 = { transform.m00, transform.m01, transform.m02, transform.m03 };
            shape.transform.m1 = { transform.m10, transform.m11, transform.m12, transform.m13 };
            shape.transform.m2 = { transform.m20, transform.m21, transform.m22, transform.m23 };
            shape.transform.m3 = { transform.m30, transform.m31, transform.m32, transform.m33 };

            shape.linearvelocity = float3(0.0f, 0.f, 0.f);
            shape.angularvelocity = float3(0.f, 0.f, 0.f, 1.f);

            shapes[num_shapes_written++] = shape;

            // If instance do not have a material, use default one.
            if (!material)
            {
                material = m_default_material.get();
            }

            auto mat_idx = mat_collector.GetItemIndex(material);
            std::fill(matids + num_matids_written, matids + num_matids_written + mesh_num_indices / 3, mat_idx);

            num_matids_written += mesh_num_indices / 3;

            // Drop dirty flag
            instance->SetDirty(false);
        }

        m_context.UnmapBuffer(0, out.vertices, vertices);
        m_context.UnmapBuffer(0, out.normals, normals);
        m_context.UnmapBuffer(0, out.uvs, uvs);
        m_context.UnmapBuffer(0, out.indices, indices);
        m_context.UnmapBuffer(0, out.materialids, matids);
        m_context.UnmapBuffer(0, out.shapes, shapes).Wait();
        
        UpdateIntersector(scene, out);
        
        ReloadIntersector(scene, out);
    }
    
    void ClwSceneController::UpdateCurrentScene(Scene1 const& scene, ClwScene& out) const
    {
        ReloadIntersector(scene, out);
    }

    void ClwSceneController::UpdateMaterials(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const
    {
        // Get new buffer size
        std::size_t mat_buffer_size = mat_collector.GetNumItems();

        // Recreate material buffer if it needs resize
        if (mat_buffer_size > out.materials.GetElementCount())
        {
            // Create material buffer
            out.materials = m_context.CreateBuffer<ClwScene::Material>(mat_buffer_size, CL_MEM_READ_ONLY);
        }

        ClwScene::Material* materials = nullptr;
        std::size_t num_materials_written = 0;

        // Map GPU materials buffer
        m_context.MapBuffer(0, out.materials, CL_MAP_WRITE, &materials).Wait();

        // Serialize
        {
            // Update material bundle first to be able to track differences
            out.material_bundle.reset(mat_collector.CreateBundle());

            // Create material iterator
            std::unique_ptr<Iterator> mat_iter(mat_collector.CreateIterator());

            // Iterate and serialize
            for (; mat_iter->IsValid(); mat_iter->Next())
            {
                WriteMaterial(mat_iter->ItemAs<Material const>(), mat_collector, tex_collector, materials + num_materials_written);
                ++num_materials_written;
            }
        }

        // Unmap material buffer
        m_context.UnmapBuffer(0, out.materials, materials);
    }

    void ClwSceneController::ReloadIntersector(Scene1 const& scene, ClwScene& inout) const
    {
        m_api->DetachAll();

        for (auto& s : inout.visible_shapes)
        {
            m_api->AttachShape(s);
        }

        m_api->Commit();
    }

    void ClwSceneController::UpdateTextures(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const
    {
        // Get new buffer size
        std::size_t tex_buffer_size = tex_collector.GetNumItems();
        std::size_t tex_data_buffer_size = 0;

        if (tex_buffer_size == 0)
        {
            out.textures = m_context.CreateBuffer<ClwScene::Texture>(1, CL_MEM_READ_ONLY);
            out.texturedata = m_context.CreateBuffer<char>(1, CL_MEM_READ_ONLY);
            return;
        }

        // Recreate material buffer if it needs resize
        if (tex_buffer_size > out.textures.GetElementCount())
        {
            // Create material buffer
            out.textures = m_context.CreateBuffer<ClwScene::Texture>(tex_buffer_size, CL_MEM_READ_ONLY);
        }

        ClwScene::Texture* textures = nullptr;
        std::size_t num_textures_written = 0;

        // Map GPU materials buffer
        m_context.MapBuffer(0, out.textures, CL_MAP_WRITE, &textures).Wait();

        // Update material bundle first to be able to track differences
        out.texture_bundle.reset(tex_collector.CreateBundle());

        // Create material iterator
        std::unique_ptr<Iterator> tex_iter(tex_collector.CreateIterator());

        // Iterate and serialize
        for (; tex_iter->IsValid(); tex_iter->Next())
        {
            auto tex = tex_iter->ItemAs<Texture const>();

            WriteTexture(tex, tex_data_buffer_size, textures + num_textures_written);

            ++num_textures_written;

            tex_data_buffer_size += tex->GetSizeInBytes();
        }

        // Unmap material buffer
        m_context.UnmapBuffer(0, out.textures, textures);

        // Recreate material buffer if it needs resize
        if (tex_data_buffer_size > out.texturedata.GetElementCount())
        {
            // Create material buffer
            out.texturedata = m_context.CreateBuffer<char>(tex_data_buffer_size, CL_MEM_READ_ONLY);
        }

        char* data = nullptr;
        std::size_t num_bytes_written = 0;

        tex_iter->Reset();

        // Map GPU materials buffer
        m_context.MapBuffer(0, out.texturedata, CL_MAP_WRITE, &data).Wait();

        // Write texture data for all textures
        for (; tex_iter->IsValid(); tex_iter->Next())
        {
            auto tex = tex_iter->ItemAs<Texture const>();

            WriteTextureData(tex, data + num_bytes_written);

            num_bytes_written += tex->GetSizeInBytes();
        }

        // Unmap material buffer
        m_context.UnmapBuffer(0, out.texturedata, data);
    }

    // Convert Material:: types to ClwScene:: types
    static ClwScene::Bxdf GetMaterialType(Material const* material)
    {
        // Distinguish between single bxdf materials and compound ones
        if (auto bxdf = dynamic_cast<SingleBxdf const*>(material))
        {
            switch (bxdf->GetBxdfType())
            {
            case SingleBxdf::BxdfType::kZero: return ClwScene::Bxdf::kZero;
            case SingleBxdf::BxdfType::kLambert: return ClwScene::Bxdf::kLambert;
            case SingleBxdf::BxdfType::kEmissive: return ClwScene::Bxdf::kEmissive;
            case SingleBxdf::BxdfType::kPassthrough: return ClwScene::Bxdf::kPassthrough;
            case SingleBxdf::BxdfType::kTranslucent: return ClwScene::Bxdf::kTranslucent;
            case SingleBxdf::BxdfType::kIdealReflect: return ClwScene::Bxdf::kIdealReflect;
            case SingleBxdf::BxdfType::kIdealRefract: return ClwScene::Bxdf::kIdealRefract;
            case SingleBxdf::BxdfType::kMicrofacetGGX: return ClwScene::Bxdf::kMicrofacetGGX;
            case SingleBxdf::BxdfType::kMicrofacetBeckmann: return ClwScene::Bxdf::kMicrofacetBeckmann;
            case SingleBxdf::BxdfType::kMicrofacetRefractionGGX: return ClwScene::Bxdf::kMicrofacetRefractionGGX;
            case SingleBxdf::BxdfType::kMicrofacetRefractionBeckmann: return ClwScene::Bxdf::kMicrofacetRefractionBeckmann;
            }
        }
        else if (auto mat = dynamic_cast<MultiBxdf const*>(material))
        {
            switch (mat->GetType())
            {
            case MultiBxdf::Type::kMix: return ClwScene::Bxdf::kMix;
            case MultiBxdf::Type::kLayered: return ClwScene::Bxdf::kLayered;
            case MultiBxdf::Type::kFresnelBlend: return ClwScene::Bxdf::kFresnelBlend;
            }
        }
        else
        {
            return ClwScene::Bxdf::kZero;
        }

        return ClwScene::Bxdf::kZero;
    }

    void ClwSceneController::WriteMaterial(Material const* material, Collector& mat_collector, Collector& tex_collector, void* data) const
    {
        auto clw_material = reinterpret_cast<ClwScene::Material*>(data);

        // Convert material type and sidedness
        auto type = GetMaterialType(material);

        clw_material->type = type;
        clw_material->thin = material->IsThin() ? 1 : 0;

        switch (type)
        {
        case ClwScene::Bxdf::kZero:
            clw_material->kx = RadeonRays::float4();
            break;

            // We need to convert roughness for the following materials
        case ClwScene::Bxdf::kMicrofacetGGX:
        case ClwScene::Bxdf::kMicrofacetBeckmann:
        case ClwScene::Bxdf::kMicrofacetRefractionGGX:
        case ClwScene::Bxdf::kMicrofacetRefractionBeckmann:
        {
            Material::InputValue value = material->GetInputValue("roughness");

            if (value.type == Material::InputType::kFloat4)
            {
                clw_material->ns = value.float_value.x;
                clw_material->nsmapidx = -1;
            }
            else if (value.type == Material::InputType::kTexture)
            {
                clw_material->nsmapidx = value.tex_value ? tex_collector.GetItemIndex(value.tex_value) : -1;
            }
            else
            {
                // TODO: should not happen
                assert(false);
            }

            // Intentionally missing break here
        }

        // For the rest we need to conver albedo, normal map, fresnel factor, ior
        case ClwScene::Bxdf::kLambert:
        case ClwScene::Bxdf::kEmissive:
        case ClwScene::Bxdf::kPassthrough:
        case ClwScene::Bxdf::kTranslucent:
        case ClwScene::Bxdf::kIdealRefract:
        case ClwScene::Bxdf::kIdealReflect:
        {
            Material::InputValue value = material->GetInputValue("albedo");

            if (value.type == Material::InputType::kFloat4)
            {
                clw_material->kx = value.float_value;
                clw_material->kxmapidx = -1;
            }
            else if (value.type == Material::InputType::kTexture)
            {
                clw_material->kxmapidx = value.tex_value ? tex_collector.GetItemIndex(value.tex_value) : -1;
            }
            else
            {
                // TODO: should not happen
                assert(false);
            }

            value = material->GetInputValue("normal");

            if (value.type == Material::InputType::kTexture && value.tex_value)
            {
                clw_material->nmapidx = tex_collector.GetItemIndex(value.tex_value);
                clw_material->bump_flag = 0;
            }
            else
            {
                value = material->GetInputValue("bump");

                if (value.type == Material::InputType::kTexture && value.tex_value)
                {
                    clw_material->nmapidx = tex_collector.GetItemIndex(value.tex_value);
                    clw_material->bump_flag = 1;
                }
                else
                {
                    clw_material->nmapidx = -1;
                    clw_material->bump_flag = 0;
                }
            }

            value = material->GetInputValue("fresnel");

            if (value.type == Material::InputType::kFloat4)
            {
                clw_material->fresnel = value.float_value.x > 0 ? 1.f : 0.f;
            }
            else
            {
                clw_material->fresnel = 0.f;
            }

            value = material->GetInputValue("ior");

            if (value.type == Material::InputType::kFloat4)
            {
                clw_material->ni = value.float_value.x;
            }
            else
            {
                clw_material->ni = 1.f;
            }

            value = material->GetInputValue("roughness");

            if (value.type == Material::InputType::kFloat4)
            {
                clw_material->ns = value.float_value.x;
            }
            else
            {
                clw_material->ns = 0.99f;
            }

            break;
        }

        // For compound materials we need to convert dependencies
        // and weights.
        case ClwScene::Bxdf::kMix:
        case ClwScene::Bxdf::kFresnelBlend:
        {
            Material::InputValue value0 = material->GetInputValue("base_material");
            Material::InputValue value1 = material->GetInputValue("top_material");

            if (value0.type == Material::InputType::kMaterial &&
                value1.type == Material::InputType::kMaterial)
            {
                clw_material->brdfbaseidx = mat_collector.GetItemIndex(value0.mat_value);
                clw_material->brdftopidx = mat_collector.GetItemIndex(value1.mat_value);
            }
            else
            {
                // Should not happen
                assert(false);
            }

            if (type == ClwScene::Bxdf::kMix)
            {
                clw_material->fresnel = 0.f;

                Material::InputValue value = material->GetInputValue("weight");

                if (value.type == Material::InputType::kTexture)
                {
                    clw_material->nsmapidx = tex_collector.GetItemIndex(value.tex_value);
                }
                else
                {
                    clw_material->nsmapidx = -1;
                    clw_material->ns = value.float_value.x;
                }
            }
            else
            {
                clw_material->fresnel = 1.f;

                Material::InputValue value = material->GetInputValue("ior");

                if (value.type == Material::InputType::kFloat4)
                {
                    clw_material->ni = value.float_value.x;
                }
                else
                {
                    // Should not happen
                    assert(false);
                }
            }
        }

        default:
            break;
        }

        material->SetDirty(false);
    }

    // Convert Light:: types to ClwScene:: types
    static int GetLightType(Light const* light)
    {

        if (dynamic_cast<PointLight const*>(light))
        {
            return ClwScene::kPoint;
        }
        else if (dynamic_cast<DirectionalLight const*>(light))
        {
            return ClwScene::kDirectional;
        }
        else if (dynamic_cast<SpotLight const*>(light))
        {
            return ClwScene::kSpot;
        }
        else if (dynamic_cast<ImageBasedLight const*>(light))
        {
            return ClwScene::kIbl;
        }
        else
        {
            return ClwScene::LightType::kArea;
        }
    }

    void ClwSceneController::WriteLight(Scene1 const& scene, Light const* light, Collector& tex_collector, void* data) const
    {
        auto clw_light = reinterpret_cast<ClwScene::Light*>(data);

        auto type = GetLightType(light);

        clw_light->type = type;

        switch (type)
        {
        case ClwScene::kPoint:
        {
            clw_light->p = light->GetPosition();
            clw_light->intensity = light->GetEmittedRadiance();
            break;
        }

        case ClwScene::kDirectional:
        {
            clw_light->d = light->GetDirection();
            clw_light->intensity = light->GetEmittedRadiance();
            break;
        }

        case ClwScene::kSpot:
        {
            clw_light->p = light->GetPosition();
            clw_light->d = light->GetDirection();
            clw_light->intensity = light->GetEmittedRadiance();

            auto cone_shape = static_cast<SpotLight const*>(light)->GetConeShape();
            clw_light->ia = cone_shape.x;
            clw_light->oa = cone_shape.y;
            break;
        }

        case ClwScene::kIbl:
        {
            // TODO: support this
            clw_light->multiplier = static_cast<ImageBasedLight const*>(light)->GetMultiplier();
            auto tex = static_cast<ImageBasedLight const*>(light)->GetTexture();
            clw_light->tex = tex_collector.GetItemIndex(tex);
            clw_light->texdiffuse = clw_light->tex;
            break;
        }

        case ClwScene::kArea:
        {
            // TODO: optimize this linear search
            auto shape = static_cast<AreaLight const*>(light)->GetShape();

            std::unique_ptr<Iterator> shape_iter(scene.CreateShapeIterator());

            auto idx = GetShapeIdx(shape_iter.get(), shape);

            clw_light->shapeidx = static_cast<int>(idx);
            clw_light->primidx = static_cast<int>(static_cast<AreaLight const*>(light)->GetPrimitiveIdx());
            break;
        }


        default:
            assert(false);
            break;
        }
    }

    void ClwSceneController::UpdateLights(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, ClwScene& out) const
    {
        std::size_t num_lights_written = 0;

        auto num_lights = scene.GetNumLights();

        // Create light buffer if needed
        if (num_lights > out.lights.GetElementCount())
        {
            out.lights = m_context.CreateBuffer<ClwScene::Light>(num_lights, CL_MEM_READ_ONLY);
        }

        ClwScene::Light* lights = nullptr;

        m_context.MapBuffer(0, out.lights, CL_MAP_WRITE, &lights).Wait();

        std::unique_ptr<Iterator> light_iter(scene.CreateLightIterator());

        // Disable IBL by default
        out.envmapidx = -1;

        // Serialize
        {
            for (; light_iter->IsValid(); light_iter->Next())
            {
                auto light = light_iter->ItemAs<Light const>();
                WriteLight(scene, light, tex_collector, lights + num_lights_written);
                ++num_lights_written;

                // Find and update IBL idx
                auto ibl = dynamic_cast<ImageBasedLight const*>(light_iter->ItemAs<Light const>());
                if (ibl)
                {
                    out.envmapidx = static_cast<int>(num_lights_written - 1);
                }

                light->SetDirty(false);
            }
        }

        m_context.UnmapBuffer(0, out.lights, lights);

        out.num_lights = static_cast<int>(num_lights_written);
    }


    // Convert texture format into ClwScene:: types
    static ClwScene::TextureFormat GetTextureFormat(Texture const* texture)
    {
        switch (texture->GetFormat())
        {
        case Texture::Format::kRgba8: return ClwScene::TextureFormat::RGBA8;
        case Texture::Format::kRgba16: return ClwScene::TextureFormat::RGBA16;
        case Texture::Format::kRgba32: return ClwScene::TextureFormat::RGBA32;
        default: return ClwScene::TextureFormat::RGBA8;
        }
    }

    void ClwSceneController::WriteTexture(Texture const* texture, std::size_t data_offset, void* data) const
    {
        auto clw_texture = reinterpret_cast<ClwScene::Texture*>(data);

        auto dim = texture->GetSize();

        clw_texture->w = dim.x;
        clw_texture->h = dim.y;
        clw_texture->fmt = GetTextureFormat(texture);
        clw_texture->dataoffset = static_cast<int>(data_offset);
    }

    void ClwSceneController::WriteTextureData(Texture const* texture, void* data) const
    {
        auto begin = texture->GetData();
        auto end = begin + texture->GetSizeInBytes();
        std::copy(begin, end, static_cast<char*>(data));
    }
}
