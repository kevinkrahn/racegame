#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../game.h"
#include "../scene.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"

enum struct PickupType : u32
{
    MONEY,
    ARMOR,
};

class Pickup : public PlaceableEntity
{
    Mesh* mesh;
    PickupType pickupType = PickupType::MONEY;

public:
    Pickup* setup(glm::vec3 const& position, PickupType pickupType)
    {
        this->position = position;
        this->pickupType = pickupType;
        return this;
    }

    void onCreate(Scene* scene) override
    {
        this->entityFlags |= Entity::TRANSIENT;
        if (pickupType == PickupType::MONEY)
        {
            mesh = g_res.getMesh("money.Money");
        }
        else
        {
            mesh = g_res.getMesh("wrench.Wrench");
        }

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
        collisionShape->setLocalPose(PxTransform(PxVec3(0, 0, 1.4f)));

        scene->getPhysicsScene()->addActor(*actor);
    }

    void onTrigger(ActorUserData* userData) override
    {
        if (userData->entityType == ActorUserData::VEHICLE
                // prevents multiple vehicles from collecting the same pickup on the same frame
                && !this->isDestroyed())
        {
            Vehicle* v = userData->vehicle;
            if (pickupType == PickupType::MONEY)
            {
                v->addPickupBonus();
                g_audio.playSound3D(&g_res.sounds->money, SoundType::GAME_SFX, position);
            }
            else
            {
                v->fixup();
                g_audio.playSound3D(&g_res.sounds->fixup, SoundType::GAME_SFX, position);
            }
            this->destroy();
        }
    }

    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override
    {
        glm::mat4 t =
            glm::translate(glm::mat4(1.f), glm::vec3(0, 0, glm::sin((f32)scene->getWorldTime()) * 0.2f)) *
            transform * glm::rotate(glm::mat4(1.f), (f32)scene->getWorldTime(), glm::vec3(0, 0, 1));
        LitSettings s;
        s.mesh = mesh;
        if (pickupType == PickupType::MONEY)
        {
            s.color = glm::vec3(0.95f, 0.47f, 0.02f);
            s.emit = s.color * 0.5f;
        }
        else
        {
            s.color = glm::vec3(0.2f);
            s.emit = glm::vec3(0.125f);
            s.reflectionStrength = 0.9f;
            s.reflectionBias = 0.8f;
        }
        s.worldTransform = t;
        rw->push(LitRenderable(s));
    }

    DataFile::Value serializeState() override
    {
        DataFile::Value dict = PlaceableEntity::serializeState();
        dict["pickupType"] = DataFile::makeInteger((i64)pickupType);
        return dict;
    }

    void deserializeState(DataFile::Value& data) override
    {
        PlaceableEntity::deserializeState(data);
        pickupType = (PickupType)data["pickupType"].integer(0);
    }
};
