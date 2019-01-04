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
	vehicleMaterial = game.physx.physics->createMaterial(0.1f, 0.1f, 0.4f);
	trackMaterial   = game.physx.physics->createMaterial(0.4f, 0.4f, 0.4f);
	offroadMaterial = game.physx.physics->createMaterial(0.4f, 0.4f, 0.1f);

    // construct scene from blender data
    auto& sceneData = game.resources.getScene(name);
    auto& entities = sceneData["entities"].array();
    bool foundStart = false;

    struct TrackMesh
    {
        Mesh const& mesh;
        glm::mat4 transform;
        f32 boundX, boundY, boundZ;
    };
    std::vector<TrackMesh> trackMeshes;
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
                    error("Scene does not contain a track graph.\n");
                }
                else
                {
                    trackGraph = TrackGraph(start,
                            game.resources.getMesh((*it)["data_name"].string().c_str()),
                            (*it)["matrix"].convertBytes<glm::mat4>());
                }
            }

            Mesh const& mesh = game.resources.getMesh(dataName.c_str());
            if (mesh.elementSize < 3)
            {
                continue;
            }

            bool isTrack = name.find("Track") != std::string::npos;
            bool isSand = name.find("Sand") != std::string::npos;
            staticEntities.push_back({
                mesh.renderHandle,
                transform,
                { isTrack || isSand ? ActorUserData::TRACK : ActorUserData::SCENERY }
            });

            if (isTrack)
            {
                trackMeshes.push_back({
                    mesh,
                    transform,
                    (f32)e["bound_x"].real(),
                    (f32)e["bound_y"].real(),
                    (f32)e["bound_z"].real(),
                });
            }

            PxMaterial* material = isTrack ? trackMaterial : offroadMaterial;
            if (isSand) material = offroadMaterial;
	        PxRigidStatic* actor = game.physx.physics->createRigidStatic(convert(transform));
            PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxTriangleMeshGeometry(game.resources.getCollisionMesh(dataName.c_str()),
                        PxMeshScale(convert(scaleOf(transform)))), *material);
            shape->setQueryFilterData(PxFilterData(COLLISION_FLAG_GROUND, 0, 0, DRIVABLE_SURFACE));
            shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_GROUND, -1, 0, 0));
            actor->userData = &staticEntities.back().userData;
	        physicsScene->addActor(*actor);
        }
        else if (entityType == "PATH")
        {
            auto& data = e["points"].bytearray();
            paths.push_back(std::vector<glm::vec3>(
                        (glm::vec3*)data.data(),
                        (glm::vec3*)(data.data() + data.size())));
        }
    }

    if (!foundStart)
    {
        FATAL_ERROR("Track does not have a starting point!");
    }

    // render HUD track texture
    glm::vec3 minP(FLT_MAX), maxP(-FLT_MAX);
    for (auto& t : trackMeshes)
    {
        glm::vec3 min = t.mesh.aabbMin;
        glm::vec3 max = t.mesh.aabbMax;
        glm::vec3 corners[8] = {
            { min.x, min.y, min.z },
            { max.x, min.y, min.z },
            { max.x, max.y, min.z },
            { min.x, max.y, min.z },
            { min.x, min.y, max.z },
            { max.x, min.y, max.z },
            { max.x, max.y, max.z },
            { min.x, max.y, max.z },
        };
        for (glm::vec3& v : corners)
        {
            v = glm::vec3(t.transform * glm::vec4(v, 1.0));
            if (v.x < minP.x) minP.x = v.x;
            if (v.y < minP.y) minP.y = v.y;
            if (v.z < minP.z) minP.z = v.z;
            if (v.x > maxP.x) maxP.x = v.x;
            if (v.y > maxP.y) maxP.y = v.y;
            if (v.z > maxP.z) maxP.z = v.z;
        }
    }
    f32 pad = 20.f;
    trackOrtho = glm::rotate(glm::mat4(1.f), f32(M_PI * -0.5f), { 0, 0, 1 }) * glm::ortho(maxP.x + pad, minP.x - pad, minP.y - pad,
            maxP.y + pad, -maxP.z - 10.f, -minP.z + 10.f);

    for (auto& t : trackMeshes)
    {
        trackItems.push_back({ t.mesh.renderHandle, trackOrtho * t.transform });
    }
    trackAspectRatio = (maxP.x - minP.x) / (maxP.y - minP.y);

    trackItems.push_back({
        game.resources.getMesh("world.Quad").renderHandle,
        trackOrtho * start * glm::translate(glm::mat4(1.f), { 0, 0, -4 }) * glm::scale(glm::mat4(1.f), { 5, 16, 1 }),
        glm::vec3(0.2f)
    });
}

Scene::~Scene()
{
    physicsScene->release();
	vehicleMaterial->release();
	trackMaterial->release();
	offroadMaterial->release();
}

void Scene::onStart()
{
    const u32 numVehicles = 8;
    for (u32 i=0; i<numVehicles; ++i)
    {
        glm::vec3 offset = -glm::vec3(6 + i / 4 * 8, -9.f + i % 4 * 6, 0.f);

        PxRaycastBuffer hit;
        if (!raycastStatic(translationOf(glm::translate(start, offset + glm::vec3(0, 0, 8))),
                -zAxisOf(start), 30.f, &hit))
        {
            FATAL_ERROR("The starting point is too high in the air!");
        }

        VehicleData* vehicleData = i % 2 == 0 ? &racecar : &car;
        glm::mat4 vehicleTransform = glm::translate(glm::mat4(1.f),
                convert(hit.block.position + hit.block.normal * vehicleData->getRestOffset())) * rotationOf(start);

        const PxMaterial* surfaceMaterials[] = { trackMaterial, offroadMaterial };
        vehicles.push_back(
                std::make_unique<Vehicle>(*this, vehicleTransform, -offset,
                    vehicleData, vehicleMaterial, surfaceMaterials, i == 0, i < viewportCount, i));
    }
}

void Scene::onUpdate(f32 deltaTime)
{
    physicsScene->simulate(deltaTime);
    physicsScene->fetchResults(true);

    game.renderer.setViewportCount(viewportCount);
    game.renderer.addDirectionalLight(glm::vec3(-0.5f, -0.5f, -1.f), glm::vec3(1.0));

    // draw static entities
    for (auto const& e : staticEntities)
    {
        game.renderer.drawMesh(e.renderHandle, e.worldTransform);
    }

    // draw vehicle debris
    for (auto const& d : vehicleDebris)
    {
        game.renderer.drawMesh(d.renderHandle, convert(d.rigidBody->getGlobalPose()));
    }

    // update projectiles
    for (auto it = projectiles.begin(); it != projectiles.end();)
    {
        glm::vec3 prevPosition = it->position;
        it->position += it->velocity * deltaTime;
        game.renderer.drawMesh(game.resources.getMesh("world.Bullet"),
                glm::translate(glm::mat4(1.f), it->position) *
                glm::transpose(glm::lookAt(glm::vec3(0.f), it->velocity, glm::vec3(0, 0, 1))));
        // TODO: use sweep instead of raycast
        PxRaycastBuffer hit;
        if (raycast(prevPosition,
                    glm::normalize(it->position - prevPosition),
                    glm::length(it->position - prevPosition), &hit))
        {
            ActorUserData* data = (ActorUserData*)hit.block.actor->userData;
            if (data && data->entityType == ActorUserData::VEHICLE)
            {
                vehicles[data->vehicleIndex]->applyDamage(50.f, it->instigator);
            }
            it = projectiles.erase(it);
            continue;
        }

        ++it;
    }

    // update vehicles
    for (u32 i=0; i<vehicles.size(); ++i)
    {
        vehicles[i]->onUpdate(deltaTime, *this, i);
    }

    // determine vehicle placement
    SmallVec<u32> placements;
    for (u32 i=0; i<vehicles.size(); ++i)
    {
        placements.push_back(i);
    }
    f32 maxT = trackGraph.getStartNode()->t;
    std::sort(placements.begin(), placements.end(), [&](u32 a, u32 b) {
        return maxT - vehicles[a]->graphResult.currentLapDistance + vehicles[a]->currentLap * maxT >
               maxT - vehicles[b]->graphResult.currentLapDistance + vehicles[b]->currentLap * maxT;
    });
    for (u32 i=0; i<placements.size(); ++i)
    {
        vehicles[placements[i]]->placement = i;
    }

    // override placement with finish order for vehicles that have finished the race
    for (u32 i=0; i<finishOrder.size(); ++i)
    {
        vehicles[finishOrder[i]]->placement = i;
    }

    // debug text
#if 1
    Vehicle const& playerVehicle = *vehicles[0];
    game.resources.getFont("font", 23).drawText(str(
                "FPS: ", 1.f / deltaTime,
                "\nEngine RPM: ", playerVehicle.getEngineRPM(),
                "\nSpeed: ", playerVehicle.getForwardSpeed() * 3.6f,
                "\nGear: ", playerVehicle.getCurrentGear(),
                "\nProgress: ", playerVehicle.graphResult.currentLapDistance,
                "\nLow Mark: ", playerVehicle.graphResult.lapDistanceLowMark).c_str(), { game.windowWidth * 0.38f, 20 }, glm::vec3(1));
#endif

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

    if (game.input.isKeyPressed(KEY_F3))
    {
        trackGraphDebugVisualizationEnabled = !trackGraphDebugVisualizationEnabled;
    }

    if (trackGraphDebugVisualizationEnabled)
    {
        trackGraph.debugDraw();
    }

    // draw HUD track
    u32 arrowMesh = game.resources.getMesh("world.TrackArrow").renderHandle;
    SmallVec<RenderTextureItem, 16> dynamicItems;
    for (auto const& v : vehicles)
    {
        glm::vec3 pos = v->getPosition();
        dynamicItems.push_back({
            arrowMesh,
            trackOrtho * glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 2) + pos)
                * glm::rotate(glm::mat4(1.f), pointDirection(pos, pos + v->getForwardVector()) + f32(M_PI) * 0.5f, { 0, 0, 1 })
                * glm::scale(glm::mat4(1.f), glm::vec3(10.f)),
            glm::vec3(1, 0, 0)
        });
    }
    u32 size = (u32)(game.windowHeight * 0.22f);
    glm::vec2 hudTrackPos;
    if (viewportCount == 1) hudTrackPos = glm::vec2(size * 0.5f + 50.f) + glm::vec2(0, 30);
    else if (viewportCount == 2) hudTrackPos = glm::vec2(game.windowWidth - size * 0.5f - 60, game.windowHeight * 0.5f);
    else if (viewportCount == 3)
    {
        hudTrackPos = glm::vec2(game.windowWidth, game.windowHeight) * 0.75f;
        size = (u32)(game.windowHeight * 0.36f);
    }
    else if (viewportCount == 4) hudTrackPos = glm::vec2(game.windowWidth, game.windowHeight) * 0.5f;
    game.renderer.drawTrack2D(trackItems, dynamicItems, size, (u32)(size * trackAspectRatio), hudTrackPos);
}

void Scene::onEnd()
{
}

void Scene::attackCredit(u32 instigator, u32 victim)
{
    if (instigator == victim)
    {
        vehicles[instigator]->addNotification("ACCIDENT!");
    }
    else
    {
        vehicles[instigator]->addNotification("ATTACK BONUS!");
    }
}

bool Scene::raycastStatic(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit) const
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(COLLISION_FLAG_GROUND, 0, 0, 0);
    if (hit)
    {
        return physicsScene->raycast(convert(from), convert(dir), dist, *hit, PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
    else
    {
        PxRaycastBuffer tmpHit;
        return physicsScene->raycast(convert(from), convert(dir), dist, tmpHit, PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
}

bool Scene::raycast(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit)
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.flags |= PxQueryFlag::eDYNAMIC;
    filter.data = PxFilterData(COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS, 0, 0, 0);
    if (hit)
    {
        return physicsScene->raycast(convert(from), convert(dir), dist, *hit, PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
    else
    {
        PxRaycastBuffer tmpHit;
        return physicsScene->raycast(convert(from), convert(dir), dist, tmpHit, PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
}

bool Scene::sweepStatic(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxSweepBuffer* hit) const
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(COLLISION_FLAG_GROUND, 0, 0, 0);
    PxTransform initialPose(convert(from), PxQuat(PxIdentity));
    if (hit)
    {
        return physicsScene->sweep(PxSphereGeometry(radius), initialPose, convert(dir), dist, *hit, PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
    else
    {
        PxSweepBuffer tmpHit;
        return physicsScene->sweep(PxSphereGeometry(radius), initialPose, convert(dir), dist, tmpHit, PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
}

bool Scene::sweep(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxSweepBuffer* hit) const
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.flags |= PxQueryFlag::eDYNAMIC;
    filter.data = PxFilterData(COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS, 0, 0, 0);
    PxTransform initialPose(convert(from), PxQuat(PxIdentity));
    if (hit)
    {
        return physicsScene->sweep(PxSphereGeometry(radius), initialPose, convert(dir), dist, *hit, PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
    else
    {
        PxSweepBuffer tmpHit;
        return physicsScene->sweep(PxSphereGeometry(radius), initialPose, convert(dir), dist, tmpHit, PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
}
