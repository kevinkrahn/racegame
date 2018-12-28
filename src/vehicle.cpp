#include "vehicle.h"
#include "game.h"
#include <vehicle/PxVehicleUtil.h>

const u32 WHEEL_FRONT_LEFT  = PxVehicleDrive4WWheelOrder::eFRONT_LEFT;
const u32 WHEEL_FRONT_RIGHT = PxVehicleDrive4WWheelOrder::eFRONT_RIGHT;
const u32 WHEEL_REAR_LEFT   = PxVehicleDrive4WWheelOrder::eREAR_LEFT;
const u32 WHEEL_REAR_RIGHT  = PxVehicleDrive4WWheelOrder::eREAR_RIGHT;

const u32 NUM_WHEELS = 4;
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

    PxVehicleDrivableSurfaceToTireFrictionPairs* surfaceTirePairs =
        PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(MAX_NUM_TIRE_TYPES, MAX_NUM_SURFACE_TYPES);

    surfaceTirePairs->setup(MAX_NUM_TIRE_TYPES, MAX_NUM_SURFACE_TYPES, materials, surfaceTypes);

    for(u32 i = 0; i < MAX_NUM_SURFACE_TYPES; i++)
    {
        for(u32 j = 0; j < MAX_NUM_TIRE_TYPES; j++)
        {
            surfaceTirePairs->setTypePairFriction(i, j, frictionTable[i]);
        }
    }

    return surfaceTirePairs;
}

static void configureUserData(PxVehicleWheels* vehicle, ActorUserData* actorUserData, ShapeUserData* shapeUserDatas)
{
    vehicle->getRigidDynamicActor()->userData = actorUserData;
    actorUserData->vehicle = vehicle;

    PxShape* shapes[NUM_WHEELS + 1];
    vehicle->getRigidDynamicActor()->getShapes(shapes, NUM_WHEELS + 1);
    for(PxU32 i = 0; i < vehicle->mWheelsSimData.getNbWheels(); i++)
    {
        const PxI32 shapeId = vehicle->mWheelsSimData.getWheelShapeMapping(i);
        shapes[shapeId]->userData = &shapeUserDatas[i];
        shapeUserDatas[i].isWheel = true;
        shapeUserDatas[i].wheelId = i;
    }
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

static PxConvexMesh* createChassisMesh(glm::vec3 dims)
{
    const PxF32 x = dims.x * 0.5f;
    const PxF32 y = dims.y * 0.5f;
    const PxF32 z = dims.z * 0.5f;
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

Vehicle::Vehicle(PxScene* scene, glm::mat4 const& transform,
        PhysicsVehicleSettings& settings, PxMaterial* vehicleMaterial, const PxMaterial** surfaceMaterials)
{
    sceneQueryData = VehicleSceneQueryData::allocate(1, NUM_WHEELS, QUERY_HITS_PER_WHEEL, 1,
            &WheelSceneQueryPreFilterNonBlocking, &WheelSceneQueryPostFilterNonBlocking, game.physx.allocator);
    batchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *sceneQueryData, scene);
    frictionPairs = createFrictionPairs(settings, surfaceMaterials);

    if (glm::length2(settings.chassisMOI) == 0.f)
    {
        glm::vec3 dims = settings.chassisDimensions;
        settings.chassisMOI = glm::vec3(
            (dims.y*dims.y + dims.z*dims.z) * settings.chassisMass / 12.0f,
            (dims.x*dims.x + dims.z*dims.z) * settings.chassisMass / 12.0f,
            (dims.x*dims.x + dims.y*dims.y) * settings.chassisMass / 12.0f );
    }

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

    PxVehicleChassisData rigidBodyData;
    rigidBodyData.mMOI = convert(settings.chassisMOI);
    rigidBodyData.mMass = settings.chassisMass;
    rigidBodyData.mCMOffset = convert(settings.chassisCMOffset);

    PxFilterData chassisSimFilterData(COLLISION_FLAG_CHASSIS, COLLISION_FLAG_CHASSIS, 0, 0);
    PxFilterData wheelSimFilterData(0, 0, 0, 0);

    PxRigidDynamic* actor = game.physx.physics->createRigidDynamic(PxTransform(PxIdentity));
    PxFilterData wheelQryFilterData(0, 0, 0, UNDRIVABLE_SURFACE);
    PxFilterData chassisQryFilterData(0, 0, 0, UNDRIVABLE_SURFACE);

    for(PxU32 i = 0; i < NUM_WHEELS; i++)
    {
        PxConvexMeshGeometry geom(wheelConvexMeshes[i]);
        PxShape* wheelShape = PxRigidActorExt::createExclusiveShape(*actor, geom, *wheelMaterials[i]);
        wheelShape->setQueryFilterData(wheelQryFilterData);
        wheelShape->setSimulationFilterData(wheelSimFilterData);
        wheelShape->setLocalPose(PxTransform(PxIdentity));
    }

    PxConvexMesh* chassisConvexMesh = createChassisMesh(settings.chassisDimensions);
    PxConvexMesh* chassisConvexMeshes[1] = { chassisConvexMesh };
    PxMaterial* chassisMaterials[1] = { vehicleMaterial };
    for(PxU32 i = 0; i < ARRAY_SIZE(chassisConvexMeshes); i++)
    {
        PxShape* chassisShape = PxRigidActorExt::createExclusiveShape(*actor, PxConvexMeshGeometry(chassisConvexMeshes[i]), *chassisMaterials[i]);
        chassisShape->setQueryFilterData(chassisQryFilterData);
        chassisShape->setSimulationFilterData(chassisSimFilterData);
        chassisShape->setLocalPose(PxTransform(PxIdentity));
        chassisShape->setContactOffset(0.1f);
        chassisShape->setRestOffset(0.08f);
    }

    actor->setMass(rigidBodyData.mMass);
    actor->setMassSpaceInertiaTensor(rigidBodyData.mMOI);
    actor->setCMassLocalPose(PxTransform(rigidBodyData.mCMOffset, PxQuat(PxIdentity)));

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
    wheels[WHEEL_REAR_LEFT].mMaxHandBrakeTorque  = settings.maxHandbrakeTorque;
    wheels[WHEEL_REAR_RIGHT].mMaxHandBrakeTorque = settings.maxHandbrakeTorque;
    wheels[WHEEL_FRONT_LEFT].mMaxSteer = settings.maxSteerAngle;
    wheels[WHEEL_FRONT_RIGHT].mMaxSteer = settings.maxSteerAngle;

    PxVehicleTireData tires[NUM_WHEELS];
    for(PxU32 i = 0; i < NUM_WHEELS; i++)
    {
        tires[i].mType = NORMAL_TIRE;
    }

    PxVec3 wheelCenterOffsets[NUM_WHEELS];
    wheelCenterOffsets[WHEEL_FRONT_LEFT]  = convert(settings.wheelPositions[WHEEL_FRONT_LEFT]);
    wheelCenterOffsets[WHEEL_FRONT_RIGHT] = convert(settings.wheelPositions[WHEEL_FRONT_RIGHT]);
    wheelCenterOffsets[WHEEL_REAR_LEFT]   = convert(settings.wheelPositions[WHEEL_REAR_LEFT]);
    wheelCenterOffsets[WHEEL_REAR_RIGHT]  = convert(settings.wheelPositions[WHEEL_REAR_RIGHT]);

    // set up suspension
    PxVehicleSuspensionData suspensions[NUM_WHEELS];
    PxF32 suspSprungMasses[NUM_WHEELS];
    PxVehicleComputeSprungMasses(NUM_WHEELS, wheelCenterOffsets, convert(settings.chassisCMOffset),
            settings.chassisMass, 2, suspSprungMasses);
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
        wheelCentreCMOffsets[i] = wheelCenterOffsets[i] - convert(settings.chassisCMOffset);
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

    // configure the userdata
    configureUserData(vehicle4W, &actorUserData, wheelShapeUserData);

    vehicle4W->getRigidDynamicActor()->setGlobalPose(convert(transform));
    vehicle4W->getRigidDynamicActor()->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    scene->addActor(*vehicle4W->getRigidDynamicActor());

    vehicle4W->setToRestState();
    vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    vehicle4W->mDriveDynData.setUseAutoGears(true);
    vehicle4W->mDriveDynData.setAutoBoxSwitchTime(settings.autoBoxSwitchTime);
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
	    { wheelQueryResults, getNumWheels() }
	};
	PxVehicleUpdates(timestep, grav, *frictionPairs, 1, vehicles, vehicleQueryResults);

    isInAir = PxVehicleIsInAir(vehicleQueryResults[0]);
}

void Vehicle::onUpdate(f32 deltaTime, u32 vehicleIndex)
{
}
