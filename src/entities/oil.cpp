#include "oil.h"
#include "../renderer.h"
#include "../scene.h"
#include "../game.h"
#include "../vehicle.h"
#include "../billboard.h"

Oil* Oil::setup(glm::vec3 const& pos)
{
    position = pos;
    rotation = glm::rotate(rotation, PI * 0.5f, glm::vec3(0, 1, 0));
    scale = glm::vec3(6.f);
    return this;
}

void Oil::onCreateEnd(Scene* scene)
{
    rotation = glm::rotate(rotation,
            random(scene->randomSeries, 0.f, PI * 2.f), glm::vec3(1, 0, 0));

    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.placeableEntity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxBoxGeometry(convert(scale * 0.5f)), *g_game.physx.materials.generic);
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
    decal.setTexture(g_res.getTexture("icon_oil"), g_res.getTexture("oil_normal"));
    decal.setPriority(TransparentDepth::OIL_GLUE);
    decal.begin(transform);
    scene->track->applyDecal(decal);
    decal.end();
}

void Oil::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    decal.draw(rw);
}

void Oil::onPreview(RenderWorld* rw)
{
    rw->setViewportCamera(0, glm::vec3(0.f, 0.1f, 20.f), glm::vec3(0.f), 1.f, 200.f, 50.f);
    drawBillboard(rw, g_res.getTexture("icon_oil"), glm::vec3(0, 0, 2.f),
                glm::vec4(1.f), 8.f, 0.f, false);
}

void Oil::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected, u8 selectIndex)
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
