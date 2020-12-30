#pragma once

#include "../vehicle_data.h"
#include "../resources.h"

class VCoolCar : public VehicleData
{
public:
    VCoolCar()
    {
        name = "Cool Car";
        description = "Pretty great";
        price = 10000;
        weaponSlots = {
            { "FRONT WEAPON", WeaponType::FRONT_WEAPON, WeaponClass::HOOD1 },
            { "REAR WEAPON", WeaponType::REAR_WEAPON },
            { "PASSIVE ABILITY", WeaponType::SPECIAL_ABILITY },
        };

        loadModelData("vehicle_coolcar");
        initStandardUpgrades();
    }

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        copySceneDataToTuning(tuning);

        tuning.maxHitPoints = 100;

        tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_REARWD;
        tuning.chassisMass = 1280;
        tuning.wheelMassFront = 15;
        tuning.wheelMassRear = 15;
        tuning.wheelDampingRate = 0.1f;
        tuning.wheelOffroadDampingRate = 28;
        //tuning.frontToeAngle = radians(-0.5f); // more responsive to inputs
        tuning.frontToeAngle = radians(0.f);
        //tuning.rearToeAngle = radians(4.5f); // faster recovery from slide
        tuning.rearToeAngle = radians(0.9f); // faster recovery from slide
        tuning.trackTireFriction = 2.4f;
        tuning.offroadTireFriction = 1.3f;

        tuning.rearTireGripPercent = 0.98f;
        tuning.constantDownforce = 0.001f;
        tuning.forwardDownforce = 0.004f;
        tuning.topSpeed = 35.f;
        tuning.driftBoost = 0.f;

        tuning.maxEngineOmega = 900.f;
        tuning.peekEngineTorque = 1150.f;
        tuning.engineDampingFullThrottle = 0.3f;
        tuning.engineDampingZeroThrottleClutchEngaged = 1.5f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.6f;
        tuning.maxBrakeTorque = 6000.f;
        tuning.maxSteerAngle = radians(55.f);
        tuning.clutchStrength = 5.f;
        tuning.gearSwitchTime = 0.15f;
        tuning.autoBoxSwitchTime = 1.2f;
        tuning.gearRatios = { -4.3f, 0.f, 4.f, 1.65f, 1.21f, 0.95f };
        tuning.finalGearRatio = 4.3f;

        tuning.suspensionMaxCompression = 0.1f;
        tuning.suspensionMaxDroop = 0.18f;
        tuning.suspensionSpringStrength = 30000.f;
        tuning.suspensionSpringDamperRate = 5000.f;

        tuning.camberAngleAtRest = -0.07f;
        tuning.camberAngleAtMaxDroop = 0.f;
        tuning.camberAngleAtMaxCompression = -0.13f;

        tuning.frontAntiRollbarStiffness = 8000.f;
        tuning.rearAntiRollbarStiffness = 8000.f;
        tuning.ackermannAccuracy = 0.8f;
        tuning.centerOfMass = { 0.4f, 0.f, -0.6f };

        for (auto& u : configuration.performanceUpgrades)
        {
            PerformanceUpgrade& upgrade = availableUpgrades[u.upgradeIndex];
            switch (upgrade.upgradeType)
            {
                case PerformanceUpgradeType::ENGINE:
                    tuning.peekEngineTorque += 15.f * u.upgradeLevel;
                    tuning.topSpeed += 1.2f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::TIRES:
                    tuning.trackTireFriction += 0.11f * u.upgradeLevel;
                    tuning.offroadTireFriction += 0.02f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::ARMOR:
                    tuning.maxHitPoints += 10.f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::WEIGHT_REDUCTION:
                    tuning.chassisMass -= 30.f * u.upgradeLevel;
                    break;
                // TODO: Add visible lowering of suspension
                case PerformanceUpgradeType::SUSPENSION:
                    tuning.frontAntiRollbarStiffness += 600.f * u.upgradeLevel;
                    tuning.rearAntiRollbarStiffness += 600.f * u.upgradeLevel;
                    tuning.suspensionSpringStrength += 1100.f * u.upgradeLevel;
                    tuning.suspensionSpringDamperRate += 600.f * u.upgradeLevel;
                    tuning.trackTireFriction += 0.05f * u.upgradeLevel;
                    break;
                default:
                    println("Unhandled upgrade: %s", upgrade.name);
                    break;
            }
        }
    }
};
