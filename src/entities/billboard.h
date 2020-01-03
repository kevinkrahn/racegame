#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../mesh_renderables.h"
#include "../scene.h"
#include "../game.h"
#include "../gui.h"

class Billboard : public PlaceableEntity
{
    LitRenderable glow;
    LitRenderable lights;
    LitRenderable metal;
    LitRenderable sign;
    LitRenderable wood;
    i32 billboardIndex = 0;

public:
    Billboard()
    {
        glow.settings.mesh = g_res.getMesh("billboard.BillboardGlow");
        glow.settings.color = glm::vec3(1.f);
        glow.settings.emit = glm::vec3(3.f);
        glow.settings.specularStrength = 0.1f;
        glow.settings.texture = &g_res.textures->white;

        lights.settings.mesh = g_res.getMesh("billboard.BillboardLights");
        lights.settings.color = glm::vec3(0.05f);
        lights.settings.specularStrength = 0.2f;
        lights.settings.specularPower = 500.f;
        lights.settings.reflectionLod = 3.f;
        lights.settings.reflectionStrength = 0.8f;
        lights.settings.texture = &g_res.textures->white;

        metal.settings.mesh = g_res.getMesh("billboard.BillboardMetal");
        metal.settings.color = glm::vec3(0.05f);
        metal.settings.specularStrength = 0.2f;
        metal.settings.specularPower = 1000.f;
        metal.settings.reflectionLod = 2.f;
        metal.settings.reflectionStrength = 0.9f;
        metal.settings.texture = &g_res.textures->white;

        sign.settings.mesh = g_res.getMesh("billboard.BillboardSign");
        sign.settings.color = glm::vec3(1.f);

        wood.settings.mesh = g_res.getMesh("billboard.BillboardWood");
        wood.settings.color = glm::vec3(1.f);
        wood.settings.specularStrength = 0.05f;
        wood.settings.specularPower = 10.f;
        wood.settings.texture = &g_res.textures->wood;
    }

    void applyDecal(class Decal& decal) override
    {
        decal.addMesh(lights.settings.mesh, transform);
        decal.addMesh(metal.settings.mesh, transform);
        decal.addMesh(sign.settings.mesh, transform);
        decal.addMesh(sign.settings.mesh, transform);
        decal.addMesh(wood.settings.mesh, transform);
    }

    void onCreate(class Scene* scene) override
    {
        updateTransform(scene);

        actor = g_game.physx.physics->createRigidStatic(
                PxTransform(convert(position), convert(rotation)));
        physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
        physicsUserData.entity = this;
        actor->userData = &physicsUserData;

        PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
                PxTriangleMeshGeometry(g_res.getMesh("billboard.BillboardMetal")->getCollisionMesh(), PxMeshScale(convert(scale))),
                *scene->genericMaterial);
        collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_OBJECT | COLLISION_FLAG_SELECTABLE, DECAL_SIGN, 0, DRIVABLE_SURFACE));
        collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_OBJECT, -1, 0, 0));

        collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
                PxTriangleMeshGeometry(g_res.getMesh("billboard.BillboardWood")->getCollisionMesh(), PxMeshScale(convert(scale))),
                *scene->genericMaterial);
        collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_OBJECT | COLLISION_FLAG_SELECTABLE, DECAL_SIGN, 0, DRIVABLE_SURFACE));
        collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_OBJECT, -1, 0, 0));

        collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
                PxTriangleMeshGeometry(g_res.getMesh("billboard.BillboardSign")->getCollisionMesh(), PxMeshScale(convert(scale))),
                *scene->genericMaterial);
        collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_OBJECT | COLLISION_FLAG_SELECTABLE, DECAL_SIGN, 0, DRIVABLE_SURFACE));
        collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_OBJECT, -1, 0, 0));

        scene->getPhysicsScene()->addActor(*actor);
    }

    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override
    {
        static Texture* billboardTextures[] = {
            &g_res.textures->billboard_1,
            &g_res.textures->billboard_2,
            &g_res.textures->billboard_3,
            &g_res.textures->billboard_4,
        };

        glow.settings.worldTransform = transform;
        lights.settings.worldTransform = transform;
        metal.settings.worldTransform = transform;
        sign.settings.worldTransform = transform;
        wood.settings.worldTransform = transform;
        sign.settings.texture = billboardTextures[billboardIndex];
        rw->add(&glow);
        rw->add(&lights);
        rw->add(&metal);
        rw->add(&sign);
        rw->add(&wood);
    }

    void onPreview(RenderWorld* rw) override
    {
        rw->setViewportCamera(0, glm::vec3(3.f, 1.f, 4.f) * 4.5f,
                glm::vec3(0, 0, 3.5f), 1.f, 200.f, 32.f);
        transform = glm::mat4(1.f);
        onRender(rw, nullptr, 0.f);
    }

    void onEditModeRender(RenderWorld* rw, class Scene* scene, bool isSelected) override
    {
        if (isSelected)
        {
            rw->push(WireframeRenderable(lights.settings.mesh, transform));
            rw->push(WireframeRenderable(metal.settings.mesh, transform));
            rw->push(WireframeRenderable(wood.settings.mesh, transform));
        }
    }

    void showDetails(Scene* scene) override
    {
        const char* billboardNames[] = {
            "Ramifications",
            "You Want This Car",
            "Your ad here",
            "Don't Look",
        };

        g_gui.beginSelect("Billboard", &billboardIndex, true);
        for (i32 i=0; i<(i32)ARRAY_SIZE(billboardNames); ++i)
        {
            g_gui.option(billboardNames[i], i, nullptr);
        }
        g_gui.end();
    }

    DataFile::Value serializeState() override
    {
        DataFile::Value dict = PlaceableEntity::serializeState();
        dict["billboardIndex"] = DataFile::makeInteger(billboardIndex);
        return dict;
    }

    void deserializeState(DataFile::Value& data) override
    {
        PlaceableEntity::deserializeState(data);
        billboardIndex = (i32)data["billboardIndex"].integer(0);
    }

    EditorCategory getEditorCategory(u32 variationIndex) const override { return EditorCategory::ROADSIDE; }
};
