#pragma once

#include "../vehicle_data.h"

class VSportscar : public VehicleData
{
public:
    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        name = "Sportscar";
        description = "Cool car";
        price = 8000;

        tuning.maxHitPoints = 100;

        tuning.specs.acceleration = 0.5f;
        tuning.specs.speed = 0.6f;
        tuning.specs.armor = 0.5f;
        tuning.specs.weight = 0.5f;
        tuning.specs.handling = 0.5f;

        tuning.physics.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_REARWD;
        tuning.physics.chassisDensity = 89;
        tuning.physics.wheelMassFront = 20;
        tuning.physics.wheelMassRear = 20;
        tuning.physics.wheelDampingRate = 0.1f;
        tuning.physics.wheelOffroadDampingRate = 30;
        tuning.physics.frontToeAngle = glm::radians(-0.5f); // more responsive to inputs
        tuning.physics.rearToeAngle = glm::radians(4.5f); // faster recovery from slide
        tuning.physics.trackTireFriction = 3.14f;
        tuning.physics.offroadTireFriction = 1.5f;

        tuning.physics.rearTireGripPercent = 0.835f;
        tuning.physics.constantDownforce = 0.f;
        tuning.physics.forwardDownforce = 0.001f;
        tuning.physics.topSpeed = 33.f;
        tuning.physics.driftBoost = 0.f;

        tuning.physics.maxEngineOmega = 650.f;
        tuning.physics.peekEngineTorque = 930.f;
        tuning.physics.engineDampingFullThrottle = 0.3f;
        tuning.physics.engineDampingZeroThrottleClutchEngaged = 1.5f;
        tuning.physics.engineDampingZeroThrottleClutchDisengaged = 0.6f;
        tuning.physics.maxBrakeTorque = 6000.f;
        tuning.physics.maxSteerAngle = glm::radians(52.f);
        tuning.physics.clutchStrength = 5.f;
        tuning.physics.gearSwitchTime = 0.15f;
        tuning.physics.autoBoxSwitchTime = 1.2f;
        tuning.physics.gearRatios = { -4.3f, 0.f, 4.f, 1.65f, 1.21f, 0.95f };
        tuning.physics.finalGearRatio = 4.3f;

        tuning.physics.suspensionMaxCompression = 0.05f;
        tuning.physics.suspensionMaxDroop = 0.15f;
        tuning.physics.suspensionSpringStrength = 28000.f;
        tuning.physics.suspensionSpringDamperRate = 5000.f;

        tuning.physics.camberAngleAtRest = -0.08f;
        tuning.physics.camberAngleAtMaxDroop = 0.f;
        tuning.physics.camberAngleAtMaxCompression = -0.15f;

        tuning.physics.frontAntiRollbarStiffness = 7000.f;
        tuning.physics.rearAntiRollbarStiffness = 7000.f;

        tuning.physics.ackermannAccuracy = 0.5f;

        loadSceneData("sportscar.Vehicle", tuning);
    }
};
