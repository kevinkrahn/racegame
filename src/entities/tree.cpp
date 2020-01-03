#include "tree.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

Tree::Tree()
{
    this->meshTrunk = g_res.getMesh("tree.Trunk");
    this->meshLeaves = g_res.getMesh("tree.Leaves");
    this->texTrunk = &g_res.textures->bark;
    this->texLeaves = &g_res.textures->leaves;
}

void Tree::onCreate(Scene* scene)
{
    updateTransform(scene);

    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(meshTrunk->getCollisionMesh(), PxMeshScale(convert(scale))), *scene->genericMaterial);
    collisionShape->setQueryFilterData(PxFilterData(
                COLLISION_FLAG_OBJECT | COLLISION_FLAG_SELECTABLE, DECAL_GROUND, 0, DRIVABLE_SURFACE));
    collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_OBJECT, -1, 0, 0));
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

void Tree::onPreview(RenderWorld* rw)
{
    rw->setViewportCamera(0, glm::vec3(3.f, 3.f, 3.f) * 10.f,
            glm::vec3(0, 0, 8), 1.f, 200.f, 32.f);
    transform = glm::mat4(1.f);
    onRender(rw, nullptr, 0.f);
}

void Tree::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
{
    if (isSelected)
    {
        rw->push(WireframeRenderable(meshTrunk, transform));
        rw->push(WireframeRenderable(meshLeaves, transform));
    }
}
