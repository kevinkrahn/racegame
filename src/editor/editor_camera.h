#pragma once

#include "../math.h"
#include "../renderer.h"

class EditorCamera
{
    f32 cameraDistance = 90.f;
    f32 zoomSpeed = 0.f;
    f32 cameraAngle = 0.f;
    f32 cameraRotateSpeed = 0.f;
    glm::vec3 cameraTarget = glm::vec3(0, 0, 0);
    glm::vec3 cameraVelocity = glm::vec3(0, 0, 0);
    glm::vec2 lastMousePosition;
    glm::vec3 cameraFrom;
    Camera camera;

public:
    glm::vec3 getCameraTarget() const { return cameraTarget; }
    glm::vec3 getCameraFrom() const { return cameraFrom; }
    Camera const& getCamera() const { return camera; }

    void update(f32 deltaTime, RenderWorld* rw);
    glm::vec3 getMouseRay(RenderWorld* rw) const;
};
