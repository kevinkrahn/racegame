#pragma once

#include "renderable.h"
#include "resources.h"
#include "renderer.h"
#include "material.h"

class LitMaterialRenderable : public Renderable
{
public:
    Material* material;
    glm::mat4 transform;
    Mesh* mesh;
    GLuint textureColor;
    GLuint textureNormal;
    u32 pickID;
    u8 stencil;
    bool highlightModeEnabled;
    u8 highlightStep;

    LitMaterialRenderable(Mesh* mesh, glm::mat4 const& worldTransform, Material* material,
            u32 pickID=0, u8 stencil=0, bool highlightModeEnabled=false, u8 highlightStep=0)
        : material(material), transform(worldTransform), mesh(mesh), pickID(pickID), stencil(stencil),
        highlightModeEnabled(highlightModeEnabled), highlightStep(highlightStep)
    {
        textureColor = material->colorTexture
            ? g_res.getTexture(material->colorTexture)->handle : g_res.white.handle;
#if 0
        textureNormal = material->normalMapTexture
            ? g_res.getTexture(material->normalMapTexture)->handle : g_res.identityNormal.handle;
#else
        textureNormal = material->normalMapTexture
            ? g_res.getTexture(material->normalMapTexture)->handle : 0;
#endif
    }

    i32 getPriority() const override
    {
        return 20 + material->isCullingEnabled
            + (material->isTransparent ? 11000 : 0)
            + (material->depthOffset != 0.f ? -1 : 0)
            + (!material->isDepthWriteEnabled ? 10 : 0)
            + (material->alphaCutoff > 0.f ? 100 : 0)
            + (textureNormal != 0 ? 500 : 0)
            + (highlightModeEnabled ? 250000 : 0)
            + (highlightStep * 50000);
    }

    void onDepthPrepassPriorityTransition(Renderer* renderer) override
    {
        if (material->isDepthWriteEnabled && !highlightModeEnabled)
        {
            glUseProgram(renderer->getShaderProgram(
                (material->alphaCutoff > 0.f || material->isTransparent) ? "lit_discard" : "lit"));
        }
    }

    void onDepthPrepass(Renderer* renderer) override
    {
        if (material->isDepthWriteEnabled && !material->isTransparent && !highlightModeEnabled)
        {
            glUniform1f(8, material->windAmount);
            if (material->alphaCutoff > 0.f)
            {
                glUniform1f(5, material->alphaCutoff);
            }
            glBindTextureUnit(0, textureColor);
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(transform));
            glBindVertexArray(mesh->vao);
            glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
        }
    }

    void onShadowPassPriorityTransition(Renderer* renderer) override
    {
        if (material->castsShadow && !highlightModeEnabled)
        {
            glUseProgram(renderer->getShaderProgram(
                (material->shadowAlphaCutoff > 0.f || material->isTransparent) ? "lit_discard" : "lit"));
        }
    }

    void onShadowPass(Renderer* renderer) override
    {
        if (material->castsShadow && !highlightModeEnabled)
        {
            glUniform1f(8, material->windAmount);
            if (material->shadowAlphaCutoff > 0.f)
            {
                glUniform1f(5, material->shadowAlphaCutoff);
            }
            glBindTextureUnit(0, textureColor);
            glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(transform));
            glBindVertexArray(mesh->vao);
            glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
        }
    }

    void onLitPassPriorityTransition(Renderer* renderer) override
    {
        glDepthFunc(GL_LEQUAL);

        if (material->isCullingEnabled)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);
        }
        else
        {
            glDisable(GL_CULL_FACE);
        }

        if (material->isDepthReadEnabled)
        {
            glEnable(GL_DEPTH_TEST);
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        if (material->isTransparent)
        {
            glEnable(GL_BLEND);
            glDepthMask(material->isDepthWriteEnabled ? GL_TRUE : GL_FALSE);
        }
        else
        {
            glDepthMask(GL_FALSE);
            glDisable(GL_BLEND);
        }

        if (material->depthOffset != 0.f)
        {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(0.f, -100.f * material->depthOffset);
        }
        else
        {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }

        if (highlightModeEnabled)
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	        glDisable(GL_CULL_FACE);
            glStencilMask(0xFF);

	        if (highlightStep == 0)
	        {
	            glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_EQUAL);
	            glDepthMask(GL_FALSE);
	        }
	        else if (highlightStep < 3)
	        {
	            glEnable(GL_DEPTH_TEST);
                glDepthFunc(GL_LESS);
	            glDepthMask(GL_TRUE);
	        }
	        else
	        {
	            glDisable(GL_DEPTH_TEST);
	            glDepthMask(GL_FALSE);
	        }
        }
        else
        {
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilMask(0xFF);
        }

        const char* shaderProgram = material->alphaCutoff > 0.f ? "lit_discard" : "lit";
        if (material->normalMapTexture != 0)
        {
            shaderProgram = "lit_normal_map";
        }
        glUseProgram(renderer->getShaderProgram(shaderProgram));
    }

    void onLitPass(Renderer* renderer) override
    {
        glBindTextureUnit(0, textureColor);
        if (textureNormal)
        {
            glBindTextureUnit(5, textureNormal);
        }

        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(transform));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(transform));
        if (material->alphaCutoff > 0.f)
        {
            glUniform1f(5, material->alphaCutoff);
        }
        glUniform1f(8, material->windAmount);
        glBindVertexArray(mesh->vao);

        if (highlightModeEnabled)
        {
            if (highlightStep == 0)
            {
                glStencilFunc(GL_ALWAYS, stencil, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            }
            else if (highlightStep == 1)
            {
                glStencilFunc(GL_EQUAL, stencil, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            }
            else if (highlightStep == 2)
            {
                glStencilFunc(GL_ALWAYS, stencil | STENCIL_HIDDEN, 0xFF);
                glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            }
            else if (highlightStep == 3)
            {
                glStencilFunc(GL_EQUAL, 0, 0xFF);
                glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
            }
            glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
        }
        else
        {
            glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normalMatrix));
            glUniform3fv(2, 1, (GLfloat*)&material->color);
            glUniform3f(3, material->fresnelBias, material->fresnelScale, material->fresnelPower);
            glUniform3f(4, material->specularPower, material->specularStrength, 0.f);
            glm::vec3 emission = material->emit * material->emitPower;
            glUniform3fv(6, 1, (GLfloat*)&emission);
            glUniform3f(7, material->reflectionStrength, material->reflectionLod, material->reflectionBias);
            glStencilFunc(GL_ALWAYS, stencil, 0xFF);
            glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
        }
    }

    void onPickPassPriorityTransition(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram(
                    material->alphaCutoff > 0.f ? "lit_discard_id" : "lit_id"));
    }

    void onPickPass(Renderer* renderer) override
    {
        if (pickID == 0)
        {
            return;
        }

        glBindTextureUnit(0, textureColor);

        glm::mat3 normalMatrix = glm::inverseTranspose(glm::mat3(transform));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(transform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(normalMatrix));
        if (material->alphaCutoff > 0.f)
        {
            glUniform1f(5, material->alphaCutoff);
        }
        glUniform1f(8, material->windAmount);
        glUniform1ui(9, pickID);

        glBindVertexArray(mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->numIndices, GL_UNSIGNED_INT, 0);
    }

    std::string getDebugString() const override
    {
        return "LitMaterialRenderable";
    }
};

struct LitSettings
{
    glm::vec3 color = { 1, 1, 1 };
    f32 specularPower = 50.f;
    glm::vec3 emit = { 0, 0, 0 };
    f32 specularStrength = 0.f;
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
            texture = &g_res.white;
        }
        settings.texture = texture;
        settings.worldTransform = worldTransform;
    }
    LitRenderable(LitSettings const& settings) : settings(settings)
    {
        if (!this->settings.texture)
        {
            this->settings.texture = &g_res.white;
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
        if (settings.minAlpha > 0.f)
        {
            glUniform1f(5, settings.minAlpha);
        }
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
        if (settings.minAlpha > 0.f)
        {
            glUniform1f(5, settings.transparent ? 0.5f : settings.minAlpha);
        }
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
        glStencilMask(0x0);
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
        //glUniform1f(5, settings.minAlpha);
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

    i32 getPriority() const override { return 400000 + priorityOffset; }

    void onLitPassPriorityTransition(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram("overlay"));
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glDisable(GL_BLEND);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glStencilMask(0x0);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
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
        glStencilMask(0x0);

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
