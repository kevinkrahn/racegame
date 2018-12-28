#pragma once

#include "game.h"

const u32 WHEEL_FRONT_LEFT  = PxVehicleDrive4WWheelOrder::eFRONT_LEFT;
const u32 WHEEL_FRONT_RIGHT = PxVehicleDrive4WWheelOrder::eFRONT_RIGHT;
const u32 WHEEL_REAR_LEFT   = PxVehicleDrive4WWheelOrder::eREAR_LEFT;
const u32 WHEEL_REAR_RIGHT  = PxVehicleDrive4WWheelOrder::eREAR_RIGHT;

#define BLOCKING_SWEEPS 0

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
    f32 maxSteerAngle = PI * 0.33f;
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
    COLLISION_FLAG_GROUND  = 1 << 0,
    COLLISION_FLAG_WHEEL   = 1 << 1,
    COLLISION_FLAG_CHASSIS = 1 << 2,
};

enum
{
    DRIVABLE_SURFACE = 0xffff0000,
    UNDRIVABLE_SURFACE = 0x0000ffff
};

enum
{
    SURFACE_TYPE_TARMAC,
    SURFACE_TYPE_SAND,
    MAX_NUM_SURFACE_TYPES
};

enum
{
    NORMAL_TIRE,
    MAX_NUM_TIRE_TYPES
};

PxQueryHitType::Enum WheelSceneQueryPreFilterBlocking(
    PxFilterData filterData0, PxFilterData filterData1,
    const void* constantBlock, PxU32 constantBlockSize,
    PxHitFlags& queryFlags)
{
    return ((0 == (filterData1.word3 & DRIVABLE_SURFACE)) ? PxQueryHitType::eNONE : PxQueryHitType::eBLOCK);
}

PxQueryHitType::Enum WheelSceneQueryPostFilterBlocking(
    PxFilterData queryFilterData, PxFilterData objectFilterData,
    const void* constantBlock, PxU32 constantBlockSize,
    const PxQueryHit& hit)
{
    return (static_cast<const PxSweepHit&>(hit)).hadInitialOverlap() ?  PxQueryHitType::eNONE : PxQueryHitType::eBLOCK;
}

PxQueryHitType::Enum WheelSceneQueryPreFilterNonBlocking(
    PxFilterData filterData0, PxFilterData filterData1,
    const void* constantBlock, PxU32 constantBlockSize,
    PxHitFlags& queryFlags)
{
    return ((0 == (filterData1.word3 & DRIVABLE_SURFACE)) ? PxQueryHitType::eNONE : PxQueryHitType::eTOUCH);
}

PxQueryHitType::Enum WheelSceneQueryPostFilterNonBlocking(
    PxFilterData queryFilterData, PxFilterData objectFilterData,
    const void* constantBlock, PxU32 constantBlockSize,
    const PxQueryHit& hit)
{
    return (static_cast<const PxSweepHit&>(hit)).hadInitialOverlap() ? PxQueryHitType::eNONE : PxQueryHitType::eTOUCH;
}

PxF32 steerVsForwardSpeedData[2*8] =
{
	0.0f,		0.75f,
	5.0f,		0.75f,
	30.0f,		0.5f,
	120.0f,		0.25f,
	PX_MAX_F32, PX_MAX_F32,
	PX_MAX_F32, PX_MAX_F32,
	PX_MAX_F32, PX_MAX_F32,
	PX_MAX_F32, PX_MAX_F32
};
PxFixedSizeLookupTable<8> steerVsForwardSpeedTable(steerVsForwardSpeedData, 4);

PxVehicleKeySmoothingData keySmoothingData =
{
	{
		6.0f,	//rise rate eANALOG_INPUT_ACCEL
		6.0f,	//rise rate eANALOG_INPUT_BRAKE
		6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE
		1.6f,	//rise rate eANALOG_INPUT_STEER_LEFT
		1.6f,	//rise rate eANALOG_INPUT_STEER_RIGHT
	},
	{
		10.0f,	//fall rate eANALOG_INPUT_ACCEL
		10.0f,	//fall rate eANALOG_INPUT_BRAKE
		10.0f,	//fall rate eANALOG_INPUT_HANDBRAKE
		4.5f,	//fall rate eANALOG_INPUT_STEER_LEFT
		4.5f	//fall rate eANALOG_INPUT_STEER_RIGHT
	}
};

PxVehiclePadSmoothingData padSmoothingData =
{
	{
		6.0f,	//rise rate eANALOG_INPUT_ACCEL
		6.0f,	//rise rate eANALOG_INPUT_BRAKE
		6.0f,	//rise rate eANALOG_INPUT_HANDBRAKE
		1.8f,	//rise rate eANALOG_INPUT_STEER_LEFT
		1.8f,	//rise rate eANALOG_INPUT_STEER_RIGHT
	},
	{
		10.0f,	//fall rate eANALOG_INPUT_ACCEL
		10.0f,	//fall rate eANALOG_INPUT_BRAKE
		10.0f,	//fall rate eANALOG_INPUT_HANDBRAKE
		5.0f,	//fall rate eANALOG_INPUT_STEER_LEFT
		5.0f	//fall rate eANALOG_INPUT_STEER_RIGHT
	}
};

class VehicleSceneQueryData
{
public:
    static VehicleSceneQueryData* allocate(
        const PxU32 maxNumVehicles, const PxU32 maxNumWheelsPerVehicle, const PxU32 maxNumHitPointsPerWheel, const PxU32 numVehiclesInBatch,
        PxBatchQueryPreFilterShader preFilterShader, PxBatchQueryPostFilterShader postFilterShader,
        PxAllocatorCallback& allocator)
    {
        const PxU32 sqDataSize = ((sizeof(VehicleSceneQueryData) + 15) & ~15);

        const PxU32 maxNumWheels = maxNumVehicles * maxNumWheelsPerVehicle;
        const PxU32 raycastResultSize = ((sizeof(PxRaycastQueryResult) * maxNumWheels + 15) & ~15);
        const PxU32 sweepResultSize = ((sizeof(PxSweepQueryResult) * maxNumWheels + 15) & ~15);

        const PxU32 maxNumHitPoints = maxNumWheels * maxNumHitPointsPerWheel;
        const PxU32 raycastHitSize = ((sizeof(PxRaycastHit) * maxNumHitPoints + 15) & ~15);
        const PxU32 sweepHitSize = ((sizeof(PxSweepHit) * maxNumHitPoints + 15) & ~15);

        const PxU32 size = sqDataSize + raycastResultSize + raycastHitSize + sweepResultSize + sweepHitSize;
        PxU8* buffer = static_cast<PxU8*>(allocator.allocate(size, NULL, NULL, 0));

        VehicleSceneQueryData* sqData = new(buffer) VehicleSceneQueryData();
        sqData->mNumQueriesPerBatch = numVehiclesInBatch * maxNumWheelsPerVehicle;
        sqData->mNumHitResultsPerQuery = maxNumHitPointsPerWheel;
        buffer += sqDataSize;

        sqData->mRaycastResults = reinterpret_cast<PxRaycastQueryResult*>(buffer);
        buffer += raycastResultSize;

        sqData->mRaycastHitBuffer = reinterpret_cast<PxRaycastHit*>(buffer);
        buffer += raycastHitSize;

        sqData->mSweepResults = reinterpret_cast<PxSweepQueryResult*>(buffer);
        buffer += sweepResultSize;

        sqData->mSweepHitBuffer = reinterpret_cast<PxSweepHit*>(buffer);
        buffer += sweepHitSize;

        for (PxU32 i = 0; i < maxNumWheels; i++)
        {
            new(sqData->mRaycastResults + i) PxRaycastQueryResult();
            new(sqData->mSweepResults + i) PxSweepQueryResult();
        }

        for (PxU32 i = 0; i < maxNumHitPoints; i++)
        {
            new(sqData->mRaycastHitBuffer + i) PxRaycastHit();
            new(sqData->mSweepHitBuffer + i) PxSweepHit();
        }

        sqData->mPreFilterShader = preFilterShader;
        sqData->mPostFilterShader = postFilterShader;

        return sqData;
    }

    void free(PxAllocatorCallback& allocator)
    {
        allocator.deallocate(this);
    }

    static PxBatchQuery* setUpBatchedSceneQuery(const PxU32 batchId, const VehicleSceneQueryData& vehicleSceneQueryData, PxScene* scene)
    {
	    const PxU32 maxNumQueriesInBatch =  vehicleSceneQueryData.mNumQueriesPerBatch;
	    const PxU32 maxNumHitResultsInBatch = vehicleSceneQueryData.mNumQueriesPerBatch*vehicleSceneQueryData.mNumHitResultsPerQuery;

	    PxBatchQueryDesc sqDesc(maxNumQueriesInBatch, maxNumQueriesInBatch, 0);

	    sqDesc.queryMemory.userRaycastResultBuffer = vehicleSceneQueryData.mRaycastResults + batchId*maxNumQueriesInBatch;
	    sqDesc.queryMemory.userRaycastTouchBuffer = vehicleSceneQueryData.mRaycastHitBuffer + batchId*maxNumHitResultsInBatch;
	    sqDesc.queryMemory.raycastTouchBufferSize = maxNumHitResultsInBatch;

	    sqDesc.queryMemory.userSweepResultBuffer = vehicleSceneQueryData.mSweepResults + batchId*maxNumQueriesInBatch;
	    sqDesc.queryMemory.userSweepTouchBuffer = vehicleSceneQueryData.mSweepHitBuffer + batchId*maxNumHitResultsInBatch;
	    sqDesc.queryMemory.sweepTouchBufferSize = maxNumHitResultsInBatch;

	    sqDesc.preFilterShader = vehicleSceneQueryData.mPreFilterShader;

	    sqDesc.postFilterShader = vehicleSceneQueryData.mPostFilterShader;

	    return scene->createBatchQuery(sqDesc);
    }

    PxRaycastQueryResult* getRaycastQueryResultBuffer(const PxU32 batchId)
    {
	    return (mRaycastResults + batchId * mNumQueriesPerBatch);
    }

    PxSweepQueryResult* getSweepQueryResultBuffer(const PxU32 batchId)
    {
        return (mSweepResults + batchId * mNumQueriesPerBatch);
    }

    PxU32 getQueryResultBufferSize() const
    {
        return mNumQueriesPerBatch;
    }

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

PxVehicleDrivableSurfaceToTireFrictionPairs* createFrictionPairs(PhysicsVehicleSettings const& settings,
        PxMaterial** materials)
{
    f32 frictionTable[] = {
        settings.trackTireFriction, // TARMAC
        settings.offroadTireFriction, // SAND
    };

    PxVehicleDrivableSurfaceType surfaceTypes[MAX_NUM_SURFACE_TYPES];
    surfaceTypes[0].mType = SURFACE_TYPE_TARMAC;
    surfaceTypes[1].mType = SURFACE_TYPE_SAND;

    const PxMaterial* surfaceMaterials[MAX_NUM_SURFACE_TYPES];
    surfaceMaterials[0] = materials[0];
    surfaceMaterials[1] = materials[1];

    PxVehicleDrivableSurfaceToTireFrictionPairs* surfaceTirePairs =
        PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(MAX_NUM_TIRE_TYPES, MAX_NUM_SURFACE_TYPES);

    surfaceTirePairs->setup(MAX_NUM_TIRE_TYPES, MAX_NUM_SURFACE_TYPES, surfaceMaterials, surfaceTypes);

    for(u32 i = 0; i < MAX_NUM_SURFACE_TYPES; i++)
    {
        for(u32 j = 0; j < MAX_NUM_TIRE_TYPES; j++)
        {
            surfaceTirePairs->setTypePairFriction(i, j, frictionTable[i]);
        }
    }

    return surfaceTirePairs;
}

struct ActorUserData
{
    PxVehicleWheels* vehicle = NULL;
    PxActor* actor = NULL;
};

struct ShapeUserData
{
    bool isWheel = false;
    PxU32 wheelId = 0xffffffff;
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

	~PhysicsVehicle()
	{
	    vehicle4W->getRigidDynamicActor()->release();
	    vehicle4W->free();
	    sceneQueryData->free(game.physx.allocator);
	    batchQuery->release();
	    frictionPairs->release();
	}

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

    f32 getEngineRPM() const
    {
        return vehicle4W->mDriveDynData.getEngineRotationSpeed() * 9.5493f + 900.f;
    }

    f32 getForwardSpeed() const
    {
        return vehicle4W->computeForwardSpeed();
    }

    f32 getSidewaysSpeed() const
    {
        return vehicle4W->computeSidewaysSpeed();
    }

    PxRigidDynamic* getRigidBody() const
    {
        return vehicle4W->getRigidDynamicActor();
    }

    /*
    Mat4 getTransform() const
    {
        return Mat4(PxMat44(getRigidBody()->getGlobalPose()).front());
    }

    Vec3 getPosition() const
    {
        PxVec3 p = getRigidBody()->getGlobalPose().p;
        return Vec3(p.x, p.y, p.z);
    }

    Vec3 getForwardVector() const
    {
        PxVec3 v = getRigidBody()->getGlobalPose().q.getBasisVector0();
        return Vec3(v.x, v.y, v.z);
    }

    Vec3 getRightVector() const
    {
        PxVec3 v = getRigidBody()->getGlobalPose().q.getBasisVector1();
        return Vec3(v.x, v.y, v.z);
    }

    void reset(glm::mat4 const& transform) const
    {
        vehicle4W->setToRestState();
        vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
        Vec3 p = transform.position();
        Mat4 r = transform.rotation();
        PxMat33 rot;
        rot[0] = PxVec3(r[0].x, r[0].y, r[0].z);
        rot[1] = PxVec3(r[1].x, r[1].y, r[1].z);
        rot[2] = PxVec3(r[2].x, r[2].y, r[2].z);
        getRigidBody()->setGlobalPose(PxTransform(PxVec3(p.x, p.y, p.z), PxQuat(rot)));
    }
    */

    void updatePhysics(PxScene* scene, f32 timestep, bool digital,
            f32 accel, f32 brake, f32 steer, bool handbrake, bool canGo, bool onlyBrake)

    void onUpdate(f32 deltaTime, Scene* scene, u32 vehicleIndex);
};

PxRigidDynamic* createVehicleActor(
    const PxVehicleChassisData& chassisData,
    PxMaterial** wheelMaterials, PxConvexMesh** wheelConvexMeshes, const PxU32 numWheels, const PxFilterData& wheelSimFilterData,
    PxMaterial** chassisMaterials, PxConvexMesh** chassisConvexMeshes, const PxU32 numChassisMeshes, const PxFilterData& chassisSimFilterData)
{
    PxRigidDynamic* actor = game.physx.physics->createRigidDynamic(PxTransform(PxIdentity));

    PxFilterData wheelQryFilterData(0, 0, 0, UNDRIVABLE_SURFACE);
    PxFilterData chassisQryFilterData(0, 0, 0, UNDRIVABLE_SURFACE);

    for(PxU32 i = 0; i < numWheels; i++)
    {
        PxConvexMeshGeometry geom(wheelConvexMeshes[i]);
        PxShape* wheelShape = PxRigidActorExt::createExclusiveShape(*vehActor, geom, *wheelMaterials[i]);
        wheelShape->setQueryFilterData(wheelQryFilterData);
        wheelShape->setSimulationFilterData(wheelSimFilterData);
        wheelShape->setLocalPose(PxTransform(PxIdentity));
    }

    for(PxU32 i = 0; i < numChassisMeshes; i++)
    {
        PxShape* chassisShape = PxRigidActorExt::createExclusiveShape(*vehActor, PxConvexMeshGeometry(chassisConvexMeshes[i]), *chassisMaterials[i]);
        chassisShape->setQueryFilterData(chassisQryFilterData);
        chassisShape->setSimulationFilterData(chassisSimFilterData);
        chassisShape->setLocalPose(PxTransform(PxIdentity));
        chassisShape->setContactOffset(0.1f);
        chassisShape->setRestOffset(0.08f);
    }

    actor->setMass(chassisData.mMass);
    actor->setMassSpaceInertiaTensor(chassisData.mMOI);
    actor->setCMassLocalPose(PxTransform(chassisData.mCMOffset,PxQuat(PxIdentity)));

    return vehActor;
}

void configureUserData(PxVehicleWheels* vehicle, ActorUserData* actorUserData, ShapeUserData* shapeUserDatas)
{
    vehicle->getRigidDynamicActor()->userData = actorUserData;
    actorUserData->vehicle = vehicle;

    PxShape* shapes[PX_MAX_NB_WHEELS + 1];
    vehicle->getRigidDynamicActor()->getShapes(shapes, PX_MAX_NB_WHEELS + 1);
    for(PxU32 i = 0; i < vehicle->mWheelsSimData.getNbWheels(); i++)
    {
        const PxI32 shapeId = vehicle->mWheelsSimData.getWheelShapeMapping(i);
        shapes[shapeId]->userData = &shapeUserDatas[i];
        shapeUserDatas[i].isWheel = true;
        shapeUserDatas[i].wheelId = i;
    }
}

PxConvexMesh* createConvexMesh(const PxVec3* verts, const PxU32 numVerts)
{
    PxConvexMeshDesc convexDesc;
    convexDesc.points.count  = numVerts;
    convexDesc.points.stride = sizeof(PxVec3);
    convexDesc.points.data   = verts;
    convexDesc.flags         = PxConvexFlag::eCOMPUTE_CONVEX;

    PxConvexMesh* convexMesh = nullptr;
    PxDefaultMemoryOutputStream buf;
    if(gCooking->cookConvexMesh(convexDesc, buf))
    {
        PxDefaultMemoryInputData id(buf.getData(), buf.getSize());
        convexMesh = gPhysics->createConvexMesh(id);
    }

    return convexMesh;
}

PxConvexMesh* createChassisMesh(Vec3 dims)
{
    const PxF32 x = dims.x*0.5f;
    const PxF32 y = dims.y*0.5f;
    const PxF32 z = dims.z*0.5f;
    PxVec3 verts[8] =
    {
        PxVec3(x,y,-z),
        PxVec3(x,y,z),
        PxVec3(x,-y,z),
        PxVec3(x,-y,-z),
        PxVec3(-x,y,-z),
        PxVec3(-x,y,z),
        PxVec3(-x,-y,z),
        PxVec3(-x,-y,-z)
    };

    return createConvexMesh(verts, 8);
}

PxConvexMesh* createWheelMesh(const PxF32 width, const PxF32 radius)
{
    PxVec3 points[2*16];
    for(PxU32 i = 0; i < 16; i++)
    {
        const PxF32 cosTheta = PxCos(i*PxPi*2.0f/16.0f);
        const PxF32 sinTheta = PxSin(i*PxPi*2.0f/16.0f);
        const PxF32 z = radius*cosTheta;
        const PxF32 x = radius*sinTheta;
        points[2*i+0] = PxVec3(x, -width/2.0f, z);
        points[2*i+1] = PxVec3(x, +width/2.0f, z);
    }

    return createConvexMesh(points, 32);
}

#if BLOCKING_SWEEPS
    const u32 queryHitsPerWheel = 1;
#else
    const u32 queryHitsPerWheel = 8;
#endif

PhysicsVehicle* createPhysicsVehicle(PxScene* scene, Mat4 const& transform,
        PxMaterial* bodyMaterial, PxMaterial** surfaceMaterials, PhysicsVehicleSettings settings)
{
    Vehicle* vehicle = new Vehicle();

    PxQueryHitType::Enum(*sceneQueryPreFilter)(PxFilterData, PxFilterData, const void*, PxU32, PxHitFlags&);
    PxQueryHitType::Enum(*sceneQueryPostFilter)(PxFilterData, PxFilterData, const void*, PxU32, const PxQueryHit&);
#if BLOCKING_SWEEPS
    sceneQueryPreFilter = &WheelSceneQueryPreFilterBlocking;
    sceneQueryPostFilter = &WheelSceneQueryPostFilterBlocking;
#else
    sceneQueryPreFilter = &WheelSceneQueryPreFilterNonBlocking;
    sceneQueryPostFilter = &WheelSceneQueryPostFilterNonBlocking;
#endif
    vehicle->sceneQueryData = VehicleSceneQueryData::allocate(1, PX_MAX_NB_WHEELS, queryHitsPerWheel, 1, sceneQueryPreFilter, sceneQueryPostFilter, gAllocator);
    vehicle->batchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *vehicle->sceneQueryData, scene);

    vehicle->frictionPairs = createFrictionPairs(settings, surfaceMaterials);

    if (lengthSquared(settings.chassisMOI) == 0.f)
    {
        Vec3 dims = settings.chassisDimensions;
        settings.chassisMOI = Vec3(
            (dims.y*dims.y + dims.z*dims.z) * settings.chassisMass / 12.0f,
            (dims.x*dims.x + dims.z*dims.z) * settings.chassisMass / 12.0f,
            (dims.x*dims.x + dims.y*dims.y) * settings.chassisMass / 12.0f);
    }

    PxFilterData chassisSimFilterData(COLLISION_FLAG_CHASSIS, COLLISION_FLAG_CHASSIS, 0, 0);
    PxFilterData wheelSimFilterData(0, 0, 0, 0);

    PxRigidDynamic* veh4WActor = nullptr;
    PxConvexMesh* wheelMeshFront = createWheelMesh(settings.wheelWidthFront, settings.wheelRadiusFront);
    PxConvexMesh* wheelMeshRear = createWheelMesh(settings.wheelWidthRear, settings.wheelRadiusRear);
    PxConvexMesh* wheelConvexMeshes[PX_MAX_NB_WHEELS];
    PxMaterial* wheelMaterials[PX_MAX_NB_WHEELS];
    const u32 numWheels = 4;

    for(u32 i = PxVehicleDrive4WWheelOrder::eFRONT_LEFT; i <= PxVehicleDrive4WWheelOrder::eFRONT_RIGHT; ++i)
    {
        wheelConvexMeshes[i] = wheelMeshFront;
        wheelMaterials[i] = bodyMaterial;
    }

    for(u32 i = PxVehicleDrive4WWheelOrder::eREAR_LEFT; i < numWheels; ++i)
    {
        wheelConvexMeshes[i] = wheelMeshRear;
        wheelMaterials[i] = bodyMaterial;
    }

    PxConvexMesh* chassisConvexMesh = createChassisMesh(settings.chassisDimensions);
    PxConvexMesh* chassisConvexMeshes[1] = { chassisConvexMesh };
    PxMaterial* chassisMaterials[1] = { bodyMaterial };

    PxVehicleChassisData rigidBodyData;
    rigidBodyData.mMOI = toPxVec3(settings.chassisMOI);
    rigidBodyData.mMass = settings.chassisMass;
    rigidBodyData.mCMOffset = toPxVec3(settings.chassisCMOffset);

    veh4WActor = createVehicleActor(rigidBodyData,
        wheelMaterials, wheelConvexMeshes, numWheels, wheelSimFilterData,
        chassisMaterials, chassisConvexMeshes, 1, chassisSimFilterData);

    PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(numWheels);

    PxVec3 wheelCenterOffsets[PX_MAX_NB_WHEELS];
    wheelCenterOffsets[WHEEL_FRONT_LEFT]  = toPxVec3(settings.wheelPositions[WHEEL_FRONT_LEFT]);
    wheelCenterOffsets[WHEEL_FRONT_RIGHT] = toPxVec3(settings.wheelPositions[WHEEL_FRONT_RIGHT]);
    wheelCenterOffsets[WHEEL_REAR_LEFT]   = toPxVec3(settings.wheelPositions[WHEEL_REAR_LEFT]);
    wheelCenterOffsets[WHEEL_REAR_RIGHT]  = toPxVec3(settings.wheelPositions[WHEEL_REAR_RIGHT]);

    f32 wheelMOIFront = 0.5f * settings.wheelMassFront * square(settings.wheelRadiusFront);
    f32 wheelMOIRear = 0.5f * settings.wheelMassRear * square(settings.wheelRadiusRear);

    PxVehicleWheelData wheels[PX_MAX_NB_WHEELS];
    for(u32 i = PxVehicleDrive4WWheelOrder::eFRONT_LEFT; i <= PxVehicleDrive4WWheelOrder::eFRONT_RIGHT; ++i)
    {
        wheels[i].mMass   = settings.wheelMassFront;
        wheels[i].mMOI    = wheelMOIFront;
        wheels[i].mRadius = settings.wheelRadiusFront;
        wheels[i].mWidth  = settings.wheelWidthFront;
        wheels[i].mMaxBrakeTorque = settings.maxBrakeTorque;
        wheels[i].mDampingRate = settings.wheelDampingRate;
    }
    for(u32 i = PxVehicleDrive4WWheelOrder::eREAR_LEFT; i < numWheels; ++i)
    {
        wheels[i].mMass   = settings.wheelMassRear;
        wheels[i].mMOI    = wheelMOIRear;
        wheels[i].mRadius = settings.wheelRadiusRear;
        wheels[i].mWidth  = settings.wheelWidthRear;
        wheels[i].mMaxBrakeTorque = settings.maxBrakeTorque;
        wheels[i].mDampingRate = settings.wheelDampingRate;
    }
    wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mToeAngle  =  settings.frontToeAngle;
    wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mToeAngle = -settings.frontToeAngle;
    wheels[PxVehicleDrive4WWheelOrder::eREAR_LEFT].mMaxHandBrakeTorque  = settings.maxHandbrakeTorque;
    wheels[PxVehicleDrive4WWheelOrder::eREAR_RIGHT].mMaxHandBrakeTorque = settings.maxHandbrakeTorque;
    wheels[PxVehicleDrive4WWheelOrder::eFRONT_LEFT].mMaxSteer = settings.maxSteerAngle;
    wheels[PxVehicleDrive4WWheelOrder::eFRONT_RIGHT].mMaxSteer = settings.maxSteerAngle;

    PxVehicleTireData tires[PX_MAX_NB_WHEELS];
    for(PxU32 i = 0; i < numWheels; i++)
    {
        tires[i].mType = NORMAL_TIRE;
    }

    // set up suspension
    PxVehicleSuspensionData suspensions[PX_MAX_NB_WHEELS];
    PxF32 suspSprungMasses[PX_MAX_NB_WHEELS];
    PxVehicleComputeSprungMasses(numWheels, wheelCenterOffsets, toPxVec3(settings.chassisCMOffset),
            settings.chassisMass, 2, suspSprungMasses);
    for(PxU32 i = 0; i < numWheels; i++)
    {
        suspensions[i].mMaxCompression = settings.suspensionMaxCompression;
        suspensions[i].mMaxDroop = settings.suspensionMaxDroop;
        suspensions[i].mSpringStrength = settings.suspensionSpringStrength;
        suspensions[i].mSpringDamperRate = settings.suspensionSpringDamperRate;
        suspensions[i].mSprungMass = suspSprungMasses[i];
    }

    for(PxU32 i = 0; i < numWheels; i+=2)
    {
        suspensions[i + 0].mCamberAtRest =  settings.camberAngleAtRest;
        suspensions[i + 1].mCamberAtRest = -settings.camberAngleAtRest;
        suspensions[i + 0].mCamberAtMaxDroop =  settings.camberAngleAtMaxDroop;
        suspensions[i + 1].mCamberAtMaxDroop = -settings.camberAngleAtMaxDroop;
        suspensions[i + 0].mCamberAtMaxCompression =  settings.camberAngleAtMaxCompression;
        suspensions[i + 1].mCamberAtMaxCompression = -settings.camberAngleAtMaxCompression;
    }

    PxVec3 suspTravelDirections[PX_MAX_NB_WHEELS];
    PxVec3 wheelCentreCMOffsets[PX_MAX_NB_WHEELS];
    PxVec3 suspForceAppCMOffsets[PX_MAX_NB_WHEELS];
    PxVec3 tireForceAppCMOffsets[PX_MAX_NB_WHEELS];
    for(PxU32 i = 0; i < numWheels; i++)
    {
        suspTravelDirections[i] = PxVec3(0, 0, -1);
        // wheel center offset is offset from rigid body center of mass.
        wheelCentreCMOffsets[i] = wheelCenterOffsets[i] - toPxVec3(settings.chassisCMOffset);
        // suspension force application point 0.3 metres below rigid body center of mass.
        suspForceAppCMOffsets[i] = PxVec3(wheelCentreCMOffsets[i].x, wheelCentreCMOffsets[i].y, -0.3f);
        // tire force application point 0.3 metres below rigid body center of mass.
        tireForceAppCMOffsets[i] = PxVec3(wheelCentreCMOffsets[i].x, wheelCentreCMOffsets[i].y, -0.3f);
    }

    PxFilterData qryFilterData(0, 0, 0, UNDRIVABLE_SURFACE);
    for(PxU32 i = 0; i < numWheels; i++)
    {
        wheelsSimData->setWheelData(i, wheels[i]);
        wheelsSimData->setTireData(i, tires[i]);
        wheelsSimData->setSuspensionData(i, suspensions[i]);
        wheelsSimData->setSuspTravelDirection(i, suspTravelDirections[i]);
        wheelsSimData->setWheelCentreOffset(i, wheelCentreCMOffsets[i]);
        wheelsSimData->setSuspForceAppPointOffset(i, suspForceAppCMOffsets[i]);
        wheelsSimData->setTireForceAppPointOffset(i, tireForceAppCMOffsets[i]);
        wheelsSimData->setSceneQueryFilterData(i, qryFilterData);
        wheelsSimData->setWheelShapeMapping(i, PxI32(i));
    }

    PxVehicleAntiRollBarData barFront;
    barFront.mWheel0 = PxVehicleDrive4WWheelOrder::eFRONT_LEFT;
    barFront.mWheel1 = PxVehicleDrive4WWheelOrder::eFRONT_RIGHT;
    barFront.mStiffness = settings.frontAntiRollbarStiffness;
    wheelsSimData->addAntiRollBarData(barFront);
    PxVehicleAntiRollBarData barRear;
    barRear.mWheel0 = PxVehicleDrive4WWheelOrder::eREAR_LEFT;
    barRear.mWheel1 = PxVehicleDrive4WWheelOrder::eREAR_RIGHT;
    barRear.mStiffness = settings.rearAntiRollbarStiffness;
    wheelsSimData->addAntiRollBarData(barRear);

    PxVehicleDriveSimData4W driveSimData;
    PxVehicleDifferential4WData diff;
    diff.mType = settings.differential;
    driveSimData.setDiffData(diff);

    PxVehicleEngineData engine;
    engine.mPeakTorque = settings.peekEngineTorque;
    engine.mMaxOmega = settings.maxEngineOmega;
    engine.mDampingRateFullThrottle = settings.engineDampingFullThrottle;
    engine.mDampingRateZeroThrottleClutchEngaged = settings.engineDampingZeroThrottleClutchEngaged;
    engine.mDampingRateZeroThrottleClutchDisengaged = settings.engineDampingZeroThrottleClutchDisengaged;
    driveSimData.setEngineData(engine);

    PxVehicleGearsData gears;
    gears.mNbRatios = settings.gearRatios.size();
    for (u32 i=0; i<settings.gearRatios.size(); ++i)
    {
        gears.mRatios[i] = settings.gearRatios[i];
    }
    gears.mFinalRatio = settings.finalGearRatio;
    gears.mSwitchTime = settings.gearSwitchTime;
    driveSimData.setGearsData(gears);

    PxVehicleClutchData clutch;
    clutch.mStrength = settings.clutchStrength;
    driveSimData.setClutchData(clutch);

    // ackermann steer accuracy
    PxVehicleAckermannGeometryData ackermann;
    ackermann.mAccuracy = settings.ackermannAccuracy;
    ackermann.mAxleSeparation =
        wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).x-
        wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).x;
    ackermann.mFrontWidth =
        wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_RIGHT).y-
        wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eFRONT_LEFT).y;
    ackermann.mRearWidth =
        wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_RIGHT).y -
        wheelsSimData->getWheelCentreOffset(PxVehicleDrive4WWheelOrder::eREAR_LEFT).y;
    driveSimData.setAckermannGeometryData(ackermann);

    // create a vehicle from the wheels and drive sim data.
    PxVehicleDrive4W* vehicle4W = PxVehicleDrive4W::allocate(numWheels);
    vehicle4W->setup(gPhysics, veh4WActor, *wheelsSimData, driveSimData, numWheels - 4);
    wheelsSimData->free();

    // configure the userdata
    configureUserData(vehicle4W, &vehicle->actorUserData, vehicle->wheelShapeUserData);

    Vec3 pos = transform.position();
    Mat4 r = transform.rotation();
    PxMat33 rot;
    rot[0] = PxVec3(r[0].x, r[0].y, r[0].z);
    rot[1] = PxVec3(r[1].x, r[1].y, r[1].z);
    rot[2] = PxVec3(r[2].x, r[2].y, r[2].z);
    PxTransform startTransform(PxVec3(pos.x, pos.y, pos.z), PxQuat(rot));
    vehicle4W->getRigidDynamicActor()->setGlobalPose(startTransform);
    vehicle4W->getRigidDynamicActor()->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    scene->addActor(*vehicle4W->getRigidDynamicActor());

    vehicle4W->setToRestState();
    vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    vehicle4W->mDriveDynData.setUseAutoGears(true);
    vehicle4W->mDriveDynData.setAutoBoxSwitchTime(settings.autoBoxSwitchTime);

    vehicle->vehicle4W = vehicle4W;

    return vehicle;
}
