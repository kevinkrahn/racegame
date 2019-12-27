#include "static_mesh.h"
#include "../scene.h"
#include "../renderer.h"
#include "../vehicle.h"
#include "../mesh_renderables.h"
#include "../game.h"

const char* meshScenes[] = {
    "rock.Scene",
    "world.Tunnel",
    "world.Sign",
    "cactus.Scene",
    "world.Cube",
    "plants.Plant1",
    "plants.Plant2",
    "ctvpole.CTVPole",
};

StaticMesh* StaticMesh::setup(u32 meshIndex, glm::vec3 const& position, glm::vec3 const& scale, f32 zRotation)
{
    this->position = position;
    this->scale = scale;
    this->rotation = glm::rotate(this->rotation, zRotation, glm::vec3(0, 0, 1));
    this->meshIndex = meshIndex;
    return this;
}

void StaticMesh::onCreate(Scene* scene)
{
    updateTransform(scene);
    actor = g_game.physx.physics->createRigidStatic(
            PxTransform(convert(position), convert(rotation)));
    physicsUserData.entityType = ActorUserData::SELECTABLE_ENTITY;
    physicsUserData.entity = this;
    actor->userData = &physicsUserData;

    static std::map<std::string, LitSettings> materials = {
        { "Rock", LitSettings(glm::vec3(1.f), 0.1f, 50.f, 0.1f, 2.f, -0.2f, &g_res.textures->rock) },
        { "Concrete", LitSettings(glm::vec3(1.f), 0.2f, 50.f, 0.f, 2.f, -0.2f, &g_res.textures->concrete) },
        { "Plastic", LitSettings(glm::vec3(1.f), 0.2f, 50.f, 0.1f, 2.f, -0.2f, nullptr, true, false, 0.f, 0.1f, 3.f) },
        { "Cactus", LitSettings(glm::vec3(1.f), 0.06f, 5.f, 0.1f, 2.f, -0.2f, &g_res.textures->cactus) },
        { "Plant1", LitSettings(glm::vec3(1.f), 0.0001f, 0.0001f, 0.f, 0.f, 0.f, &g_res.textures->plant1, true, false, 0.5f) },
        { "Plant2", LitSettings(glm::vec3(1.f), 0.0001f, 0.0001f, 0.f, 0.f, 0.f, &g_res.textures->plant2, true, false, 0.5f) },
        { "Metal", LitSettings(glm::vec3(0.01f), 0.2f, 120.f, 0.f, 0.f, 0.f, nullptr, true, false, 0.f, 0.4f, 4.f) },
    };

    DataFile::Value::Dict& sceneData = g_res.getScene(meshScenes[meshIndex]);
    for (auto& e : sceneData["entities"].array())
    {
        std::string name = e["name"].string();
        name = name.substr(0, name.find('.'));
        glm::mat4 transform = e["matrix"].convertBytes<glm::mat4>();

        LitSettings s = materials[name];
        std::string const& meshName = e["data_name"].string();
        s.mesh = g_res.getMesh(meshName.c_str());

        PxShape* collisionShape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(s.mesh->getCollisionMesh(), PxMeshScale(convert(scale * scaleOf(transform)))),
                *scene->genericMaterial);
        collisionShape->setLocalPose(convert(transform));
        //collisionShape->setQueryFilterData(PxFilterData(COLLISION_FLAG_SELECTABLE, 0, 0, UNDRIVABLE_SURFACE));
        collisionShape->setQueryFilterData(PxFilterData(
                    COLLISION_FLAG_GROUND | COLLISION_FLAG_SELECTABLE, DECAL_GROUND, 0, DRIVABLE_SURFACE));
        collisionShape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND, -1, 0, 0));

        meshes.push_back({ s, transform, collisionShape });
    }

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
