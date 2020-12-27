#pragma once

#include "../vehicle_data.h"
#include "../resources.h"

class VSportscar : public VehicleData
{
public:
    VSportscar()
    {
        name = "Sportscar";
        description = "Not really a sportscar.";
        price = 10000;
        weaponSlots = {
            { "FRONT WEAPON", WeaponType::FRONT_WEAPON, WeaponClass::HOOD1 },
            { "ROOF WEAPON", WeaponType::FRONT_WEAPON, WeaponClass::ROOF1 },
            { "REAR WEAPON", WeaponType::REAR_WEAPON }
        };
        defaultColorHsv = Vec3(0.05f, 0.1f, 0.05f);

        loadModelData("vehicle_sportscar");
        initStandardUpgrades();
    }

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        copySceneDataToTuning(tuning);

        tuning.maxHitPoints = 120;

        tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_REARWD;
        tuning.chassisMass = 1390;
        tuning.wheelMassFront = 20;
        tuning.wheelMassRear = 20;
        tuning.wheelDampingRate = 0.1f;
        tuning.wheelOffroadDampingRate = 28;
        //tuning.frontToeAngle = radians(-0.5f); // more responsive to inputs
        tuning.frontToeAngle = radians(0.f);
        //tuning.rearToeAngle = radians(4.5f); // faster recovery from slide
        tuning.rearToeAngle = radians(0.9f); // faster recovery from slide
        tuning.trackTireFriction = 2.38f;
        tuning.offroadTireFriction = 1.2f;

        tuning.rearTireGripPercent = 0.96f;
        tuning.constantDownforce = 0.f;
        tuning.forwardDownforce = 0.001f;
        tuning.topSpeed = 33.f;
        tuning.driftBoost = 0.02f;

        tuning.maxEngineOmega = 750.f;
        tuning.peekEngineTorque = 1100.f;
        tuning.engineDampingFullThrottle = 0.3f;
        tuning.engineDampingZeroThrottleClutchEngaged = 1.5f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.6f;
        tuning.maxBrakeTorque = 6000.f;
        tuning.maxSteerAngle = radians(51.f);
        tuning.clutchStrength = 5.f;
        tuning.gearSwitchTime = 0.15f;
        tuning.autoBoxSwitchTime = 1.2f;
        tuning.gearRatios = { -4.3f, 0.f, 4.f, 1.65f, 1.21f, 0.95f };
        tuning.finalGearRatio = 4.3f;

        tuning.suspensionMaxCompression = 0.15f;
        tuning.suspensionMaxDroop = 0.25f;
        tuning.suspensionSpringStrength = 28000.f;
        tuning.suspensionSpringDamperRate = 5000.f;

        tuning.camberAngleAtRest = -0.07f;
        tuning.camberAngleAtMaxDroop = 0.f;
        tuning.camberAngleAtMaxCompression = -0.13f;

        tuning.frontAntiRollbarStiffness = 7000.f;
        tuning.rearAntiRollbarStiffness = 7000.f;
        tuning.ackermannAccuracy = 0.5f;
        tuning.centerOfMass = { 0.28f, 0.f, -0.6f };

        for (auto& u : configuration.performanceUpgrades)
        {
            PerformanceUpgrade& upgrade = availableUpgrades[u.upgradeIndex];
            switch (upgrade.upgradeType)
            {
                case PerformanceUpgradeType::ENGINE:
                    tuning.peekEngineTorque += 11.f * u.upgradeLevel;
                    tuning.topSpeed += 1.2f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::TIRES:
                    tuning.trackTireFriction += 0.16f * u.upgradeLevel;
                    tuning.offroadTireFriction += 0.05f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::ARMOR:
                    tuning.maxHitPoints += 12.f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::WEIGHT_REDUCTION:
                    tuning.chassisMass -= 20.f * u.upgradeLevel;
                    break;
                // TODO: Add visible lowering of suspension
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
