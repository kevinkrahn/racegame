#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class Flash : public Entity
{
    glm::vec3 position;
    glm::vec3 velocity;
    f32 life = 0.f;
    f32 scale;
    f32 angle;

public:
    Flash(glm::vec3 const& position, glm::vec3 const& velocity, f32 scale)
        : position(position), velocity(velocity), scale(scale) {}

    void onCreate(Scene* scene) override;
    void onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime) override;
    void onRender(Renderer* renderer, Scene* scene, f32 deltaTime) override;
};
