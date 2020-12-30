#pragma once

#include "math.h"
#include "vehicle_data.h"

struct Driver
{
    Str64 playerName = "no-name";
    Str64 controllerGuid;
    u32 leaguePoints = 0;
    i32 credits = 10000;
    i32 aiIndex = -1;
    i32 controllerID = -1;
    i32 vehicleIndex = -1;
    u32 lastPlacement = 0;
    bool isPlayer = false;
    bool hasCamera = false;
    bool useKeyboard = false;
    VehicleConfiguration vehicleConfig;

    VehicleConfiguration* getVehicleConfig()
    {
        assert(vehicleIndex != -1);
        return &vehicleConfig;
    }

    VehicleTuning getTuning()
    {
        assert(vehicleIndex != -1);
        VehicleTuning tuning;
        g_vehicles[vehicleIndex]->initTuning(*getVehicleConfig(), tuning);
        return tuning;
    }

    VehicleData* getVehicleData()
    {
        assert(vehicleIndex != -1);
        return g_vehicles[vehicleIndex].get();
    }

    i32 getVehicleValue()
    {
        if (vehicleIndex == -1)
        {
            return 0;
        }
        i32 value = g_vehicles[vehicleIndex]->price;
        VehicleData* vd = getVehicleData();
        for (auto& upgrade : vehicleConfig.performanceUpgrades)
        {
            for (i32 i=1; i<=upgrade.upgradeLevel; ++i)
            {
                value += vd->availableUpgrades[upgrade.upgradeIndex].price * i;
            }
        }
        for (u32 i=0; i<ARRAY_SIZE(vehicleConfig.weaponIndices); ++i)
        {
            i32 weaponIndex = vehicleConfig.weaponIndices[i];
            if (weaponIndex != -1)
            {
                value += g_weapons[weaponIndex].info.price * vehicleConfig.weaponUpgradeLevel[i];
            }
        }
        return value / 2;
    }

    Driver(bool hasCamera, bool isPlayer, bool useKeyboard,
            i32 controllerID=0, i32 vehicleIndex=-1, i32 aiIndex=-1);

    Driver() = default;
    Driver(Driver&& other) = default;
    Driver& operator = (Driver&& other) = default;

    void aiUpgrades(RandomSeries& series);

    void serialize(Serializer& s)
    {
        s.field(playerName);
        s.field(controllerGuid);
        s.field(leaguePoints);
        s.field(credits);
        s.field(aiIndex);
        s.field(vehicleIndex);
        s.field(lastPlacement);
        s.field(isPlayer);
        s.field(hasCamera);
        s.field(useKeyboard);
        s.field(vehicleIndex);
        s.field(vehicleConfig);
    }
};
