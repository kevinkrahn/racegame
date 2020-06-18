#include "vehicle_data.h"
#include "game.h"
#include "resources.h"
#include "scene.h"
#include "vehicle.h"

VehicleStats VehicleTuning::computeVehicleStats()
{
    VehicleStats stats;
    stats.topSpeed = topSpeed / 100.f;
    stats.armor = maxHitPoints / 300.f;
    stats.mass = chassisMass / 2500.f;

    static PxScene* physicsScene = nullptr;
    if (!physicsScene)
    {
        PxSceneDesc sceneDesc(g_game.physx.physics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.f, 0.f, -15.f); // TODO: share this constant with scene
        sceneDesc.cpuDispatcher = g_game.physx.dispatcher;
        sceneDesc.filterShader = vehicleFilterShader;
        sceneDesc.flags |= PxSceneFlag::eENABLE_CCD;
        //sceneDesc.simulationEventCallback = this;
        sceneDesc.solverType = PxSolverType::eTGS;
        sceneDesc.broadPhaseType = PxBroadPhaseType::eABP;

        physicsScene = g_game.physx.physics->createScene(sceneDesc);
    }

    // create floor
    PxRigidStatic* actor = g_game.physx.physics->createRigidStatic(PxTransform(PxIdentity));
    Mesh* floorMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
    PxShape* shape = PxRigidActorExt::createExclusiveShape(*actor,
            PxTriangleMeshGeometry(floorMesh->getCollisionMesh(),
                PxMeshScale(200.f)), *g_game.physx.materials.track);
    shape->setQueryFilterData(
            PxFilterData(COLLISION_FLAG_TRACK, 0, 0, DRIVABLE_SURFACE));
    shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_TRACK, -1, 0, 0));
    physicsScene->addActor(*actor);

    // create vehicle
    // TODO: add a few timesteps of delay to make sure that suspension is at rest position
    Mat4 startTransform = Mat4::translation({ 0, 0, getRestOffset() });
    VehiclePhysics v;
    v.setup(nullptr, physicsScene, startTransform, this);

    const f32 timestep = 1.f / 50.f;

    // TODO: Use PhysX scratch buffer to reduce allocations

    // acceleration
    for (u32 iterations=0; iterations<1000; ++iterations)
    {
        physicsScene->simulate(timestep);
        physicsScene->fetchResults(true);

        v.update(physicsScene, timestep, true, 1.f, 0.f, 0.f, false, true, false);
        if (v.getForwardSpeed() >= 28.f)
        {
            f32 time = iterations * timestep;
            stats.acceleration = clamp((8.f - time) / 8.f, 0.f, 1.f);
            //print("Acceleration time: ", time, '\n');
            break;
        }
    }

    // grip
    v.reset(startTransform);
    //v.getRigidBody()->setLinearVelocity(PxVec3(0, 0, 0));
    //v.getRigidBody()->setAngularVelocity(PxVec3(0, 0, 0));
    v.getRigidBody()->addForce(PxVec3(0, 20.f, 0), PxForceMode::eVELOCITY_CHANGE);
    for (u32 iterations=0; iterations<1000; ++iterations)
    {
        physicsScene->simulate(timestep);
        physicsScene->fetchResults(true);

        v.update(physicsScene, timestep, true, 0.f, 0.f, 0.f, false, true, false);

        // prevent vehicle from moving forwards or backwards
        PxVec3 vel = v.getRigidBody()->getLinearVelocity();
        vel.x = 0;
        v.getRigidBody()->setLinearVelocity(vel);

        // prevent vehicle from rotating in a way that would disturb the simulation
        PxVec3 angularVel = v.getRigidBody()->getAngularVelocity();
        angularVel.z = 0.f;
        v.getRigidBody()->setAngularVelocity(angularVel);

        // apply downforce which will affect grip
        f32 downforce = (forwardDownforce * 0.65f + constantDownforce) * 25.f;
        PxVec3 down = v.getRigidBody()->getGlobalPose().q.getBasisVector2() * -1.f;
        v.getRigidBody()->addForce(down * downforce, PxForceMode::eACCELERATION);

        f32 magnitude = v.getRigidBody()->getLinearVelocity().y;
        if (magnitude < 0.1f)
        {
            f32 time = iterations * timestep;
            stats.grip = clamp((0.8f - time) / 0.8f, 0.f, 1.f);
            //print("Stop time: ", time, '\n');
            break;
        }
    }

    // offroad acceleration
    v.reset(startTransform);
    shape->setQueryFilterData(
            PxFilterData(COLLISION_FLAG_TERRAIN, 0, 0, DRIVABLE_SURFACE));
    shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_TERRAIN, -1, 0, 0));
    shape->setMaterials(&g_game.physx.materials.offroad, 1);
    for (u32 iterations=0; iterations<2000; ++iterations)
    {
        physicsScene->simulate(timestep);
        physicsScene->fetchResults(true);

        v.update(physicsScene, timestep, true, 1.f, 0.f, 0.f, false, true, false);
        if (v.getForwardSpeed() >= 16.f)
        {
            f32 time = iterations * timestep;
            stats.offroad = clamp((8.f - time) / 8.f, 0.f, 1.f);
            //print("Offroad acceleration time: ", time, '\n');
            break;
        }
    }

    actor->release();

    return stats;
}

void VehicleData::initStandardUpgrades()
{
    availableUpgrades = {
        {
            "ENGINE",
            "Upgrade the engine to improve acceleration and top speed.",
            g_res.getTexture("icon_pistons"),
            PerformanceUpgradeType::ENGINE,
            5,
            1500,
        },
        {
            "TIRES",
            "Equip better tires for improved traction and overall handling.",
            g_res.getTexture("icon_wheel"),
            PerformanceUpgradeType::TIRES,
            5,
            1000,
        },
        {
            "ARMOR",
            "Upgrade armor to improve resistance against all forms of damage.",
            g_res.getTexture("icon_armor"),
            PerformanceUpgradeType::ARMOR,
            5,
            1000,
        },
        {
            "SUSPENSION",
            "Upgrade the suspension to improve handling.",
            g_res.getTexture("icon_suspension"),
            PerformanceUpgradeType::SUSPENSION,
            2,
            1250,
        },
        {
            "WEIGHT",
            "Strips out unnecessary parts of the vehicle to improve acceleration and handling.",
            g_res.getTexture("icon_weight"),
            PerformanceUpgradeType::WEIGHT_REDUCTION,
            2,
            1500,
        },
    };
}

VehicleConfiguration::Upgrade* VehicleConfiguration::getUpgrade(i32 upgradeIndex)
{
    return performanceUpgrades.find([&](auto& u) { return u.upgradeIndex == upgradeIndex; });
}

bool VehicleConfiguration::canAddUpgrade(struct Driver* driver, i32 upgradeIndex)
{
    Upgrade* upgrade = getUpgrade(upgradeIndex);
    i32 upgradeLevel = upgrade ? (upgrade->upgradeLevel + 1) : 1;
    auto& upgradeInfo = driver->getVehicleData()->availableUpgrades[upgradeIndex];
    if (upgradeLevel > upgradeInfo.maxUpgradeLevel)
    {
        return false;
    }
    return true;
}

void VehicleConfiguration::addUpgrade(i32 upgradeIndex)
{
    Upgrade* upgrade = getUpgrade(upgradeIndex);
    if (!upgrade)
    {
        performanceUpgrades.push_back({ upgradeIndex, 1 });
    }
    else
    {
        ++upgrade->upgradeLevel;
    }
}

void VehicleData::loadModelData(const char* modelName)
{
    chassisBatch.begin();
    chassisOneMaterialBatch.begin();

    Model* model = g_res.getModel(modelName);

    for (auto& obj : model->objects)
    {
        Mat4 transform = obj.getTransform();
        std::string const& name = obj.name;
        Mesh* mesh = &model->meshes[obj.meshIndex];
        u32 debrisCollectionIndex = UINT32_MAX;
        for (i32 i=0; i<(i32)model->collections.size(); ++i)
        {
            if (model->collections[i].name.find("Debris") != std::string::npos)
            {
                debrisCollectionIndex = i;
                break;
            }
        }
        bool isInDebrisCollection = !!obj.collectionIndexes.find(debrisCollectionIndex);
        if (name.find("debris") != std::string::npos ||
            name.find("Debris") != std::string::npos ||
            name.find("FL") != std::string::npos ||
            name.find("FR") != std::string::npos ||
            name.find("RL") != std::string::npos ||
            name.find("RR") != std::string::npos ||
            isInDebrisCollection)
        {
            PxConvexMesh* collisionMesh = mesh->getConvexCollisionMesh();
            PxMaterial* material = g_game.physx.physics->createMaterial(0.3f, 0.3f, 0.1f);
            PxShape* shape = g_game.physx.physics->createShape(
                    PxConvexMeshGeometry(collisionMesh, PxMeshScale(convert(transform.scale()))), *material);
            shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_DEBRIS,
                        COLLISION_FLAG_TERRAIN | COLLISION_FLAG_OBJECT | COLLISION_FLAG_CHASSIS, 0, 0));
            material->release();
            debrisChunks.push_back({
                mesh,
                transform,
                shape,
                g_res.getMaterial(obj.materialGuid),
            });
        }

        bool isOnlyInDebrisCollection = isInDebrisCollection && obj.collectionIndexes.size() == 1;
        if (isOnlyInDebrisCollection)
        {
            continue;
        }
        if (name.find("Chassis") != std::string::npos)
        {
            Material* mat = g_res.getMaterial(obj.materialGuid);
            chassisBatch.add(mat, transform, mesh);
            chassisOneMaterialBatch.add(&g_res.defaultMaterial, transform, mesh);
        }
        if (name.find("FL") != std::string::npos)
        {
            wheelMeshes[WHEEL_FRONT_RIGHT].push_back({
                mesh,
                Mat4::scaling(obj.scale),
                nullptr,
                g_res.getMaterial(obj.materialGuid),
            });
            frontWheelMeshRadius = max(frontWheelMeshRadius, obj.bounds.z * 0.5f);
            frontWheelMeshWidth = max(frontWheelMeshWidth, obj.bounds.y);
            wheelPositions[WHEEL_FRONT_RIGHT] = transform.position();
        }
        else if (name.find("RL") != std::string::npos)
        {
            wheelMeshes[WHEEL_REAR_RIGHT].push_back({
                mesh,
                Mat4::scaling(transform.scale()),
                nullptr,
                g_res.getMaterial(obj.materialGuid),
            });
            rearWheelMeshRadius = max(rearWheelMeshRadius, obj.bounds.z * 0.5f);
            rearWheelMeshWidth = max(rearWheelMeshWidth, obj.bounds.y);
            wheelPositions[WHEEL_REAR_RIGHT] = transform.position();
        }
        else if (name.find("FR") != std::string::npos)
        {
            wheelMeshes[WHEEL_FRONT_LEFT].push_back({
                mesh,
                Mat4::scaling(obj.scale),
                nullptr,
                g_res.getMaterial(obj.materialGuid),
            });
            wheelPositions[WHEEL_FRONT_LEFT] = transform.position();
        }
        else if (name.find("RR") != std::string::npos)
        {
            wheelMeshes[WHEEL_REAR_LEFT].push_back({
                mesh,
                Mat4::scaling(obj.scale),
                nullptr,
                g_res.getMaterial(obj.materialGuid),
            });
            wheelPositions[WHEEL_REAR_LEFT] = obj.position;
        }
        else if (name.find("Collision") != std::string::npos)
        {
            collisionMeshes.push_back({ mesh->getConvexCollisionMesh(), transform });
            collisionWidth = max(collisionWidth, obj.bounds.y);
        }
        else if (name.find("COM") != std::string::npos)
        {
            sceneCenterOfMass = obj.position;
        }
        else if (name.find("WeaponMount1") != std::string::npos)
        {
            weaponMounts[0] = Mat4::translation(obj.position) * transform.rotation();
        }
        else if (name.find("WeaponMount2") != std::string::npos)
        {
            weaponMounts[1] = Mat4::translation(obj.position) * transform.rotation();
        }
        else if (name.find("WeaponMount3") != std::string::npos)
        {
            weaponMounts[2] = Mat4::translation(obj.position) * transform.rotation();
        }
        else if (name.find("ExhaustHole") != std::string::npos)
        {
            exhaustHoles.push_back(obj.position);
        }
    }

    chassisBatch.end(true);
    chassisOneMaterialBatch.end();
}

void VehicleData::copySceneDataToTuning(VehicleTuning& tuning)
{
    for (u32 i=0; i<ARRAY_SIZE(wheelPositions); ++i)
    {
        tuning.wheelPositions[i] = wheelPositions[i];
    }
    tuning.collisionWidth = collisionWidth;
    tuning.wheelWidthFront = frontWheelMeshWidth;
    tuning.wheelWidthRear = rearWheelMeshWidth;
    tuning.wheelRadiusFront = frontWheelMeshRadius;
    tuning.wheelRadiusRear = rearWheelMeshRadius;
    tuning.centerOfMass = sceneCenterOfMass;
    tuning.collisionMeshes = collisionMeshes;
}

void VehicleData::render(RenderWorld* rw, Mat4 const& transform,
        Mat4* wheelTransforms, VehicleConfiguration& config, Vehicle* vehicle,
        bool isBraking, bool isHidden, Vec4 const& shield)
{
    Material* originalPaintMaterial = g_res.getMaterial("paint_material");
    if (config.dirty)
    {
        config.reloadMaterials();
        config.dirty = false;
    }

    Mat4 defaultWheelTransforms[NUM_WHEELS];
    if (!wheelTransforms)
    {
        for (u32 i=0; i<NUM_WHEELS; ++i)
        {
            defaultWheelTransforms[i] = Mat4::translation(this->wheelPositions[i]);
        }
    }

    for (auto& m : chassisBatch.batches)
    {
        if (m.material == originalPaintMaterial)
        {
            config.paintMaterial.drawVehicle(rw, transform, &m.mesh, 2, shield,
                    config.wrapTextureGuids, config.wrapColors);
        }
        else
        {
            m.material->draw(rw, transform, &m.mesh, 2);
        }
    }
    if (isHidden)
    {
        for (auto& m : chassisOneMaterialBatch.batches)
        {
            g_res.defaultMaterial.drawHighlight(rw, transform, &m.mesh, 0, (u8)vehicle->cameraIndex);
        }
    }
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        Mat4 wheelTransform = transform *
            (wheelTransforms ? wheelTransforms[i] : defaultWheelTransforms[i]);
        if ((i & 1) == 0)
        {
            wheelTransform = wheelTransform * Mat4::rotationZ(PI);
        }

        for (auto& m : wheelMeshes[i])
        {
            Mat4 transform = wheelTransform * m.transform;
            m.material->draw(rw, transform, m.mesh, 2);
            if (isHidden)
            {
                g_res.defaultMaterial.drawHighlight(rw, transform, m.mesh, 0, (u8)vehicle->cameraIndex);
            }
        }
    }

    if (vehicle)
    {
        for (auto& w : vehicle->frontWeapons)
        {
            w->render(rw, transform, config, *this);
        }
        for (auto& w : vehicle->rearWeapons)
        {
            w->render(rw, transform, config, *this);
        }
        if (vehicle->specialAbility)
        {
            vehicle->specialAbility->render(rw, transform, config, *this);
        }
    }
    else
    {
        for (u32 i=0; i<ARRAY_SIZE(VehicleConfiguration::frontWeaponIndices); ++i)
        {
            if (config.frontWeaponIndices[i] != -1)
            {
                auto w = g_weapons[config.frontWeaponIndices[i]].create();
                w->upgradeLevel = config.frontWeaponUpgradeLevel[i];
                w->mountTransform = weaponMounts[i];
                w->refillAmmo();
                w->render(rw, transform, config, *this);
            }
        }
        for (u32 i=0; i<ARRAY_SIZE(VehicleConfiguration::rearWeaponIndices); ++i)
        {
            if (config.rearWeaponIndices[i] != -1)
            {
                auto w = g_weapons[config.rearWeaponIndices[i]].create();
                w->upgradeLevel = config.rearWeaponUpgradeLevel[i];
                w->refillAmmo();
                w->render(rw, transform, config, *this);
            }
        }
        if (config.specialAbilityIndex != -1)
        {
            auto w = g_weapons[config.specialAbilityIndex].create();
            w->render(rw, transform, config, *this);
        }
    }
}

void VehicleData::renderDebris(RenderWorld* rw,
        Array<VehicleDebris> const& debris, VehicleConfiguration& config)
{
    Material* originalPaintMaterial = g_res.getMaterial("paint_material");
    for (auto const& d : debris)
    {
        Mat4 scale = Mat4::scaling(d.meshInfo->transform.scale());
        Mat4 transform = Mat4(d.rigidBody->getGlobalPose()) * scale;
        if (d.meshInfo->material == originalPaintMaterial)
        {
            config.paintMaterial.drawVehicle(rw, transform, d.meshInfo->mesh, 0, Vec4(0.f),
                    config.wrapTextureGuids, config.wrapColors);
        }
        else
        {
            d.meshInfo->material->draw(rw, transform, d.meshInfo->mesh, 0);
        }
    }
}

#include "weapons/blaster.h"
#include "weapons/phantom.h"
#include "weapons/machinegun.h"
#include "weapons/explosive_mine.h"
#include "weapons/jumpjets.h"
#include "weapons/rocket_booster.h"
#include "weapons/ram_booster.h"
#include "weapons/underplating.h"
#include "weapons/kinetic_armor.h"
#include "weapons/missiles.h"
#include "weapons/bouncer.h"
#include "weapons/oil.h"
#include "weapons/glue.h"

#include "vehicles/mini.h"
#include "vehicles/sportscar.h"
#include "vehicles/racecar.h"
#include "vehicles/truck.h"
#include "vehicles/stationwagon.h"
#include "vehicles/coolcar.h"
#include "vehicles/muscle.h"

void initializeVehicleData()
{
    registerWeapon<WBlaster>();
    registerWeapon<WMachineGun>();
    registerWeapon<WMissiles>();
    registerWeapon<WBouncer>();
    registerWeapon<WExplosiveMine>();
    registerWeapon<WJumpJets>();
    registerWeapon<WRocketBooster>();
    registerWeapon<WRamBooster>();
    registerWeapon<WOil>();
    registerWeapon<WGlue>();
    registerWeapon<WUnderPlating>();
    registerWeapon<WKineticArmor>();
    registerWeapon<WPhantom>();

    //registerVehicle<VMini>();
    registerVehicle<VStationWagon>();
    registerVehicle<VCoolCar>();
    registerVehicle<VMuscle>();
    //registerVehicle<VSportscar>();
    //registerVehicle<VTruck>();
    //registerVehicle<VRacecar>();

    registerAI("Vendetta",        1.f,   0.5f, 1.f,  1.f,   srgb(0.75f, 0.01f, 0.01f),   "Station Wagon", -1);
    registerAI("Dumb Dumb",       0.f,   0.f,  0.f,  0.f,   srgb(0.75f, 0.01f, 0.01f),   "Muscle Car", 1);
    registerAI("Rad Racer",       0.5f,  0.5f, 0.6f, 0.25f, srgb(0.95f, 0.47f, 0.02f),   "Cool Car", 1);
    registerAI("Me First",        0.9f,  0.1f, 0.1f, 0.1f,  srgb(0.9f, 0.9f, 0.f),       "Cool Car", 2, Vec4(0,0,0,1));
    registerAI("Automosqueal",    0.5f,  1.f,  1.f,  0.25f, srgb(0.01f, 0.01f, 0.85f),   "Muscle Car", 2, Vec4(0.7,0.7,0.01,1));
    registerAI("Rocketeer",       0.25f, 1.f,  0.1f, 0.f,   srgb(0.01f, 0.01f, 0.3f),    "Muscle Car", 1, Vec4(0.5,0,0,1));
    registerAI("Zoom-Zoom",       1.f,   0.1f, 0.8f, 1.f,   srgb(0.01f, 0.7f, 0.8f),     "Station Wagon", -1);
    registerAI("Octane",          0.7f,  0.2f, 0.2f, 0.2f,  srgb(0.91f, 0.91f, 0.91f),   "Cool Car", 2, Vec4(0,0,0,1));
    registerAI("Joe Blow",        0.5f,  0.5f, 0.5f, 0.5f,  srgb(0.03f, 0.03f, 0.03f),   "Station Wagon", 0);
    registerAI("Square Triangle", 0.3f,  0.4f, 0.1f, 0.7f,  srgb(0.01f, 0.75f, 0.01f),   "Muscle Car", -1);
    registerAI("Questionable",    0.4f,  0.6f, 0.6f, 0.7f,  srgb(0.42f, 0.015f, 0.015f), "Cool Car", 0);
    registerAI("McCarface",       0.9f,  0.9f, 0.f,  0.1f,  srgb(0.2f, 0.1f, 0.06f),     "Station Wagon", -1);
}
