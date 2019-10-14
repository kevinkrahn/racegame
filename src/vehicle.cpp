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

#include <vehicle/PxVehicleUtil.h>
#include <iomanip>

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
        PhysicsVehicleSettings const& settings, const PxMaterial** materials)
{
    constexpr u32 NUM_SURFACE_TYPES = 2;
    f32 frictionTable[NUM_SURFACE_TYPES] = {
        settings.trackTireFriction,
        settings.offroadTireFriction,
    };

    PxVehicleDrivableSurfaceType surfaceTypes[NUM_SURFACE_TYPES] = { { 0 }, { 1 } };

    const u32 numTireTypes = 2;
    PxVehicleDrivableSurfaceToTireFrictionPairs* surfaceTirePairs =
        PxVehicleDrivableSurfaceToTireFrictionPairs::allocate(numTireTypes, NUM_SURFACE_TYPES);

    surfaceTirePairs->setup(numTireTypes, NUM_SURFACE_TYPES, materials, surfaceTypes);

    f32 tireFriction[numTireTypes] = {
        1.f,
        settings.rearTireGripPercent,
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

void Vehicle::setupPhysics(PxScene* scene, PhysicsVehicleSettings const& settings, PxMaterial* vehicleMaterial,
	    const PxMaterial** surfaceMaterials, glm::mat4 const& transform)
{
    sceneQueryData = VehicleSceneQueryData::allocate(1, NUM_WHEELS, QUERY_HITS_PER_WHEEL, 1,
            &WheelSceneQueryPreFilterNonBlocking, &WheelSceneQueryPostFilterNonBlocking, g_game.physx.allocator);
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

    PxRigidDynamic* actor = g_game.physx.physics->createRigidDynamic(convert(transform));

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
    autobox.setLatency(settings.autoBoxSwitchTime);
    driveSimData.setAutoBoxData(autobox);

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
    vehicle4W->setup(g_game.physx.physics, actor, *wheelsSimData, driveSimData, NUM_WHEELS - 4);
    wheelsSimData->free();

    actor->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, true);
    scene->addActor(*actor);

    vehicle4W->setToRestState();
    vehicle4W->mDriveDynData.forceGearChange(PxVehicleGearsData::eFIRST);
    vehicle4W->mDriveDynData.setUseAutoGears(true);
    //vehicle4W->mDriveDynData.setAutoBoxSwitchTime(settings.autoBoxSwitchTime);
}

Vehicle::Vehicle(Scene* scene, glm::mat4 const& transform, glm::vec3 const& startOffset,
	    Driver* driver, PxMaterial* vehicleMaterial, const PxMaterial** surfaceMaterials,
	    u32 vehicleIndex, i32 cameraIndex)
{
    this->cameraTarget = translationOf(transform);
    this->cameraFrom = cameraTarget;
    this->startTransform = transform;
    this->targetOffset = startOffset;
    this->startOffset = startOffset;
    this->lastDamagedBy = vehicleIndex;
    this->vehicleIndex = vehicleIndex;
    this->offsetChangeInterval = random(scene->randomSeries, 5.f, 15.f);
    this->followPathIndex = scene->getTrackGraph().getPaths().size() > 0 ?
        irandom(scene->randomSeries, 0, (u32)scene->getTrackGraph().getPaths().size()) : 0;
    this->driver = driver;
    this->scene = scene;
    this->lastValidPosition = translationOf(transform);
    this->maxHitPoints = driver->vehicleTuning.maxHitPoints;
    this->hitPoints = this->maxHitPoints;
    this->cameraIndex = cameraIndex;

    engineSound = g_audio.playSound3D(g_resources.getSound("engine2"),
            SoundType::VEHICLE, translationOf(transform), true);
    tireSound = g_audio.playSound3D(g_resources.getSound("tires"),
            SoundType::VEHICLE, translationOf(transform), true, 1.f, 0.f);

    setupPhysics(scene->getPhysicsScene(), driver->vehicleTuning.physics, vehicleMaterial, surfaceMaterials, transform);
    actorUserData.entityType = ActorUserData::VEHICLE;
    actorUserData.vehicle = this;
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
    auto& primaryWeapon = g_weapons[driver->vehicleConfig.primaryWeaponIndex];
    primaryWeaponAmmo = primaryWeapon->getAmmoCountForUpgradeLevel(
            driver->vehicleConfig.primaryWeaponUpgradeLevel);

    auto& specialWeapon = g_weapons[driver->vehicleConfig.specialWeaponIndex];
    specialWeaponAmmo = specialWeapon->getAmmoCountForUpgradeLevel(
            driver->vehicleConfig.specialWeaponUpgradeLevel);
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
            f32 forwardSpeed = getForwardSpeed();

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
            }
            if (brake > 0.f)
            {
                if (vehicle4W->computeForwardSpeed() < 1.5f && accel < 0.001f)
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
            f32 topSpeed = driver->vehicleTuning.physics.topSpeed;
            if (forwardSpeed > topSpeed)
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
            driver->vehicleTuning.physics.forwardDownforce * glm::abs(getForwardSpeed()) +
            driver->vehicleTuning.physics.constantDownforce * getRigidBody()->getLinearVelocity().magnitude();
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
            * driver->vehicleTuning.physics.driftBoost * 20.f;
        PxVec3 boostDir = getRigidBody()->getLinearVelocity().getNormalized();
        getRigidBody()->addForce(boostDir * driftBoost, PxForceMode::eACCELERATION);
    }
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

void Vehicle::drawWeaponAmmo(Renderer* renderer, glm::vec2 pos, u32 weaponIndex, u32 ammo)
{
    f32 iconSize = glm::floor(g_game.windowHeight * 0.05f);
    Texture* iconbg = g_resources.getTexture("iconbg");
    renderer->push2D(QuadRenderable(iconbg, pos + glm::vec2(iconSize * 0.5f, 0.f),
                iconSize, iconSize, glm::vec3(0.35f)));
    renderer->push2D(QuadRenderable(iconbg, pos, iconSize, iconSize));
    const char* weaponIcon = g_weapons[weaponIndex]->icon;
    renderer->push2D(QuadRenderable(g_resources.getTexture(weaponIcon),
                pos, iconSize, iconSize));
    u32 ammoUnitCount = g_weapons[weaponIndex]->ammoUnitCount;
    u32 maxAmmo = g_weapons[weaponIndex]
        ->getAmmoCountForUpgradeLevel(driver->vehicleConfig.primaryWeaponUpgradeLevel);
    u32 ammoTickCount = maxAmmo / ammoUnitCount;
    f32 ammoTickMargin = iconSize * 0.025f;
    f32 ammoTickHeight = (f32)(iconSize - iconSize * 0.2f) / (f32)ammoTickCount;
    Texture* ammoTickTex = g_resources.getTexture("ammotick");
    for (u32 i=0; i<ammo; ++i)
    {
        renderer->push2D(QuadRenderable(ammoTickTex,
                    pos + glm::vec2(iconSize + ammoTickMargin * 2.f,
                        ammoTickHeight * i + (iconSize * 0.1f) + ammoTickMargin * 0.5f),
                    iconSize * 0.32f, ammoTickHeight - ammoTickMargin));
    }
}

void Vehicle::drawHUD(Renderer* renderer, f32 deltaTime)
{
    if (cameraIndex >= 0)
    {
        Font& font1 = g_resources.getFont("font", (u32)(g_game.windowHeight * 0.04f));
        Font& font2 = g_resources.getFont("font", (u32)(g_game.windowHeight * 0.08f));
        Font& font3 = g_resources.getFont("font", (u32)(g_game.windowHeight * 0.05f));

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
        f32 o200 = (f32)g_game.windowHeight * 0.20f;

        char* p = tstr(glm::min(currentLap, scene->getTotalLaps()));
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
        drawWeaponAmmo(renderer, offset + glm::vec2(weaponIconX, d.y * g_game.windowHeight * 0.018f),
                driver->vehicleConfig.primaryWeaponIndex, primaryWeaponAmmo);
        drawWeaponAmmo(renderer, offset +
                glm::vec2(weaponIconX + g_game.windowHeight * 0.1f, d.y * g_game.windowHeight * 0.018f),
                driver->vehicleConfig.specialWeaponIndex, specialWeaponAmmo);

        // healthbar
        Texture* white = g_resources.getTexture("white");
        const f32 healthPercentage = glm::clamp(hitPoints / maxHitPoints, 0.f, 1.f);
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
            renderer->push2D(TextRenderable(&font3, it->str,
                layout.offsets[cameraIndex] * dim + layout.scale * dim * 0.5f -
                    glm::vec2(0, layout.scale.y * dim.y * 0.3) + glm::vec2(0, count * dim.y * 0.04),
                { 1, 1, 1 }, 1.f, 1.f, HorizontalAlign::CENTER, VerticalAlign::CENTER));
            ++count;
            it->timeLeft -= deltaTime;
            if (it->timeLeft <= 0.f)
            {
                notifications.erase(it);
                continue;
            }
            ++it;
        }

        // vehicle debug info
        if (scene->isDebugOverlayEnabled)
        {
            Font* f = &g_resources.getFont("font", 20);
            const char* gearNames[] = { "REVERSE", "NEUTRAL", "1", "2", "3", "4", "5", "6", "7", "8" };
            char* debugText = tstr(
                "Engine RPM: ", getEngineRPM(),
                "\nSpeed: ", getForwardSpeed(),// * 3.6f,
                "\nGear: ", gearNames[vehicle4W->mDriveDynData.mCurrentGear],
                "\nProgress: ", graphResult.currentLapDistance,
                "\nLow Mark: ", graphResult.lapDistanceLowMark);
            renderer->push2D(QuadRenderable(white, { 220 + 10, g_game.windowHeight - 10 },
                        { 220 + 220, g_game.windowHeight - (30 + f->stringDimensions(debugText).y) },
                        {}, {}, { 0, 0, 0 }, 0.6f));
            renderer->push2D(TextRenderable(f, debugText,
                { 220 + 20, g_game.windowHeight - 20 }, glm::vec3(1), 1.f, 1.f, HorizontalAlign::LEFT, VerticalAlign::BOTTOM));
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
        rw->push(OverlayRenderable(g_resources.getMesh("world.Arrow"),
                cameraIndex, transform, driver->vehicleColor));
    }

    glm::mat4 wheelTransforms[NUM_WHEELS];
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        wheelTransforms[i] = convert(wheelQueryResults[i].localPose);
    }
    g_vehicles[driver->vehicleIndex]->render(rw, transform, wheelTransforms, driver);
    g_vehicles[driver->vehicleIndex]->renderDebris(rw, vehicleDebris, driver);
}

void Vehicle::updateCamera(RenderWorld* rw, f32 deltaTime)
{
    glm::vec3 pos = lastValidPosition;
#if 0
    cameraTarget = pos + glm::vec3(0, 0, 2.f);
    cameraFrom = smoothMove(cameraFrom,
            cameraTarget - getForwardVector() * 10.f + glm::vec3(0, 0, 3.f), 8.f, deltaTime);
    renderer->setViewportCamera(cameraIndex, cameraFrom, cameraTarget, 4.f, 200.f, 60.f);
#else
    cameraTarget = smoothMove(cameraTarget,
            pos + glm::vec3(glm::normalize(glm::vec2(getForwardVector())), 0.f) * getForwardSpeed() * 0.3f,
            5.f, deltaTime);
    f32 camDistance = 80.f;
    cameraTarget += screenShakeOffset * (screenShakeTimer * 0.5f);
    cameraFrom = cameraTarget + glm::normalize(glm::vec3(1.f, 1.f, 1.25f)) * camDistance;
    rw->setViewportCamera(cameraIndex, cameraFrom, cameraTarget, 25.f, 200.f);
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
        if (engineSound) g_audio.setSoundVolume(engineSound, 0.f);
        if (tireSound) g_audio.setSoundVolume(tireSound, 0.f);
        if (deadTimer <= 0.f)
        {
            if (engineSound) g_audio.setSoundVolume(engineSound, 1.f);
            backupTimer = 0.f;
            deadTimer = 0.f;
            hitPoints = maxHitPoints;

            const TrackGraph::Node* node = graphResult.lastNode;
            glm::vec2 dir(node->direction);
            glm::vec3 pos = node->position -
                glm::vec3(targetOffset.x * dir + targetOffset.y * glm::vec2(-dir.y, dir.x), 0);

            reset(glm::translate(glm::mat4(1.f), pos + glm::vec3(0, 0, 5)) *
                  glm::rotate(glm::mat4(1.f), node->angle, glm::vec3(0, 0, 1)));
        }
        if (cameraIndex >= 0)
        {
            updateCamera(rw, deltaTime);
        }

        return;
    }

    glm::vec3 currentPosition = getPosition();
    glm::mat4 transform = getTransform();
    bool canGo = true;
    if (!finishedRace)
    {
        if (isPlayerControlled)
        {
            if (g_input.isKeyPressed(KEY_U) && driver->useKeyboard)
            {
                scene->createExplosion(currentPosition, previousVelocity, 10.f);
            }

            f32 accel = 0.f;
            f32 brake = 0.f;
            f32 steer = 0.f;
            bool digital = false;
            bool shoot = false;
            bool shootSpecial = false;
            if (driver->useKeyboard)
            {
                digital = true;
                accel = g_input.isKeyDown(KEY_UP);
                brake = g_input.isKeyDown(KEY_DOWN);
                steer = (f32)g_input.isKeyDown(KEY_LEFT) - (f32)g_input.isKeyDown(KEY_RIGHT);
                shoot = g_input.isKeyPressed(KEY_C);
                shootSpecial = g_input.isKeyPressed(KEY_V);
            }
            if (!driver->useKeyboard || (scene->numHumanDrivers() == 1))
            {
                Controller* controller = g_input.getController(driver->controllerID);
                if (controller)
                {
                    accel = nonZeroOrDefault(controller->getAxis(AXIS_TRIGGER_RIGHT), accel);
                    brake = nonZeroOrDefault(controller->getAxis(AXIS_TRIGGER_LEFT), brake);
                    steer = nonZeroOrDefault(-controller->getAxis(AXIS_LEFT_X), steer);
                    shoot = shoot || controller->isButtonPressed(BUTTON_RIGHT_SHOULDER);
                    shootSpecial = shootSpecial || controller->isButtonPressed(BUTTON_LEFT_SHOULDER);
                }
            }

            updatePhysics(scene->getPhysicsScene(), deltaTime, digital,
                    accel, brake, steer, false, true, false);

            if (g_input.isKeyPressed(KEY_F))
            {
                getRigidBody()->addForce(PxVec3(0, 0, 10), PxForceMode::eVELOCITY_CHANGE);
                getRigidBody()->addTorque(
                        getRigidBody()->getGlobalPose().q.rotate(PxVec3(5, 0, 0)),
                        PxForceMode::eVELOCITY_CHANGE);
            }

            if (shoot)
            {
                firePrimaryWeapon();
            }

            if (shootSpecial)
            {
                fireSpecialWeapon();
            }
        }
        else if (scene->getTrackGraph().getPaths().size() > 0)
        {
            auto const& paths = scene->getTrackGraph().getPaths();

            i32 previousIndex = targetPointIndex - 1;
            if (previousIndex < 0)
	    {
                previousIndex = (i32)paths[followPathIndex].size() - 1;
	    }

            glm::vec3 nextP = paths[followPathIndex][targetPointIndex];
            glm::vec3 previousP = paths[followPathIndex][previousIndex];
            glm::vec2 dir = glm::normalize(glm::vec2(nextP) - glm::vec2(previousP));

            glm::vec3 targetP = nextP -
                glm::vec3(targetOffset.x * dir + targetOffset.y * glm::vec2(-dir.y, dir.x), 0);

            glm::vec2 dirToTargetP = glm::normalize(glm::vec2(currentPosition - targetP));
            f32 steerAngle = glm::dot(glm::vec2(getRightVector()), dirToTargetP);

            f32 forwardTestDist = 14.f;
            f32 sideTestDist = 9.f;
            f32 testAngle = 0.65f;
            glm::vec3 testDir1(glm::rotate(glm::mat4(1.f), testAngle, { 0, 0, 1 }) * glm::vec4(getForwardVector(), 1.0));
            glm::vec3 testDir2(glm::rotate(glm::mat4(1.f), -testAngle, { 0, 0, 1 }) * glm::vec4(getForwardVector(), 1.0));
            //renderer->drawLine(currentPosition, currentPosition + testDir1 * sideTestDist);
            //renderer->drawLine(currentPosition, currentPosition + testDir2 * sideTestDist);

            //f32 accel = 0.85f;
            f32 accel = 1.f;
            f32 brake = 0.f;
            bool isSomethingBlockingMe = isBlocking(driver->vehicleTuning.collisionWidth / 2 + 0.05f,
                    getForwardVector(), forwardTestDist);
            if (isSomethingBlockingMe && glm::dot(glm::vec2(getForwardVector()), -dirToTargetP) > 0.8f)
            {
                const f32 avoidSteerAmount = 0.5f;
                bool left = isBlocking(0.5f, testDir1, sideTestDist);
                bool right = isBlocking(0.5f, testDir2, sideTestDist);
                if (!left && !right)
                {
                    glm::vec3 d = glm::normalize(targetP - currentPosition);
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
                    steerAngle = glm::dot(glm::vec2(getRightVector()), glm::normalize(glm::vec2(currentPosition - targetP)));
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
                if (std::isnan(steerAngle))
                {
                    steerAngle = 0.f;
                }
                updatePhysics(scene->getPhysicsScene(), deltaTime, false, accel, brake,
                        -steerAngle, false, canGo, false);
            }

            if (glm::length2(nextP - currentPosition) < square(30.f))
            {
                ++targetPointIndex;
                if (targetPointIndex >= paths[followPathIndex].size())
                {
                    targetPointIndex = 0;
                }
            }
        }
        else
        {
            updatePhysics(scene->getPhysicsScene(), deltaTime, false, 0.f, 0.f, -0.f, false, false, false);
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
    lastValidPosition = currentPosition;

    const f32 maxSkippableDistance = 250.f;
    if (canGo)
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
            if (!finishedRace && currentLap >= scene->getTotalLaps())
            {
                finishedRace = true;
                scene->vehicleFinish(vehicleIndex);
            }
            ++currentLap;
            graphResult.lapDistanceLowMark = scene->getTrackGraph().getStartNode()->t;
            graphResult.currentLapDistance = scene->getTrackGraph().getStartNode()->t;
            resetAmmo();
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
        f32 gearRatio = glm::abs(driver->vehicleTuning.physics.gearRatios[
            vehicle4W->mDriveDynData.mCurrentGear]);
        engineRPM = smoothMove(engineRPM, rotationSpeed * gearRatio, 1.9f, deltaTime);

        //g_audio.setSoundPitch(engineSound, 0.8f + getEngineRPM() * 0.0007f);
        g_audio.setSoundPitch(engineSound, 1.f + engineRPM * 0.04f);
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
        filter.data = PxFilterData(COLLISION_FLAG_GROUND | COLLISION_FLAG_TRACK, 0, 0, 0);
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
        PxRaycastBuffer hit;
        if (scene->raycastStatic(currentPosition, { 0, 0, -1 }, 3.0f, &hit))
        {
            onGround = true;
            PxMaterial* hitMaterial = hit.block.shape->getMaterialFromInternalFaceIndex(hit.block.faceIndex);
            if (hitMaterial != scene->trackMaterial && hitMaterial != scene->offroadMaterial)
            {
                applyDamage(100.f, vehicleIndex);
            }
        }
    }

    // update wheels
    smokeTimer = glm::max(0.f, smokeTimer - deltaTime);
    const f32 smokeInterval = 0.015f;
    bool smoked = false;
    u32 numWheelsOnGround = 0;
    bool anyWheelOnRoad = false;
    f32 maxSlip = 0.f;
    for (u32 i=0; i<NUM_WHEELS; ++i)
    {
        auto info = wheelQueryResults[i];
        bool isWheelOffroad = false;
        if (!info.isInAir)
        {
            ++numWheelsOnGround;

            PxVehicleWheelData d = vehicle4W->mWheelsSimData.getWheelData(i);

            // increase damping when offroad
            if (info.tireSurfaceMaterial == scene->offroadMaterial)
            {
                d.mDampingRate = driver->vehicleTuning.physics.wheelOffroadDampingRate;
                isWheelOffroad = true;
            }
            else
            {
                d.mDampingRate = driver->vehicleTuning.physics.wheelDampingRate;
                anyWheelOnRoad = true;
            }

            vehicle4W->mWheelsSimData.setWheelData(i, d);
        }

        f32 lateralSlip = glm::abs(info.lateralSlip) - 0.4f;
        f32 longitudinalSlip = glm::abs(info.longitudinalSlip) - 0.6f;
        f32 slip = glm::max(lateralSlip, longitudinalSlip);
        bool wasWheelSlipping = isWheelSlipping[i];
        isWheelSlipping[i] = slip > 0.f && !info.isInAir;
        maxSlip = glm::max(maxSlip, slip);

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
                scene->smoke.spawn(
                    wheelPosition - glm::vec3(0, 0, 0.2f),
                    (vel + glm::vec3(0, 0, 1)) * 0.8f,
                    glm::min(1.f, slip * 0.4f));
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
                scene->smoke.spawn(
                    wheelPosition - glm::vec3(0, 0, 0.2f),
                    (vel + glm::vec3(0, 0, 1)) * 0.8f,
                    glm::clamp(glm::max(slip, glm::abs(wheelRotationSpeed * 0.022f)), 0.f, 1.f),
                    glm::vec4(0.58f, 0.50f, 0.22f, 1.f));
                smoked = true;
            }
        }

        // add tire marks
        if (isWheelSlipping[i] && !isWheelOffroad)
        {
            f32 wheelRadius = i < 2 ? driver->vehicleTuning.physics.wheelRadiusFront
                : driver->vehicleTuning.physics.wheelRadiusRear;
            f32 wheelWidth = i < 2 ? driver->vehicleTuning.physics.wheelWidthFront
                : driver->vehicleTuning.physics.wheelWidthRear;
            glm::vec3 tn = convert(info.tireContactNormal);
            PxTransform contactPose = info.localPose;
            glm::vec3 markPosition = tn * -wheelRadius
                + translationOf(transform * convert(info.localPose));
            //glm::vec4 color = isWheelOffroad ?  glm::vec4(0.45f, 0.39f, 0.12f, 1.f) : glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
            tireMarkRibbons[i].addPoint(markPosition, tn, wheelWidth / 2,
                    glm::vec4(1.f, 1.f, 1.f, glm::clamp(slip * 3.f, 0.f, 1.f)));
        }
        else if (wasWheelSlipping)
        {
            tireMarkRibbons[i].capWithLastPoint();
        }
    }

    g_audio.setSoundVolume(tireSound,
            anyWheelOnRoad ? glm::min(1.f, glm::min(maxSlip * 1.2f,
                    getRigidBody()->getLinearVelocity().magnitude() * 0.1f)) : 0.f);

    if (smoked) smokeTimer = smokeInterval;

    // destroy vehicle if it is flipped and unable to move
    if (onGround && numWheelsOnGround <= 1
            && getRigidBody()->getLinearVelocity().magnitude() < 5.f)
    {
        flipTimer += deltaTime;
        if (flipTimer > 1.8f)
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
    f32 damagePercent = hitPoints / maxHitPoints;
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

void Vehicle::blowUp()
{
    glm::mat4 transform = getTransform();
    for (auto& d : g_vehicles[driver->vehicleIndex]->debrisChunks)
    {
		PxRigidDynamic* body = g_game.physx.physics->createRigidDynamic(
			    PxTransform(convert(transform * d.transform)));
		body->attachShape(*d.collisionShape);
		PxRigidBodyExt::updateMassAndInertia(*body, 10.0f);
		scene->getPhysicsScene()->addActor(*body);
	    body->setLinearVelocity(
	            convert(previousVelocity) +
	            convert(glm::vec3(glm::normalize(rotationOf(transform)
	                        * glm::vec4(translationOf(d.transform), 1.0)))
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
    deadTimer = 1.f;
    engineRPM = 0.f;
    scene->createExplosion(translationOf(transform), previousVelocity, 10.f);
    scene->attackCredit(lastDamagedBy, vehicleIndex);
    reset(glm::translate(glm::mat4(1.f), { 0, 0, 1000 }));
}

void Vehicle::firePrimaryWeapon()
{
    if (primaryWeaponAmmo == 0)
    {
        // TODO: play no-no sound
        return;
    }
    auto& primaryWeapon = g_weapons[driver->vehicleConfig.primaryWeaponIndex];
    primaryWeaponAmmo -= primaryWeapon->fire(scene, this);
}

void Vehicle::fireSpecialWeapon()
{
    if (specialWeaponAmmo == 0)
    {
        // TODO: play no-no sound
        return;
    }
    auto& specialWeapon = g_weapons[driver->vehicleConfig.specialWeaponIndex];
    specialWeaponAmmo -= specialWeapon->fire(scene, this);
}
