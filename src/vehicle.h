#pragma once

#include "math.h"
#include <PxPhysicsAPI.h>
#include <vector>

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

enum
{
    NORMAL_TIRE,
    MAX_NUM_TIRE_TYPES
};

struct ActorUserData
{
    PxVehicleWheels* vehicle = nullptr;
    PxActor* actor = nullptr;
};

struct ShapeUserData
{
    bool isWheel = false;
    PxU32 wheelId = 0xffffffff;
};

class VehicleSceneQueryData
{
public:
    static VehicleSceneQueryData* allocate(
        const PxU32 maxNumVehicles, const PxU32 maxNumWheelsPerVehicle, const PxU32 maxNumHitPointsPerWheel, const PxU32 numVehiclesInBatch,
        PxBatchQueryPreFilterShader preFilterShader, PxBatchQueryPostFilterShader postFilterShader,
        PxAllocatorCallback& allocator);
    void free(PxAllocatorCallback& allocator) { allocator.deallocate(this); }
    static PxBatchQuery* setUpBatchedSceneQuery(const PxU32 batchId, const VehicleSceneQueryData& vehicleSceneQueryData, PxScene* scene);
    PxRaycastQueryResult* getRaycastQueryResultBuffer(const PxU32 batchId) { return (mRaycastResults + batchId * mNumQueriesPerBatch); }
    PxSweepQueryResult* getSweepQueryResultBuffer(const PxU32 batchId) { return (mSweepResults + batchId * mNumQueriesPerBatch); }
    PxU32 getQueryResultBufferSize() const { return mNumQueriesPerBatch; }

private:
    PxU32 mNumQueriesPerBatch;
    PxU32 mNumHitResultsPerQuery;
    PxRaycastQueryResult* mRaycastResults;
    PxSweepQueryResult* mSweepResults;
    PxRaycastHit* mRaycastHitBuffer;
    PxSweepHit* mSweepHitBuffer;
    PxBatchQueryPreFilterShader mPreFilterShader;
    PxBatchQueryPostFilterShader mPostFilterShader;
};

class Vehicle
{
public:
    PxVehicleDrive4W* vehicle4W;
    ActorUserData actorUserData;
    ShapeUserData wheelShapeUserData[PX_MAX_NB_WHEELS];
    VehicleSceneQueryData* sceneQueryData;
    PxBatchQuery* batchQuery;
    PxVehicleDrivableSurfaceToTireFrictionPairs* frictionPairs;
	PxWheelQueryResult wheelQueryResults[PX_MAX_NB_WHEELS];
	bool isInAir = true;

	Vehicle(PxScene* scene, glm::mat4 const& transform,
	        PhysicsVehicleSettings& settings, PxMaterial* material, const PxMaterial** surfaceMaterials);
	~Vehicle();

	u32 getNumWheels() const
	{
        return vehicle4W->mWheelsSimData.getNbWheels();
	}

    const char* getCurrentGear() const
    {
        switch (vehicle4W->mDriveDynData.mCurrentGear)
        {
            case 0: return "REVERSE";
            case 1: return "NEUTRAL";
            case 2: return "1";
            case 3: return "2";
            case 4: return "3";
            case 5: return "4";
            case 6: return "5";
            case 7: return "6";
        }
        return "";
    }

    f32 getEngineRPM() const { return vehicle4W->mDriveDynData.getEngineRotationSpeed() * 9.5493f + 900.f; }
    f32 getForwardSpeed() const { return vehicle4W->computeForwardSpeed(); }
    f32 getSidewaysSpeed() const { return vehicle4W->computeSidewaysSpeed(); }
    PxRigidDynamic* getRigidBody() const { return vehicle4W->getRigidDynamicActor(); }
    glm::mat4 getTransform() const { return convert(PxMat44(getRigidBody()->getGlobalPose())); }
    glm::vec3 getPosition() const { return convert(getRigidBody()->getGlobalPose().p); }
    glm::vec3 getForwardVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector0()); }
    glm::vec3 getRightVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector1()); }

    void reset(glm::mat4 const& transform) const;

    void updatePhysics(PxScene* scene, f32 timestep, bool digital,
            f32 accel, f32 brake, f32 steer, bool handbrake, bool canGo, bool onlyBrake);

    void onUpdate(f32 deltaTime, u32 vehicleIndex);
};

PxFilterFlags VehicleFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize);
