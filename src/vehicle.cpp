#include "vehicle.h"
#include "game.h"
#include "decal.h"
#include "scene.h"
#include "renderer.h"
#include "2d.h"
#include "input.h"
#include "mesh_renderables.h"
#include "billboard.h"
#include "weapon.h"
#include "imgui.h"

#include <vehicle/PxVehicleUtil.h>
#include <iomanip>

const f32 camDistance = 80.f;

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

void Vehicle::setupPhysics(PxScene* scene, PxMaterial* vehicleMaterial,
        const PxMaterial** surfaceMaterials, glm::mat4 const& transform)
{
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

    PxFilterData chassisSimFilterData(COLLISION_FLAG_CHASSIS,
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

    //PxVec3 centerOfMassOffset = convert(tuning.centerOfMass);
    PxVec3 centerOfMassOffset = { 0, 0, 0 };
    PxRigidBodyExt::setMassAndUpdateInertia(*actor, tuning.chassisMass,
            &centerOfMassOffset, false);
    //PxRigidBodyExt::updateMassAndInertia(*actor, tuning.chassisDensity);
    centerOfMassOffset = convert(tuning.centerOfMass);
    actor->setCMassLocalPose(PxTransform(centerOfMassOffset, PxQuat(PxIdentity)));
    actor->userData = &actorUserData;

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

Vehicle::Vehicle(Scene* scene, glm::mat4 const& transform, glm::vec3 const& startOffset,
        Driver* driver, VehicleTuning&& tuning, PxMaterial* vehicleMaterial, const PxMaterial** surfaceMaterials,
        u32 vehicleIndex, i32 cameraIndex)
{
    this->cameraTarget = translationOf(transform) - camDistance * 0.5f;
    this->cameraFrom = cameraTarget;
    this->startTransform = transform;
    this->targetOffset = startOffset;
    this->startOffset = startOffset;
    this->lastDamagedBy = vehicleIndex;
    this->vehicleIndex = vehicleIndex;
    this->offsetChangeInterval = random(scene->randomSeries, 6.f, 15.f);
	if (driver->aiIndex != -1)
	{
		auto& ai = g_ais[driver->aiIndex];
		f32 skill = clamp(1.f - ai.drivingSkill
				+ random(scene->randomSeries, -0.2f, 0.2f), 0.f, 0.99999f);
		this->preferredFollowPathIndex = (u32)(glm::max(0, (i32)scene->getPaths().size()) * skill);
		this->currentFollowPathIndex = this->preferredFollowPathIndex;
		// TODO: change preferredFollowPathIndex per lap?
	}
    this->driver = driver;
    this->scene = scene;
    this->lastValidPosition = translationOf(transform);
    this->cameraIndex = cameraIndex;
    this->tuning = std::move(tuning);
    this->hitPoints = this->tuning.maxHitPoints;
    this->previousTargetPosition = translationOf(transform);

    engineSound = g_audio.playSound3D(&g_res.sounds->engine2,
            SoundType::VEHICLE, translationOf(transform), true);
    tireSound = g_audio.playSound3D(&g_res.sounds->tires,
            SoundType::VEHICLE, translationOf(transform), true, 1.f, 0.f);

    setupPhysics(scene->getPhysicsScene(), vehicleMaterial, surfaceMaterials, transform);
    actorUserData.entityType = ActorUserData::VEHICLE;
    actorUserData.vehicle = this;

    // create weapons
    for (u32 i=0; i<ARRAY_SIZE(VehicleConfiguration::frontWeaponIndices); ++i)
    {
        VehicleConfiguration* config = driver->getVehicleConfig();
        if (config->frontWeaponIndices[i] != -1)
        {
            frontWeapons.push_back(g_weapons[config->frontWeaponIndices[i]].create());
            frontWeapons.back()->upgradeLevel = config->frontWeaponUpgradeLevel[i];
            frontWeapons.back()->mountTransform = driver->getVehicleData()->weaponMounts[i];
        }
    }
    for (u32 i=0; i<ARRAY_SIZE(VehicleConfiguration::rearWeaponIndices); ++i)
    {
        VehicleConfiguration* config = driver->getVehicleConfig();
        if (config->rearWeaponIndices[i] != -1)
        {
            rearWeapons.push_back(g_weapons[config->rearWeaponIndices[i]].create());
            rearWeapons.back()->upgradeLevel = config->rearWeaponUpgradeLevel[i];
        }
    }
    if (driver->getVehicleConfig()->specialAbilityIndex != -1)
    {
        specialAbility = g_weapons[driver->getVehicleConfig()->specialAbilityIndex].create();
    }
}

Vehicle::~Vehicle()
{
    if (engineSound) g_audio.stopSound(engineSound);
    if (tireSound) g_audio.stopSound(tireSound);
    for (auto& d : vehicleDebris)
    {
        d.rigidBody->release();
    }
    vehicle4W->getRigidDynamicActor()->release();
    vehicle4W->free();
    sceneQueryData->free(g_game.physx.allocator);
    // TODO: find out why this causes an exception
    //batchQuery->release();
    frictionPairs->release();
}

void Vehicle::resetAmmo()
{
    for(auto& w : frontWeapons)
    {
        w->refillAmmo();
    }
    for(auto& w : rearWeapons)
    {
        w->refillAmmo();
    }
    if (specialAbility)
    {
        specialAbility->refillAmmo();
    }
}

void Vehicle::reset(glm::mat4 const& transform)
{
    airTime = 0.f;
    airBonusGracePeriod = 0.f;
    savedAirTime = 0.f;
    vehicle4W->setToRestState();
    vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    getRigidBody()->setGlobalPose(convert(transform));
    for (auto& w : frontWeapons)
    {
        w->reset();
    }
    for (auto& w : rearWeapons)
    {
        w->reset();
    }
    if (specialAbility)
    {
        specialAbility->reset();
    }
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        wheelOilCoverage[i] = 0.f;
    }
}

void Vehicle::updatePhysics(PxScene* scene, f32 timestep, bool digital,
        f32 accel, f32 brake, f32 steer, bool handbrake, bool canGo, bool onlyBrake)
{
    engineThrottle = 0.f;
    isBraking = handbrake || brake > 0.2f || onlyBrake;
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
                engineThrottle = glm::max(accel, brake);
            }

            if (accel > 0.f)
            {
                if (vehicle4W->mDriveDynData.mCurrentGear == PxVehicleGearsData::eREVERSE
                        || forwardSpeed < 7.f)
                {
                    //vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
                    vehicle4W->mDriveDynData.setTargetGear(PxVehicleGearsData::eFIRST);
                }
                if (digital) inputs.setDigitalAccel(true);
                else inputs.setAnalogAccel(accel);
                //inputs.setAnalogAccel(driver->isPlayer ? 0.9f : 1.f);
            }
            if (brake > 0.f)
            {
                if (forwardSpeed < 1.5f && accel < 0.001f)
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
            f32 speedDiff = forwardSpeed - tuning.topSpeed;
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
            tuning.forwardDownforce * glm::abs(getForwardSpeed()) +
            tuning.constantDownforce * getRigidBody()->getLinearVelocity().magnitude();
        getRigidBody()->addForce(down * downforce, PxForceMode::eACCELERATION);

        f32 maxSlip = 0.f;
        for (u32 i=0; i<NUM_WHEELS; ++i)
        {
            auto info = wheelQueryResults[i];
            if (!info.isInAir)
            {
                f32 lateralSlip = glm::abs(info.lateralSlip) - 0.3f;
                maxSlip = glm::max(maxSlip, lateralSlip);
            }
        }
        f32 driftBoost = glm::min(maxSlip, 1.f) * accel
            * tuning.driftBoost * 20.f;
        PxVec3 boostDir = getRigidBody()->getLinearVelocity().getNormalized();
        getRigidBody()->addForce(boostDir * driftBoost, PxForceMode::eACCELERATION);
    }
}

void Vehicle::drawWeaponAmmo(Renderer* renderer, glm::vec2 pos, Weapon* weapon,
        bool showAmmo, bool selected)
{
    f32 iconSize = glm::floor(g_game.windowHeight * 0.05f);
    if (showAmmo)
    {
        Texture* iconbg = &g_res.textures->weapon_iconbg;
        renderer->push2D(QuadRenderable(iconbg, pos, iconSize * 1.5f, iconSize));
    }
    else
    {
        Texture* iconbg = &g_res.textures->iconbg;
        renderer->push2D(QuadRenderable(iconbg, pos, iconSize, iconSize));
    }

    renderer->push2D(QuadRenderable(weapon->info.icon,
                pos, iconSize, iconSize));
    if (selected)
    {
        Texture* selectedTex = &g_res.textures->weapon_iconbg_selected;
        renderer->push2D(QuadRenderable(selectedTex, pos, iconSize * 1.5f, iconSize));
    }
    if (showAmmo)
    {
        u32 ammoTickCountMax = weapon->getMaxAmmo() / weapon->ammoUnitCount;
        u32 ammoTickCount = (weapon->ammo + weapon->ammoUnitCount - 1) / weapon->ammoUnitCount;
        f32 ammoTickMargin = iconSize * 0.025f;
        f32 ammoTickHeight = (f32)(iconSize - iconSize * 0.2f) / (f32)ammoTickCountMax;
        Texture* ammoTickTex = &g_res.textures->ammotick;
        for (u32 i=0; i<ammoTickCount; ++i)
        {
            renderer->push2D(QuadRenderable(ammoTickTex,
                        pos + glm::vec2(iconSize + ammoTickMargin * 2.f,
                            ammoTickHeight * i + (iconSize * 0.1f) + ammoTickMargin * 0.5f),
                        iconSize * 0.32f, ammoTickHeight - ammoTickMargin));
        }
    }
}

void Vehicle::drawHUD(Renderer* renderer, f32 deltaTime)
{
    if (cameraIndex >= 0)
    {
        Font& font1 = g_res.getFont("font_bold", (u32)(g_game.windowHeight * 0.04f));
        Font& font2 = g_res.getFont("font_bold", (u32)(g_game.windowHeight * 0.08f));
        Font& font3 = g_res.getFont("font_bold", (u32)(g_game.windowHeight * 0.05f));

        ViewportLayout const& layout =
            viewportLayout[renderer->getRenderWorld()->getViewportCount() - 1];
        glm::vec2 dim(g_game.windowWidth, g_game.windowHeight);
        glm::vec2 offset = layout.offsets[cameraIndex] * dim;
        glm::vec2 d(1.f, 1.f);
        if (offset.y > 0.f)
        {
            offset.y = g_game.windowHeight - font2.getHeight();
            d.y = -1;
        }
        glm::vec2 vdim = dim * layout.scale;
        glm::vec2 voffset = layout.offsets[cameraIndex] * dim;
        if (voffset.y > 0.f)
        {
            voffset.y = (f32)g_game.windowHeight;
        }

        f32 o20 = (f32)g_game.windowHeight * 0.02f;
        f32 o25 = (f32)g_game.windowHeight * 0.03f;
        f32 o200 = (f32)g_game.windowHeight * 0.21f;

        char* p = tstr(glm::min((u32)currentLap, scene->getTotalLaps()));
        const char* lapStr = "LAP";
        f32 lapWidth = font1.stringDimensions(lapStr).x;
        renderer->push2D(TextRenderable(&font1, lapStr,
                    offset + glm::vec2(o20, d.y*o20), glm::vec3(1.f)));
        renderer->push2D(TextRenderable(&font2, p,
                    offset + glm::vec2(o25 + lapWidth, d.y*o20), glm::vec3(1.f)));
        renderer->push2D(TextRenderable(&font1, tstr('/', scene->getTotalLaps()),
                    offset + glm::vec2(o25 + lapWidth + font2.stringDimensions(p).x, d.y*o20), glm::vec3(1.f)));

        const char* placementSuffix = "th";
        if (placement == 0) placementSuffix = "st";
        else if (placement == 1) placementSuffix = "nd";
        else if (placement == 2) placementSuffix = "rd";

        glm::vec3 col = glm::mix(glm::vec3(0, 1, 0), glm::vec3(1, 0, 0), placement / 8.f);

        p = tstr(placement + 1);
        renderer->push2D(TextRenderable(&font2, p, offset + glm::vec2(o200, d.y*o20), col));
        renderer->push2D(TextRenderable(&font1, placementSuffix,
                    offset + glm::vec2(o200 + font2.stringDimensions(p).x, d.y*o20), col));

        // weapons
        f32 weaponIconX = g_game.windowHeight * 0.35f;
        for (i32 i=0; i<(i32)frontWeapons.size(); ++i)
        {
            auto& w = frontWeapons[i];
            drawWeaponAmmo(renderer, offset +
                    glm::vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                    w.get(), true, i == currentFrontWeaponIndex && frontWeapons[i]->ammo > 0);
            weaponIconX += g_game.windowHeight * 0.1f;
        }
        for (i32 i=0; i<(i32)rearWeapons.size(); ++i)
        {
            auto& w = rearWeapons[i];
            drawWeaponAmmo(renderer, offset +
                    glm::vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                    w.get(), true, i == currentRearWeaponIndex && rearWeapons[i]->ammo > 0);
            weaponIconX += g_game.windowHeight * 0.1f;
        }
        if (specialAbility)
        {
            drawWeaponAmmo(renderer, offset +
                    glm::vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                    specialAbility.get(), false, false);
        }

        // healthbar
        Texture* white = &g_res.textures->white;
        const f32 healthPercentage = glm::clamp(hitPoints / tuning.maxHitPoints, 0.f, 1.f);
        const f32 maxHealthbarWidth = g_game.windowHeight * 0.14f;
        const f32 healthbarWidth = maxHealthbarWidth * healthPercentage;
        const f32 healthbarHeight = g_game.windowHeight * 0.009f;
        glm::vec2 pos = voffset + glm::vec2(vdim.x - o20, d.y*o20);
        renderer->push2D(QuadRenderable(white, pos + glm::vec2(-maxHealthbarWidth, healthbarHeight*d.y),
                pos, {}, {}, glm::vec3(0)));
        renderer->push2D(QuadRenderable(white, pos + glm::vec2(-healthbarWidth, healthbarHeight*d.y),
                pos, {}, {}, glm::mix(glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), healthPercentage)));

        // display notifications
        u32 count = 0;
        for (auto it = notifications.begin(); it != notifications.end();)
        {
            glm::vec2 p = glm::floor(layout.offsets[cameraIndex] * dim + layout.scale * dim * 0.5f -
                    glm::vec2(0, layout.scale.y * dim.y * 0.3f) + glm::vec2(0, count * dim.y * 0.04f));
            renderer->push2D(TextRenderable(&font3, it->text, p,
                it->color, (glm::sin(it->time * 6.f) + 1.f) * 0.3f + 0.5f,
                glm::max(1.7f - it->time * 3.f, 1.f),
                HorizontalAlign::CENTER, VerticalAlign::CENTER));
            ++count;
            it->time += deltaTime;
            if (it->time > it->secondsToKeepOnScreen)
            {
                notifications.erase(it);
                continue;
            }
            ++it;
        }
    }
}

void Vehicle::onRender(RenderWorld* rw, f32 deltaTime)
{
    glm::mat4 transform = getTransform();

    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        scene->ribbons.addChunk(&tireMarkRibbons[i]);
    }

    if (cameraIndex >= 0 && isHidden)
    {
        rw->push(OverlayRenderable(g_res.getMesh("world.Arrow"),
                cameraIndex, transform, g_vehicleColors[driver->getVehicleConfig()->colorIndex]));
    }

    glm::mat4 wheelTransforms[NUM_WHEELS];
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        wheelTransforms[i] = convert(wheelQueryResults[i].localPose);
    }
    driver->getVehicleData()->render(rw, transform,
            wheelTransforms, *driver->getVehicleConfig(), this, isBraking);
    driver->getVehicleData()->renderDebris(rw, vehicleDebris,
            *driver->getVehicleConfig());

    // visualize path finding
#if 0
    if (motionPath.size() > 0)
    {
        Mesh* sphere = g_res.getMesh("world.Sphere");
        //Font* font = &g_res.getFont("font", 18);
        for (auto& p : motionPath)
        {
            rw->push(LitRenderable(sphere,
                        glm::translate(glm::mat4(1.f), p.p + glm::vec3(0, 0, 1))
                            * glm::scale(glm::mat4(1.f), glm::vec3(0.25f)),
                        nullptr, glm::vec3(1, 1, 0)));

            /*
            glm::vec4 tp = g_game.renderer->getRenderWorld()->getCamera(0).viewProjection *
                glm::vec4(p.p + glm::vec3(0, 0, 1), 1.f);
            tp.x = (((tp.x / tp.w) + 1.f) / 2.f) * g_game.windowWidth;
            tp.y = ((-1.f * (tp.y / tp.w) + 1.f) / 2.f) * g_game.windowHeight;
            g_game.renderer->push2D(TextRenderable(font,
                tstr(std::fixed, std::setprecision(1), p.g, ", ", p.h), { tp.x, tp.y },
                glm::vec3(0.f, 0.f, 1.f), 1.f, 1.f, HorizontalAlign::CENTER));
            */
        }
    }
#endif
}

void Vehicle::updateCamera(RenderWorld* rw, f32 deltaTime)
{
    glm::vec3 pos = lastValidPosition;
#if 0
    cameraTarget = pos + glm::vec3(0, 0, 2.f);
    cameraFrom = smoothMove(cameraFrom,
            cameraTarget - getForwardVector() * 10.f + glm::vec3(0, 0, 3.f), 8.f, deltaTime);
    rw->setViewportCamera(cameraIndex, cameraFrom, cameraTarget, 4.f, 200.f, 60.f);
#else
    cameraTarget = smoothMove(cameraTarget,
            pos + glm::vec3(glm::normalize(glm::vec2(getForwardVector())), 0.f) * getForwardSpeed() * 0.3f,
            5.f, deltaTime);
    cameraTarget += screenShakeOffset * (screenShakeTimer * 0.5f);
    cameraFrom = cameraTarget + glm::normalize(glm::vec3(1.f, 1.f, 1.25f)) * camDistance;
    rw->setViewportCamera(cameraIndex, cameraFrom, cameraTarget, 23.f, 250.f);
#endif
}

void Vehicle::onUpdate(RenderWorld* rw, f32 deltaTime)
{
    bool isPlayerControlled = driver->isPlayer;

    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        tireMarkRibbons[i].update(deltaTime);
    }

    // update debris chunks
    for (auto it = vehicleDebris.begin(); it != vehicleDebris.end();)
    {
        it->life -= deltaTime;
        if (it->life <= 0.f)
        {
            scene->getPhysicsScene()->removeActor(*it->rigidBody);
            it->rigidBody->release();
            //std::swap(*it, vehicleDebris.back());
            *it = vehicleDebris.back();
            vehicleDebris.pop_back();
        }
        else
        {
            ++it;
        }
    }

    if (screenShakeTimer > 0.f)
    {
        screenShakeTimer = glm::max(screenShakeTimer - deltaTime, 0.f);
        screenShakeDirChangeTimer -= screenShakeDirChangeTimer;
        if (screenShakeDirChangeTimer <= 0.f)
        {
            screenShakeDirChangeTimer = random(scene->randomSeries, 0.01f, 0.025f);
            glm::vec3 targetOffset = glm::normalize(glm::vec3(
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f))) * random(scene->randomSeries, 0.75f, 1.5f);
            screenShakeVelocity = glm::normalize(targetOffset - screenShakeOffset) *
                random(scene->randomSeries, 20.f, 30.f);
        }
        screenShakeOffset += screenShakeVelocity * deltaTime;
    }

    if (deadTimer > 0.f)
    {
        deadTimer -= deltaTime;
        if (engineSound)
        {
            g_audio.setSoundVolume(engineSound, 0.f);
        }
        if (tireSound)
        {
            g_audio.setSoundVolume(tireSound, 0.f);
        }
        if (deadTimer <= 0.f)
        {
            backupTimer = 0.f;
            deadTimer = 0.f;
            hitPoints = tuning.maxHitPoints;

            glm::vec3 pos = graphResult.position;
            const TrackGraph::Node* node = graphResult.lastNode;
            if (!graphResult.lastNode)
            {
                node = scene->getTrackGraph().getEndNode();
            }
            pos += glm::vec3(random(scene->randomSeries, -5.f, 5.f),
                    random(scene->randomSeries, -5.f, 5.f), 0.f);
            pos = scene->findValidPosition(pos, 5.f);

            reset(glm::translate(glm::mat4(1.f), pos + glm::vec3(0, 0, 6.5f)) *
                  glm::rotate(glm::mat4(1.f), node->angle, glm::vec3(0, 0, 1)));
        }
        if (cameraIndex >= 0)
        {
            updateCamera(rw, deltaTime);
        }

        return;
    }

    input = {};
    if (isPlayerControlled)
    {
        updatePlayerInput(deltaTime, rw);
    }
    else if (scene->getPaths().size() > 0)
    {
        updateAiInput(deltaTime, rw);
    }

    // update weapons
    for (i32 i=0; i<(i32)frontWeapons.size(); ++i)
    {
        auto& w = frontWeapons[i];
        w->update(scene, this, currentFrontWeaponIndex == i ? input.beginShoot : false,
                currentFrontWeaponIndex == i ? input.holdShoot : false, deltaTime);
        if (w->ammo == 0 && currentFrontWeaponIndex == i)
        {
            input.switchFrontWeapon = true;
        }
    }
    for (i32 i=0; i<(i32)rearWeapons.size(); ++i)
    {
        auto& w = rearWeapons[i];
        w->update(scene, this, currentRearWeaponIndex == i ? input.beginShootRear : false,
                currentRearWeaponIndex == i ? input.holdShootRear : false, deltaTime);
        if (w->ammo == 0 && currentRearWeaponIndex == i)
        {
            input.switchRearWeapon = true;
        }
    }

    if (input.switchFrontWeapon)
    {
        currentFrontWeaponIndex = (currentFrontWeaponIndex + 1) % frontWeapons.size();
    }
    if (input.switchRearWeapon)
    {
        currentRearWeaponIndex = (currentRearWeaponIndex + 1) % rearWeapons.size();
    }

    if (specialAbility)
    {
        specialAbility->update(scene, this, false, false, deltaTime);
    }

    // update vehicle physics
    if (!finishedRace)
    {
        updatePhysics(scene->getPhysicsScene(), deltaTime,
                input.digital, input.accel, input.brake, input.steer, false, scene->canGo(), false);
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

    glm::vec3 currentPosition = getPosition();
    glm::mat4 transform = getTransform();
    lastValidPosition = currentPosition;

    // periodically choose random offset
    offsetChangeTimer += deltaTime;
    if (offsetChangeTimer > offsetChangeInterval)
    {
        //targetOffset.x = clamp(targetOffset.x + random(scene->randomSeries, -1.f, 1.f), -4.5f, 4.5f);
        //targetOffset.y = clamp(targetOffset.y + random(scene->randomSeries, -1.f, 1.f), -4.5f, 4.5f);
        targetOffset.x = random(scene->randomSeries, -2.5f, 2.5f);
        targetOffset.y = random(scene->randomSeries, -2.5f, 2.5f);
        offsetChangeTimer = 0.f;
        offsetChangeInterval = random(scene->randomSeries, 6.f, 16.f);
    }

    const f32 maxSkippableDistance = 250.f;
    if (scene->canGo())
    {
        scene->getTrackGraph().findLapDistance(currentPosition, graphResult, maxSkippableDistance);
    }

    // check if crossed finish line
    if (!finishedRace && graphResult.lapDistanceLowMark < maxSkippableDistance)
    {
        glm::vec3 finishLinePosition = translationOf(scene->getStart());
        glm::vec3 dir = glm::normalize(currentPosition - finishLinePosition);
        if (glm::dot(xAxisOf(scene->getStart()), dir) > 0.f
                && glm::length2(currentPosition - finishLinePosition) < square(40.f))
        {
            if (!finishedRace && (u32)currentLap >= scene->getTotalLaps())
            {
                finishedRace = true;
                scene->vehicleFinish(vehicleIndex);
            }
            if (currentLap > 0)
            {
                g_audio.playSound3D(&g_res.sounds->lap, SoundType::GAME_SFX, currentPosition);
            }
            ++currentLap;
            if ((u32)currentLap == scene->getTotalLaps())
            {
                addNotification("LAST LAP!", 2.f, glm::vec3(1, 0, 0));
            }
            graphResult.lapDistanceLowMark = scene->getTrackGraph().getStartNode()->t;
            graphResult.currentLapDistance = scene->getTrackGraph().getStartNode()->t;
            resetAmmo();
        }
    }

    // check for lapping bonus
    if (!finishedRace)
    {
        for (auto& v : scene->getVehicles())
        {
            if (v.get() == this)
            {
                continue;
            }

            f32 lapDistance = scene->getTrackGraph().getStartNode()->t;
            f32 myDistance = glm::max((i32)currentLap - 1, 0) * lapDistance
                + (lapDistance - graphResult.lapDistanceLowMark);
            f32 otherDistance = glm::max((i32)v->currentLap - 1, 0) * lapDistance
                + (lapDistance - v->graphResult.lapDistanceLowMark);

            if (myDistance - otherDistance > lapDistance + lappingOffset[v->vehicleIndex])
            {
                lappingOffset[v->vehicleIndex] += lapDistance;
                addBonus("LAPPING BONUS!", LAPPING_BONUS_AMOUNT, glm::vec3(1, 1, 0));
                // TODO: play sound
            }
        }
    }

    if (engineSound)
    {
        f32 rotationSpeed = 0.f;
        for (u32 i=0; i<NUM_WHEELS; ++i)
        {
            rotationSpeed += glm::abs(vehicle4W->mWheelsDynData.getWheelRotationSpeed(i));
        }
        rotationSpeed /= NUM_WHEELS;
        f32 gearRatio = glm::abs(tuning.gearRatios[vehicle4W->mDriveDynData.mCurrentGear]);
        engineRPM = smoothMove(engineRPM, glm::min(rotationSpeed * gearRatio, 150.f), 1.9f, deltaTime);

        //g_audio.setSoundPitch(engineSound, 0.8f + getEngineRPM() * 0.0007f);
        g_audio.setSoundPitch(engineSound, 1.f + engineRPM * 0.04f);

        engineThrottleLevel = smoothMove(engineThrottleLevel,
                engineThrottle, 1.5f, deltaTime);
        g_audio.setSoundVolume(engineSound, clamp(engineThrottleLevel + 0.7f, 0.f, 1.f));

        g_audio.setSoundPosition(engineSound, lastValidPosition);
    }
    if (tireSound)
    {
        g_audio.setSoundPosition(tireSound, lastValidPosition);
    }

    if (cameraIndex >= 0)
    {
        updateCamera(rw, deltaTime);

        // detect if vehicle is hidden behind something
        glm::vec3 rayStart = currentPosition;
        glm::vec3 diff = cameraFrom - rayStart;
        glm::vec3 rayDir = glm::normalize(diff);
        f32 dist = glm::length(diff);
        PxRaycastBuffer hit;
        PxQueryFilterData filter;
        filter.flags |= PxQueryFlag::eANY_HIT;
        filter.flags |= PxQueryFlag::eSTATIC;
        filter.data = PxFilterData(COLLISION_FLAG_TERRAIN | COLLISION_FLAG_OBJECT | COLLISION_FLAG_TRACK, 0, 0, 0);
        PxHitFlags hitFlags(PxHitFlag::eMESH_BOTH_SIDES | PxHitFlag::eMESH_ANY);
        isHidden = true;
        for (u32 i=0; i<NUM_WHEELS; ++i)
        {
            glm::vec3 wheelPosition = transform *
                glm::vec4(convert(wheelQueryResults[i].localPose.p), 1.0);
            if (!scene->getPhysicsScene()->raycast(convert(wheelPosition),
                        convert(rayDir), dist, hit, hitFlags, filter))
            {
                isHidden = false;
                break;
            }
        }
    }

    // destroy vehicle if off track or out of bounds
    bool onGround = false;
    if (currentPosition.z < -16.f)
    {
        applyDamage(100.f, vehicleIndex);
    }
    else
    {
        bool validGround = false;
        PxRaycastBuffer hit;
        if (scene->raycastStatic(currentPosition, { 0, 0, -1 }, 3.0f, &hit))
        {
            onGround = true;
            PxMaterial* hitMaterial = hit.block.shape->getMaterialFromInternalFaceIndex(hit.block.faceIndex);
            if (hitMaterial == scene->trackMaterial || hitMaterial == scene->offroadMaterial)
            {
                validGround = true;
            }
        }
        if (!validGround)
        {
            if (scene->raycastStatic(currentPosition, { 0, 0, -1 }, 3.0f, nullptr, COLLISION_FLAG_TRACK))
            {
                onGround = true;
                validGround = true;
            }
        }
        if (onGround && !validGround)
        {
            applyDamage(100.f, vehicleIndex);
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
    checkGroundSpots();
    bool isDustyRoad = false;
    for (auto& d : groundSpots)
    {
        if (d.groundType == GroundSpot::DUST)
        {
            isDustyRoad = true;
            break;
        }
    }

    // update wheels
    smokeTimer = glm::max(0.f, smokeTimer - deltaTime);
    const f32 smokeInterval = 0.015f;
    bool smoked = false;
    u32 numWheelsOnTrack = 0;
    isOnTrack = false;
    bool isTouchingAnyGlue = false;
    f32 maxSlip = 0.f;
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        auto info = wheelQueryResults[i];
        glm::vec3 wheelPosition = transform * glm::vec4(convert(info.localPose.p), 1.f);
        bool isWheelOffroad = false;
        bool isWheelOnTrack = false;
        if (!info.isInAir)
        {
            auto filterData = info.tireContactShape->getSimulationFilterData();
            if ((filterData.word0 & COLLISION_FLAG_TRACK)
                    || info.tireSurfaceMaterial == scene->offroadMaterial)
            {
                ++numWheelsOnTrack;
            }

            PxVehicleWheelData d = vehicle4W->mWheelsSimData.getWheelData(i);

            // increase damping when offroad
            if (info.tireSurfaceMaterial == scene->offroadMaterial)
            {
                d.mDampingRate = tuning.wheelOffroadDampingRate;
                isWheelOffroad = true;
            }
            else
            {
                if (info.tireSurfaceMaterial == scene->trackMaterial)
                {
                    isWheelOnTrack = true;
                }

                d.mDampingRate = tuning.wheelDampingRate;
                isOnTrack = true;

                // cover wheels with oil if driving over oil
                for (auto& d : groundSpots)
                {
                    if (d.groundType == GroundSpot::OIL)
                    {
                        f32 dist = glm::distance2(d.p, wheelPosition);
                        if (dist < square(d.radius))
                        {
                            wheelOilCoverage[i] = 2.f;
                        }
                    }
                    else if (d.groundType == GroundSpot::GLUE)
                    {
                        f32 dist = glm::distance2(d.p, wheelPosition);
                        if (dist < square(d.radius))
                        {
                            PxVec3 vel = getRigidBody()->getLinearVelocity();
                            f32 speed = vel.magnitude();
                            f32 originalSpeed = speed;
                            speed = glm::max(speed - deltaTime * (40.f - glm::distance(d.p, wheelPosition) * 2.f),
                                    glm::min(originalSpeed, 8.f));
                            getRigidBody()->setLinearVelocity(vel.getNormalized() * speed);
                            isTouchingAnyGlue = true;

                            if (!isStuckOnGlue)
                            {
                                g_audio.playSound3D(&g_res.sounds->sticky, SoundType::GAME_SFX,
                                        getPosition(), false, 1.f, 0.95f);
                                isStuckOnGlue = true;
                            }
                        }
                    }
                }

                // decrease traction if wheel is covered with oil
                if (wheelOilCoverage[i] > 0.f)
                {
                    f32 amount = glm::clamp(wheelOilCoverage[i], 0.f, 1.f);
                    f32 oilFriction = glm::lerp(tuning.trackTireFriction, 0.95f, amount);
                    frictionPairs->setTypePairFriction(0, 0, oilFriction);
                    frictionPairs->setTypePairFriction(0, 1,
                            oilFriction * tuning.rearTireGripPercent);
                }
                else
                {
                    frictionPairs->setTypePairFriction(0, 0, tuning.trackTireFriction);
                    frictionPairs->setTypePairFriction(0, 1,
                            tuning.trackTireFriction * tuning.rearTireGripPercent);
                }
            }

            vehicle4W->mWheelsSimData.setWheelData(i, d);
        }

        f32 lateralSlip = glm::abs(info.lateralSlip) - 0.4f;
        f32 longitudinalSlip = glm::abs(info.longitudinalSlip) - 0.6f;
        f32 slip = glm::max(lateralSlip, longitudinalSlip);
        bool wasWheelSlipping = isWheelSlipping[i];
        isWheelSlipping[i] = (slip > 0.f || wheelOilCoverage[i] > 0.f) && !info.isInAir;
        maxSlip = glm::max(maxSlip, slip);
        wheelOilCoverage[i] = glm::max(wheelOilCoverage[i] - deltaTime, 0.f);

        // create smoke
        if (slip > 0.f && !info.isInAir && isWheelOnTrack)
        {
            if (smokeTimer == 0.f)
            {
                glm::vec3 vel(glm::normalize(glm::vec3(
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f))));
                scene->smoke.spawn(
                    wheelPosition - glm::vec3(0, 0, 0.2f),
                    (vel + glm::vec3(0, 0, 1)) * 0.8f,
                    glm::min(1.f, slip * 0.4f));
                smoked = true;
            }
        }

        if ((isWheelOffroad || isDustyRoad) && smokeTimer == 0.f)
        {
            f32 dustAmount = 1.f;
            if (!isWheelOffroad)
            {
                dustAmount = 0.f;
                for (auto& d : groundSpots)
                {
                    if (d.groundType == GroundSpot::DUST)
                    {
                        f32 dist = glm::distance(d.p, wheelPosition);
                        f32 amount = clamp(1.f - dist / d.radius, 0.f, 1.f);
                        if (amount > dustAmount)
                        {
                            dustAmount = amount;
                        }
                    }
                }
            }

            f32 wheelRotationSpeed = glm::abs(vehicle4W->mWheelsDynData.getWheelRotationSpeed(i));
            if (wheelRotationSpeed > 5.f || slip > 0.f)
            {
                glm::vec3 vel(glm::normalize(glm::vec3(
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f),
                    random(scene->randomSeries, -1.f, 1.f))));
                scene->smoke.spawn(
                    wheelPosition - glm::vec3(0, 0, 0.2f),
                    (vel + glm::vec3(0, 0, 1)) * 0.8f,
                    glm::clamp(
                        glm::max(slip, glm::abs(wheelRotationSpeed * 0.022f)) * dustAmount,
                        0.f, 1.f),
                    glm::vec4(0.58f, 0.50f, 0.22f, 1.f));
                smoked = true;
            }
        }

        // add tire marks
        if (isWheelSlipping[i] && isWheelOnTrack)
        {
            f32 wheelRadius = i < 2 ? tuning.wheelRadiusFront
                : tuning.wheelRadiusRear;
            f32 wheelWidth = i < 2 ? tuning.wheelWidthFront
                : tuning.wheelWidthRear;
            glm::vec3 tn = convert(info.tireContactNormal);
            PxTransform contactPose = info.localPose;
            glm::vec3 markPosition = tn * -wheelRadius
                + translationOf(transform * convert(info.localPose));
            //glm::vec4 color = isWheelOffroad ?  glm::vec4(0.45f, 0.39f, 0.12f, 1.f) : glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
            f32 alpha = glm::clamp(glm::max(slip * 3.f, wheelOilCoverage[i]), 0.f, 1.f);
            glm::vec3 color(1.f - glm::clamp(wheelOilCoverage[i], 0.f, 1.f));
            tireMarkRibbons[i].addPoint(markPosition, tn, wheelWidth / 2,
                    glm::vec4(color, alpha));
        }
        else if (wasWheelSlipping)
        {
            tireMarkRibbons[i].capWithLastPoint();
        }
    }
    if (!isTouchingAnyGlue)
    {
        isStuckOnGlue = false;
    }

    g_audio.setSoundVolume(tireSound,
            isOnTrack ? glm::min(1.f, glm::min(maxSlip * 1.2f,
                    getRigidBody()->getLinearVelocity().magnitude() * 0.1f)) : 0.f);

    if (smoked)
    {
        smokeTimer = smokeInterval;
    }

    // destroy vehicle if it is flipped and unable to move
    if (onGround && numWheelsOnTrack <= 2
            && getRigidBody()->getLinearVelocity().magnitude() < 6.f)
    {
        flipTimer += deltaTime;
        if (flipTimer > 1.5f)
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
        blowUp();
    }

    // spawn smoke when critically damaged
    smokeTimerDamage = glm::max(0.f, smokeTimerDamage - deltaTime);
    f32 damagePercent = hitPoints / tuning.maxHitPoints;
    if (smokeTimerDamage <= 0.f && damagePercent < 0.3f)
    {
        // TODO: make the effect more intense the more critical the damage (fire and sparks?)
        glm::vec3 vehicleVel = previousVelocity + getForwardVector();
        glm::vec3 vel = glm::vec3(glm::normalize(glm::vec3(
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f),
                random(scene->randomSeries, -1.f, 1.f))))
            + glm::vec3(0, 0, 1)
            + vehicleVel * 0.5f;
        scene->smoke.spawn(currentPosition + glm::vec3(0, 0, 0.5f),
            vel, 1.f - damagePercent, glm::vec4(glm::vec3(0.5f), 1.f), 0.5f);
        smokeTimerDamage = 0.015f;
    }
    if (g_input.isKeyPressed(KEY_R) && isPlayerControlled)
    {
        applyDamage(15, vehicleIndex);
    }

    // give air bonus if in air long enough
    if (!onGround)
    {
        airTime += deltaTime;
        airBonusGracePeriod = 0.f;
    }
    else
    {
        if (airTime > 2.f)
        {
            savedAirTime = airTime;
        }
        airTime = 0.f;
        airBonusGracePeriod += deltaTime;
        if (savedAirTime > 0.f && airBonusGracePeriod > 0.4f && totalAirBonuses < 10)
        {
            if (savedAirTime > 4.f)
            {
                addBonus("BIG AIR BONUS!", BIG_AIR_BONUS_AMOUNT, glm::vec3(1, 0.6f, 0.05f));
                ++totalAirBonuses;
            }
            else if (savedAirTime > 2.f)
            {
                addBonus("AIR BONUS!", AIR_BONUS_AMOUNT, glm::vec3(1, 0.7f, 0.05f));
                ++totalAirBonuses;
            }
            airBonusGracePeriod = 0.f;
            savedAirTime = 0.f;
        }
    }

    previousVelocity = convert(getRigidBody()->getLinearVelocity());
}

void Vehicle::shakeScreen(f32 intensity)
{
    if (intensity * 0.08f > screenShakeTimer)
    {
        screenShakeTimer = intensity * 0.08f;
        screenShakeOffset = glm::vec3(0.f);
    }
}

void Vehicle::applyDamage(f32 amount, u32 instigator)
{
    if (specialAbility)
    {
        amount = specialAbility->onDamage(amount, this);
    }
    if (amount > 0.f)
    {
        hitPoints -= amount;
        lastDamagedBy = instigator;
        if (instigator != vehicleIndex)
        {
            lastOpponentDamagedBy = instigator;
            lastTimeDamagedByOpponent = scene->getWorldTime();
        }
        if (smokeTimerDamage <= 0.f)
        {
            smokeTimerDamage = 0.015f;
        }
    }
}

void Vehicle::blowUp()
{
    glm::mat4 transform = getTransform();
    for (auto& d : g_vehicles[driver->vehicleIndex]->debrisChunks)
    {
        PxRigidDynamic* body = g_game.physx.physics->createRigidDynamic(
                PxTransform(convert(transform * d.transform)));
        body->attachShape(*d.collisionShape);
        PxRigidBodyExt::updateMassAndInertia(*body, 10.f);
        scene->getPhysicsScene()->addActor(*body);
        body->setLinearVelocity(
                convert(previousVelocity) +
                convert(glm::vec3(glm::normalize(rotationOf(transform)
                            * glm::vec4(translationOf(d.transform), 1.f)))
                    * random(scene->randomSeries, 3.f, 14.f) + glm::vec3(0, 0, 9.f)));
        body->setAngularVelocity(PxVec3(
                    random(scene->randomSeries, 0.f, 9.f),
                    random(scene->randomSeries, 0.f, 9.f),
                    random(scene->randomSeries, 0.f, 9.f)));

        createVehicleDebris(VehicleDebris{
            &d,
            body,
            random(scene->randomSeries, 5.f, 6.f)
        });
    }
    deadTimer = 0.8f;
    engineRPM = 0.f;
    scene->createExplosion(translationOf(transform), previousVelocity, 10.f);
    if (scene->getWorldTime() - lastTimeDamagedByOpponent < 0.5)
    {
        scene->attackCredit(lastOpponentDamagedBy, vehicleIndex);
    }
    else
    {
        scene->attackCredit(lastDamagedBy, vehicleIndex);
    }
    Sound* sounds[] = {
        &g_res.sounds->explosion4,
        &g_res.sounds->explosion5,
        &g_res.sounds->explosion6,
        &g_res.sounds->explosion7,
    };
    u32 index = irandom(scene->randomSeries, 0, ARRAY_SIZE(sounds));
    g_audio.playSound3D(sounds[index], SoundType::GAME_SFX,
            getPosition(), false, 1.f, 0.95f);
    reset(glm::translate(glm::mat4(1.f), { 0, 0, 1000 }));
    if (raceStatistics.destroyed + raceStatistics.accidents == 10)
    {
        addBonus("VICTIM BONUS", VICTIM_BONUS_AMOUNT, glm::vec3(1.f, 0.3f, 0.02f));
    }
}

void Vehicle::onTrigger(ActorUserData* userData)
{
}

void Vehicle::checkGroundSpots()
{
    groundSpots.clear();

    PxOverlapHit hitBuffer[8];
    PxOverlapBuffer hit(hitBuffer, ARRAY_SIZE(hitBuffer));
    PxQueryFilterData filter;
    filter.flags = PxQueryFlag::eSTATIC;
    filter.data = PxFilterData(COLLISION_FLAG_DUST | COLLISION_FLAG_OIL | COLLISION_FLAG_GLUE, 0, 0, 0);
    f32 radius = 1.5f;
    if (scene->getPhysicsScene()->overlap(PxSphereGeometry(radius),
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
                glm::max(
                        glm::abs(userData->placeableEntity->scale.x),
                        glm::max(glm::abs(userData->placeableEntity->scale.y),
                            glm::abs(userData->placeableEntity->scale.z))) * 0.48f });
        }
    }
}

void Vehicle::showDebugInfo()
{
    ImGui::Text("Engine RPM: %f", getEngineRPM());
    ImGui::Text("Speed: %f", getForwardSpeed());
    const char* gearNames[] = { "REVERSE", "NEUTRAL", "1", "2", "3", "4", "5", "6", "7", "8" };
    ImGui::Text("Speed: %s", gearNames[vehicle4W->mDriveDynData.mCurrentGear]);
    ImGui::Text("Lap Progress: %f", graphResult.currentLapDistance);
    ImGui::Text("Lap Low Mark: %f", graphResult.lapDistanceLowMark);
}

void Vehicle::updatePlayerInput(f32 deltaTime, RenderWorld* rw)
{
    if (driver->useKeyboard || scene->getNumHumanDrivers() == 1)
    {
        input.digital = true;
        input.accel = g_input.isKeyDown(KEY_UP);
        input.brake = g_input.isKeyDown(KEY_DOWN);
        input.steer = (f32)g_input.isKeyDown(KEY_LEFT) - (f32)g_input.isKeyDown(KEY_RIGHT);
        input.beginShoot = g_input.isKeyPressed(KEY_C);
        input.holdShoot = g_input.isKeyDown(KEY_C);
        input.beginShootRear = g_input.isKeyPressed(KEY_V);
        input.holdShootRear = g_input.isKeyDown(KEY_V);
        input.switchFrontWeapon = g_input.isKeyPressed(KEY_X);
        input.switchRearWeapon = g_input.isKeyPressed(KEY_B);
    }
    else if (!driver->controllerGuid.empty())
    {
        Controller* controller = g_input.getController(driver->controllerID);
        if (!controller)
        {
            driver->controllerID = g_input.getControllerId(driver->controllerGuid);
            controller = g_input.getController(driver->controllerID);
        }
        if (controller)
        {
            input.accel = controller->getAxis(AXIS_TRIGGER_RIGHT);
            input.brake = controller->getAxis(AXIS_TRIGGER_LEFT);
            input.steer = -controller->getAxis(AXIS_LEFT_X);
            input.beginShoot = controller->isButtonPressed(BUTTON_RIGHT_SHOULDER);
            input.holdShoot = controller->isButtonDown(BUTTON_RIGHT_SHOULDER);
            input.beginShootRear = controller->isButtonPressed(BUTTON_LEFT_SHOULDER);
            input.holdShootRear = controller->isButtonDown(BUTTON_LEFT_SHOULDER);
            input.switchFrontWeapon = controller->isButtonPressed(BUTTON_X);
            input.switchRearWeapon = controller->isButtonPressed(BUTTON_Y);
        }
    }
    if (scene->getNumHumanDrivers() == 1)
    {
        for (auto& c : g_input.getControllers())
        {
            const Controller* controller = &c.second;
            f32 val = controller->getAxis(AXIS_TRIGGER_RIGHT);
            if (glm::abs(val) > 0.f)
            {
                input.accel = val;
                input.digital = false;
            }
            val = controller->getAxis(AXIS_TRIGGER_LEFT);
            if (glm::abs(val) > 0.f)
            {
                input.brake = val;
                input.digital = false;
            }
            val = -controller->getAxis(AXIS_LEFT_X);
            if (glm::abs(val) > 0.f)
            {
                input.steer = val;
                input.digital = false;
            }
            input.beginShoot = input.beginShoot || controller->isButtonPressed(BUTTON_RIGHT_SHOULDER);
            input.holdShoot = input.holdShoot || controller->isButtonDown(BUTTON_RIGHT_SHOULDER);
            input.beginShootRear = input.beginShootRear || controller->isButtonPressed(BUTTON_LEFT_SHOULDER);
            input.holdShootRear = input.holdShootRear || controller->isButtonDown(BUTTON_LEFT_SHOULDER);
            input.switchFrontWeapon = input.switchFrontWeapon || controller->isButtonPressed(BUTTON_X);
            input.switchRearWeapon = input.switchRearWeapon || controller->isButtonPressed(BUTTON_Y);
        }
    }

    if (g_input.isKeyPressed(KEY_F))
    {
        getRigidBody()->addForce(PxVec3(0, 0, 10), PxForceMode::eVELOCITY_CHANGE);
        getRigidBody()->addTorque(
                getRigidBody()->getGlobalPose().q.rotate(PxVec3(5, 0, 0)),
                PxForceMode::eVELOCITY_CHANGE);
    }

#if 0
    // test path finding
    if (g_input.isMouseButtonPressed(MOUSE_RIGHT))
    {
        Camera const& cam = rw->getCamera(0);
        glm::vec2 mousePos = g_input.getMousePosition();
        glm::vec3 rayDir = screenToWorldRay(mousePos,
                glm::vec2(g_game.windowWidth, g_game.windowHeight), cam.view, cam.projection);
        PxRaycastBuffer hit;
        if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit))
        {
            glm::vec3 hitPoint = convert(hit.block.position);
            glm::vec3 forwardVector = getForwardVector();
            bool isSomethingBlockingMe = isBlocking(tuning.collisionWidth / 2 + 0.05f,
                    forwardVector, 20.f);
            scene->getMotionGrid().findPath(currentPosition, hitPoint, isSomethingBlockingMe,
                    forwardVector, motionPath);
        }
    }
#endif
}

void Vehicle::updateAiInput(f32 deltaTime, RenderWorld* rw)
{
    auto& ai = g_ais[driver->aiIndex];

    glm::vec3 currentPosition = getPosition();
    glm::vec3 forwardVector = getForwardVector();

    RacingLine::Point targetPathPoint =
        scene->getPaths()[currentFollowPathIndex].getPointAt(distanceAlongPath);

    // look for other paths if too far off course
    const f32 pathStepSize = 14.f;
    f32 facingBias = glm::dot(forwardVector, glm::normalize(targetPathPoint.position - currentPosition)) * 5.f;
    if (currentFollowPathIndex != preferredFollowPathIndex ||
            glm::distance(currentPosition, targetPathPoint.position) - facingBias > 26.f)
    {
        f32 minPathScore = FLT_MAX;
        for (u32 i=0; i<scene->getPaths().size(); ++i)
        {
            RacingLine::Point testPoint =
                scene->getPaths()[i].getNearestPoint(currentPosition, graphResult.currentLapDistance);
            f32 pathScore = glm::distance(testPoint.position, currentPosition);

            // prioritize the preferred path
            if (i == preferredFollowPathIndex)
            {
                pathScore -= 15.f;
            }

            // prioritize this path if the vehicle is facing the targeted point
            pathScore -= glm::dot(forwardVector, glm::normalize(testPoint.position - currentPosition))
                * 18.f;

            if (minPathScore > pathScore)
            {
                minPathScore = pathScore;
                currentFollowPathIndex = i;
                targetPathPoint = testPoint;
            }
        }
        f32 pathLength = scene->getPaths()[currentFollowPathIndex].length;
        distanceAlongPath = targetPathPoint.distanceToHere
            + (pathLength * glm::max(0, currentLap - 1)) + pathStepSize;
        targetPathPoint = scene->getPaths()[currentFollowPathIndex].getPointAt(distanceAlongPath);
    }
    else
    {
        if (glm::distance2(currentPosition, targetPathPoint.position) < square(16.f))
        {
            distanceAlongPath += pathStepSize;
        }
        else
        {
            f32 pathLength = scene->getPaths()[currentFollowPathIndex].length;
            f32 distanceToHere = scene->getPaths()[currentFollowPathIndex]
                    .getNearestPoint(currentPosition, graphResult.currentLapDistance).distanceToHere
                    + (pathLength * glm::max(0, currentLap - 1));
            if (distanceToHere > distanceAlongPath)
            {
                distanceAlongPath = distanceToHere + pathStepSize;
            }
        }

        // TODO: check if we have line of site to the path point
    }

    // TODO: Use targetSpeed of the racingLine to modulate throttle

    glm::vec2 diff = glm::vec2(previousTargetPosition) - glm::vec2(targetPathPoint.position);
    glm::vec2 dir = glm::length2(diff) > 0.f ? glm::normalize(diff) : glm::vec2(forwardVector);
    glm::vec3 targetP = targetPathPoint.position -
        glm::vec3(targetOffset.x * dir + targetOffset.y * glm::vec2(-dir.y, dir.x), 0);
    glm::vec2 dirToTargetP = glm::normalize(glm::vec2(currentPosition) - glm::vec2(targetP));
    previousTargetPosition = targetPathPoint.position;
#if 0
    rw->push(LitRenderable(g_res.getMesh("world.Sphere"),
                glm::translate(glm::mat4(1.f), targetP), nullptr, glm::vec3(1, 0, 0)));
#endif

    input.accel = 1.f;
    input.brake = 0.f;
    input.steer = clamp(glm::dot(glm::vec2(getRightVector()), dirToTargetP) * 1.2f, -1.f, 1.f);
    f32 aggression = glm::min(glm::max(((f32)scene->getWorldTime() - 3.f) * 0.3f, 0.f),
            ai.aggression);

    // obstacle avoidance
#if 1
    u32 flags = COLLISION_FLAG_DYNAMIC | COLLISION_FLAG_OIL  |
                COLLISION_FLAG_GLUE    | COLLISION_FLAG_MINE |
                COLLISION_FLAG_CHASSIS;// | COLLISION_FLAG_BOOSTER;
    PxSweepBuffer hit;
    f32 sweepLength = 13.f + ai.awareness * 8.f;
    PxRigidBody* ignoreBody = getRigidBody();
    isBlocked = false;
    isNearHazard = false;
    f32 mySpeed = getRigidBody()->getLinearVelocity().magnitude();
    if (mySpeed < 35.f && scene->sweep(tuning.collisionWidth * 0.5f + 0.05f,
            currentPosition + glm::vec3(0, 0, 0.25f), forwardVector, sweepLength,
            &hit, ignoreBody, flags))
    {
        bool shouldAvoid = true;

        // if the object is moving away then don't attempt to avoid it
        if (hit.block.actor->getType() == PxActorType::eRIGID_DYNAMIC)
        {
            f32 mySpeed = this->getForwardSpeed();
            f32 dot = ((PxRigidDynamic*)hit.block.actor)->getLinearVelocity().dot(
                    getRigidBody()->getLinearVelocity());
            if (mySpeed > 5.f && glm::abs(dot - mySpeed) < 5.f)
            {
                shouldAvoid = false;
            }
        }

        // if chasing a target, don't try to avoid opponent vehicles
        ActorUserData* userData = (ActorUserData*)hit.block.actor->userData;
        if (target && userData && userData->entityType == ActorUserData::VEHICLE)
        {
            shouldAvoid = false;
        }

        // avoid booster pads that are facing the wrong way
        // TODO: fix this (it seems to have the wrong effect; the ai drive toward the backwards boosters)
        if (userData && userData->flags & ActorUserData::BOOSTER)
        {
            if (glm::dot(-dirToTargetP,
                    glm::normalize(glm::vec2(yAxisOf(userData->placeableEntity->transform)))) > 0.5f)
            {
                shouldAvoid = false;
            }
        }

        if (shouldAvoid)
        {
            isBlocked = true;
            isNearHazard = true;
            bool foundOpening = false;
            for (u32 sweepOffsetCount = 1; sweepOffsetCount <= 3; ++sweepOffsetCount)
            {
                // TODO: prefer to swerve to the side that is closer to the angle the AI wanted to steer to anyways
                for (i32 sweepSide = -1; sweepSide < 2; sweepSide += 1)
                {
                    if (sweepSide == 0)
                    {
                        continue;
                    }
                    glm::vec3 rightVector = getRightVector();
                    f32 cw = tuning.collisionWidth * 0.25f;
                    glm::vec3 sweepFrom = currentPosition
                        + rightVector * (f32)sweepSide * (sweepOffsetCount * tuning.collisionWidth * 0.6f);
                    u32 collisionFlags = COLLISION_FLAG_DYNAMIC | COLLISION_FLAG_OIL  |
                                         COLLISION_FLAG_GLUE    | COLLISION_FLAG_MINE |
                                         COLLISION_FLAG_CHASSIS | COLLISION_FLAG_OBJECT;
                    if (!scene->sweep(cw, sweepFrom, forwardVector, sweepLength * 0.7f, nullptr,
                            ignoreBody, collisionFlags))
                    {
                        input.steer = -(0.4f + sweepOffsetCount * 0.1f) * sweepSide;
                        foundOpening = true;
                        break;
                    }
                }
                if (foundOpening)
                {
                    break;
                }
            }
        }
    }
    /*
    glm::vec4 c = isBlocked ? glm::vec4(1, 0, 0, 1) : glm::vec4(0, 1, 0, 1);
    scene->debugDraw.line(currentPosition, currentPosition + forwardVector * sweepLength, c, c);
    */
#endif

    // get behind target
#if 1
    if (!isInAir && aggression > 0.f)
    {
        if (!target && frontWeapons.size() > 0
                && frontWeapons[currentFrontWeaponIndex]->ammo > 0)
        {
            f32 maxTargetDist = aggression * 25.f + 15.f;
            f32 lowestTargetPriority = FLT_MAX;
            for (auto& v : scene->getVehicles())
            {
                if (v.get() == this)
                {
                    continue;
                }

                glm::vec2 diff = glm::vec2(v->getPosition()) - glm::vec2(currentPosition);
                glm::vec2 targetDiff = glm::normalize(-diff);
                f32 d = glm::length2(diff);
                f32 dot = glm::dot(glm::vec2(getForwardVector()), targetDiff);
                f32 targetPriority = d + dot * 4.f;
                if (dot < aggression && d < square(maxTargetDist) && targetPriority < lowestTargetPriority)
                {
                    target = v.get();
                    lowestTargetPriority = targetPriority;
                }
            }
        }

        if (target && !target->isDead())
        {
            targetTimer += deltaTime;
            if (targetTimer > (1.f - aggression) * 2.f)
            {
                glm::vec2 targetDiff = currentPosition - target->getPosition();
                // only steer toward the target if doing so would not result in veering off course
                if (glm::dot(targetDiff, dirToTargetP) > 0.8f - (aggression * 0.2f))
                {
                    f32 targetSteerAngle = glm::dot(glm::vec2(getRightVector()), targetDiff) * 0.4f;
                    input.steer = clamp(targetSteerAngle, -0.5f, 0.5f);
                }
                else
                {
                    targetTimer = 0.f;
                    target = nullptr;
                }
            }
            if (targetTimer > 3.f + aggression)
            {
                targetTimer = 0.f;
                target = nullptr;
            }
        }
        else
        {
            targetTimer = 0.f;
            target = nullptr;
        }
    }
#endif

    // fear
#if 1
    if (ai.fear > 0.f)
    {
        // TODO: shouldn't this use fear rather than aggression?
        f32 fearRayLength = aggression * 35.f + 10.f;
        if (scene->sweep(0.5f, currentPosition, -getForwardVector(),
                    fearRayLength, nullptr, getRigidBody(), COLLISION_FLAG_CHASSIS))
        {
            fearTimer += deltaTime;
            if (fearTimer > 1.f * (1.f - ai.fear) + 0.5f)
            {
                input.steer += glm::sin((f32)scene->getWorldTime() * 3.f)
                    * (ai.fear * 0.25f);
            }
            isFollowed = true;
        }
        else
        {
            fearTimer = 0.f;
            isFollowed = false;
        }
        /*
        scene->debugDraw.line(
                currentPosition,
                currentPosition-getForwardVector()*fearRayLength,
                glm::vec4(1, 0, 0, 1), glm::vec4(1, 0, 0, 1));
        */
    }
#endif

    if (scene->canGo())
    {
        if (isBackingUp)
        {
            isBackingUp = true;
            input.accel = 0.f;
            input.brake = 1.f;
            input.steer *= -1.f;
            backupTimer += deltaTime;
            if (backupTimer > 4.5f || getForwardSpeed() < -9.f)
            {
                backupTimer = 0.f;
                isBackingUp = false;
            }
            else if (backupTimer > 1.f &&
                !scene->sweep(tuning.collisionWidth * 0.3f, currentPosition, forwardVector,
                    6.f, nullptr, getRigidBody(), COLLISION_FLAG_OBJECT | COLLISION_FLAG_CHASSIS))
            {
                backupTimer = 0.f;
                isBackingUp = false;
            }
        }
        else
        {
            if (getForwardSpeed() < 2.5f &&
                scene->sweep(tuning.collisionWidth * 0.3f, currentPosition, forwardVector,
                    4.2f, nullptr, getRigidBody(), COLLISION_FLAG_OBJECT | COLLISION_FLAG_CHASSIS))
            {
                backupTimer += deltaTime;
                if (backupTimer > 0.6f)
                {
                    isBackingUp = true;
                    backupTimer = 0.f;
                }
            }
            else if (getForwardSpeed() < 2.f)
            {
                backupTimer += deltaTime;
                if (backupTimer > 2.f)
                {
                    isBackingUp = true;
                    backupTimer = 0.f;
                }
            }
            else
            {
                backupTimer = 0.f;
            }

            if (getForwardSpeed() < -0.25f)
            {
                input.steer *= -1;
            }
        }
    }

    // front weapons
    if (aggression > 0.f && frontWeapons.size() > 0
            && frontWeapons[currentFrontWeaponIndex]->ammo > 0)
    {
        f32 rayLength = aggression * 50.f + 10.f;
        /*
        scene->debugDraw.line(
                currentPosition,
                currentPosition+getForwardVector()*rayLength,
                glm::vec4(0, 1, 0, 1), glm::vec4(0, 1, 0, 1));
        */
        if (scene->sweep(0.5f, currentPosition,
                    getForwardVector(),
                    rayLength, nullptr, getRigidBody(), COLLISION_FLAG_CHASSIS))
        {
            attackTimer += deltaTime;
        }
        else
        {
            attackTimer = 0.f;
        }

        if (frontWeapons[currentFrontWeaponIndex]->fireMode == Weapon::CONTINUOUS)
        {
            if (attackTimer > 2.f * (1.f - aggression) + 0.4f)
            {
                input.holdShoot = true;
            }
        }
        else
        {
            if (attackTimer > 2.5f * (1.f - aggression) + 0.7f)
            {
                attackTimer = 0.f;
                input.beginShoot = true;
            }
        }
    }
    else
    {
        attackTimer = 0.f;
    }

    // rear weapons
    if (rearWeapons.size() > 0)
    {
        rearWeaponTimer += deltaTime;
        if (rearWeaponTimer > 0.2f)
        {
            if (rearWeapons[currentRearWeaponIndex]->shouldUse(scene, this))
            {
                input.beginShootRear = true;
            }
            rearWeaponTimer = 0.f;
        }
    }

    // TODO: make the sign correct in the first place
    input.steer *= -1.f;
}
