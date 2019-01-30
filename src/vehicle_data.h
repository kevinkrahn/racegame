#pragma once

#include "math.h"
#include "smallvec.h"
#include "datafile.h"
#include "material.h"
#include "mesh.h"

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

    glm::vec3 wheelPositions[4];
};

struct VehicleData
{
    PhysicsVehicleSettings physics;
    struct VehicleMesh
    {
        Mesh* mesh;
        glm::mat4 transform;
        Material* material = nullptr;
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

    std::string name;
    std::string description;

    u32 price;
    f32 collisionWidth = 0.f;

    f32 getRestOffset() const
    {
        f32 wheelZ = -physics.wheelPositions[0].z;
        return wheelZ + physics.wheelRadiusFront;
    }

    struct DebrisChunk
    {
        Mesh* mesh;
        glm::mat4 transform;
        PxShape* collisionShape;
        Material* material = nullptr;
    };
    SmallVec<DebrisChunk, 32> debrisChunks;
};

void loadVehicleData(DataFile::Value& data, VehicleData& vehicleData);
