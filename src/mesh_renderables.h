#pragma once

#include "renderable.h"
#include "resources.h"
#include "renderer.h"

struct LitSettings
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
};

class LitRenderable : public Renderable
{
    LitSettings settings;

public:
    LitRenderable(Mesh* mesh, glm::mat4 const& worldTransform, Texture* texture = nullptr, glm::vec3 color = {1, 1, 1})
    {
        settings.mesh = mesh;
        settings.color = color;
        if (!texture)
        {
            texture = g_resources.getTexture("white");
        }
        settings.texture = texture;
        settings.worldTransform = worldTransform;
    }
    LitRenderable(LitSettings const& settings) : settings(settings)
    {
        if (!this->settings.texture)
        {
            this->settings.texture = g_resources.getTexture("white");
        }
    }

    i32 getPriority() const override { return 0 + settings.culling; }

    void onDepthPrepassPriorityTransition(Renderer* renderer) override
    {
        // TODO: create simpler shader for depth pass
        glUseProgram(renderer->getShaderProgram("lit"));
    }

    void onDepthPrepass(Renderer* renderer) override
    {
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(settings.worldTransform));
        glBindVertexArray(settings.mesh->vao);
        glDrawElements(GL_TRIANGLES, settings.mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    void onShadowPassPriorityTransition(Renderer* renderer) override
    {
        // TODO: create simpler shader for depth pass
        glUseProgram(renderer->getShaderProgram("lit"));
    }

    void onShadowPass(Renderer* renderer) override
    {
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(settings.worldTransform));
        glBindVertexArray(settings.mesh->vao);
        glDrawElements(GL_TRIANGLES, settings.mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    void onLitPassPriorityTransition(Renderer* renderer) override
    {
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_EQUAL);
        glCullFace(GL_BACK);
        if (settings.culling)
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
        glBindTextureUnit(0, settings.texture->handle);

        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(settings.worldTransform));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(settings.worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniform3fv(2, 1, (GLfloat*)&settings.color);
        glUniform3f(3, settings.fresnelBias, settings.fresnelScale, settings.fresnelPower);

        glBindVertexArray(settings.mesh->vao);
        glDrawElements(GL_TRIANGLES, settings.mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    std::string getDebugString() const override { return "LitRenderable"; };
};

class OverlayRenderable : public Renderable
{
    glm::vec3 color = { 1, 1, 1 };
    Mesh* mesh = nullptr;
    glm::mat4 worldTransform;
    u32 cameraIndex = 0;
    i32 priorityOffset = 0;
    bool onlyDepth = false;

public:
    OverlayRenderable(Mesh* mesh, u32 cameraIndex, glm::mat4 const& worldTransform,
            glm::vec3 const& color, i32 priorityOffset = 0, bool onlyDepth=false)
        : mesh(mesh), cameraIndex(cameraIndex), onlyDepth(onlyDepth),
            worldTransform(worldTransform), color(color), priorityOffset(priorityOffset) {}

    i32 getPriority() const override { return 250000 + priorityOffset; }

    void onLitPassPriorityTransition(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram("overlay"));
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
        glDepthFunc(GL_LEQUAL);
    }

    void onLitPass(Renderer* renderer) override
    {
        if (onlyDepth)
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        }
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(worldTransform));
        glUniform3f(1, color.x, color.y, color.z);
        glUniform1i(2, cameraIndex);
        glBindVertexArray(mesh->vao);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.f, -1000000.f);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        if (onlyDepth)
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
    }

    std::string getDebugString() const override { return "OverlayRenderable"; };
};

class WireframeRenderable : public Renderable
{
    glm::vec4 color = { 1, 1, 1, 1 };
    Mesh* mesh = nullptr;
    glm::mat4 worldTransform;

public:
    WireframeRenderable(Mesh* mesh, glm::mat4 const& worldTransform, glm::vec4 color = {0.8f, 0.8f, 0.8f, 1})
        : mesh(mesh), worldTransform(worldTransform), color(color) { }

    i32 getPriority() const override { return 200000; }

    void onLitPass(Renderer* renderer) override
    {
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);

        Camera const& camera = renderer->getCamera(0);
        glUseProgram(renderer->getShaderProgram("debug"));
        glUniform4f(2, color.x, color.y, color.z, color.w);
        glUniform1i(3, 1);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(camera.viewProjection * worldTransform));

        glBindVertexArray(mesh->vao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    std::string getDebugString() const override { return "WireframeRenderable"; };
};
