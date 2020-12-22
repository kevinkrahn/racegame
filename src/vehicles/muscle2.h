#pragma once

#include "../vehicle_data.h"

class VMuscle2 : public VehicleData
{
public:
    VMuscle2()
    {
        name = "Other Car";
        description = "It goes zoom zoom";
        price = 8990;
        frontWeaponCount = 1;
        rearWeaponCount = 1;
        defaultColorHsv = Vec3(0.08f, 0.95f, 0.98f);

        loadModelData("vehicle_muscle2");
        initStandardUpgrades();
    }

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        copySceneDataToTuning(tuning);

        tuning.maxHitPoints = 115;

        tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_REARWD;
        tuning.chassisMass = 1500;
        tuning.wheelMassFront = 16;
        tuning.wheelMassRear = 16;
        tuning.wheelDampingRate = 0.1f;
        tuning.wheelOffroadDampingRate = 25;
        //tuning.frontToeAngle = radians(-0.5f); // more responsive to inputs
        tuning.frontToeAngle = radians(-0.02f);
        //tuning.rearToeAngle = radians(4.5f); // faster recovery from slide
        tuning.rearToeAngle = radians(0.95f); // faster recovery from slide
        tuning.trackTireFriction = 2.2f;
        tuning.offroadTireFriction = 1.85f;

        tuning.rearTireGripPercent = 0.97f;
        tuning.constantDownforce = 0.f;
        tuning.forwardDownforce = 0.003f;
        tuning.topSpeed = 31.5f;
        tuning.driftBoost = 0.05f;

        tuning.maxEngineOmega = 800.f;
        tuning.peekEngineTorque = 1040.f;
        tuning.engineDampingFullThrottle = 0.3f;
        tuning.engineDampingZeroThrottleClutchEngaged = 1.4f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.5f;
        tuning.maxBrakeTorque = 6200.f;
        tuning.maxSteerAngle = radians(50.f);
        tuning.clutchStrength = 5.5f;
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

        for (auto& u : configuration.performanceUpgrades)
        {
            PerformanceUpgrade& upgrade = availableUpgrades[u.upgradeIndex];
            switch (upgrade.upgradeType)
            {
                case PerformanceUpgradeType::ENGINE:
                    tuning.peekEngineTorque += 12.f * u.upgradeLevel;
                    tuning.topSpeed += 1.5f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::TIRES:
                    tuning.trackTireFriction += 0.1f * u.upgradeLevel;
                    tuning.offroadTireFriction += 0.08f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::ARMOR:
                    tuning.maxHitPoints += 13.f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::WEIGHT_REDUCTION:
                    tuning.chassisMass -= 30.f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::SUSPENSION:
                    tuning.frontAntiRollbarStiffness += 500.f * u.upgradeLevel;
                    tuning.rearAntiRollbarStiffness += 500.f * u.upgradeLevel;
                    tuning.suspensionSpringStrength += 1000.f * u.upgradeLevel;
                    tuning.suspensionSpringDamperRate += 500.f * u.upgradeLevel;
                    break;
                default:
                    println("Unhandled upgrade: %s", upgrade.name);
                    break;
            }
        }
    }
};
