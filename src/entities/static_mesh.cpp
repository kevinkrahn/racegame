#include "static_mesh.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

const char* meshNames[] = {
    "rock.rock",
    "world.Tunnel",
    "world.Sign",
    "cactus.cactus",
    "world.Cube",
    "plants.Plant1",
    "plants.Plant2",
};

StaticMesh* StaticMesh::setup(u32 meshIndex, glm::vec3 const& position, glm::vec3 const& scale, f32 zRotation)
{
    this->position = position;
    this->scale = scale;
    this->rotation = glm::rotate(this->rotation, zRotation, glm::vec3(0, 0, 1));
    this->meshIndex = meshIndex;
    return this;
}

void StaticMesh::onCreate(Scene* scene)
{
    updateTransform(scene);

    mesh = g_res.getMesh(meshNames[meshIndex]);

    Texture* textures[] = {
        &g_res.textures->rock,
        &g_res.textures->concrete,
        &g_res.textures->white,
        &g_res.textures->cactus,
        &g_res.textures->concrete,
        &g_res.textures->plant1,
        &g_res.textures->plant2,
    };
    tex = textures[meshIndex];

    actor = g_game.physx.physics->createRigidStatic(
            PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(mesh->getCollisionMesh(), PxMeshScale(convert(scale))), *scene->genericMaterial);
    if (meshIndex == 5 || meshIndex == 6)
    {
        collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_SELECTABLE, 0, 0, UNDRIVABLE_SURFACE));
    }
    else
    {
        collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_GROUND | COLLISION_FLAG_SELECTABLE, DECAL_GROUND, 0, DRIVABLE_SURFACE));
    }
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
    settings.specularStrength = 0.1f;
    settings.texture = tex;
    settings.worldTransform = transform;
    if (meshIndex == 5 || meshIndex == 6)
    {
        settings.minAlpha = 0.5f;
        //settings.culling = false;
    }
    rw->push(LitRenderable(settings));
}

void StaticMesh::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
{
    if (isSelected)
    {
        rw->push(WireframeRenderable(mesh, transform));
    }
}

DataFile::Value StaticMesh::serializeState()
{
    DataFile::Value dict = PlaceableEntity::serializeState();
    dict["meshIndex"] = DataFile::makeInteger(meshIndex);
    return dict;
}

void StaticMesh::deserializeState(DataFile::Value& data)
{
    PlaceableEntity::deserializeState(data);
    meshIndex = (u32)data["meshIndex"].integer(0);
}

void StaticMesh::applyDecal(Decal& decal)
{
    decal.addMesh(mesh, transform);
}
