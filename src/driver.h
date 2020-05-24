#pragma once

#include "math.h"
#include "vehicle_data.h"

struct Driver
{
    std::string playerName = "no-name";
    std::string controllerGuid;
    u32 leaguePoints = 0;
    i32 credits = 10000;
    i32 aiIndex = -1;
    i32 controllerID = -1;
    i32 vehicleIndex = -1;
    u32 lastPlacement = 0;
    bool isPlayer = false;
    bool hasCamera = false;
    bool useKeyboard = false;

    struct OwnedVehicle
    {
        i32 vehicleIndex;
        VehicleConfiguration vehicleConfig;

        void serialize(Serializer& s)
        {
            s.field(vehicleIndex);
            s.field(vehicleConfig);
        }
    };
    Array<OwnedVehicle> ownedVehicles;

    VehicleConfiguration* getVehicleConfig()
    {
        auto ownedVehicle = ownedVehicles.find([&](auto& e) { return e.vehicleIndex == vehicleIndex; });
        assert(ownedVehicle);
        return &ownedVehicle->vehicleConfig;
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


    Driver(bool hasCamera, bool isPlayer, bool useKeyboard,
            i32 controllerID=0, i32 vehicleIndex=-1, i32 colorIndex=0, i32 aiIndex=-1);

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
        s.field(ownedVehicles);
    }
};
