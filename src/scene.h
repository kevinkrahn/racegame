#pragma once

#include "misc.h"
#include "vehicle.h"
#include <vector>
#include <glm/mat4x4.hpp>

struct StaticEntity
{
    u32 renderHandle;
    glm::mat4 worldTransform;
};

struct Checkpoint
{
    glm::vec3 position;
    f32 t;
    glm::vec3 direction;
    f32 angle;
    u32 connections[4];
    u32 connectionCount;
};

class Scene
{
    std::vector<StaticEntity> staticEntities;
    std::vector<Vehicle> vehicles;

public:
    Scene(const char* name);

    void onStart();
    void onEnd();
    void onUpdate(f32 deltaTime);
};
