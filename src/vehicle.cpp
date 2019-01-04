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

Vehicle::Vehicle(Scene const& scene, glm::mat4 const& transform, glm::vec3 const& startOffset,
        VehicleData* data, PxMaterial* vehicleMaterial, const PxMaterial** surfaceMaterials,
        bool isPlayerControlled, bool hasCamera, u32 vehicleIndex)
{
    this->isPlayerControlled = isPlayerControlled;
    this->hasCamera = hasCamera;
    this->vehicleData = data;
    this->cameraTarget = translationOf(transform);
    this->targetOffset = startOffset;

    setupPhysics(scene.getPhysicsScene(), data->physics, vehicleMaterial, surfaceMaterials, transform);
    actorUserData.entityType = ActorUserData::VEHICLE;
    actorUserData.vehicleIndex = vehicleIndex;
}

Vehicle::~Vehicle()
{
	vehicle4W->getRigidDynamicActor()->release();
	vehicle4W->free();
	sceneQueryData->free(game.physx.allocator);
	batchQuery->release();
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

void Vehicle::onUpdate(f32 deltaTime, Scene& scene, u32 vehicleIndex)
{
    if (deadTimer > 0.f)
    {
        deadTimer -= deltaTime;
        if (deadTimer <= 0.f)
        {
            backupTimer = 0.f;
            deadTimer = 0.f;
            hitPoints = 100.f;

            // TODO: instead of using targetOffset, attempt to place the vehicle in the middle of the
            // road and offset if there is an obstruction
            const TrackGraph::Node* node = graphResult.lastNode;
            glm::vec2 dir(node->direction);
            glm::vec3 pos = node->position -
                glm::vec3(targetOffset.x * dir + targetOffset.y * glm::vec2(-dir.y, dir.x), 0);

            reset(glm::translate(glm::mat4(1.f), pos + glm::vec3(0, 0, 5)) *
                  glm::rotate(glm::mat4(1.f), node->angle, glm::vec3(0, 0, 1)));
        }
        return;
    }

    bool canGo = true;
    if (!finishedRace)
    {
        if (isPlayerControlled)
        {
            updatePhysics(scene.getPhysicsScene(), deltaTime, true,
                    game.input.isKeyDown(KEY_UP), game.input.isKeyDown(KEY_DOWN),
                    (f32)game.input.isKeyDown(KEY_LEFT) - (f32)game.input.isKeyDown(KEY_RIGHT),
                    game.input.isKeyDown(KEY_SPACE), true, false);

            if (game.input.isKeyPressed(KEY_F))
            {
                getRigidBody()->addForce(PxVec3(0, 0, 10), PxForceMode::eVELOCITY_CHANGE);
                getRigidBody()->addTorque(
                        getRigidBody()->getGlobalPose().q.rotate(PxVec3(5, 0, 0)),
                        PxForceMode::eVELOCITY_CHANGE);
            }

            if (game.input.isKeyPressed(KEY_C))
            {
                scene.createProjectile(getPosition() + getForwardVector() * 4.f,
                        convert(getRigidBody()->getLinearVelocity()) + getForwardVector() * 40.f,
                        vehicleIndex);
            }
        }
        else
        {
            i32 previousIndex = targetPointIndex - 1;
            if (previousIndex < 0) previousIndex = scene.getPaths()[followPathIndex].size();

            glm::vec3 nextP = scene.getPaths()[followPathIndex][targetPointIndex];
            glm::vec3 previousP = scene.getPaths()[followPathIndex][previousIndex];
            glm::vec2 dir = glm::normalize(glm::vec2(nextP) - glm::vec2(previousP));
            glm::vec3 targetP = nextP -
                glm::vec3(targetOffset.x * dir + targetOffset.y * glm::vec2(-dir.y, dir.x), 0);

            f32 steerAngle = glm::dot(glm::vec2(getRightVector()),
                    glm::normalize(glm::vec2(getPosition() - targetP)));

            f32 accel = 0.85f;
            f32 brake = 0.f;

            if (canGo)
            {
                backupTimer = (getForwardSpeed() < 2.5f) ? backupTimer + deltaTime : 0.f;
                if (backupTimer > 2.f)
                {
                    accel = 0.f;
                    brake = 1.f;
                    steerAngle *= -1.f;
                    if (backupTimer > 3.5f)
                    {
                        backupTimer = 0.f;
                    }
                }
            }

            if (!finishedRace)
            {
                updatePhysics(scene.getPhysicsScene(), deltaTime, false, accel, brake,
                        -steerAngle, false, canGo, false);
            }

            if (glm::length2(nextP - getPosition()) < square(30.f))
            {
                ++targetPointIndex;
                if (targetPointIndex >= scene.getPaths()[followPathIndex].size())
                {
                    targetPointIndex = 0;
                }
            }
        }
    }
    else
    {
        updatePhysics(scene.getPhysicsScene(), deltaTime, false, 0.f,
                controlledBrakingTimer < 0.5f ? 0.f : 0.5f, 0.f, 0.f, true, true);
        if (getForwardSpeed() > 1.f)
        {
            controlledBrakingTimer = std::min(controlledBrakingTimer + deltaTime, 1.f);
        }
        else
        {
            controlledBrakingTimer = std::max(controlledBrakingTimer - deltaTime, 0.f);
        }
    }

    const f32 maxSkippableDistance = 250.f;
    if (canGo)
    {
        scene.getTrackGraph().findLapDistance(getPosition(), graphResult, maxSkippableDistance);
    }

    // check if crossed finish line
    if (!finishedRace && graphResult.lapDistanceLowMark < maxSkippableDistance)
    {
        glm::vec3 finishLinePosition = translationOf(scene.getStart());
        glm::vec3 dir = glm::normalize(getPosition() - finishLinePosition);
        if (glm::dot(xAxisOf(scene.getStart()), dir) > 0.f
                && glm::length2(getPosition() - finishLinePosition) < square(40.f))
        {
            if (currentLap >= scene.getTotalLaps())
            {
                finishedRace = true;
                scene.vehicleFinish(vehicleIndex);
            }
            else
            {
                ++currentLap;
                graphResult.lapDistanceLowMark = scene.getTrackGraph().getStartNode()->t;
                graphResult.currentLapDistance = scene.getTrackGraph().getStartNode()->t;
            }
        }
    }

    if (hasCamera)
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
        game.renderer.setViewportCamera(vehicleIndex, cameraFrom, cameraTarget, 10.f, 200.f);
    }

    // destroy vehicle if off track or out of bounds
    bool onGround = false;
    if (getPosition().z < -8.f)
    {
        hitPoints = 0.f;
    }
    else
    {
        PxRaycastBuffer hit;
        if (scene.raycastStatic(getPosition(), { 0, 0, -1 }, 2.5f, &hit))
        {
            onGround = true;
            if (hit.block.actor->userData)
            {
                if (((ActorUserData*)hit.block.actor->userData)->entityType != ActorUserData::TRACK)
                {
                    hitPoints = 0.f;
                }
            }
        }
    }

    // update wheels
    u32 numWheelsOnGround = 0;
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        auto info = wheelQueryResults[i];
        if (!info.isInAir)
        {
            ++numWheelsOnGround;

            // increase damping when offroad
            if (info.tireSurfaceMaterial == scene.offroadMaterial)
            {
                PxVehicleWheelData d = vehicle4W->mWheelsSimData.getWheelData(i);
                d.mDampingRate = vehicleData->physics.offroadDampingRate;
                vehicle4W->mWheelsSimData.setWheelData(i, d);
            }
            else
            {
                PxVehicleWheelData d = vehicle4W->mWheelsSimData.getWheelData(i);
                d.mDampingRate = vehicleData->physics.wheelDampingRate;
                vehicle4W->mWheelsSimData.setWheelData(i, d);
            }
        }
    }

    // destroy vehicle if it is flipped and unable to move
    if (onGround && numWheelsOnGround <= 1 && getRigidBody()->getLinearVelocity().magnitude() < 2.f)
    {
        flipTimer += deltaTime;
        if (flipTimer > 2.5f)
        {
            hitPoints = 0.f;
        }
    }
    else
    {
        flipTimer = 0.f;
    }

    // explode
    if (hitPoints <= 0.f)
    {
        for(auto& d : vehicleData->debrisChunks)
        {
            glm::mat4 t = getTransform();
			PxRigidDynamic* body = game.physx.physics->createRigidDynamic(PxTransform(convert(t * d.transform)));
			body->attachShape(*d.collisionShape);
			PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
			scene.getPhysicsScene()->addActor(*body);
	        body->setLinearVelocity(
	                getRigidBody()->getLinearVelocity() +
	                convert(glm::vec3(glm::normalize(rotationOf(t) * glm::vec4(translationOf(d.transform), 1.0)))
	                    * random(scene.randomSeries, 5.f, 20.f)));

            scene.createVehicleDebris(VehicleDebris{
                body,
                d.mesh,
                0.f
            });
        }
        deadTimer = 1.f;
        reset(glm::translate(glm::mat4(1.f), { 0, 0, -100 }));
    }

    // draw chassis
    for (auto& mesh : vehicleData->chassisMeshes)
    {
        game.renderer.drawMesh(mesh.renderHandle, getTransform() * mesh.transform);
    }

    // draw wheels
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        glm::mat4 m1 = getTransform();
        glm::mat4 m2 = convert(wheelQueryResults[i].localPose);
        glm::mat4 wheelTransform = m1 * m2;
        if ((i & 1) == 0)
        {
            wheelTransform = glm::rotate(wheelTransform, f32(M_PI), glm::vec3(0, 0, 1));
        }
        auto& mesh = i < 2 ? vehicleData->wheelMeshFront : vehicleData->wheelMeshRear;
        game.renderer.drawMesh(mesh.renderHandle, wheelTransform * mesh.transform);
    }

    // HUD
    if (hasCamera)
    {
        Font& font1 = game.resources.getFont("font", 32);
        Font& font2 = game.resources.getFont("font", 64);

        glm::vec2 offset = viewportLayout[viewportCount-1].offsets[vehicleIndex] * glm::vec2(game.windowWidth, game.windowHeight);
        glm::vec2 d(1.f, 1.f);
        if (offset.y > 0.f)
        {
            offset.y = game.windowHeight - font2.getHeight();
            d.y = -1;
        }

        std::string p = str(currentLap);
        font1.drawText(str("LAP").c_str(), offset + glm::vec2(20, d.y*20), glm::vec3(1.f));
        font2.drawText(p.c_str(), offset + glm::vec2(70, d.y*20), glm::vec3(1.f));
        font1.drawText(str('/', scene.getTotalLaps()).c_str(),
                offset + glm::vec2(70 + font2.stringDimensions(p.c_str()).x, d.y*20), glm::vec3(1.f));

        const char* placementSuffix[] = { "st", "nd", "rd", "th", "th", "th", "th", "th" };
        //const glm::vec3 placementColor[] = { { 1, 1, 1 }, { 0.9f, 1, 1 }, { 1.f, 1.f, 0.9f } }
        glm::vec3 col = glm::mix(glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), placement / 8.f);

        p = str(placement + 1);
        font2.drawText(p.c_str(), offset + glm::vec2(150, d.y*20), col);
        font1.drawText(str(placementSuffix[placement]).c_str(),
                offset + glm::vec2(150 + font2.stringDimensions(p.c_str()).x, d.y*20), col);
    }
}
