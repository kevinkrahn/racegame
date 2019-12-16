#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../game.h"
#include "../scene.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"

class Pickup : public PlaceableEntity
{
    Mesh* mesh;

public:
    Pickup* setup(glm::vec3 const& position)
    {
        this->position = position;
        return this;
    }

    void onCreate(Scene* scene) override
    {
        this->entityFlags |= Entity::TRANSIENT;
        mesh = g_res.getMesh("money.Money");
        print("creating pickup\n");

        updateTransform(scene);
        actor = g_game.physx.physics->createRigidStatic(
                PxTransform(convert(position), convert(rotation)));
        physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
        physicsUserData.entity = this;
        actor->userData = &physicsUserData;

        PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxSphereGeometry((scale * scaleOf(transform)).z), *scene->genericMaterial);
        collisionShape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE | COLLISION_FLAG_PICKUP,
                    DECAL_NONE, 0, UNDRIVABLE_SURFACE));
        collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_PICKUP, -1, 0, 0));
        collisionShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        collisionShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);

        scene->getPhysicsScene()->addActor(*actor);
    }

    void onTrigger(ActorUserData* userData) override
    {
        if (userData->entityType == ActorUserData::VEHICLE
                // prevents multiple vehicles from collecting the same pickup on the same frame
                && !this->isDestroyed())
        {
            Vehicle* v = userData->vehicle;
            v->addPickupBonus();
            this->destroy();

            g_audio.playSound3D(&g_res.sounds->money, SoundType::GAME_SFX, position);
        }
    }

    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override
    {
        glm::mat4 t =
            glm::translate(glm::mat4(1.f), glm::vec3(0, 0, glm::sin((f32)scene->getWorldTime()) * 0.2f)) *
            transform * glm::rotate(glm::mat4(1.f), (f32)scene->getWorldTime(), glm::vec3(0, 0, 1));
        LitSettings s;
        s.mesh = mesh;
        s.color = glm::vec3(0.95f, 0.47f, 0.02f);
        s.emit = s.color * 0.5f;
        s.worldTransform = t;
        rw->push(LitRenderable(s));
    }
};
