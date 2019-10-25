#include "static_mesh.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

const char* meshNames[] = {
    "world.Rock",
    "world.Tunnel",
    "world.Sign",
    "cactus.cactus",
    "world.Cube",
};

const char* texNames[] = {
    "rock",
    "concrete",
    "white",
    "cactus",
    "concrete",
};

StaticMesh::StaticMesh(u32 meshIndex, glm::vec3 const& position, glm::vec3 const& scale, f32 zRotation)
{
    this->position = position;
    this->scale = scale;
    this->rotation = glm::rotate(this->rotation, zRotation, glm::vec3(0, 0, 1));
    this->meshIndex = meshIndex;
    mesh = g_resources.getMesh(meshNames[meshIndex]);
    tex = g_resources.getTexture(texNames[meshIndex]);
}

void StaticMesh::onCreate(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(mesh->getCollisionMesh(), PxMeshScale(convert(scale))), *scene->genericMaterial);
    collisionShape->setQueryFilterData(PxFilterData(
                COLLISION_FLAG_GROUND | COLLISION_FLAG_SELECTABLE, DECAL_GROUND, 0, DRIVABLE_SURFACE));
    collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND, -1, 0, 0));
    scene->getPhysicsScene()->addActor(*actor);
}

void StaticMesh::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    LitSettings settings;
    settings.mesh = mesh;
    settings.fresnelScale = 0.2f;
    settings.fresnelPower = 1.7f;
    settings.fresnelBias = -0.2f;
    settings.texture = tex;
    settings.worldTransform = transform;
    rw->push(LitRenderable(settings));
}

void StaticMesh::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
{
    if (isSelected)
    {
        rw->push(WireframeRenderable(mesh, transform));
    }
}

DataFile::Value StaticMesh::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["entityID"] = DataFile::makeInteger((i64)SerializedEntityID::STATIC_MESH);
    dict["position"] = DataFile::makeVec3(position);
    dict["rotation"] = DataFile::makeVec4({ rotation.x, rotation.y, rotation.z, rotation.w });
    dict["scale"] = DataFile::makeVec3(scale);
    dict["meshIndex"] = DataFile::makeInteger(meshIndex);
    return dict;
}

void StaticMesh::deserialize(DataFile::Value& data)
{
    position = data["position"].vec3();
    glm::vec4 r = data["rotation"].vec4();
    rotation = glm::quat(r.w, r.x, r.y, r.z);
    scale = data["scale"].vec3();
    meshIndex = (u32)data["meshIndex"].integer(0);

    mesh = g_resources.getMesh(meshNames[meshIndex]);
    tex = g_resources.getTexture(texNames[meshIndex]);
}

void StaticMesh::applyDecal(Decal& decal)
{
    decal.addMesh(mesh, transform);
}
