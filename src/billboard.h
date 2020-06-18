#pragma once

#include "renderer.h"
#include "game.h"

struct BillboardRenderData
{
    GLuint tex;
    f32 scale;
    Vec4 color;
    Mat4 translation;
    Mat4 rotation;
};

void drawBillboard(RenderWorld* rw, Texture* texture, Vec3 const& position, Vec4 const& color,
        f32 scale=1.f, f32 angle=0.f, bool lit=true)
{
    static ShaderHandle shaderLit = getShaderHandle("billboard", { {"LIT"} });
    static ShaderHandle shaderUnlit = getShaderHandle("billboard", {});

    BillboardRenderData* renderData = g_game.tempMem.bump<BillboardRenderData>();
    renderData->tex = texture->handle;
    renderData->scale = scale;
    renderData->color = color;
    renderData->translation = Mat4::translation(position);
    renderData->rotation = Mat4::rotationZ(angle);

    rw->transparentPass({ lit ? shaderLit : shaderUnlit, TransparentDepth::BILLBOARD, renderData, [](void* renderData){
        BillboardRenderData* rd = (BillboardRenderData*)renderData;
        glBindTextureUnit(0, rd->tex);
        glUniform4fv(0, 1, (GLfloat*)&rd->color);
        glUniformMatrix4fv(1, 1, GL_FALSE, rd->translation.valuePtr());
        glUniform3f(2, rd->scale, rd->scale, rd->scale);
        glUniformMatrix4fv(3, 1, GL_FALSE, rd->rotation.valuePtr());
        glDrawArrays(GL_TRIANGLES, 0, 6);
    } });
}
