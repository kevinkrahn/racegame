#pragma once

#include "../vehicle_data.h"

class VMini : public VehicleData
{
public:
    VMini()
    {
        name = "Mini";
        description = "Cool car";
        price = 5000;
        loadSceneData("mini.Scene");
    }

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        copySceneDataToTuning(tuning);

        tuning.maxHitPoints = 100;

        tuning.specs.acceleration = 0.3f;
        tuning.specs.handling = 0.5f;

        tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_FRONTWD;
        tuning.chassisDensity = 92;
        tuning.wheelMassFront = 20;
        tuning.wheelMassRear = 20;
        tuning.wheelDampingRate = 0.5;
        tuning.wheelOffroadDampingRate = 30;
        tuning.frontToeAngle = 0;
        tuning.rearToeAngle = 0;
        tuning.trackTireFriction = 3.6f;
        tuning.offroadTireFriction = 1.5f;

        tuning.rearTireGripPercent = 1.f;
        tuning.constantDownforce = 0.f;
        tuning.forwardDownforce = 0.f;
        tuning.topSpeed = 29.f;
        tuning.driftBoost = 0.f;

        tuning.maxEngineOmega = 800.f;
        tuning.peekEngineTorque = 960.f;
        tuning.engineDampingFullThrottle = 0.15f;
        tuning.engineDampingZeroThrottleClutchEngaged = 1.5f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.6f;
        tuning.maxBrakeTorque = 4000.f;
        tuning.maxSteerAngle = glm::radians(40.f);
        tuning.clutchStrength = 5.f;
        tuning.gearSwitchTime = 0.11f;
        tuning.autoBoxSwitchTime = 0.6f;
        tuning.gearRatios = { -5.8f, 0.f, 5.8f, 2.4f, 1.6f, 1.25f };
        tuning.finalGearRatio = 5.3f;

        tuning.suspensionMaxCompression = 0.1f;
        tuning.suspensionMaxDroop = 0.2f;
        tuning.suspensionSpringStrength = 24000.f;
        tuning.suspensionSpringDamperRate = 4000.f;

        tuning.camberAngleAtRest = -0.01f;
        tuning.camberAngleAtMaxDroop = 0.1f;
        tuning.camberAngleAtMaxCompression = -0.1f;

        tuning.frontAntiRollbarStiffness = 8000.f;
        tuning.rearAntiRollbarStiffness = 8000.f;

        tuning.ackermannAccuracy = 0.5f;
    }
};
