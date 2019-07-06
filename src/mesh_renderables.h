#pragma once

#include "renderable.h"
#include "resources.h"
#include "renderer.h"

class LitRenderable : public Renderable
{
    glm::vec3 color = { 1, 1, 1 };
    glm::vec3 emit = { 0, 0, 0 };
    f32 specularPower = 50.f;
    f32 specularStrength = 0.2f;
    glm::vec3 specularColor = { 1, 1, 1 };
    f32 fresnelScale = 0.8f;
    f32 fresnelPower = 3.f;
    f32 fresnelBias = -0.1f;

    u32 shader;
    bool culling = true;
    bool castShadow = true;

    Mesh* mesh = nullptr;
    glm::mat4 worldTransform;

public:
    LitRenderable(Mesh* mesh, glm::mat4 const& worldTransform) : mesh(mesh), worldTransform(worldTransform) {}

    i32 getPriority() const override { return 0; }
    void onDepthPrepass(Renderer* renderer) override
    {
        glUseProgram(renderer->getShaderProgram("lit"));
        glUniformMatrix4fv(0, 1, GL_FALSE, glm::value_ptr(worldTransform));
        glBindVertexArray(mesh->vao);
    }

    void onShadowPass(Renderer* renderer) override;
    void onLitPass(Renderer* renderer) override;
};
