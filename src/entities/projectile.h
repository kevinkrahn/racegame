#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"

class Projectile : public Entity, public PxQueryFilterCallback
{
    PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape,
            const PxRigidActor* actor, PxHitFlags& queryFlags) override
    {
        for (const PxRigidActor* a : ignoreActors)
        {
            if (a == actor)
            {
                return PxQueryHitType::eNONE;
            }
        }
        ActorUserData* userData = (ActorUserData*)actor->userData;
        if (userData && userData->entityType == ActorUserData::VEHICLE)
        {
            if (ignoreActors.size() == ignoreActors.capacity())
            {
                return PxQueryHitType::eBLOCK;
            }
            return projectileType == PHANTOM ? PxQueryHitType::eTOUCH : PxQueryHitType::eBLOCK;
        }
        return PxQueryHitType::eBLOCK;
    }

    // not used
    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
    {
        return PxQueryHitType::eBLOCK;
    }

public:
    enum ProjectileType
    {
        BLASTER,
        BULLET,
        MISSILE,
        BOUNCER,
        PHANTOM,
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
    f32 maxSpeed = 100.f;
    ProjectileType projectileType;
    SmallVec<const PxRigidActor*> ignoreActors;
    f32 homingSpeed = 0.f;

    void onHit(Scene* scene, PxSweepHit* hit);
    void createImpactParticles(Scene* scene, PxSweepHit* hit);

public:
    Projectile(glm::vec3 const& position, glm::vec3 const& velocity,
            glm::vec3 const& upVector, u32 instigator, ProjectileType projectileType);

    void onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onCreate(class Scene* scene) override;
};
