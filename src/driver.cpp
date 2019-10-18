#include "driver.h"
#include "resources.h"

Driver::Driver(bool hasCamera, bool isPlayer, bool useKeyboard,
        u32 controllerID, i32 vehicleIndex, i32 colorIndex)
{
    this->hasCamera = hasCamera;
    this->vehicleIndex = vehicleIndex;
    this->useKeyboard = useKeyboard;
    this->controllerID = controllerID;
    this->isPlayer = isPlayer;

    if (vehicleIndex != -1)
    {
        VehicleConfiguration vehicleConfig;
        vehicleConfig.colorIndex = colorIndex;
        ownedVehicles.push_back({ vehicleIndex, vehicleConfig });
    }
}
