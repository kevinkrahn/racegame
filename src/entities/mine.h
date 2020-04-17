#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class Mine : public Entity
{
    glm::mat4 transform;
    u32 instigator;
    Mesh* mesh;
    f32 aliveTime = 0.f;

public:
    Mine(glm::mat4 const& transform, u32 instigator)
        : transform(transform), instigator(instigator)
    {
        mesh = g_res.getMesh("mine.Mine");
        // TODO: add collision mesh so that AI can avoid it
    }

    void onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
};
