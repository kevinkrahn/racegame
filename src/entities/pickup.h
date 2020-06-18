#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../game.h"
#include "../scene.h"
#include "../vehicle.h"

namespace PickupType
{
    enum
    {
        MONEY = 0,
        FIXUP = 1,
    };
};

class Pickup : public PlaceableEntity
{
    u32 pickupType = PickupType::MONEY;
    static constexpr const char* pickupName[] = { "Money", "Fixup" };

public:
    void onCreate(Scene* scene) override
    {
        this->entityFlags |= Entity::TRANSIENT;

        updateTransform(scene);
        actor = g_game.physx.physics->createRigidStatic(
                PxTransform(convert(position), convert(rotation)));
        physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
        physicsUserData.entity = this;
        actor->userData = &physicsUserData;

        PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxSphereGeometry((scale * scaleOf(transform)).z), *g_game.physx.materials.generic);
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
                v->addBonus("$$$", PICKUP_BONUS_AMOUNT);
                g_audio.playSound3D(g_res.getSound("pickup_money"), SoundType::GAME_SFX, position);
            }
            else
            {
                v->fixup();
                g_audio.playSound3D(g_res.getSound("pickup_fixup"), SoundType::GAME_SFX, position);
            }
            this->destroy();
        }
    }

    Model* getModel()
    {
        static Model* money = g_res.getModel("money");
        static Model* wrench = g_res.getModel("wrench");
        return pickupType == PickupType::MONEY ? money : wrench;
    }

    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override
    {
        Mat4 t =
            glm::translate(Mat4(1.f), Vec3(0, 0, sinf((f32)scene->getWorldTime()) * 0.2f)) *
            transform * glm::rotate(Mat4(1.f), (f32)scene->getWorldTime(), Vec3(0, 0, 1));
        Model* model = getModel();
        for (auto& obj : model->objects)
        {
            g_res.getMaterial(obj.materialGuid)->draw(rw, t * obj.getTransform(),
                    &model->meshes[obj.meshIndex]);
        }
    }

    void onPreview(RenderWorld* rw) override
    {
        rw->setViewportCamera(0, Vec3(3.f, 3.f, 4.f) * 1.25f,
                Vec3(0.f, 0.f, 1.25f), 1.f, 200.f, 32.f);
        Model* model = getModel();
        for (auto& obj : model->objects)
        {
            g_res.getMaterial(obj.materialGuid)->draw(rw, obj.getTransform(),
                    &model->meshes[obj.meshIndex]);
        }
    }

    void serializeState(Serializer& s) override
    {
        PlaceableEntity::serializeState(s);
        s.field(pickupType);
    }

    Array<PropPrefabData> generatePrefabProps() override
    {
        return {
            { PropCategory::PICKUPS, "Money",
                [](Entity* e) { ((Pickup*)e)->pickupType = PickupType::MONEY; } },
            { PropCategory::PICKUPS, "Fixup",
                [](Entity* e) { ((Pickup*)e)->pickupType = PickupType::FIXUP; } },
        };
    }

    const char* getName() const override { return pickupName[pickupType]; }
};
