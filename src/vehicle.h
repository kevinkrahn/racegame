#pragma once

#include "math.h"
#include "vehicle_data.h"
#include "track_graph.h"
#include "ribbon.h"
#include "driver.h"
#include "scene.h"

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
// TODO: should be private
public:
	Driver* driver;
	Scene* scene;
	u32 vehicleIndex;
	Decal testDecal;
	i32 cameraIndex = -1;

    // physics data
    bool isInAir = true;
    PxVehicleDrive4W* vehicle4W;
    ActorUserData actorUserData;
    VehicleSceneQueryData* sceneQueryData;
    PxBatchQuery* batchQuery;
    PxVehicleDrivableSurfaceToTireFrictionPairs* frictionPairs;
	PxWheelQueryResult wheelQueryResults[PX_MAX_NB_WHEELS];
	VehicleTuning tuning;

    // gameplay data
	bool finishedRace = false;
    glm::vec3 cameraTarget;
    glm::vec3 cameraFrom;
    f32 hitPoints = 0.f;
    u32 currentLap = 0;
	i32 placement = 1;
    TrackGraph::QueryResult graphResult;
    u32 followPathIndex = 0;
    u32 targetPointIndex = 0;
    glm::vec3 targetOffset = glm::vec3(0);
    glm::vec3 startOffset = glm::vec3(0);
    glm::mat4 startTransform;
	f32 backupTimer = 0.f;
	f32 flipTimer = 0.f;
	f32 deadTimer = 0.f;
	f32 controlledBrakingTimer = 0.f;
	u32 lastDamagedBy;
	f32 smokeTimer = 0.f;
	f32 smokeTimerDamage = 0.f;
	f32 offsetChangeTimer = 0.f;
	f32 offsetChangeInterval = 5.f;
    u32 engineSound = 0;
    u32 tireSound = 0;
    glm::vec3 lastValidPosition;
    bool isHidden = false;
    glm::vec3 previousVelocity;
    f32 engineRPM = 0.f;

    // ai timers
    f32 attackTimer = 0.f;
    f32 targetTimer = 0.f;
    Vehicle* target = nullptr;
    f32 fearTimer = 0.f;

    std::unique_ptr<Weapon>
        frontWeapons[ARRAY_SIZE(VehicleConfiguration::frontWeaponIndices)];
    std::unique_ptr<Weapon>
        rearWeapons[ARRAY_SIZE(VehicleConfiguration::frontWeaponIndices)];

    glm::vec3 screenShakeVelocity = glm::vec3(0);
    glm::vec3 screenShakeOffset = glm::vec3(0);
    f32 screenShakeTimer = 0.f;
    f32 screenShakeDirChangeTimer = 0.f;

    bool isWheelSlipping[NUM_WHEELS] = {};
	Ribbon tireMarkRibbons[NUM_WHEELS];

    struct DustSpot
    {
        glm::vec3 p;
        f32 radius;
    };
    SmallVec<DustSpot, 16> dustSpots;
    bool checkRoadDust();

    std::vector<VehicleDebris> vehicleDebris;
    void createVehicleDebris(VehicleDebris const& debris) { vehicleDebris.push_back(debris); }

	struct Notification
	{
        const char* str;
        f32 timeLeft;
	};
	SmallVec<Notification> notifications;

	RaceStatistics raceStatistics;

	void setupPhysics(PxScene* scene, PxMaterial* vehicleMaterial,
	        const PxMaterial** surfaceMaterials, glm::mat4 const& transform);

    void updatePhysics(PxScene* scene, f32 timestep, bool digital,
            f32 accel, f32 brake, f32 steer, bool handbrake, bool canGo, bool onlyBrake);

    bool isBlocking(f32 radius, glm::vec3 const& dir, f32 dist);

public:
	Vehicle(class Scene* scene, glm::mat4 const& transform, glm::vec3 const& startOffset,
	        Driver* driver, VehicleTuning&& tuning, PxMaterial* material, const PxMaterial** surfaceMaterials,
	        u32 vehicleIndex, i32 cameraIndex);
	~Vehicle();

    f32 getEngineRPM() const { return vehicle4W->mDriveDynData.getEngineRotationSpeed() * 9.5493f + 900.f; }
    f32 getForwardSpeed() const { return vehicle4W->computeForwardSpeed(); }
    f32 getSidewaysSpeed() const { return vehicle4W->computeSidewaysSpeed(); }
    PxRigidDynamic* getRigidBody() const { return vehicle4W->getRigidDynamicActor(); }
    glm::mat4 getTransform() const { return convert(PxMat44(getRigidBody()->getGlobalPose())); }
    glm::vec3 getPosition() const { return convert(getRigidBody()->getGlobalPose().p); }
    glm::vec3 getForwardVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector0()); }
    glm::vec3 getRightVector() const { return convert(getRigidBody()->getGlobalPose().q.getBasisVector1()); }
    Driver* getDriver() const { return driver; }

    void blowUp();
    void reset(glm::mat4 const& transform) const;
    void applyDamage(f32 amount, u32 instigator)
    {
        hitPoints -= amount;
        lastDamagedBy = instigator;
        if (smokeTimerDamage <= 0.f)
        {
            smokeTimerDamage = 0.015f;
        }
    }
    void addNotification(const char* str)
    {
        if (notifications.size() < notifications.capacity()) notifications.push_back({ str, 2.f });
    }

    void onUpdate(RenderWorld* rw, f32 deltaTime);
    void onRender(RenderWorld* rw, f32 deltaTime);
    void drawWeaponAmmo(Renderer* renderer, glm::vec2 pos, Weapon* weapon);
    void drawHUD(class Renderer* rw, f32 deltaTime);
    void shakeScreen(f32 intensity);
    void updateCamera(RenderWorld* rw, f32 deltaTime);
    void resetAmmo();
    void onTrigger(ActorUserData* userData);
};
