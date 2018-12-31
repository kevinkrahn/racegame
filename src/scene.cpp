#include "scene.h"
#include "game.h"
#include "vehicle.h"
#include <algorithm>

Scene::Scene(const char* name)
{
    // create PhysX scene
    PxSceneDesc sceneDesc(game.physx.physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, 0.f, -12.1f);
    sceneDesc.cpuDispatcher = game.physx.dispatcher;
    sceneDesc.filterShader  = VehicleFilterShader;
    sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;

    physicsScene = game.physx.physics->createScene(sceneDesc);

    if (PxPvdSceneClient* pvdClient = physicsScene->getScenePvdClient())
    {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    // create physics materials
	vehicleMaterial = game.physx.physics->createMaterial(0.1f, 0.1f, 0.5f);
	trackMaterial   = game.physx.physics->createMaterial(0.4f, 0.4f, 0.4f);
	offroadMaterial = game.physx.physics->createMaterial(0.4f, 0.4f, 0.1f);

    // construct scene from blender data
    auto& sceneData = game.resources.getScene(name);
    auto& entities = sceneData["entities"].array();
    bool foundStart = false;
    for (auto& e : entities)
    {
        std::string entityType = e["type"].string();
        if (entityType == "MESH")
        {
            glm::mat4 transform = e["matrix"].convertBytes<glm::mat4>();
            std::string dataName = e["data_name"].string();
            std::string name = e["name"].string();

            if (name.find("Start") != std::string::npos)
            {
                start = transform;
                foundStart = true;
                auto it = std::find_if(entities.begin(), entities.end(), [](auto& e) {
                    return e["name"].string().find("TrackGraph") != std::string::npos;
                });
                if (it == entities.end())
                {
                    error("Scene does not contain a track graph. The AI will not know where to drive!\n");
                    continue;
                }
                trackGraph = TrackGraph(start,
                        game.resources.getMesh((*it)["data_name"].string().c_str()),
                        (*it)["matrix"].convertBytes<glm::mat4>());
            }

            Mesh const& mesh = game.resources.getMesh(dataName.c_str());
            if (mesh.elementSize < 3)
            {
                continue;
            }

            staticEntities.push_back({
                mesh.renderHandle,
                transform
            });

            PxVec3 scale(glm::length(glm::vec3(transform[0])),
                         glm::length(glm::vec3(transform[1])),
                         glm::length(glm::vec3(transform[2])));

	        PxRigidStatic* actor = game.physx.physics->createRigidStatic(convert(transform));
            PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxTriangleMeshGeometry(game.resources.getCollisionMesh(dataName.c_str()),
                        PxMeshScale(scale)), *offroadMaterial);
            shape->setQueryFilterData(PxFilterData(COLLISION_FLAG_GROUND, 0, 0, DRIVABLE_SURFACE));
            shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND, -1, 0, 0));
	        physicsScene->addActor(*actor);
        }
    }

    if (!foundStart)
    {
        FATAL_ERROR("Track does not have a starting point!");
    }
}

Scene::~Scene()
{
    physicsScene->release();
}

void Scene::onStart()
{
    const u32 numVehicles = 8;
    VehicleData& vehicleData = car;
    for (u32 i=0; i<numVehicles; ++i)
    {
        glm::vec3 offset = -glm::vec3(6 + i / 4 * 8, -9.f + i % 4 * 6, 0.f);

        PxRaycastBuffer hit;
        PxQueryFilterData filter;
        filter.flags |= PxQueryFlag::eSTATIC;
        filter.data = PxFilterData(COLLISION_FLAG_GROUND, 0, 0, 0);
        PxVec3 from = convert(translationOf(glm::translate(start, offset + glm::vec3(0, 0, 8))));
        PxVec3 dir = convert(-zAxisOf(start));
        if (!physicsScene->raycast(from, dir, 30.f, hit, PxHitFlags(PxHitFlag::eDEFAULT), filter))
        {
            FATAL_ERROR("The starting point is too high in the air!");
        }

        glm::mat4 vehicleTransform = glm::translate(glm::mat4(1.f),
                convert(hit.block.position + hit.block.normal * vehicleData.getRestOffset())) * rotationOf(start);

        const PxMaterial* surfaceMaterials[] = { trackMaterial, offroadMaterial };
        vehicles.push_back(
                std::make_unique<Vehicle>(physicsScene, vehicleTransform, vehicleData, vehicleMaterial,
                    surfaceMaterials, i == 0, i < 2));
    }
}

void Scene::onUpdate(f32 deltaTime)
{
    physicsScene->simulate(deltaTime);
    physicsScene->fetchResults(true);

    game.renderer.setBackgroundColor(glm::vec3(0.1, 0.1, 0.1));
    game.renderer.setViewportCount(2);
    game.renderer.addDirectionalLight(glm::vec3(-0.5f, -0.5f, -1.f), glm::vec3(1.0));
    //game.renderer.drawQuad2D(game.resources.getTexture("circle").renderHandle,
            //{ 50, 50 }, { 100, 100 }, { 0.f, 0.f }, { 1.f, 1.f }, { 1, 1, 1 }, 1.f);

    game.resources.getFont("font", 30).drawText(str("FPS: ", 1.f / deltaTime).c_str(), 20, 0, glm::vec3(1));

    for (auto const& e : staticEntities)
    {
        game.renderer.drawMesh(e.renderHandle, e.worldTransform);
    }

    for (u32 i=0; i<vehicles.size(); ++i)
    {
        vehicles[i]->onUpdate(deltaTime, physicsScene, i);
    }

    if (game.input.isKeyPressed(KEY_F2))
    {
        physicsDebugVisualizationEnabled = !physicsDebugVisualizationEnabled;
        if (physicsDebugVisualizationEnabled)
        {
	        physicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
	        physicsScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 2.0f);
	        physicsScene->setVisualizationParameter(PxVisualizationParameter::eBODY_MASS_AXES, 1.0f);
	        physicsScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 2.0f);
	        physicsScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL, 2.0f);
	        physicsScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);
        }
        else
        {
	        physicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.0f);
        }
    }

    if (physicsDebugVisualizationEnabled)
    {
        const PxRenderBuffer& rb = physicsScene->getRenderBuffer();
        for(PxU32 i=0; i < rb.getNbLines(); ++i)
        {
            const PxDebugLine& line = rb.getLines()[i];
            game.renderer.drawLine(convert(line.pos0), convert(line.pos1), rgbaFromU32(line.color0), rgbaFromU32(line.color1));
        }
    }
}

void Scene::onEnd()
{
}

