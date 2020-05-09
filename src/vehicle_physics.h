#pragma once

#include "math.h"
#include "vehicle_data.h"
#include "collision_flags.h"

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

PxFilterFlags vehicleFilterShader(
    PxFilterObjectAttributes attributes0, PxFilterData filterData0,
    PxFilterObjectAttributes attributes1, PxFilterData filterData1,
    PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
    PX_UNUSED(attributes0);
    PX_UNUSED(attributes1);
    PX_UNUSED(constantBlock);
    PX_UNUSED(constantBlockSize);

    if( (0 == (filterData0.word0 & filterData1.word1)) && (0 == (filterData1.word0 & filterData0.word1)) )
    {
        return PxFilterFlag::eSUPPRESS;
    }

    pairFlags = PxPairFlag::eCONTACT_DEFAULT;
    pairFlags |= PxPairFlags(PxU16(filterData0.word2 | filterData1.word2));

    if (((filterData0.word0 & COLLISION_FLAG_CHASSIS) || (filterData1.word0 & COLLISION_FLAG_CHASSIS)) &&
        (!(filterData0.word0 & COLLISION_FLAG_DEBRIS) && !(filterData1.word0 & COLLISION_FLAG_DEBRIS)))
    {
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
        pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
        pairFlags |= PxPairFlag::eNOTIFY_CONTACT_POINTS;
    }

    return PxFilterFlags();
}

struct WheelInfo
{
    glm::mat4 transform;
    glm::vec3 position;
    glm::vec3 contactNormal;
    glm::vec3 contactPosition;
    f32 rotationSpeed = 0.f;
    f32 lateralSlip = 0.f;
    f32 longitudinalSlip = 0.f;
    f32 oilCoverage = 0.f;
    f32 dustAmount = 0.f;
    bool isTouchingTrack = false;
    bool isOffroad = false;
    bool isTouchingGlue = false;
    bool isInAir = false;
};

struct GroundSpot
{
    enum GroundType
    {
        DUST,
        OIL,
        GLUE,
    };
    u32 groundType;
    glm::vec3 p;
    f32 radius;
};

struct IgnoredGroundSpot
{
    class Entity* e;
    f32 t;
};

class VehiclePhysics
{
    PxVehicleDrive4W* vehicle4W;
    VehicleSceneQueryData* sceneQueryData;
    PxBatchQuery* batchQuery;
    PxVehicleDrivableSurfaceToTireFrictionPairs* frictionPairs;
	f32 engineThrottle = 0.f;
	VehicleTuning* tuning;
	PxWheelQueryResult wheelQueryResults[NUM_WHEELS];

	bool isInAir = true;

    SmallVec<GroundSpot, 16> groundSpots;
    SmallVec<IgnoredGroundSpot> ignoredGroundSpots;

    void checkGroundSpots(PxScene* physicsScene, f32 deltaTime);
    void updateWheelInfo(f32 deltaTime);

public:
    WheelInfo wheelInfo[NUM_WHEELS];

    void setup(void* userData, PxScene* scene, glm::mat4 const& transform, VehicleTuning* tuning);
    ~VehiclePhysics();

    void update(PxScene* scene, f32 timestep, bool digital, f32 accel, f32 brake, f32 steer,
            bool handbrake, bool canGo, bool onlyBrake);
    void reset(glm::mat4 const& transform);
    f32 getEngineRPM() const { return vehicle4W->mDriveDynData.getEngineRotationSpeed() * 9.5493f + 900.f; }
    f32 getForwardSpeed() const { return vehicle4W->computeForwardSpeed(); }
    f32 getSidewaysSpeed() const { return vehicle4W->computeSidewaysSpeed(); }
    PxRigidDynamic* getRigidBody() const { return vehicle4W->getRigidDynamicActor(); }
    glm::mat4 getTransform() const { return convert(PxMat44(getRigidBody()->getGlobalPose())); }
    glm::vec3 getPosition() const { return convert(getRigidBody()->getGlobalPose().p); }
    glm::vec3 getForwardVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector0()); }
    glm::vec3 getRightVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector1()); }
    glm::vec3 getUpVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector2()); }
    u32 getCurrentGear()  const{ return vehicle4W->mDriveDynData.mCurrentGear; }
    f32 getCurrentGearRatio()  const{ return tuning->gearRatios[getCurrentGear()]; }
    f32 getAverageWheelRotationSpeed() const;
    f32 getEngineThrottle() const { return engineThrottle; }
    void addIgnoredGroundSpot(class Entity* e) { ignoredGroundSpots.push_back({ e, 1.f }); }
};
