#pragma once

#include "../math.h"
#include "../renderer.h"

class EditorCamera
{
    f32 cameraDistance = 90.f;
    f32 zoomSpeed = 0.f;
    f32 cameraYaw = 3.14f;
    f32 cameraPitch = -1.f;
    f32 cameraYawSpeed = 0.f;
    f32 cameraPitchSpeed = 0.f;
    glm::vec3 cameraTarget = glm::vec3(0, 0, 0);
    glm::vec3 cameraVelocity = glm::vec3(0, 0, 0);
    glm::vec2 lastMousePosition;
    glm::vec3 cameraFrom;
    f32 nearDistance = 2.f;
    f32 farDistance = 380.f;
    Camera camera;

public:
    glm::vec3 getCameraTarget() const { return cameraTarget; }
    glm::vec3 getCameraFrom() const { return cameraFrom; }
    Camera const& getCamera() const { return camera; }

    void update(f32 deltaTime, RenderWorld* rw);
    glm::vec3 getMouseRay(RenderWorld* rw) const;

    void setNearFar(f32 nearDistance, f32 farDistance)
    {
        this->nearDistance = nearDistance;
        this->farDistance = farDistance;
    }
    void setCameraDistance(f32 distance) { cameraDistance = distance; }
};
