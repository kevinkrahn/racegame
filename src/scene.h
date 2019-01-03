#pragma once

#include "misc.h"
#include "math.h"
#include "track_graph.h"
#include <vector>

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

struct ActorUserData
{
    enum
    {
        TRACK,
        SCENERY,
        VEHICLE
    };
    u32 entityType;
    union
    {
        u32 vehicleIndex;
    };
};

struct StaticEntity
{
    u32 renderHandle;
    glm::mat4 worldTransform;
    ActorUserData userData;
};

struct VehicleDebris
{
    PxRigidDynamic* rigidBody;
    u32 renderHandle;
    f32 life = 0.f;
};

struct Projectile
{
    glm::vec3 position;
    glm::vec3 velocity;
    u32 instigator;
};

class Scene
{
    glm::mat4 start;
    u32 totalLaps = 4;

    std::vector<StaticEntity> staticEntities;
    std::vector<VehicleDebris> vehicleDebris;
    SmallVec<std::unique_ptr<class Vehicle>> vehicles;
    SmallVec<u32> finishOrder;
    SmallVec<std::vector<glm::vec3>> paths;
    std::vector<Projectile> projectiles;

    bool physicsDebugVisualizationEnabled = false;
    bool trackGraphDebugVisualizationEnabled = false;
    PxScene* physicsScene = nullptr;

    TrackGraph trackGraph;

public:
    RandomSeries randomSeries;
    PxMaterial* vehicleMaterial = nullptr;
    PxMaterial* trackMaterial = nullptr;
    PxMaterial* offroadMaterial = nullptr;

    Scene(const char* name);
    ~Scene();

    void onStart();
    void onEnd();
    void onUpdate(f32 deltaTime);

    void vehicleFinish(u32 n) { finishOrder.push_back(n); }

    glm::mat4 const& getStart() const { return start; }
    PxScene* const& getPhysicsScene() const { return physicsScene; }
    TrackGraph const& getTrackGraph() const { return trackGraph; }
    SmallVec<std::vector<glm::vec3>> const& getPaths() const { return paths; }
    u32 getTotalLaps() const { return totalLaps; }

    bool raycastStatic(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit=nullptr) const;
    bool raycast(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit=nullptr);

    void createVehicleDebris(VehicleDebris const& debris) { vehicleDebris.push_back(debris); }
    void createProjectile(glm::vec3 const& position, glm::vec3 const& velocity, u32 instigator)
    {
        projectiles.push_back({ position, velocity, instigator });
    };
};
