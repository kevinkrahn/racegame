#include "driver.h"
#include "resources.h"

Driver::Driver(bool hasCamera, bool isPlayer, bool useKeyboard,
        u32 controllerID, i32 vehicleIndex, i32 colorIndex, i32 aiIndex)
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

    if (aiIndex != -1 && !isPlayer)
    {
        this->ai = g_ais[aiIndex];
        this->playerName = this->ai.name;
    }
}
