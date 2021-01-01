#pragma once

#include "../vehicle_data.h"

class VRacecar : public VehicleData
{
public:
    VRacecar()
    {
        name = "Racecar";
        description = "Excessive speed.";
        price = 1;
        weaponSlots = {
            { "FRONT WEAPON", WeaponType::FRONT_WEAPON, WeaponClass::HOOD1 | WeaponClass::NARROW },
            { "REAR WEAPON", WeaponType::REAR_WEAPON },
            { "PASSIVE ABILITY", WeaponType::SPECIAL_ABILITY },
        };
    }

    void initializeTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        loadModelData("vehicle_racecar", tuning);

        tuning.maxHitPoints = 150;

        tuning.differential = VehicleDifferentialType::LS_RWD;
        tuning.chassisMass = 1100;
        tuning.wheelMassFront = 20;
        tuning.wheelMassRear = 20;
        tuning.wheelDampingRate = 0.24f;
        tuning.wheelOffroadDampingRate = 85;
        tuning.frontToeAngle = radians(0.f); // more responsive to inputs
        tuning.rearToeAngle = radians(0.01f); // faster recovery from slide
        tuning.trackTireFriction = 6.f;
        tuning.offroadTireFriction = 1.1f;

        tuning.rearTireGripPercent = 1.f;
        tuning.constantDownforce = 0.6f;
        tuning.forwardDownforce = 0.f;
        tuning.topSpeed = 50.f;
        tuning.driftBoost = 0.f;

        tuning.maxEngineOmega = 950.f;
        tuning.peekEngineTorque = 2250.f;
        tuning.engineDampingFullThrottle = 0.14f;
        tuning.engineDampingZeroThrottleClutchEngaged = 3.f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.8f;
        tuning.maxBrakeTorque = 15000.f;
        tuning.maxSteerAngleDegrees = 45.f;
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
        tuning.centerOfMass = { 0.f, 0.f, -0.1f };
    }
};
