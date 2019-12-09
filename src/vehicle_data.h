#pragma once

#include "math.h"
#include "datafile.h"
#include "mesh.h"
#include "weapon.h"
#include <algorithm>
#include <functional>

#define WHEEL_FRONT_LEFT  PxVehicleDrive4WWheelOrder::eFRONT_LEFT
#define WHEEL_FRONT_RIGHT PxVehicleDrive4WWheelOrder::eFRONT_RIGHT
#define WHEEL_REAR_LEFT   PxVehicleDrive4WWheelOrder::eREAR_LEFT
#define WHEEL_REAR_RIGHT  PxVehicleDrive4WWheelOrder::eREAR_RIGHT
#define NUM_WHEELS 4

struct ComputerDriverData
{
    const char* name = "Computer Driver";
    f32 drivingSkill = 1.f; // [0,1] how optimal of a path the AI takes on the track
    f32 aggression = 1.f;   // [0,1] how likely the AI is to go out of its way to attack other drivers
    f32 awareness = 1.f;    // [0,1] how likely the AI is to attempt to avoid hitting other drivers and obstacles
    f32 fear = 1.f;         // [0,1] how much the AI tries to evade other drivers
};

struct RegisteredWeapon
{
    WeaponInfo info;
    std::function<std::unique_ptr<Weapon>()> create;
};

struct VehicleMesh
{
    Mesh* mesh;
    glm::mat4 transform;
    PxShape* collisionShape;
    bool isBody;
};

struct VehicleDebris
{
    VehicleMesh* meshInfo;
    PxRigidDynamic* rigidBody;
    f32 life = 0.f;
};

struct VehicleCollisionsMesh
{
    PxConvexMesh* convexMesh;
    glm::mat4 transform;
};

struct VehicleTuning
{
    std::vector<VehicleCollisionsMesh> collisionMeshes;

    f32 chassisMass = 1400.f;
    glm::vec3 centerOfMass = { 0.f, 0.f, -0.2f };

    f32 wheelMassFront = 30.f;
    f32 wheelWidthFront = 0.4f;
    f32 wheelRadiusFront = 0.6f;
    f32 wheelMassRear = 30.f;
    f32 wheelWidthRear = 0.4f;
    f32 wheelRadiusRear = 0.6f;

    f32 wheelDampingRate = 0.25f;
    f32 wheelOffroadDampingRate = 15.f;
    f32 trackTireFriction = 3.f;
    f32 offroadTireFriction = 2.f;
    f32 frontToeAngle = 0.f;
    f32 rearToeAngle = 0.f;

    f32 rearTireGripPercent = 1.f;
    f32 topSpeed = 33.f;
    f32 constantDownforce = 0.005f;
    f32 forwardDownforce = 0.005f;
    f32 driftBoost = 0.f;

    f32 maxEngineOmega = 600.f;
    f32 peekEngineTorque = 800.f;
    f32 engineDampingFullThrottle = 0.15f;
    f32 engineDampingZeroThrottleClutchEngaged = 2.f;
    f32 engineDampingZeroThrottleClutchDisengaged = 0.35f;
    f32 maxHandbrakeTorque = 10000.f;
    f32 maxBrakeTorque = 10000.f;
    f32 maxSteerAngle = PI * 0.33f;
    f32 clutchStrength = 10.f;
    f32 gearSwitchTime = 0.2f;
    f32 autoBoxSwitchTime = 0.25f;

    // reverse, neutral, first, second, third...
    SmallVec<f32, 12> gearRatios = { -4.f, 0.f, 4.f, 2.f, 1.5f, 1.1f, 1.f };
    f32 finalGearRatio = 4.f;

    f32 suspensionMaxCompression = 0.2f;
    f32 suspensionMaxDroop = 0.3f;
    f32 suspensionSpringStrength = 35000.0f;
    f32 suspensionSpringDamperRate = 4500.0f;

    f32 camberAngleAtRest = 0.f;
    f32 camberAngleAtMaxDroop = 0.01f;
    f32 camberAngleAtMaxCompression = -0.01f;

    f32 frontAntiRollbarStiffness = 10000.0f;
    f32 rearAntiRollbarStiffness = 10000.0f;

    f32 ackermannAccuracy = 1.f;

    PxVehicleDifferential4WData::Enum differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;

    glm::vec3 wheelPositions[NUM_WHEELS];

    // all [0,1]
    struct
    {
        f32 acceleration = 0.f;
        f32 handling = 0.f;
        f32 offroad = 0.f;
    } specs;

    f32 collisionWidth = 0.f;
    f32 maxHitPoints;

    f32 getRestOffset() const
    {
        f32 wheelZ = -wheelPositions[0].z;
        return wheelZ + wheelRadiusFront;
    }
};

glm::vec3 g_vehicleColors[] = {
    { 0.9f, 0.9f, 0.9f },
    { 0.7f, 0.01f, 0.01f },
    { 0.02f, 0.02f, 0.02f },
    { 0.01f, 0.8f, 0.01f },
    { 0.01f, 0.01f, 0.8f },
    { 0.8f, 0.01f, 0.8f },
    { 0.01f, 0.8f, 0.8f },
    { 0.5f, 0.2f, 0.8f },
    { 0.9f, 0.01f, 0.5f },
    { 0.02f, 0.7f, 0.1f },
    { 0.9f, 0.45f, 0.01f },
};

std::string g_vehicleColorNames[] = {
    "White", "Red", "Black", "Green", "Blue", "Something0",
    "Something1", "Something2", "Something3",
    "Something4", "Orange"
};

struct VehicleConfiguration
{
    i32 colorIndex = 0;

    i32 frontWeaponIndices[3] = { -1, -1, -1 };
    u32 frontWeaponUpgradeLevel[3] = { 0, 0, 0 };
    i32 rearWeaponIndices[3] = { -1, -1, -1 };
    u32 rearWeaponUpgradeLevel[3] = { 0, 0, 0 };
    i32 specialAbilityIndex = -1;

    struct Upgrade
    {
        i32 upgradeIndex;
        i32 upgradeLevel;
    };
    std::vector<Upgrade> performanceUpgrades;

    Upgrade* getUpgrade(i32 upgradeIndex);
    bool canAddUpgrade(struct Driver* driver, i32 upgradeIndex);
    void addUpgrade(i32 upgradeIndex);

    DataFile::Value serialize()
    {
        DataFile::Value data = DataFile::makeDict();
        data["colorIndex"] = DataFile::makeInteger(colorIndex);
        data["frontWeapon0"] = DataFile::makeInteger(frontWeaponIndices[0]);
        data["frontWeapon1"] = DataFile::makeInteger(frontWeaponIndices[1]);
        data["frontWeapon2"] = DataFile::makeInteger(frontWeaponIndices[2]);
        data["frontWeapon0UpgradeLevel"] = DataFile::makeInteger(frontWeaponUpgradeLevel[0]);
        data["frontWeapon1UpgradeLevel"] = DataFile::makeInteger(frontWeaponUpgradeLevel[1]);
        data["frontWeapon2UpgradeLevel"] = DataFile::makeInteger(frontWeaponUpgradeLevel[2]);
        data["rearWeapon0"] = DataFile::makeInteger(rearWeaponIndices[0]);
        data["rearWeapon1"] = DataFile::makeInteger(rearWeaponIndices[1]);
        data["rearWeapon2"] = DataFile::makeInteger(rearWeaponIndices[2]);
        data["rearWeapon0UpgradeLevel"] = DataFile::makeInteger(rearWeaponUpgradeLevel[0]);
        data["rearWeapon1UpgradeLevel"] = DataFile::makeInteger(rearWeaponUpgradeLevel[1]);
        data["rearWeapon2UpgradeLevel"] = DataFile::makeInteger(rearWeaponUpgradeLevel[2]);
        data["specialAbilityIndex"] = DataFile::makeInteger(specialAbilityIndex);
        data["performanceUpgrades"] = DataFile::makeArray();
        for (auto& u : performanceUpgrades)
        {
            DataFile::Value upgrade = DataFile::makeDict();
            upgrade["upgradeIndex"] = DataFile::makeInteger(u.upgradeIndex);
            upgrade["upgradeLevel"] = DataFile::makeInteger(u.upgradeLevel);
            data["performanceUpgrades"].array().push_back(upgrade);
        }
        return data;
    }

    void deserialize(DataFile::Value& data)
    {
        colorIndex = (i32)data["colorIndex"].integer();
        frontWeaponIndices[0] = (i32)data["frontWeapon0"].integer();
        frontWeaponIndices[1] = (i32)data["frontWeapon1"].integer();
        frontWeaponIndices[2] = (i32)data["frontWeapon2"].integer();
        frontWeaponUpgradeLevel[0] = (u32)data["frontWeapon0UpgradeLevel"].integer();
        frontWeaponUpgradeLevel[1] = (u32)data["frontWeapon1UpgradeLevel"].integer();
        frontWeaponUpgradeLevel[2] = (u32)data["frontWeapon2UpgradeLevel"].integer();
        rearWeaponIndices[0] = (i32)data["rearWeapon0"].integer();
        rearWeaponIndices[1] = (i32)data["rearWeapon1"].integer();
        rearWeaponIndices[2] = (i32)data["rearWeapon2"].integer();
        rearWeaponUpgradeLevel[0] = (u32)data["rearWeapon0UpgradeLevel"].integer();
        rearWeaponUpgradeLevel[1] = (u32)data["rearWeapon1UpgradeLevel"].integer();
        rearWeaponUpgradeLevel[2] = (u32)data["rearWeapon2UpgradeLevel"].integer();
        specialAbilityIndex = (i32)data["specialAbilityIndex"].integer();
        auto& upgrades = data["performanceUpgrades"].array();
        for (auto& u : upgrades)
        {
            Upgrade upgrade;
            upgrade.upgradeIndex = (i32)u["upgradeIndex"].integer();
            upgrade.upgradeLevel = (i32)u["upgradeLevel"].integer();
            performanceUpgrades.push_back(upgrade);
        }
    }
};

enum struct PerformanceUpgradeType
{
    ENGINE,
    ARMOR,
    TIRES,
    SUSPENSION,
    AERODYNAMICS,
    TRANSMISSION,
    WEIGHT_REDUCTION,
    UNDER_PLATING,
    ALL_WHEEL_DRIVE,
};

struct PerformanceUpgrade
{
    const char* name;
    const char* description;
    const char* icon;
    PerformanceUpgradeType upgradeType;
    i32 maxUpgradeLevel = 5;
    i32 price;
};

struct VehicleData
{
    SmallVec<VehicleMesh, 128> chassisMeshes;
    VehicleMesh wheelMeshFront;
    VehicleMesh wheelMeshRear;

    glm::vec3 wheelPositions[NUM_WHEELS] = {};
    f32 frontWheelMeshRadius = 0.f;
    f32 frontWheelMeshWidth = 0.f;
    f32 rearWheelMeshRadius = 0.f;
    f32 rearWheelMeshWidth = 0.f;
    f32 collisionWidth = 0.f;
    glm::vec3 sceneCenterOfMass = glm::vec3(0.f);

    std::vector<VehicleCollisionsMesh> collisionMeshes;
    std::vector<PerformanceUpgrade> availableUpgrades;

    const char* name ="";
    const char* description ="";
    i32 price = 0;
    u32 frontWeaponCount = 1;
    u32 rearWeaponCount = 1;

    std::vector<VehicleMesh> debrisChunks;

    virtual ~VehicleData() {}
    virtual void render(class RenderWorld* rw, glm::mat4 const& transform,
            glm::mat4* wheelTransforms, VehicleConfiguration const& config);
    virtual void renderDebris(class RenderWorld* rw,
            std::vector<VehicleDebris> const& debris, VehicleConfiguration const& config);

    virtual void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) = 0;
    void loadSceneData(const char* sceneName);
    void copySceneDataToTuning(VehicleTuning& tuning);
};

std::vector<std::unique_ptr<VehicleData>> g_vehicles;
std::vector<RegisteredWeapon> g_weapons;
std::vector<ComputerDriverData> g_ais;

template <typename T>
void registerWeapon()
{
    g_weapons.push_back({
        (T().info),
        [] { return std::make_unique<T>(); }
    });
}

template <typename T>
void registerVehicle()
{
    g_vehicles.push_back(std::make_unique<T>());
}

void registerAI(const char* name, f32 drivingSkill, f32 aggression, f32 awareness, f32 fear)
{
    g_ais.push_back({ name, drivingSkill, aggression, awareness, fear });
}

void initializeVehicleData();
