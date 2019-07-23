#include "driver.h"
#include "resources.h"

Driver::Driver(bool hasCamera, bool isPlayer, bool useKeyboard, VehicleData* vehicleData,
        u32 colorIndex, u32 controllerID)
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
