#pragma once

#include "misc.h"
#include "math.h"
#include "track_graph.h"
#include <vector>
#include <PxPhysicsAPI.h>

using namespace physx;

enum
{
    COLLISION_FLAG_GROUND  = 1 << 0,
    COLLISION_FLAG_WHEEL   = 1 << 1,
    COLLISION_FLAG_CHASSIS = 1 << 2,
};

enum
{
    DRIVABLE_SURFACE = 0xffff0000,
    UNDRIVABLE_SURFACE = 0x0000ffff
};

enum
{
    SURFACE_TYPE_TARMAC,
    SURFACE_TYPE_SAND,
    MAX_NUM_SURFACE_TYPES
};

struct StaticEntity
{
    u32 renderHandle;
    glm::mat4 worldTransform;
};

class Scene
{
private:
    std::vector<StaticEntity> staticEntities;
    std::vector<class Vehicle*> vehicles;

    bool physicsDebugVisualizationEnabled = false;
    PxScene* physicsScene;

    PxMaterial* vehicleMaterial;
    PxMaterial* trackMaterial;
    PxMaterial* offroadMaterial;

    TrackGraph trackGraph;

public:
    Scene(const char* name);
    ~Scene();

    void onStart();
    void onEnd();
    void onUpdate(f32 deltaTime);
};
