#include "scene.h"
#include "game.h"
#include "vehicle.h"
#include "renderer.h"
#include "mesh_renderables.h"
#include "input.h"
#include "terrain.h"
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
    sceneDesc.gravity = PxVec3(0.f, 0.f, -12.1f);
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
    trackMaterial   = g_game.physx.physics->createMaterial(0.4f, 0.4f, 0.4f);
    offroadMaterial = g_game.physx.physics->createMaterial(0.4f, 0.4f, 0.1f);

    if (!name)
    {
        // loading empty scene, so add default terrain
        terrain = new Terrain();
        addEntity(terrain);
        return;
    }

    // construct scene from blender data
    auto& sceneData = g_resources.getScene(name);
    auto& entities = sceneData["entities"].array();
    bool foundStart = false;

    struct TrackMesh
    {
        Mesh* mesh;
        glm::mat4 transform;
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
                            *g_resources.getMesh((*it)["data_name"].string().c_str()),
                            (*it)["matrix"].convertBytes<glm::mat4>());
                }
            }

            Mesh* mesh = g_resources.getMesh(dataName.c_str());
            if (mesh->elementSize < 3)
            {
                continue;
            }

            bool isTrack = name.find("Track") != std::string::npos;
            bool isSand = name.find("Sand") != std::string::npos;
            physicsUserData.push_back(std::make_unique<ActorUserData>());
            physicsUserData.back()->entityType = (isTrack || isSand) ? ActorUserData::TRACK : ActorUserData::SCENERY;

            Material* mat = nullptr;
            if (e["properties"].hasKey("material"))
            {
                mat = g_resources.getMaterial(e["properties"]["material"].string().c_str());
            }
            staticEntities.push_back({
                mesh,
                transform,
                mat,
                isTrack
            });

            if (isTrack)
            {
                trackMeshes.push_back({ mesh, transform });
            }

            PxMaterial* material = isTrack ? trackMaterial : offroadMaterial;
            if (isSand) material = offroadMaterial;

            PxRigidStatic* actor = g_game.physx.physics->createRigidStatic(convert(transform));
            PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor,
                    PxTriangleMeshGeometry(g_resources.getCollisionMesh(dataName.c_str()),
                        PxMeshScale(convert(scaleOf(transform)))), *material);
            shape->setQueryFilterData(PxFilterData(
                        isTrack ? COLLISION_FLAG_TRACK : COLLISION_FLAG_GROUND, 0, 0, DRIVABLE_SURFACE));
            shape->setSimulationFilterData(PxFilterData(
                        isTrack ? COLLISION_FLAG_TRACK : COLLISION_FLAG_GROUND, -1, 0, 0));
            actor->userData = physicsUserData.back().get();
            physicsScene->addActor(*actor);
        }
        else if (entityType == "PATH")
        {
            std::string name = e["name"].string();
            if (name.find("TrackPath") != std::string::npos)
            {
                auto& data = e["points"].bytearray();
                paths.push_back(std::vector<glm::vec3>(
                            (glm::vec3*)data.data(),
                            (glm::vec3*)(data.data() + data.size())));
            }
        }
    }

    if (!foundStart)
    {
        FATAL_ERROR("Track does not have a starting point!");
    }

    BoundingBox bb = { glm::vec3(FLT_MAX), glm::vec3(-FLT_MAX) };
    for (auto& t : trackMeshes)
    {
        t.mesh->buildOctree();
        bb = bb.growToFit(t.mesh->aabb.transform(t.transform));
    }
    f32 pad = 30.f;
    trackOrtho = glm::rotate(glm::mat4(1.f), f32(M_PI * -0.5f), { 0, 0, 1 })
        * glm::ortho(bb.max.x + pad, bb.min.x - pad, bb.min.y - pad,
                bb.max.y + pad, -bb.max.z - 10.f, -bb.min.z + 10.f);

    for (auto& t : trackMeshes)
    {
        trackItems.push_back({ t.mesh, trackOrtho * t.transform, glm::vec3(1.f), true });
    }
    trackAspectRatio = (bb.max.x - bb.min.x) / (bb.max.y - bb.min.y);

    trackItems.push_back({
        g_resources.getMesh("world.Quad"),
        trackOrtho * start * glm::translate(glm::mat4(1.f), { 0, 0, -3 }) * glm::scale(glm::mat4(1.f), { 5, 16, 1 }),
        glm::vec3(0.2f),
        true
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
    if (isEditing)
    {
        editor.onStart(this);
        return;
    }

    const PxMaterial* surfaceMaterials[] = { trackMaterial, offroadMaterial };
    for (u32 i=0; i<g_game.state.drivers.size(); ++i)
    {
        Driver* driver = &g_game.state.drivers[i];

        glm::vec3 offset = -glm::vec3(6 + i / 4 * 8, -9.f + i % 4 * 6, 0.f);

        PxRaycastBuffer hit;
        if (!raycastStatic(translationOf(glm::translate(start, offset + glm::vec3(0, 0, 8))),
                -zAxisOf(start), 30.f, &hit))
        {
            FATAL_ERROR("The starting point is too high in the air!");
        }

        glm::mat4 vehicleTransform = glm::translate(glm::mat4(1.f),
                convert(hit.block.position + hit.block.normal * driver->vehicleData->getRestOffset())) * rotationOf(start);

        vehicles.push_back(std::make_unique<Vehicle>(this, vehicleTransform, -offset,
            driver, vehicleMaterial, surfaceMaterials, i));
    }
}

void Scene::onUpdate(Renderer* renderer, f32 deltaTime)
{
    physicsScene->simulate(deltaTime);
    physicsScene->fetchResults(true);

    u32 viewportCount = (u32)std::count_if(g_game.state.drivers.begin(), g_game.state.drivers.end(),
            [](auto& d) { return d.hasCamera; });
    renderer->setViewportCount(viewportCount);
    renderer->addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.0));

    for (auto const& e : entities) {
        e->onUpdate(renderer, this, deltaTime);
    }

    // draw static entities
    for (auto const& e : staticEntities)
    {
        renderer->drawMesh(e.mesh, e.worldTransform, e.material);
    }

    // update vehicles
    i32 cameraIndex = 0;
    SmallVec<glm::vec3> listenerPositions;
    for (u32 i=0; i<vehicles.size(); ++i)
    {
        vehicles[i]->onUpdate(renderer, deltaTime, vehicles[i]->getDriver()->hasCamera ? cameraIndex : -1);
        if (vehicles[i]->getDriver()->hasCamera)
        {
            ++cameraIndex;
            listenerPositions.push_back(vehicles[i]->lastValidPosition);
        }
    }
    g_audio.setListeners(listenerPositions);

    // determine vehicle placement
    if (vehicles.size() > 0)
    {
        SmallVec<u32, MAX_VEHICLES> placements;
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
    }

    if (g_input.isKeyPressed(KEY_F2))
    {
        isPhysicsDebugVisualizationEnabled = !isPhysicsDebugVisualizationEnabled;
        if (isPhysicsDebugVisualizationEnabled)
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

    if (isPhysicsDebugVisualizationEnabled)
    {
        Camera const& cam = renderer->getCamera(0);
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
            debugCameraPosition = renderer->getCamera(0).position;
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
        renderer->setViewportCamera(0, cameraFrom, cameraTarget, 15.f, 900.f);
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

    renderer->add(&ribbons);
    renderer->add(&smoke);
    renderer->add(&debugDraw);

    // draw HUD track
    if (!isEditing)
    {
        Mesh* arrowMesh = g_resources.getMesh("world.TrackArrow");
        SmallVec<TrackPreview2D::RenderItem, MAX_VEHICLES> dynamicItems;
        for (auto const& v : vehicles)
        {
            glm::vec3 pos = v->getPosition();
            dynamicItems.push_back({
                arrowMesh,
                trackOrtho * glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 2) + pos)
                    * glm::rotate(glm::mat4(1.f), pointDirection(pos, pos + v->getForwardVector()) + f32(M_PI) * 0.5f, { 0, 0, 1 })
                    * glm::scale(glm::mat4(1.f), glm::vec3(10.f)),
                v->getDriver()->vehicleColor,
                false
            });
        }
        u32 size = (u32)(g_game.windowHeight * 0.23f);
        glm::vec2 hudTrackPos;
        if (viewportCount == 1) hudTrackPos = glm::vec2(size * 0.5f + 50.f) + glm::vec2(0, 30);
        else if (viewportCount == 2) hudTrackPos = glm::vec2(size * 0.5f + 60, g_game.windowHeight * 0.5f);
        else if (viewportCount == 3)
        {
            hudTrackPos = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.75f;
            size = (u32)(g_game.windowHeight * 0.36f);
        }
        else if (viewportCount == 4) hudTrackPos = glm::vec2(g_game.windowWidth, g_game.windowHeight) * 0.5f;

        trackPreview2D.update(renderer, trackItems, dynamicItems, size, (u32)(size * trackAspectRatio), hudTrackPos);
        renderer->add(&trackPreview2D);
    }

    if (isDebugOverlayEnabled)
    {
        Font* font1 = &g_resources.getFont("font", 20);
        Font* font2 = &g_resources.getFont("font", 18);
        std::string debugText = str(
            "FPS: ", 1.f / g_game.realDeltaTime,
            "\nDelta: ", g_game.realDeltaTime,
            "\nDilation: ", g_game.timeDilation,
            "\nResolution: ", g_game.config.resolutionX, "x", g_game.config.resolutionY,
            "\nTmpRenderMem: ", std::fixed, std::setprecision(2), renderer->getTempRenderBufferSize() / 1024.f, "kb");

        renderer->push(QuadRenderable(g_resources.getTexture("white"), { 10, g_game.windowHeight - 10 },
                    { 220, g_game.windowHeight - (30 + font1->stringDimensions(debugText.c_str()).y) },
                    {}, {}, { 0, 0, 0 }, 0.6));
        renderer->push(TextRenderable(font1, debugText,
            { 20, g_game.windowHeight - 20 }, glm::vec3(1), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::BOTTOM));

        std::string debugRenderListText = renderer->getDebugRenderList();
        auto dim = font2->stringDimensions(debugRenderListText.c_str());
        renderer->push(QuadRenderable(g_resources.getTexture("white"), { 10, 10 },
                    { 30 + dim.x, 30 + dim.y }, {}, {}, { 0, 0, 0 }, 0.6));
        renderer->push(TextRenderable(font2, debugRenderListText,
            { 20, 20 }, glm::vec3(0.1f, 1.f, 0.1f), 1.f, 1.f));
    }

    if (isEditing)
    {
        editor.onUpdate(this, renderer, deltaTime);
    }
}

void Scene::onEndUpdate(f32 deltaTime)
{
    for (auto& e : newEntities) {
        e->onCreate(this);
        entities.push_back(std::move(e));
    }
    newEntities.clear();

    for (auto it = entities.begin(); it != entities.end();) {
        if ((*it)->isDestroyed()) {
            entities.erase(it);
        } else {
            ++it;
        }
    }
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
            vehicles[instigator]->addNotification("ACCIDENT!");
        }
        else
        {
            vehicles[instigator]->addNotification("ATTACK BONUS!");
        }
    }
}

bool Scene::raycastStatic(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit) const
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(COLLISION_FLAG_GROUND | COLLISION_FLAG_TRACK, 0, 0, 0);
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
    filter.data = PxFilterData(COLLISION_FLAG_GROUND | COLLISION_FLAG_TRACK, 0, 0, 0);
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

bool Scene::sweep(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxSweepBuffer* hit, PxRigidActor* ignore) const
{
    PxQueryFilterData filter;
    filter.flags |= PxQueryFlag::eSTATIC;
    filter.flags |= PxQueryFlag::eDYNAMIC;
    if (ignore)
    {
        filter.flags |= PxQueryFlag::ePREFILTER;
    }
    IgnoreActor cb(ignore);
    filter.data = PxFilterData(COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS, 0, 0, 0);
    PxTransform initialPose(convert(from), PxQuat(PxIdentity));
    if (hit)
    {
        return physicsScene->sweep(PxSphereGeometry(radius), initialPose, convert(dir), dist,
                *hit, PxHitFlags(PxHitFlag::eDEFAULT), filter, &cb);
    }
    else
    {
        PxSweepBuffer tmpHit;
        return physicsScene->sweep(PxSphereGeometry(radius), initialPose, convert(dir), dist, tmpHit, PxHitFlags(PxHitFlag::eDEFAULT), filter, &cb);
    }
}

void Scene::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
{
    PxContactPairPoint contactPoints[32];

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
                    f32 damage = glm::min(magnitude * 0.001f, 50.f);

                    // apply damage
                    if (a && a->entityType == ActorUserData::VEHICLE)
                    {
                        u32 instigator = (b && b->entityType == ActorUserData::VEHICLE) ? b->vehicle->vehicleIndex : a->vehicle->vehicleIndex;
                        a->vehicle->applyDamage(damage, instigator);
                    }
                    if (b && b->entityType == ActorUserData::VEHICLE)
                    {
                        u32 instigator = (a && a->entityType == ActorUserData::VEHICLE) ? a->vehicle->vehicleIndex : b->vehicle->vehicleIndex;
                        b->vehicle->applyDamage(damage, instigator);
                    }

                    g_audio.playSound3D(g_resources.getSound("impact"), convert(contactPoints[j].position));
                    // TODO: create sparks at contactPoints[j].position
                }
            }
        }
    }
}
