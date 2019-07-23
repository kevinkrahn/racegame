#pragma once

#include "renderable.h"
#include "renderer.h"

class QuadRenderable : public Renderable2D {
    struct Vertex
    {
        glm::vec2 xy;
        glm::vec2 uv;
    };

    Texture* tex;
    Vertex points[4];
    glm::vec4 color;

public:
    QuadRenderable(Texture* texture, glm::vec2 p1, glm::vec2 p2, glm::vec2 t1=glm::vec2(0), glm::vec2 t2=glm::vec2(1),
            glm::vec3 color=glm::vec3(1.0), f32 alpha=1.f) : tex(texture), color(glm::vec4(color, alpha))
    {
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
    }

    QuadRenderable(Texture* texture, glm::vec2 p1, f32 width, f32 height,
            glm::vec3 color=glm::vec3(1.0), f32 alpha=1.f) : tex(texture), color(glm::vec4(color, alpha))
    {
        glm::vec2 t1(0, 0);
        glm::vec2 t2(1, 1);
        glm::vec2 p2 = p1 + glm::vec2(width, height);
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
    }

    void on2DPass(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram("tex2D"));
        glBindTextureUnit(0, tex->handle);
        glUniform4fv(0, 4, (GLfloat*)&points);
        glUniform4fv(4, 1, (GLfloat*)&color);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};

class TrackPreview2D : public Renderable2D {
    bool isInitialized = false;
    GLuint multisampleFramebuffer, multisampleTex, multisampleDepthBuffer;
    GLuint destFramebuffer, destTex;
    u32 width = 0;
    u32 height = 0;

    struct Vertex
    {
        glm::vec2 xy;
        glm::vec2 uv;
    };
    Vertex points[4];

public:
    struct RenderItem
    {
        Mesh* mesh;
        glm::mat4 transform;
        glm::vec3 color = glm::vec3(1.0);
        bool overwriteColor = false;
    };

    TrackPreview2D() {}
    ~TrackPreview2D()
    {
        if (isInitialized)
        {
            cleanup();
        }
    }

    void cleanup()
    {
        glDeleteFramebuffers(1, &multisampleFramebuffer);
        glDeleteFramebuffers(1, &destFramebuffer);
        glDeleteRenderbuffers(1, &multisampleDepthBuffer);
        glDeleteTextures(1, &multisampleTex);
        glDeleteTextures(1, &destTex);
    }

    void update(Renderer* renderer, std::vector<RenderItem> const& staticItems,
                std::vector<RenderItem> const& dynamicItems, u32 width, u32 height, glm::vec2 pos)
    {
        if (this->width != width || this->height != height)
        {
            if (isInitialized)
            {
                cleanup();
            }

            this->width = width;
            this->height = height;

            // multisample
            const u32 samples = 4;
            glGenTextures(1, &this->multisampleTex);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, this->multisampleTex);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGBA, width, height, GL_TRUE);

            glGenRenderbuffers(1, &this->multisampleDepthBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, this->multisampleDepthBuffer);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH_COMPONENT, width, height);

            glGenFramebuffers(1, &this->multisampleFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, this->multisampleFramebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, this->multisampleTex, 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, this->multisampleDepthBuffer);

            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

            // dest
            glGenTextures(1, &this->destTex);
            glBindTexture(GL_TEXTURE_2D, this->destTex);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glGenFramebuffers(1, &this->destFramebuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, this->destFramebuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->destTex, 0);

            assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

            glBindTexture(GL_TEXTURE_2D, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);

            isInitialized = true;
        }

        glViewport(0, 0, width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, this->multisampleFramebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glDisable(GL_BLEND);

        glUseProgram(renderer->getShaderProgram("mesh2D"));

        for (auto& item : staticItems)
        {
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(item.transform));
            glUniform3fv(1, 1, (GLfloat*)&item.color);
            glUniform1i(2, item.overwriteColor);
            glBindVertexArray(item.mesh->vao);
            glDrawElements(GL_TRIANGLES, item.mesh->numIndices, GL_UNSIGNED_INT, 0);
        }

        for (auto& item : dynamicItems)
        {
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(item.transform));
            glUniform3fv(1, 1, (GLfloat*)&item.color);
            glUniform1i(2, item.overwriteColor);
            glBindVertexArray(item.mesh->vao);
            glDrawElements(GL_TRIANGLES, item.mesh->numIndices, GL_UNSIGNED_INT, 0);
        }

        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multisampleFramebuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->destFramebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glm::vec2 size(width, height);
        glm::vec2 p1 = pos - size * 0.5f;
        glm::vec2 p2 = pos + size * 0.5f;
        glm::vec2 t1 = { 0.f, 0.f };
        glm::vec2 t2 = { 1.f, 1.f };
        points[0] = { p1, t1 };
        points[1] = { { p2.x, p1.y }, { t2.x, t1.y } };
        points[2] = { { p1.x, p2.y }, { t1.x, t2.y } };
        points[3] = { p2, t2 };
    }

    void on2DPass(Renderer* renderer) override
    {
        if (!isInitialized)
        {
            return;
        }

        glm::vec4 color(1.0);
        glUseProgram(renderer->getShaderProgram("tex2D"));
        glBindTextureUnit(0, destTex);
        glUniform4fv(0, 4, (GLfloat*)&points);
        glUniform4fv(4, 1, (GLfloat*)&color);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
};
