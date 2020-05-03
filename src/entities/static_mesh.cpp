#include "static_mesh.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

void StaticMesh::loadModel()
{
    model = g_res.getModel(modelGuid);
    objects.clear();
    for (auto& obj : model->objects)
    {
        objects.push_back({ &obj });
    }
}

void StaticMesh::onCreate(Scene* scene)
{
    updateTransform(scene);
    loadModel();

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
    if (scene->isBatched && model->modelUsage != ModelUsage::DYNAMIC_PROP)
    {
        return;
    }
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
                        g_res.getMaterial(o.modelObject->materialGuid), entityCounterID));

        }
    }
}

void StaticMesh::onBatch(Batcher& batcher)
{
    if (model->modelUsage == ModelUsage::DYNAMIC_PROP)
    {
        return;
    }
    for (auto& o : objects)
    {
        if (o.modelObject->isVisible)
        {
            batcher.add(g_res.getMaterial(o.modelObject->materialGuid),
                transform * o.modelObject->getTransform(), &model->meshes[o.modelObject->meshIndex]);
        }
    }
}

void StaticMesh::onPreview(RenderWorld* rw)
{
    transform = glm::mat4(1.f);
    BoundingBox bb = model->getBoundingbox(transform);
    glm::vec3 dir = glm::normalize(glm::vec3(3.f, 1.f, 4.f));
    f32 dist = 3.f;
    for (u32 i=1; i<200; ++i)
    {
        rw->setViewportCamera(0, dir * dist, (bb.max + bb.min) * 0.5f, 1.f, 200.f, 32.f);

        glm::vec3 points[] = {
            { bb.min.x, bb.min.y, bb.min.z },
            { bb.min.x, bb.max.y, bb.min.z },
            { bb.max.x, bb.min.y, bb.min.z },
            { bb.max.x, bb.max.y, bb.min.z },
            { bb.min.x, bb.min.y, bb.max.z },
            { bb.min.x, bb.max.y, bb.max.z },
            { bb.max.x, bb.min.y, bb.max.z },
            { bb.max.x, bb.max.y, bb.max.z },
        };

        bool allPointsVisible = true;
        f32 margin = 0.01f;
        for (auto& p : points)
        {
            glm::vec4 tp = rw->getCamera(0).viewProjection * glm::vec4(p, 1.f);
            tp.x = (((tp.x / tp.w) + 1.f) / 2.f);
            tp.y = ((-1.f * (tp.y / tp.w) + 1.f) / 2.f);
            if (tp.x < margin || tp.x > 1.f - margin || tp.y < margin || tp.y > 1.f - margin)
            {
                allPointsVisible = false;
                break;
            }
        }

        if (allPointsVisible)
        {
            break;
        }

        dist += 1.f;
    }

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
                /*
                rw->push(WireframeRenderable(&model->meshes[o.modelObject->meshIndex],
                            transform * o.modelObject->getTransform()));
                */
                rw->addHighlightMesh(&model->meshes[o.modelObject->meshIndex],
                        transform * o.modelObject->getTransform(), irandom(scene->randomSeries, 0, 10000));
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
        // TODO: Remove this once scenes are updated
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

    for (auto& res : g_res.resources)
    {
        // TODO: make a better way to iterate over resources of a specific type
        if (res.second->type != ResourceType::MODEL)
        {
            continue;
        }
        Model* model = (Model*)res.second.get();
        if (model->modelUsage == ModelUsage::DYNAMIC_PROP
                || model->modelUsage == ModelUsage::STATIC_PROP)
        {
            i64 guid = model->guid;
            result.push_back({
                model->category,
                model->name,
                [guid](PlaceableEntity* e) {
                    StaticMesh* sm = (StaticMesh*)e;
                    sm->modelGuid = guid;
                    sm->loadModel();
                }
            });
        }
    }

    return result;
}
