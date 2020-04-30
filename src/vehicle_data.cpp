#include "vehicle_data.h"
#include "game.h"
#include "resources.h"
#include "scene.h"
#include "mesh_renderables.h"
#include "vehicle.h"

void VehicleData::initStandardUpgrades()
{
    availableUpgrades = {
        {
            "Engine",
            "Upgrades the engine to improve\nacceleration and top speed.",
            g_res.getTexture("icon_pistons"),
            PerformanceUpgradeType::ENGINE,
            5,
            1500,
        },
        {
            "Tires",
            "Equips better tires for improved traction\nand overall handling.",
            g_res.getTexture("icon_wheel"),
            PerformanceUpgradeType::TIRES,
            5,
            1000,
        },
        {
            "Armor",
            "Adds additional armor to improve\nresistance against all forms of damage.",
            g_res.getTexture("icon_armor"),
            PerformanceUpgradeType::ARMOR,
            5,
            1000,
        },
        {
            "Suspension",
            "Upgrades the suspension to be stiffer\nand more stable around corners.",
            g_res.getTexture("icon_suspension"),
            PerformanceUpgradeType::SUSPENSION,
            2,
            1250,
        },
        {
            "Weight Reduction",
            "Strips out unnecessary parts of the vehicle.\nThe reduced weight will improve acceleration and handling.",
            g_res.getTexture("icon_weight"),
            PerformanceUpgradeType::WEIGHT_REDUCTION,
            2,
            1500,
        },
    };
}

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

void VehicleData::loadModelData(const char* modelName)
{
    Model* model = g_res.getModel(modelName);

    for (auto& obj : model->objects)
    {
        glm::mat4 transform = obj.getTransform();
        std::string const& name = obj.name;
        Mesh* mesh = &model->meshes[obj.meshIndex];
        if (name.find("debris") != std::string::npos ||
            name.find("Debris") != std::string::npos ||
            name.find("FL") != std::string::npos ||
            name.find("FR") != std::string::npos ||
            name.find("RL") != std::string::npos ||
            name.find("RR") != std::string::npos)
        {
            PxConvexMesh* collisionMesh = mesh->getConvexCollisionMesh();
            PxMaterial* material = g_game.physx.physics->createMaterial(0.3f, 0.3f, 0.1f);
            PxShape* shape = g_game.physx.physics->createShape(
                    PxConvexMeshGeometry(collisionMesh, PxMeshScale(convert(scaleOf(transform)))), *material);
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
        else if (name.find("Chassis") != std::string::npos)
        {
            chassisMeshes.push_back({
                mesh,
                transform,
                nullptr,
                g_res.getMaterial(obj.materialGuid),
            });
        }
        if (name.find("FL") != std::string::npos)
        {
            wheelMeshes[WHEEL_FRONT_RIGHT].push_back({
                mesh,
                glm::scale(glm::mat4(1.f), obj.scale),
                nullptr,
                g_res.getMaterial(obj.materialGuid),
            });
            frontWheelMeshRadius = glm::max(frontWheelMeshRadius, obj.bounds.z * 0.5f);
            frontWheelMeshWidth = glm::max(frontWheelMeshWidth, obj.bounds.y);
            wheelPositions[WHEEL_FRONT_RIGHT] = transform[3];
        }
        else if (name.find("RL") != std::string::npos)
        {
            wheelMeshes[WHEEL_REAR_RIGHT].push_back({
                mesh,
                glm::scale(glm::mat4(1.f), scaleOf(transform)),
                nullptr,
                g_res.getMaterial(obj.materialGuid),
            });
            rearWheelMeshRadius = glm::max(rearWheelMeshRadius, obj.bounds.z * 0.5f);
            rearWheelMeshWidth = glm::max(rearWheelMeshWidth, obj.bounds.y);
            wheelPositions[WHEEL_REAR_RIGHT] = transform[3];
        }
        else if (name.find("FR") != std::string::npos)
        {
            wheelMeshes[WHEEL_FRONT_LEFT].push_back({
                mesh,
                glm::scale(glm::mat4(1.f), obj.scale),
                nullptr,
                g_res.getMaterial(obj.materialGuid),
            });
            wheelPositions[WHEEL_FRONT_LEFT] = transform[3];
        }
        else if (name.find("RR") != std::string::npos)
        {
            wheelMeshes[WHEEL_REAR_LEFT].push_back({
                mesh,
                glm::scale(glm::mat4(1.f), obj.scale),
                nullptr,
                g_res.getMaterial(obj.materialGuid),
            });
            wheelPositions[WHEEL_REAR_LEFT] = obj.position;
        }
        else if (name.find("Collision") != std::string::npos)
        {
            collisionMeshes.push_back({ mesh->getConvexCollisionMesh(), transform });
            collisionWidth = glm::max(collisionWidth, obj.bounds.y);
        }
        else if (name.find("COM") != std::string::npos)
        {
            sceneCenterOfMass = obj.position;
        }
        else if (name.find("WeaponMount1") != std::string::npos)
        {
            weaponMounts[0] = glm::translate(glm::mat4(1.f), obj.position) * rotationOf(transform);
        }
        else if (name.find("WeaponMount2") != std::string::npos)
        {
            weaponMounts[1] = glm::translate(glm::mat4(1.f), obj.position) * rotationOf(transform);
        }
        else if (name.find("WeaponMount3") != std::string::npos)
        {
            weaponMounts[2] = glm::translate(glm::mat4(1.f), obj.position) * rotationOf(transform);
        }
        else if (name.find("ExhaustHole") != std::string::npos)
        {
            exhaustHoles.push_back(obj.position);
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
        glm::mat4* wheelTransforms, VehicleConfiguration const& config, Vehicle* vehicle,
        bool isBraking)
{
    Material* originalPaintMaterial = g_res.getMaterial("paint_material");
    coloredPaintMaterial = *originalPaintMaterial;
    coloredPaintMaterial.color = g_vehicleColors[config.colorIndex];

    for (auto& m : chassisMeshes)
    {
        Material* mat = m.material;
        if (mat == originalPaintMaterial)
        {
            mat = &coloredPaintMaterial;
        }
        rw->push(LitMaterialRenderable(m.mesh, transform * m.transform, mat));
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

        for (auto& m : wheelMeshes[i])
        {
            Material* mat = m.material;
            if (mat == originalPaintMaterial)
            {
                mat = &coloredPaintMaterial;
            }
            rw->push(LitMaterialRenderable(m.mesh, wheelTransform * m.transform, mat));
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
        std::vector<VehicleDebris> const& debris, VehicleConfiguration const& config)
{
    Material* originalPaintMaterial = g_res.getMaterial("paint_material");
    for (auto const& d : debris)
    {
        Material* mat = d.meshInfo->material;
        if (mat == originalPaintMaterial)
        {
            mat = &coloredPaintMaterial;
        }
        rw->push(LitMaterialRenderable(d.meshInfo->mesh, convert(d.rigidBody->getGlobalPose()), mat));
    }
}

#include "weapons/blaster.h"
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

    //registerVehicle<VMini>();
    registerVehicle<VStationWagon>();
    registerVehicle<VCoolCar>();
    registerVehicle<VMuscle>();
    //registerVehicle<VSportscar>();
    //registerVehicle<VTruck>();
    //registerVehicle<VRacecar>();

    registerAI("Vendetta",        1.f,   0.5f, 1.f,  1.f,   "Red",         "Station Wagon");
    registerAI("Dumb Dumb",       0.f,   0.f,  0.f,  0.f,   "Light Brown", "Muscle Car");
    registerAI("Rad Racer",       0.5f,  0.5f, 0.6f, 0.25f, "Orange",      "Cool Car");
    registerAI("Me First",        0.9f,  0.1f, 0.1f, 0.1f,  "Yellow",      "Cool Car");
    registerAI("Automosqueal",    0.5f,  1.f,  1.f,  0.25f, "Blue",        "Muscle Car");
    registerAI("Rocketeer",       0.25f, 1.f,  0.1f, 0.f,   "Dark Blue",   "Muscle Car");
    registerAI("Zoom-Zoom",       1.f,   0.1f, 0.8f, 1.f,   "Aruba",       "Station Wagon");
    registerAI("Octane",          0.7f,  0.2f, 0.2f, 0.2f,  "White",       "Cool Car");
    registerAI("Joe Blow",        0.5f,  0.5f, 0.5f, 0.5f,  "Black",       "Station Wagon");
    registerAI("Square Triangle", 0.3f,  0.4f, 0.1f, 0.7f,  "Green",       "Muscle Car");
    registerAI("Questionable",    0.4f,  0.6f, 0.6f, 0.7f,  "Maroon",      "Cool Car");
    registerAI("McCarface",       0.9f,  0.9f, 0.f,  0.1f,  "Dark Brown",  "Station Wagon");
}
