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
        mesh = g_resources.getMesh("mine.Mine");
    }

    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    void onRender(Renderer* renderer, Scene* scene, f32 deltaTime) override;
};
