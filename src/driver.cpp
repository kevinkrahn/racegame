#include "driver.h"
#include "resources.h"

glm::vec3 vehicleColors[] = {
    { 0.7f, 0.01f, 0.01f },
    { 0.02f, 0.02f, 0.02f },
    { 0.01f, 0.8f, 0.01f },
    { 0.01f, 0.01f, 0.8f },
    { 0.8f, 0.01f, 0.8f },
    { 0.01f, 0.8f, 0.8f },
    { 0.5f, 0.2f, 0.8f },
    { 0.9f, 0.01f, 0.5f },
    { 0.02f, 0.7f, 0.1f },
};

Driver::Driver(bool hasCamera, bool isPlayer, bool useKeyboard, i32 vehicleIndex,
        u32 colorIndex, u32 controllerID)
{
    this->hasCamera = hasCamera;
    this->vehicleIndex = vehicleIndex;
    this->useKeyboard = useKeyboard;
    this->controllerID = controllerID;
    this->vehicleColor = vehicleColors[colorIndex];
    this->isPlayer = isPlayer;
}
