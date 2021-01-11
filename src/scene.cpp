#include "scene.h"
#include "game.h"
#include "vehicle.h"
#include "renderer.h"
#include "input.h"
#include "terrain.h"
#include "track.h"
#include "entities/static_mesh.h"
#include "entities/static_decal.h"
#include "entities/flash.h"
#include "entities/booster.h"
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
        println("Loading empty scene. Adding default entities.");
        addEntity(g_entities[0].create(0)); // terrain
        addEntity(g_entities[1].create(1)); // track
        addEntity(g_entities[4].create(4)); // starting point

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
        Array<OwnedPtr<Entity>> savedNewEntities = move(newEntities);
        for (auto& e : savedNewEntities)
        {
            e->setPersistent(true);
            e->onCreate(this);
        }
        for (auto& e : savedNewEntities)
        {
            e->onCreateEnd(this);
            entities.push(move(e));
        }
    }

    assert(track != nullptr);
    assert(start != nullptr);
    track->buildTrackGraph(&trackGraph, start->transform);
    trackPreviewPosition = start->position;
    trackPreviewCameraTarget = trackPreviewPosition;
    trackPreviewCameraFrom = trackPreviewPosition + normalize(Vec3(1.f, 1.f, 1.25f)) * 15.f;

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
    Mat4 const& start = this->start->transform;

    u32 driversPerRow = 5;
    f32 width = 16 * this->start->transform.scale().y;
    i32 cameraIndex = 0;

    track->buildTrackGraph(&trackGraph, start);

    // if no racing lines have been defined for the track generate them from the track graph
    if (paths.empty())
    {
        println("Scene has no paths defined. Generating paths.");
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
                racingLine.points.push(point);
            }
            paths.push(move(racingLine));
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
        createOrder.push({ &driver, driver.hasCamera ? cameraIndex : -1});
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

        //Vec3 offset = -Vec3(6 + i / 4 * 8, -7.5f + i % 4 * 5, 0.f);
        f32 distanceBehindStartingLine = 4.f;
        Vec3 offset = -Vec3(
                distanceBehindStartingLine + i / driversPerRow * 7.5f,
                -(width/2) + (i % driversPerRow) * (width / (driversPerRow - 1)),
                0.f);
        Vec3 zdir = normalize(start.zAxis());

        PxRaycastBuffer hit;
        if (!raycastStatic((start * Mat4::translation(offset + zdir * 10.f)).position(),
                -zdir, 200.f, &hit))
        {
            error("The starting point is too high in the air!");
            stopRace();
            return;
        }

        VehicleTuning tuning = driverInfo.driver->getTuning();
        Mat4 vehicleTransform = Mat4::translation(
                hit.block.position + hit.block.normal * tuning.getRestOffset()) * start.rotation();

        vehicles.push(new Vehicle(this, vehicleTransform, -offset,
            driverInfo.driver, move(tuning), i, driverInfo.cameraIndex));

        placements.push(i);
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
    println("Built %u batches in %.2f seconds", batcher.batches.size(), timeTakenToBuildBatches);
    isBatched = true;
}

void Scene::stopRace()
{
    // TODO: fix this
    trackPreviewCameraFrom = g_game.renderer->getRenderWorld()->getCamera(0).position;
    trackPreviewCameraTarget =
        vehicles.findIf([](auto& v) { return v->cameraIndex == 0; })->get()->getPosition();

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
        else if ((e->entityFlags & EntityFlags::DYNAMIC) == EntityFlags::DYNAMIC)
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

void Scene::onUpdate(Renderer* renderer, f32 deltaTime)
{
    TIMED_BLOCK();

    RenderWorld* rw = renderer->getRenderWorld();
    rw->setShadowBounds({}, false);
    rw->setClearColor(g_game.isEditing || g_game.isDebugCameraEnabled);

    if (!isPaused)
    {
        bool showPauseMenu = g_input.isKeyPressed(KEY_ESCAPE);
        for (auto& pair : g_input.getControllers())
        {
            if (pair.value.isButtonPressed(BUTTON_START))
            {
                showPauseMenu = true;
                break;
            }
        }
        if ((!g_game.isEditing && isRaceInProgress) && showPauseMenu)
        {
            setPaused(true);
        }
    }

    u32 viewportCount = 0;
    if (!isRaceInProgress)
    {
        viewportCount = 1;
    }
    else
    {
		for (auto& driver : g_game.state.drivers)
		{
            if (driver.hasCamera)
            {
                ++viewportCount;
            }
		}
    }
    rw->setViewportCount(viewportCount);
    //rw->addDirectionalLight(Vec3(0.2f, 0.5f, -0.8f), Vec3(1.0));
    rw->addDirectionalLight(Vec3(lengthdir(sunDir, 1.f) * cosf(sunDirZ), sinf(sunDirZ)),
            sunColor, sunStrength);

    if (!isPaused)
    {
        worldTime += deltaTime;
        rw->updateWorldTime(worldTime);

        SmallArray<Vec3> listenerPositions;
        if (!g_game.isEditing && !isRaceInProgress && isCameraTourEnabled
                && trackGraph.getPaths().size() > 0)
        {
            // move the camera around the track
            auto path = trackGraph.getPaths()[0];
            Vec3 targetP = path[currentTrackPreviewPoint]->position;
            Vec3 diff = targetP - trackPreviewPosition;
            if (lengthSquared(diff) < square(30.f))
            {
                currentTrackPreviewPoint = (currentTrackPreviewPoint + 1) % (u32)path.size();
            }
            f32 accel = 8.0f;
            trackPreviewVelocity += normalize(diff) * deltaTime * accel;
            f32 maxSpeed = 15.f;
            if (length(trackPreviewVelocity) > maxSpeed)
            {
                trackPreviewVelocity = normalize(trackPreviewVelocity) * maxSpeed;
            }
            trackPreviewPosition += trackPreviewVelocity * deltaTime;

            f32 camDistance = 100.f;
            Vec3 cameraTarget = trackPreviewPosition;
            Vec3 cameraFrom = cameraTarget + normalize(Vec3(1.f, 1.f, 1.25f)) * camDistance;

            trackPreviewCameraTarget = smoothMove(trackPreviewCameraTarget, cameraTarget, 5.f, deltaTime);
            trackPreviewCameraFrom = smoothMove(trackPreviewCameraFrom, cameraFrom, 5.f, deltaTime);

            rw->setViewportCamera(0, trackPreviewCameraFrom, trackPreviewCameraTarget, 25.f, 200.f);

            listenerPositions.push(cameraTarget);
        }

        // TODO: Use PhysX scratch buffer to reduce allocations
        if (isRaceInProgress)
        {
            physicsMouseDrag(renderer);
            physicsScene->simulate(deltaTime);
            physicsScene->fetchResults(true);
        }
        else
        {
            rw->setMotionBlur(0, Vec2(0.f));
        }

        // update vehicles
        for (u32 i=0; i<vehicles.size(); ++i)
        {
            vehicles[i]->onUpdate(rw, deltaTime);
            if (vehicles[i]->cameraIndex >= 0)
            {
                listenerPositions.push(vehicles[i]->lastValidPosition);
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
                    placements.push(i);
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
            if (finishTimer >= 5.f && !g_game.isEditing)
            {
#if 0
                // TODO: Tell the player to press a button
                if (didSelect())
                {
                    buildRaceResults();
                    stopRace();
                    g_game.menu.showRaceResults();
                }
#else
                    buildRaceResults();
                    stopRace();
                    g_game.menu.showRaceResults();
#endif
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
        entities.push(move(e));
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
        BoundingBox bb = computeCameraFrustumBoundingBox(inverse(cam.viewProjection));
        debugDraw.boundingBox(bb, Mat4(1.f), Vec4(1.f));

        physicsScene->setVisualizationCullingBox(PxBounds3(convert(bb.min), convert(bb.max)));

        const PxRenderBuffer& rb = physicsScene->getRenderBuffer();
        for(PxU32 i=0; i < rb.getNbLines(); ++i)
        {
            const PxDebugLine& line = rb.getLines()[i];
            debugDraw.line(line.pos0, line.pos1, rgbaFromU32(line.color0), rgbaFromU32(line.color1));
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
            Vec4 color(1.f, 1.f, 0.f, 1.f);
            for (u32 i=1; i<path.points.size(); ++i)
            {
                debugDraw.line(path.points[i-1].position, path.points[i].position, color, color);
            }
            debugDraw.line(path.points.back().position, path.points.front().position, color, color);
        }
    }

    ribbons.draw(rw);
    smoke.draw(rw);
    sparks.draw(rw);
    debugDraw.draw(rw);

    // draw HUD track
    if (isRaceInProgress)
    {
        u32 size = (u32)(g_game.windowHeight * 0.39f * g_game.config.gameplay.hudTrackScale);
        Vec2 hudTrackPos;
        if (viewportCount == 1)
        {
            hudTrackPos = Vec2(size * 0.5f + 15.f) + Vec2(0, 15);
        }
        else if (viewportCount == 2)
        {
            hudTrackPos = Vec2(g_game.windowWidth * 0.5f, size * 0.5f + 15.f);
        }
        /*
        else if (viewportCount == 2)
        {
            hudTrackPos = Vec2(size * 0.5f + 60, g_game.windowHeight * 0.5f);
        }
        */
        else if (viewportCount == 3)
        {
            hudTrackPos = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.75f;
        }
        else if (viewportCount == 4)
        {
            hudTrackPos = Vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        }

        updateTrackPreview(renderer, size);
        if (!(isPaused && renderer->getRenderWorld()->getViewportCount() == 4))
        {
            Vec2 size = trackPreview2D.getSize();
            ui::rectUV(ui::IMAGE, trackPreview2D.getTexture(), hudTrackPos-size*0.5f,
                        size, {0,1}, {1,0});
        }
    }

    if (backgroundSound)
    {
        f32 volume = (g_game.isEditing && !isRaceInProgress) ? 0.f : 1.f;
        g_audio.setSoundVolume(backgroundSound, volume);
    }
}

void Scene::physicsMouseDrag(Renderer* renderer)
{
    static Vec3 dragPlaneOffset;
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

        Vec3 rayDir = editorCamera.getMouseRay(renderer->getRenderWorld());
        if (g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            PxRaycastBuffer hit;
            if (raycast(editorCamera.getCameraFrom(), rayDir, 1000.f, &hit))
            {
                if (hit.block.actor->getType() == PxActorType::eRIGID_DYNAMIC)
                {
                    dragPlaneOffset = Vec3(hit.block.position) - editorCamera.getCameraFrom();
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
                Vec3 hitPoint = editorCamera.getCameraFrom() + rayDir * hitDist;
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
    finishOrder.push(n);
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
            SoundType::GAME_SFX, getStart().position());
}

void Scene::updateTrackPreview(Renderer* renderer, u32 size)
{
    BoundingBox bb = track->getBoundingBox();

#if 1
    if (!hasTrackPreview)
    {
        hasTrackPreview = true;

        Vec3 bbCenter = (bb.min + bb.max) * 0.5f;
        Vec3 dir = normalize(Vec3(1.f));
        f32 dist = 850.f;
        Vec3 camPosition;
        Mat4 viewProjection;
        for (u32 i=1; i<150; ++i)
        {
            camPosition = (bbCenter - Vec3(0, 0, 20)) + dir * dist;
            Mat4 view = Mat4::lookAt(camPosition, bbCenter, Vec3(0, 0, 1));
            Mat4 projection = Mat4::perspective(radians(26.f), 1.f, 1.f, 2500.f);
            viewProjection = projection * view;

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
                Vec4 tp = viewProjection * Vec4(p, 1.f);
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
        startMesh->vao, startMesh->numIndices, start->transform, Vec3(1.f), false);

    Mesh* trackMesh = track->getPreviewMesh(this);
    trackPreview2D.drawItem(trackMesh->vao, (u32)trackMesh->numIndices, Mat4(1.f),
            Vec3(1.f), true, 1);
    trackPreview2D.drawItem(trackMesh->vao, (u32)trackMesh->numIndices,
            Mat4::translation({ 0, 0, -5 }), Vec3(0.2f), true, 0);
    /*
    for (u32 i=0; i<20; ++i)
    {
        trackPreview2D.drawItem(trackMesh->vao, (u32)trackMesh->numIndices,
                Mat4::translation(
                    { 0, 0, track->getBoundingBox().min.z - 7.f + (i/20.f * 7.f)})
                    * Mat4::scaling({ 1, 1, i/20.f }), Vec3(0.18f), true, 0);
    }
    */

    Mesh* sphereMesh = &g_res.getModel("sphere")->meshes.front();
    //Mesh* cubeMesh = &g_res.getModel("HUDCar")->meshes.front();
    for (auto const& v : vehicles)
    {
        trackPreview2D.drawItem(sphereMesh->vao, sphereMesh->numIndices,
            Mat4::translation(Vec3(0, 0, 2.1f) + v->getPosition())
                * Mat4::scaling(Vec3(5.1f)),
            v->getDriver()->getVehicleConfig()->cosmetics.color, false, 2);
        /*
        trackPreview2D.drawItem(cubeMesh->vao, cubeMesh->numIndices, v->getTransform(),
            v->getDriver()->getVehicleConfig()->color, false, 0);
        */
    }
#else

    f32 pad = 20.f;
    bb.min.x -= pad;
    bb.min.y -= pad;
    bb.max.x += pad;
    bb.max.y += pad;

    Vec3 offset = -(bb.min + (bb.max - bb.min) * 0.5f);

    f32 rot = PI * 0.25f;
    Mat4 transform = Mat4::rotationZ(f32(rot)) * Mat4::translation(offset);
    bb = bb.transform(transform);
    f32 radius = max(bb.max.x, max(bb.max.y, bb.max.z));
    bb.min = Vec3(-radius);
    bb.max = Vec3(radius);

    Mat4 trackOrtho = ortho(bb.max.x, bb.min.x, bb.min.y,
                bb.max.y, -bb.max.z - 10.f, -bb.min.z + 10.f) * transform;

    trackPreview2D.setCamViewProjection(trackOrtho);
    trackPreview2D.beginUpdate(renderer, size, size);

    Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("Quad");
    trackPreview2D.drawItem(
        quadMesh->vao, quadMesh->numIndices,
        start->transform * Mat4::translation({ 0, 0, -2 })
            * Mat4::scaling({ 4, 24, 1 }), Vec3(0.03f), true);

    Mesh* trackMesh = track->getPreviewMesh(this);
    trackPreview2D.drawItem(trackMesh->vao, (u32)trackMesh->numIndices, Mat4(1.f),
            Vec3(1.f), true, 1);

    Mesh* mesh = g_res.getModel("misc")->getMeshByName("TrackArrow");
    for (auto const& v : vehicles)
    {
        Vec3 pos = v->getPosition();
        trackPreview2D.drawItem(mesh->vao, mesh->numIndices,
            Mat4::translation(Vec3(0, 0, 2 + v->vehicleIndex*0.01) + pos)
                * Mat4::rotationZ(pointDirection(pos, pos + v->getForwardVector()) + PI * 0.5f)
                * Mat4::scaling(Vec3(10.f)),
            v->getDriver()->getVehicleConfig()->color, false, 0);
    }
#endif

    trackPreview2D.endUpdate();
}

void Scene::writeTrackData()
{
    DataFile::Value data = DataFile::makeDict();
    Serializer s(data, false);
    serialize(s);
    g_res.getTrackData(guid)->data = move(data);
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
            vehicles[instigator]->addBonus("ATTACK BONUS", ATTACK_BONUS_AMOUNT, Vec3(1.f));
            vehicles[victim]->addNotification("DESTROYED", 2.f, Vec3(1.f, 0.3f, 0.02f));
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
                v->placement = 9999;
            }
            else
            {
                auto& ai = *v->getAI();
                // generate fake stats for the drivers that have not yet finished the race
                for (u32 i = v->currentLap + 1; i<totalLaps; ++i)
                {
                    stats.accidents += irandom(randomSeries, 0, 2);
                    u32 attackBonuses = irandom(randomSeries, 0,
                            (u32)(numDriversStillDriving * ai.aggression * 0.5f));
                    stats.frags += attackBonuses;
                    stats.bonuses.push({ "", attackBonuses * ATTACK_BONUS_AMOUNT });
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
        raceResults.push({
            v->placement,
            v->driver,
            move(v->raceStatistics),
            v->placement != 9999
        });
    }
    raceResults.sort([](auto& a, auto&b) { return a.placement < b.placement; });
    for (u32 i=0; i<(u32)raceResults.size(); ++i)
    {
        raceResults[i].placement = i;
    }
}

void Scene::applyAreaForce(Vec3 const& position, f32 strength) const
{
    for (auto& v : vehicles)
    {
        f32 dist = distance(v->getPosition(), position);
        f32 radius = strength * 1.5f;
        if (dist < radius)
        {
            v->shakeScreen(powf(
                        clamp(1.f - dist / radius, 0.f, 1.f), 0.5f) * radius);
        }
    }
    // TODO: apply force to nearby physics bodies
}

void Scene::createExplosion(Vec3 const& position, Vec3 const& velocity, f32 strength)
{
    applyAreaForce(position, strength);
    for (u32 i=0; i<8; ++i)
    {
        f32 s = random(randomSeries, 0.3f, 0.9f);
        f32 size = strength * 0.1f;
        Vec3 offset = Vec3(
            random(randomSeries, -1.f, 1.f),
            random(randomSeries, -1.f, 1.f),
            random(randomSeries, -1.f, 1.f)) * size;
        Vec3 vel = normalize(Vec3(
            random(randomSeries, -1.f, 1.f),
            random(randomSeries, -1.f, 1.f),
            random(randomSeries, -1.f, 1.f))) * random(randomSeries, 0.75f, 1.5f)
            + Vec3(0, 0, random(randomSeries, 2.f, 3.f)) + velocity * 0.9f;
        smoke.spawn(position + offset, vel, 1.f, Vec4(s, s, s, 1.f), random(randomSeries, 3.f, 5.f));
    }
    for (u32 i=0; i<8; ++i)
    {
        Vec3 vel = velocity + normalize(Vec3(
            random(randomSeries, -0.5f, 0.5f),
            random(randomSeries, -0.5f, 0.5f),
            random(randomSeries, -0.25f, 0.75f))) * random(randomSeries, 5.f, 12.f);
        sparks.spawn(position, vel, 1.f,
                Vec4(Vec3(1.f, random(randomSeries, 0.55f, 0.7f), 0.02f), 1.f) * 2.f);
    }

    addEntity(new Flash(position + Vec3(0, 0, 1.f), velocity * 0.8f, strength * 0.5f));
}

bool Scene::raycastStatic(Vec3 const& from, Vec3 const& dir, f32 dist, PxRaycastBuffer* hit, u32 flags) const
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

bool Scene::raycast(Vec3 const& from, Vec3 const& dir, f32 dist, PxRaycastBuffer* hit)
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

bool Scene::sweepStatic(f32 radius, Vec3 const& from, Vec3 const& dir,
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

bool Scene::sweep(f32 radius, Vec3 const& from, Vec3 const& dir, f32 dist,
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
                f32 damage = min(magnitude * 0.00058f, 100.f);

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
                            myDamage = max(myDamage, minDamage);
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
                            myDamage = max(myDamage, 5.f);
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

                if (damage > 0.1f)
                {
                    g_audio.playSound3D(g_res.getSound("impact"),
                            SoundType::GAME_SFX, contactPoints[j].position, false, 1.f,
                            clamp(damage * 0.2f, 0.1f, 1.f));
                }
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
                Vec3 velOffset = Vec3(
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
                    f32 alpha = min((magnitude - minMagnitude) * 0.25f, 1.f);
                    Vec3 collisionVelocity = velA + velB * 0.5f;
                    sparks.spawn(contactPoints[j].position,
                            (contactPoints[j].normal + velOffset)
                                * random(randomSeries, 4.f, 5.f) + collisionVelocity * 0.4f, 1.f,
                            Vec4(Vec3(1.f, random(randomSeries, 0.55f, 0.7f), 0.02f) * 2.f, alpha));
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
            if ((entity->entityFlags & EntityFlags::PERSISTENT) == EntityFlags::PERSISTENT)
            {
                auto dict = DataFile::makeDict();
                Serializer s(dict, false);
                entity->serialize(s);
                entityArray.push(move(dict));
            }
        }
    }
}

Entity* Scene::deserializeEntity(DataFile::Value& val)
{
    i32 entityID = (i32)val.dict(true).val()["entityID"].integer().val();
    Serializer s(val, true);
    Entity* entity = g_entities[entityID].create(entityID);
    entity->serializeState(s);
    this->addEntity(entity);
    return entity;
}

Array<DataFile::Value> Scene::serializeTransientEntities()
{
    Array<DataFile::Value> transientEntities;
    for (auto& entity : this->entities)
    {
        if ((entity->entityFlags & EntityFlags::TRANSIENT) == EntityFlags::TRANSIENT)
        {
            auto dict = DataFile::makeDict();
            Serializer s(dict, false);
            entity->serialize(s);
            transientEntities.push(move(dict));
        }
    }
    return transientEntities;
}

void Scene::deserializeTransientEntities(Array<DataFile::Value>& entities)
{
    for (auto it = this->entities.begin(); it != this->entities.end();)
    {
        if (((*it)->entityFlags & EntityFlags::TRANSIENT) == EntityFlags::TRANSIENT)
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
    ImGui::Text("Scene Name: %s", name.data());
    ImGui::Text("Entities: %i", entities.size());
    ImGui::Text("Generated Paths: %s", hasGeneratedPaths ? "true" : "false");
    ImGui::Text("World Time: %.4f", worldTime);
    if (auto playerVehicle = vehicles.findIf([](auto& v) { return v->driver->isPlayer; }))
    {
        ImGui::Gap();
        playerVehicle->get()->showDebugInfo();
    }
}

f32 Scene::timeUntilStart() const
{
    return g_game.isEditing ? 0.f : 3.f - (f32)worldTime;
}

Vec3 Scene::findValidPosition(Vec3 const& pos, f32 collisionRadius, f32 checkRadius)
{
    PxTransform transform(PxIdentity);
    transform.p = convert(pos);
    PxSphereGeometry geometry(collisionRadius);

    PxOverlapBuffer overlapHit;
    PxRaycastBuffer raycastHit;

    PxQueryFilterData filterObstacles;
    filterObstacles.flags |= PxQueryFlag::eANY_HIT | PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;
    filterObstacles.data = PxFilterData(
            COLLISION_FLAG_OBJECT | COLLISION_FLAG_CHASSIS |
            COLLISION_FLAG_MINE | COLLISION_FLAG_SPLINE, 0, 0, 0);

    PxQueryFilterData filterTrack;
    filterTrack.flags |= PxQueryFlag::eANY_HIT | PxQueryFlag::eSTATIC;
    filterTrack.data = PxFilterData(COLLISION_FLAG_TRACK, 0, 0, 0);

    PxVec3 offsets[] = {
        PxVec3(1, 0, 0),
        PxVec3(-1, 0, 0),
        PxVec3(0, 1, 0),
        PxVec3(0, -1, 0)
    };

    for (u32 i=0; i<20; ++i)
    {
        if (!physicsScene->overlap(geometry, transform, overlapHit, filterObstacles))
        {
            bool onTrack = true;
            for (PxVec3& offset : offsets)
            {
                if (!physicsScene->raycast(transform.p + offset * 2.5f + PxVec3(0, 0, 5.f), PxVec3(0, 0, -1), 10.f, raycastHit,
                        PxHitFlags(PxHitFlag::eMESH_ANY), filterTrack))
                {
                    onTrack = false;
                    break;
                }
            }
            if (onTrack)
            {
                return transform.p;
            }
        }

        Vec3 offset(
                random(randomSeries, -checkRadius, checkRadius),
                random(randomSeries, -checkRadius, checkRadius), 0.f);
        transform.p = convert(pos + offset);
    }

    return pos;
}

void Scene::setPaused(bool paused)
{
    isPaused = paused;
    g_audio.setPaused(isPaused);
    if (isPaused)
    {
        g_game.menu.showPauseMenu();
    }
}

void Scene::forfeitRace()
{
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
