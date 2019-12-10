#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../mesh_renderables.h"
#include "../scene.h"
#include "../game.h"

class WaterBarrel : public PlaceableEntity
{
    LitRenderable lit = LitRenderable({});

public:
    WaterBarrel* setup(glm::vec3 const& position = {0, 0, 0}, f32 zRotation=0.f)
    {
        this->position = position;
        this->rotation = glm::rotate(this->rotation, zRotation, glm::vec3(0, 0, 1));
        return this;
    }

    void applyDecal(class Decal& decal) override
    {
        decal.addMesh(lit.settings.mesh, transform);
    }

    void onCreate(class Scene* scene) override
    {
        entityFlags |= DYNAMIC;

        updateTransform(scene);

        PxRigidDynamic* body = g_game.physx.physics->createRigidDynamic(
                PxTransform(convert(position), convert(rotation)));
        physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
        physicsUserData.entity = this;
        body->userData = &physicsUserData;
        PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*body,
                PxConvexMeshGeometry(g_res.getMesh("barrel.Collision")->getConvexCollisionMesh(), PxMeshScale(convert(scale))),
                *scene->genericMaterial);
        collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_DYNAMIC | COLLISION_FLAG_SELECTABLE, DECAL_NONE, 0, UNDRIVABLE_SURFACE));
        collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_DYNAMIC, -1, 0, 0));
        PxRigidBodyExt::updateMassAndInertia(*body, 150.f);
        actor = body;
        scene->getPhysicsScene()->addActor(*actor);

        lit.settings.mesh = g_res.getMesh("barrel.Barrel");
        lit.settings.color = { 1, 1, 1 };
        lit.settings.texture = &g_res.textures->white;
        lit.settings.fresnelScale = 0.2f;
        lit.settings.fresnelPower = 1.7f;
        lit.settings.fresnelBias = -0.2f;
        lit.settings.specularStrength = 0.1f;
        lit.settings.texture = &g_res.textures->white;
        lit.settings.worldTransform = transform;
    }

    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override
    {
        lit.settings.worldTransform = convert(actor->getGlobalPose());
        transform = lit.settings.worldTransform;
        rw->add(&lit);
    }

    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override
    {
        if (isSelected)
        {
            rw->push(WireframeRenderable(lit.settings.mesh, transform));
        }
    }
};
