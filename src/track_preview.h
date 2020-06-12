#pragma once

#include "renderer.h"

class TrackPreview2D
{
    bool isInitialized = false;
    GLuint multisampleFramebuffer, multisampleTex, multisampleDepthBuffer;
    GLuint destFramebuffer, destTex;
    u32 width = 0;
    u32 height = 0;
    glm::vec3 camPosition;
    glm::mat4 viewProjection;

    Texture outputTexture;

public:
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

    void beginUpdate(Renderer* renderer, u32 width, u32 height)
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

		glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 1, -1, "2D Track HUD");
        glViewport(0, 0, width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, this->multisampleFramebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glClearColor(0.f, 0.f, 0.f, 0.f);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
        glDisable(GL_BLEND);

        static ShaderHandle shader = getShaderHandle("mesh2D");
        glUseProgram(renderer->getShaderProgram(shader));
        glUniform3fv(5, 1, (GLfloat*)&camPosition);
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(viewProjection));
    }

    void setCamViewProjection(glm::mat4 const& viewProjection)
    {
        this->viewProjection = viewProjection;
    }

    void setCamPosition(glm::vec3 camPosition)
    {
        this->camPosition = camPosition;
    }

    void drawItem(GLuint vao, u32 numIndices, glm::mat4 const& worldTransform,
            glm::vec3 const& color=glm::vec3(1.0), bool overwriteColor=false, i32 shadeMode=0)
    {
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(worldTransform));
        glUniform3fv(2, 1, (GLfloat*)&color);
        glUniform1i(3, overwriteColor);
        glUniform1i(4, shadeMode);
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, 0);
    }

    void endUpdate()
    {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, this->multisampleFramebuffer);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->destFramebuffer);
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glPopDebugGroup();

		outputTexture.handle = destTex;
		outputTexture.width = width;
		outputTexture.height = height;
		outputTexture.name = "2D Track Preview";
    }

    Texture* getTexture() { return &outputTexture; }

    glm::vec2 getSize() const { return { width, height }; }
};
