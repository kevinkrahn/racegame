#pragma once

#include "math.h"
#include "vehicle_data.h"

struct Driver
{
    Str64 playerName = "no-name";
    Str64 controllerGuid;
    u32 leaguePoints = 0;
    i32 credits = 10000;
    i64 aiDriverGUID = 0;
    i32 controllerID = -1;
    i64 vehicleGuid = 0;
    u32 lastPlacement = 0;
    bool isPlayer = false;
    bool hasCamera = false;
    bool useKeyboard = false;
    VehicleConfiguration vehicleConfig;

    VehicleConfiguration* getVehicleConfig()
    {
        return &vehicleConfig;
    }

    VehicleTuning getTuning()
    {
        VehicleData* v = g_res.getVehicle(vehicleGuid);
        assert(v);
        VehicleTuning tuning;
        v->initTuning(*getVehicleConfig(), tuning);
        return tuning;
    }

    VehicleData* getVehicleData()
    {
        return g_res.getVehicle(vehicleGuid);
    }

    i32 getVehicleValue()
    {
        VehicleData* v = g_res.getVehicle(vehicleGuid);
        if (!v)
        {
            return 0;
        }
        i32 value = v->price;
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
            i32 controllerID=0, i64 vehicleGuid=0, i64 aiDriverGUID=0);

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
        s.field(aiDriverGUID);
        s.field(lastPlacement);
        s.field(isPlayer);
        s.field(hasCamera);
        s.field(useKeyboard);
        s.field(vehicleGuid);
        s.field(vehicleConfig);
    }
};
