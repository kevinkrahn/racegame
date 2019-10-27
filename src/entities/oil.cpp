#include "oil.h"
#include "../renderer.h"
#include "../scene.h"
#include "../game.h"
#include "../gui.h"
#include "../vehicle.h"

Oil* Oil::setup(glm::vec3 const& pos)
{
    position = pos;
    rotation = glm::rotate(rotation, (f32)M_PI * 0.5f, glm::vec3(0, 1, 0));
    scale = glm::vec3(6.f);
    return this;
}

void Oil::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.placeableEntity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxBoxGeometry(convert(scale * 0.5f)), *scene->genericMaterial);
    collisionShape->setQueryFilterData(
            PxFilterData(COLLISION_FLAG_SELECTABLE | COLLISION_FLAG_OIL, 0, 0, 0));
    collisionShape->setSimulationFilterData(PxFilterData(0, 0, 0, 0));
    scene->getPhysicsScene()->addActor(*actor);

    updateTransform(scene);
}

void Oil::updateTransform(Scene* scene)
{
    if (actor)
    {
        PlaceableEntity::updateTransform(scene);

        PxShape* shape = nullptr;
        actor->getShapes(&shape, 1);
        shape->setGeometry(PxBoxGeometry(convert(
                        glm::abs(glm::max(glm::vec3(0.01f), scale) * 0.5f))));
    }
    tex = g_resources.getTexture("oil");
    decal.setTexture(tex);
    decal.setPriority(9001);
    decal.begin(transform);
    scene->track->applyDecal(decal);
    decal.end();
}

void Oil::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    rw->add(&decal);
}

void Oil::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
{
    BoundingBox decalBoundingBox{ glm::vec3(-0.5f), glm::vec3(0.5f) };
    if (isSelected)
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, glm::vec4(1, 1, 1, 1));
    }
    else
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, glm::vec4(1, 0.5f, 0, 1));
    }
}
