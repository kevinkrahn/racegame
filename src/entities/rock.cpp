#include "rock.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

void Rock::onCreate(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(mesh->getCollisionMesh(), PxMeshScale(convert(scale))), *scene->offroadMaterial);
    collisionShape->setQueryFilterData(PxFilterData(
                COLLISION_FLAG_GROUND | COLLISION_FLAG_SELECTABLE, DECAL_GROUND, 0, DRIVABLE_SURFACE));
    collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND, -1, 0, 0));
    scene->getPhysicsScene()->addActor(*actor);
}

void Rock::onUpdate(Renderer* renderer, Scene* scene, f32 deltaTime)
{
    LitSettings settings;
    settings.mesh = mesh;
    settings.fresnelScale = 0.3f;
    settings.fresnelPower = 1.5f;
    settings.fresnelBias = -0.2f;
    settings.texture = tex;
    settings.worldTransform = transform;
    renderer->push(LitRenderable(settings));
}

void Rock::onEditModeRender(Renderer* renderer, Scene* scene, bool isSelected)
{
    if (isSelected)
    {
        renderer->push(WireframeRenderable(mesh, transform));
    }
}

DataFile::Value Rock::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["entityID"] = DataFile::makeInteger((i64)SerializedEntityID::ROCK);
    dict["position"] = DataFile::makeVec3(position);
    dict["rotation"] = DataFile::makeVec4({ rotation.x, rotation.y, rotation.z, rotation.w });
    dict["scale"] = DataFile::makeVec3(scale);
    return dict;
}

void Rock::deserialize(DataFile::Value& data)
{
    position = data["position"].vec3();
    glm::vec4 r = data["rotation"].vec4();
    rotation = glm::quat(r.w, r.x, r.y, r.z);
    scale = data["scale"].vec3();
}

void Rock::applyDecal(Decal& decal)
{
    decal.addMesh(mesh, transform);
}
