#pragma once

#include "renderable.h"
#include "resources.h"
#include "renderer.h"

class LitRenderable : public Renderable
{
    glm::vec3 color = { 1, 1, 1 };
    f32 specularPower = 50.f;
    glm::vec3 emit = { 0, 0, 0 };
    f32 specularStrength = 0.2f;
    glm::vec3 specularColor = { 1, 1, 1 };
    f32 fresnelScale = 0.8f;
    f32 fresnelPower = 3.f;
    f32 fresnelBias = -0.1f;
    Texture* texture = nullptr;
    Mesh* mesh = nullptr;
    glm::mat4 worldTransform;
    bool culling = true;
    bool castShadow = true;

public:
    LitRenderable(Mesh* mesh, glm::mat4 const& worldTransform, Texture* texture = nullptr, glm::vec3 color = {1, 1, 1})
        : mesh(mesh), worldTransform(worldTransform)
    {
        if (!texture)
        {
            texture = g_resources.getTexture("white");
        }
        this->texture = texture;
    }

    LitRenderable* shadow(bool shadow)
    {
        castShadow = shadow;
        return this;
    }

    i32 getPriority() const override { return 0 + culling; }

    void onDepthPrepassPriorityTransition(Renderer* renderer) override
    {
        // TODO: create simpler shader for depth pass
        glUseProgram(renderer->getShaderProgram("lit"));
    }

    void onDepthPrepass(Renderer* renderer) override
    {
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(worldTransform));
        glBindVertexArray(mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    void onShadowPassPriorityTransition(Renderer* renderer) override
    {
        // TODO: create simpler shader for depth pass
        glUseProgram(renderer->getShaderProgram("lit"));
    }

    void onShadowPass(Renderer* renderer) override
    {
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(worldTransform));
        glBindVertexArray(mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    void onLitPassPriorityTransition(Renderer* renderer) override
    {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_EQUAL);
        glCullFace(GL_BACK);
        if (culling)
        {
            glEnable(GL_CULL_FACE);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }
        glUseProgram(renderer->getShaderProgram("lit"));
    }

    void onLitPass(Renderer* renderer) override
    {
        glBindTextureUnit(0, texture->handle);

        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(worldTransform));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniform3f(2, color.x, color.y, color.z);

        glBindVertexArray(mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    std::string getDebugString() const override { return "LitRenderable"; };
};

class OverlayRenderable : public Renderable
{
    glm::vec3 color = { 1, 1, 1 };
    Mesh* mesh = nullptr;
    glm::mat4 worldTransform;
    u32 cameraIndex = 0;

public:
    OverlayRenderable(Mesh* mesh, u32 cameraIndex, glm::mat4 const& worldTransform, glm::vec3 const& color)
        : mesh(mesh), cameraIndex(cameraIndex), worldTransform(worldTransform), color(color) {}

    i32 getPriority() const override { return 20000; }

    void onLitPassPriorityTransition(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram("overlay"));
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_FALSE);
        glDisable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
    }

    void onLitPass(Renderer* renderer) override
    {
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(worldTransform));
        glUniform3f(1, color.x, color.y, color.z);
        glUniform1i(2, cameraIndex);
        glBindVertexArray(mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    std::string getDebugString() const override { return "OverlayRenderable"; };
};
