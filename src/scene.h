#pragma once

#include "math.h"
#include "track_graph.h"
#include "entity.h"
#include "ribbon.h"
#include "smoke_particles.h"
#include "debug_draw.h"
#include "driver.h"
#include "editor.h"
#include "2d.h"
#include "terrain.h"
#include "track.h"
#include "datafile.h"
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

class Scene : public PxSimulationEventCallback
{
private:
    bool isEditing = true;
    Editor editor;

    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> newEntities;

    std::vector<u32> finishOrder;
    std::vector<std::unique_ptr<class Vehicle>> vehicles;
    std::vector<u32> placements;

    bool debugCamera = false;
    glm::vec3 debugCameraPosition;
    PxScene* physicsScene = nullptr;
    TrackPreview2D trackPreview2D;
    TrackGraph trackGraph;
    u32 totalLaps = 4;

    // physx callbacks
    void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)  { PX_UNUSED(constraints); PX_UNUSED(count); }
    void onWake(PxActor** actors, PxU32 count)                          { PX_UNUSED(actors); PX_UNUSED(count); }
    void onSleep(PxActor** actors, PxU32 count)                         { PX_UNUSED(actors); PX_UNUSED(count); }
    void onTrigger(PxTriggerPair* pairs, PxU32 count)                   { PX_UNUSED(pairs); PX_UNUSED(count); }
    void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) {}
    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs);

public:
    bool isDebugOverlayEnabled = false;
    bool isPhysicsDebugVisualizationEnabled = false;
    bool isTrackGraphDebugVisualizationEnabled = false;
    bool isRaceInProgress = false;

    RandomSeries randomSeries;
    PxMaterial* vehicleMaterial = nullptr;
    PxMaterial* trackMaterial = nullptr;
    PxMaterial* offroadMaterial = nullptr;
    SmokeParticles smoke;
    RibbonRenderable ribbons;
    DebugDraw debugDraw;
    Terrain* terrain = nullptr;
    Track* track = nullptr;

    Scene(const char* name);
    ~Scene();

    void startRace(glm::mat4 const& start);
    void stopRace();

    DataFile::Value serialize();
    void deserialize(DataFile::Value& data);

    void onStart();
    void onEnd();
    void onUpdate(class Renderer* renderer, f32 deltaTime);
    void onEndUpdate(f32 deltaTime);

    void vehicleFinish(u32 n) { finishOrder.push_back(n); }
    Vehicle* getVehicle(u32 n) const { return vehicles.size() > n ? vehicles[n].get() : nullptr; }
    void attackCredit(u32 instigator, u32 victim);

    glm::mat4 getStart() const { return track->getStart(); }
    PxScene* const& getPhysicsScene() const { return physicsScene; }
    TrackGraph const& getTrackGraph() const { return trackGraph; }
    u32 getTotalLaps() const { return totalLaps; }

    bool raycastStatic(glm::vec3 const& from, glm::vec3 const& dir, f32 dist,
            PxRaycastBuffer* hit=nullptr, u32 flags=COLLISION_FLAG_GROUND | COLLISION_FLAG_TRACK) const;
    bool raycast(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit=nullptr);
    bool sweepStatic(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist,
            PxSweepBuffer* hit=nullptr, u32 flags=COLLISION_FLAG_GROUND | COLLISION_FLAG_TRACK) const;
    bool sweep(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxSweepBuffer* hit=nullptr, PxRigidActor* ignore=nullptr) const;

    void addEntity(Entity* entity) { newEntities.push_back(std::unique_ptr<Entity>(entity)); }
};
