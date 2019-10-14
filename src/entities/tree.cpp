#include "tree.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

Tree::Tree(glm::vec3 const& position, glm::vec3 const& scale, f32 zRotation)
{
    this->position = position;
    this->scale = scale;
    this->rotation = glm::rotate(this->rotation, zRotation, glm::vec3(0, 0, 1));
}

void Tree::onCreate(Scene* scene)
{
    this->meshTrunk = g_resources.getMesh("tree.Trunk");
    this->meshLeaves = g_resources.getMesh("tree.Leaves");
    this->texTrunk = g_resources.getTexture("bark");
    this->texLeaves = g_resources.getTexture("leaves");
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(meshTrunk->getCollisionMesh(), PxMeshScale(convert(scale))), *scene->genericMaterial);
    collisionShape->setQueryFilterData(PxFilterData(
                COLLISION_FLAG_GROUND | COLLISION_FLAG_SELECTABLE, DECAL_GROUND, 0, DRIVABLE_SURFACE));
    collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND, -1, 0, 0));
    scene->getPhysicsScene()->addActor(*actor);
}

void Tree::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    LitSettings settings;
    settings.mesh = meshTrunk;
    settings.texture = texTrunk;
    settings.fresnelScale = 0.2f;
    settings.fresnelPower = 1.7f;
    settings.fresnelBias = -0.2f;
    settings.worldTransform = transform;
    rw->push(LitRenderable(settings));

    settings.mesh = meshLeaves;
    settings.texture = texLeaves;
    settings.fresnelScale = 0.f;
    settings.specularPower = 0.f;
    settings.specularStrength = 0.f;
    settings.minAlpha = 0.5f;
    //settings.transparent = true;
    rw->push(LitRenderable(settings));
}

void Tree::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
{
    if (isSelected)
    {
        rw->push(WireframeRenderable(meshTrunk, transform));
        rw->push(WireframeRenderable(meshLeaves, transform));
    }
}

DataFile::Value Tree::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["entityID"] = DataFile::makeInteger((i64)SerializedEntityID::TREE);
    dict["position"] = DataFile::makeVec3(position);
    dict["rotation"] = DataFile::makeVec4({ rotation.x, rotation.y, rotation.z, rotation.w });
    dict["scale"] = DataFile::makeVec3(scale);
    return dict;
}

void Tree::deserialize(DataFile::Value& data)
{
    position = data["position"].vec3();
    glm::vec4 r = data["rotation"].vec4();
    rotation = glm::quat(r.w, r.x, r.y, r.z);
    scale = data["scale"].vec3();
}
