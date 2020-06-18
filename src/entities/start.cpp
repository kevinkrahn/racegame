#include "start.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
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
                    PxMeshScale(convert(scale))), *g_game.physx.materials.generic);
        collisionShape->setQueryFilterData(
                PxFilterData(0, DECAL_SIGN, 0, DRIVABLE_SURFACE));
        collisionShape->setSimulationFilterData(
                PxFilterData(COLLISION_FLAG_OBJECT, -1, 0, 0));
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
    Mat4 decalTransform = transform *
        Mat4::scaling(Vec3(size * 0.0625f, size, 30.f)) * Mat4::rotationY(PI * 0.5);
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
            Vec3 lightStack1[] = {
                { 2.6f, 2.564f, 9.284f },
                { 2.6f, 2.564f, 8.000f },
                { 2.6f, 2.564f, 6.738f },
            };
            Vec3 lightStack2[] = {
                { 2.6f, 4.316f, 9.284f },
                { 2.6f, 4.316f, 8.000f },
                { 2.6f, 4.316f, 6.738f },
            };

            Vec3 p1 = lightStack1[countIndex];
            Vec3 p2 = lightStack2[countIndex];
            Vec3 p3 = Vec3(-p1.x, p1.y, p1.z);
            Vec3 p4 = Vec3(-p2.x, p2.y, p2.z);
            Vec3 p5 = Vec3(p1.x, -p1.y, p1.z);
            Vec3 p6 = Vec3(p2.x, -p2.y, p2.z);
            Vec3 p7 = Vec3(-p1.x, -p1.y, p1.z);
            Vec3 p8 = Vec3(-p2.x, -p2.y, p2.z);
            Vec3 positions[] = { p1, p2, p3, p4, p5, p6, p7, p8 };

            Texture* flare = g_res.getTexture("flare");
            Vec4 col = countIndex == 2
                ? Vec4(0.01f, 1.f, 0.01f, 0.6f) : Vec4(1.f, 0.01f, 0.01f, 0.6f);

            for (auto& v : positions)
            {
                drawBillboard(rw, flare, Vec3(transform * Vec4(v, 1.f)),
                            col, countIndex == 2 ? 1.3f : 1.f, 0.f, false);
            }

            Vec3 color = (countIndex == 2 ?
                    Vec3(0.1f, 1.f, 0.1f) : Vec3(1.f, 0.1f, 0.1f)) * (f32)(countIndex + 1);
            rw->addPointLight(position + Vec3(0, 0, 8.f - countIndex * 0.1f), color, 20.f, 2.f);
        }
    }

    for (auto& obj : model->objects)
    {
        g_res.getMaterial(obj.materialGuid)->draw(rw, transform * obj.getTransform(),
                &model->meshes[obj.meshIndex]);
    }

    finishLineDecal.draw(rw);
}

void Start::onPreview(RenderWorld* rw)
{
    rw->setViewportCamera(0, Vec3(3.f, 3.f, 3.5f) * 6.f,
            Vec3(0.f, 0.f, 2.f), 1.f, 200.f, 50.f);
    for (auto& obj : model->objects)
    {
        g_res.getMaterial(obj.materialGuid)->draw(rw, transform * obj.getTransform(),
                &model->meshes[obj.meshIndex]);
    }
}

void Start::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected, u8 selectIndex)
{
    for (auto& obj : model->objects)
    {
        Material* mat = g_res.getMaterial(obj.materialGuid);
        Mat4 t = transform * obj.getTransform();
        Mesh* mesh = &model->meshes[obj.meshIndex];
        if (isSelected)
        {
            mat->drawHighlight(rw, t, mesh, selectIndex);
        }
        mat->drawPick(rw, t, mesh, entityCounterID);
    }
}
