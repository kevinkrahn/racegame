#include "vehicle_data.h"
#include "game.h"
#include "resources.h"
#include "scene.h"
#include "mesh_renderables.h"

VehicleConfiguration::Upgrade* VehicleConfiguration::getUpgrade(i32 upgradeIndex)
{
    auto currentUpgrade = std::find_if(
            performanceUpgrades.begin(),
            performanceUpgrades.end(),
            [&](auto& u) { return u.upgradeIndex == upgradeIndex; });
    return currentUpgrade != performanceUpgrades.end() ? &(*currentUpgrade) : nullptr;
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

void VehicleData::loadSceneData(const char* sceneName)
{
    DataFile::Value::Dict& scene = g_resources.getScene(sceneName);
    for (auto& e : scene["entities"].array())
    {
        std::string name = e["name"].string();
        glm::mat4 transform = e["matrix"].convertBytes<glm::mat4>();
        if (name.find("debris") != std::string::npos ||
            name.find("Debris") != std::string::npos ||
            name.find("FL") != std::string::npos ||
            name.find("FR") != std::string::npos ||
            name.find("RL") != std::string::npos ||
            name.find("RR") != std::string::npos)
        {
            std::string const& meshName = e["data_name"].string();
            Mesh* mesh = g_resources.getMesh(meshName.c_str());
            PxConvexMesh* collisionMesh = mesh->getConvexCollisionMesh();
            PxMaterial* material = g_game.physx.physics->createMaterial(0.3f, 0.3f, 0.1f);
            PxShape* shape = g_game.physx.physics->createShape(
                    PxConvexMeshGeometry(collisionMesh, PxMeshScale(convert(scaleOf(transform)))), *material);
            shape->setSimulationFilterData(PxFilterData(COLLISION_FLAG_DEBRIS,
                        COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS, 0, 0));
            material->release();
            debrisChunks.push_back({
                mesh,
                transform,
                shape,
                name.find("Body") != std::string::npos
            });
        }
        else if (name.find("Chassis") != std::string::npos)
        {
            chassisMeshes.push_back({
                g_resources.getMesh(e["data_name"].string().c_str()),
                transform,
                nullptr,
                name.find("Body") != std::string::npos
            });
        }
        if (name.find("FL") != std::string::npos)
        {
            wheelMeshFront = {
                g_resources.getMesh(e["data_name"].string().c_str()),
                glm::scale(glm::mat4(1.f), scaleOf(transform))
            };
            frontWheelMeshRadius = e["bound_z"].real() * 0.5f;
            frontWheelMeshWidth = e["bound_y"].real();
            wheelPositions[WHEEL_FRONT_RIGHT] = transform[3];
        }
        else if (name.find("RL") != std::string::npos)
        {
            wheelMeshRear = {
                g_resources.getMesh(e["data_name"].string().c_str()),
                glm::scale(glm::mat4(1.f), scaleOf(transform))
            };
            rearWheelMeshRadius = e["bound_z"].real() * 0.5f;
            rearWheelMeshWidth = e["bound_y"].real();
            wheelPositions[WHEEL_REAR_RIGHT] = transform[3];
        }
        else if (name.find("FR") != std::string::npos)
        {
            wheelPositions[WHEEL_FRONT_LEFT] = transform[3];
        }
        else if (name.find("RR") != std::string::npos)
        {
            wheelPositions[WHEEL_REAR_LEFT] = transform[3];
        }
        else if (name.find("Collision") != std::string::npos)
        {
            collisionMeshes.push_back({
                g_resources.getMesh(e["data_name"].string().c_str())->getConvexCollisionMesh(),
                transform
            });
            collisionWidth =
                glm::max(collisionWidth, (f32)e["bound_y"].real());
        }
        else if (name.find("COM") != std::string::npos)
        {
            sceneCenterOfMass = transform[3];
        }
    }
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

void VehicleData::render(RenderWorld* rw, glm::mat4 const& transform,
        glm::mat4* wheelTransforms, VehicleConfiguration const& config)
{
    for (auto& m : chassisMeshes)
    {
        LitSettings s;
        s.mesh = m.mesh;
        s.worldTransform = transform * m.transform;
        s.texture = nullptr;
        s.color = m.isBody ? g_vehicleColors[config.colorIndex] : glm::vec3(1.f);
        s.specularStrength = m.isBody ? 0.15f : 0.3f;
        s.specularPower = m.isBody ? 20.f : 200.f;
        rw->push(LitRenderable(s));
    }

    glm::mat4 defaultWheelTransforms[NUM_WHEELS];
    if (!wheelTransforms)
    {
        for (u32 i=0; i<NUM_WHEELS; ++i)
        {
            defaultWheelTransforms[i] = glm::translate(glm::mat4(1.f),
                    this->wheelPositions[i]);
        }
    }

    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        glm::mat4 wheelTransform = transform *
            (wheelTransforms ? wheelTransforms[i] : defaultWheelTransforms[i]);
        if ((i & 1) == 0)
        {
            wheelTransform = glm::rotate(wheelTransform, PI, glm::vec3(0, 0, 1));
        }
        auto& mesh = i < 2 ? wheelMeshFront : wheelMeshRear;
        rw->push(LitRenderable(mesh.mesh, wheelTransform * mesh.transform, nullptr));
    }
}

void VehicleData::renderDebris(RenderWorld* rw,
        std::vector<VehicleDebris> const& debris, VehicleConfiguration const& config)
{
    for (auto const& d : debris)
    {
        LitSettings s;
        s.mesh = d.meshInfo->mesh;
        s.worldTransform = convert(d.rigidBody->getGlobalPose());
        s.texture = nullptr;
        s.color = d.meshInfo->isBody ? g_vehicleColors[config.colorIndex] : glm::vec3(1.f);
        s.specularStrength = d.meshInfo->isBody ? 0.15f : 0.3f;
        s.specularPower = d.meshInfo->isBody ? 20.f : 200.f;
        rw->push(LitRenderable(s));
    }
}

#include "weapons/blaster.h"
#include "weapons/machinegun.h"
#include "weapons/explosive_mine.h"
#include "weapons/jumpjets.h"
#include "weapons/rocket_booster.h"
#include "weapons/ram_booster.h"
#include "weapons/missiles.h"
#include "weapons/bouncer.h"
#include "weapons/oil.h"

#include "vehicles/mini.h"
#include "vehicles/sportscar.h"
#include "vehicles/racecar.h"
#include "vehicles/truck.h"

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

    registerVehicle<VMini>();
    registerVehicle<VSportscar>();
    registerVehicle<VTruck>();
    registerVehicle<VRacecar>();

    registerAI("Vendetta",        1.f,   0.5f, 1.f,  1.f);
    registerAI("Dumb Dumb",       0.f,   0.f, 0.f,  0.f);
    registerAI("Rad Racer",       0.5f,  0.5f, 0.6f, 0.25f);
    registerAI("Me First",        0.9f,  0.1f, 0.1f, 0.1f);
    registerAI("Automosqueal",    0.5f,  1.f,  1.f,  0.25f);
    registerAI("Rocketeer",       0.25f, 1.f,  0.1f, 0.f);
    registerAI("Zoom-Zoom",       1.f,   0.1f, 0.8f, 1.f);
    registerAI("Octane",          0.7f,  0.2f, 0.2f, 0.2f);
    registerAI("Joe Blow",        0.5f,  0.5f, 0.5f, 0.5f);
    registerAI("Square Triangle", 0.3f,  0.4f, 0.1f, 0.7f);
    registerAI("Questionable",    0.4f,  0.6f, 0.6f, 0.7f);
    registerAI("McCarface",       0.9f,  0.9f, 0.f,  0.1f);
}
