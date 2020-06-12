#include "material.h"
#include "game.h"

void Material::loadShaderHandles(SmallArray<ShaderDefine> additionalDefines)
{
    u32 renderFlags = 0;
    //if (isCullingEnabled) { renderFlags |= RenderFlags::BACKFACE_CULL; }
    if (isDepthReadEnabled) { renderFlags |= RenderFlags::DEPTH_READ; }
    if (isDepthWriteEnabled) { renderFlags |= RenderFlags::DEPTH_WRITE; }

    colorShaderHandle = 0;
    if (isVisible)
    {
        SmallArray<ShaderDefine> defines = additionalDefines;
        if (alphaCutoff > 0.f) { defines.push_back({ "ALPHA_DISCARD" }); }
        if (normalMapTexture != 0) { defines.push_back({ "NORMAL_MAP" }); }
        colorShaderHandle = getShaderHandle("lit", defines, renderFlags, depthOffset);
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

    auto setRenderData = [](void* renderData) {
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
    };

    auto setDepthData = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        if (d->alphaCutoff > 0.f)
        {
            glBindTextureUnit(0, d->textureColor);
            glUniform1f(5, d->alphaCutoff);
        }
        glUniform1f(8, d->windAmount);
    };

    if (isTransparent || depthOffset > 0.f || !isDepthWriteEnabled || !isDepthReadEnabled)
    {
        rw->transparentPass(colorShaderHandle, { mesh->vao, mesh->numIndices, d, setRenderData });
    }
    else
    {
        rw->depthPrepass(depthShaderHandle, { mesh->vao, mesh->numIndices, d, setDepthData });
        rw->opaqueColorPass(colorShaderHandle, { mesh->vao, mesh->numIndices, d, setRenderData, stencil });
    }

    if (castsShadow)
    {
        rw->shadowPass(shadowShaderHandle, { mesh->vao, mesh->numIndices, d, setDepthData });
    }
}

void Material::drawHighlight(RenderWorld* rw, glm::mat4 const& transform, Mesh* mesh, u8 stencil, u8 cameraIndex)
{
    MaterialRenderData* d = g_game.tempMem.bump<MaterialRenderData>();
    d->worldTransform = transform;
    d->textureColor = textureColorHandle;
    d->alphaCutoff = alphaCutoff;

    auto setRenderData = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        if (d->alphaCutoff > 0.f)
        {
            glBindTextureUnit(0, d->textureColor);
            glUniform1f(5, d->alphaCutoff);
        }
        glUniform1f(8, d->windAmount);
    };

    rw->highlightPass(colorShaderHandle,
            { mesh->vao, mesh->numIndices, d, setRenderData, stencil, cameraIndex });
}

void Material::drawVehicle(class RenderWorld* rw, glm::mat4 const& transform, struct Mesh* mesh,
        u8 stencil, glm::vec3 const& shieldColor)
{
    MaterialRenderData* d = g_game.tempMem.bump<MaterialRenderData>();
    d->worldTransform = transform;
    d->normalTransform = glm::inverseTranspose(glm::mat3(transform));
    d->textureColor = textureColorHandle;
    d->textureNormal = textureNormalHandle;
    d->color = color;
    d->fresnelBias = fresnelBias;
    d->fresnelPower = fresnelPower;
    d->fresnelScale = fresnelScale;
    d->specularPower = specularPower;
    d->specularStrength = specularStrength;
    d->reflectionStrength = reflectionStrength;
    d->reflectionLod = reflectionLod;
    d->reflectionBias = reflectionBias;
    d->shieldColor = shieldColor;

    auto setRenderData = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glBindTextureUnit(0, d->textureColor);
        glBindTextureUnit(5, d->textureNormal);
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        glUniform3fv(2, 1, (GLfloat*)&d->color);
        glUniform3f(3, d->fresnelBias, d->fresnelScale, d->fresnelPower);
        glUniform3f(4, d->specularPower, d->specularStrength, 0.f);
        glUniform3f(6, 0.f, 0.f, 0.f);
        glUniform3f(7, d->reflectionStrength, d->reflectionLod, d->reflectionBias);
        glUniform1f(8, 0.f);
        glUniform3fv(10, 1, (GLfloat*)&d->shieldColor);
    };

    auto setDepthData = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
        glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
        glUniform1f(8, 0.f);
    };

    rw->depthPrepass(depthShaderHandle, { mesh->vao, mesh->numIndices, d, setDepthData });
    rw->opaqueColorPass(colorShaderHandle, { mesh->vao, mesh->numIndices, d, setRenderData, stencil });
    rw->shadowPass(shadowShaderHandle, { mesh->vao, mesh->numIndices, d, setDepthData });
}
