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
        BULLET
    };

private:
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 upVector;
    u32 instigator;
    Mesh* bulletMesh;
    f32 life;
    f32 groundFollow;
    f32 collisionRadius;
    u32 damage;
    ProjectileType projectileType;

public:
    Projectile(glm::vec3 const& position, glm::vec3 const& velocity,
            glm::vec3 const& upVector, u32 instigator, ProjectileType projectileType)
        : position(position), velocity(velocity), upVector(upVector),
          instigator(instigator), projectileType(projectileType)
    {
        bulletMesh = g_resources.getMesh("world.Bullet");
        switch(projectileType)
        {
            case BLASTER:
                life = 3.f;
                groundFollow = 1.f;
                collisionRadius = 0.3f;
                damage = 60.f;
                break;
            case BULLET:
                life = 2.f;
                groundFollow = 0.1f;
                collisionRadius = 0.1f;
                damage = 10.f;
                break;
        }
    }

    void onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
};
