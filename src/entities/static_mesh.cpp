#include "static_mesh.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

void StaticMesh::onCreate(Scene* scene)
{
    updateTransform(scene);

    model = g_res.getModel(modelGuid);
    if (model->modelUsage == ModelUsage::DYNAMIC_PROP)
    {
        actor = g_game.physx.physics->createRigidDynamic(
                PxTransform(convert(position), convert(rotation)));

        for (auto& obj : model->objects)
        {
            PxShape* shape = nullptr;
            if (obj.isCollider)
            {
                Mesh& mesh = model->meshes[obj.meshIndex];
                shape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxConvexMeshGeometry(mesh.getConvexCollisionMesh(),
                        PxMeshScale(convert(scale * obj.scale))), *scene->genericMaterial);
                shape->setQueryFilterData(PxFilterData(
                            COLLISION_FLAG_DYNAMIC | COLLISION_FLAG_SELECTABLE, DECAL_NONE,
                            0, UNDRIVABLE_SURFACE));
                shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_DYNAMIC, -1, 0, 0));
            }
            objects.push_back({ &obj, shape });
        }
        PxRigidBodyExt::updateMassAndInertia(*((PxRigidDynamic*)actor), model->density);
    }
    else if (model->modelUsage == ModelUsage::STATIC_PROP)
    {
        actor = g_game.physx.physics->createRigidStatic(
                PxTransform(convert(position), convert(rotation)));

        for (auto& obj : model->objects)
        {
            PxShape* shape = nullptr;
            if (obj.isCollider)
            {
                Mesh& mesh = model->meshes[obj.meshIndex];
                shape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxTriangleMeshGeometry(mesh.getCollisionMesh(),
                        PxMeshScale(convert(scale * obj.scale))), *scene->genericMaterial);
                shape->setQueryFilterData(PxFilterData(
                            COLLISION_FLAG_OBJECT | COLLISION_FLAG_SELECTABLE, DECAL_GROUND, 0, DRIVABLE_SURFACE));
                shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_OBJECT, -1, 0, 0));
            }
            objects.push_back({ &obj, shape });
        }
    }

    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;
    scene->getPhysicsScene()->addActor(*actor);
    updateTransform(scene);
}

void StaticMesh::updateTransform(Scene* scene)
{
    transform = glm::translate(glm::mat4(1.f), position)
        * glm::mat4_cast(rotation)
        * glm::scale(glm::mat4(1.f), scale);

    if (actor)
    {
        actor->setGlobalPose(PxTransform(convert(position), convert(rotation)));
        for (auto& o : objects)
        {
            if (o.shape)
            {
                if (model->modelUsage == ModelUsage::DYNAMIC_PROP)
                {
                    PxConvexMeshGeometry geom;
                    if (o.shape->getConvexMeshGeometry(geom))
                    {
                        glm::mat4 t = glm::scale(glm::mat4(1.f), scale) * o.modelObject->getTransform();
                        o.shape->setLocalPose(convert(t));
                        geom.scale = convert(scale * o.modelObject->scale);
                        o.shape->setGeometry(geom);
                    }
                }
                else
                {
                    PxTriangleMeshGeometry geom;
                    if (o.shape->getTriangleMeshGeometry(geom))
                    {
                        glm::mat4 t = glm::scale(glm::mat4(1.f), scale) * o.modelObject->getTransform();
                        o.shape->setLocalPose(convert(t));
                        geom.scale = convert(scale * o.modelObject->scale);
                        o.shape->setGeometry(geom);
                    }
                }
            }
        }
    }
}

void StaticMesh::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    glm::mat4 t = transform;
    if (model->modelUsage == ModelUsage::DYNAMIC_PROP)
    {
        t = convert(actor->getGlobalPose());
    }
    for (auto& o : objects)
    {
        if (o.modelObject->isVisible)
        {
            rw->push(LitMaterialRenderable(&model->meshes[o.modelObject->meshIndex],
                        t * o.modelObject->getTransform(),
                        g_res.getMaterial(o.modelObject->materialGuid)));

        }
    }
}

void StaticMesh::onPreview(RenderWorld* rw)
{
    rw->setViewportCamera(0, glm::vec3(3.f, 1.f, 4.f) * 4.5f,
            glm::vec3(0, 0, 3.5f), 1.f, 200.f, 32.f);
    // TODO: automatically compute camera for optimal view of model
    for (auto& o : objects)
    {
        if (o.modelObject->isVisible)
        {
            rw->push(LitMaterialRenderable(&model->meshes[o.modelObject->meshIndex],
                        transform * o.modelObject->getTransform(),
                        g_res.getMaterial(o.modelObject->materialGuid)));

        }
    }
}

void StaticMesh::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
{
    if (isSelected)
    {
        for (auto& o : objects)
        {
            if (o.modelObject->isVisible)
            {
                rw->push(WireframeRenderable(&model->meshes[o.modelObject->meshIndex],
                            transform * o.modelObject->getTransform()));
            }
        }
    }
}

void StaticMesh::applyDecal(Decal& decal)
{
    for (auto& o : objects)
    {
        if (o.modelObject->isVisible)
        {
            decal.addMesh(&model->meshes[o.modelObject->meshIndex],
                    transform * o.modelObject->getTransform());
        }
    }
}

void StaticMesh::serializeState(Serializer& s)
{
    PlaceableEntity::serializeState(s);
    s.field(modelGuid);
    if (s.deserialize && modelGuid == 0)
    {
        const char* meshIndexToMeshNameMap[] = {
            "rock",
            "tunnel",
            "sign",
            "cactus",
            "concrete_cube",
            "plant1",
            "plant2",
            "CTVPole",
            "windmill",
        };
        u32 meshIndex;
        s.field(meshIndex);
        assert(meshIndex < ARRAY_SIZE(meshIndexToMeshNameMap));
        modelGuid = g_res.getModel(meshIndexToMeshNameMap[meshIndex])->guid;
    }
}

std::vector<PropPrefabData> StaticMesh::generatePrefabProps()
{
    std::vector<PropPrefabData> result;

    for (auto& model : g_res.models)
    {
        if (model.second->modelUsage == ModelUsage::DYNAMIC_PROP
                || model.second->modelUsage == ModelUsage::STATIC_PROP)
        {
            i64 guid = model.first;
            result.push_back({
                model.second->category,
                model.second->name,
                [guid](PlaceableEntity* e) {
                    ((StaticMesh*)e)->modelGuid = guid;
                    ((StaticMesh*)e)->model = g_res.getModel(guid);
                }
            });
        }
    }

    return result;
}
