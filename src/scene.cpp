#include "scene.h"
#include "game.h"
#include "vehicle.h"
#include "renderer.h"
#include "mesh_renderables.h"
#include "input.h"
#include "terrain.h"
#include "track.h"
#include "entities/static_mesh.h"
#include "entities/static_decal.h"
#include "entities/flash.h"
#include "entities/booster.h"
#include "gui.h"
#include "imgui.h"

Scene::Scene(TrackData* data)
{
    smoke.texture = g_res.getTexture("smoke");

    sparks.texture = g_res.getTexture("flash");
    sparks.alphaCurve = { {0.f, 1.f}, {1.f, 0.f} };
    sparks.scaleCurve = { {0.f, 1.f}, {1.f, 0.f} };
    sparks.minLife = 0.25f;
    sparks.maxLife = 0.35f;
    sparks.minScale = 0.3f;
    sparks.maxScale = 0.5f;
    sparks.lit = false;

    // create PhysX scene
    PxSceneDesc sceneDesc(g_game.physx.physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, 0.f, -15.f);
    sceneDesc.cpuDispatcher = g_game.physx.dispatcher;
    sceneDesc.filterShader = vehicleFilterShader;
    sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    sceneDesc.simulationEventCallback = this;
    sceneDesc.solverType = PxSolverType::eTGS;
    sceneDesc.broadPhaseType = PxBroadPhaseType::eABP;

    physicsScene = g_game.physx.physics->createScene(sceneDesc);

    if (PxPvdSceneClient* pvdClient = physicsScene->getScenePvdClient())
    {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    if (!data->data.hasValue())
    {
        print("Loading empty scene. Adding default entities.");
        addEntity(g_entities[0].create()); // terrain
        addEntity(g_entities[1].create()); // track
        addEntity(g_entities[4].create()); // starting point

        guid = data->guid;
        name = data->name;
    }
    else
    {
        Serializer s(data->data, true);
        serialize(s);
    }

    while (newEntities.size() > 0)
    {
        Array<OwnedPtr<Entity>> savedNewEntities = std::move(newEntities);
        for (auto& e : savedNewEntities)
        {
            e->setPersistent(true);
            e->onCreate(this);
        }
        for (auto& e : savedNewEntities)
        {
            e->onCreateEnd(this);
            entities.push_back(std::move(e));
        }
    }

    assert(track != nullptr);
    assert(start != nullptr);
    track->buildTrackGraph(&trackGraph, start->transform);
    trackPreviewPosition = start->position;
    trackPreviewCameraTarget = trackPreviewPosition;
    trackPreviewCameraFrom = trackPreviewPosition + glm::vec3(0, 0, 5);

    if (!g_game.isEditing)
    {
        buildBatches();
    }
}

Scene::~Scene()
{
    physicsScene->release();
    if (backgroundSound)
    {
        g_audio.stopSound(backgroundSound);
    }
}

void Scene::startRace()
{
    if (!this->start)
    {
        error("There is no starting point!");
        return;
    }
    glm::mat4 const& start = this->start->transform;

    u32 driversPerRow = 5;
    f32 width = 16 * scaleOf(this->start->transform).y;
    i32 cameraIndex = 0;

    track->buildTrackGraph(&trackGraph, start);

    // if no racing lines have been defined for the track generate them from the track graph
    if (paths.empty())
    {
        print("Scene has no paths defined. Generating paths.\n");
        hasGeneratedPaths = true;
        paths.reserve(trackGraph.getPaths().size());
        for (auto& path : trackGraph.getPaths())
        {
            RacingLine racingLine;
            racingLine.points.reserve(path.size());
            for (TrackGraph::Node* p : path)
            {
                RacingLine::Point point;
                point.position = p->position;
                racingLine.points.push_back(point);
            }
            paths.push_back(std::move(racingLine));
        }
    }
    for (auto& p : paths)
    {
        p.build(trackGraph);
    }
    //motionGrid.build(this);

    struct OrderedDriver
    {
        Driver* driver;
        i32 cameraIndex;
    };
    Array<OrderedDriver> createOrder;
    for (auto& driver : g_game.state.drivers)
    {
        createOrder.push_back({ &driver, driver.hasCamera ? cameraIndex : -1});
        if (driver.hasCamera)
        {
            ++cameraIndex;
        }
    }
    createOrder.sort([](auto& a, auto& b) { return a.driver->lastPlacement < b.driver->lastPlacement; });

#if 0
    u32 numVehicles = 2;
    if (createOrder.size() > numVehicles)
    {
        createOrder.resize(numVehicles);
    }
#endif

    numHumanDrivers = 0;
    for (u32 i=0; i<createOrder.size(); ++i)
    {
        OrderedDriver driverInfo = createOrder[i];
        if (driverInfo.driver->isPlayer)
        {
            ++numHumanDrivers;
        }

        //glm::vec3 offset = -glm::vec3(6 + i / 4 * 8, -7.5f + i % 4 * 5, 0.f);
        glm::vec3 offset = -glm::vec3(
                6 + i / driversPerRow * 8,
                -(width/2) + (i % driversPerRow) * (width / (driversPerRow - 1)),
                0.f);
        glm::vec3 zdir = glm::normalize(zAxisOf(start));

        PxRaycastBuffer hit;
        if (!raycastStatic(translationOf(glm::translate(start, offset + zdir * 10.f)),
                -zdir, 200.f, &hit))
        {
            error("The starting point is too high in the air!");
            stopRace();
            return;
        }

        VehicleTuning tuning = driverInfo.driver->getTuning();
        glm::mat4 vehicleTransform = glm::translate(glm::mat4(1.f),
                convert(hit.block.position + hit.block.normal *
                    tuning.getRestOffset())) * rotationOf(start);

        vehicles.push_back(new Vehicle(this, vehicleTransform, -offset,
            driverInfo.driver, std::move(tuning), i, driverInfo.cameraIndex));

        placements.push_back(i);
    }

    isRaceInProgress = true;
}

void Scene::buildBatches()
{
    f64 t = getTime();
    batcher.begin();
    for (auto& e : entities)
    {
        e->onBatch(batcher);
    }
    batcher.end();
    f64 timeTakenToBuildBatches = getTime() - t;
    print("Built ", batcher.batches.size(), " batches in ", timeTakenToBuildBatches, " seconds\n");
    isBatched = true;
}

void Scene::stopRace()
{
    trackPreviewCameraFrom = g_game.renderer->getRenderWorld()->getCamera(0).position;
    trackPreviewCameraTarget =
        vehicles.find([](auto& v) { return v->cameraIndex == 0; })->get()->getPosition();

    finishOrder.clear();
    placements.clear();
    vehicles.clear();
    isRaceInProgress = false;
    g_audio.setPaused(false);
    g_audio.stopAllGameplaySounds();
    readyToGo = false;
    allPlayersFinished = false;
    finishTimer = 0.f;
    smoke.clear();

    if (hasGeneratedPaths)
    {
        hasGeneratedPaths = false;
        paths.clear();
    }

    if (dragJoint)
    {
        dragJoint->release();
        dragJoint = nullptr;
    }

    for (auto& e : entities)
    {
        if (!e->isPersistent())
        {
            e->destroy();
        }
        else if (e->entityFlags & Entity::DYNAMIC)
        {
            ((PlaceableEntity*)e.get())->updateTransform(this);
        }
    }
}

void Scene::onStart()
{
    backgroundSound = g_audio.playSound(
            g_res.getSound("environment"), SoundType::MUSIC, true, 1.f, 0.f);
}

bool Scene::canGo() const
{
    return readyToGo || g_game.isEditing;
}

void Scene::onUpdate(Renderer* renderer, f32 deltaTime)
{
    TIMED_BLOCK();

    RenderWorld* rw = renderer->getRenderWorld();
    rw->setShadowBounds({}, false);
    rw->setClearColor(g_game.isEditing || g_game.isDebugCameraEnabled);

    bool showPauseMenu = g_input.isKeyPressed(KEY_ESCAPE);
    for (auto& pair : g_input.getControllers())
    {
        if (pair.second.isButtonPressed(BUTTON_START))
        {
            showPauseMenu = true;
            break;
        }
    }
    if ((!g_game.isEditing && isRaceInProgress) && showPauseMenu)
    {
        isPaused = !isPaused;
        g_audio.setPaused(isPaused);
    }

    u32 viewportCount = (!isRaceInProgress) ? 1 : (u32)std::count_if(g_game.state.drivers.begin(), g_game.state.drivers.end(),
            [](auto& d) { return d.hasCamera; });
    rw->setViewportCount(viewportCount);
    //rw->addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));
    // TODO: make light direction configurable in editor
    rw->addDirectionalLight(glm::vec3(0.2f, 0.5f, -0.8f), glm::vec3(1.0));

    if (!isPaused)
    {
        worldTime += deltaTime;
        rw->updateWorldTime(worldTime);

        if (worldTime >= 3.f)
        {
            readyToGo = true;
        }

        SmallArray<glm::vec3> listenerPositions;
        if (!g_game.isEditing && !isRaceInProgress && isCameraTourEnabled
                && trackGraph.getPaths().size() > 0)
        {
            // move the camera around the track
            auto path = trackGraph.getPaths()[0];
            glm::vec3 targetP = path[currentTrackPreviewPoint]->position;
            glm::vec3 diff = targetP - trackPreviewPosition;
            if (glm::length2(diff) < square(30.f))
            {
                currentTrackPreviewPoint = (currentTrackPreviewPoint + 1) % (u32)path.size();
            }
            f32 accel = 8.0f;
            trackPreviewVelocity += glm::normalize(diff) * deltaTime * accel;
            f32 maxSpeed = 15.f;
            if (glm::length(trackPreviewVelocity) > maxSpeed)
            {
                trackPreviewVelocity = glm::normalize(trackPreviewVelocity) * maxSpeed;
            }
            trackPreviewPosition += trackPreviewVelocity * deltaTime;

            f32 camDistance = 100.f;
            glm::vec3 cameraTarget = trackPreviewPosition;
            glm::vec3 cameraFrom = cameraTarget + glm::normalize(glm::vec3(1.f, 1.f, 1.25f)) * camDistance;

            trackPreviewCameraTarget = smoothMove(trackPreviewCameraTarget, cameraTarget, 5.f, deltaTime);
            trackPreviewCameraFrom = smoothMove(trackPreviewCameraFrom, cameraFrom, 5.f, deltaTime);

            rw->setViewportCamera(0, trackPreviewCameraFrom, trackPreviewCameraTarget, 25.f, 200.f);

            listenerPositions.push_back(cameraTarget);
        }

        // TODO: Use PhysX scratch buffer to reduce allocations
        if (isRaceInProgress)
        {
            physicsMouseDrag(renderer);
            physicsScene->simulate(deltaTime);
            physicsScene->fetchResults(true);
        }

        // update vehicles
        for (u32 i=0; i<vehicles.size(); ++i)
        {
            vehicles[i]->onUpdate(rw, deltaTime);
            if (vehicles[i]->cameraIndex >= 0)
            {
                listenerPositions.push_back(vehicles[i]->lastValidPosition);
            }
        }

        // update entities
        for (auto const& e : entities)
        {
            e->onUpdate(rw, this, deltaTime);
        }

        // determine vehicle placement
        if (vehicles.size() > 0)
        {
            if (canGo())
            {
                placements.clear();
                for (u32 i=0; i<vehicles.size(); ++i)
                {
                    placements.push_back(i);
                }
                f32 maxT = trackGraph.getStartNode()->t;
                placements.sort([&](u32 a, u32 b) {
                    return maxT - vehicles[a]->graphResult.currentLapDistance + vehicles[a]->currentLap * maxT >
                        maxT - vehicles[b]->graphResult.currentLapDistance + vehicles[b]->currentLap * maxT;
                });
                for (u32 i=0; i<placements.size(); ++i)
                {
                    vehicles[placements[i]]->placement = i;
                }
            }

            // override placement with finish order for vehicles that have finished the race
            for (u32 i=0; i<finishOrder.size(); ++i)
            {
                vehicles[finishOrder[i]]->placement = i;
            }
        }

        smoke.update(deltaTime);
        sparks.update(deltaTime);
        ribbons.update(deltaTime);

        g_audio.setListeners(listenerPositions);

        if (allPlayersFinished && isRaceInProgress)
        {
            finishTimer += deltaTime;
            if (finishTimer >= 4.f && !g_game.isEditing)
            {
                // TODO: Tell the player to press a button
                if (g_gui.didSelect())
                {
                    buildRaceResults();
                    stopRace();
                    g_game.menu.showRaceResults();
                }
            }
        }

        if ((g_game.isEditing && !isRaceInProgress) || g_game.isDebugCameraEnabled)
        {
            editorCamera.update(deltaTime, rw);
        }
    }

    // render vehicles
    for (u32 i=0; i<vehicles.size(); ++i)
    {
        vehicles[i]->onRender(rw, deltaTime);
        vehicles[i]->drawHUD(renderer, deltaTime);
    }

    // delete destroyed entities
    for (auto it = entities.begin(); it != entities.end();)
    {
        if ((*it)->isDestroyed())
        {
            it = entities.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // render entities
    for (auto const& e : entities)
    {
        e->onRender(rw, this, deltaTime);
    }

    // render the batches
    batcher.render(rw);

    // handle newly created entities
    for (auto& e : newEntities)
    {
        e->onCreate(this);
    }
    for (auto& e : newEntities)
    {
        e->onCreateEnd(this);
        entities.push_back(std::move(e));
    }
    newEntities.clear();

    if (g_game.isPhysicsDebugVisualizationEnabled)
    {
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 2.0f);
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eBODY_MASS_AXES, 1.0f);
        //physicsScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 2.0f);
        //physicsScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL, 2.0f);
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);

        Camera const& cam = rw->getCamera(0);
        BoundingBox bb = computeCameraFrustumBoundingBox(glm::inverse(cam.viewProjection));
        debugDraw.boundingBox(bb, glm::mat4(1.f), glm::vec4(1.f));

        physicsScene->setVisualizationCullingBox(PxBounds3(convert(bb.min), convert(bb.max)));

        const PxRenderBuffer& rb = physicsScene->getRenderBuffer();
        for(PxU32 i=0; i < rb.getNbLines(); ++i)
        {
            const PxDebugLine& line = rb.getLines()[i];
            debugDraw.line(convert(line.pos0), convert(line.pos1), rgbaFromU32(line.color0), rgbaFromU32(line.color1));
        }
    }
    else
    {
        physicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.0f);
    }

    if (g_game.isTrackGraphDebugVisualizationEnabled)
    {
        trackGraph.debugDraw(&debugDraw, renderer);
    }

    if (g_game.isMotionGridDebugVisualizationEnabled)
    {
        //motionGrid.debugDraw(rw);
    }

    if (g_game.isPathVisualizationEnabled)
    {
        for (auto& path : paths)
        {
            glm::vec4 color(1.f, 1.f, 0.f, 1.f);
            for (u32 i=1; i<path.points.size(); ++i)
            {
                debugDraw.line(path.points[i-1].position, path.points[i].position, color, color);
            }
            debugDraw.line(path.points.back().position, path.points.front().position, color, color);
        }
    }

    rw->add(&ribbons);
    rw->add(&smoke);
    rw->add(&sparks);
    rw->add(&debugDraw);

    // draw HUD track
    if (isRaceInProgress)
    {
        u32 size = (u32)(g_game.windowHeight * 0.39f * g_game.config.gameplay.hudTrackScale);
        glm::vec2 hudTrackPos;
        if (viewportCount == 1)
        {
            hudTrackPos = glm::vec2(size * 0.5f + 15.f) + glm::vec2(0, 15);
        }
        else if (viewportCount == 2)
        {
            hudTrackPos = glm::vec2(g_game.windowWidth * 0.5f, size * 0.5f + 15.f);
        }
        /*
        else if (viewportCount == 2)
        {
            hudTrackPos = glm::vec2(size * 0.5f + 60, g_game.windowHeight * 0.5f);
        }
        */
        else if (viewportCount == 3)
        {
            hudTrackPos = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.75f;
        }
        else if (viewportCount == 4)
        {
            hudTrackPos = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        }

        drawTrackPreview(renderer, size, hudTrackPos);
        if (!(isPaused && renderer->getRenderWorld()->getViewportCount() == 4))
        {
            renderer->add2D(&trackPreview2D);
        }
    }

    if (isPaused)
    {
        // pause menu
        g_gui.beginPanel("PAUSED",
                glm::vec2(g_game.windowWidth * 0.5f, g_game.windowHeight * 0.35f),
                0.5f, true, true);
        if (g_gui.button("Resume"))
        {
            isPaused = false;
            g_audio.setPaused(false);
        }
        if (g_gui.button("Forfeit Race"))
        {
            isPaused = false;
            g_game.isEditing = false;
            if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
            {
                buildRaceResults();
                stopRace();
                g_game.menu.showRaceResults();
            }
            else
            {
                stopRace();
                g_game.menu.showMainMenu();
            }
        }
        if (g_gui.button("Quit to Desktop"))
        {
            g_game.shouldExit = true;
        }
        g_gui.end();
    }

    if (backgroundSound)
    {
        f32 volume = (g_game.isEditing && !isRaceInProgress) ? 0.f : 1.f;
        g_audio.setSoundVolume(backgroundSound, volume);
    }
}

void Scene::physicsMouseDrag(Renderer* renderer)
{
    static glm::vec3 dragPlaneOffset;
    static Vehicle* dragVehicle = nullptr;
    static f32 previousLinearDamping;
    static f32 previousAngularDamping;
    static PxRigidDynamic* dragActor = nullptr;
    static PxVec3 dragActorVelocity;

    // drag objects around with mouse when debug camera is enabled
    if (g_game.isDebugCameraEnabled && isRaceInProgress)
    {
        if (dragJoint && dragVehicle)
        {
            if (dragVehicle->isDead())
            {
                dragActor->setLinearDamping(previousLinearDamping);
                dragActor->setAngularDamping(previousAngularDamping);
                dragJoint->release();
                dragJoint = nullptr;
                dragVehicle = nullptr;
            }
            else
            {
                dragVehicle->restoreHitPoints();
            }
        }

        glm::vec3 rayDir = editorCamera.getMouseRay(renderer->getRenderWorld());
        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            PxRaycastBuffer hit;
            if (raycast(editorCamera.getCameraFrom(), rayDir, 1000.f, &hit))
            {
                if (hit.block.actor->getType() == PxActorType::eRIGID_DYNAMIC)
                {
                    dragPlaneOffset = convert(hit.block.position) - editorCamera.getCameraFrom();
                    PxTransform localFrame(PxIdentity);
                    localFrame.p = hit.block.actor->getGlobalPose().transformInv(hit.block.position);
                    PxTransform globalFrame(PxIdentity);
                    globalFrame.p = hit.block.position;
                    dragJoint = PxDistanceJointCreate(*g_game.physx.physics, hit.block.actor,
                            localFrame, nullptr, globalFrame);
                    dragJoint->setMaxDistance(0.5f);
                    dragActor = (PxRigidDynamic*)hit.block.actor;
                    previousLinearDamping = dragActor->getLinearDamping();
                    previousAngularDamping = dragActor->getAngularDamping();
                    dragActorVelocity = dragActor->getLinearVelocity();
                    dragActor->setLinearDamping(5.f);
                    dragActor->setAngularDamping(5.f);
                    dragActor->wakeUp();
                    ActorUserData* userData = (ActorUserData*)hit.block.actor->userData;
                    if (userData && userData->entityType == ActorUserData::VEHICLE)
                    {
                        dragVehicle = userData->vehicle;
                    }
                }
            }
        }

        if (g_input.isMouseButtonDown(MOUSE_LEFT))
        {
            if (dragJoint)
            {
                f32 hitDist = rayPlaneIntersection(editorCamera.getCameraFrom(), rayDir,
                        -rayDir, editorCamera.getCameraFrom() + dragPlaneOffset);
                glm::vec3 hitPoint = editorCamera.getCameraFrom() + rayDir * hitDist;
                PxTransform globalFrame(PxIdentity);
                globalFrame.p = convert(hitPoint);
                dragJoint->setLocalPose(PxJointActorIndex::eACTOR1, globalFrame);
            }
        }

        if (g_input.isMouseButtonReleased(MOUSE_LEFT))
        {
            if (dragJoint)
            {
                dragJoint->release();
                dragJoint = nullptr;
                dragActor->setLinearVelocity(dragActorVelocity);
                dragActor->setLinearDamping(previousLinearDamping);
                dragActor->setAngularDamping(previousAngularDamping);
            }
        }
    }

    if (dragActor)
    {
        dragActorVelocity = dragActor->getLinearVelocity();
    }
}

void Scene::onEndUpdate()
{
    ribbons.endUpdate();
}

void Scene::vehicleFinish(u32 n)
{
    finishOrder.push_back(n);
    u32 playerFinishCount = 0;
    for (auto& v : vehicles)
    {
        if (v->driver->isPlayer && v->finishedRace)
        {
            ++playerFinishCount;
        }
    }
    if (numHumanDrivers == playerFinishCount && !allPlayersFinished)
    {
        allPlayersFinished = true;
    }

    auto& v = vehicles[n];
    g_audio.playSound3D(v->placement < 5 ? g_res.getSound("cheer") : g_res.getSound("clapping"),
            SoundType::GAME_SFX, translationOf(getStart()));
}

void Scene::drawTrackPreview(Renderer* renderer, u32 size, glm::vec2 hudTrackPos)
{
    BoundingBox bb = track->getBoundingBox();

#if 1
    if (!hasTrackPreview)
    {
        hasTrackPreview = true;

        glm::vec3 bbCenter = (bb.min + bb.max) * 0.5f;
        glm::vec3 dir = glm::normalize(glm::vec3(1.f));
        f32 dist = 850.f;
        glm::vec3 camPosition;
        glm::mat4 viewProjection;
        for (u32 i=1; i<150; ++i)
        {
            camPosition = (bbCenter - glm::vec3(0, 0, 20)) + dir * dist;
            glm::mat4 view = glm::lookAt(camPosition, bbCenter, glm::vec3(0, 0, 1));
            glm::mat4 projection = glm::perspective(glm::radians(26.f), 1.f, 1.f, 2500.f);
            viewProjection = projection * view;

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
                glm::vec4 tp = viewProjection * glm::vec4(p, 1.f);
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
            dist += 10.f;
        }
        trackPreview2D.setCamPosition(camPosition);
        trackPreview2D.setCamViewProjection(viewProjection);
    }
    trackPreview2D.beginUpdate(renderer, size, size);

    //Mesh* startMesh = &g_res.getModel("misc")->meshes.front();
    Mesh* startMesh = &g_res.getModel("HUDStart")->meshes.back();
    trackPreview2D.drawItem(
        startMesh->vao, startMesh->numIndices, start->transform, glm::vec3(1.f), false);

    Mesh* trackMesh = track->getPreviewMesh(this);
    trackPreview2D.drawItem(trackMesh->vao, (u32)trackMesh->numIndices, glm::mat4(1.f),
            glm::vec3(1.f), true, 1);
    trackPreview2D.drawItem(trackMesh->vao, (u32)trackMesh->numIndices,
            glm::translate(glm::mat4(1.f), { 0, 0, -5 }), glm::vec3(0.2f), true, 0);
    /*
    for (u32 i=0; i<20; ++i)
    {
        trackPreview2D.drawItem(trackMesh->vao, (u32)trackMesh->numIndices,
                glm::translate(glm::mat4(1.f),
                    { 0, 0, track->getBoundingBox().min.z - 7.f + (i/20.f * 7.f)})
                    * glm::scale(glm::mat4(1.f), { 1, 1, i/20.f }), glm::vec3(0.18f), true, 0);
    }
    */

    Mesh* sphereMesh = &g_res.getModel("sphere")->meshes.front();
    //Mesh* cubeMesh = &g_res.getModel("HUDCar")->meshes.front();
    for (auto const& v : vehicles)
    {
        trackPreview2D.drawItem(sphereMesh->vao, sphereMesh->numIndices,
            glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 2.1f) + v->getPosition())
                * glm::scale(glm::mat4(1.f), glm::vec3(5.1f)),
            g_vehicleColors[v->getDriver()->getVehicleConfig()->colorIndex], false, 2);
        /*
        trackPreview2D.drawItem(cubeMesh->vao, cubeMesh->numIndices, v->getTransform(),
            g_vehicleColors[v->getDriver()->getVehicleConfig()->colorIndex], false, 0);
        */
    }
#else

    f32 pad = 20.f;
    bb.min.x -= pad;
    bb.min.y -= pad;
    bb.max.x += pad;
    bb.max.y += pad;

    glm::vec3 offset = -(bb.min + (bb.max - bb.min) * 0.5f);

    f32 rot = PI * 0.25f;
    glm::mat4 transform = glm::rotate(glm::mat4(1.f), f32(rot), { 0, 0, 1 })
            * glm::translate(glm::mat4(1.f), offset);
    bb = bb.transform(transform);
    f32 radius = glm::max(bb.max.x, glm::max(bb.max.y, bb.max.z));
    bb.min = glm::vec3(-radius);
    bb.max = glm::vec3(radius);

    glm::mat4 trackOrtho = glm::ortho(bb.max.x, bb.min.x, bb.min.y,
                bb.max.y, -bb.max.z - 10.f, -bb.min.z + 10.f) * transform;

    trackPreview2D.setCamViewProjection(trackOrtho);
    trackPreview2D.beginUpdate(renderer, size, size);

    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
    trackPreview2D.drawItem(
        quadMesh->vao, quadMesh->numIndices,
        start->transform * glm::translate(glm::mat4(1.f), { 0, 0, -2 })
            * glm::scale(glm::mat4(1.f), { 4, 24, 1 }), glm::vec3(0.03f), true);

    Mesh* trackMesh = track->getPreviewMesh(this);
    trackPreview2D.drawItem(trackMesh->vao, (u32)trackMesh->numIndices, glm::mat4(1.f),
            glm::vec3(1.f), true, 1);

    Mesh* mesh = g_res.getModel("misc")->getMeshByName("world.TrackArrow");
    for (auto const& v : vehicles)
    {
        glm::vec3 pos = v->getPosition();
        trackPreview2D.drawItem(mesh->vao, mesh->numIndices,
            glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 2 + v->vehicleIndex*0.01) + pos)
                * glm::rotate(glm::mat4(1.f), pointDirection(pos, pos + v->getForwardVector()) + f32(M_PI) * 0.5f, { 0, 0, 1 })
                * glm::scale(glm::mat4(1.f), glm::vec3(10.f)),
            g_vehicleColors[v->getDriver()->getVehicleConfig()->colorIndex], false, 0);
    }
#endif

    trackPreview2D.endUpdate(hudTrackPos);
}

void Scene::writeTrackData()
{
    DataFile::Value data = DataFile::makeDict();
    Serializer s(data, false);
    serialize(s);
    g_res.getTrackData(guid)->data = std::move(data);
}

void Scene::onEnd()
{
    if (g_game.isEditing)
    {
        writeTrackData();
    }
}

void Scene::attackCredit(u32 instigator, u32 victim)
{
    if (!vehicles[victim]->finishedRace)
    {
        if (instigator == victim)
        {
            ++vehicles[instigator]->raceStatistics.accidents;
            vehicles[instigator]->addNotification("ACCIDENT!");
        }
        else
        {
            ++vehicles[victim]->raceStatistics.destroyed;
            ++vehicles[instigator]->raceStatistics.frags;
            vehicles[instigator]->addBonus("ATTACK BONUS", ATTACK_BONUS_AMOUNT, glm::vec3(1.f));
            std::string instigatorName = vehicles[instigator]->driver->playerName;
            for (auto& ch : instigatorName)
            {
                ch = toupper(ch);
            }
            vehicles[victim]->addNotification("DESTROYED", 2.f, glm::vec3(1.f, 0.3f, 0.02f));
        }
    }
}

void Scene::buildRaceResults()
{
    u32 numDriversStillDriving = 0;
    for (auto& v : vehicles)
    {
        if (!v->finishedRace)
        {
            ++numDriversStillDriving;
        }
    }

    for (auto& v : vehicles)
    {
        RaceStatistics& stats = v->raceStatistics;
        if (!v->finishedRace)
        {
            if (v->driver->isPlayer)
            {
                print("Player did not finish: ", v->currentLap, "/", totalLaps, " laps\n");
                v->placement = 9999;
            }
            else
            {
                auto& ai = g_ais[v->driver->aiIndex];
                print("AI did not finish: ", v->currentLap, "/", totalLaps, " laps\n");
                // generate fake stats for the drivers that have not yet finished the race
                for (u32 i = v->currentLap + 1; i<totalLaps; ++i)
                {
                    stats.accidents += irandom(randomSeries, 0, 2);
                    u32 attackBonuses = irandom(randomSeries, 0,
                            (u32)(numDriversStillDriving * ai.aggression * 0.5f));
                    stats.frags += attackBonuses;
                    stats.bonuses.push_back({ "", attackBonuses * ATTACK_BONUS_AMOUNT });
                    while (attackBonuses > 0)
                    {
                        auto& v2 = vehicles[irandom(randomSeries, 0, (i32)vehicles.size())];
                        if (v2.get() != v.get())
                        {
                            ++v2->raceStatistics.destroyed;
                            --attackBonuses;
                        }
                    }
                }
                v->placement = v->placement +
                    (u32)(random(randomSeries, 0.f, ai.drivingSkill) +
                            random(randomSeries, 0.f, ai.awareness) +
                            random(randomSeries, 0.f, ai.aggression));
            }
        }
    }
    for (auto& v : vehicles)
    {
        raceResults.push_back({
            v->placement,
            v->driver,
            std::move(v->raceStatistics),
            v->placement != 9999
        });
    }
    raceResults.sort([](auto& a, auto&b) { return a.placement < b.placement; });
    for (u32 i=0; i<(u32)raceResults.size(); ++i)
    {
        raceResults[i].placement = i;
    }
}

void Scene::applyAreaForce(glm::vec3 const& position, f32 strength) const
{
    for (auto& v : vehicles)
    {
        f32 dist = glm::distance(v->getPosition(), position);
        f32 radius = strength * 1.5f;
        if (dist < radius)
        {
            v->shakeScreen(glm::pow(
                        glm::clamp(1.f - dist / radius, 0.f, 1.f), 0.5f) * radius);
        }
    }
    // TODO: apply force to nearby physics bodies
}

void Scene::createExplosion(glm::vec3 const& position, glm::vec3 const& velocity, f32 strength)
{
    applyAreaForce(position, strength);
    for (u32 i=0; i<8; ++i)
    {
        f32 s = random(randomSeries, 0.3f, 0.9f);
        f32 size = strength * 0.1f;
        glm::vec3 offset = glm::vec3(
            random(randomSeries, -1.f, 1.f),
            random(randomSeries, -1.f, 1.f),
            random(randomSeries, -1.f, 1.f)) * size;
        glm::vec3 vel = glm::normalize(glm::vec3(
            random(randomSeries, -1.f, 1.f),
            random(randomSeries, -1.f, 1.f),
            random(randomSeries, -1.f, 1.f))) * random(randomSeries, 0.75f, 1.5f)
            + glm::vec3(0, 0, random(randomSeries, 2.f, 3.f)) + velocity * 0.9f;
        smoke.spawn(position + offset, vel, 1.f, glm::vec4(s, s, s, 1.f), random(randomSeries, 3.f, 5.f));
    }
    for (u32 i=0; i<8; ++i)
    {
        glm::vec3 vel = velocity + glm::normalize(glm::vec3(
            random(randomSeries, -0.5f, 0.5f),
            random(randomSeries, -0.5f, 0.5f),
            random(randomSeries, -0.25f, 0.75f))) * random(randomSeries, 5.f, 12.f);
        sparks.spawn(position, vel, 1.f,
                glm::vec4(glm::vec3(1.f, random(randomSeries, 0.55f, 0.7f), 0.02f), 1.f) * 2.f);
    }

    addEntity(new Flash(position + glm::vec3(0, 0, 1.f), velocity * 0.8f, strength * 0.5f));
}

bool Scene::raycastStatic(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit, u32 flags) const
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(flags, 0, 0, 0);
    if (hit)
    {
        return physicsScene->raycast(convert(from), convert(dir), dist, *hit,
                PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
    else
    {
        PxRaycastBuffer tmpHit;
        return physicsScene->raycast(convert(from), convert(dir), dist, tmpHit,
                PxHitFlags(PxHitFlag::eDEFAULT), filter);
    }
}

bool Scene::raycast(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit)
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.flags |= PxQueryFlag::eDYNAMIC;
    filter.data = PxFilterData(COLLISION_FLAG_TERRAIN | COLLISION_FLAG_OBJECT
            | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_DYNAMIC, 0, 0, 0);
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

bool Scene::sweepStatic(f32 radius, glm::vec3 const& from, glm::vec3 const& dir,
        f32 dist, PxSweepBuffer* hit, u32 flags) const
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(flags, 0, 0, 0);
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

class IgnoreActor : public PxQueryFilterCallback
{
    PxRigidActor* ignoreActor;

public:
    IgnoreActor(PxRigidActor* ignoreActor) : ignoreActor(ignoreActor) {}

    PxQueryHitType::Enum preFilter(const PxFilterData& filterData, const PxShape* shape,
            const PxRigidActor* actor, PxHitFlags& queryFlags) override
    {
        if (actor == ignoreActor) return PxQueryHitType::eNONE;
        return PxQueryHitType::eBLOCK;
    }

    PxQueryHitType::Enum postFilter(const PxFilterData& filterData, const PxQueryHit& hit) override
    {
        return PxQueryHitType::eBLOCK;
    }
};

bool Scene::sweep(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist,
        PxSweepBuffer* hit, PxRigidActor* ignore, u32 flags) const
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.flags |= PxQueryFlag::eDYNAMIC;
    if (ignore)
    {
        filter.flags |= PxQueryFlag::ePREFILTER;
    }
    IgnoreActor cb(ignore);
    filter.data = PxFilterData(flags, 0, 0, 0);
    PxTransform initialPose(convert(from), PxQuat(PxIdentity));
    if (hit)
    {
        return physicsScene->sweep(PxSphereGeometry(radius), initialPose, convert(dir), dist,
                *hit, PxHitFlags(PxHitFlag::eDEFAULT), filter, &cb);
    }
    else
    {
        PxSweepBuffer tmpHit;
        return physicsScene->sweep(PxSphereGeometry(radius), initialPose, convert(dir), dist,
                tmpHit, PxHitFlags(PxHitFlag::eDEFAULT), filter, &cb);
    }
}

void Scene::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
    PxContactPairPoint contactPoints[64];

    for(u32 i=0; i<nbPairs; ++i)
    {
        u32 contactCount = pairs[i].contactCount;
        assert(contactCount < ARRAY_SIZE(contactPoints));

        if(contactCount == 0)
        {
            continue;
        }
        pairs[i].extractContacts(contactPoints, contactCount);

        for(u32 j=0; j<contactCount; ++j)
        {
            f32 magnitude = contactPoints[j].impulse.magnitude();
            if (magnitude > 1350.f)
            {
                ActorUserData* a = (ActorUserData*)pairHeader.actors[0]->userData;
                ActorUserData* b = (ActorUserData*)pairHeader.actors[1]->userData;
                f32 damage = glm::min(magnitude * 0.00058f, 100.f);

                // apply damage
                if (a && a->entityType == ActorUserData::VEHICLE)
                {
                    f32 myDamage = damage;
                    if (b && b->entityType == ActorUserData::VEHICLE)
                    {
                        if (b->vehicle->hasAbility("Ram Booster"))
                        {
                            myDamage *= 2.75f;
                            // TODO: Shouldn't minDamage be multiplied by deltaTime?
                            f32 minDamage = 5.f;
                            myDamage = std::max(myDamage, minDamage);
                        }
                        if (a->vehicle->hasAbility("Ram Booster"))
                        {
                            myDamage *= 0.75f;
                        }
                    }
                    u32 instigator = (b && b->entityType == ActorUserData::VEHICLE)
                        ? b->vehicle->vehicleIndex : a->vehicle->vehicleIndex;
                    a->vehicle->applyDamage(myDamage, instigator);
                    if (damage > 5.f)
                    {
                        a->vehicle->shakeScreen(damage * 0.35f);
                    }
                }
                if (b && b->entityType == ActorUserData::VEHICLE)
                {
                    f32 myDamage = damage;
                    if (a && a->entityType == ActorUserData::VEHICLE)
                    {
                        if (a->vehicle->hasAbility("Ram Booster"))
                        {
                            myDamage *= 2.75f;
                            // TODO: test this out
                            myDamage = std::max(myDamage, 5.f);
                        }
                        if (b->vehicle->hasAbility("Ram Booster"))
                        {
                            myDamage *= 0.75f;
                        }
                    }
                    u32 instigator = (a && a->entityType == ActorUserData::VEHICLE)
                        ? a->vehicle->vehicleIndex : b->vehicle->vehicleIndex;
                    b->vehicle->applyDamage(myDamage, instigator);
                    if (damage > 5.f)
                    {
                        b->vehicle->shakeScreen(damage * 0.35f);
                    }
                }

                g_audio.playSound3D(g_res.getSound("impact"),
                        SoundType::GAME_SFX, convert(contactPoints[j].position));
            }

            PxMaterial* materialA = pairs[i].shapes[0]->getMaterialFromInternalFaceIndex(
                    contactPoints[j].internalFaceIndex0);
            PxMaterial* materialB = pairs[i].shapes[1]->getMaterialFromInternalFaceIndex(
                    contactPoints[j].internalFaceIndex1);
            if (magnitude > 20.f &&
                    materialA != g_game.physx.materials.offroad &&
                    materialB != g_game.physx.materials.offroad)
                    // && pairs[i].events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
            {
                glm::vec3 velOffset = glm::vec3(
                    random(randomSeries, -0.25f, 0.25f),
                    random(randomSeries, -0.25f, 0.25f),
                    random(randomSeries, 1.f, 2.f));

                PxVec3 velA = pairHeader.actors[0]->getConcreteType() == PxConcreteType::eRIGID_DYNAMIC ?
                    pairHeader.actors[0]->is<PxRigidDynamic>()->getLinearVelocity() : PxVec3(0, 0, 0);
                PxVec3 velB = pairHeader.actors[1]->getConcreteType() == PxConcreteType::eRIGID_DYNAMIC ?
                    pairHeader.actors[1]->is<PxRigidDynamic>()->getLinearVelocity() : PxVec3(0, 0, 0);
                PxVec3 velocityDiff = velA - velB;
                f32 magnitude = velocityDiff.magnitude();
                f32 minMagnitude = 10.f;
                if (magnitude > minMagnitude)
                {
                    f32 alpha = glm::min((magnitude - minMagnitude) * 0.25f, 1.f);
                    glm::vec3 collisionVelocity = convert(velA + velB) * 0.5f;
                    sparks.spawn(
                            convert(contactPoints[j].position),
                            (convert(contactPoints[j].normal) + velOffset)
                                * random(randomSeries, 4.f, 5.f) + collisionVelocity * 0.4f, 1.f,
                            glm::vec4(glm::vec3(1.f, random(randomSeries, 0.55f, 0.7f), 0.02f) * 2.f, alpha));
                }
            }
        }
    }
}

void Scene::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
    for (u32 i=0; i<count; ++i)
    {
        if (pairs[i].flags & (PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER
                    | PxTriggerPairFlag::eREMOVED_SHAPE_OTHER))
        {
            continue;
        }

        ActorUserData* userData = (ActorUserData*)pairs[i].triggerShape->userData;
        ActorUserData* otherUserData = (ActorUserData*)pairs[i].otherShape->userData;
        if (!userData)
        {
            userData = (ActorUserData*)pairs[i].triggerActor->userData;
        }
        if (!otherUserData)
        {
            otherUserData = (ActorUserData*)pairs[i].otherActor->userData;
        }

        if (userData && otherUserData)
        {
            if (userData->entityType == ActorUserData::ENTITY
                    || userData->entityType == ActorUserData::SELECTABLE_ENTITY)
            {
                userData->entity->onTrigger(otherUserData);
            }
            if (otherUserData->entityType == ActorUserData::VEHICLE)
            {
                otherUserData->vehicle->onTrigger(userData);
            }
        }
    }
}

void Scene::serialize(Serializer& s)
{
    s.write("type", ResourceType::TRACK);
    s.field(guid);
    s.field(name);
    s.field(notes);
    s.field(totalLaps);
    s.field(version);
    s.field(paths);

    if (s.deserialize)
    {
        auto& entityArray = s.dict["entities"].array(true).val();
        /*
        if (version == 0)
        {
            // do something different
        }
        */
        for (auto& val : entityArray)
        {
            deserializeEntity(val);
        }
    }
    else
    {
        s.dict["entities"] = DataFile::makeArray();
        auto& entityArray = s.dict["entities"].array().val();
        for (auto& entity : this->entities)
        {
            if (entity->entityFlags & Entity::PERSISTENT)
            {
                auto dict = DataFile::makeDict();
                Serializer s(dict, false);
                entity->serialize(s);
                entityArray.push_back(std::move(dict));
            }
        }
    }
}

Entity* Scene::deserializeEntity(DataFile::Value& val)
{
    i32 entityID = (i32)val.dict(true).val()["entityID"].integer().val();
    Serializer s(val, true);
    Entity* entity = g_entities[entityID].create();
    entity->serializeState(s);
    this->addEntity(entity);
    return entity;
}

Array<DataFile::Value> Scene::serializeTransientEntities()
{
    Array<DataFile::Value> transientEntities;
    for (auto& entity : this->entities)
    {
        if (entity->entityFlags & Entity::TRANSIENT)
        {
            auto dict = DataFile::makeDict();
            Serializer s(dict, false);
            entity->serialize(s);
            transientEntities.push_back(std::move(dict));
        }
    }
    return transientEntities;
}

void Scene::deserializeTransientEntities(Array<DataFile::Value>& entities)
{
    for (auto it = this->entities.begin(); it != this->entities.end();)
    {
        if ((*it)->entityFlags & Entity::TRANSIENT)
        {
            it = this->entities.erase(it);
        }
        else
        {
            ++it;
        }
    }
    for (auto& e : entities)
    {
        deserializeEntity(e)->setPersistent(true);
    }
}

void Scene::showDebugInfo()
{
    ImGui::Gap();
    ImGui::Text("Scene Name: %s", name.c_str());
    ImGui::Text("Entities: %i", entities.size());
    ImGui::Text("Generated Paths: %s", hasGeneratedPaths ? "true" : "false");
    ImGui::Text("World Time: %.4f", worldTime);
    if (auto playerVehicle = vehicles.find([](auto& v) { return v->driver->isPlayer; }))
    {
        ImGui::Gap();
        (*playerVehicle)->showDebugInfo();
    }
}

glm::vec3 Scene::findValidPosition(glm::vec3 const& pos, f32 collisionRadius, f32 checkRadius)
{
    PxTransform transform(PxIdentity);
    transform.p = convert(pos);
    PxSphereGeometry geometry(collisionRadius);

    PxOverlapBuffer overlapHit;
    PxRaycastBuffer raycastHit;

    PxQueryFilterData filterObstacles;
    filterObstacles.flags |= PxQueryFlag::eANY_HIT | PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;
    filterObstacles.data = PxFilterData(
            COLLISION_FLAG_OBJECT | COLLISION_FLAG_CHASSIS | COLLISION_FLAG_MINE, 0, 0, 0);

    PxQueryFilterData filterTrack;
    filterTrack.flags |= PxQueryFlag::eANY_HIT | PxQueryFlag::eSTATIC;
    filterTrack.data = PxFilterData(COLLISION_FLAG_TRACK, 0, 0, 0);

    PxVec3 offsets[] = {
        PxVec3(1, 0, 0),
        PxVec3(-1, 0, 0),
        PxVec3(0, 1, 0),
        PxVec3(0, -1, 0)
    };

    for (u32 i=0; i<10; ++i)
    {
        if (!physicsScene->overlap(geometry, transform, overlapHit, filterObstacles))
        {
            bool onTrack = true;
            for (PxVec3& offset : offsets)
            {
                if (!physicsScene->raycast(transform.p + offset + PxVec3(0, 0, 5.f), PxVec3(0, 0, -1), 10.f, raycastHit,
                        PxHitFlags(PxHitFlag::eMESH_ANY), filterTrack))
                {
                    onTrack = false;
                    break;
                }
            }
            if (onTrack)
            {
                return convert(transform.p);
            }
        }

        glm::vec3 offset(
                random(randomSeries, -checkRadius, checkRadius),
                random(randomSeries, -checkRadius, checkRadius), 0.f);
        transform.p = convert(pos + offset);
    }

    return pos;
}
