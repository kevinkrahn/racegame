#include "booster.h"
#include "../renderer.h"
#include "../scene.h"
#include "../game.h"
#include "../gui.h"
#include "../vehicle.h"

Booster::Booster(glm::vec3 const& pos)
{
    position = pos;
    rotation = glm::rotate(rotation, (f32)M_PI * 0.5f, glm::vec3(0, 1, 0));
    scale = glm::vec3(8.f);
}

void Booster::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.placeableEntity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxBoxGeometry(convert(scale * 0.5f)), *scene->genericMaterial);
    collisionShape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, 0));
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
                        glm::abs(glm::max(glm::vec3(0.01f), scale) * 0.5f))));
    }
    tex = g_resources.getTexture("booster");
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
                active = true;
            }
        }
    }

    intensity = smoothMove(intensity, active ? 3.5f : 1.25f, 6.f, deltaTime);
}

void Booster::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    decal.setColor(backwards ? glm::vec3(intensity, 0.f, 0.f) : glm::vec3(0.f, intensity, 0.f));
    rw->add(&decal);
}

void Booster::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
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

DataFile::Value Booster::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["entityID"] = DataFile::makeInteger((i64)SerializedEntityID::BOOSTER);
    dict["position"] = DataFile::makeVec3(position);
    dict["rotation"] = DataFile::makeVec4({ rotation.x, rotation.y, rotation.z, rotation.w });
    dict["scale"] = DataFile::makeVec3(scale);
    dict["backwards"] = DataFile::makeBool(backwards);
    return dict;
}

void Booster::deserialize(DataFile::Value& data)
{
    position = data["position"].vec3();
    glm::vec4 r = data["rotation"].vec4();
    rotation = glm::quat(r.w, r.x, r.y, r.z);
    scale = data["scale"].vec3();
    backwards = data["backwards"].boolean();
}

void Booster::showDetails(Scene* scene)
{
    g_gui.toggle("Backwards", this->backwards);
}
