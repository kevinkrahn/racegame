#pragma once

#include "math.h"
#include "vehicle_data.h"
#include <algorithm>

struct Driver
{
    u32 leaguePoints = 0;
    i32 credits = 10000;

    bool isPlayer = false;
    bool hasCamera = false;
    std::string playerName = "no-name";
    i32 aiIndex = -1;
    bool useKeyboard = false;
    u32 controllerID = 0;
    std::string controllerGuid;

    struct OwnedVehicle
    {
        i32 vehicleIndex;
        VehicleConfiguration vehicleConfig;
    };
    std::vector<OwnedVehicle> ownedVehicles;

    i32 vehicleIndex = -1;

    u32 lastPlacement = 0;

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

    void aiUpgrades(RandomSeries& series);

    DataFile::Value serialize()
    {
        DataFile::Value data = DataFile::makeDict();
        data["leaguePoints"] = DataFile::makeInteger(leaguePoints);
        data["credits"] = DataFile::makeInteger(credits);
        data["isPlayer"] = DataFile::makeBool(isPlayer);
        data["aiIndex"] = DataFile::makeInteger(aiIndex);
        data["hasCamera"] = DataFile::makeBool(hasCamera);
        data["playerName"] = DataFile::makeString(playerName);
        data["useKeyboard"] = DataFile::makeBool(useKeyboard);
        data["controllerGuid"] = DataFile::makeString(controllerGuid);
        data["ownedVehicles"] = DataFile::makeArray();
        for (auto& v : ownedVehicles)
        {
            DataFile::Value vehicleData = DataFile::makeDict();
            vehicleData["vehicleIndex"] = DataFile::makeInteger(vehicleIndex);
            vehicleData["vehicleConfig"] = v.vehicleConfig.serialize();
            data["ownedVehicles"].array().push_back(std::move(vehicleData));
        }
        data["vehicleIndex"] = DataFile::makeInteger(vehicleIndex);
        data["lastPlacement"] = DataFile::makeInteger(lastPlacement);
        return data;
    }

    void deserialize(DataFile::Value& data)
    {
        leaguePoints = (u32)data["leaguePoints"].integer();
        credits = (i32)data["credits"].integer();
        isPlayer = data["isPlayer"].boolean();
        aiIndex = (i32)data["aiIndex"].integer();
        hasCamera = data["hasCamera"].boolean();
        playerName = data["playerName"].string();
        useKeyboard = data["useKeyboard"].boolean();
        controllerGuid = data["controllerGuid"].string();
        auto& vehicles = data["ownedVehicles"].array();
        for(auto& v : vehicles)
        {
            OwnedVehicle ov;
            ov.vehicleIndex = (i32)v["vehicleIndex"].integer();
            ov.vehicleConfig.deserialize(v["vehicleConfig"]);
            ownedVehicles.push_back(std::move(ov));
        }
        vehicleIndex = (i32)data["vehicleIndex"].integer();
        lastPlacement = (u32)data["lastPlacement"].integer();
    }
};
