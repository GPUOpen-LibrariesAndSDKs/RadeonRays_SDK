#include "gl_scene_controller.h"

#include <cassert>

#define CHECK_GL_ERROR {auto err = glGetError(); assert(err == GL_NO_ERROR);}

namespace Baikal
{
    GlSceneController::GlSceneController()
    {
    }

    GlSceneController::~GlSceneController()
    {
    }

    void GlSceneController::UpdateCamera(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const
    {
        auto camera = scene.GetCamera();
        out.view_transform = reinterpret_cast<PerspectiveCamera const*>(camera)->GetViewMatrix();
        out.proj_transform = reinterpret_cast<PerspectiveCamera const*>(camera)->GetProjectionMatrix();
    }

    void GlSceneController::UpdateShapes(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const
    {
        // Delete all meshes previously created
        for (auto& iter : out.shapes)
        {
            if (iter.second.num_triangles > 0)
            {
                glDeleteBuffers(1, &iter.second.index_buffer);
                glDeleteBuffers(1, &iter.second.vertex_buffer);
            }
        }

        std::unique_ptr<Iterator> shape_iter(scene.CreateShapeIterator());

        // Sort shapes into meshes and instances sets.
        std::set<Mesh const*> meshes;
        // Excluded meshes are meshes which are not in the scene, 
        // but are references by at least one instance.
        std::set<Mesh const*> excluded_meshes;
        std::set<Instance const*> instances;
        SplitMeshesAndInstances(shape_iter.get(), meshes, instances, excluded_meshes);

        for (auto& iter : meshes)
        {
            auto mesh = iter;

            GlShapeData new_data;
            glGenBuffers(1, &new_data.vertex_buffer);
            glGenBuffers(1, &new_data.index_buffer);

            new_data.transform = mesh->GetTransform().transpose();
            new_data.num_triangles = mesh->GetNumIndices() / 3;

            auto vertex_ptr = mesh->GetVertices();
            auto normal_ptr = mesh->GetNormals();
            auto uv_ptr = mesh->GetUVs();

            auto num_vertices = std::min(mesh->GetNumVertices(), mesh->GetNumNormals());

            std::vector<GlVertex> vertices(num_vertices);
            for (auto i = 0; i < num_vertices; ++i)
            {
                vertices[i].p = vertex_ptr[i];
                vertices[i].n = normal_ptr[i];
                vertices[i].uv = uv_ptr[i];
            }

            glBindBuffer(GL_ARRAY_BUFFER, new_data.vertex_buffer); CHECK_GL_ERROR;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, new_data.index_buffer); CHECK_GL_ERROR;

            glBufferData(GL_ARRAY_BUFFER,
                sizeof(GlVertex) * num_vertices,
                &vertices[0],
                GL_STATIC_DRAW); CHECK_GL_ERROR;

            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                sizeof(std::uint32_t) * (std::uint32_t)mesh->GetNumIndices(),
                mesh->GetIndices(),
                GL_STATIC_DRAW); CHECK_GL_ERROR;

            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

            out.shapes[mesh] = new_data;
        }
    }

    void GlSceneController::UpdateLights(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const
    {
    }

    void GlSceneController::UpdateMaterials(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const
    {
    }

    void GlSceneController::UpdateTextures(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, GlScene& out) const
    {
    }

    Material const* GlSceneController::GetDefaultMaterial() const
    {
        return nullptr;
    }

    void GlSceneController::UpdateCurrentScene(Scene1 const& scene, GlScene& out) const
    {

    }

}