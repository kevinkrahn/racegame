#pragma once

#include "math.h"
#include "smallvec.h"

const u32 WHEEL_FRONT_LEFT  = PxVehicleDrive4WWheelOrder::eFRONT_LEFT;
const u32 WHEEL_FRONT_RIGHT = PxVehicleDrive4WWheelOrder::eFRONT_RIGHT;
const u32 WHEEL_REAR_LEFT   = PxVehicleDrive4WWheelOrder::eREAR_LEFT;
const u32 WHEEL_REAR_RIGHT  = PxVehicleDrive4WWheelOrder::eREAR_RIGHT;
const u32 NUM_WHEELS = 4;

struct PhysicsVehicleSettings
{
    struct CollisionsMesh
    {
        PxConvexMesh* convexMesh;
        glm::mat4 transform;
    };
    SmallVec<CollisionsMesh> collisionMeshes;

    f32 chassisDensity = 120.f;
    glm::vec3 centerOfMass = { 0.f, 0.f, -0.2f };

    f32 wheelMassFront = 30.f;
    f32 wheelWidthFront = 0.4f;
    f32 wheelRadiusFront = 0.6f;
    f32 wheelMassRear = 30.f;
    f32 wheelWidthRear = 0.4f;
    f32 wheelRadiusRear = 0.6f;

    f32 wheelDampingRate = 0.25f;
    f32 offroadDampingRate = 15.f;
    f32 trackTireFriction = 3.f;
    f32 offroadTireFriction = 2.f;
    f32 frontToeAngle = 0.f;
    f32 rearToeAngle = 0.f;

    f32 maxEngineOmega = 600.f;
    f32 peekEngineTorque = 800.f;
    f32 engineDampingFullThrottle = 0.15f;
    f32 engineDampingZeroThrottleClutchEngaged = 2.f;
    f32 engineDampingZeroThrottleClutchDisengaged = 0.35f;
    f32 maxHandbrakeTorque = 10000.f;
    f32 maxBrakeTorque = 10000.f;
    f32 maxSteerAngle = f32(M_PI) * 0.33f;
    f32 clutchStrength = 10.f;
    f32 gearSwitchTime = 0.2f;
    f32 autoBoxSwitchTime = 0.25f;

    // reverse, neutral, first, second, third...
    SmallVec<f32> gearRatios = { -4.f, 0.f, 4.f, 2.f, 1.5f, 1.1f, 1.f };
    f32 finalGearRatio = 4.f;

    f32 suspensionMaxCompression = 0.2f;
    f32 suspensionMaxDroop = 0.3f;
    f32 suspensionSpringStrength = 20000.0f; // 35000.f
    f32 suspensionSpringDamperRate = 2000.0f; // 4500.f

    f32 camberAngleAtRest = 0.f;
    f32 camberAngleAtMaxDroop = 0.01f;
    f32 camberAngleAtMaxCompression = -0.01f;

    f32 frontAntiRollbarStiffness = 10000.0f;
    f32 rearAntiRollbarStiffness = 10000.0f;

    f32 ackermannAccuracy = 1.f;

    PxVehicleDifferential4WData::Enum differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;
        //PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
        //PxVehicleDifferential4WData::eDIFF_TYPE_LS_FRONTWD;
        //PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_FRONTWD;

    glm::vec3 wheelPositions[4];
};

struct VehicleData
{
    PhysicsVehicleSettings physics;
    struct VehicleMesh
    {
        u32 renderHandle;
        glm::mat4 transform;
    };
    SmallVec<VehicleMesh> chassisMeshes;
    VehicleMesh wheelMeshFront;
    VehicleMesh wheelMeshRear;

    // all [0,1]
    struct
    {
        f32 acceleration;
        f32 speed;
        f32 armor;
        f32 weight;
        f32 handling;
    } specs;

    char* name;
    char* description;

    u32 price;
    f32 baseArmor;
    f32 collisionWidth = 0.f;

    f32 getRestOffset() const
    {
        f32 wheelZ = -physics.wheelPositions[0].z;
        return wheelZ + physics.wheelRadiusFront;
    }

    struct DebrisChunk
    {
        u32 mesh;
        glm::mat4 transform;
        PxShape* collisionShape;
    };
    SmallVec<DebrisChunk, 32> debrisChunks;
};

VehicleData car;
VehicleData racecar;
VehicleData cubevan;
VehicleData sportscar;

void loadVehicleScene(const char* sceneName, VehicleData* vehicleData);

inline void initVehicleData()
{
    car.physics.chassisDensity = 100.f;
    car.physics.wheelMassFront = car.physics.wheelMassRear = 20.f;
    car.physics.frontToeAngle = -0.01f;
    car.physics.rearToeAngle = 0.f;
    car.physics.wheelDampingRate = 0.22f;
    car.physics.offroadDampingRate = 50.f;
    car.physics.trackTireFriction = 3.9f;
    car.physics.offroadTireFriction = 1.4f;
    car.physics.maxEngineOmega = 700.f;
    car.physics.peekEngineTorque = 410.f;
    car.physics.engineDampingFullThrottle = 0.14f;
    car.physics.engineDampingZeroThrottleClutchEngaged = 2.6f;
    car.physics.engineDampingZeroThrottleClutchDisengaged = 0.6f;
    car.physics.maxHandbrakeTorque = 6000.f;
    car.physics.maxBrakeTorque = 2000.f;
    car.physics.maxSteerAngle = M_PI * 0.22f;
    car.physics.clutchStrength = 8.f;
    car.physics.gearRatios = { -4.5f, 0.f, 4.5f, 2.0f, 1.6f, 1.15f, 0.92f };
    car.physics.finalGearRatio = 4.f;
    car.physics.suspensionMaxCompression = 0.15f;
    car.physics.suspensionMaxDroop = 0.25f;
    car.physics.autoBoxSwitchTime = 0.26f;
    car.physics.gearSwitchTime = 0.22f;
    car.physics.suspensionSpringStrength = 23000.0f;
    car.physics.suspensionSpringDamperRate = 3000.0f;
    car.physics.camberAngleAtRest = 0.f;
    car.physics.camberAngleAtMaxDroop = 0.02f;
    car.physics.camberAngleAtMaxCompression = -0.02f;
    car.physics.frontAntiRollbarStiffness = 8000.0f;
    car.physics.rearAntiRollbarStiffness = 8000.0f;
    car.physics.ackermannAccuracy = 0.6f;
    car.physics.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_FRONTWD;
    loadVehicleScene("car.Vehicle", &car);

    racecar.physics.chassisDensity = 80.f;
    racecar.physics.wheelMassFront = racecar.physics.wheelMassRear = 22.f;
    racecar.physics.frontToeAngle = 0.f;
    racecar.physics.rearToeAngle = 0.01f;
    racecar.physics.wheelDampingRate = 0.24f;
    racecar.physics.offroadDampingRate = 90.f;
    racecar.physics.trackTireFriction = 4.4f;
    racecar.physics.offroadTireFriction = 1.4f;
    racecar.physics.maxEngineOmega = 900.f;
    racecar.physics.peekEngineTorque = 900.f;
    racecar.physics.engineDampingFullThrottle = 0.14f;
    racecar.physics.engineDampingZeroThrottleClutchEngaged = 3.0f;
    racecar.physics.engineDampingZeroThrottleClutchDisengaged = 0.8f;
    racecar.physics.maxHandbrakeTorque = 15000.f;
    racecar.physics.maxBrakeTorque = 15000.f;
    racecar.physics.maxSteerAngle = M_PI * 0.32f;
    racecar.physics.clutchStrength = 12.f;
    racecar.physics.suspensionMaxCompression = 0.06f;
    racecar.physics.suspensionMaxDroop = 0.13f;
    racecar.physics.autoBoxSwitchTime = 0.16f;
    racecar.physics.gearSwitchTime = 0.12f;
    racecar.physics.suspensionSpringStrength = 30000.0f;
    racecar.physics.suspensionSpringDamperRate = 5000.0f;
    racecar.physics.camberAngleAtRest = 0.f;
    racecar.physics.camberAngleAtMaxDroop = 0.02f;
    racecar.physics.camberAngleAtMaxCompression = -0.02f;
    racecar.physics.frontAntiRollbarStiffness = 12000.0f;
    racecar.physics.rearAntiRollbarStiffness = 12000.0f;
    racecar.physics.ackermannAccuracy = 1.f;
    racecar.physics.differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;
    loadVehicleScene("racecar.Vehicle", &racecar);

    cubevan.physics.chassisDensity = 100.f;
    cubevan.physics.wheelMassFront = cubevan.physics.wheelMassRear = 25.f;
    cubevan.physics.frontToeAngle = 0.f;
    cubevan.physics.rearToeAngle = 0.f;
    cubevan.physics.wheelDampingRate = 0.29f;
    cubevan.physics.offroadDampingRate = 20.f;
    cubevan.physics.trackTireFriction = 3.3f;
    cubevan.physics.offroadTireFriction = 2.5f;
    cubevan.physics.maxEngineOmega = 700.f;
    cubevan.physics.peekEngineTorque = 1000.f;
    cubevan.physics.engineDampingFullThrottle = 0.18f;
    cubevan.physics.engineDampingZeroThrottleClutchEngaged = 3.2f;
    cubevan.physics.engineDampingZeroThrottleClutchDisengaged = 0.8f;
    cubevan.physics.maxHandbrakeTorque = 12000.f;
    cubevan.physics.maxBrakeTorque = 12000.f;
    cubevan.physics.maxSteerAngle = M_PI * 0.26f;
    cubevan.physics.clutchStrength = 15.f;
    cubevan.physics.suspensionMaxCompression = 0.1f;
    cubevan.physics.suspensionMaxDroop = 0.17f;
    cubevan.physics.autoBoxSwitchTime = 0.22f;
    cubevan.physics.gearSwitchTime = 0.2f;
    cubevan.physics.suspensionSpringStrength = 30000.0f;
    cubevan.physics.suspensionSpringDamperRate = 6000.0f;
    cubevan.physics.camberAngleAtRest = 0.f;
    cubevan.physics.camberAngleAtMaxDroop = 0.02f;
    cubevan.physics.camberAngleAtMaxCompression = -0.02f;
    cubevan.physics.frontAntiRollbarStiffness = 5500.0f;
    cubevan.physics.rearAntiRollbarStiffness = 5500.0f;
    cubevan.physics.ackermannAccuracy = 0.5f;
    cubevan.physics.differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;
    loadVehicleScene("cubevan.Vehicle", &cubevan);

    sportscar.physics.chassisDensity = 100.f;
    sportscar.physics.wheelMassFront = sportscar.physics.wheelMassRear = 20.f;
    sportscar.physics.frontToeAngle = 0.f;
    sportscar.physics.rearToeAngle = 0.01f;
    sportscar.physics.wheelDampingRate = 0.24f;
    sportscar.physics.offroadDampingRate = 60.f;
    sportscar.physics.trackTireFriction = 3.0f;
    sportscar.physics.offroadTireFriction = 1.5f;
    sportscar.physics.maxEngineOmega = 600.f;
    sportscar.physics.peekEngineTorque = 450.f;
    sportscar.physics.engineDampingFullThrottle = 0.12f;
    sportscar.physics.engineDampingZeroThrottleClutchEngaged = 2.1f;
    sportscar.physics.engineDampingZeroThrottleClutchDisengaged = 0.6f;
    sportscar.physics.maxHandbrakeTorque = 12000.f;
    sportscar.physics.maxBrakeTorque = 12000.f;
    sportscar.physics.maxSteerAngle = M_PI * 0.285f;
    sportscar.physics.clutchStrength = 11.f;
    sportscar.physics.suspensionMaxCompression = 0.05f;
    sportscar.physics.suspensionMaxDroop = 0.15f;
    sportscar.physics.autoBoxSwitchTime = 0.2f;
    sportscar.physics.gearSwitchTime = 0.16f;
    sportscar.physics.suspensionSpringStrength = 28000.0f;
    sportscar.physics.suspensionSpringDamperRate = 5000.0f;
    sportscar.physics.camberAngleAtRest = 0.f;
    sportscar.physics.camberAngleAtMaxDroop = 0.04f;
    sportscar.physics.camberAngleAtMaxCompression = -0.04f;
    sportscar.physics.frontAntiRollbarStiffness = 10000.0f;
    sportscar.physics.rearAntiRollbarStiffness = 10000.0f;
    sportscar.physics.ackermannAccuracy = 0.8f;
    sportscar.physics.differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;
    loadVehicleScene("sportscar.Vehicle", &sportscar);
}
