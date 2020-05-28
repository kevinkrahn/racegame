#include "material.h"
#include "game.h"

void Material::loadShaderHandles(SmallArray<ShaderDefine> additionalDefines)
{
    colorShaderHandle = 0;
    if (isVisible)
    {
        SmallArray<ShaderDefine> defines = additionalDefines;
        if (alphaCutoff > 0.f) { defines.push_back({ "ALPHA_DISCARD" }); }
        if (normalMapTexture != 0) { defines.push_back({ "NORMAL_MAP" }); }
        colorShaderHandle = getShaderHandle("lit", defines);
    }
    shadowShaderHandle = 0;
    if (castsShadow)
    {
        SmallArray<ShaderDefine> defines = additionalDefines;
        defines.push_back({ "DEPTH_ONLY" });
        if (shadowAlphaCutoff > 0.f) { defines.push_back({ "ALPHA_DISCARD" }); }
        shadowShaderHandle = getShaderHandle("lit", defines);
    }
    depthShaderHandle = 0;
    if (isDepthWriteEnabled)
    {
        SmallArray<ShaderDefine> defines = additionalDefines;
        defines.push_back({ "DEPTH_ONLY" });
        if (alphaCutoff > 0.f) { defines.push_back({ "ALPHA_DISCARD" }); }
        depthShaderHandle = getShaderHandle("lit", defines);
    }
    {
        SmallArray<ShaderDefine> defines = additionalDefines;
        defines.push_back({ "OUT_ID" });
        if (alphaCutoff > 0.f) { defines.push_back({ "ALPHA_DISCARD" }); }
        pickShaderHandle = getShaderHandle("lit", defines);
    }
    renderFlags = 0;
    if (isCullingEnabled) { renderFlags |= RenderFlags::CULLING; }
    if (depthOffset != 0) { renderFlags |= RenderFlags::DEPTH_OFFSET; }
    if (!isDepthReadEnabled != 0) { renderFlags |= RenderFlags::NO_DEPTH_READ; }
    if (!isDepthWriteEnabled != 0) { renderFlags |= RenderFlags::NO_DEPTH_WRITE; }
    if (isTransparent) { renderFlags |= RenderFlags::TRANSPARENT; }
    textureColorHandle = colorTexture
        ? g_res.getTexture(colorTexture)->handle : g_res.white.handle;
    textureNormalHandle = normalMapTexture
        ? g_res.getTexture(normalMapTexture)->handle : 0;
}

struct MaterialRenderData
{
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
};

void addRenderItemFromMaterial(RenderWorld* rw, Material* material,
        glm::mat4 const& transform, Mesh* mesh)
{
    MaterialRenderData* d = g_game.tempMem.bump<MaterialRenderData>();
    d->worldTransform = transform;
    d->normalTransform = glm::inverseTranspose(glm::mat3(transform));
    d->textureColor = material->textureColorHandle;
    d->textureNormal = material->textureNormalHandle;
    d->color = material->color;
    d->emission = material->emit * material->emitPower;
    d->fresnelBias = material->fresnelBias;
    d->fresnelPower = material->fresnelPower;
    d->fresnelScale = material->fresnelScale;
    d->specularPower = material->specularPower;
    d->specularStrength = material->specularStrength;
    d->reflectionStrength = material->reflectionStrength;
    d->reflectionLod = material->reflectionLod;
    d->reflectionBias = material->reflectionBias;
    d->alphaCutoff = material->alphaCutoff;
    d->shadowAlphaCutoff = material->shadowAlphaCutoff;
    d->windAmount = material->windAmount;

    rw->addColorPassItem({
        material->colorShaderHandle,
        mesh->vao,
        mesh->numIndices,
        material->renderFlags,
        material->depthOffset, [d] {
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
        }
    });

    if (material->castsShadow)
    {
        rw->addShadowPassItem({
            material->shadowShaderHandle,
            mesh->vao,
            mesh->numIndices,
            material->renderFlags,
            material->depthOffset, [d] {
                glBindTextureUnit(0, d->textureColor);
                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
                glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
                if (d->shadowAlphaCutoff > 0.f) { glUniform1f(5, d->shadowAlphaCutoff); }
                glUniform1f(8, d->windAmount);
            }
        });
    }

    if (material->isDepthWriteEnabled)
    {
        rw->addDepthPrepassItem({
            material->depthShaderHandle,
            mesh->vao,
            mesh->numIndices,
            material->renderFlags,
            material->depthOffset, [d] {
                glBindTextureUnit(0, d->textureColor);
                glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(d->worldTransform));
                glUniformMatrix3fv(1, 1, GL_FALSE, glm::value_ptr(d->normalTransform));
                if (d->alphaCutoff > 0.f) { glUniform1f(5, d->alphaCutoff); }
                glUniform1f(8, d->windAmount);
            }
        });
    }
}
