#pragma once

#include "../vehicle_data.h"

class VRacecar : public VehicleData
{
public:
    VRacecar()
    {
        name = "Racecar";
        description = "It is very fast";
        //price = 80000;
        price = 1;
        loadSceneData("racecar.Vehicle");
    }

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        copySceneDataToTuning(tuning);

        tuning.maxHitPoints = 150;

        tuning.specs.acceleration = 1.f;
        tuning.specs.handling = 0.9f;

        tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;
        tuning.chassisDensity = 78;
        tuning.wheelMassFront = 20;
        tuning.wheelMassRear = 20;
        tuning.wheelDampingRate = 0.24f;
        tuning.wheelOffroadDampingRate = 85;
        tuning.frontToeAngle = glm::radians(0.f); // more responsive to inputs
        tuning.rearToeAngle = glm::radians(0.01f); // faster recovery from slide
        tuning.trackTireFriction = 4.3f;
        tuning.offroadTireFriction = 1.4f;

        tuning.rearTireGripPercent = 1.f;
        tuning.constantDownforce = 0.015f;
        tuning.forwardDownforce = 0.005f;
        tuning.topSpeed = 100.f;
        tuning.driftBoost = 0.f;

        tuning.maxEngineOmega = 900.f;
        tuning.peekEngineTorque = 950.f;
        tuning.engineDampingFullThrottle = 0.14f;
        tuning.engineDampingZeroThrottleClutchEngaged = 3.f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.8f;
        tuning.maxBrakeTorque = 15000.f;
        tuning.maxSteerAngle = glm::radians(52.f);
        tuning.clutchStrength = 12.f;
        tuning.gearSwitchTime = 0.04f;
        tuning.autoBoxSwitchTime = 0.5f;
        tuning.gearRatios = { -4.f, 0.f, 5.f, 2.7f, 2.f, 1.6f, 1.2f, 1.0f, 0.9f };
        tuning.finalGearRatio = 4.f;

        tuning.suspensionMaxCompression = 0.05f;
        tuning.suspensionMaxDroop = 0.11f;
        tuning.suspensionSpringStrength = 30000.f;
        tuning.suspensionSpringDamperRate = 5000.f;

        tuning.camberAngleAtRest = 0.f;
        tuning.camberAngleAtMaxDroop = 0.02f;
        tuning.camberAngleAtMaxCompression = -0.02f;

        tuning.frontAntiRollbarStiffness = 12500.f;
        tuning.rearAntiRollbarStiffness = 12500.f;

        tuning.ackermannAccuracy = 1.f;
    }
};
