#pragma once

#include "math.h"
#include "game.h"
#include <vector>

const u32 WHEEL_FRONT_LEFT  = PxVehicleDrive4WWheelOrder::eFRONT_LEFT;
const u32 WHEEL_FRONT_RIGHT = PxVehicleDrive4WWheelOrder::eFRONT_RIGHT;
const u32 WHEEL_REAR_LEFT   = PxVehicleDrive4WWheelOrder::eREAR_LEFT;
const u32 WHEEL_REAR_RIGHT  = PxVehicleDrive4WWheelOrder::eREAR_RIGHT;
const u32 NUM_WHEELS = 4;

struct PhysicsVehicleSettings
{
    f32 chassisMass = 1000.f;
    glm::vec3 chassisDimensions = { 5.0f, 2.5f, 2.0f };
    glm::vec3 chassisCMOffset = { 0.25f, 0.f, -1.f + 0.65f };

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

    f32 maxEngineOmega = 600.f;
    f32 peekEngineTorque = 800.f;
    f32 engineDampingFullThrottle = 0.15f;
    f32 engineDampingZeroThrottleClutchEngaged = 2.f;
    f32 engineDampingZeroThrottleClutchDisengaged = 0.35f;
    f32 maxHandbrakeTorque = 10000.f;
    f32 maxBrakeTorque = 10000.f;
    f32 maxSteerAngle = M_PI * 0.33f;
    f32 clutchStrength = 10.f;
    f32 gearSwitchTime = 0.2f;
    f32 autoBoxSwitchTime = 0.25f;

    // reverse, neutral, first, second, third...
    std::vector<f32> gearRatios = { -4.f, 0.f, 4.f, 2.f, 1.5f, 1.1f, 1.f };
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

    glm::vec3 chassisMOI = glm::vec3(0.f); // will be calculated if left at 0

    PxVehicleDifferential4WData::Enum differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;
        //PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
        //PxVehicleDifferential4WData::eDIFF_TYPE_LS_FRONTWD;
        //PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_FRONTWD;

    glm::vec3 wheelPositions[4];
};

struct VehicleData
{
    PhysicsVehicleSettings physics;
    u32 chassisMesh;
    u32 wheelMeshFront;
    u32 wheelMeshRear;

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

    f32 getRestOffset()
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
    std::vector<DebrisChunk> debrisChunks;
};

VehicleData car;

inline void loadVehicleScene(const char* sceneName, VehicleData* vehicleData)
{
    DataFile::Value::Dict& scene = game.resources.getScene(sceneName);
    for (auto& e : scene["entities"].array())
    {
        std::string name = e["name"].string();
        if (name == "Chassis")
        {
            vehicleData->chassisMesh = game.resources.getMesh(e["data_name"].string().c_str()).renderHandle;
        }
        else if (name == "FL")
        {
            vehicleData->wheelMeshFront = game.resources.getMesh(e["data_name"].string().c_str()).renderHandle;
            vehicleData->physics.wheelPositions[WHEEL_FRONT_RIGHT] =
                e["matrix"].convertBytes<glm::mat4>()[3];
        }
        else if (name == "RL")
        {
            vehicleData->wheelMeshRear = game.resources.getMesh(e["data_name"].string().c_str()).renderHandle;
            vehicleData->physics.wheelPositions[WHEEL_REAR_RIGHT] =
                e["matrix"].convertBytes<glm::mat4>()[3];
        }
        else if (name == "FR")
        {
            vehicleData->physics.wheelPositions[WHEEL_FRONT_LEFT] =
                e["matrix"].convertBytes<glm::mat4>()[3];
        }
        else if (name == "RR")
        {
            vehicleData->physics.wheelPositions[WHEEL_REAR_LEFT] =
                e["matrix"].convertBytes<glm::mat4>()[3];
        }
        else if (name.find("debris") != std::string::npos)
        {
            vehicleData->debrisChunks.push_back({
                game.resources.getMesh(e["data_name"].string().c_str()).renderHandle,
                e["matrix"].convertBytes<glm::mat4>(),
                nullptr
            });
        }
    }
}

inline void initVehicleData()
{
    car.physics.chassisMass = 700.f;
    car.physics.chassisDimensions = { 3.8f, 2.f, 1.6f };
    car.physics.chassisCMOffset = { 0.1f, 0.f, -0.7f };
    car.physics.wheelMassFront = car.physics.wheelMassRear = 20.f;
    car.physics.wheelWidthFront = car.physics.wheelWidthRear = 0.25f;
    car.physics.wheelRadiusFront = car.physics.wheelRadiusRear = 0.4f;
    car.physics.frontToeAngle = 0.f;
    car.physics.wheelDampingRate = 0.22f;
    car.physics.offroadDampingRate = 15.f;
    car.physics.trackTireFriction = 3.9f;
    car.physics.offroadTireFriction = 1.25f;
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
    car.physics.autoBoxSwitchTime = 0.26f;
    car.physics.gearSwitchTime = 0.22f;
    car.physics.suspensionMaxCompression = 0.15f;
    car.physics.suspensionMaxDroop = 0.25f;
    car.physics.frontToeAngle = 0.f;
    car.physics.wheelDampingRate = 0.22f;
    car.physics.offroadDampingRate = 15.f;
    car.physics.trackTireFriction = 3.9f;
    car.physics.offroadTireFriction = 1.25f;
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
    car.physics.autoBoxSwitchTime = 0.26f;
    car.physics.gearSwitchTime = 0.22f;
    car.physics.suspensionMaxCompression = 0.15f;
    car.physics.suspensionMaxDroop = 0.25f;
    car.physics.suspensionSpringStrength = 23000.0f;
    car.physics.suspensionSpringDamperRate = 3000.0f;
    car.physics.camberAngleAtRest = 0.f;
    car.physics.camberAngleAtMaxDroop = 0.02f;
    car.physics.camberAngleAtMaxCompression = -0.02f;
    car.physics.frontAntiRollbarStiffness = 8000.0f;
    car.physics.rearAntiRollbarStiffness = 8000.0f;
    car.physics.ackermannAccuracy = 1.f;
    car.physics.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_FRONTWD;
    loadVehicleScene("car.Vehicle", &car);
}
