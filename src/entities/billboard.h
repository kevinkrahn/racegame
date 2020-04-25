#pragma once

#include "../math.h"
#include "../entity.h"
#include "../resources.h"
#include "../mesh_renderables.h"
#include "../scene.h"
#include "../game.h"
#include "../imgui.h"
#include "../model.h"

class Billboard : public PlaceableEntity
{
    Model* model = nullptr;
    ModelObject* billboardObject = nullptr;
    //Texture* billboardTexture = nullptr;

public:
    Billboard()
    {
        model = g_res.getModel("billboard");
        billboardObject = model->getObjByName("BillboardSign");
    }

    void applyDecal(class Decal& decal) override
    {
        for (auto& obj : model->objects)
        {
            decal.addMesh(&model->meshes[obj.meshIndex], obj.getTransform() * transform);
        }
    }

    void onCreate(class Scene* scene) override
    {
        updateTransform(scene);

        actor = g_game.physx.physics->createRigidStatic(
                PxTransform(convert(position), convert(rotation)));
        physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
        physicsUserData.entity = this;
        actor->userData = &physicsUserData;

        for (auto& obj : model->objects)
        {
            PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxTriangleMeshGeometry(model->meshes[obj.meshIndex].getCollisionMesh(),
                        PxMeshScale(convert(scale * obj.scale))), *scene->genericMaterial);
            collisionShape->setQueryFilterData(PxFilterData(
                        COLLISION_FLAG_OBJECT | COLLISION_FLAG_SELECTABLE, DECAL_SIGN, 0, DRIVABLE_SURFACE));
            collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_OBJECT, -1, 0, 0));
            collisionShape->setLocalPose(convert(obj.getTransform()));
        }

        scene->getPhysicsScene()->addActor(*actor);
    }

    void onRender(RenderWorld* rw, Scene* scene, f32 deltaTime) override
    {
        for (auto& obj : model->objects)
        {
            rw->push(LitMaterialRenderable(&model->meshes[obj.meshIndex],
                        obj.getTransform() * transform, g_res.getMaterial(obj.materialGuid)));
        }
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
            for (auto& obj : model->objects)
            {
                rw->push(WireframeRenderable(&model->meshes[obj.meshIndex], obj.getTransform() * transform));
            }
        }
    }

    void showDetails(Scene* scene) override
    {
        // TODO
    }

    void serializeState(Serializer& s) override
    {
        PlaceableEntity::serializeState(s);
    }

    EditorCategory getEditorCategory(u32 variationIndex) const override { return EditorCategory::ROADSIDE; }
};
