#pragma once

#include "misc.h"
#include "math.h"

class Editor
{
    f32 cameraDistance = 90.f;
    f32 zoomSpeed = 0.f;
    f32 cameraAngle = 0.f;
    f32 cameraRotateSpeed = 0.f;
    glm::vec3 cameraTarget = glm::vec3(0, 0, 0);
    glm::vec2 lastMousePosition;
    glm::vec3 cameraVelocity = glm::vec3(0, 0, 0);
    glm::vec3 debugP = glm::vec3(0, 0, 0);

    f32 brushRadius = 8.f;
    f32 brushFalloff = 1.f;
    f32 brushStrength = 15.f;

    f32 brushStartZ = 0.f;

    enum TerrainTool
    {
        RAISE,
        PERTURB,
        FLATTEN,
        SMOOTH,
        MAX
    } terrainTool = TerrainTool::RAISE;

public:
    void onStart(class Scene* scene);
    void onUpdate(class Scene* scene, class Renderer* renderer, f32 deltaTime);
};
