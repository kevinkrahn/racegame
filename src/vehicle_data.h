#pragma once

#include "math.h"
#include "datafile.h"
#include "mesh.h"
#include "weapon.h"
#include "material.h"
#include "batcher.h"
#include "vinyl_pattern.h"

#define WHEEL_FRONT_LEFT  PxVehicleDrive4WWheelOrder::eFRONT_LEFT
#define WHEEL_FRONT_RIGHT PxVehicleDrive4WWheelOrder::eFRONT_RIGHT
#define WHEEL_REAR_LEFT   PxVehicleDrive4WWheelOrder::eREAR_LEFT
#define WHEEL_REAR_RIGHT  PxVehicleDrive4WWheelOrder::eREAR_RIGHT
#define NUM_WHEELS 4

struct RegisteredWeapon
{
    WeaponInfo info;
    // TODO: remove std::function
    std::function<OwnedPtr<Weapon>()> create;
};

struct VehicleMesh
{
    Mesh* mesh;
    Mat4 transform;
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
    Mat4 transform;
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
    Vec3 centerOfMass = { 0.f, 0.f, -0.2f };

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

    Vec3 wheelPositions[NUM_WHEELS];

    f32 collisionWidth = 0.f;
    f32 collisionLength = 0.f;
    f32 maxHitPoints;

    f32 getRestOffset() const
    {
        f32 wheelZ = -wheelPositions[0].z;
        return wheelZ + wheelRadiusFront;
    }

    VehicleStats computeVehicleStats();
};

struct VehicleCosmeticConfiguration
{
    Vec3 color = Vec3(0.95f);
    Vec3 hsv = Vec3(0.f, 0.f, 0.95f);
    f32 paintShininess = 1.f;

    i64 vinylGuids[3] = { 0, 0, 0 };
    Vec4 vinylColors[3] = { Vec4(1.f), Vec4(1.f), Vec4(1.f) };
    Vec3 vinylColorsHSV[3] = { {0,0,1}, {0,0,1}, {0,0,1} };

    void serialize(Serializer& s)
    {
        s.field(color);
        s.field(hsv);
        s.field(paintShininess);
        s.field(vinylGuids);
        s.field(vinylColors);
        s.field(vinylColorsHSV);
    }
};

struct VehicleConfiguration
{
    VehicleCosmeticConfiguration cosmetics;

    i32 weaponIndices[4] = { -1, -1, -1, -1 };
    u32 weaponUpgradeLevel[4] = { 0, 0, 0, 0 };

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
        s.field(cosmetics);
        s.field(weaponIndices);
        s.field(weaponUpgradeLevel);
        s.field(performanceUpgrades);
    }

    bool dirty = true;
    Material paintMaterial;

    void reloadMaterials()
    {
        Material* originalPaintMaterial = g_res.getMaterial("paint_material");
        Material* mattePaintMaterial = g_res.getMaterial("paint_material_matte");
        paintMaterial = *originalPaintMaterial;
        paintMaterial.color = cosmetics.color;
        paintMaterial.loadShaderHandles({ {"VEHICLE"} });
        paintMaterial.reflectionLod = lerp(mattePaintMaterial->reflectionLod, originalPaintMaterial->reflectionLod, cosmetics.paintShininess);
        paintMaterial.reflectionBias = lerp(mattePaintMaterial->reflectionBias, originalPaintMaterial->reflectionBias, cosmetics.paintShininess);
        paintMaterial.reflectionStrength = lerp(mattePaintMaterial->reflectionStrength, originalPaintMaterial->reflectionStrength, cosmetics.paintShininess);
        paintMaterial.specularPower = lerp(mattePaintMaterial->specularPower, originalPaintMaterial->specularPower, cosmetics.paintShininess);
        paintMaterial.specularStrength = lerp(mattePaintMaterial->specularStrength, originalPaintMaterial->specularStrength, cosmetics.paintShininess);
        paintMaterial.fresnelBias = lerp(mattePaintMaterial->fresnelBias, originalPaintMaterial->fresnelBias, cosmetics.paintShininess);
        paintMaterial.fresnelPower = lerp(mattePaintMaterial->fresnelPower, originalPaintMaterial->fresnelPower, cosmetics.paintShininess);
        paintMaterial.fresnelScale = lerp(mattePaintMaterial->fresnelScale, originalPaintMaterial->fresnelScale, cosmetics.paintShininess);
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

    SmallArray<VehicleMesh, 10> wheelMeshes[NUM_WHEELS];

    Vec3 wheelPositions[NUM_WHEELS] = {};
    Mat4 weaponMounts[3] = {
        Mat4::translation({ 3.f, 0.f, 0.6f }),
        Mat4::translation({ 0.f, 0.f, 2.f }),
        Mat4::translation({ -2.f, 0.f, 2.f }),
    };
    SmallArray<Mat4> exhaustHoles;
    f32 frontWheelMeshRadius = 0.f;
    f32 frontWheelMeshWidth = 0.f;
    f32 rearWheelMeshRadius = 0.f;
    f32 rearWheelMeshWidth = 0.f;
    f32 collisionWidth = 0.f;
    f32 collisionLength = 0.f;
    Vec3 sceneCenterOfMass = Vec3(0.f);

    Array<VehicleCollisionsMesh> collisionMeshes;
    Array<PerformanceUpgrade> availableUpgrades;

    const char* name ="";
    const char* description ="";
    Vec3 defaultColorHsv = Vec3(0.f, 0.f, 0.95f);
    i32 price = 0;
    SmallArray<WeaponSlot, 4> weaponSlots;

    Array<VehicleMesh> debrisChunks;

    virtual ~VehicleData() {}
    virtual void render(class RenderWorld* rw, Mat4 const& transform,
            Mat4* wheelTransforms, VehicleConfiguration& config,
            class Vehicle* vehicle=nullptr, bool isBraking=false, bool isHidden=false,
            Vec4 const& shield={0,0,0,0});
    virtual void renderDebris(class RenderWorld* rw,
            Array<VehicleDebris> const& debris, VehicleConfiguration& config);

    virtual void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) = 0;
    void loadModelData(const char* modelName);
    void copySceneDataToTuning(VehicleTuning& tuning);
    void initStandardUpgrades();
    u32 getWeaponSlotCount(WeaponType weaponType)
    {
        u32 count = 0;
        for (auto& w : weaponSlots)
        {
            if (w.weaponType == weaponType)
            {
                ++count;
            }
        }
        return count;
    }
    i32 getWeaponSlotIndex(WeaponType weaponType, u32 n)
    {
        u32 count = 0, index = 0;
        for (auto& w : weaponSlots)
        {
            if (w.weaponType == weaponType)
            {
                if (count == n)
                {
                    return index;
                }
                ++count;
            }
            ++index;
        }
        return -1;
    }
};

Array<OwnedPtr<VehicleData>> g_vehicles;
Array<RegisteredWeapon> g_weapons;

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

void initializeVehicleData();
