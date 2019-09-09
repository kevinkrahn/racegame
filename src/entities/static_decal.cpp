#include "static_decal.h"
#include "../renderer.h"
#include "../scene.h"
#include "../game.h"

StaticDecal::StaticDecal(glm::vec3 const& pos)
{
    position = pos;
    scale = glm::vec3(16.f);
    rotation = glm::rotate(rotation, (f32)M_PI * 0.5f, glm::vec3(0, 1, 0));
    tex = g_resources.getTexture("thing");
}

void StaticDecal::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxBoxGeometry(convert(scale * 0.5f)), *scene->offroadMaterial);
    collisionShape->setQueryFilterData(PxFilterData(
                COLLISION_FLAG_SELECTABLE, 0, 0, 0));
    collisionShape->setSimulationFilterData(PxFilterData(0, 0, 0, 0));
    scene->getPhysicsScene()->addActor(*actor);

    updateTransform(scene);
}

void StaticDecal::updateTransform(Scene* scene)
{
    PlaceableEntity::updateTransform(scene);
    decal.begin(transform);
    scene->track->addTrackMeshesToDecal(decal);
    decal.end();
    if (actor)
    {
        PxShape* shape = nullptr;
        actor->getShapes(&shape, 1);
        shape->setGeometry(PxBoxGeometry(convert(
                        glm::abs(glm::max(glm::vec3(0.01f), scale) * 0.5f))));
    }
}

void StaticDecal::onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime)
{
    decal.setTexture(tex);
    renderer->add(&decal);
}

void StaticDecal::onEditModeRender(Renderer* renderer, Scene* scene, bool isSelected)
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

DataFile::Value StaticDecal::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["entityID"] = DataFile::makeInteger((i64)SerializedEntityID::STATIC_DECAL);
    dict["position"] = DataFile::makeVec3(position);
    dict["rotation"] = DataFile::makeVec4({ rotation.x, rotation.y, rotation.z, rotation.w });
    dict["scale"] = DataFile::makeVec3(scale);
    return dict;
}

void StaticDecal::deserialize(DataFile::Value& data)
{
    position = data["position"].vec3();
    glm::vec4 r = data["rotation"].vec4();
    rotation = glm::quat(r.w, r.x, r.y, r.z);
    scale = data["scale"].vec3();
}
