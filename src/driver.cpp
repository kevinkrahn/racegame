#include "driver.h"
#include "game.h"

Driver::Driver(bool hasCamera, bool isPlayer, bool useKeyboard, VehicleData* vehicleData,
        u32 colorIndex, u32 controllerID)
{
    auto& color = game.resources.getVehicleColors()[colorIndex];

    this->hasCamera = hasCamera;
    this->vehicleData = vehicleData;
    this->vehicleColor = color->color;
    this->vehicleMaterial = color->material.get();
    this->useKeyboard = useKeyboard;
    this->controllerID = controllerID;
    if (isPlayer)
    {
        this->playerProfile = new PlayerProfile{ "Test" };
    }
}
