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
    f32 fresnelScale = 0.f;
    f32 fresnelPower = 2.5f;
    f32 fresnelBias = -0.2f;
    Texture* texture = nullptr;
    Mesh* mesh = nullptr;
    glm::mat4 worldTransform;
    bool culling = true;
    bool castShadow = true;
    bool transparent = false;
    f32 minAlpha = 0.f;
    f32 reflectionStrength = 0.f;
    f32 reflectionLod = 1.f;
    f32 reflectionBias = 0.2f;

    LitSettings() {}
    LitSettings(glm::vec3 const& color, f32 specularStrength=0.2f, f32 specularPower=50.f,
            f32 fresnelScale=0.f, f32 fresnelPower=2.5f, f32 fresnelBias=-0.2f,
            Texture* texture=nullptr, bool culling=true, bool transparent=false,
            f32 minAlpha=0.f, f32 reflectionStrength=0.f, f32 reflectionLod=1.f, f32 reflectionBias=0.2f)
        : color(color), specularPower(specularPower), specularStrength(specularStrength),
        fresnelScale(fresnelScale), fresnelPower(fresnelPower),
        fresnelBias(fresnelBias), texture(texture), culling(culling), transparent(transparent),
        minAlpha(minAlpha), reflectionStrength(reflectionStrength), reflectionLod(reflectionLod),
        reflectionBias(reflectionBias)
    {}
};

class LitRenderable : public Renderable
{
public:
    LitSettings settings;

    LitRenderable() {}
    LitRenderable(Mesh* mesh, glm::mat4 const& worldTransform,
            Texture* texture = nullptr, glm::vec3 color = {1, 1, 1})
    {
        settings.mesh = mesh;
        settings.color = color;
        if (!texture)
        {
            texture = &g_res.textures->white;
        }
        settings.texture = texture;
        settings.worldTransform = worldTransform;
    }
    LitRenderable(LitSettings const& settings) : settings(settings)
    {
        if (!this->settings.texture)
        {
            this->settings.texture = &g_res.textures->white;
        }
    }

    i32 getPriority() const override
    {
        return 20 + settings.culling + (settings.transparent ? 11000 : 0) + (settings.minAlpha > 0.f);
    }

    void onDepthPrepassPriorityTransition(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram(
            (settings.minAlpha > 0.f || settings.transparent) ? "lit_discard" : "lit"));
    }

    void onDepthPrepass(Renderer* renderer) override
    {
        if (settings.transparent)
        {
            return;
        }
        glUniform1f(5, settings.minAlpha);
        glBindTextureUnit(0, settings.texture->handle);
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(settings.worldTransform));
        glBindVertexArray(settings.mesh->vao);
        glDrawElements(GL_TRIANGLES, settings.mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    void onShadowPassPriorityTransition(Renderer* renderer) override
    {
        onDepthPrepassPriorityTransition(renderer);
    }

    void onShadowPass(Renderer* renderer) override
    {
        glUniform1f(5, settings.transparent ? 0.5f : settings.minAlpha);
        glBindTextureUnit(0, settings.texture->handle);
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
        glUseProgram(renderer->getShaderProgram("lit"));

        /*
        if (settings.culling)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        if (settings.transparent)
        {
            glEnable(GL_BLEND);
            glDepthFunc(GL_LEQUAL);
        }
        else
        {
            glDisable(GL_BLEND);
            glDepthFunc(GL_EQUAL);
        }
        glUseProgram(renderer->getShaderProgram(
                    settings.minAlpha > 0.f ? "lit_discard" : "lit"));
                    */
    }

    void onLitPass(Renderer* renderer) override
    {
        glBindTextureUnit(0, settings.texture->handle);

        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(settings.worldTransform));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(settings.worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glUniform3fv(2, 1, (GLfloat*)&settings.color);
        glUniform3f(3, settings.fresnelBias, settings.fresnelScale, settings.fresnelPower);
        glUniform3f(4, settings.specularPower, settings.specularStrength, 0.f);
        glUniform1f(5, settings.minAlpha);
        glUniform3fv(6, 1, (GLfloat*)&settings.emit);
        glUniform3f(7, settings.reflectionStrength, settings.reflectionLod, settings.reflectionBias);

        glBindVertexArray(settings.mesh->vao);
        glDrawElements(GL_TRIANGLES, settings.mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    std::string getDebugString() const override
    {
        return settings.transparent ? "LitRenderable(t)" : "LitRenderable";
    }
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
        : color(color), mesh(mesh), worldTransform(worldTransform),
          cameraIndex(cameraIndex), priorityOffset(priorityOffset), onlyDepth(onlyDepth) {}

    i32 getPriority() const override { return 250000 + priorityOffset; }

    void onLitPassPriorityTransition(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram("overlay"));
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
    }

    void onLitPass(Renderer* renderer) override
    {
        if (renderer->getCurrentRenderingCameraIndex() != cameraIndex)
        {
            return;
        }

        if (onlyDepth)
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        }
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(worldTransform));
        glUniform3f(1, color.x, color.y, color.z);
        glBindVertexArray(mesh->vao);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(0.f, -2000000.f);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
        glDisable(GL_POLYGON_OFFSET_FILL);
        if (onlyDepth)
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
    }

    std::string getDebugString() const override { return "OverlayRenderable"; }
};

class WireframeRenderable : public Renderable
{
    glm::vec4 color = { 1, 1, 1, 1 };
    Mesh* mesh = nullptr;
    glm::mat4 worldTransform;

public:
    WireframeRenderable(Mesh* mesh, glm::mat4 const& worldTransform, glm::vec4 color = {0.8f, 0.8f, 0.8f, 1})
        : color(color), mesh(mesh), worldTransform(worldTransform) { }

    i32 getPriority() const override { return 200000; }

    void onLitPass(Renderer* renderer) override
    {
        if (renderer->getCurrentRenderingCameraIndex() != 0)
        {
            return;
        }

        glDepthFunc(GL_LEQUAL);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_CULL_FACE);

        Camera const& camera = renderer->getRenderWorld()->getCamera(0);
        glUseProgram(renderer->getShaderProgram("debug"));
        glUniform4f(2, color.x, color.y, color.z, color.w);
        glUniform1i(3, 1);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(camera.viewProjection * worldTransform));

        glBindVertexArray(mesh->vao);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    std::string getDebugString() const override { return "WireframeRenderable"; }
};
