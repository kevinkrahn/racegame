#include "booster.h"
#include "../renderer.h"
#include "../scene.h"
#include "../game.h"
#include "../vehicle.h"
#include "../billboard.h"
#include "../imgui.h"

Booster::Booster()
{
    rotation = glm::rotate(rotation, (f32)M_PI * 0.5f, Vec3(0, 1, 0));
    scale = Vec3(8.f);
}

void Booster::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.placeableEntity = this;
    physicsUserData.flags = ActorUserData::BOOSTER;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxBoxGeometry(convert(scale * 0.5f)), *g_game.physx.materials.generic);
    collisionShape->setQueryFilterData(
            PxFilterData(COLLISION_FLAG_SELECTABLE | COLLISION_FLAG_BOOSTER, 0, 0, 0));
    collisionShape->setSimulationFilterData(PxFilterData(0, 0, 0, 0));
    scene->getPhysicsScene()->addActor(*actor);

    updateTransform(scene);
}

void Booster::updateTransform(Scene* scene)
{
    if (actor)
    {
        PlaceableEntity::updateTransform(scene);

        PxShape* shape = nullptr;
        actor->getShapes(&shape, 1);
        shape->setGeometry(PxBoxGeometry(convert(
                        absolute(max(Vec3(0.01f), scale) * 0.5f))));
    }
    tex = g_res.getTexture("booster");
    decal.setTexture(tex);
    decal.begin(transform);
    scene->track->applyDecal(decal);
    decal.end();
}

void Booster::onUpdate(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    active = false;

    PxOverlapHit hitBuffer[8];
    PxOverlapBuffer hit(hitBuffer, ARRAY_SIZE(hitBuffer));
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eDYNAMIC;
    filter.data = PxFilterData(COLLISION_FLAG_CHASSIS, 0, 0, 0);
    f32 radius = 2.5f;
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
                vehicle->getRigidBody()->addForce(convert(yAxisOf(transform)) * 15.f,
                        PxForceMode::eACCELERATION);
                vehicle->setMotionBlur(1.f, 1.5f);
                active = true;
            }
        }
    }

    intensity = smoothMove(intensity, active ? 3.5f : 1.25f, 6.f, deltaTime);
}

void Booster::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    decal.setColor(backwards ? Vec4(intensity, 0.f, 0.f, 1.f) : Vec4(0.f, intensity, 0.f, 1.f));
    decal.draw(rw);
}

void Booster::onPreview(RenderWorld* rw)
{
    rw->setViewportCamera(0, Vec3(0.f, 0.1f, 20.f), Vec3(0.f), 1.f, 200.f, 50.f);
    Vec3 color = backwards ? Vec3(intensity, 0.f, 0.f) : Vec3(0.f, intensity, 0.f);
    drawBillboard(rw, g_res.getTexture("booster"), Vec3(0, 0, 2.f),
                Vec4(color, 1.f), 8.f, 0.f, false);
}

void Booster::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected, u8 selectIndex)
{
    BoundingBox decalBoundingBox{ Vec3(-0.5f), Vec3(0.5f) };
    if (isSelected)
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, Vec4(1, 1, 1, 1));
    }
    else
    {
        scene->debugDraw.boundingBox(decalBoundingBox, transform, Vec4(1, 0.5f, 0, 1));
    }
}

void Booster::showDetails(Scene* scene)
{
    ImGui::Checkbox("Backwards", &this->backwards);
}
