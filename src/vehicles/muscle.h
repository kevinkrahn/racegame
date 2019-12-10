#pragma once

#include "../vehicle_data.h"

class VMuscle : public VehicleData
{
public:
    VMuscle()
    {
        name = "Muscle Car";
        description = "It can drive";
        price = 8000;
        frontWeaponCount = 1;
        rearWeaponCount = 1;

        loadSceneData("muscle.Vehicle");
    }

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        copySceneDataToTuning(tuning);

        tuning.maxHitPoints = 100;

        tuning.specs.acceleration = 0.25f;
        tuning.specs.handling = 0.4f;
        tuning.specs.offroad = 0.23f;

        tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_REARWD;
        tuning.chassisMass = 1550;
        tuning.wheelMassFront = 15;
        tuning.wheelMassRear = 15;
        tuning.wheelDampingRate = 0.1f;
        tuning.wheelOffroadDampingRate = 28;
        //tuning.frontToeAngle = glm::radians(-0.5f); // more responsive to inputs
        tuning.frontToeAngle = glm::radians(0.f);
        //tuning.rearToeAngle = glm::radians(4.5f); // faster recovery from slide
        tuning.rearToeAngle = glm::radians(0.9f); // faster recovery from slide
        tuning.trackTireFriction = 2.4f;
        tuning.offroadTireFriction = 1.5f;

        tuning.rearTireGripPercent = 0.98f;
        tuning.constantDownforce = 0.f;
        tuning.forwardDownforce = 0.002f;
        tuning.topSpeed = 31.f;
        tuning.driftBoost = 0.f;

        tuning.maxEngineOmega = 800.f;
        tuning.peekEngineTorque = 1020.f;
        tuning.engineDampingFullThrottle = 0.3f;
        tuning.engineDampingZeroThrottleClutchEngaged = 1.5f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.6f;
        tuning.maxBrakeTorque = 6000.f;
        tuning.maxSteerAngle = glm::radians(52.f);
        tuning.clutchStrength = 5.f;
        tuning.gearSwitchTime = 0.16f;
        tuning.autoBoxSwitchTime = 1.2f;
        tuning.gearRatios = { -4.3f, 0.f, 4.f, 1.65f, 1.21f, 0.95f };
        tuning.finalGearRatio = 4.3f;

        tuning.suspensionMaxCompression = 0.12f;
        tuning.suspensionMaxDroop = 0.15f;
        tuning.suspensionSpringStrength = 32000.f;
        tuning.suspensionSpringDamperRate = 5200.f;

        tuning.camberAngleAtRest = -0.05f;
        tuning.camberAngleAtMaxDroop = 0.f;
        tuning.camberAngleAtMaxCompression = -0.1f;

        tuning.frontAntiRollbarStiffness = 8000.f;
        tuning.rearAntiRollbarStiffness = 8000.f;
        tuning.ackermannAccuracy = 0.8f;
        tuning.centerOfMass = { 0.09f, 0.f, -0.6f };
    }
};
