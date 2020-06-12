#pragma once

#include "renderer.h"
#include "game.h"

struct BillboardRenderData
{
    GLuint tex;
    f32 scale;
    glm::vec4 color;
    glm::mat4 translation;
    glm::mat4 rotation;
};

void drawBillboard(RenderWorld* rw, Texture* texture, glm::vec3 const& position, glm::vec4 const& color,
        f32 scale=1.f, f32 angle=0.f, bool lit=true)
{
    static ShaderHandle shaderLit = getShaderHandle("billboard", { {"LIT"} });
    static ShaderHandle shaderUnlit = getShaderHandle("billboard", {});
    static Mesh* mesh = g_res.getModel("misc")->getMeshByName("world.Sphere");

    BillboardRenderData* renderData = g_game.tempMem.bump<BillboardRenderData>();
    renderData->tex = texture->handle;
    renderData->scale = scale;
    renderData->color = color;
    renderData->translation = glm::translate(glm::mat4(1.f), position);
    renderData->rotation = glm::rotate(glm::mat4(1.f), angle, { 0, 0, 1 });
    rw->transparentPass(lit ? shaderLit : shaderUnlit, { mesh->vao, 6, renderData, [](void* renderData){
        BillboardRenderData* rd = (BillboardRenderData*)renderData;
        glBindTextureUnit(0, rd->tex);
        glUniform4fv(0, 1, (GLfloat*)&rd->color);
        glUniformMatrix4fv(1, 1, GL_FALSE, glm::value_ptr(rd->translation));
        glUniform3f(2, rd->scale, rd->scale, rd->scale);
        glUniformMatrix4fv(3, 1, GL_FALSE, glm::value_ptr(rd->rotation));
    } });
}
