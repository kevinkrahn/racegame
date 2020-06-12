#include "material.h"
#include "game.h"

void Material::loadShaderHandles(SmallArray<ShaderDefine> additionalDefines)
{
    u32 renderFlags = 0;
    if (isDepthReadEnabled) { renderFlags |= RenderFlags::DEPTH_READ; }
    if (isDepthWriteEnabled) { renderFlags |= RenderFlags::DEPTH_WRITE; }

    colorShaderHandle = 0;
    if (isVisible)
    {
        SmallArray<ShaderDefine> defines = additionalDefines;
        if (alphaCutoff > 0.f) { defines.push_back({ "ALPHA_DISCARD" }); }
        if (normalMapTexture != 0) { defines.push_back({ "NORMAL_MAP" }); }
        colorShaderHandle = getShaderHandle("lit", defines, renderFlags, 100 * depthOffset);
    }
    shadowShaderHandle = 0;
    if (castsShadow)
    {
        SmallArray<ShaderDefine> defines = additionalDefines;
        defines.push_back({ "DEPTH_ONLY" });
        if (shadowAlphaCutoff > 0.f) { defines.push_back({ "ALPHA_DISCARD" }); }
        shadowShaderHandle = getShaderHandle("lit", defines, renderFlags);
    }
    depthShaderHandle = 0;
    if (isDepthWriteEnabled)
    {
        SmallArray<ShaderDefine> defines = additionalDefines;
        defines.push_back({ "DEPTH_ONLY" });
        if (alphaCutoff > 0.f) { defines.push_back({ "ALPHA_DISCARD" }); }
        depthShaderHandle = getShaderHandle("lit", defines, renderFlags);
    }
    {
        SmallArray<ShaderDefine> defines = additionalDefines;
        defines.push_back({ "OUT_ID" });
        if (alphaCutoff > 0.f) { defines.push_back({ "ALPHA_DISCARD" }); }
        pickShaderHandle = getShaderHandle("lit", defines, renderFlags);
    }
    textureColorHandle = colorTexture
        ? g_res.getTexture(colorTexture)->handle : g_res.white.handle;
    textureNormalHandle = normalMapTexture
        ? g_res.getTexture(normalMapTexture)->handle : 0;
}

struct MaterialRenderData
{
#ifndef NDEBUG
    Material* material = nullptr;
#endif
    GLuint vao;
    u32 indexCount;
    glm::mat4 worldTransform;
    glm::mat3 normalTransform;
    GLuint textureColor;
    GLuint textureNormal;
    glm::vec3 color;
    glm::vec3 emission;
    f32 fresnelBias, fresnelScale, fresnelPower;
    f32 specularPower, specularStrength;
    f32 reflectionStrength, reflectionLod, reflectionBias;
    f32 alphaCutoff, shadowAlphaCutoff;
    f32 windAmount;
    glm::vec3 shieldColor;
};

void Material::draw(RenderWorld* rw, glm::mat4 const& transform, Mesh* mesh, u8 stencil)
{
    MaterialRenderData* d = g_game.tempMem.bump<MaterialRenderData>();
#ifndef NDEBUG
    d->material = this;
#endif
    d->vao = mesh->vao;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->normalTransform = glm::inverseTranspose(glm::mat3(transform));
    d->textureColor = textureColorHandle;
    d->textureNormal = textureNormalHandle;
    d->color = color;
    d->emission = emit * emitPower;
    d->fresnelBias = fresnelBias;
    d->fresnelPower = fresnelPower;
    d->fresnelScale = fresnelScale;
    d->specularPower = specularPower;
    d->specularStrength = specularStrength;
    d->reflectionStrength = reflectionStrength;
    d->reflectionLod = reflectionLod;
    d->reflectionBias = reflectionBias;
    d->alphaCutoff = alphaCutoff;
    d->shadowAlphaCutoff = shadowAlphaCutoff;
    d->windAmount = windAmount;

    auto renderColor = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glBindTextureUnit(0, d->textureColor);
        glBindTextureUnit(5, d->textureNormal);
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        glUniform3fv(2, 1, (GLfloat*)&d->color);
        glUniform3f(3, d->fresnelBias, d->fresnelScale, d->fresnelPower);
        glUniform3f(4, d->specularPower, d->specularStrength, 0.f);
        if (d->alphaCutoff > 0.f) { glUniform1f(5, d->alphaCutoff); }
        glUniform3fv(6, 1, (GLfloat*)&d->emission);
        glUniform3f(7, d->reflectionStrength, d->reflectionLod, d->reflectionBias);
        glUniform1f(8, d->windAmount);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    auto renderDepth = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        if (d->alphaCutoff > 0.f)
        {
            glBindTextureUnit(0, d->textureColor);
            glUniform1f(5, d->alphaCutoff);
        }
        glUniform1f(8, d->windAmount);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    if (isTransparent || depthOffset > 0.f || !isDepthWriteEnabled || !isDepthReadEnabled)
    {
        rw->transparentPass(colorShaderHandle, { d, renderColor });
    }
    else
    {
        rw->depthPrepass(depthShaderHandle, { d, renderDepth });
        rw->opaqueColorPass(colorShaderHandle, { d, renderColor, stencil });
    }

    if (castsShadow)
    {
        rw->shadowPass(shadowShaderHandle, { d, renderDepth });
    }
}

void Material::drawHighlight(RenderWorld* rw, glm::mat4 const& transform, Mesh* mesh, u8 stencil, u8 cameraIndex)
{
    MaterialRenderData* d = g_game.tempMem.bump<MaterialRenderData>();
#ifndef NDEBUG
    d->material = this;
#endif
    d->vao = mesh->vao;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->textureColor = textureColorHandle;
    d->alphaCutoff = alphaCutoff;

    auto render = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        if (d->alphaCutoff > 0.f)
        {
            glBindTextureUnit(0, d->textureColor);
            glUniform1f(5, d->alphaCutoff);
        }
        glUniform1f(8, d->windAmount);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    rw->highlightPass(colorShaderHandle, { d, render, stencil, cameraIndex });
}

void Material::drawVehicle(class RenderWorld* rw, glm::mat4 const& transform, struct Mesh* mesh,
        u8 stencil, glm::vec3 const& shieldColor)
{
    MaterialRenderData* d = g_game.tempMem.bump<MaterialRenderData>();
#ifndef NDEBUG
    d->material = this;
#endif
    d->vao = mesh->vao;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->normalTransform = glm::inverseTranspose(glm::mat3(transform));
    d->textureColor = textureColorHandle;
    d->textureNormal = textureNormalHandle;
    d->color = color;
    d->emission = emit * emitPower;
    d->fresnelBias = fresnelBias;
    d->fresnelPower = fresnelPower;
    d->fresnelScale = fresnelScale;
    d->specularPower = specularPower;
    d->specularStrength = specularStrength;
    d->reflectionStrength = reflectionStrength;
    d->reflectionLod = reflectionLod;
    d->reflectionBias = reflectionBias;
    d->shieldColor = shieldColor;

    auto renderOpaque = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glBindTextureUnit(0, d->textureColor);
        glBindTextureUnit(5, d->textureNormal);
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        glUniform3fv(2, 1, (GLfloat*)&d->color);
        glUniform3f(3, d->fresnelBias, d->fresnelScale, d->fresnelPower);
        glUniform3f(4, d->specularPower, d->specularStrength, 0.f);
        glUniform3fv(6, 1, (GLfloat*)&d->emission);
        glUniform3f(7, d->reflectionStrength, d->reflectionLod, d->reflectionBias);
        glUniform1f(8, 0.f);
        glUniform3fv(10, 1, (GLfloat*)&d->shieldColor);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    auto renderDepth = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        glUniform1f(8, 0.f);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    rw->depthPrepass(depthShaderHandle, { d, renderDepth });
    rw->opaqueColorPass(colorShaderHandle, { d, renderOpaque, stencil });
    rw->shadowPass(shadowShaderHandle, { d, renderOpaque });
}

struct SimpleRenderData
{
    GLuint vao;
    GLuint tex;
    u32 indexCount;
    glm::mat4 worldTransform;
    glm::mat4 normalTransform;
    glm::vec3 color;
    glm::vec3 emit;
};

void drawSimple(RenderWorld* rw, Mesh* mesh, Texture* tex, glm::mat4 const& transform,
        glm::vec3 const& color, glm::vec3 const& emit)
{
    static ShaderHandle shader = getShaderHandle("lit");
    static ShaderHandle depthShader = getShaderHandle("lit", { { "DEPTH_ONLY" } });

    SimpleRenderData* d = g_game.tempMem.bump<SimpleRenderData>();
    d->vao = mesh->vao;
    d->tex = tex->handle;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->normalTransform = glm::inverseTranspose(glm::mat3(transform));
    d->color = color;
    d->emit = emit;

    auto renderOpaque = [](void* renderData) {
        SimpleRenderData* d = (SimpleRenderData*)renderData;
        glBindTextureUnit(0, d->tex);
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        glUniform3fv(2, 1, (GLfloat*)&d->color);
        glUniform3f(3, 0.f, 0.f, 0.f);
        glUniform3f(4, 300.f, 0.1f, 0.f);
        glUniform3fv(6, 1, (GLfloat*)&d->emit);
        glUniform3f(7, 0.f, 0.f, 0.f);
        glUniform1f(8, 0.f);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    auto renderDepth = [](void* renderData) {
        SimpleRenderData* d = (SimpleRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        glUniform1f(8, 0.f);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    rw->depthPrepass(depthShader, { d, renderDepth });
    rw->opaqueColorPass(shader, { d, renderOpaque, 0 });
    rw->shadowPass(depthShader, { d, renderOpaque });
}

void drawWireframe(RenderWorld* rw, Mesh* mesh, glm::mat4 const& transform, glm::vec4 color)
{
    static ShaderHandle shader = getShaderHandle("debug");

    SimpleRenderData* d = g_game.tempMem.bump<SimpleRenderData>();
    d->vao = mesh->vao;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->color = color;
    d->emit.r = color.a;

    auto renderOpaque = [](void* renderData) {
        SimpleRenderData* d = (SimpleRenderData*)renderData;

        Camera const& camera = g_game.renderer->getRenderWorld()->getCamera(0);
        glUniform4f(2, d->color.x, d->color.y, d->color.z, d->emit.r);
        glUniform1i(3, 1);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(camera.viewProjection * d->worldTransform));

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    };

    rw->transparentPass(shader, { d, renderOpaque });
}

void drawOverlay(RenderWorld* rw, Mesh* mesh, glm::mat4 const& transform, glm::vec3 const& color,
        bool onlyDepth)
{

}

#if 0
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
#endif
