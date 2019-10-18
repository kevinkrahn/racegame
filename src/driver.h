#pragma once

#include "math.h"
#include "vehicle_data.h"
#include <algorithm>

struct Driver
{
    u32 leaguePoints = 0;
    //i32 credits = 10000;
    i32 credits = 100000;

    bool isPlayer = false;
    bool hasCamera = false;
    std::string playerName = "no-name";
    ComputerDriverData ai;
    bool useKeyboard = false;
    u32 controllerID = 0;
    i32 lastColorIndex = 0;

    struct OwnedVehicle
    {
        i32 vehicleIndex;
        VehicleConfiguration vehicleConfig;
    };
    std::vector<OwnedVehicle> ownedVehicles;

    i32 vehicleIndex = -1;

    VehicleConfiguration* getVehicleConfig()
    {
        auto ownedVehicle = std::find_if(ownedVehicles.begin(),
                        ownedVehicles.end(),
                        [&](auto& e) { return e.vehicleIndex == vehicleIndex; });
        assert(ownedVehicle != ownedVehicles.end());
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
            u32 controllerID=0, i32 vehicleIndex=-1, i32 colorIndex=0, i32 aiIndex=-1);

    Driver() = default;
    Driver(Driver&& other) = default;
    Driver& operator = (Driver&& other) = default;
};
