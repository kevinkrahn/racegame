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
    glm::mat4 start;
    u32 totalLaps = 4;

    std::vector<StaticEntity> staticEntities;
    SmallVec<std::unique_ptr<class Vehicle>> vehicles;
    SmallVec<u32> finishOrder;
    SmallVec<std::vector<glm::vec3>> paths;

    bool physicsDebugVisualizationEnabled = false;
    bool trackGraphDebugVisualizationEnabled = false;
    PxScene* physicsScene = nullptr;

    PxMaterial* vehicleMaterial = nullptr;
    PxMaterial* trackMaterial = nullptr;
    PxMaterial* offroadMaterial = nullptr;

    TrackGraph trackGraph;

public:
    Scene(const char* name);
    ~Scene();

    void onStart();
    void onEnd();
    void onUpdate(f32 deltaTime);

    glm::mat4 const& getStart() const { return start; }
    PxScene* const& getPhysicsScene() const { return physicsScene; }
    TrackGraph const& getTrackGraph() const { return trackGraph; }
    SmallVec<std::vector<glm::vec3>> const& getPaths() const { return paths; }
    u32 getTotalLaps() const { return totalLaps; }
};
