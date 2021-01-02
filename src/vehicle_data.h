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

Array<RegisteredWeapon> g_weapons;

template <typename T>
void registerWeapon()
{
    g_weapons.push({
        (T().info),
        [] { return OwnedPtr<Weapon>(new T); }
    });
}

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

struct VehicleCollisionMesh
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

enum struct VehicleDifferentialType
{
    OPEN_FWD,
    OPEN_RWD,
    OPEN_4WD,
    LS_FWD,
    LS_RWD,
    LS_4WD,
    NONE,
};

struct UpgradeStats
{
    f32 chassisMassDiff = 0.f;
    f32 wheelDampingRateDiff = 0.f;
    f32 wheelOffroadDampingRateDiff = 0.f;
    f32 trackTireFrictionDiff = 0.f;
    f32 offroadTireFrictionDiff = 0.f;
    f32 topSpeedDiff = 0.f;
    f32 constantDownforceDiff = 0.f;
    f32 forwardDownforceDiff = 0.f;
    f32 maxEngineOmegaDiff = 0.f;
    f32 peakEngineTorqueDiff = 0.f;
    f32 maxHitPointsDiff = 0.f;
    f32 antiRollbarStiffnessDiff = 0.f;
    f32 suspensionSpringStrengthDiff = 0.f;
    f32 suspensionSpringDamperRateDiff = 0.f;

    VehicleDifferentialType differential = VehicleDifferentialType::NONE;

    void serialize(Serializer& s)
    {
        s.field(chassisMassDiff);
        s.field(wheelDampingRateDiff);
        s.field(wheelOffroadDampingRateDiff);
        s.field(trackTireFrictionDiff);
        s.field(offroadTireFrictionDiff);
        s.field(topSpeedDiff);
        s.field(constantDownforceDiff);
        s.field(forwardDownforceDiff);
        s.field(maxEngineOmegaDiff);
        s.field(peakEngineTorqueDiff);
        s.field(maxHitPointsDiff);
        s.field(antiRollbarStiffnessDiff);
        s.field(suspensionSpringStrengthDiff);
        s.field(suspensionSpringDamperRateDiff);
        s.field(differential);
    }
};

struct PerformanceUpgrade
{
    Str32 name;
    Str64 description;
    i64 iconGuid;
    i32 price = 1000;
    i32 maxUpgradeLevel = 5;
    UpgradeStats stats;

    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(description);
        s.field(iconGuid);
        s.field(price);
        s.field(maxUpgradeLevel);
        s.field(stats);
    }
};

struct VehicleTuning
{
    f32 chassisMass = 1400.f;
    Vec3 centerOfMass = { 0.f, 0.f, -0.2f };

    f32 wheelDampingRate = 0.25f;
    f32 wheelOffroadDampingRate = 15.f;
    f32 trackTireFriction = 3.f;
    f32 offroadTireFriction = 2.f;

    f32 topSpeed = 33.f;
    f32 constantDownforce = 0.005f;
    f32 forwardDownforce = 0.005f;
    f32 driftBoost = 0.f;
    f32 rearTireGripPercent = 1.f;

    f32 maxEngineOmega = 600.f;
    f32 peakEngineTorque = 800.f;
    f32 engineDampingFullThrottle = 0.15f;
    f32 engineDampingZeroThrottleClutchEngaged = 2.f;
    f32 engineDampingZeroThrottleClutchDisengaged = 0.35f;
    f32 maxBrakeTorque = 10000.f;
    f32 maxSteerAngleDegrees = 60.f;

    // reverse, neutral, first, second, third...
    SmallArray<f32, 10> gearRatios;
    f32 finalGearRatio = 4.f;

    f32 suspensionMaxCompression = 0.2f;
    f32 suspensionMaxDroop = 0.3f;
    f32 suspensionSpringStrength = 35000.f;
    f32 suspensionSpringDamperRate = 4500.f;

    // TODO: should these be in degrees instead of radians?
    f32 camberAngleAtRest = 0.f;
    f32 camberAngleAtMaxDroop = 0.01f;
    f32 camberAngleAtMaxCompression = -0.01f;

    f32 frontAntiRollbarStiffness = 10000.f;
    f32 rearAntiRollbarStiffness = 10000.f;

    f32 ackermannAccuracy = 1.f;

    VehicleDifferentialType differential = VehicleDifferentialType::LS_RWD;

    f32 maxHitPoints = 100.f;

    // not very useful values
    f32 wheelMassFront = 30.f;
    f32 wheelMassRear = 30.f;
    f32 frontToeAngle = 0.f;
    f32 rearToeAngle = 0.f;
    f32 clutchStrength = 10.f;
    f32 gearSwitchTime = 0.2f;
    f32 autoBoxSwitchTime = 0.25f;
    f32 maxHandbrakeTorque = 10000.f;

    // these values are computed or derived from model
    f32 collisionWidth = 0.f;
    f32 collisionLength = 0.f;
    Vec3 wheelPositions[NUM_WHEELS] = {};
    Array<VehicleCollisionMesh> collisionMeshes;
    f32 wheelWidthRear = 0.4f;
    f32 wheelRadiusRear = 0.6f;
    f32 wheelWidthFront = 0.4f;
    f32 wheelRadiusFront = 0.6f;
    Mat4 weaponMounts[3] = {
        Mat4::translation({ 3.f, 0.f, 0.6f }),
        Mat4::translation({ 0.f, 0.f, 2.f }),
        Mat4::translation({ -2.f, 0.f, 2.f }),
    };
    SmallArray<VehicleMesh, 10> wheelMeshes[NUM_WHEELS];
    Array<VehicleMesh> debrisChunks;
    Batcher chassisBatch;
    Batcher chassisOneMaterialBatch;

    VehicleTuning()
    {
        gearRatios.push(-4.f);
        gearRatios.push(0.f);
        gearRatios.push(4.f);
        gearRatios.push(2.f);
        gearRatios.push(1.5f);
        gearRatios.push(1.1f);
        gearRatios.push(1.f);
    }

    VehicleTuning(VehicleTuning const&) = delete;
    VehicleTuning(VehicleTuning &&) = default;
    VehicleTuning& operator = (VehicleTuning const&) = delete;
    VehicleTuning& operator = (VehicleTuning &&) = default;

    void copy(VehicleTuning* dest)
    {
        // this is not great
        memcpy(dest, this, sizeof(VehicleTuning));
    }

    f32 getRestOffset() const
    {
        f32 wheelZ = -wheelPositions[0].z;
        return wheelZ + wheelRadiusFront;
    }

    VehicleStats computeVehicleStats();

    void serialize(Serializer& s)
    {
        s.field(chassisMass);
        s.field(centerOfMass);

        s.field(wheelDampingRate);
        s.field(wheelOffroadDampingRate);
        s.field(trackTireFriction);
        s.field(offroadTireFriction);

        s.field(topSpeed);
        s.field(constantDownforce);
        s.field(forwardDownforce);
        s.field(driftBoost);
        s.field(rearTireGripPercent);

        s.field(maxEngineOmega);
        s.field(peakEngineTorque);
        s.field(engineDampingFullThrottle);
        s.field(engineDampingZeroThrottleClutchEngaged);
        s.field(engineDampingZeroThrottleClutchDisengaged);
        s.field(maxBrakeTorque);
        s.field(maxSteerAngleDegrees);

        s.field(gearRatios);
        s.field(finalGearRatio);

        s.field(suspensionMaxCompression);
        s.field(suspensionMaxDroop);
        s.field(suspensionSpringStrength);
        s.field(suspensionSpringDamperRate);

        s.field(camberAngleAtRest);
        s.field(camberAngleAtMaxDroop);
        s.field(camberAngleAtMaxCompression);

        s.field(frontAntiRollbarStiffness);
        s.field(rearAntiRollbarStiffness);

        s.field(ackermannAccuracy);

        s.field(differential);

        s.field(maxHitPoints);

        // less useful values
        f32 wheelMassFront = 30.f;
        f32 wheelMassRear = 30.f;
        f32 frontToeAngle = 0.f;
        f32 rearToeAngle = 0.f;
        f32 clutchStrength = 10.f;
        f32 gearSwitchTime = 0.2f;
        f32 autoBoxSwitchTime = 0.25f;
        f32 maxHandbrakeTorque = 10000.f;
    }

    void applyUpgrade(PerformanceUpgrade const& upgrade, u32 level)
    {
        assert(level > 0);
        assert(level < upgrade.maxUpgradeLevel);

        chassisMass += upgrade.stats.chassisMassDiff * level;
        wheelDampingRate += upgrade.stats.wheelDampingRateDiff * level;
        wheelOffroadDampingRate += upgrade.stats.wheelOffroadDampingRateDiff * level;
        trackTireFriction += upgrade.stats.trackTireFrictionDiff * level;
        offroadTireFriction += upgrade.stats.offroadTireFrictionDiff * level;
        topSpeed += upgrade.stats.topSpeedDiff * level;
        constantDownforce += upgrade.stats.constantDownforceDiff * level;
        forwardDownforce += upgrade.stats.forwardDownforceDiff * level;
        maxEngineOmega += upgrade.stats.maxEngineOmegaDiff * level;
        peakEngineTorque += upgrade.stats.peakEngineTorqueDiff * level;
        maxHitPoints += upgrade.stats.maxHitPointsDiff * level;
        frontAntiRollbarStiffness += upgrade.stats.antiRollbarStiffnessDiff * level;
        rearAntiRollbarStiffness += upgrade.stats.antiRollbarStiffnessDiff * level;
        suspensionSpringStrength += upgrade.stats.suspensionSpringStrengthDiff;
        suspensionSpringDamperRate += upgrade.stats.suspensionSpringDamperRateDiff;

        if (upgrade.stats.differential != VehicleDifferentialType::NONE)
        {
            differential = upgrade.stats.differential;
        }
    }
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

struct VehicleData : public Resource
{
protected:
    virtual void initializeTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) {}

public:
    VehicleTuning defaultTuning;

    bool showInCarLot = true;
    Str64 description ="";
    Vec3 defaultColorHsv = Vec3(0.f, 0.f, 0.95f);
    f32 defaultShininess = 1.f;
    i32 price = 0;
    Array<PerformanceUpgrade> availableUpgrades;
    SmallArray<WeaponSlot, 4> weaponSlots;
    i64 modelGuid;

    // not serialized
    // TODO: this should be moved into VehicleTuning
    SmallArray<Mat4> exhaustHoles;

    void serialize(Serializer& s)
    {
        Resource::serialize(s);

        s.field(defaultTuning);
        s.field(showInCarLot);
        s.field(description);
        s.field(defaultColorHsv);
        s.field(defaultShininess);
        s.field(price);
        s.field(weaponSlots);
        s.field(availableUpgrades);
        s.field(modelGuid);
    }

    virtual ~VehicleData() {}
    virtual void render(class RenderWorld* rw, Mat4 const& transform,
            Mat4* wheelTransforms, VehicleConfiguration& config, VehicleTuning* tuning=nullptr,
            class Vehicle* vehicle=nullptr, bool isBraking=false, bool isHidden=false,
            Vec4 const& shield={0,0,0,0});
    virtual void renderDebris(class RenderWorld* rw,
            Array<VehicleDebris> const& debris, VehicleConfiguration& config);

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning)
    {
        defaultTuning.copy(&tuning);
        //initializeTuning(configuration, tuning);
        loadModelData(tuning);
    }
    void loadModelData(const char* modelName, VehicleTuning& tuning);
    void loadModelData(VehicleTuning& tuning);
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

void initializeVehicleData();
