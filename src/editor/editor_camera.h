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
    Vec3 cameraTarget = Vec3(0, 0, 0);
    Vec3 cameraVelocity = Vec3(0, 0, 0);
    Vec2 lastMousePosition;
    Vec3 cameraFrom;
    f32 nearDistance = 2.f;
    f32 farDistance = 380.f;
    Camera camera;

public:
    Vec3 getCameraTarget() const { return cameraTarget; }
    Vec3 getCameraFrom() const { return cameraFrom; }
    Camera const& getCamera() const { return camera; }

    void update(f32 deltaTime, RenderWorld* rw);
    Vec3 getMouseRay(RenderWorld* rw) const;

    void setNearFar(f32 nearDistance, f32 farDistance)
    {
        this->nearDistance = nearDistance;
        this->farDistance = farDistance;
    }
    void setCameraDistance(f32 distance) { cameraDistance = distance; }
};
