#include "rsrenderer.h"
#include "Core/output.h"

#include <cassert>

#define CHECK_GL_ERROR {auto err = glGetError(); assert(err == GL_NO_ERROR);}

namespace Baikal 
{
    GlOutput::GlOutput(std::uint32_t w, std::uint32_t h)
        : Output(w, h)
        , m_width(w)
        , m_height(h)
    {
        glGenFramebuffers(1, &m_frame_buffer); CHECK_GL_ERROR;
        glBindFramebuffer(GL_FRAMEBUFFER, m_frame_buffer); CHECK_GL_ERROR;

        glGenTextures(1, &m_texture);
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_FLOAT, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);

        glGenRenderbuffers(1, &m_depth_buffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depth_buffer);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_buffer);

        glBindFramebuffer(GL_FRAMEBUFFER, 0); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;
        glBindRenderbuffer(GL_RENDERBUFFER, 0); CHECK_GL_ERROR;
    }

    void GlOutput::GetData(RadeonRays::float3* data) const
    {
        glBindTexture(GL_TEXTURE_2D, m_texture); CHECK_GL_ERROR;
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &data); CHECK_GL_ERROR;
        glBindTexture(GL_TEXTURE_2D, 0); CHECK_GL_ERROR;
    }

    void GlOutput::Clear(RadeonRays::float3 const& val)
    {
        glClearColor(val.x, val.y, val.z, 1.f); CHECK_GL_ERROR;
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); CHECK_GL_ERROR;
    }

    GLuint GlOutput::GetGlTexture() const
    {
        return m_texture;
    }

    GLuint GlOutput::GetGlFramebuffer() const
    {
        return m_frame_buffer;
    }


    struct RsRenderer::RenderData
    {
    };

    RsRenderer::RsRenderer()
        : m_render_data(new RenderData)
        , m_output(nullptr)
    {
    }

    RsRenderer::~RsRenderer()
    {

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

    void RsRenderer::Render(Scene1 const& scene)
    {
        Collector mat_collector;
        Collector tex_collector;
        GlScene& gl_scene = m_scene_controller.CompileScene(scene, mat_collector, tex_collector);

        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GlOutput*>(m_output)->GetGlFramebuffer()); CHECK_GL_ERROR;

        GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
        glDrawBuffers(1, draw_buffers); CHECK_GL_ERROR;

        glEnable(GL_DEPTH_TEST);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            throw std::runtime_error("Framebuffer binding failed");
        }

        glViewport(0, 0, m_output->width(), m_output->height()); CHECK_GL_ERROR;

        Clear(RadeonRays::float3(0.1f, 0.1f, 0.1f), *m_output);

        GLuint program = m_shader_manager.GetProgram("../App/GLSL/basic");
        glUseProgram(program); CHECK_GL_ERROR;

        GLuint position_attr = glGetAttribLocation(program, "inPosition"); CHECK_GL_ERROR;
        GLuint normal_attr = glGetAttribLocation(program, "inNormal"); CHECK_GL_ERROR;
       // GLuint texcoord_attr = glGetAttribLocation(program, "inUv"); CHECK_GL_ERROR;

        glEnableVertexAttribArray(position_attr); CHECK_GL_ERROR;
        glEnableVertexAttribArray(normal_attr); CHECK_GL_ERROR;
        //glEnableVertexAttribArray(texcoord_attr); CHECK_GL_ERROR;

        GLuint view_loc = glGetUniformLocation(program, "g_View"); CHECK_GL_ERROR;
        GLuint proj_loc = glGetUniformLocation(program, "g_Proj"); CHECK_GL_ERROR;
        GLuint world_loc = glGetUniformLocation(program, "g_World"); CHECK_GL_ERROR;

        glUniformMatrix4fv(view_loc, 1, GL_FALSE, &gl_scene.view_transform.m00); CHECK_GL_ERROR;
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, &gl_scene.proj_transform.m00); CHECK_GL_ERROR;

        for (auto& iter : gl_scene.shapes)
        {
            glUniformMatrix4fv(world_loc, 1, GL_FALSE, &iter.second.transform.m00); CHECK_GL_ERROR;

            glBindBuffer(GL_ARRAY_BUFFER, iter.second.vertex_buffer); CHECK_GL_ERROR;
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iter.second.index_buffer); CHECK_GL_ERROR;

            glVertexAttribPointer(position_attr, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex), 0); CHECK_GL_ERROR;
            glVertexAttribPointer(normal_attr, 3, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)(sizeof(RadeonRays::float3))); CHECK_GL_ERROR;
            //glVertexAttribPointer(texcoord_attr, 2, GL_FLOAT, GL_FALSE, sizeof(GlVertex), (void*)(sizeof(RadeonRays::float3) * 2)); CHECK_GL_ERROR;

            glDrawElements(GL_TRIANGLES, iter.second.num_triangles * 3, GL_UNSIGNED_INT, nullptr); CHECK_GL_ERROR;
        }

        glDisableVertexAttribArray(position_attr); CHECK_GL_ERROR;
        glDisableVertexAttribArray(normal_attr); CHECK_GL_ERROR;
        //glDisableVertexAttribArray(texcoord_attr); CHECK_GL_ERROR;
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