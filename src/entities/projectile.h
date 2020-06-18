#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../particle_system.h"

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
            return passThroughVehicles ? PxQueryHitType::eTOUCH : PxQueryHitType::eBLOCK;
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
    Mesh* bulletMesh;
    Vec3 position;
    Vec3 velocity;
    Vec3 upVector;
    u32 damage;
    u32 instigator;
    f32 life;
    f32 collisionRadius;
    f32 accel = 0.f;
    f32 maxSpeed = 100.f;
    ProjectileType projectileType;
    f32 homingSpeed = 0.f;
    SmallArray<const PxRigidActor*> ignoreActors;
    bool groundFollow = false;
    bool passThroughVehicles = false;
    bool bounceOffEnvironment = false;
    f32 explosionStrength = 0.f;

    ParticleEmitter impactEmitter;
    SmallArray<const char*, 4> environmentImpactSounds;
    SmallArray<const char*, 4> vehicleImpactSounds;

    void onHit(Scene* scene, PxSweepHit* hit);
    void createImpactParticles(Scene* scene, PxSweepHit* hit);

public:
    Projectile(Vec3 const& position, Vec3 const& velocity,
            Vec3 const& upVector, u32 instigator, ProjectileType projectileType)
        : position(position), velocity(velocity), upVector(upVector),
            instigator(instigator), projectileType(projectileType) {}

    void onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override;
    void onCreate(class Scene* scene) override;
};
