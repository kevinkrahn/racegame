#include "static_mesh.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
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

        for (auto& obj : objects)
        {
            if (obj.modelObject->isCollider)
            {
                Mesh& mesh = model->meshes[obj.modelObject->meshIndex];
                obj.shape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxConvexMeshGeometry(mesh.getConvexCollisionMesh(),
                        PxMeshScale(convert(scale * obj.modelObject->scale))), *g_game.physx.materials.generic);
                obj.shape->setQueryFilterData(
                        PxFilterData(COLLISION_FLAG_DYNAMIC, DECAL_NONE, 0, UNDRIVABLE_SURFACE));
                obj.shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_DYNAMIC, -1, 0, 0));
            }
        }
        PxRigidBodyExt::updateMassAndInertia(*((PxRigidDynamic*)actor), model->density);
    }
    else if (model->modelUsage == ModelUsage::STATIC_PROP)
    {
        actor = g_game.physx.physics->createRigidStatic(
                PxTransform(convert(position), convert(rotation)));

        for (auto& obj : objects)
        {
            if (obj.modelObject->isCollider)
            {
                Mesh& mesh = model->meshes[obj.modelObject->meshIndex];
                obj.shape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxTriangleMeshGeometry(mesh.getCollisionMesh(),
                        PxMeshScale(convert(scale * obj.modelObject->scale))), *g_game.physx.materials.generic);
                obj.shape->setQueryFilterData(
                        PxFilterData(COLLISION_FLAG_OBJECT, DECAL_GROUND, 0, DRIVABLE_SURFACE));
                obj.shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_OBJECT, -1, 0, 0));
            }
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
    transform = Mat4::translation(position) * Mat4(rotation) * Mat4::scaling(scale);

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
                        Mat4 t = Mat4::scaling(scale) * o.modelObject->getTransform();
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
                        Mat4 t = Mat4::scaling(scale) * o.modelObject->getTransform();
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
    Mat4 t = transform;
    if (model->modelUsage == ModelUsage::DYNAMIC_PROP)
    {
        t = Mat4(actor->getGlobalPose());
    }
    for (auto& o : objects)
    {
        if (o.modelObject->isVisible)
        {
            g_res.getMaterial(o.modelObject->materialGuid)->draw(rw,
                    t * o.modelObject->getTransform(), &model->meshes[o.modelObject->meshIndex]);
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
    transform = Mat4(1.f);
    BoundingBox bb = model->getBoundingbox(transform);
    Vec3 dir = normalize(Vec3(3.f, 1.f, 4.f));
    f32 dist = 3.f;
    for (u32 i=1; i<200; ++i)
    {
        rw->setViewportCamera(0, dir * dist, (bb.max + bb.min) * 0.5f, 1.f, 200.f, 32.f);

        Vec3 points[] = {
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
            Vec4 tp = rw->getCamera(0).viewProjection * Vec4(p, 1.f);
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
            g_res.getMaterial(o.modelObject->materialGuid)->draw(rw,
                    transform * o.modelObject->getTransform(), &model->meshes[o.modelObject->meshIndex]);

        }
    }
}

void StaticMesh::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected, u8 selectIndex)
{
    for (auto& o : objects)
    {
        if (o.modelObject->isVisible)
        {
            Material* mat = g_res.getMaterial(o.modelObject->materialGuid);
            Mat4 t = transform * o.modelObject->getTransform();
            Mesh* mesh = &model->meshes[o.modelObject->meshIndex];
            if (isSelected)
            {
                mat->drawHighlight(rw, t, mesh, selectIndex);
            }
            mat->drawPick(rw, t, mesh, entityCounterID);
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

Array<PropPrefabData> StaticMesh::generatePrefabProps()
{
    Array<PropPrefabData> result;
    g_res.iterateResourceType(ResourceType::MODEL, [&](Resource* res){
        Model* model = (Model*)res;
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
    });
    return result;
}
