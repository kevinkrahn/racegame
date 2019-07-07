#pragma once

#include "math.h"
#include "track_graph.h"
#include "material.h"
#include "entity.h"
#include "ribbon.h"
#include "smoke_particles.h"
#include "debug_draw.h"
#include "driver.h"
#include "editor.h"
#include "2d.h"
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
        VEHICLE,
        ENTITY,
    };
    u32 entityType;
    union
    {
        class Vehicle* vehicle;
        Entity* entity;
    };
};

struct StaticEntity
{
    Mesh* mesh;
    glm::mat4 worldTransform;
    Material* material = nullptr;
    bool isTrack = false;
};

class Scene : public PxSimulationEventCallback
{
private:
    bool isEditing = true;
    Editor editor;
    class Terrain* terrain = nullptr;

    glm::mat4 start;
    u32 totalLaps = 4;

    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> newEntities;

    std::vector<StaticEntity> staticEntities;
    std::vector<std::unique_ptr<ActorUserData>> physicsUserData;
    SmallVec<u32, MAX_VEHICLES> finishOrder;
    SmallVec<std::vector<glm::vec3>> paths;
    SmallVec<std::unique_ptr<class Vehicle>, MAX_VEHICLES> vehicles;

    bool debugCamera = false;
    glm::vec3 debugCameraPosition;
    PxScene* physicsScene = nullptr;

    glm::mat4 trackOrtho;
    std::vector<TrackPreview2D::RenderItem> trackItems;
    TrackPreview2D trackPreview2D;
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
    bool isDebugOverlayEnabled = false;
    bool isPhysicsDebugVisualizationEnabled = false;
    bool isTrackGraphDebugVisualizationEnabled = false;

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
    Vehicle* getVehicle(u32 n) const { return vehicles[n].get(); }
    void attackCredit(u32 instigator, u32 victim);

    glm::mat4 const& getStart() const { return start; }
    PxScene* const& getPhysicsScene() const { return physicsScene; }
    TrackGraph const& getTrackGraph() const { return trackGraph; }
    SmallVec<std::vector<glm::vec3>> const& getPaths() const { return paths; }
    u32 getTotalLaps() const { return totalLaps; }

    bool raycastStatic(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit=nullptr) const;
    bool raycast(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit=nullptr);
    bool sweepStatic(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxSweepBuffer* hit=nullptr) const;
    bool sweep(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxSweepBuffer* hit=nullptr, PxRigidActor* ignore=nullptr) const;

    void addEntity(Entity* entity) { newEntities.push_back(std::unique_ptr<Entity>(entity)); }
};
