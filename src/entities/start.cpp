#include "start.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

void Start::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(mesh->getCollisionMesh(), PxMeshScale(convert(scale))), *scene->offroadMaterial);
    collisionShape->setQueryFilterData(PxFilterData(
                COLLISION_FLAG_GROUND | COLLISION_FLAG_SELECTABLE, DECAL_SIGN, 0, DRIVABLE_SURFACE));
    collisionShape->setSimulationFilterData(PxFilterData(
                COLLISION_FLAG_GROUND, -1, 0, 0));
    scene->getPhysicsScene()->addActor(*actor);
    finishLineDecal.setTexture(g_resources.getTexture("checkers"));
    updateTransform(scene);
}

void Start::updateTransform(Scene* scene)
{
    PlaceableEntity::updateTransform(scene);
    glm::mat4 decalTransform = transform *
        glm::scale(glm::mat4(1.f), { 22.f * 0.0625f, 22.f, 30.f }) *
        glm::rotate(glm::mat4(1.f), f32(M_PI * 0.5), { 0, 1, 0 });
    finishLineDecal.begin(decalTransform);
    scene->track->applyDecal(finishLineDecal);
    finishLineDecal.end();
}

void Start::onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime)
{
    LitSettings settings;
    settings.mesh = mesh;
    settings.fresnelScale = 0.3f;
    settings.fresnelPower = 1.5f;
    settings.fresnelBias = -0.2f;
    settings.texture = g_resources.getTexture("white");
    settings.worldTransform = transform;
    renderer->push(LitRenderable(settings));
    renderer->add(&finishLineDecal);
}

void Start::onEditModeRender(Renderer* renderer, Scene* scene, bool isSelected)
{
    if (isSelected)
    {
        renderer->push(WireframeRenderable(mesh, transform));
        renderer->push(LitRenderable(g_resources.getMesh("world.Arrow"),
                transform *
                glm::translate(glm::mat4(1.f), {10, 0, -2}) *
                glm::scale(glm::mat4(1.f), glm::vec3(3.f))));
    }
}

DataFile::Value Start::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["entityID"] = DataFile::makeInteger((i64)SerializedEntityID::START);
    dict["position"] = DataFile::makeVec3(position);
    dict["rotation"] = DataFile::makeVec4({ rotation.x, rotation.y, rotation.z, rotation.w });
    dict["scale"] = DataFile::makeVec3(scale);
    return dict;
}

void Start::deserialize(DataFile::Value& data)
{
    position = data["position"].vec3();
    glm::vec4 r = data["rotation"].vec4();
    rotation = glm::quat(r.w, r.x, r.y, r.z);
    scale = data["scale"].vec3();
}

void Start::applyDecal(Decal& decal)
{
    decal.addMesh(mesh, transform);
}
