#pragma once

#include "misc.h"
#include "math.h"

class Editor
{
    f32 cameraDistance = 90.f;
    f32 cameraAngle = 0.f;
    glm::vec3 cameraTarget = glm::vec3(0, 0, 0);
    glm::vec2 lastMousePosition;

public:
    void onUpdate(f32 deltaTime);
};
