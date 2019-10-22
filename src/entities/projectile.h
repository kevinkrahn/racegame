#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class Projectile : public Entity
{
public:
    enum ProjectileType
    {
        BLASTER,
        BULLET,
        MISSILE,
        BOUNCER,
    };

private:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 upVector;
    u32 instigator;
    Mesh* bulletMesh;
    f32 life;
    bool groundFollow;
    f32 collisionRadius;
    u32 damage;
    f32 accel = 0.f;
    ProjectileType projectileType;

public:
    Projectile(glm::vec3 const& position, glm::vec3 const& velocity,
            glm::vec3 const& upVector, u32 instigator, ProjectileType projectileType);

    void onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
};
