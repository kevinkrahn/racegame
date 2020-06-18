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
        colorShaderHandle = getShaderHandle("lit", defines, renderFlags, -100.f * depthOffset);
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
    Mat4 worldTransform;
    Mat3 normalTransform;
    GLuint textureColor;
    GLuint textureNormal;
    Vec3 color;
    Vec3 emission;
    f32 fresnelBias, fresnelScale, fresnelPower;
    f32 specularPower, specularStrength;
    f32 reflectionStrength, reflectionLod, reflectionBias;
    f32 alphaCutoff, shadowAlphaCutoff;
    f32 windAmount;
    u32 pickValue;
};

void Material::draw(RenderWorld* rw, Mat4 const& transform, Mesh* mesh, u8 stencil)
{
    MaterialRenderData* d = g_game.tempMem.bump<MaterialRenderData>();
#ifndef NDEBUG
    d->material = this;
#endif
    d->vao = mesh->vao;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->normalTransform = inverseTranspose(Mat3(transform));
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
        glUniformMatrix4fv(0, 1, GL_FALSE, d->worldTransform.valuePtr());
        glUniformMatrix3fv(1, 1, GL_FALSE, d->normalTransform.valuePtr());
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
        glUniformMatrix4fv(0, 1, GL_FALSE, d->worldTransform.valuePtr());
        glUniformMatrix3fv(1, 1, GL_FALSE, d->normalTransform.valuePtr());
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
        i32 priority = depthOffset > 0.f ? TransparentDepth::FLAT_SPLINE : 0;
        rw->transparentPass({ colorShaderHandle, priority, d, renderColor });
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

void Material::drawPick(RenderWorld* rw, Mat4 const& transform, Mesh* mesh, u32 pickValue)
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
    d->windAmount = windAmount;
    d->pickValue = pickValue;

    auto render = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, d->worldTransform.valuePtr());
        glUniformMatrix3fv(1, 1, GL_FALSE, d->normalTransform.valuePtr());
        if (d->alphaCutoff > 0.f)
        {
            glBindTextureUnit(0, d->textureColor);
            glUniform1f(5, d->alphaCutoff);
        }
        glUniform1f(8, d->windAmount);
        glUniform1ui(9, d->pickValue);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };
    rw->pickPass(pickShaderHandle, { d, render });
}

void Material::drawHighlight(RenderWorld* rw, Mat4 const& transform, Mesh* mesh, u8 stencil, u8 cameraIndex)
{
    MaterialRenderData* d = g_game.tempMem.bump<MaterialRenderData>();
#ifndef NDEBUG
    d->material = this;
#endif
    d->textureColor = textureColorHandle;
    d->vao = mesh->vao;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->textureColor = textureColorHandle;
    d->alphaCutoff = alphaCutoff;
    d->windAmount = windAmount;

    auto render = [](void* renderData) {
        MaterialRenderData* d = (MaterialRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, d->worldTransform.valuePtr());
        if (d->alphaCutoff > 0.f)
        {
            glBindTextureUnit(0, d->textureColor);
            glUniform1f(5, d->alphaCutoff);
        }
        glUniform1f(8, d->windAmount);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    rw->highlightPass(depthShaderHandle, { d, render, stencil, cameraIndex });
}

struct VehicleRenderData
{
#ifndef NDEBUG
    Material* material = nullptr;
#endif
    GLuint vao;
    u32 indexCount;
    Mat4 worldTransform;
    Mat3 normalTransform;
    Vec3 color;
    Vec3 emission;
    f32 fresnelBias, fresnelScale, fresnelPower;
    f32 specularPower, specularStrength;
    f32 reflectionStrength, reflectionLod, reflectionBias;
    Vec4 shield;
    GLuint wrapTexture[3];
    Vec4 wrapColor[3];
};

void Material::drawVehicle(class RenderWorld* rw, Mat4 const& transform, struct Mesh* mesh,
        u8 stencil, Vec4 const& shield, i64 wrapTextureGuids[3], Vec4 wrapColor[3])
{
    VehicleRenderData* d = g_game.tempMem.bump<VehicleRenderData>();
#ifndef NDEBUG
    d->material = this;
#endif
    d->vao = mesh->vao;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->normalTransform = inverseTranspose(Mat3(transform));
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
    d->shield = shield;
    d->wrapTexture[0] = g_res.getTexture(wrapTextureGuids[0])->handle;
    d->wrapTexture[1] = g_res.getTexture(wrapTextureGuids[1])->handle;
    d->wrapTexture[2] = g_res.getTexture(wrapTextureGuids[2])->handle;
    d->wrapColor[0] = wrapTextureGuids[0] != 0 ? wrapColor[0] : Vec4(0);
    d->wrapColor[1] = wrapTextureGuids[1] != 0 ? wrapColor[1] : Vec4(0);
    d->wrapColor[2] = wrapTextureGuids[2] != 0 ? wrapColor[2] : Vec4(0);

    auto renderOpaque = [](void* renderData) {
        VehicleRenderData* d = (VehicleRenderData*)renderData;
        glBindTextureUnit(6, d->wrapTexture[0]);
        glBindTextureUnit(7, d->wrapTexture[1]);
        glBindTextureUnit(8, d->wrapTexture[2]);
        glUniformMatrix4fv(0, 1, GL_FALSE, d->worldTransform.valuePtr());
        glUniformMatrix3fv(1, 1, GL_FALSE, d->normalTransform.valuePtr());
        glUniform3fv(2, 1, (GLfloat*)&d->color);
        glUniform3f(3, d->fresnelBias, d->fresnelScale, d->fresnelPower);
        glUniform3f(4, d->specularPower, d->specularStrength, 0.f);
        glUniform3fv(6, 1, (GLfloat*)&d->emission);
        glUniform3f(7, d->reflectionStrength, d->reflectionLod, d->reflectionBias);
        glUniform4fv(10, 1, (GLfloat*)&d->shield);
        glUniform4fv(11, 3, (GLfloat*)&d->wrapColor);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    auto renderDepth = [](void* renderData) {
        VehicleRenderData* d = (VehicleRenderData*)renderData;
        glUniformMatrix4fv(0, 1, GL_FALSE, d->worldTransform.valuePtr());
        glUniformMatrix3fv(1, 1, GL_FALSE, d->normalTransform.valuePtr());
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    rw->depthPrepass(depthShaderHandle, { d, renderDepth });
    rw->shadowPass(shadowShaderHandle, { d, renderDepth });
    rw->opaqueColorPass(colorShaderHandle, { d, renderOpaque, stencil });
}

struct SimpleRenderData
{
    GLuint vao;
    GLuint tex;
    u32 indexCount;
    Mat4 worldTransform;
    Mat3 normalTransform;
    Vec3 color;
    Vec3 emit;
};

void drawSimple(RenderWorld* rw, Mesh* mesh, Texture* tex, Mat4 const& transform,
        Vec3 const& color, Vec3 const& emit)
{
    static ShaderHandle shader = getShaderHandle("lit");
    static ShaderHandle depthShader = getShaderHandle("lit", { { "DEPTH_ONLY" } });

    SimpleRenderData* d = g_game.tempMem.bump<SimpleRenderData>();
    d->vao = mesh->vao;
    d->tex = tex->handle;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->normalTransform = inverseTranspose(Mat3(transform));
    d->color = color;
    d->emit = emit;

    auto renderOpaque = [](void* renderData) {
        SimpleRenderData* d = (SimpleRenderData*)renderData;
        glBindTextureUnit(0, d->tex);
        glUniformMatrix4fv(0, 1, GL_FALSE, d->worldTransform.valuePtr());
        glUniformMatrix3fv(1, 1, GL_FALSE, d->normalTransform.valuePtr());
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
        glUniformMatrix4fv(0, 1, GL_FALSE, d->worldTransform.valuePtr());
        glUniformMatrix3fv(1, 1, GL_FALSE, d->normalTransform.valuePtr());
        glUniform1f(8, 0.f);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);
    };

    rw->depthPrepass(depthShader, { d, renderDepth });
    rw->shadowPass(depthShader, { d, renderDepth });
    rw->opaqueColorPass(shader, { d, renderOpaque, 0 });
}

void drawWireframe(RenderWorld* rw, Mesh* mesh, Mat4 const& transform, Vec4 color)
{
    static ShaderHandle shader = getShaderHandle("debug");

    SimpleRenderData* d = g_game.tempMem.bump<SimpleRenderData>();
    d->vao = mesh->vao;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->color = color.rgb;
    d->emit.r = color.a;

    auto renderOpaque = [](void* renderData) {
        SimpleRenderData* d = (SimpleRenderData*)renderData;

        Camera const& camera = g_game.renderer->getRenderWorld()->getCamera(0);
        glUniform4f(2, d->color.x, d->color.y, d->color.z, d->emit.r);
        glUniform1i(3, 1);
        Mat4 t = camera.viewProjection * d->worldTransform;
        glUniformMatrix4fv(1, 1, GL_FALSE, t.valuePtr());

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    };

    rw->transparentPass({ shader, TransparentDepth::DEBUG_LINES, d, renderOpaque });
}

void drawOverlay(RenderWorld* rw, Mesh* mesh, Mat4 const& transform, Vec3 const& color,
        i32 priorityOffset, bool onlyDepth)
{
    static ShaderHandle shader = getShaderHandle("overlay", {},
            RenderFlags::DEPTH_READ | RenderFlags::DEPTH_WRITE, -3000000.f);

    struct OverlayRenderData
    {
        Mat4 worldTransform;
        Vec3 color;
        GLuint vao;
        u32 indexCount;
        bool onlyDepth;
    };

    OverlayRenderData* d = g_game.tempMem.bump<OverlayRenderData>();
    d->vao = mesh->vao;
    d->indexCount = mesh->numIndices;
    d->worldTransform = transform;
    d->color = color;
    d->onlyDepth = onlyDepth;

    auto render = [](void* renderData) {
        OverlayRenderData* d = (OverlayRenderData*)renderData;

        glDepthMask(GL_TRUE);

        if (d->onlyDepth)
        {
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        }

        glUniformMatrix4fv(0, 1, GL_FALSE, d->worldTransform.valuePtr());
        glUniform3fv(1, 1, (f32*)&d->color);
        glBindVertexArray(d->vao);
        glDrawElements(GL_TRIANGLES, d->indexCount, GL_UNSIGNED_INT, 0);

        if (d->onlyDepth)
        {
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        }
    };

    rw->transparentPass({ shader, TransparentDepth::OVERLAY + priorityOffset, d, render });
}
