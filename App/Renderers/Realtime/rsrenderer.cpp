#include "rsrenderer.h"
#include "Core/output.h"
#include "../PT/ptrenderer.h"
#include "../../CLW/clw_scene_controller.h"
#include "../../CLW/clwoutput.h"

#include <cassert>

#ifdef DEBUG
#define CHECK_GL_ERROR {auto err = glGetError(); assert(err == GL_NO_ERROR);}
#else
#define CHECK_GL_ERROR
#endif

namespace Baikal 
{
    GlOutput::GlOutput(std::uint32_t w, std::uint32_t h)
        : Output(w, h)
        , m_width(w)
        , m_height(h)
    {
        glGenFramebuffers(1, &m_frame_buffer); CHECK_GL_ERROR;
        glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer); CHECK_GL_ERROR;

        glGenRenderbuffers(1, &m_color_buffer); CHECK_GL_ERROR;
        glBindRenderbuffer(GL_RENDERBUFFER, m_color_buffer); CHECK_GL_ERROR;
        glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA16F, w, h); CHECK_GL_ERROR;
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_color_buffer); CHECK_GL_ERROR;

        glGenRenderbuffers(1, &m_depth_buffer); CHECK_GL_ERROR;
        glBindRenderbuffer(GL_RENDERBUFFER, m_depth_buffer); CHECK_GL_ERROR;
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h); CHECK_GL_ERROR;
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_buffer); CHECK_GL_ERROR;

        glBindFramebuffer(GL_FRAMEBUFFER, 0); CHECK_GL_ERROR;
        glBindRenderbuffer(GL_RENDERBUFFER, 0); CHECK_GL_ERROR;
    }

    GlOutput::~GlOutput()
    {
        glDeleteRenderbuffers(1, &m_color_buffer);
        glDeleteRenderbuffers(1, &m_depth_buffer);
        glDeleteFramebuffers(1, &m_frame_buffer);
    }

    void GlOutput::GetData(RadeonRays::float3* data) const
    {
    }

    void GlOutput::Clear(RadeonRays::float3 const& val)
    {
        glClearColor(val.x, val.y, val.z, 1.f); CHECK_GL_ERROR;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); CHECK_GL_ERROR;
    }

    GLuint GlOutput::GetGlFramebuffer() const
    {
        return m_frame_buffer;
    }


    struct RsRenderer::RenderData
    {
        GLuint quad_vertices;
        GLuint quad_indices;

        std::unique_ptr<PtRenderer> ray_tracer;
        Output* ray_tracer_output;
        std::unique_ptr<ClwSceneController> ray_tracer_controller;
        GLuint gl_interop_buffer;
        GLuint gl_interop_position_buffer;
        GLuint gl_interop_shadow_buffer;
        CLWImage2D cl_interop_buffer;
        CLWImage2D cl_interop_position_buffer;
        CLWImage2D cl_interop_shadow_buffer;
        CLWContext cl_context;

        GLuint frame_buffer_texture;
        GLuint depth_buffer;
        GLuint frame_buffer;
    };

    RsRenderer::RsRenderer()
        : m_render_data(new RenderData)
        , m_output(nullptr)
    {
        glGenBuffers(1, &m_render_data->quad_indices); CHECK_GL_ERROR;
        glGenBuffers(1, &m_render_data->quad_vertices); CHECK_GL_ERROR;

        // create Vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, m_render_data->quad_vertices); CHECK_GL_ERROR;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_render_data->quad_indices); CHECK_GL_ERROR;

        float quad_vdata[] =
        {
            -1, -1, 0.5, 0, 0,
            1, -1, 0.5, 1, 0,
            1, 1, 0.5, 1, 1,
            -1, 1, 0.5, 0, 1
        };

        GLshort quad_idata[] =
        {
            0, 1, 3,
            3, 1, 2
        };

        // fill data
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vdata), quad_vdata, GL_STATIC_DRAW); CHECK_GL_ERROR;
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quad_idata), quad_idata, GL_STATIC_DRAW); CHECK_GL_ERROR;

        glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_GL_ERROR;

        std::vector<CLWPlatform> platforms;
        CLWPlatform::CreateAllPlatforms(platforms);
        if (platforms[0].GetDevice(0).HasGlInterop())
        {
            cl_context_properties props[] =
            {
                //OpenCL platform
                CL_CONTEXT_PLATFORM, (cl_context_properties)((cl_platform_id)platforms[0]),
                //OpenGL context
                CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
                //HDC used to create the OpenGL context
                CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
                0
            };

            m_render_data->cl_context = CLWContext::Create(platforms[0].GetDevice(0), props);

            m_render_data->ray_tracer.reset(new PtRenderer(m_render_data->cl_context, 0, 3));
            m_render_data->ray_tracer_controller.reset(new ClwSceneController(m_render_data->cl_context, 0));
            m_render_data->ray_tracer_output = m_render_data->ray_tracer->CreateOutput(400, 300);
            m_render_data->ray_tracer->SetOutput(m_render_data->ray_tracer_output);

            // Create interop texture
            glGenTextures(1, &m_render_data->gl_interop_buffer); CHECK_GL_ERROR;
            glBindTexture(GL_TEXTURE_2D, m_render_data->gl_interop_buffer); CHECK_GL_ERROR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_GL_ERROR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_GL_ERROR;

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 400, 300, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); CHECK_GL_ERROR;

            glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;

            m_render_data->cl_interop_buffer = m_render_data->cl_context.CreateImage2DFromGLTexture(m_render_data->gl_interop_buffer);

            glGenTextures(1, &m_render_data->gl_interop_position_buffer); CHECK_GL_ERROR;
            glBindTexture(GL_TEXTURE_2D, m_render_data->gl_interop_position_buffer); CHECK_GL_ERROR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_GL_ERROR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_GL_ERROR;

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 400, 300, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); CHECK_GL_ERROR;

            glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;

            m_render_data->cl_interop_position_buffer = m_render_data->cl_context.CreateImage2DFromGLTexture(m_render_data->gl_interop_position_buffer);

            glGenTextures(1, &m_render_data->gl_interop_shadow_buffer); CHECK_GL_ERROR;
            glBindTexture(GL_TEXTURE_2D, m_render_data->gl_interop_shadow_buffer); CHECK_GL_ERROR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); CHECK_GL_ERROR;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); CHECK_GL_ERROR;

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 400, 300, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr); CHECK_GL_ERROR;

            glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;

            m_render_data->cl_interop_shadow_buffer = m_render_data->cl_context.CreateImage2DFromGLTexture(m_render_data->gl_interop_shadow_buffer);

            glGenFramebuffers(1, &m_render_data->frame_buffer); CHECK_GL_ERROR;
            glBindFramebuffer(GL_FRAMEBUFFER, m_render_data->frame_buffer); CHECK_GL_ERROR;

            glGenRenderbuffers(1, &m_render_data->frame_buffer); CHECK_GL_ERROR;
            glGenTextures(1, &m_render_data->frame_buffer_texture);
            glBindTexture(GL_TEXTURE_2D, m_render_data->frame_buffer_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 800, 600, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

            // Poor filtering. Needed !
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_render_data->frame_buffer_texture, 0); CHECK_GL_ERROR;

            glGenRenderbuffers(1, &m_render_data->depth_buffer); CHECK_GL_ERROR;
            glBindRenderbuffer(GL_RENDERBUFFER, m_render_data->depth_buffer); CHECK_GL_ERROR;
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 800, 600); CHECK_GL_ERROR;
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_render_data->depth_buffer); CHECK_GL_ERROR;
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindFramebuffer(GL_FRAMEBUFFER, 0); CHECK_GL_ERROR;
            glBindRenderbuffer(GL_RENDERBUFFER, 0); CHECK_GL_ERROR;
        }
        else
        {
            throw std::runtime_error("GL interop is not supported");
        }
    }

    RsRenderer::~RsRenderer()
    {
        glDeleteBuffers(1, &m_render_data->quad_indices);
        glDeleteBuffers(1, &m_render_data->quad_vertices);
    }

    Output* RsRenderer::CreateOutput(std::uint32_t w, std::uint32_t h) const
    {
        return new GlOutput(w, h);
    }

    void RsRenderer::DeleteOutput(Output* output) const
    {
        delete output;
    }

    void RsRenderer::Clear(RadeonRays::float3 const& val, Output& output) const
    {
        static_cast<GlOutput&>(output).Clear(val);
    }

    void RsRenderer::Preprocess(Scene1 const& scene)
    {
    }

    void RsRenderer::RenderBackground(GlScene const& scene)
    {
        auto& texture = scene.textures[scene.ibl_texture_idx];
        auto program = m_shader_manager.GetProgram("../App/GLSL/ibl");

        glDisable(GL_DEPTH_TEST); CHECK_GL_ERROR;
        glDepthMask(GL_FALSE);

        glClear(GL_COLOR_BUFFER_BIT); CHECK_GL_ERROR;

        glBindBuffer(GL_ARRAY_BUFFER, m_render_data->quad_vertices); CHECK_GL_ERROR;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_render_data->quad_indices); CHECK_GL_ERROR;
        glUseProgram(program); CHECK_GL_ERROR;

        GLuint ibl_loc = glGetUniformLocation(program, "IblTexture");
        assert(ibl_loc >= 0);
        CHECK_GL_ERROR;

        GLuint camera_fwd_loc = glGetUniformLocation(program, "CameraForward"); CHECK_GL_ERROR;
        GLuint camera_up_loc = glGetUniformLocation(program, "CameraUp"); CHECK_GL_ERROR;
        GLuint camera_right_loc = glGetUniformLocation(program, "CameraRight"); CHECK_GL_ERROR;
        GLuint camera_focal_length_loc = glGetUniformLocation(program, "CameraFocalLength"); CHECK_GL_ERROR;
        GLuint camera_sensor_size_loc = glGetUniformLocation(program, "CameraSensorSize"); CHECK_GL_ERROR;
        GLuint ibl_multiplier_loc = glGetUniformLocation(program, "IblMultiplier"); CHECK_GL_ERROR;

        glUniform1i(ibl_loc, 0); CHECK_GL_ERROR;

        auto camera_fwd = scene.camera->GetForwardVector();
        auto camera_up = scene.camera->GetUpVector();
        auto camera_right = scene.camera->GetRightVector();
        auto camera_sensor_size = scene.camera->GetSensorSize();
        glUniform3fv(camera_fwd_loc, 1, &camera_fwd.x); CHECK_GL_ERROR;
        glUniform3fv(camera_up_loc, 1, &camera_up.x); CHECK_GL_ERROR;
        glUniform3fv(camera_right_loc, 1, &camera_right.x); CHECK_GL_ERROR;
        glUniform2fv(camera_sensor_size_loc, 1, &camera_sensor_size.x); CHECK_GL_ERROR;
        glUniform1f(camera_focal_length_loc, scene.camera->GetFocalLength()); CHECK_GL_ERROR;
        glUniform1f(ibl_multiplier_loc, scene.ibl_multiplier); CHECK_GL_ERROR;

        glActiveTexture(GL_TEXTURE0); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, texture.texture); CHECK_GL_ERROR;

        GLuint position_attr = glGetAttribLocation(program, "inPosition"); CHECK_GL_ERROR;
        GLuint texcoord_attr = glGetAttribLocation(program, "inUv"); CHECK_GL_ERROR;

        glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0); CHECK_GL_ERROR;
        glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3)); CHECK_GL_ERROR;

        glEnableVertexAttribArray(position_attr); CHECK_GL_ERROR;
        glEnableVertexAttribArray(texcoord_attr); CHECK_GL_ERROR;

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        CHECK_GL_ERROR;

        glDepthMask(GL_TRUE);
        glDisableVertexAttribArray(texcoord_attr); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;
        glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
        glUseProgram(0); CHECK_GL_ERROR;
    }

    void RsRenderer::ResolveGI(GlScene const& scene, int radius)
    {
        auto program = m_shader_manager.GetProgram("../App/GLSL/indirect");

        glDisable(GL_DEPTH_TEST); CHECK_GL_ERROR;
        glDepthMask(GL_FALSE);

        glClear(GL_COLOR_BUFFER_BIT); CHECK_GL_ERROR;

        glBindBuffer(GL_ARRAY_BUFFER, m_render_data->quad_vertices); CHECK_GL_ERROR;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_render_data->quad_indices); CHECK_GL_ERROR;
        glUseProgram(program); CHECK_GL_ERROR;

        GLuint indirect_loc = glGetUniformLocation(program, "IndirectTexture");
        CHECK_GL_ERROR;
        GLuint direct_loc = glGetUniformLocation(program, "DirectTexture");
        CHECK_GL_ERROR;
        GLuint position_loc = glGetUniformLocation(program, "PositionTexture");
        CHECK_GL_ERROR;
        GLuint shadow_loc = glGetUniformLocation(program, "ShadowTexture");
        CHECK_GL_ERROR;
        GLuint radius_loc = glGetUniformLocation(program, "FilterRadius");
        CHECK_GL_ERROR;

        glUniform1i(indirect_loc, 0); CHECK_GL_ERROR;
        glUniform1i(direct_loc, 1); CHECK_GL_ERROR;
        glUniform1i(position_loc, 2); CHECK_GL_ERROR;
        glUniform1i(shadow_loc, 3); CHECK_GL_ERROR;
        glUniform1i(radius_loc, radius); CHECK_GL_ERROR;

        glActiveTexture(GL_TEXTURE0); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, m_render_data->gl_interop_buffer); CHECK_GL_ERROR;
        glActiveTexture(GL_TEXTURE1); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, m_render_data->frame_buffer_texture); CHECK_GL_ERROR;
        glActiveTexture(GL_TEXTURE2); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, m_render_data->gl_interop_position_buffer); CHECK_GL_ERROR;
        glActiveTexture(GL_TEXTURE3); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, m_render_data->gl_interop_shadow_buffer); CHECK_GL_ERROR;

        GLuint position_attr = glGetAttribLocation(program, "inPosition"); CHECK_GL_ERROR;
        GLuint texcoord_attr = glGetAttribLocation(program, "inUv"); CHECK_GL_ERROR;

        glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, 0); CHECK_GL_ERROR;
        glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*)(sizeof(float) * 3)); CHECK_GL_ERROR;

        glEnableVertexAttribArray(position_attr); CHECK_GL_ERROR;
        glEnableVertexAttribArray(texcoord_attr); CHECK_GL_ERROR;

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        CHECK_GL_ERROR;

        glDepthMask(GL_TRUE);
        glDisableVertexAttribArray(texcoord_attr); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;
        glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
        glUseProgram(0); CHECK_GL_ERROR;
    }

    void RsRenderer::Render(Scene1 const& scene)
    {
        Collector mat_collector;
        Collector tex_collector;
        GlScene& gl_scene = m_scene_controller.CompileScene(scene, mat_collector, tex_collector, false);

        static int sample_count = 0;
        if (scene.GetCamera()->IsDirty())
        {
            sample_count = 1;
            reinterpret_cast<ClwOutput*>(m_render_data->ray_tracer_output)->Clear(float3());
        }

        for (int i = 0; i < 4; ++i)
        m_render_data->ray_tracer->Render(scene);

        {
            glFinish();
            std::vector<cl_mem> objects;
            objects.push_back(m_render_data->cl_interop_buffer);
            objects.push_back(m_render_data->cl_interop_position_buffer);
            objects.push_back(m_render_data->cl_interop_shadow_buffer);
            m_render_data->cl_context.AcquireGLObjects(0, objects);

            CLWKernel copykernel = m_render_data->ray_tracer->GetCopyKernel();

            int argc = 0;
            copykernel.SetArg(argc++, reinterpret_cast<ClwOutput*>(m_render_data->ray_tracer_output)->data());
            copykernel.SetArg(argc++, m_render_data->ray_tracer_output->width());
            copykernel.SetArg(argc++, m_render_data->ray_tracer_output->height());
            copykernel.SetArg(argc++, 2.2f);
            copykernel.SetArg(argc++, m_render_data->cl_interop_buffer);

            int globalsize = m_render_data->ray_tracer_output->width() * m_render_data->ray_tracer_output->height();
            m_render_data->cl_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, copykernel);

            argc = 0;
            copykernel.SetArg(argc++, reinterpret_cast<ClwOutput*>(m_render_data->ray_tracer_output)->position());
            copykernel.SetArg(argc++, m_render_data->ray_tracer_output->width());
            copykernel.SetArg(argc++, m_render_data->ray_tracer_output->height());
            copykernel.SetArg(argc++, 1.f);
            copykernel.SetArg(argc++, m_render_data->cl_interop_position_buffer);

            globalsize = m_render_data->ray_tracer_output->width() * m_render_data->ray_tracer_output->height();
            m_render_data->cl_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, copykernel);

            argc = 0;
            copykernel.SetArg(argc++, reinterpret_cast<ClwOutput*>(m_render_data->ray_tracer_output)->shadow());
            copykernel.SetArg(argc++, m_render_data->ray_tracer_output->width());
            copykernel.SetArg(argc++, m_render_data->ray_tracer_output->height());
            copykernel.SetArg(argc++, 1.f);
            copykernel.SetArg(argc++, m_render_data->cl_interop_shadow_buffer);

            globalsize = m_render_data->ray_tracer_output->width() * m_render_data->ray_tracer_output->height();
            m_render_data->cl_context.Launch1D(0, ((globalsize + 63) / 64) * 64, 64, copykernel);

            m_render_data->cl_context.ReleaseGLObjects(0, objects);
            m_render_data->cl_context.Finish(0);
        }

        {
            glBindFramebuffer(GL_FRAMEBUFFER, m_render_data->frame_buffer); CHECK_GL_ERROR;

            GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, draw_buffers); CHECK_GL_ERROR;

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                throw std::runtime_error("Framebuffer binding failed");
            }
        }

        glViewport(0, 0, m_output->width(), m_output->height()); CHECK_GL_ERROR;
        Clear(RadeonRays::float3(0.1f, 0.1f, 0.1f), *m_output);

        if (gl_scene.ibl_texture_idx != -1)
        {
            RenderBackground(gl_scene);
        }

        glEnable(GL_DEPTH_TEST);

        GLuint program = m_shader_manager.GetProgram("../App/GLSL/basic");
        glUseProgram(program); CHECK_GL_ERROR;

        glEnable(GL_TEXTURE_2D); CHECK_GL_ERROR;

        GLuint position_attr = glGetAttribLocation(program, "inPosition"); CHECK_GL_ERROR;
        GLuint normal_attr = glGetAttribLocation(program, "inNormal"); CHECK_GL_ERROR;
        GLuint texcoord_attr = glGetAttribLocation(program, "inUv"); CHECK_GL_ERROR;

        glEnableVertexAttribArray(position_attr); CHECK_GL_ERROR;
        glEnableVertexAttribArray(normal_attr); CHECK_GL_ERROR;
        glEnableVertexAttribArray(texcoord_attr); CHECK_GL_ERROR;

        GLuint view_loc = glGetUniformLocation(program, "g_View"); CHECK_GL_ERROR;
        GLuint proj_loc = glGetUniformLocation(program, "g_Proj"); CHECK_GL_ERROR;
        GLuint world_loc = glGetUniformLocation(program, "g_World"); CHECK_GL_ERROR;
        GLuint camera_pos_loc = glGetUniformLocation(program, "CameraPosition"); CHECK_GL_ERROR;
        GLuint diffuse_albedo_loc = glGetUniformLocation(program, "DiffuseAlbedo"); CHECK_GL_ERROR;
        GLuint diffuse_roughness_loc = glGetUniformLocation(program, "DiffuseRoughness"); CHECK_GL_ERROR;
        GLuint gloss_albedo_loc = glGetUniformLocation(program, "GlossAlbedo"); CHECK_GL_ERROR;
        GLuint gloss_roughness_loc = glGetUniformLocation(program, "GlossRoughness"); CHECK_GL_ERROR;
        GLuint has_diffuse_albedo_texture_loc = glGetUniformLocation(program, "HasDiffuseAlbedoTexture"); CHECK_GL_ERROR;
        GLuint diffuse_albedo_texture_loc = glGetUniformLocation(program, "DiffuseAlbedoTexture"); CHECK_GL_ERROR;
        GLuint ior_loc = glGetUniformLocation(program, "Ior"); CHECK_GL_ERROR;

        GLuint has_ibl_texture_loc = glGetUniformLocation(program, "HasIblTexture"); CHECK_GL_ERROR;
        GLuint ibl_texture_loc = glGetUniformLocation(program, "IblTexture"); CHECK_GL_ERROR;
        GLuint ibl_multiplier_loc = glGetUniformLocation(program, "IblMultiplier"); CHECK_GL_ERROR;

        auto view_transform = gl_scene.camera->GetViewMatrix();
        auto proj_transform = gl_scene.camera->GetProjectionMatrix();
        auto camera_position = gl_scene.camera->GetPosition();

        glUniformMatrix4fv(view_loc, 1, GL_FALSE, &view_transform.m00); CHECK_GL_ERROR;
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, &proj_transform.m00); CHECK_GL_ERROR;
        glUniform3fv(camera_pos_loc, 1, &camera_position.x); CHECK_GL_ERROR;
        glUniform1i(has_ibl_texture_loc, gl_scene.ibl_texture_idx); CHECK_GL_ERROR;
        glUniform1f(ibl_multiplier_loc, gl_scene.ibl_multiplier); CHECK_GL_ERROR;

        if (gl_scene.ibl_texture_idx != -1)
        {
            auto& texture = gl_scene.textures[gl_scene.ibl_texture_idx];
            glActiveTexture(GL_TEXTURE1); CHECK_GL_ERROR;
            glBindTexture(GL_TEXTURE_2D, texture.texture); CHECK_GL_ERROR;
            glUniform1i(ibl_texture_loc, 1); CHECK_GL_ERROR;
        }

        for (auto& iter : gl_scene.shapes)
        {
            glUniformMatrix4fv(world_loc, 1, GL_FALSE, &iter.second.transform.m00); CHECK_GL_ERROR;

            glBindBuffer(GL_ARRAY_BUFFER, iter.second.vertex_buffer); CHECK_GL_ERROR;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iter.second.index_buffer); CHECK_GL_ERROR;

            glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex), 0); CHECK_GL_ERROR;
            glVertexAttribPointer(normal_attr, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)(sizeof(RadeonRays::float3))); CHECK_GL_ERROR;
            glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)(sizeof(RadeonRays::float3) * 2)); CHECK_GL_ERROR;

            GlMaterialData& material_data(gl_scene.materials[iter.second.material_idx]);
            glUniform3fv(diffuse_albedo_loc, 1, &material_data.diffuse_color.x); CHECK_GL_ERROR;
            glUniform3fv(gloss_albedo_loc, 1, &material_data.gloss_color.x); CHECK_GL_ERROR;
            glUniform1f(diffuse_roughness_loc, material_data.diffuse_roughness); CHECK_GL_ERROR;
            glUniform1f(gloss_roughness_loc, material_data.gloss_roughness); CHECK_GL_ERROR;
            glUniform1f(ior_loc, material_data.ior); CHECK_GL_ERROR;
            glUniform1i(has_diffuse_albedo_texture_loc, material_data.diffuse_texture_idx); CHECK_GL_ERROR;

            if (material_data.diffuse_texture_idx != -1)
            {
                auto& texture = gl_scene.textures[material_data.diffuse_texture_idx];
                glActiveTexture(GL_TEXTURE0); CHECK_GL_ERROR;
                glBindTexture(GL_TEXTURE_2D, texture.texture); CHECK_GL_ERROR;
                glUniform1i(diffuse_albedo_texture_loc, 0); CHECK_GL_ERROR;
            }

            glDrawElements(GL_TRIANGLES, iter.second.num_triangles * 3, GL_UNSIGNED_INT, nullptr); CHECK_GL_ERROR;
        }

        {
            glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GlOutput*>(m_output)->GetGlFramebuffer()); CHECK_GL_ERROR;


            GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
            glDrawBuffers(1, draw_buffers); CHECK_GL_ERROR;

            if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                throw std::runtime_error("Framebuffer binding failed");
            }
        }

        //glBindFramebuffer(GL_READ_FRAMEBUFFER, m_render_data->frame_buffer); CHECK_GL_ERROR;
        //glBlitFramebuffer(0, 0, m_output->width(), m_output->height(), 0, 0, m_output->width(), m_output->height(), GL_COLOR_BUFFER_BIT, GL_NEAREST); CHECK_GL_ERROR;
        float filter = pow(1.f - clamp(sample_count / 128.f, 0.f, 1.f), 8.f);
        int radius = (int)clamp((10.f * filter), 1, 10);
        ResolveGI(gl_scene, radius);
        //glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); CHECK_GL_ERROR;
        sample_count+=1;

        glDisable(GL_TEXTURE_2D); CHECK_GL_ERROR;
        glDisableVertexAttribArray(position_attr); CHECK_GL_ERROR;
        glDisableVertexAttribArray(normal_attr); CHECK_GL_ERROR;
        glDisableVertexAttribArray(texcoord_attr); CHECK_GL_ERROR;
        glDisable(GL_DEPTH_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;
        glBindBuffer(GL_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); CHECK_GL_ERROR;
        glUseProgram(0); CHECK_GL_ERROR;

    }

    void RsRenderer::SetOutput(Output* output)
    {
        m_output = output;
    }

    void RsRenderer::RunBenchmark(Scene1 const& scene, std::uint32_t num_passes, BenchmarkStats& stats)
    {

    }

}