#pragma once

#include "misc.h"
#include "math.h"
#include "track_graph.h"
#include "material.h"
#include "entity.h"
#include "ribbon.h"
#include "smoke_particles.h"
#include "debug_draw.h"
#include <vector>

enum
{
    COLLISION_FLAG_TRACK   = 1 << 0,
    COLLISION_FLAG_GROUND  = 1 << 1,
    COLLISION_FLAG_WHEEL   = 1 << 2,
    COLLISION_FLAG_CHASSIS = 1 << 3,
    COLLISION_FLAG_DEBRIS  = 1 << 4,
};

enum
{
    DRIVABLE_SURFACE = 0xffff0000,
    UNDRIVABLE_SURFACE = 0x0000ffff
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
        class Vehicle* vehicle;
    };
};

struct StaticEntity
{
    Mesh* mesh;
    glm::mat4 worldTransform;
    Material* material = nullptr;
    bool isTrack = false;
};

struct VehicleDebris
{
    PxRigidDynamic* rigidBody;
    Mesh* mesh;
    f32 life = 0.f;
    glm::vec3 color;
    Material* material = nullptr;
};

struct Projectile
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 upVector;
    u32 instigator;
};

class Scene : public PxSimulationEventCallback
{
private:
    glm::mat4 start;
    u32 totalLaps = 4;

    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> newEntities;

    std::vector<StaticEntity> staticEntities;
    std::vector<std::unique_ptr<ActorUserData>> physicsUserData;
    std::vector<VehicleDebris> vehicleDebris;
    SmallVec<std::unique_ptr<class Vehicle>> vehicles;
    SmallVec<u32> finishOrder;
    SmallVec<std::vector<glm::vec3>> paths;
    std::vector<Projectile> projectiles;

    bool debugCamera = false;
    glm::vec3 debugCameraPosition;
    bool isPhysicsDebugVisualizationEnabled = false;
    bool isTrackGraphDebugVisualizationEnabled = false;
    bool isDebugOverlayEnabled = false;
    PxScene* physicsScene = nullptr;

    glm::mat4 trackOrtho;
    std::vector<RenderTextureItem> trackItems;
    f32 trackAspectRatio;

    TrackGraph trackGraph;

    // physx callbacks
    void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)  { PX_UNUSED(constraints); PX_UNUSED(count); }
    void onWake(PxActor** actors, PxU32 count)                          { PX_UNUSED(actors); PX_UNUSED(count); }
    void onSleep(PxActor** actors, PxU32 count)                         { PX_UNUSED(actors); PX_UNUSED(count); }
    void onTrigger(PxTriggerPair* pairs, PxU32 count)                   { PX_UNUSED(pairs); PX_UNUSED(count); }
    void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) {}
    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs);

public:
    RandomSeries randomSeries;
    PxMaterial* vehicleMaterial = nullptr;
    PxMaterial* trackMaterial = nullptr;
    PxMaterial* offroadMaterial = nullptr;
    SmokeParticles smoke;
    RibbonRenderable ribbons;
    DebugDraw debugDraw;

    Scene(const char* name);
    ~Scene();

    void onStart();
    void onEnd();
    void onUpdate(class Renderer* renderer, f32 deltaTime);
    void onEndUpdate(f32 deltaTime);

    void vehicleFinish(u32 n) { finishOrder.push_back(n); }

    glm::mat4 const& getStart() const { return start; }
    PxScene* const& getPhysicsScene() const { return physicsScene; }
    TrackGraph const& getTrackGraph() const { return trackGraph; }
    SmallVec<std::vector<glm::vec3>> const& getPaths() const { return paths; }
    u32 getTotalLaps() const { return totalLaps; }

    bool raycastStatic(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit=nullptr) const;
    bool raycast(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit=nullptr);
    bool sweepStatic(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxSweepBuffer* hit=nullptr) const;
    bool sweep(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxSweepBuffer* hit=nullptr, PxRigidActor* ignore=nullptr) const;

    void createVehicleDebris(VehicleDebris const& debris) { vehicleDebris.push_back(debris); }
    void createProjectile(glm::vec3 const& position, glm::vec3 const& velocity, glm::vec3 const& upVector, u32 instigator)
    {
        projectiles.push_back({ position, velocity, upVector, instigator });
    };

    void attackCredit(u32 instigator, u32 victim);

    template <typename T>
    Entity* spawnEntity() {
        newEntities.push_back(std::unique_ptr<Entity>(new T()));
        return newEntities.back().get();
    }

    void destroyEntity(Entity* e) {
        e->isMarkedForDeletion = true;
    }
};
