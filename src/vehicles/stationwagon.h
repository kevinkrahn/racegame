#pragma once

#include "../vehicle_data.h"
#include "../resources.h"

class VStationWagon : public VehicleData
{
public:
    VStationWagon()
    {
        name = "Station Wagon";
        description = "Not so great.";
        price = 8000;
        frontWeaponCount = 1;
        rearWeaponCount = 2;

        loadSceneData("stationwagon.Vehicle");

        availableUpgrades = {
            {
                "Engine",
                "Upgrades the engine to improve\nacceleration and top speed.",
                &g_res.textures->icon_pistons,
                PerformanceUpgradeType::ENGINE,
                5,
                1500,
            },
            {
                "Tires",
                "Equips better tires for improved traction\nand overall handling.",
                &g_res.textures->icon_wheel,
                PerformanceUpgradeType::TIRES,
                5,
                1000,
            },
            {
                "Armor",
                "Adds additional armor to improve\nresistance against all forms of damage.",
                &g_res.textures->icon_armor,
                PerformanceUpgradeType::ARMOR,
                5,
                1000,
            },
            {
                "Suspension",
                "Upgrades the suspension to be stiffer\nand more stable around corners.",
                &g_res.textures->icon_suspension,
                PerformanceUpgradeType::SUSPENSION,
                2,
                1250,
            },
        };
    }

    void initTuning(VehicleConfiguration const& configuration, VehicleTuning& tuning) override
    {
        copySceneDataToTuning(tuning);

        tuning.maxHitPoints = 100;

        tuning.specs.acceleration = 0.25f;
        tuning.specs.handling = 0.3f;
        tuning.specs.offroad = 0.2f;

        tuning.differential = PxVehicleDifferential4WData::eDIFF_TYPE_OPEN_REARWD;
        tuning.chassisMass = 1350;
        tuning.wheelMassFront = 20;
        tuning.wheelMassRear = 20;
        tuning.wheelDampingRate = 0.1f;
        tuning.wheelOffroadDampingRate = 28;
        //tuning.frontToeAngle = glm::radians(-0.5f); // more responsive to inputs
        tuning.frontToeAngle = glm::radians(0.f);
        //tuning.rearToeAngle = glm::radians(4.5f); // faster recovery from slide
        tuning.rearToeAngle = glm::radians(0.9f); // faster recovery from slide
        tuning.trackTireFriction = 2.38f;
        tuning.offroadTireFriction = 1.1f;

        tuning.rearTireGripPercent = 0.96f;
        tuning.constantDownforce = 0.f;
        tuning.forwardDownforce = 0.001f;
        tuning.topSpeed = 33.f;
        tuning.driftBoost = 0.f;

        tuning.maxEngineOmega = 720.f;
        tuning.peekEngineTorque = 990.f;
        tuning.engineDampingFullThrottle = 0.3f;
        tuning.engineDampingZeroThrottleClutchEngaged = 1.5f;
        tuning.engineDampingZeroThrottleClutchDisengaged = 0.6f;
        tuning.maxBrakeTorque = 6000.f;
        tuning.maxSteerAngle = glm::radians(51.f);
        tuning.clutchStrength = 5.f;
        tuning.gearSwitchTime = 0.15f;
        tuning.autoBoxSwitchTime = 1.2f;
        tuning.gearRatios = { -4.3f, 0.f, 4.f, 1.65f, 1.21f, 0.95f };
        tuning.finalGearRatio = 4.3f;

        tuning.suspensionMaxCompression = 0.15f;
        tuning.suspensionMaxDroop = 0.25f;
        tuning.suspensionSpringStrength = 26000.f;
        tuning.suspensionSpringDamperRate = 5000.f;

        tuning.camberAngleAtRest = -0.07f;
        tuning.camberAngleAtMaxDroop = 0.f;
        tuning.camberAngleAtMaxCompression = -0.13f;

        tuning.frontAntiRollbarStiffness = 6500.f;
        tuning.rearAntiRollbarStiffness = 6500.f;
        tuning.ackermannAccuracy = 0.5f;
        tuning.centerOfMass = { 0.09f, 0.f, -0.6f };

        for (auto& u : configuration.performanceUpgrades)
        {
            PerformanceUpgrade& upgrade = availableUpgrades[u.upgradeIndex];
            switch (upgrade.upgradeType)
            {
                case PerformanceUpgradeType::ENGINE:
                    tuning.peekEngineTorque += 11.f * u.upgradeLevel;
                    tuning.topSpeed += 1.2f * u.upgradeLevel;
                    tuning.specs.acceleration += 0.05f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::TIRES:
                    tuning.trackTireFriction += 0.16f * u.upgradeLevel;
                    tuning.offroadTireFriction += 0.05f * u.upgradeLevel;
                    tuning.specs.acceleration += 0.02f * u.upgradeLevel;
                    tuning.specs.offroad += 0.05f * u.upgradeLevel;
                    tuning.specs.handling += 0.06f * u.upgradeLevel;
                    break;
                case PerformanceUpgradeType::ARMOR:
                    tuning.maxHitPoints += 12.f * u.upgradeLevel;
                    break;
                // TODO: Add visible lowering of suspension
                case PerformanceUpgradeType::SUSPENSION:
                    tuning.frontAntiRollbarStiffness += 500.f * u.upgradeLevel;
                    tuning.rearAntiRollbarStiffness += 500.f * u.upgradeLevel;
                    tuning.suspensionSpringStrength += 1000.f * u.upgradeLevel;
                    tuning.suspensionSpringDamperRate += 500.f * u.upgradeLevel;
                    tuning.specs.handling += 0.05f * u.upgradeLevel;
                    break;
                default:
                    print("Unhandled upgrade: ", upgrade.name, '\n');
                    break;
            }
        }
    }
};
