#pragma once

#include "../vehicle_data.h"

class VMini : public VehicleData
{
public:
    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        name = "Mini";
        description = "Cool car";
        price = 5000;

        tuning.maxHitPoints = 100;

        tuning.specs.acceleration = 0.5f;
        tuning.specs.speed = 0.5f;
        tuning.specs.armor = 0.5f;
        tuning.specs.weight = 0.5f;
        tuning.specs.handling = 0.5f;

        tuning.physics.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_FRONTWD;
        tuning.physics.chassisDensity = 92;
        tuning.physics.wheelMassFront = 20;
        tuning.physics.wheelMassRear = 20;
        tuning.physics.wheelDampingRate = 0.5;
        tuning.physics.wheelOffroadDampingRate = 30;
        tuning.physics.frontToeAngle = 0;
        tuning.physics.rearToeAngle = 0;
        tuning.physics.trackTireFriction = 3.6f;
        tuning.physics.offroadTireFriction = 1.5f;

        tuning.physics.rearTireGripPercent = 1.f;
        tuning.physics.constantDownforce = 0.f;
        tuning.physics.forwardDownforce = 0.f;
        tuning.physics.topSpeed = 29.f;
        tuning.physics.driftBoost = 0.f;

        tuning.physics.maxEngineOmega = 800.f;
        tuning.physics.peekEngineTorque = 960.f;
        tuning.physics.engineDampingFullThrottle = 0.15f;
        tuning.physics.engineDampingZeroThrottleClutchEngaged = 1.5f;
        tuning.physics.engineDampingZeroThrottleClutchDisengaged = 0.6f;
        tuning.physics.maxBrakeTorque = 4000.f;
        tuning.physics.maxSteerAngle = glm::radians(40.f);
        tuning.physics.clutchStrength = 5.f;
        tuning.physics.gearSwitchTime = 0.11f;
        tuning.physics.autoBoxSwitchTime = 0.6f;
        tuning.physics.gearRatios = { -5.8f, 0.f, 5.8f, 2.4f, 1.6f, 1.25f };
        tuning.physics.finalGearRatio = 5.3f;

        tuning.physics.suspensionMaxCompression = 0.1f;
        tuning.physics.suspensionMaxDroop = 0.2f;
        tuning.physics.suspensionSpringStrength = 24000.f;
        tuning.physics.suspensionSpringDamperRate = 4000.f;

        tuning.physics.camberAngleAtRest = -0.01f;
        tuning.physics.camberAngleAtMaxDroop = 0.1f;
        tuning.physics.camberAngleAtMaxCompression = -0.1f;

        tuning.physics.frontAntiRollbarStiffness = 8000.f;
        tuning.physics.rearAntiRollbarStiffness = 8000.f;

        tuning.physics.ackermannAccuracy = 0.5f;

        loadSceneData("mini.Scene", tuning);
    }
};
