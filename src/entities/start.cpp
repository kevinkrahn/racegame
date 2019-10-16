#include "start.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"
#include "../billboard.h"

void Start::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(mesh->getCollisionMesh(), PxMeshScale(convert(scale))), *scene->genericMaterial);
    collisionShape->setQueryFilterData(PxFilterData(
                COLLISION_FLAG_SELECTABLE, DECAL_SIGN, 0, DRIVABLE_SURFACE));
    collisionShape->setSimulationFilterData(PxFilterData(
                COLLISION_FLAG_GROUND, -1, 0, 0));
    scene->getPhysicsScene()->addActor(*actor);
    finishLineDecal.setTexture(g_resources.getTexture("checkers"));
    updateTransform(scene);
}

void Start::updateTransform(Scene* scene)
{
    PlaceableEntity::updateTransform(scene);
    f32 size = 21.f;
    glm::mat4 decalTransform = transform *
        glm::scale(glm::mat4(1.f), { size * 0.0625f, size, 30.f }) *
        glm::rotate(glm::mat4(1.f), f32(M_PI * 0.5), { 0, 1, 0 });
    finishLineDecal.begin(decalTransform);
    scene->track->applyDecal(finishLineDecal);
    finishLineDecal.end();
}

void Start::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    i32 countIndex = -1;

    if (scene->getWorldTime() > 1.f)
    {
        countIndex = 0;
    }
    if (scene->getWorldTime() > 2.f)
    {
        countIndex = 1;
    }
    if (scene->getWorldTime() > 3.f)
    {
        countIndex = 2;
    }

    if (countIndex >= 0 && scene->isRaceInProgress)
    {
        glm::vec3 lightStack1[] = {
            { 2.6f, 2.564f, 9.284f },
            { 2.6f, 2.564f, 8.000f },
            { 2.6f, 2.564f, 6.738f },
        };
        glm::vec3 lightStack2[] = {
            { 2.6f, 4.316f, 9.284f },
            { 2.6f, 4.316f, 8.000f },
            { 2.6f, 4.316f, 6.738f },
        };

        glm::vec3 p1 = lightStack1[countIndex];
        glm::vec3 p2 = lightStack2[countIndex];
        glm::vec3 p3 = glm::vec3(-p1.x, p1.y, p1.z);
        glm::vec3 p4 = glm::vec3(-p2.x, p2.y, p2.z);
        glm::vec3 p5 = glm::vec3(p1.x, -p1.y, p1.z);
        glm::vec3 p6 = glm::vec3(p2.x, -p2.y, p2.z);
        glm::vec3 p7 = glm::vec3(-p1.x, -p1.y, p1.z);
        glm::vec3 p8 = glm::vec3(-p2.x, -p2.y, p2.z);
        glm::vec3 positions[] = { p1, p2, p3, p4, p5, p6, p7, p8 };

        Texture* flare = g_resources.getTexture("flare");
        glm::vec4 col = countIndex == 2
            ? glm::vec4(0.01f, 1.f, 0.01f, 0.6f) : glm::vec4(1.f, 0.01f, 0.01f, 0.6f);

        for (auto& v : positions)
        {
            rw->push(BillboardRenderable(flare, transform * glm::vec4(v, 1.f),
                        col, countIndex == 2 ? 1.3f : 1.f));
        }
    }

    LitSettings settings;
    settings.mesh = mesh;
    settings.fresnelScale = 0.3f;
    settings.fresnelPower = 1.5f;
    settings.fresnelBias = -0.15f;
    settings.specularPower = 60.f;
    settings.specularStrength = 0.3f;
    settings.texture = g_resources.getTexture("white");
    settings.worldTransform = transform;
    rw->push(LitRenderable(settings));
    settings.mesh = meshLights;
    rw->push(LitRenderable(settings));
    rw->add(&finishLineDecal);
}

void Start::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
{
    if (isSelected)
    {
        rw->push(WireframeRenderable(mesh, transform));
        rw->push(LitRenderable(g_resources.getMesh("world.Arrow"),
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
