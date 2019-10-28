#pragma once

#include "../vehicle_data.h"

class VTruck : public VehicleData
{
public:
    VTruck()
    {
        name = "Truck";
        description = "Rough and tough.\nDrives well off the road.";
        price = 13000;
        frontWeaponCount = 2;
        rearWeaponCount = 3;

        loadSceneData("truck.Truck");

        // TODO: add upgrade icons
        availableUpgrades = {
            {
                "Engine",
                "Upgrades the engine to improve\nacceleration and top speed.",
                nullptr,
                PerformanceUpgradeType::ENGINE,
                5,
                1500,
            },
            {
                "Tires",
                "Equips better tires for improved traction\nand overall handling.",
                nullptr,
                PerformanceUpgradeType::TIRES,
                5,
                1000,
            },
            {
                "Armor",
                "Adds additional armor to improve\nresistance against all forms of damage.",
                nullptr,
                PerformanceUpgradeType::ARMOR,
                5,
                1000,
            },
        };
    }

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        copySceneDataToTuning(tuning);

        tuning.maxHitPoints = 175;

        tuning.specs.acceleration = 0.3f;
        tuning.specs.handling = 0.5f;
        tuning.specs.offroad = 0.5f;

        tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_LS_4WD;
        tuning.chassisMass = 2090;
        tuning.wheelMassFront = 25;
        tuning.wheelMassRear = 25;
        tuning.wheelDampingRate = 0.7f;
        tuning.wheelOffroadDampingRate = 19;
        tuning.frontToeAngle = 0;
        tuning.rearToeAngle = 0;
        tuning.trackTireFriction = 2.1f;
        tuning.offroadTireFriction = 1.1f;

        tuning.rearTireGripPercent = 1.f;
        tuning.constantDownforce = 0.05f;
        tuning.forwardDownforce = 0.f;
        tuning.topSpeed = 31.f;
        tuning.driftBoost = 0.f;

        tuning.maxEngineOmega = 800.f;
        tuning.peekEngineTorque = 980.f;
        tuning.engineDampingFullThrottle = 0.15f;
        tuning.engineDampingZeroThrottleClutchEngaged = 1.5f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.6f;
        tuning.maxBrakeTorque = 6000.f;
        tuning.maxSteerAngle = glm::radians(48.f);
        tuning.clutchStrength = 4.f;
        tuning.gearSwitchTime = 0.2f;
        tuning.autoBoxSwitchTime = 0.8f;
        tuning.gearRatios = { -5.5f, 0.f, 5.5f, 2.4f, 1.6f, 1.25f };
        tuning.finalGearRatio = 5.f;

        tuning.suspensionMaxCompression = 0.15f;
        tuning.suspensionMaxDroop = 0.35f;
        tuning.suspensionSpringStrength = 50000.f;
        tuning.suspensionSpringDamperRate = 4500.f;

        tuning.camberAngleAtRest = -0.01f;
        tuning.camberAngleAtMaxDroop = 0.1f;
        tuning.camberAngleAtMaxCompression = -0.1f;

        tuning.frontAntiRollbarStiffness = 8500.f;
        tuning.rearAntiRollbarStiffness = 8500.f;
        tuning.ackermannAccuracy = 0.5f;
        tuning.centerOfMass = { 0.f, 0.f, -0.2f };

        for (auto& u : configuration.performanceUpgrades)
        {
            PerformanceUpgrade& upgrade = availableUpgrades[u.upgradeIndex];
            switch (upgrade.upgradeType)
            {
                case PerformanceUpgradeType::ENGINE:
                    tuning.peekEngineTorque += 10.f * u.upgradeLevel;
                    tuning.topSpeed += 1.f * u.upgradeLevel;
                    tuning.specs.acceleration += 0.04f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::TIRES:
                    tuning.trackTireFriction += 0.12f * u.upgradeLevel;
                    tuning.offroadTireFriction += 0.05f * u.upgradeLevel;
                    tuning.specs.acceleration += 0.02f * u.upgradeLevel;
                    tuning.specs.offroad += 0.03f * u.upgradeLevel;
                    tuning.specs.handling += 0.04f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::ARMOR:
                    tuning.maxHitPoints += 12.f * u.upgradeLevel;
                    break;
                default:
                    print("Unhandled upgrade: ", upgrade.name, '\n');
                    break;
            }
        }
    }
};
