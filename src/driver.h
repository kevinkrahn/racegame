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

glm::vec3 vehicleColors[] = {
    { 1.f, 0.1f, 0.1f }, // red
    { 0.1f, 1.f, 0.1f }, // green
    { 0.1f, 0.1f, 1.f }, // blue
    { 0.8f, 0.8f, 0.8f }, // white
    { 0.1f, 0.1f, 0.1f }, // black
    { 1.f, 0.6f, 0.f }, // orange
    { 0.f, 1.f, 1.f }, // cyan
    { 0.9f, 0.f, 0.9f }, // magenta
    { 0.1f, 1.f, 0.4f }, // aruba
    { 0.7f, 0.4f, 0.2f }, // tan
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

    Driver(bool hasCamera, bool isPlayer, bool useKeyboard, VehicleData* vehicleData,
            u32 colorIndex, u32 controllerID=0)
    {
        this->hasCamera = hasCamera;
        this->vehicleData = vehicleData;
        this->vehicleColor = vehicleColors[colorIndex];
        this->useKeyboard = useKeyboard;
        this->controllerID = controllerID;
        if (isPlayer)
        {
            this->playerProfile = new PlayerProfile{ "Test" };
        }
    }
};
