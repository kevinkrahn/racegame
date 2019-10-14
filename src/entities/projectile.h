#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class Projectile : public Entity
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 upVector;
    u32 instigator;
    Mesh* bulletMesh;
    f32 life = 3.f;

public:
    Projectile(glm::vec3 const& position, glm::vec3 const& velocity,
            glm::vec3 const& upVector, u32 instigator)
        : position(position), velocity(velocity), upVector(upVector), instigator(instigator)
    {
        bulletMesh = g_resources.getMesh("world.Bullet");
    }

    void onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
};
