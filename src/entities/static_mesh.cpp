#include "static_mesh.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

struct MeshScene
{
    const char* sceneName;
    glm::vec3 previewCameraFrom;
    glm::vec3 previewCameraTo;
    EditorCategory category;
};

MeshScene meshScenes[] = {
    { "rock.Scene", glm::vec3(8.f, 8.f, 10.f) * 2.5f, glm::vec3(0, 0, 3), EditorCategory::ROCKS },
    { "world.Tunnel", glm::vec3(8.f, 8.f, 10.f) * 3.f, glm::vec3(0, 0, 3), EditorCategory::ROADSIDE },
    { "world.Sign", glm::vec3(8.f, 8.f, 10.f) * 1.5f, glm::vec3(0, 0, 4), EditorCategory::ROADSIDE },
    { "cactus.Scene", glm::vec3(8.f, 8.f, 10.f) * 2.1f, glm::vec3(0, 0, 6), EditorCategory::VEGETATION },
    { "world.Cube", glm::vec3(8.f, 8.f, 10.f) * 0.5f, glm::vec3(0, 0, 1), EditorCategory::MISC },
    { "plants.Plant1", glm::vec3(8.f, 8.f, 10.f) * 0.4f, glm::vec3(0, 0, 1), EditorCategory::VEGETATION },
    { "plants.Plant2", glm::vec3(8.f, 8.f, 10.f) * 0.4f, glm::vec3(0, 0, 1), EditorCategory::VEGETATION },
    { "ctvpole.CTVPole", glm::vec3(8.f, 8.f, 10.f), glm::vec3(0, 0, 3), EditorCategory::ROADSIDE },
    { "windmill.Windmill", glm::vec3(14.f, 14.f, 26.f), glm::vec3(0, 0, 12), EditorCategory::ROADSIDE },
};

void StaticMesh::onCreate(Scene* scene)
{
    updateTransform(scene);
    actor = g_game.physx.physics->createRigidStatic(
            PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;

    loadMeshes();
    for (auto& m : meshes)
    {
        PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(m.s.settings.mesh->getCollisionMesh(), PxMeshScale(convert(scale * scaleOf(transform)))),
                *scene->genericMaterial);
        collisionShape->setLocalPose(convert(transform));
        //collisionShape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, UNDRIVABLE_SURFACE));
        collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_GROUND | COLLISION_FLAG_SELECTABLE, DECAL_GROUND, 0, DRIVABLE_SURFACE));
        collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND, -1, 0, 0));
        m.shape = collisionShape;
    }

    scene->getPhysicsScene()->addActor(*actor);
    updateTransform(scene);
}

void StaticMesh::loadMeshes()
{
    static std::map<std::string, LitSettings> materials = {
        { "Rock", LitSettings(glm::vec3(1.f), 0.1f, 50.f, 0.1f, 2.f, -0.2f, &g_res.textures->rock) },
        { "Concrete", LitSettings(glm::vec3(1.f), 0.2f, 50.f, 0.f, 2.f, -0.2f, &g_res.textures->concrete) },
        { "Plastic", LitSettings(glm::vec3(1.f), 0.2f, 50.f, 0.1f, 2.f, -0.2f, nullptr, true, false, 0.f, 0.1f, 3.f) },
        { "Cactus", LitSettings(glm::vec3(1.f), 0.06f, 5.f, 0.1f, 2.f, -0.2f, &g_res.textures->cactus) },
        { "Plant1", LitSettings(glm::vec3(1.f), 0.0001f, 0.0001f, 0.f, 0.f, 0.f, &g_res.textures->plant1, true, false, 0.5f) },
        { "Plant2", LitSettings(glm::vec3(1.f), 0.0001f, 0.0001f, 0.f, 0.f, 0.f, &g_res.textures->plant2, true, false, 0.5f) },
        { "Metal", LitSettings(glm::vec3(0.01f), 0.2f, 120.f, 0.f, 0.f, 0.f, nullptr, true, false, 0.f, 0.4f, 4.f) },
        { "WindmillBlades", LitSettings(glm::vec3(1.f), 0.2f, 120.f, 0.f, 0.f, 0.f, &g_res.textures->windmill_blades, true, false, 0.f, 0.4f, 4.f) },
        { "WindmillBase", LitSettings(glm::vec3(1.f), 0.1f, 50.f, 0.1f, 2.f, -0.2f, &g_res.textures->windmill_base) },
    };

    DataFile::Value::Dict& sceneData = g_res.getScene(meshScenes[meshIndex].sceneName);
    for (auto& e : sceneData["entities"].array())
    {
        std::string name = e["name"].string();
        name = name.substr(0, name.find('.'));
        glm::mat4 transform = e["matrix"].convertBytes<glm::mat4>();

        LitSettings s = materials[name];
        std::string const& meshName = e["data_name"].string();
        s.mesh = g_res.getMesh(meshName.c_str());

        meshes.push_back({ s, transform, nullptr });
    }
}

void StaticMesh::updateTransform(Scene* scene)
{
    transform = glm::translate(glm::mat4(1.f), position)
        * glm::mat4_cast(rotation)
        * glm::scale(glm::mat4(1.f), scale);

    if (actor)
    {
        actor->setGlobalPose(PxTransform(convert(position), convert(rotation)));
        for (auto& m : meshes)
        {
            if (m.shape)
            {
                PxTriangleMeshGeometry geom;
                if (m.shape->getTriangleMeshGeometry(geom))
                {
                    glm::mat4 t = glm::scale(glm::mat4(1.f), scale) * m.transform;
                    m.shape->setLocalPose(convert(t));
                    geom.scale = convert(scale * scaleOf(m.transform));
                    m.shape->setGeometry(geom);
                }
            }
        }
    }
}

void StaticMesh::onRender(RenderWorld* rw, Scene* scene, f32 deltaTime)
{
    for (auto& m : meshes)
    {
        m.s.settings.worldTransform = transform * m.transform;
        rw->add(&m.s);
    }
}

void StaticMesh::onPreview(RenderWorld* rw)
{
    loadMeshes();
    rw->setViewportCamera(0, meshScenes[meshIndex].previewCameraFrom,
            meshScenes[meshIndex].previewCameraTo, 1.f, 200.f, 40.f);
    for (auto& m : meshes)
    {
        m.s.settings.worldTransform = m.transform;
        rw->add(&m.s);
    }
}

void StaticMesh::onEditModeRender(RenderWorld* rw, Scene* scene, bool isSelected)
{
    if (isSelected)
    {
        for (auto& m : meshes)
        {
            rw->push(WireframeRenderable(m.s.settings.mesh, transform * m.transform));
        }
    }
}

DataFile::Value StaticMesh::serializeState()
{
    DataFile::Value dict = PlaceableEntity::serializeState();
    dict["meshIndex"] = DataFile::makeInteger(meshIndex);
    return dict;
}

void StaticMesh::deserializeState(DataFile::Value& data)
{
    PlaceableEntity::deserializeState(data);
    meshIndex = (u32)data["meshIndex"].integer(0);
}

void StaticMesh::applyDecal(Decal& decal)
{
    for (auto& m : meshes)
    {
        decal.addMesh(m.s.settings.mesh, transform * m.transform);
    }
}

u32 StaticMesh::getVariationCount() const { return ARRAY_SIZE(meshScenes); }

EditorCategory StaticMesh::getEditorCategory(u32 variationIndex) const
{
    return meshScenes[variationIndex].category;
}
