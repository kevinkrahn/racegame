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
#include "entities/tree.h"
#include "entities/flash.h"
#include "entities/booster.h"
#include "gui.h"
#include <algorithm>

PxFilterFlags vehicleFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    PX_UNUSED(attributes0);
    PX_UNUSED(attributes1);
    PX_UNUSED(constantBlock);
    PX_UNUSED(constantBlockSize);

    if( (0 == (filterData0.word0 & filterData1.word1)) && (0 == (filterData1.word0 & filterData0.word1)) )
    {
        return PxFilterFlag::eSUPPRESS;
    }

    pairFlags = PxPairFlag::eCONTACT_DEFAULT;
    pairFlags |= PxPairFlags(PxU16(filterData0.word2 | filterData1.word2));

    if (((filterData0.word0 & COLLISION_FLAG_CHASSIS) || (filterData1.word0 & COLLISION_FLAG_CHASSIS)) &&
        (!(filterData0.word0 & COLLISION_FLAG_DEBRIS) && !(filterData1.word0 & COLLISION_FLAG_DEBRIS)))
    {
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
        pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
    }

    return PxFilterFlags();
}

Scene::Scene(const char* name)
{
    // create PhysX scene
    PxSceneDesc sceneDesc(g_game.physx.physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, 0.f, -15.f);
    sceneDesc.cpuDispatcher = g_game.physx.dispatcher;
    sceneDesc.filterShader  = vehicleFilterShader;
    sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
    sceneDesc.simulationEventCallback = this;

    physicsScene = g_game.physx.physics->createScene(sceneDesc);

    if (PxPvdSceneClient* pvdClient = physicsScene->getScenePvdClient())
    {
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    // create physics materials
    vehicleMaterial = g_game.physx.physics->createMaterial(0.1f, 0.1f, 0.4f);
    trackMaterial   = g_game.physx.physics->createMaterial(0.3f, 0.3f, 0.4f);
    offroadMaterial = g_game.physx.physics->createMaterial(0.4f, 0.4f, 0.1f);
    genericMaterial = g_game.physx.physics->createMaterial(0.4f, 0.4f, 0.05f);

    if (!name)
    {
        // loading empty scene, so add default terrain, track, and start
        addEntity(g_entities[0].create());
        addEntity(g_entities[1].create());
        addEntity(g_entities[4].create());
    }
    else
    {
        auto data = DataFile::load(name);
        deserialize(data);
        this->filename = name;
    }

    while (newEntities.size() > 0)
    {
        for (auto& e : newEntities)
        {
            e->setPersistent(true);
            e->onCreate(this);
        }
        for (auto& e : newEntities)
        {
            e->onCreateEnd(this);
            entities.push_back(std::move(e));
        }
        newEntities.clear();
    }

    assert(track != nullptr);
    assert(start != nullptr);
    track->buildTrackGraph(&trackGraph, start->transform);
    trackPreviewPosition = start->position;
    trackPreviewCameraTarget = trackPreviewPosition;
    trackPreviewCameraFrom = trackPreviewPosition + glm::vec3(0, 0, 5);
}

Scene::~Scene()
{
    physicsScene->release();
    vehicleMaterial->release();
    trackMaterial->release();
    offroadMaterial->release();
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

    track->buildTrackGraph(&trackGraph, start);
    const PxMaterial* surfaceMaterials[] = { trackMaterial, offroadMaterial };
    u32 driversPerRow = 5;
    f32 width = 16 * scaleOf(this->start->transform).y;
    i32 cameraIndex = 0;
    struct OrderedDriver
    {
        Driver* driver;
        i32 cameraIndex;
    };
    std::vector<OrderedDriver> createOrder;
    for (auto& driver : g_game.state.drivers)
    {
        createOrder.push_back({ &driver, driver.hasCamera ? cameraIndex : -1});
        if (driver.hasCamera)
        {
            ++cameraIndex;
        }
    }
    std::sort(createOrder.begin(), createOrder.end(),
            [](auto& a, auto& b) { return a.driver->lastPlacement < b.driver->lastPlacement; });

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

        vehicles.push_back(std::make_unique<Vehicle>(this, vehicleTransform, -offset,
            driverInfo.driver, std::move(tuning), vehicleMaterial, surfaceMaterials, i,
            driverInfo.cameraIndex));
    }

    for (u32 i=0; i<vehicles.size(); ++i)
    {
        placements.push_back(i);
    }

    isRaceInProgress = true;
}

void Scene::stopRace()
{
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
    backgroundSound = g_audio.playSound(&g_res.sounds->evironment, SoundType::MUSIC, true, 1.f, 0.f);
}

void Scene::onUpdate(Renderer* renderer, f32 deltaTime)
{
    RenderWorld* rw = renderer->getRenderWorld();

    bool showPauseMenu = g_input.isKeyPressed(KEY_ESCAPE);
    for (auto& pair : g_input.getControllers())
    {
        if (pair.second.isButtonPressed(BUTTON_START))
        {
            showPauseMenu = true;
            break;
        }
    }
    if (((g_game.isEditing && !isRaceInProgress)
        || (!g_game.isEditing && isRaceInProgress)) && showPauseMenu)
    {
        isPaused = !isPaused;
        g_audio.setPaused(isPaused);
    }

    if (g_game.isEditing && !isPaused)
    {
        editor.onUpdate(this, renderer, deltaTime);
    }

    u32 viewportCount = (!isRaceInProgress) ? 1 : (u32)std::count_if(g_game.state.drivers.begin(), g_game.state.drivers.end(),
            [](auto& d) { return d.hasCamera; });
    rw->setViewportCount(viewportCount);
    rw->addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));

    if (!isPaused)
    {
        worldTime += deltaTime;
        rw->updateWorldTime(worldTime);

        if (worldTime >= 3.f)
        {
            readyToGo = true;
        }

        SmallVec<glm::vec3> listenerPositions;
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
        }

        smoke.update(deltaTime);
        ribbons.update(deltaTime);

        g_audio.setListeners(listenerPositions);

        if (allPlayersFinished && isRaceInProgress)
        {
            finishTimer += deltaTime;
            if (finishTimer >= 8.f)
            {
                buildRaceResults();
                stopRace();
                //isCameraTourEnabled = false;
                g_game.menu.showRaceResults();
            }
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

    if (g_input.isKeyPressed(KEY_F2))
    {
        isPhysicsDebugVisualizationEnabled = !isPhysicsDebugVisualizationEnabled;
        if (isPhysicsDebugVisualizationEnabled)
        {
            physicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
            physicsScene->setVisualizationParameter(PxVisualizationParameter::eACTOR_AXES, 2.0f);
            physicsScene->setVisualizationParameter(PxVisualizationParameter::eBODY_MASS_AXES, 1.0f);
            //physicsScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 2.0f);
            //physicsScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_NORMAL, 2.0f);
            physicsScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 2.0f);
        }
        else
        {
            physicsScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 0.0f);
        }
    }

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

    if (isPhysicsDebugVisualizationEnabled)
    {
        Camera const& cam = rw->getCamera(0);
        BoundingBox bb = computeCameraFrustumBoundingBox(cam.viewProjection);
        debugDraw.boundingBox(bb, glm::mat4(1.f), glm::vec4(1.f));

        physicsScene->setVisualizationCullingBox(PxBounds3(convert(bb.min), convert(bb.max)));

        const PxRenderBuffer& rb = physicsScene->getRenderBuffer();
        for(PxU32 i=0; i < rb.getNbLines(); ++i)
        {
            const PxDebugLine& line = rb.getLines()[i];
            debugDraw.line(convert(line.pos0), convert(line.pos1), rgbaFromU32(line.color0), rgbaFromU32(line.color1));
        }
    }

    if (g_input.isKeyPressed(KEY_F8))
    {
        debugCamera = !debugCamera;
        if (debugCamera)
        {
            debugCameraPosition = rw->getCamera(0).position;
        }
    }
    if (debugCamera)
    {
        f32 up = (f32)g_input.isKeyDown(KEY_W) - (f32)g_input.isKeyDown(KEY_S);
        f32 right = (f32)g_input.isKeyDown(KEY_A) - (f32)g_input.isKeyDown(KEY_D);
        f32 vertical = (f32)g_input.isKeyDown(KEY_SPACE) - (f32)g_input.isKeyDown(KEY_LCTRL);

        glm::vec2 move(up, right);
        f32 speed = glm::length2(move);
        if (speed > 0)
        {
            move = glm::normalize(move);
        }
        glm::vec3 movement = glm::vec3(-move, vertical);

        debugCameraPosition += glm::vec3(movement) * deltaTime * 80.f;
        f32 camDistance = 20.f;
        glm::vec3 cameraFrom = debugCameraPosition + glm::normalize(glm::vec3(1.f, 1.f, 1.25f)) * camDistance;
        glm::vec3 cameraTarget = debugCameraPosition;
        rw->setViewportCamera(0, cameraFrom, cameraTarget, 15.f, 900.f);
    }

    if (g_input.isKeyPressed(KEY_F3))
    {
        isTrackGraphDebugVisualizationEnabled = !isTrackGraphDebugVisualizationEnabled;
    }

    if (isTrackGraphDebugVisualizationEnabled)
    {
        trackGraph.debugDraw(&debugDraw, renderer);
    }

    if (g_input.isKeyPressed(KEY_F1))
    {
        isDebugOverlayEnabled = !isDebugOverlayEnabled;
    }

    rw->add(&ribbons);
    rw->add(&smoke);
    rw->add(&debugDraw);

    // draw HUD track
    if (isRaceInProgress)
    {
        u32 size = (u32)(g_game.windowHeight * 0.27f);
        glm::vec2 hudTrackPos;
        if (viewportCount == 1)
        {
            hudTrackPos = glm::vec2(size * 0.5f + 50.f) + glm::vec2(0, 30);
        }
        else if (viewportCount == 2)
        {
            hudTrackPos = glm::vec2(size * 0.5f + 60, g_game.windowHeight * 0.5f);
        }
        else if (viewportCount == 3)
        {
            hudTrackPos = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.75f;
            size = (u32)(g_game.windowHeight * 0.36f);
        }
        else if (viewportCount == 4)
        {
            hudTrackPos = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;
        }
        size = (u32)(size * g_game.config.gameplay.hudTrackScale);

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
        if (isRaceInProgress)
        {
            if (g_gui.button("Forfeit Race"))
            {
                isPaused = false;
                trackPreviewCameraFrom = rw->getCamera(0).position;
                trackPreviewCameraTarget = (*std::find_if(vehicles.begin(), vehicles.end(),
                        [](auto& v) { return v->cameraIndex == 0; }))->getPosition();
                g_game.isEditing = false;
                if (g_game.state.gameMode == GameMode::CHAMPIONSHIP)
                {
                    buildRaceResults();
                    stopRace();
                    //isCameraTourEnabled = false;
                    g_game.menu.showRaceResults();
                }
                else
                {
                    stopRace();
                    g_game.menu.showMainMenu();
                }
            }
        }
        else
        {
            if (g_gui.button("Main Menu"))
            {
                trackPreviewCameraTarget = editor.getCameraTarget();
                trackPreviewCameraFrom = rw->getCamera(0).position;
                stopRace();
                isPaused = false;
                g_game.isEditing = false;
                g_game.menu.showMainMenu();
            }
        }
        if (g_gui.button("Quit to Desktop"))
        {
            g_game.shouldExit = true;
        }
        g_gui.end();
    }

    if (isDebugOverlayEnabled)
    {
        Font* font1 = &g_res.getFont("font", 20);
        Font* font2 = &g_res.getFont("font", 18);
        char* debugText = tstr(
            "FPS: ", 1.f / g_game.realDeltaTime,
            "\nDelta: ", g_game.realDeltaTime * 1000.f, "ms",
            "\nDilation: ", g_game.timeDilation,
            "\nResolution: ", g_game.config.graphics.resolutionX, "x", g_game.config.graphics.resolutionY,
            "\nTmpRenderMem: ", std::fixed, std::setprecision(2), renderer->getTempRenderBufferSize() / 1024.f, "kb");

        renderer->push2D(QuadRenderable(&g_res.textures->white, { 10, g_game.windowHeight - 10 },
                    { 220, g_game.windowHeight - (30 + font1->stringDimensions(debugText).y) },
                    {}, {}, { 0, 0, 0 }, 0.6f), 10);
        renderer->push2D(TextRenderable(font1, debugText,
            { 20, g_game.windowHeight - 20 }, glm::vec3(1),
            1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::BOTTOM), 10);

        char* debugRenderListText = tstr(renderer->getDebugRenderList());
        auto dim = font2->stringDimensions(debugRenderListText);
        renderer->push2D(QuadRenderable(&g_res.textures->white, { 10, 10 },
                    { 30 + dim.x, 30 + dim.y }, {}, {}, { 0, 0, 0 }, 0.6f), 10);
        renderer->push2D(TextRenderable(font2, debugRenderListText,
            { 20, 20 }, glm::vec3(0.1f, 1.f, 0.1f), 1.f, 1.f), 10);
    }

    if (backgroundSound)
    {
        f32 volume = (g_game.isEditing && !isRaceInProgress) ? 0.f : 1.f;
        g_audio.setSoundVolume(backgroundSound, volume);
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
        g_audio.playSound(&g_res.sounds->clapping, SoundType::GAME_SFX);
    }
}

void Scene::drawTrackPreview(Renderer* renderer, u32 size, glm::vec2 hudTrackPos)
{
    BoundingBox bb = track->getBoundingBox();

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

    trackPreview2D.beginUpdate(renderer, size, size);

    Mesh* quadMesh = g_res.getMesh("world.Quad");
    trackPreview2D.drawItem(
        quadMesh->vao, quadMesh->numIndices,
        trackOrtho * start->transform * glm::translate(glm::mat4(1.f), { 0, 0, -2 })
            * glm::scale(glm::mat4(1.f), { 4, 24, 1 }), glm::vec3(0.03f), true);
    track->drawTrackPreview(&trackPreview2D, trackOrtho);

    Mesh* arrowMesh = g_res.getMesh("world.TrackArrow");
    for (auto const& v : vehicles)
    {
        glm::vec3 pos = v->getPosition();
        trackPreview2D.drawItem(arrowMesh->vao, arrowMesh->numIndices,
            trackOrtho * glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 2) + pos)
                * glm::rotate(glm::mat4(1.f), pointDirection(pos, pos + v->getForwardVector()) + f32(M_PI) * 0.5f, { 0, 0, 1 })
                * glm::scale(glm::mat4(1.f), glm::vec3(10.f)),
            g_vehicleColors[v->getDriver()->getVehicleConfig()->colorIndex], false);
    }

    trackPreview2D.endUpdate(hudTrackPos);
}

void Scene::onEnd()
{
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
        RaceStatistics& stats = v->raceStatistics;
        raceResults.push_back({
            v->placement,
            v->driver,
            stats,
            v->placement != 9999
        });
    }
    std::sort(raceResults.begin(), raceResults.end(), [](auto& a, auto&b) {
        return a.placement < b.placement;
    });
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

        if(contactCount > 0)
        {
            pairs[i].extractContacts(contactPoints, contactCount);

            for(u32 j=0; j<contactCount; ++j)
            {
                f32 magnitude = contactPoints[j].impulse.magnitude();
                if (magnitude > 1400.f)
                {
                    ActorUserData* a = (ActorUserData*)pairHeader.actors[0]->userData;
                    ActorUserData* b = (ActorUserData*)pairHeader.actors[1]->userData;
                    f32 damage = glm::min(magnitude * 0.0006f, 100.f);

                    // apply damage
                    if (a && a->entityType == ActorUserData::VEHICLE)
                    {
                        f32 myDamage = damage;
                        if (b && b->entityType == ActorUserData::VEHICLE)
                        {
                            if (b->vehicle->hasAbility("Ram Booster"))
                            {
                                myDamage *= 2.75f;
                                // TODO: test this out
                                myDamage = std::max(myDamage, 5.f);
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

                    g_audio.playSound3D(&g_res.sounds->impact,
                            SoundType::GAME_SFX, convert(contactPoints[j].position));
                    // TODO: create sparks at contactPoints[j].position
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

DataFile::Value Scene::serialize()
{
    DataFile::Value dict = DataFile::makeDict();
    dict["name"] = DataFile::makeString(name);
    dict["version"] = DataFile::makeInteger(0);
    dict["entities"] = DataFile::makeArray();
    DataFile::Value::Array& entityArray = dict["entities"].array();

    for (auto& entity : this->entities)
    {
        if (entity->entityFlags & Entity::PERSISTENT)
        {
            entityArray.push_back(entity->serialize());
        }
    }

    return dict;
}

Entity* Scene::deserializeEntity(DataFile::Value& val)
{
    i32 entityID = (i32)val["entityID"].integer();
    Entity* entity = g_entities[entityID].create();
    entity->deserializeState(val);
    this->addEntity(entity);
    return entity;
}

void Scene::deserialize(DataFile::Value& data)
{
    name = data["name"].string("");
    u32 version = (u32)data["version"].integer(0);
    auto& entityArray = data["entities"].array();
    if (version == 0)
    {
        for (auto& val : entityArray)
        {
            i32 entityID = (i32)val["entityID"].integer();
            if (entityID == 2)
            {

            }
            deserializeEntity(val);
        }
    }
    else
    {
        for (auto& val : entityArray)
        {
            deserializeEntity(val);
        }
    }
}

std::vector<DataFile::Value> Scene::serializeTransientEntities()
{
    std::vector<DataFile::Value> transientEntities;
    for (auto& entity : this->entities)
    {
        if (entity->entityFlags & Entity::TRANSIENT)
        {
            transientEntities.push_back(entity->serialize());
        }
    }
    return transientEntities;
}

void Scene::deserializeTransientEntities(std::vector<DataFile::Value>& entities)
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
