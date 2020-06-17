#pragma once

#include "math.h"
#include "datafile.h"
#include "mesh.h"
#include "weapon.h"
#include "material.h"
#include "batcher.h"

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
    glm::vec3 color;
    u32 vehicleIndex = 0;
    i32 wrapIndex = -1;
    glm::vec4 wrapColor;
};

struct RegisteredWeapon
{
    WeaponInfo info;
    std::function<OwnedPtr<Weapon>()> create;
};

struct VehicleMesh
{
    Mesh* mesh;
    glm::mat4 transform;
    PxShape* collisionShape;
    Material* material;
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

struct VehicleStats
{
    // all [0,1]
    f32 acceleration = 0.f;
    f32 topSpeed = 0.f;
    f32 armor = 0.f;
    f32 mass = 0.f;
    f32 grip = 0.f;
    f32 offroad = 0.f;
};

struct VehicleTuning
{
    Array<VehicleCollisionsMesh> collisionMeshes;

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
    SmallArray<f32, 12> gearRatios = { -4.f, 0.f, 4.f, 2.f, 1.5f, 1.1f, 1.f };
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

    f32 collisionWidth = 0.f;
    f32 maxHitPoints;

    f32 getRestOffset() const
    {
        f32 wheelZ = -wheelPositions[0].z;
        return wheelZ + wheelRadiusFront;
    }

    VehicleStats computeVehicleStats();
};

const char* g_wrapTextures[] = {
    "p_flames",
    "p_double_stripes",
    "p_splat",
    "p_test",
};

struct VehicleConfiguration
{
    glm::vec3 color = glm::vec3(0.95f);
    glm::vec3 hsv = glm::vec3(0.f, 0.f, 0.95f);
    f32 paintShininess = 1.f;

    i64 wrapTextureGuids[3] = { 0, 0, 0 };
    glm::vec4 wrapColors[3] = {};

    i32 frontWeaponIndices[3] = { -1, -1, -1 };
    u32 frontWeaponUpgradeLevel[3] = { 0, 0, 0 };
    i32 rearWeaponIndices[3] = { -1, -1, -1 };
    u32 rearWeaponUpgradeLevel[3] = { 0, 0, 0 };
    i32 specialAbilityIndex = -1;
    // TODO: should the special ability also have an upgrade level?

    struct Upgrade
    {
        i32 upgradeIndex;
        i32 upgradeLevel;

        void serialize(Serializer& s)
        {
            s.field(upgradeIndex);
            s.field(upgradeLevel);
        }
    };
    Array<Upgrade> performanceUpgrades;

    Upgrade* getUpgrade(i32 upgradeIndex);
    bool canAddUpgrade(struct Driver* driver, i32 upgradeIndex);
    void addUpgrade(i32 upgradeIndex);

    void serialize(Serializer& s)
    {
        s.field(color);
        s.field(hsv);
        s.field(paintShininess);
        s.field(wrapTextureGuids);
        s.field(wrapColors);
        s.field(frontWeaponIndices);
        s.field(frontWeaponUpgradeLevel);
        s.field(rearWeaponIndices);
        s.field(rearWeaponUpgradeLevel);
        s.field(specialAbilityIndex);
        s.field(performanceUpgrades);
    }

    bool dirty = true;
    Material paintMaterial;

    void reloadMaterials()
    {
        Material* originalPaintMaterial = g_res.getMaterial("paint_material");
        Material* mattePaintMaterial = g_res.getMaterial("paint_material_matte");
        paintMaterial = *originalPaintMaterial;
        paintMaterial.color = color;
        paintMaterial.loadShaderHandles({ {"VEHICLE"} });
        paintMaterial.reflectionLod = glm::lerp(mattePaintMaterial->reflectionLod, originalPaintMaterial->reflectionLod, paintShininess);
        paintMaterial.reflectionBias = glm::lerp(mattePaintMaterial->reflectionBias, originalPaintMaterial->reflectionBias, paintShininess);
        paintMaterial.reflectionStrength = glm::lerp(mattePaintMaterial->reflectionStrength, originalPaintMaterial->reflectionStrength, paintShininess);
        paintMaterial.specularPower = glm::lerp(mattePaintMaterial->specularPower, originalPaintMaterial->specularPower, paintShininess);
        paintMaterial.specularStrength = glm::lerp(mattePaintMaterial->specularStrength, originalPaintMaterial->specularStrength, paintShininess);
        paintMaterial.fresnelBias = glm::lerp(mattePaintMaterial->fresnelBias, originalPaintMaterial->fresnelBias, paintShininess);
        paintMaterial.fresnelPower = glm::lerp(mattePaintMaterial->fresnelPower, originalPaintMaterial->fresnelPower, paintShininess);
        paintMaterial.fresnelScale = glm::lerp(mattePaintMaterial->fresnelScale, originalPaintMaterial->fresnelScale, paintShininess);
        dirty = false;
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
    ALL_WHEEL_DRIVE,
};

struct PerformanceUpgrade
{
    const char* name;
    const char* description;
    Texture* icon;
    PerformanceUpgradeType upgradeType;
    i32 maxUpgradeLevel = 5;
    i32 price;
};

struct VehicleData
{
    Batcher chassisBatch;
    Batcher chassisOneMaterialBatch;

    SmallArray<VehicleMesh> wheelMeshes[NUM_WHEELS];

    glm::vec3 wheelPositions[NUM_WHEELS] = {};
    glm::mat4 weaponMounts[3] = {
        glm::translate(glm::mat4(1.f), { 3.f, 0.f, 0.6f }),
        glm::translate(glm::mat4(1.f), { 0.f, 0.f, 2.f }),
        glm::translate(glm::mat4(1.f), { -2.f, 0.f, 2.f }),
    };
    SmallArray<glm::vec3> exhaustHoles;
    f32 frontWheelMeshRadius = 0.f;
    f32 frontWheelMeshWidth = 0.f;
    f32 rearWheelMeshRadius = 0.f;
    f32 rearWheelMeshWidth = 0.f;
    f32 collisionWidth = 0.f;
    glm::vec3 sceneCenterOfMass = glm::vec3(0.f);

    Array<VehicleCollisionsMesh> collisionMeshes;
    Array<PerformanceUpgrade> availableUpgrades;

    const char* name ="";
    const char* description ="";
    i32 price = 0;
    u32 frontWeaponCount = 1;
    u32 rearWeaponCount = 1;

    Array<VehicleMesh> debrisChunks;

    virtual ~VehicleData() {}
    virtual void render(class RenderWorld* rw, glm::mat4 const& transform,
            glm::mat4* wheelTransforms, VehicleConfiguration& config,
            class Vehicle* vehicle=nullptr, bool isBraking=false, bool isHidden=false,
            glm::vec4 const& shield={0,0,0,0});
    virtual void renderDebris(class RenderWorld* rw,
            Array<VehicleDebris> const& debris, VehicleConfiguration& config);

    virtual void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) = 0;
    void loadModelData(const char* modelName);
    void copySceneDataToTuning(VehicleTuning& tuning);
    void initStandardUpgrades();
};

Array<OwnedPtr<VehicleData>> g_vehicles;
Array<RegisteredWeapon> g_weapons;
Array<ComputerDriverData> g_ais;

u32 findVehicleIndexByName(const char* name)
{
    for (u32 i=0; i<g_vehicles.size(); ++i)
    {
        if (strcmp(g_vehicles[i]->name, name) == 0)
        {
            return i;
        }
    }
    error("No vehicle exists with name: \"", name, "\"\n");
    return 0;
}

template <typename T>
void registerWeapon()
{
    g_weapons.push_back({
        (T().info),
        [] { return OwnedPtr<Weapon>(new T); }
    });
}

template <typename T>
void registerVehicle()
{
    g_vehicles.push_back(new T);
}

void registerAI(const char* name, f32 drivingSkill, f32 aggression, f32 awareness, f32 fear,
    glm::vec3 const& color, const char* vehicleName, i32 wrapIndex=-1, glm::vec4 const& wrapColor=glm::vec4(1.f))
{
    g_ais.push_back({
        name,
        drivingSkill,
        aggression,
        awareness,
        fear,
        color,
        findVehicleIndexByName(vehicleName),
        wrapIndex,
        wrapColor,
    });
}

void initializeVehicleData();
