#include "start.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"
#include "../billboard.h"

void Start::onCreate(Scene* scene)
{
    if (!scene->start)
    {
        scene->start = this;
    }
}

void Start::onCreateEnd(Scene* scene)
{
    actor = g_game.physx.physics->createRigidStatic(PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    for (auto& obj : model->objects)
    {
        PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
                PxTriangleMeshGeometry(model->meshes[obj.meshIndex].getCollisionMesh(),
                    PxMeshScale(convert(scale))), *scene->genericMaterial);
        collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_SELECTABLE, DECAL_SIGN, 0, DRIVABLE_SURFACE));
        collisionShape->setSimulationFilterData(PxFilterData(
                    COLLISION_FLAG_OBJECT, -1, 0, 0));
        collisionShape->setLocalPose(PxTransform(convert(obj.position), convert(obj.rotation)));
    }
    scene->getPhysicsScene()->addActor(*actor);
    finishLineDecal.setTexture(g_res.getTexture("checkers"));
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
    if (scene->isRaceInProgress && !g_game.isEditing)
    {
        if (scene->getWorldTime() > 3.f)
        {
            if (countIndex != 2)
            {
                g_audio.playSound(g_res.getSound("countdown_b"),
                        SoundType::GAME_SFX, false, 1.f, 0.5f);
            }
            countIndex = 2;
        }
        else if (scene->getWorldTime() > 2.f)
        {
            if (countIndex != 1)
            {
                g_audio.playSound(g_res.getSound("countdown_a"),
                        SoundType::GAME_SFX, false, 1.f, 0.5f);
            }
            countIndex = 1;
        }
        else if (scene->getWorldTime() > 1.f)
        {
            if (countIndex != 0)
            {
                g_audio.playSound(g_res.getSound("countdown_a"),
                        SoundType::GAME_SFX, false, 1.f, 0.5f);
            }
            countIndex = 0;
        }

        if (countIndex >= 0)
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

            Texture* flare = g_res.getTexture("flare");
            glm::vec4 col = countIndex == 2
                ? glm::vec4(0.01f, 1.f, 0.01f, 0.6f) : glm::vec4(1.f, 0.01f, 0.01f, 0.6f);

            for (auto& v : positions)
            {
                rw->push(BillboardRenderable(flare, transform * glm::vec4(v, 1.f),
                            col, countIndex == 2 ? 1.3f : 1.f, 0.f, false));
            }
        }
    }

    for (auto& obj : model->objects)
    {
        rw->push(LitMaterialRenderable(&model->meshes[obj.meshIndex], transform * obj.getTransform(),
                    g_res.getMaterial(obj.materialGuid)));
    }

    rw->add(&finishLineDecal);
}

void Start::onPreview(RenderWorld* rw)
{
    rw->setViewportCamera(0, glm::vec3(3.f, 3.f, 3.5f) * 6.f,
            glm::vec3(0.f, 0.f, 2.f), 1.f, 200.f, 50.f);
    for (auto& obj : model->objects)
    {
        rw->push(LitMaterialRenderable(&model->meshes[obj.meshIndex], transform * obj.getTransform(),
                    g_res.getMaterial(obj.materialGuid)));
    }
}

void Start::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected, u8 selectIndex)
{
    if (isSelected)
    {
        for (auto& obj : model->objects)
        {
            rw->push(WireframeRenderable(&model->meshes[obj.meshIndex], transform * obj.getTransform()));
        }
        rw->push(LitRenderable(g_res.getModel("misc")->getMeshByName("world.Arrow"),
                transform *
                glm::translate(glm::mat4(1.f), {10, 0, -2}) *
                glm::scale(glm::mat4(1.f), glm::vec3(3.f))));
    }
}
