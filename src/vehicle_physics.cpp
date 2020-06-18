#include "vehicle_physics.h"
#include "game.h"
#include "collision_flags.h"
#include <vehicle/PxVehicleUtil.h>

const u32 QUERY_HITS_PER_WHEEL = 8;

PxF32 steerVsForwardSpeedData[2*8] =
{
    0.0f,       0.75f,
    5.0f,       0.75f,
    30.0f,      0.5f,
    120.0f,     0.25f,
    PX_MAX_F32, PX_MAX_F32,
    PX_MAX_F32, PX_MAX_F32,
    PX_MAX_F32, PX_MAX_F32,
    PX_MAX_F32, PX_MAX_F32
};
PxFixedSizeLookupTable<8> steerVsForwardSpeedTable(steerVsForwardSpeedData, 4);

PxVehicleKeySmoothingData keySmoothingData = {
    {
        6.0f,   //rise rate eANALOG_INPUT_ACCEL
        6.0f,   //rise rate eANALOG_INPUT_BRAKE
        6.0f,   //rise rate eANALOG_INPUT_HANDBRAKE
        1.6f,   //rise rate eANALOG_INPUT_STEER_LEFT
        1.6f,   //rise rate eANALOG_INPUT_STEER_RIGHT
    },
    {
        10.0f,  //fall rate eANALOG_INPUT_ACCEL
        10.0f,  //fall rate eANALOG_INPUT_BRAKE
        10.0f,  //fall rate eANALOG_INPUT_HANDBRAKE
        4.5f,   //fall rate eANALOG_INPUT_STEER_LEFT
        4.5f    //fall rate eANALOG_INPUT_STEER_RIGHT
    }
};

PxVehiclePadSmoothingData padSmoothingData = {
    {
        6.0f,   //rise rate eANALOG_INPUT_ACCEL
        6.0f,   //rise rate eANALOG_INPUT_BRAKE
        6.0f,   //rise rate eANALOG_INPUT_HANDBRAKE
        1.8f,   //rise rate eANALOG_INPUT_STEER_LEFT
        1.8f,   //rise rate eANALOG_INPUT_STEER_RIGHT
    },
    {
        10.0f,  //fall rate eANALOG_INPUT_ACCEL
        10.0f,  //fall rate eANALOG_INPUT_BRAKE
        10.0f,  //fall rate eANALOG_INPUT_HANDBRAKE
        5.0f,   //fall rate eANALOG_INPUT_STEER_LEFT
        5.0f    //fall rate eANALOG_INPUT_STEER_RIGHT
    }
};

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

static PxVehicleDrivableSurfaceToTireFrictionPairs* createFrictionPairs(
        VehicleTuning const& settings, const PxMaterial** materials)
{
    constexpr u32 NUM_SURFACE_TYPES = 2;
    PxVehicleDrivableSurfaceType surfaceTypes[NUM_SURFACE_TYPES] = { { 0 }, { 1 } };

    const u32 numTireTypes = 2;
    PxVehicleDrivableSurfaceToTireFrictionPairs* surfaceTirePairs =
        PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(numTireTypes, NUM_SURFACE_TYPES);

    surfaceTirePairs->setup(numTireTypes, NUM_SURFACE_TYPES, materials, surfaceTypes);

    f32 tireFriction[numTireTypes] = {
        1.f,
        settings.rearTireGripPercent,
    };

    f32 frictionTable[NUM_SURFACE_TYPES] = {
        settings.trackTireFriction,
        settings.offroadTireFriction,
    };

    for(u32 i = 0; i < NUM_SURFACE_TYPES; i++)
    {
        for(u32 j = 0; j < numTireTypes; j++)
        {
            surfaceTirePairs->setTypePairFriction(i, j, frictionTable[i] * tireFriction[j]);
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
    if(g_game.physx.cooking->cookConvexMesh(convexDesc, buf))
    {
        PxDefaultMemoryInputData id(buf.getData(), buf.getSize());
        convexMesh = g_game.physx.physics->createConvexMesh(id);
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

void VehiclePhysics::setup(void* userData, PxScene* scene, Mat4 const& transform,
        VehicleTuning* tune)
{
    this->tuning = tune;
    VehicleTuning& tuning = *tune;

    const PxMaterial* surfaceMaterials[] = {
        g_game.physx.materials.track,
        g_game.physx.materials.offroad
    };
    PxMaterial* vehicleMaterial = g_game.physx.materials.vehicle;

    sceneQueryData = VehicleSceneQueryData::allocate(1, NUM_WHEELS, QUERY_HITS_PER_WHEEL, 1,
            &WheelSceneQueryPreFilterNonBlocking, &WheelSceneQueryPostFilterNonBlocking, g_game.physx.allocator);
    batchQuery = VehicleSceneQueryData::setUpBatchedSceneQuery(0, *sceneQueryData, scene);
    frictionPairs = createFrictionPairs(tuning, surfaceMaterials);

    PxConvexMesh* wheelConvexMeshes[NUM_WHEELS];
    PxMaterial* wheelMaterials[NUM_WHEELS];

    PxConvexMesh* wheelMeshFront = createWheelMesh(tuning.wheelWidthFront, tuning.wheelRadiusFront);
    for(u32 i = WHEEL_FRONT_LEFT; i <= WHEEL_FRONT_RIGHT; ++i)
    {
        wheelConvexMeshes[i] = wheelMeshFront;
        wheelMaterials[i] = vehicleMaterial;
    }

    PxConvexMesh* wheelMeshRear = createWheelMesh(tuning.wheelWidthRear, tuning.wheelRadiusRear);
    for(u32 i = WHEEL_REAR_LEFT; i < NUM_WHEELS; ++i)
    {
        wheelConvexMeshes[i] = wheelMeshRear;
        wheelMaterials[i] = vehicleMaterial;
    }

    PxFilterData chassisSimFilterData(
            COLLISION_FLAG_CHASSIS,
            COLLISION_FLAG_CHASSIS |
            COLLISION_FLAG_TRACK |
            COLLISION_FLAG_OBJECT |
            COLLISION_FLAG_TERRAIN |
            COLLISION_FLAG_DEBRIS |
            COLLISION_FLAG_PICKUP, 0, 0);
    PxFilterData chassisQryFilterData(COLLISION_FLAG_CHASSIS, 0, 0, UNDRIVABLE_SURFACE);
    PxFilterData wheelSimFilterData(0, 0, 0, 0);
    PxFilterData wheelQryFilterData(0, 0, 0, UNDRIVABLE_SURFACE);

    PxRigidDynamic* actor = g_game.physx.physics->createRigidDynamic(convert(transform));

    for(PxU32 i = 0; i < NUM_WHEELS; i++)
    {
        PxConvexMeshGeometry geom(wheelConvexMeshes[i]);
        PxShape* wheelShape = PxRigidActorExt::createExclusiveShape(*actor, geom, *wheelMaterials[i]);
        wheelShape->setQueryFilterData(wheelQryFilterData);
        wheelShape->setSimulationFilterData(wheelSimFilterData);
        wheelShape->setLocalPose(PxTransform(PxIdentity));
    }

    for (auto const& cm : tuning.collisionMeshes)
    {
        PxVec3 scale(length(Vec3(cm.transform[0])),
                     length(Vec3(cm.transform[1])),
                     length(Vec3(cm.transform[2])));
        PxShape* chassisShape = PxRigidActorExt::createExclusiveShape(*actor,
                PxConvexMeshGeometry(cm.convexMesh, PxMeshScale(scale)), *vehicleMaterial);
        chassisShape->setQueryFilterData(chassisQryFilterData);
        chassisShape->setSimulationFilterData(chassisSimFilterData);
        chassisShape->setLocalPose(convert(cm.transform));
        chassisShape->setContactOffset(0.1f);
        chassisShape->setRestOffset(0.08f);
    }

    //PxVec3 centerOfMassOffset = convert(tuning.centerOfMass);
    PxVec3 centerOfMassOffset = { 0, 0, 0 };
    PxRigidBodyExt::setMassAndUpdateInertia(*actor, tuning.chassisMass,
            &centerOfMassOffset, false);
    //PxRigidBodyExt::updateMassAndInertia(*actor, tuning.chassisDensity);
    centerOfMassOffset = convert(tuning.centerOfMass);
    actor->setCMassLocalPose(PxTransform(centerOfMassOffset, PxQuat(PxIdentity)));
    actor->userData = userData;

    f32 wheelMOIFront = 0.5f * tuning.wheelMassFront * square(tuning.wheelRadiusFront);
    f32 wheelMOIRear = 0.5f * tuning.wheelMassRear * square(tuning.wheelRadiusRear);

    PxVehicleWheelData wheels[NUM_WHEELS];
    for(u32 i = PxVehicleDrive4WWheelOrder::eFRONT_LEFT; i <= PxVehicleDrive4WWheelOrder::eFRONT_RIGHT; ++i)
    {
        wheels[i].mMass   = tuning.wheelMassFront;
        wheels[i].mMOI    = wheelMOIFront;
        wheels[i].mRadius = tuning.wheelRadiusFront;
        wheels[i].mWidth  = tuning.wheelWidthFront;
        wheels[i].mMaxBrakeTorque = tuning.maxBrakeTorque;
        wheels[i].mDampingRate = tuning.wheelDampingRate;
    }
    for(u32 i = PxVehicleDrive4WWheelOrder::eREAR_LEFT; i < NUM_WHEELS; ++i)
    {
        wheels[i].mMass   = tuning.wheelMassRear;
        wheels[i].mMOI    = wheelMOIRear;
        wheels[i].mRadius = tuning.wheelRadiusRear;
        wheels[i].mWidth  = tuning.wheelWidthRear;
        wheels[i].mMaxBrakeTorque = tuning.maxBrakeTorque;
        wheels[i].mDampingRate = tuning.wheelDampingRate;
    }
    wheels[WHEEL_FRONT_LEFT].mToeAngle  =  tuning.frontToeAngle;
    wheels[WHEEL_FRONT_RIGHT].mToeAngle = -tuning.frontToeAngle;
    wheels[WHEEL_REAR_LEFT].mToeAngle  =  tuning.rearToeAngle;
    wheels[WHEEL_REAR_RIGHT].mToeAngle = -tuning.rearToeAngle;
    wheels[WHEEL_REAR_LEFT].mMaxHandBrakeTorque  = tuning.maxHandbrakeTorque;
    wheels[WHEEL_REAR_RIGHT].mMaxHandBrakeTorque = tuning.maxHandbrakeTorque;
    wheels[WHEEL_FRONT_LEFT].mMaxSteer = tuning.maxSteerAngle;
    wheels[WHEEL_FRONT_RIGHT].mMaxSteer = tuning.maxSteerAngle;

    PxVehicleTireData tires[NUM_WHEELS] = { };
    tires[WHEEL_FRONT_LEFT].mType = 0;
    tires[WHEEL_FRONT_RIGHT].mType = 0;
    tires[WHEEL_REAR_LEFT].mType = 1;
    tires[WHEEL_REAR_RIGHT].mType = 1;

    for(PxU32 i = 0; i < NUM_WHEELS; i++)
    {
        // TODO: investigate other tire parameters
    }

    PxVec3 wheelCenterOffsets[NUM_WHEELS];
    wheelCenterOffsets[WHEEL_FRONT_LEFT]  = convert(tuning.wheelPositions[WHEEL_FRONT_LEFT]);
    wheelCenterOffsets[WHEEL_FRONT_RIGHT] = convert(tuning.wheelPositions[WHEEL_FRONT_RIGHT]);
    wheelCenterOffsets[WHEEL_REAR_LEFT]   = convert(tuning.wheelPositions[WHEEL_REAR_LEFT]);
    wheelCenterOffsets[WHEEL_REAR_RIGHT]  = convert(tuning.wheelPositions[WHEEL_REAR_RIGHT]);

    // set up suspension
    PxVehicleSuspensionData suspensions[NUM_WHEELS];
    PxF32 suspSprungMasses[NUM_WHEELS];
    PxVehicleComputeSprungMasses(NUM_WHEELS, wheelCenterOffsets, centerOfMassOffset,
            actor->getMass(), 2, suspSprungMasses);
    for(PxU32 i = 0; i < NUM_WHEELS; i++)
    {
        suspensions[i].mMaxCompression = tuning.suspensionMaxCompression;
        suspensions[i].mMaxDroop = tuning.suspensionMaxDroop;
        suspensions[i].mSpringStrength = tuning.suspensionSpringStrength;
        suspensions[i].mSpringDamperRate = tuning.suspensionSpringDamperRate;
        suspensions[i].mSprungMass = suspSprungMasses[i];
    }

    for(PxU32 i = 0; i < NUM_WHEELS; i+=2)
    {
        suspensions[i + 0].mCamberAtRest =  tuning.camberAngleAtRest;
        suspensions[i + 1].mCamberAtRest = -tuning.camberAngleAtRest;
        suspensions[i + 0].mCamberAtMaxDroop =  tuning.camberAngleAtMaxDroop;
        suspensions[i + 1].mCamberAtMaxDroop = -tuning.camberAngleAtMaxDroop;
        suspensions[i + 0].mCamberAtMaxCompression =  tuning.camberAngleAtMaxCompression;
        suspensions[i + 1].mCamberAtMaxCompression = -tuning.camberAngleAtMaxCompression;
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
    barFront.mStiffness = tuning.frontAntiRollbarStiffness;
    wheelsSimData->addAntiRollBarData(barFront);
    PxVehicleAntiRollBarData barRear;
    barRear.mWheel0 = PxVehicleDrive4WWheelOrder::eREAR_LEFT;
    barRear.mWheel1 = PxVehicleDrive4WWheelOrder::eREAR_RIGHT;
    barRear.mStiffness = tuning.rearAntiRollbarStiffness;
    wheelsSimData->addAntiRollBarData(barRear);

    PxVehicleDriveSimData4W driveSimData;
    PxVehicleDifferential4WData diff;
    diff.mType = tuning.differential;
    driveSimData.setDiffData(diff);

    PxVehicleEngineData engine;
    engine.mPeakTorque = tuning.peekEngineTorque;
    engine.mMaxOmega = tuning.maxEngineOmega;
    engine.mDampingRateFullThrottle = tuning.engineDampingFullThrottle;
    engine.mDampingRateZeroThrottleClutchEngaged = tuning.engineDampingZeroThrottleClutchEngaged;
    engine.mDampingRateZeroThrottleClutchDisengaged = tuning.engineDampingZeroThrottleClutchDisengaged;

#if 0
    if (!driver->isPlayer)
    {
        engine.mPeakTorque += 1400.f;
        engine.mMaxOmega += 300.f;
    }
#endif

    driveSimData.setEngineData(engine);

    PxVehicleGearsData gears;
    gears.mNbRatios = tuning.gearRatios.size();
    for (u32 i=0; i<tuning.gearRatios.size(); ++i)
    {
        gears.mRatios[i] = tuning.gearRatios[i];
    }
    gears.mFinalRatio = tuning.finalGearRatio;
    gears.mSwitchTime = tuning.gearSwitchTime;
    driveSimData.setGearsData(gears);

    PxVehicleClutchData clutch;
    clutch.mStrength = tuning.clutchStrength;
    driveSimData.setClutchData(clutch);

    PxVehicleAutoBoxData autobox;
    autobox.setDownRatios(PxVehicleGearsData::eFIRST,   0.4f);
    autobox.setDownRatios(PxVehicleGearsData::eSECOND,  0.4f);
    autobox.setDownRatios(PxVehicleGearsData::eTHIRD,   0.4f);
    autobox.setDownRatios(PxVehicleGearsData::eFOURTH,  0.4f);
    autobox.setDownRatios(PxVehicleGearsData::eFIFTH,   0.5f);
    autobox.setDownRatios(PxVehicleGearsData::eSIXTH,   0.6f);
    autobox.setDownRatios(PxVehicleGearsData::eSEVENTH, 0.6f);
    autobox.setUpRatios(PxVehicleGearsData::eFIRST,   0.90f);
    autobox.setUpRatios(PxVehicleGearsData::eSECOND,  0.88f);
    autobox.setUpRatios(PxVehicleGearsData::eTHIRD,   0.88f);
    autobox.setUpRatios(PxVehicleGearsData::eFOURTH,  0.88f);
    autobox.setUpRatios(PxVehicleGearsData::eFIFTH,   0.88f);
    autobox.setUpRatios(PxVehicleGearsData::eSIXTH,   0.88f);
    autobox.setUpRatios(PxVehicleGearsData::eSEVENTH, 0.88f);
    autobox.setLatency(tuning.autoBoxSwitchTime);
    driveSimData.setAutoBoxData(autobox);

    // ackermann steer accuracy
    PxVehicleAckermannGeometryData ackermann;
    ackermann.mAccuracy = tuning.ackermannAccuracy;
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
    vehicle4W->setup(g_game.physx.physics, actor, *wheelsSimData, driveSimData, NUM_WHEELS - 4);
    wheelsSimData->free();

    actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    scene->addActor(*actor);

    vehicle4W->setToRestState();
    vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    vehicle4W->mDriveDynData.setUseAutoGears(true);
    //vehicle4W->mDriveDynData.setAutoBoxSwitchTime(tuning.autoBoxSwitchTime);
}

VehiclePhysics::~VehiclePhysics()
{
    vehicle4W->getRigidDynamicActor()->release();
    vehicle4W->free();
    sceneQueryData->free(g_game.physx.allocator);
    // TODO: find out why this causes an exception
    //batchQuery->release();
    frictionPairs->release();
}

void VehiclePhysics::reset(Mat4 const& transform)
{
    vehicle4W->setToRestState();
    vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    getRigidBody()->setGlobalPose(convert(transform));
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        wheelInfo[i].oilCoverage = 0.f;
    }
}

void VehiclePhysics::update(PxScene* scene, f32 timestep, bool digital, f32 accel, f32 brake, f32 steer,
            bool handbrake, bool canGo, bool onlyBrake)
{
    engineThrottle = 0.f;
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
            f32 forwardSpeed = getForwardSpeed();
            engineThrottle = accel;
            if (forwardSpeed < 0.f)
            {
                engineThrottle = max(accel, brake);
            }

            if (accel > 0.f)
            {
                if (vehicle4W->mDriveDynData.mCurrentGear == PxVehicleGearsData::eREVERSE
                        || forwardSpeed < 7.f)
                {
                    //vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
                    vehicle4W->mDriveDynData.setTargetGear(PxVehicleGearsData::eFIRST);
                }
                if (digital)
                {
                    inputs.setDigitalAccel(true);
                }
                else
                {
                    inputs.setAnalogAccel(accel);
                }
            }
            if (brake > 0.f)
            {
                if (forwardSpeed < 1.5f && accel < 0.001f)
                {
                    //vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eREVERSE);
                    vehicle4W->mDriveDynData.setTargetGear(PxVehicleGearsData::eREVERSE);
                    if (digital)
                    {
                        inputs.setDigitalAccel(true);
                    }
                    else
                    {
                        inputs.setAnalogAccel(brake);
                    }
                }
                else
                {
                    if (digital)
                    {
                        inputs.setDigitalBrake(true);
                    }
                    else
                    {
                        inputs.setAnalogBrake(brake);
                    }
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
            // TODO: find better way to limit top speed
            f32 speedDiff = forwardSpeed - tuning->topSpeed;
            if (speedDiff > 1.f)
            {
                vehicle4W->mDriveDynData.setAnalogInput(
                        PxVehicleDrive4WControl::eANALOG_INPUT_ACCEL, accel * 0.42f);
            }
            else if (speedDiff > 0.f)
            {
                vehicle4W->mDriveDynData.setAnalogInput(
                        PxVehicleDrive4WControl::eANALOG_INPUT_ACCEL, accel * 0.49f);
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

    if (!isInAir)
    {
        PxVec3 down = getRigidBody()->getGlobalPose().q.getBasisVector2() * -1.f;
        f32 downforce =
            tuning->forwardDownforce * absolute(getForwardSpeed()) +
            tuning->constantDownforce * getRigidBody()->getLinearVelocity().magnitude();
        getRigidBody()->addForce(down * downforce, PxForceMode::eACCELERATION);

        f32 maxSlip = 0.f;
        for (u32 i=0; i<NUM_WHEELS; ++i)
        {
            auto info = wheelQueryResults[i];
            if (!info.isInAir)
            {
                f32 lateralSlip = absolute(info.lateralSlip) - 0.3f;
                maxSlip = max(maxSlip, lateralSlip);
            }
        }
        f32 driftBoost = min(maxSlip, 1.f) * accel * tuning->driftBoost * 20.f;
        PxVec3 boostDir = getRigidBody()->getLinearVelocity().getNormalized();
        getRigidBody()->addForce(boostDir * driftBoost, PxForceMode::eACCELERATION);
    }

    updateWheelInfo(timestep);

    checkGroundSpots(scene, timestep);
}

void VehiclePhysics::updateWheelInfo(f32 deltaTime)
{
    Mat4 transform = getTransform();

    // update wheels
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        auto info = wheelQueryResults[i];
        Vec3 wheelPosition = Vec3(transform * Vec4(convert(info.localPose.p), 1.f));
        f32 wheelRadius = i < 2 ? tuning->wheelRadiusFront : tuning->wheelRadiusRear;

        wheelInfo[i].transform = Mat4(info.localPose);
        wheelInfo[i].position = wheelPosition;
        wheelInfo[i].contactNormal = convert(info.tireContactNormal);
        wheelInfo[i].contactPosition = wheelPosition - wheelInfo[i].contactNormal * wheelRadius;
        wheelInfo[i].rotationSpeed = vehicle4W->mWheelsDynData.getWheelRotationSpeed(i);
        wheelInfo[i].lateralSlip = info.lateralSlip;
        wheelInfo[i].longitudinalSlip = info.longitudinalSlip;
        wheelInfo[i].oilCoverage = max(wheelInfo[i].oilCoverage - deltaTime, 0.f);
        wheelInfo[i].dustAmount = 0.f;
        wheelInfo[i].isTouchingTrack = false;
        wheelInfo[i].isOffroad = false;
        wheelInfo[i].isTouchingGlue = false;
        wheelInfo[i].isInAir = info.isInAir;

        if (!info.isInAir)
        {
            auto filterData = info.tireContactShape->getSimulationFilterData();

            // TODO: Is there some reason why the collision filter needs to be checked?
            if ((filterData.word0 & COLLISION_FLAG_TRACK)
                    || info.tireSurfaceMaterial == g_game.physx.materials.track)
            {
                wheelInfo[i].isTouchingTrack = true;
            }

            if (info.tireSurfaceMaterial == g_game.physx.materials.offroad)
            {
                wheelInfo[i].isOffroad = true;
                wheelInfo[i].dustAmount = 1.f;
            }

            PxVehicleWheelData d = vehicle4W->mWheelsSimData.getWheelData(i);

            // increase damping when offroad
            if (wheelInfo[i].isOffroad)
            {
                d.mDampingRate = tuning->wheelOffroadDampingRate;
            }
            else
            {
                d.mDampingRate = tuning->wheelDampingRate;

                // cover wheels with oil if driving over oil
                for (auto& d : groundSpots)
                {
                    if (d.groundType == GroundSpot::OIL)
                    {
                        f32 dist = distanceSquared(d.p, wheelPosition);
                        if (dist < square(d.radius))
                        {
                            wheelInfo[i].oilCoverage = 2.f;
                        }
                    }
                    else if (d.groundType == GroundSpot::GLUE)
                    {
                        f32 dist = distanceSquared(d.p, wheelPosition);
                        if (dist < square(d.radius))
                        {
                            wheelInfo[i].isTouchingGlue = true;
                            PxVec3 vel = getRigidBody()->getLinearVelocity();
                            f32 speed = vel.magnitude();
                            f32 originalSpeed = speed;
                            speed = max(speed - deltaTime * (50.f - distance(d.p, wheelPosition) * 2.f),
                                    min(originalSpeed, 8.f));
                            getRigidBody()->setLinearVelocity(vel.getNormalized() * speed);
                        }
                    }
                }

                // decrease traction if wheel is covered with oil
                if (wheelInfo[i].oilCoverage > 0.f)
                {
                    f32 amount = clamp(wheelInfo[i].oilCoverage, 0.f, 1.f);
                    f32 oilFriction = lerp(tuning->trackTireFriction, 0.95f, amount);
                    frictionPairs->setTypePairFriction(0, 0, oilFriction);
                    frictionPairs->setTypePairFriction(0, 1,
                            oilFriction * tuning->rearTireGripPercent);
                }
                else
                {
                    frictionPairs->setTypePairFriction(0, 0, tuning->trackTireFriction);
                    frictionPairs->setTypePairFriction(0, 1,
                            tuning->trackTireFriction * tuning->rearTireGripPercent);
                }
            }

            vehicle4W->mWheelsSimData.setWheelData(i, d);
        }

        if (!wheelInfo[i].isOffroad)
        {
            wheelInfo[i].dustAmount = 0.f;
            for (auto& d : groundSpots)
            {
                if (d.groundType == GroundSpot::DUST)
                {
                    f32 dist = distance(d.p, wheelPosition);
                    f32 amount = clamp(1.f - dist / d.radius, 0.f, 1.f);
                    if (amount > wheelInfo[i].dustAmount)
                    {
                        wheelInfo[i].dustAmount = amount;
                    }
                }
            }
        }
    }
}

f32 VehiclePhysics::getAverageWheelRotationSpeed() const
{
    f32 rotationSpeed = 0.f;
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        rotationSpeed += absolute(vehicle4W->mWheelsDynData.getWheelRotationSpeed(i));
    }
    rotationSpeed /= NUM_WHEELS;
    return rotationSpeed;
}

void VehiclePhysics::checkGroundSpots(PxScene* physicsScene, f32 deltaTime)
{
    groundSpots.clear();

    PxOverlapHit hitBuffer[8];
    PxOverlapBuffer hit(hitBuffer, ARRAY_SIZE(hitBuffer));
    PxQueryFilterData filter;
    filter.flags = PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(COLLISION_FLAG_DUST | COLLISION_FLAG_OIL | COLLISION_FLAG_GLUE, 0, 0, 0);
    f32 radius = 1.5f;
    if (physicsScene->overlap(PxSphereGeometry(radius),
            PxTransform(convert(getPosition()), PxIdentity), hit, filter))
    {
        for (u32 i=0; i<hit.getNbTouches(); ++i)
        {
            PxActor* actor = hit.getTouch(i).actor;
            ActorUserData* userData = (ActorUserData*)actor->userData;
            u32 groundType = GroundSpot::DUST;
            if ((hit.getTouch(i).shape->getQueryFilterData().word0 & COLLISION_FLAG_OIL)
                    == COLLISION_FLAG_OIL)
            {
                groundType = GroundSpot::OIL;
            }
            else if ((hit.getTouch(i).shape->getQueryFilterData().word0 & COLLISION_FLAG_GLUE)
                    == COLLISION_FLAG_GLUE)
            {
                groundType = GroundSpot::GLUE;
            }
            assert(userData);

            bool ignore = false;
            for (auto& igs : ignoredGroundSpots)
            {
                if (igs.e == userData->placeableEntity)
                {
                    ignore = true;
                    break;
                }
            }
            if (ignore)
            {
                continue;
            }

            groundSpots.push_back({
                groundType,
                userData->placeableEntity->position,
                max(
                        absolute(userData->placeableEntity->scale.x),
                        max(absolute(userData->placeableEntity->scale.y),
                            absolute(userData->placeableEntity->scale.z))) * 0.48f });
        }
    }

    for (auto it = ignoredGroundSpots.begin(); it != ignoredGroundSpots.end();)
    {
        it->t -= deltaTime;
        if (it->t <= 0.f)
        {
            ignoredGroundSpots.erase(it);
            continue;
        }
        ++it;
    }
}

