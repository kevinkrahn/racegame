#include "mine.h"
#include "../renderer.h"
#include "../scene.h"
#include "../mesh_renderables.h"
#include "../billboard.h"
#include "../vehicle.h"

void Mine::onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    bool activated = false;
    aliveTime += deltaTime;

    // TODO: use a trigger instead for better performance
    PxOverlapHit hitBuffer[8];
    PxOverlapBuffer hit(hitBuffer, ARRAY_SIZE(hitBuffer));
    PxQueryFilterData filter;
    filter.flags = PxQueryFlag::eDYNAMIC;
    filter.data = PxFilterData(COLLISION_FLAG_CHASSIS, 0, 0, 0);
    f32 radius = 1.2f;
    if (scene->getPhysicsScene()->overlap(PxSphereGeometry(radius),
            PxTransform(convert(translationOf(transform)), PxIdentity), hit, filter))
    {
        for (u32 i=0; i<hit.getNbTouches(); ++i)
        {
            PxActor* actor = hit.getTouch(i).actor;
            ActorUserData* userData = (ActorUserData*)actor->userData;
            if (userData && (userData->entityType == ActorUserData::VEHICLE))
            {
                Vehicle* vehicle = (Vehicle*)userData->vehicle;
                if (aliveTime < 1.5f && vehicle->vehicleIndex == instigator)
                {
                    continue;
                }
                else
                {
                    activated = true;
                    break;
                }
            }
        }
    }

    if (activated)
    {
        scene->createExplosion(translationOf(transform), glm::vec3(0.f), 10.f);
        this->destroy();
        Sound* sounds[] = {
            &g_res.sounds->explosion2,
            &g_res.sounds->explosion3,
        };
        u32 index = irandom(scene->randomSeries, 0, ARRAY_SIZE(sounds));
        g_audio.playSound3D(sounds[index], SoundType::GAME_SFX,
                translationOf(transform), false, 1.f, 0.95f);

        PxOverlapHit hitBuffer[8];
        PxOverlapBuffer hit(hitBuffer, ARRAY_SIZE(hitBuffer));
        PxQueryFilterData filter;
        filter.flags |= PxQueryFlag::eDYNAMIC;
        filter.data = PxFilterData(COLLISION_FLAG_CHASSIS, 0, 0, 0);
        f32 radius = 3.f;
        if (scene->getPhysicsScene()->overlap(PxSphereGeometry(radius),
                PxTransform(convert(translationOf(transform)), PxIdentity), hit, filter))
        {
            for (u32 i=0; i<hit.getNbTouches(); ++i)
            {
                PxActor* actor = hit.getTouch(i).actor;
                ActorUserData* userData = (ActorUserData*)actor->userData;
                if (userData && (userData->entityType == ActorUserData::VEHICLE))
                {
                    Vehicle* vehicle = (Vehicle*)userData->vehicle;
                    if (!vehicle->hasAbility("Underplating"))
                    {
                        vehicle->applyDamage(50.f, instigator);
                    }
                    vehicle->getRigidBody()->addForce(convert(zAxisOf(transform) * 6000.f),
                            PxForceMode::eIMPULSE);
                }
            }
        }
    }
}

void Mine::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    rw->push(LitRenderable(mesh, transform, nullptr));
    rw->push(BillboardRenderable(&g_res.textures->flare,
                translationOf(transform) + glm::vec3(0,0,0.7f), {2.f,0.02f,0.02f,0.3f},
                (glm::sin(aliveTime * 2.f) + 2.f) * 0.3f, 0.f, false));
}
