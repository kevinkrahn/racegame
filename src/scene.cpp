#include "scene.h"
#include "game.h"
#include "vehicle.h"

Scene::Scene(const char* name)
{
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

	vehicleMaterial = game.physx.physics->createMaterial(0.1f, 0.1f, 0.5f);
	trackMaterial   = game.physx.physics->createMaterial(0.4f, 0.4f, 0.4f);
	offroadMaterial = game.physx.physics->createMaterial(0.4f, 0.4f, 0.1f);

    auto& sceneData = game.resources.getScene(name);
    for (auto& e : sceneData["entities"].array())
    {
        if (e["type"].string() == "MESH")
        {
            glm::mat4 transform = glm::make_mat4((f32*)e["matrix"].bytearray().data());
            std::string dataName = e["data_name"].string();
            staticEntities.push_back({
                game.resources.getMesh(dataName.c_str()).renderHandle,
                transform
            });

            PxVec3 scale(glm::length(glm::vec3(transform[0])),
                         glm::length(glm::vec3(transform[1])),
                         glm::length(glm::vec3(transform[2])));

            PxShape* shape = game.physx.physics->createShape(
                    PxTriangleMeshGeometry(game.resources.getCollisionMesh(dataName.c_str()),
                        PxMeshScale(scale)), *offroadMaterial);
            shape->setQueryFilterData(PxFilterData(COLLISION_FLAG_GROUND, 0, 0, DRIVABLE_SURFACE));
            shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND, -1, 0, 0));

	        PxRigidStatic* triMesh = PxCreateStatic(*game.physx.physics, convert(transform), *shape);
	        physicsScene->addActor(*triMesh);

	        //shape->release();
	        //triMesh->release();
        }
    }
}

Scene::~Scene()
{
    physicsScene->release();
}

void Scene::onStart()
{

}

void Scene::onUpdate(f32 deltaTime)
{
    physicsScene->simulate(deltaTime);
    physicsScene->fetchResults(true);

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

    game.renderer.setBackgroundColor(glm::vec3(0.1, 0.1, 0.1));
    game.renderer.setViewportCount(1);
    game.renderer.setViewportCamera(0, { 5, 5, 5 }, { 0, 0, 0 });
    game.renderer.addDirectionalLight(glm::vec3(1, 1, -1), glm::vec3(1.0));
    for (auto const& e : staticEntities)
    {
        game.renderer.drawMesh(e.renderHandle, e.worldTransform);
    }
}

void Scene::onEnd()
{
}

