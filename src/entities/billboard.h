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
    i64 billboardTextureGuid = 0;

public:
    Billboard()
    {
        model = g_res.getModel("billboard");
        billboardObject = model->getObjByName("BillboardSign");
        billboardTextureGuid = g_res.getTexture("billboard_1")->guid;
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
        rw->push(LitRenderable(model->getMeshByName("billboard.BillboardSign"),
                    transform, g_res.getTexture(billboardTextureGuid)));
        if (scene->isBatched)
        {
            return;
        }
        for (auto& obj : model->objects)
        {
            if (obj.isVisible)
            {
                rw->push(LitMaterialRenderable(&model->meshes[obj.meshIndex],
                            transform * obj.getTransform(), g_res.getMaterial(obj.materialGuid)));
            }
        }
    }

    void onBatch(Batcher& batcher) override
    {
        for (auto& obj : model->objects)
        {
            if (obj.isVisible)
            {
                batcher.add(g_res.getMaterial(obj.materialGuid),
                    transform * obj.getTransform(), &model->meshes[obj.meshIndex]);
            }
        }
    }

    void onPreview(RenderWorld* rw) override
    {
        rw->setViewportCamera(0, glm::vec3(3.f, 1.f, 4.f) * 4.5f,
                glm::vec3(0, 0, 3.5f), 1.f, 200.f, 32.f);
        transform = glm::mat4(1.f);
        rw->push(LitRenderable(model->getMeshByName("billboard.BillboardSign"),
                    transform, g_res.getTexture(billboardTextureGuid)));
        for (auto& obj : model->objects)
        {
            if (obj.isVisible)
            {
                rw->push(LitMaterialRenderable(&model->meshes[obj.meshIndex],
                            transform * obj.getTransform(), g_res.getMaterial(obj.materialGuid)));
            }
        }
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
        u32 buttonCount = 0;
        for (auto& res : g_res.resources)
        {
            if (res.second->type != ResourceType::TEXTURE)
            {
                continue;
            }
            Texture* tex = (Texture*)res.second.get();
            if (tex->name.find("billboard") != std::string::npos)
            {
                f32 w = glm::min(ImGui::GetColumnWidth(), 200.f);
                f32 h = w * 0.5f;
                ImGui::PushID(tex->name.c_str());
                if (ImGui::ImageButton((void*)(uintptr_t)tex->getPreviewHandle(), { w, h }))
                {
                    billboardTextureGuid = tex->guid;
                }
                ImGui::PopID();
                ++buttonCount;
            }
        }
    }

    void serializeState(Serializer& s) override
    {
        s.field(billboardTextureGuid);
        PlaceableEntity::serializeState(s);
    }

    std::vector<PropPrefabData> generatePrefabProps() override
    {
        std::vector<PropPrefabData> result = {
            { PropCategory::NOT_NATURE, "Billboard" }
        };
        return result;
    }

    const char* getName() const override { return "Billboard"; }
};
