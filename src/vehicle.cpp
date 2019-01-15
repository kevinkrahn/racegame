#include "vehicle.h"

#include "game.h"
#include <vehicle/PxVehicleUtil.h>

const u32 QUERY_HITS_PER_WHEEL = 8;

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

PxVehicleKeySmoothingData keySmoothingData = {
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

PxVehiclePadSmoothingData padSmoothingData = {
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

PxFilterFlags VehicleFilterShader(
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

    return PxFilterFlags();
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

VehicleSceneQueryData* VehicleSceneQueryData::allocate(
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

PxBatchQuery* VehicleSceneQueryData::setUpBatchedSceneQuery(const PxU32 batchId, const VehicleSceneQueryData& vehicleSceneQueryData, PxScene* scene)
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

static PxVehicleDrivableSurfaceToTireFrictionPairs* createFrictionPairs(PhysicsVehicleSettings const& settings,
        const PxMaterial** materials)
{
    f32 frictionTable[MAX_NUM_SURFACE_TYPES] = {
        settings.trackTireFriction, // TARMAC
        settings.offroadTireFriction, // SAND
    };

    PxVehicleDrivableSurfaceType surfaceTypes[MAX_NUM_SURFACE_TYPES] = {
        { SURFACE_TYPE_TARMAC },
        { SURFACE_TYPE_SAND },
    };

    const u32 numTireTypes = 1;
    PxVehicleDrivableSurfaceToTireFrictionPairs* surfaceTirePairs =
        PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(numTireTypes, MAX_NUM_SURFACE_TYPES);

    surfaceTirePairs->setup(numTireTypes, MAX_NUM_SURFACE_TYPES, materials, surfaceTypes);

    for(u32 i = 0; i < MAX_NUM_SURFACE_TYPES; i++)
    {
        for(u32 j = 0; j < numTireTypes; j++)
        {
            surfaceTirePairs->setTypePairFriction(i, j, frictionTable[i]);
        }
    }

    return surfaceTirePairs;
}

static PxConvexMesh* createConvexMesh(const PxVec3* verts, const PxU32 numVerts)
{
    PxConvexMeshDesc convexDesc;
    convexDesc.points.count  = numVerts;
    convexDesc.points.stride = sizeof(PxVec3);
    convexDesc.points.data   = verts;
    convexDesc.flags         = PxConvexFlag::eCOMPUTE_CONVEX;

    PxConvexMesh* convexMesh = nullptr;
    PxDefaultMemoryOutputStream buf;
    if(game.physx.cooking->cookConvexMesh(convexDesc, buf))
    {
        PxDefaultMemoryInputData id(buf.getData(), buf.getSize());
        convexMesh = game.physx.physics->createConvexMesh(id);
    }

    return convexMesh;
}

static PxConvexMesh* createWheelMesh(const PxF32 width, const PxF32 radius)
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

void Vehicle::setupPhysics(PxScene* scene, PhysicsVehicleSettings const& settings, PxMaterial* vehicleMaterial,
	    const PxMaterial** surfaceMaterials, glm::mat4 const& transform)
{
    sceneQueryData = VehicleSceneQueryData::allocate(1, NUM_WHEELS, QUERY_HITS_PER_WHEEL, 1,
            &WheelSceneQueryPreFilterNonBlocking, &WheelSceneQueryPostFilterNonBlocking, game.physx.allocator);
    batchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *sceneQueryData, scene);
    frictionPairs = createFrictionPairs(settings, surfaceMaterials);

    PxConvexMesh* wheelConvexMeshes[NUM_WHEELS];
    PxMaterial* wheelMaterials[NUM_WHEELS];

    PxConvexMesh* wheelMeshFront = createWheelMesh(settings.wheelWidthFront, settings.wheelRadiusFront);
    for(u32 i = WHEEL_FRONT_LEFT; i <= WHEEL_FRONT_RIGHT; ++i)
    {
        wheelConvexMeshes[i] = wheelMeshFront;
        wheelMaterials[i] = vehicleMaterial;
    }

    PxConvexMesh* wheelMeshRear = createWheelMesh(settings.wheelWidthRear, settings.wheelRadiusRear);
    for(u32 i = WHEEL_REAR_LEFT; i < NUM_WHEELS; ++i)
    {
        wheelConvexMeshes[i] = wheelMeshRear;
        wheelMaterials[i] = vehicleMaterial;
    }

    PxFilterData chassisSimFilterData(COLLISION_FLAG_CHASSIS, COLLISION_FLAG_CHASSIS, 0, 0);
    PxFilterData chassisQryFilterData(COLLISION_FLAG_CHASSIS, 0, 0, UNDRIVABLE_SURFACE);
    PxFilterData wheelSimFilterData(0, 0, 0, 0);
    PxFilterData wheelQryFilterData(0, 0, 0, UNDRIVABLE_SURFACE);

    PxRigidDynamic* actor = game.physx.physics->createRigidDynamic(convert(transform));

    for(PxU32 i = 0; i < NUM_WHEELS; i++)
    {
        PxConvexMeshGeometry geom(wheelConvexMeshes[i]);
        PxShape* wheelShape = PxRigidActorExt::createExclusiveShape(*actor, geom, *wheelMaterials[i]);
        wheelShape->setQueryFilterData(wheelQryFilterData);
        wheelShape->setSimulationFilterData(wheelSimFilterData);
        wheelShape->setLocalPose(PxTransform(PxIdentity));
    }

    for (auto const& cm : settings.collisionMeshes)
    {
        PxVec3 scale(glm::length(glm::vec3(cm.transform[0])),
                     glm::length(glm::vec3(cm.transform[1])),
                     glm::length(glm::vec3(cm.transform[2])));
        PxShape* chassisShape = PxRigidActorExt::createExclusiveShape(*actor,
                PxConvexMeshGeometry(cm.convexMesh, PxMeshScale(scale)), *vehicleMaterial);
        chassisShape->setQueryFilterData(chassisQryFilterData);
        chassisShape->setSimulationFilterData(chassisSimFilterData);
        chassisShape->setLocalPose(convert(cm.transform));
        chassisShape->setContactOffset(0.1f);
        chassisShape->setRestOffset(0.08f);
    }

    PxVec3 centerOfMassOffset = convert(settings.centerOfMass);
    PxRigidBodyExt::updateMassAndInertia(*actor, settings.chassisDensity);
    actor->setCMassLocalPose(PxTransform(centerOfMassOffset, PxQuat(PxIdentity)));
    actor->userData = &actorUserData;

    f32 wheelMOIFront = 0.5f * settings.wheelMassFront * square(settings.wheelRadiusFront);
    f32 wheelMOIRear = 0.5f * settings.wheelMassRear * square(settings.wheelRadiusRear);

    PxVehicleWheelData wheels[NUM_WHEELS];
    for(u32 i = PxVehicleDrive4WWheelOrder::eFRONT_LEFT; i <= PxVehicleDrive4WWheelOrder::eFRONT_RIGHT; ++i)
    {
        wheels[i].mMass   = settings.wheelMassFront;
        wheels[i].mMOI    = wheelMOIFront;
        wheels[i].mRadius = settings.wheelRadiusFront;
        wheels[i].mWidth  = settings.wheelWidthFront;
        wheels[i].mMaxBrakeTorque = settings.maxBrakeTorque;
        wheels[i].mDampingRate = settings.wheelDampingRate;
    }
    for(u32 i = PxVehicleDrive4WWheelOrder::eREAR_LEFT; i < NUM_WHEELS; ++i)
    {
        wheels[i].mMass   = settings.wheelMassRear;
        wheels[i].mMOI    = wheelMOIRear;
        wheels[i].mRadius = settings.wheelRadiusRear;
        wheels[i].mWidth  = settings.wheelWidthRear;
        wheels[i].mMaxBrakeTorque = settings.maxBrakeTorque;
        wheels[i].mDampingRate = settings.wheelDampingRate;
    }
    wheels[WHEEL_FRONT_LEFT].mToeAngle  =  settings.frontToeAngle;
    wheels[WHEEL_FRONT_RIGHT].mToeAngle = -settings.frontToeAngle;
    wheels[WHEEL_REAR_LEFT].mToeAngle  =  settings.rearToeAngle;
    wheels[WHEEL_REAR_RIGHT].mToeAngle = -settings.rearToeAngle;
    wheels[WHEEL_REAR_LEFT].mMaxHandBrakeTorque  = settings.maxHandbrakeTorque;
    wheels[WHEEL_REAR_RIGHT].mMaxHandBrakeTorque = settings.maxHandbrakeTorque;
    wheels[WHEEL_FRONT_LEFT].mMaxSteer = settings.maxSteerAngle;
    wheels[WHEEL_FRONT_RIGHT].mMaxSteer = settings.maxSteerAngle;

    PxVehicleTireData tires[NUM_WHEELS];
    for(PxU32 i = 0; i < NUM_WHEELS; i++)
    {
        tires[i].mType = 0;
        // TODO: investigate other tire parameters
    }

    PxVec3 wheelCenterOffsets[NUM_WHEELS];
    wheelCenterOffsets[WHEEL_FRONT_LEFT]  = convert(settings.wheelPositions[WHEEL_FRONT_LEFT]);
    wheelCenterOffsets[WHEEL_FRONT_RIGHT] = convert(settings.wheelPositions[WHEEL_FRONT_RIGHT]);
    wheelCenterOffsets[WHEEL_REAR_LEFT]   = convert(settings.wheelPositions[WHEEL_REAR_LEFT]);
    wheelCenterOffsets[WHEEL_REAR_RIGHT]  = convert(settings.wheelPositions[WHEEL_REAR_RIGHT]);

    // set up suspension
    PxVehicleSuspensionData suspensions[NUM_WHEELS];
    PxF32 suspSprungMasses[NUM_WHEELS];
    PxVehicleComputeSprungMasses(NUM_WHEELS, wheelCenterOffsets, centerOfMassOffset,
            actor->getMass(), 2, suspSprungMasses);
    for(PxU32 i = 0; i < NUM_WHEELS; i++)
    {
        suspensions[i].mMaxCompression = settings.suspensionMaxCompression;
        suspensions[i].mMaxDroop = settings.suspensionMaxDroop;
        suspensions[i].mSpringStrength = settings.suspensionSpringStrength;
        suspensions[i].mSpringDamperRate = settings.suspensionSpringDamperRate;
        suspensions[i].mSprungMass = suspSprungMasses[i];
    }

    for(PxU32 i = 0; i < NUM_WHEELS; i+=2)
    {
        suspensions[i + 0].mCamberAtRest =  settings.camberAngleAtRest;
        suspensions[i + 1].mCamberAtRest = -settings.camberAngleAtRest;
        suspensions[i + 0].mCamberAtMaxDroop =  settings.camberAngleAtMaxDroop;
        suspensions[i + 1].mCamberAtMaxDroop = -settings.camberAngleAtMaxDroop;
        suspensions[i + 0].mCamberAtMaxCompression =  settings.camberAngleAtMaxCompression;
        suspensions[i + 1].mCamberAtMaxCompression = -settings.camberAngleAtMaxCompression;
    }

    PxVec3 suspTravelDirections[NUM_WHEELS];
    PxVec3 wheelCentreCMOffsets[NUM_WHEELS];
    PxVec3 suspForceAppCMOffsets[NUM_WHEELS];
    PxVec3 tireForceAppCMOffsets[NUM_WHEELS];
    for(PxU32 i = 0; i < NUM_WHEELS; i++)
    {
        suspTravelDirections[i] = PxVec3(0, 0, -1);
        // wheel center offset is offset from rigid body center of mass.
        wheelCentreCMOffsets[i] = wheelCenterOffsets[i] - centerOfMassOffset;
        // suspension force application point 0.3 metres below rigid body center of mass.
        suspForceAppCMOffsets[i] = PxVec3(wheelCentreCMOffsets[i].x, wheelCentreCMOffsets[i].y, -0.3f);
        // tire force application point 0.3 metres below rigid body center of mass.
        tireForceAppCMOffsets[i] = PxVec3(wheelCentreCMOffsets[i].x, wheelCentreCMOffsets[i].y, -0.3f);
    }

    PxVehicleWheelsSimData* wheelsSimData = PxVehicleWheelsSimData::allocate(NUM_WHEELS);
    PxFilterData qryFilterData(0, 0, 0, UNDRIVABLE_SURFACE);
    for(PxU32 i = 0; i < NUM_WHEELS; i++)
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
    vehicle4W = PxVehicleDrive4W::allocate(NUM_WHEELS);
    vehicle4W->setup(game.physx.physics, actor, *wheelsSimData, driveSimData, NUM_WHEELS - 4);
    wheelsSimData->free();

    actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    scene->addActor(*actor);

    vehicle4W->setToRestState();
    vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    vehicle4W->mDriveDynData.setUseAutoGears(true);
    vehicle4W->mDriveDynData.setAutoBoxSwitchTime(settings.autoBoxSwitchTime);
}

Vehicle::Vehicle(Scene* scene, glm::mat4 const& transform, glm::vec3 const& startOffset,
	    Driver* driver, PxMaterial* vehicleMaterial, const PxMaterial** surfaceMaterials,
	    u32 vehicleIndex)
{
    this->cameraTarget = translationOf(transform);
    this->targetOffset = startOffset;
    this->startOffset = startOffset;
    this->lastDamagedBy = vehicleIndex;
    this->vehicleIndex = vehicleIndex;
    this->offsetChangeInterval = random(scene->randomSeries, 5.f, 15.f);
    this->followPathIndex = irandom(scene->randomSeries, 0, scene->getPaths().size());
    this->driver = driver;
    this->scene = scene;

    setupPhysics(scene->getPhysicsScene(), driver->vehicleData->physics, vehicleMaterial, surfaceMaterials, transform);
    actorUserData.entityType = ActorUserData::VEHICLE;
    actorUserData.vehicle = this;
}

Vehicle::~Vehicle()
{
	vehicle4W->getRigidDynamicActor()->release();
	vehicle4W->free();
	sceneQueryData->free(game.physx.allocator);
    // TODO: find out why this causes an exception
	//batchQuery->release();
	frictionPairs->release();
}

void Vehicle::reset(glm::mat4 const& transform) const
{
    vehicle4W->setToRestState();
    vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    getRigidBody()->setGlobalPose(convert(transform));
}

void Vehicle::updatePhysics(PxScene* scene, f32 timestep, bool digital,
        f32 accel, f32 brake, f32 steer, bool handbrake, bool canGo, bool onlyBrake)
{
    if (canGo)
    {
        PxVehicleDrive4WRawInputData inputs;
        if (onlyBrake)
        {
            inputs.setAnalogBrake(brake);
            PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(
                    padSmoothingData, steerVsForwardSpeedTable, inputs, timestep,
                    isInAir, *vehicle4W);
        }
        else
        {
            if (accel)
            {
                if (vehicle4W->mDriveDynData.mCurrentGear == PxVehicleGearsData::eREVERSE || getForwardSpeed() < 7.f)
                {
                    //vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
                    vehicle4W->mDriveDynData.setTargetGear(PxVehicleGearsData::eFIRST);
                }
                if (digital) inputs.setDigitalAccel(true);
                else inputs.setAnalogAccel(accel);
            }
            if (brake)
            {
                if (vehicle4W->computeForwardSpeed() < 1.5f && !accel)
                {
                    //vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eREVERSE);
                    vehicle4W->mDriveDynData.setTargetGear(PxVehicleGearsData::eREVERSE);
                    if (digital) inputs.setDigitalAccel(true);
                    else inputs.setAnalogAccel(brake);
                }
                else
                {
                    if (digital) inputs.setDigitalBrake(true);
                    else inputs.setAnalogBrake(brake);
                }
            }
            if (digital)
            {
                inputs.setDigitalHandbrake(handbrake);
                inputs.setDigitalSteerLeft(steer < 0);
                inputs.setDigitalSteerRight(steer > 0);

                PxVehicleDrive4WSmoothDigitalRawInputsAndSetAnalogInputs(
                        keySmoothingData, steerVsForwardSpeedTable, inputs, timestep,
                        isInAir, *vehicle4W);
            }
            else
            {
                inputs.setAnalogHandbrake(handbrake);
                inputs.setAnalogSteer(steer);

                PxVehicleDrive4WSmoothAnalogRawInputsAndSetAnalogInputs(
                        padSmoothingData, steerVsForwardSpeedTable, inputs, timestep,
                        isInAir, *vehicle4W);
            }
        }
    }

	PxVehicleWheels* vehicles[1] = { vehicle4W };
	PxSweepQueryResult* sweepResults = sceneQueryData->getSweepQueryResultBuffer(0);
	const PxU32 sweepResultsSize = sceneQueryData->getQueryResultBufferSize();
	PxVehicleSuspensionSweeps(batchQuery, 1, vehicles, sweepResultsSize, sweepResults, QUERY_HITS_PER_WHEEL, NULL, 1.0f, 1.01f);

	const PxVec3 grav = scene->getGravity();
	PxVehicleWheelQueryResult vehicleQueryResults[1] = {
	    { wheelQueryResults, NUM_WHEELS }
	};
	PxVehicleUpdates(timestep, grav, *frictionPairs, 1, vehicles, vehicleQueryResults);

    isInAir = PxVehicleIsInAir(vehicleQueryResults[0]);
}

bool Vehicle::isBlocking(f32 radius, glm::vec3 const& dir, f32 dist)
{
    PxSweepBuffer hit;
    if (!scene->sweep(radius, getPosition() + glm::vec3(0, 0, 0.25f), dir, dist, &hit, getRigidBody()))
    {
        return false;
    }
    if (hit.block.actor->getType() == PxActorType::eRIGID_STATIC)
    {
        return true;
    }

    glm::vec3 otherVelocity = convert(((PxRigidDynamic*)hit.block.actor)->getLinearVelocity());
    glm::vec3 myVelocity = convert(getRigidBody()->getLinearVelocity());
    if (glm::dot(glm::normalize(otherVelocity), glm::normalize(myVelocity)) > 0.5f)
    {
        if (glm::length(myVelocity) - glm::length(otherVelocity) > 1.f)
        {
            return true;
        }
    }

    return false;
}

void Vehicle::onUpdate(f32 deltaTime, i32 cameraIndex)
{
    // HUD
    if (cameraIndex >= 0)
    {
        Font& font1 = game.resources.getFont("font", game.windowHeight * 0.04);
        Font& font2 = game.resources.getFont("font", game.windowHeight * 0.08);

        ViewportLayout const& layout = viewportLayout[scene->getViewportCount() - 1];
        glm::vec2 dim(game.windowWidth, game.windowHeight);
        glm::vec2 offset = layout.offsets[cameraIndex] * dim;
        glm::vec2 d(1.f, 1.f);
        if (offset.y > 0.f)
        {
            offset.y = game.windowHeight - font2.getHeight();
            d.y = -1;
        }

        f32 o20 = game.windowHeight * 0.02f;
        f32 o25 = game.windowHeight * 0.03f;
        f32 o200 = game.windowHeight * 0.20f;

        std::string p = str(glm::min(currentLap, scene->getTotalLaps()));
        f32 lapWidth = font1.stringDimensions(str("LAP").c_str()).x;
        font1.drawText(str("LAP").c_str(), offset + glm::vec2(o20, d.y*o20), glm::vec3(1.f));
        font2.drawText(p.c_str(), offset +
                glm::vec2(o25 + lapWidth, d.y*o20), glm::vec3(1.f));
        font1.drawText(str('/', scene->getTotalLaps()).c_str(),
                offset + glm::vec2(o25 + lapWidth + font2.stringDimensions(p.c_str()).x, d.y*o20), glm::vec3(1.f));

        const char* placementSuffix[] = { "st", "nd", "rd", "th", "th", "th", "th", "th" };
        glm::vec3 col = glm::mix(glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), placement / 8.f);

        p = str(placement + 1);
        font2.drawText(p.c_str(), offset + glm::vec2(o200, d.y*o20), col);
        font1.drawText(str(placementSuffix[placement]).c_str(),
                offset + glm::vec2(o200 + font2.stringDimensions(p.c_str()).x, d.y*o20), col);

        // display notifications
        u32 count = 0;
        for (auto it = notifications.begin(); it != notifications.end();)
        {
            font1.drawText(it->str, layout.offsets[cameraIndex] * dim + layout.scale * dim * 0.5f - glm::vec2(0, layout.scale.y * dim.y * 0.3) + glm::vec2(0, count * dim.y * 0.03),
                    { 1, 1, 1 }, 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::CENTER);
            ++count;
            it->timeLeft -= deltaTime;
            if (it->timeLeft <= 0.f)
            {
                notifications.erase(it);
                continue;
            }
            ++it;
        }
    }

    if (deadTimer > 0.f)
    {
        deadTimer -= deltaTime;
        if (deadTimer <= 0.f)
        {
            backupTimer = 0.f;
            deadTimer = 0.f;
            hitPoints = 100.f;

            const TrackGraph::Node* node = graphResult.lastNode;
            glm::vec2 dir(node->direction);
            glm::vec3 pos = node->position -
                glm::vec3(targetOffset.x * dir + targetOffset.y * glm::vec2(-dir.y, dir.x), 0);

            reset(glm::translate(glm::mat4(1.f), pos + glm::vec3(0, 0, 5)) *
                  glm::rotate(glm::mat4(1.f), node->angle, glm::vec3(0, 0, 1)));
        }
        return;
    }

    glm::mat4 transform = getTransform();
    bool canGo = true;
    bool isPlayerControlled = driver->playerProfile != nullptr;
    if (!finishedRace)
    {
        if (isPlayerControlled)
        {
            f32 accel = 0.f;
            f32 brake = 0.f;
            f32 steer = 0.f;
            bool digital = false;
            bool shoot = false;
            if (driver->useKeyboard)
            {
                digital = true;
                accel = game.input.isKeyDown(KEY_UP);
                brake = game.input.isKeyDown(KEY_DOWN);
                steer = (f32)game.input.isKeyDown(KEY_LEFT) - (f32)game.input.isKeyDown(KEY_RIGHT);
                shoot = game.input.isKeyPressed(KEY_C);
            }
            else
            {
                Controller* controller = game.input.getController(driver->controllerID);
                if (controller)
                {
                    accel = controller->getAxis(AXIS_TRIGGER_RIGHT);
                    brake = controller->getAxis(AXIS_TRIGGER_LEFT);
                    steer = -controller->getAxis(AXIS_LEFT_X);
                    shoot = controller->isButtonPressed(BUTTON_RIGHT_SHOULDER);
                }
            }

            updatePhysics(scene->getPhysicsScene(), deltaTime, digital,
                    accel, brake, steer, false, true, false);

            if (game.input.isKeyPressed(KEY_F))
            {
                getRigidBody()->addForce(PxVec3(0, 0, 10), PxForceMode::eVELOCITY_CHANGE);
                getRigidBody()->addTorque(
                        getRigidBody()->getGlobalPose().q.rotate(PxVec3(5, 0, 0)),
                        PxForceMode::eVELOCITY_CHANGE);
            }

            if (shoot)
            {
                fireWeapon();
            }
        }
        else
        {
            i32 previousIndex = targetPointIndex - 1;
            if (previousIndex < 0) previousIndex = scene->getPaths()[followPathIndex].size() - 1;

            glm::vec3 nextP = scene->getPaths()[followPathIndex][targetPointIndex];
            glm::vec3 previousP = scene->getPaths()[followPathIndex][previousIndex];
            glm::vec2 dir = glm::normalize(glm::vec2(nextP) - glm::vec2(previousP));

            glm::vec3 targetP = nextP -
                glm::vec3(targetOffset.x * dir + targetOffset.y * glm::vec2(-dir.y, dir.x), 0);

            glm::vec3 position = getPosition();
            glm::vec2 dirToTargetP = glm::normalize(glm::vec2(position - targetP));
            f32 steerAngle = glm::dot(glm::vec2(getRightVector()), dirToTargetP);

            f32 forwardTestDist = 14.f;
            f32 sideTestDist = 9.f;
            f32 testAngle = 0.65f;
            glm::vec3 testDir1(glm::rotate(glm::mat4(1.f), testAngle, { 0, 0, 1 }) * glm::vec4(getForwardVector(), 1.0));
            glm::vec3 testDir2(glm::rotate(glm::mat4(1.f), -testAngle, { 0, 0, 1 }) * glm::vec4(getForwardVector(), 1.0));
            //game.renderer.drawLine(position, position + testDir1 * sideTestDist);
            //game.renderer.drawLine(position, position + testDir2 * sideTestDist);

            f32 accel = 0.85f;
            f32 brake = 0.f;
            bool isSomethingBlockingMe = isBlocking(driver->vehicleData->collisionWidth / 2 + 0.05f,
                    getForwardVector(), forwardTestDist);
            if (isSomethingBlockingMe && glm::dot(glm::vec2(getForwardVector()), -dirToTargetP) > 0.8f)
            {
                const f32 avoidSteerAmount = 0.5f;
                bool left = isBlocking(0.5f, testDir1, sideTestDist);
                bool right = isBlocking(0.5f, testDir2, sideTestDist);
                if (!left && !right)
                {
                    glm::vec3 d = glm::normalize(targetP - position);
                    f32 diff1 = glm::dot(d, testDir1);
                    f32 diff2 = glm::dot(d, testDir2);
                    steerAngle = diff1 < diff2 ?
                        glm::min(steerAngle, -avoidSteerAmount) : glm::max(steerAngle, avoidSteerAmount);
                }
                else if (!left)
                {
                    steerAngle = glm::min(steerAngle, -avoidSteerAmount);
                }
                else if (!right)
                {
                    steerAngle = glm::max(steerAngle, avoidSteerAmount);
                }
                else
                {
                    targetP = nextP;
                    steerAngle = glm::dot(glm::vec2(getRightVector()), glm::normalize(glm::vec2(position - targetP)));
                    if (getForwardSpeed() > 18.f)
                    {
                        brake = 0.8f;
                    }
                }
            }

            if (canGo)
            {
                backupTimer = (getForwardSpeed() < 2.5f) ? backupTimer + deltaTime : 0.f;
                if (backupTimer > 2.f)
                {
                    accel = 0.f;
                    brake = 1.f;
                    steerAngle *= -1.f;
                    if (backupTimer > 5.f || getForwardSpeed() < -9.f)
                    {
                        backupTimer = 0.f;
                    }
                }
            }

            if (!finishedRace)
            {
                updatePhysics(scene->getPhysicsScene(), deltaTime, false, accel, brake,
                        -steerAngle, false, canGo, false);
            }

            if (glm::length2(nextP - position) < square(30.f))
            {
                ++targetPointIndex;
                if (targetPointIndex >= scene->getPaths()[followPathIndex].size())
                {
                    targetPointIndex = 0;
                }
            }
        }

        offsetChangeTimer += deltaTime;
        if (offsetChangeTimer > offsetChangeInterval)
        {
            targetOffset.x = random(scene->randomSeries, -8.f, 8.f);
            targetOffset.y = random(scene->randomSeries, -8.f, 8.f);
            offsetChangeTimer = 0.f;
            offsetChangeInterval = random(scene->randomSeries, 5.f, 15.f);
        }
    }
    else
    {
        updatePhysics(scene->getPhysicsScene(), deltaTime, false, 0.f,
                controlledBrakingTimer < 0.5f ? 0.f : 0.5f, 0.f, 0.f, true, true);
        if (getForwardSpeed() > 1.f)
        {
            controlledBrakingTimer = glm::min(controlledBrakingTimer + deltaTime, 1.f);
        }
        else
        {
            controlledBrakingTimer = glm::max(controlledBrakingTimer - deltaTime, 0.f);
        }
    }

    const f32 maxSkippableDistance = 250.f;
    if (canGo)
    {
        scene->getTrackGraph().findLapDistance(getPosition(), graphResult, maxSkippableDistance);
    }

    // check if crossed finish line
    if (!finishedRace && graphResult.lapDistanceLowMark < maxSkippableDistance)
    {
        glm::vec3 finishLinePosition = translationOf(scene->getStart());
        glm::vec3 dir = glm::normalize(getPosition() - finishLinePosition);
        if (glm::dot(xAxisOf(scene->getStart()), dir) > 0.f
                && glm::length2(getPosition() - finishLinePosition) < square(40.f))
        {
            if (!finishedRace && currentLap >= scene->getTotalLaps())
            {
                finishedRace = true;
                scene->vehicleFinish(vehicleIndex);
            }
            ++currentLap;
            graphResult.lapDistanceLowMark = scene->getTrackGraph().getStartNode()->t;
            graphResult.currentLapDistance = scene->getTrackGraph().getStartNode()->t;
        }
    }

    if (cameraIndex >= 0)
    {
        glm::vec3 pos = getPosition();
#if 0
        cameraTarget = pos;
#else
        cameraTarget = smoothMove(cameraTarget,
                pos + glm::vec3(glm::normalize(glm::vec2(getForwardVector())), 0.f) * getForwardSpeed() * 0.3f,
                5.f, deltaTime);
#endif
        f32 camDistance = 80.f;
        glm::vec3 cameraFrom = cameraTarget + glm::normalize(glm::vec3(1.f, 1.f, 1.25f)) * camDistance;
        game.renderer.setViewportCamera(cameraIndex, cameraFrom, cameraTarget, 30.f, 180.f);

        // draw arrow if vehicle is hidden behind something
        glm::vec3 rayStart = getPosition();
        glm::vec3 diff = cameraFrom - rayStart;
        glm::vec3 rayDir = glm::normalize(diff);
        f32 dist = glm::length(diff);
        PxRaycastBuffer hit;
        PxQueryFilterData filter;
        filter.flags |= PxQueryFlag::eANY_HIT;
        filter.flags |= PxQueryFlag::eSTATIC;
        filter.data = PxFilterData(COLLISION_FLAG_GROUND, 0, 0, 0);
        bool visible = false;
        for (u32 i=0; i<NUM_WHEELS; ++i)
        {
            glm::vec3 wheelPosition = transform * glm::vec4(convert(wheelQueryResults[i].localPose.p), 1.0);
            if (!scene->raycastStatic(wheelPosition, rayDir, dist))
            {
                visible = true;
                break;
            }
        }
        if (!visible)
        {
            game.renderer.drawMeshOverlay(game.resources.getMesh("world.Arrow").renderHandle,
                    cameraIndex, transform, glm::vec3(driver->vehicleColor));
        }
    }

    // destroy vehicle if off track or out of bounds
    bool onGround = false;
    if (getPosition().z < -8.f)
    {
        applyDamage(100.f, vehicleIndex);
    }
    else
    {
        PxRaycastBuffer hit;
        if (scene->raycastStatic(getPosition(), { 0, 0, -1 }, 3.0f, &hit))
        {
            onGround = true;
            if (hit.block.actor->userData)
            {
                if (((ActorUserData*)hit.block.actor->userData)->entityType != ActorUserData::TRACK)
                {
                    applyDamage(100.f, vehicleIndex);
                }
            }
        }
    }

    // update wheels
    smokeTimer = glm::max(0.f, smokeTimer - deltaTime);
    const f32 smokeInterval = 0.015f;
    bool smoked = false;
    u32 numWheelsOnGround = 0;
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        auto info = wheelQueryResults[i];
        bool isWheelOffroad = false;
        if (!info.isInAir)
        {
            ++numWheelsOnGround;

            // increase damping when offroad
            if (info.tireSurfaceMaterial == scene->offroadMaterial)
            {
                PxVehicleWheelData d = vehicle4W->mWheelsSimData.getWheelData(i);
                d.mDampingRate = driver->vehicleData->physics.offroadDampingRate;
                vehicle4W->mWheelsSimData.setWheelData(i, d);
                isWheelOffroad = true;
            }
            else
            {
                PxVehicleWheelData d = vehicle4W->mWheelsSimData.getWheelData(i);
                d.mDampingRate = driver->vehicleData->physics.wheelDampingRate;
                vehicle4W->mWheelsSimData.setWheelData(i, d);
            }
        }

        f32 lateralSlip = glm::abs(info.lateralSlip) - 0.4f;
        f32 longitudinalSlip = glm::abs(info.longitudinalSlip) - 0.6f;
        f32 slip = glm::max(lateralSlip, longitudinalSlip);
        bool wasWheelSlipping = isWheelSlipping[i];
        isWheelSlipping[i] = slip > 0.f && !info.isInAir;

        // create smoke
        if (slip > 0.f && !info.isInAir && !isWheelOffroad)
        {
            if (smokeTimer == 0.f)
            {
                glm::vec3 wheelPosition = transform * glm::vec4(convert(info.localPose.p), 1.0);
                glm::vec3 vel(glm::normalize(glm::vec3(
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f))));
                scene->smokeParticleSystem.spawn(
                    wheelPosition - glm::vec3(0, 0, 0.2f),
                    (vel + glm::vec3(0, 0, 1)) * 0.8f,
                    glm::min(1.f, slip * 0.5f));
                smoked = true;
            }
        }

        if (isWheelOffroad && smokeTimer == 0.f)
        {
            f32 wheelRotationSpeed = vehicle4W->mWheelsDynData.getWheelRotationSpeed(i);
            if (wheelRotationSpeed > 5.f || slip > 0.f)
            {
                glm::vec3 wheelPosition = transform * glm::vec4(convert(info.localPose.p), 1.0);
                glm::vec3 vel(glm::normalize(glm::vec3(
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f))));
                scene->smokeParticleSystem.spawn(
                    wheelPosition - glm::vec3(0, 0, 0.2f),
                    (vel + glm::vec3(0, 0, 1)) * 0.8f,
                    glm::clamp(glm::max(slip, glm::abs(wheelRotationSpeed * 0.022f)), 0.f, 1.f),
                    glm::vec4(0.58f, 0.50f, 0.22f, 1.f));
                smoked = true;
            }
        }

        // add tire marks
        if (isWheelSlipping[i])
        {
            f32 wheelRadius = i < 2 ? driver->vehicleData->physics.wheelRadiusFront : driver->vehicleData->physics.wheelRadiusRear;
            f32 wheelWidth = i < 2 ? driver->vehicleData->physics.wheelWidthFront : driver->vehicleData->physics.wheelWidthRear;
            glm::vec3 tn = convert(info.tireContactNormal);
            PxTransform contactPose = info.localPose;
            glm::vec3 markPosition = tn * -wheelRadius
                + translationOf(transform * convert(info.localPose));
            glm::vec4 color = isWheelOffroad ?
                glm::vec4(0.45f, 0.39f, 0.12f, 1.f) : glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
            tireMarkRibbons[i].addPoint(markPosition, tn, wheelWidth / 2, color);
        }
        else if (wasWheelSlipping)
        {
            tireMarkRibbons[i].capWithLastPoint();
        }
    }

    if (smoked) smokeTimer = smokeInterval;

    // destroy vehicle if it is flipped and unable to move
    if (onGround && numWheelsOnGround <= 1 && getRigidBody()->getLinearVelocity().magnitude() < 2.f)
    {
        flipTimer += deltaTime;
        if (flipTimer > 2.5f)
        {
            applyDamage(100.f, vehicleIndex);
        }
    }
    else
    {
        flipTimer = 0.f;
    }

    // explode
    if (hitPoints <= 0.f)
    {
        for(auto& d : driver->vehicleData->debrisChunks)
        {
			PxRigidDynamic* body = game.physx.physics->createRigidDynamic(
			        PxTransform(convert(transform * d.transform)));
			body->attachShape(*d.collisionShape);
			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			scene->getPhysicsScene()->addActor(*body);
	        body->setLinearVelocity(
	                getRigidBody()->getLinearVelocity() +
	                convert(glm::vec3(glm::normalize(rotationOf(transform) * glm::vec4(translationOf(d.transform), 1.0)))
	                    * random(scene->randomSeries, 5.f, 22.f) + glm::vec3(0, 0, 5.f)));
	        body->setAngularVelocity(PxVec3(
	                    random(scene->randomSeries, 0.f, 8.f),
	                    random(scene->randomSeries, 0.f, 8.f),
	                    random(scene->randomSeries, 0.f, 8.f)));

            scene->createVehicleDebris(VehicleDebris{
                body,
                d.mesh,
                0.f,
                driver->vehicleColor
            });
        }
        deadTimer = 1.f;
        reset(glm::translate(glm::mat4(1.f), { 0, 0, -100 }));
        scene->attackCredit(lastDamagedBy, vehicleIndex);
    }

    // draw chassis
    for (auto& mesh : driver->vehicleData->chassisMeshes)
    {
        game.renderer.drawMesh(mesh.renderHandle, transform * mesh.transform, driver->vehicleColor);
    }

    // draw wheels
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        glm::mat4 wheelTransform = transform * convert(wheelQueryResults[i].localPose);
        if ((i & 1) == 0)
        {
            wheelTransform = glm::rotate(wheelTransform, f32(M_PI), glm::vec3(0, 0, 1));
        }
        auto& mesh = i < 2 ? driver->vehicleData->wheelMeshFront : driver->vehicleData->wheelMeshRear;
        game.renderer.drawMesh(mesh.renderHandle, wheelTransform * mesh.transform);
    }
}

void Vehicle::fireWeapon()
{
    glm::mat4 transform = getTransform();
    f32 minSpeed = 40.f;
    glm::vec3 vel = convert(getRigidBody()->getLinearVelocity()) + getForwardVector() * minSpeed;
    if (glm::length2(vel) < square(minSpeed))
    {
        vel = glm::normalize(vel) * minSpeed;
    }
    scene->createProjectile(getPosition() + getForwardVector() * 3.f + getRightVector() * 0.8f,
            vel, zAxisOf(transform), vehicleIndex);
    scene->createProjectile(getPosition() + getForwardVector() * 3.f - getRightVector() * 0.8f,
            vel, zAxisOf(transform), vehicleIndex);
}
