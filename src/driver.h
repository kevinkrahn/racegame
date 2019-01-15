#pragma once

#include "math.h"
#include "vehicle_data.h"

struct PlayerProfile
{
    std::string name;
};

struct ComputerDriverData
{
    std::string name;
    f32 drivingSkill; // [0,1] how optimal of a path the AI takes on the track
    f32 aggression;   // [0,1] how likely the AI is to go out of its way to attack other drivers
    f32 awareness;    // [0,1] how likely the AI is to attempt to avoid hitting other drivers and obstacles
    f32 fear;         // [0,1] how much the AI tries to evade other drivers
};

struct Driver
{
    u32 leaguePoints = 0;

    bool hasCamera = false;
    PlayerProfile* playerProfile = nullptr;
    ComputerDriverData aiDriverData;
    bool useKeyboard = false;
    u32 controllerID = 0;

    VehicleData* vehicleData = nullptr;
    glm::vec3 vehicleColor = { 1.f, 1.f, 1.f };

    Driver(bool hasCamera, bool isPlayer, bool useKeyboard, VehicleData* vehicleData, u32 controllerID=0)
    {
        this->hasCamera = hasCamera;
        this->vehicleData = vehicleData;
        this->useKeyboard = useKeyboard;
        this->controllerID = controllerID;
        if (isPlayer)
        {
            this->playerProfile = new PlayerProfile{ "Test" };
        }
    }
};
