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
#include "entities/start.h"
#include "collision_flags.h"
#include <vector>

struct RaceStatistics
{
    i32 attackBonuses = 0;
    i32 lappingBonuses = 0;
    i32 accidents = 0;
    i32 destroyed = 0;
};

struct RaceResult
{
    i32 placement;
    Driver* driver;
    RaceStatistics statistics;
    bool finishedRace;

    i32 getBonus() const
    {
        return statistics.attackBonuses * 110 + statistics.lappingBonuses * 300;
    }

    i32 getLeaguePointsEarned() const
    {
        return finishedRace ? (glm::max((10 - (i32)placement), 0) * 100) : 0;
    }

    i32 getCreditsEarned() const
    {
        return getLeaguePointsEarned() + getBonus();
    }
};

class Scene : public PxSimulationEventCallback
{
private:
    Editor editor;

    std::vector<std::unique_ptr<Entity>> entities;
    std::vector<std::unique_ptr<Entity>> newEntities;

    std::vector<u32> finishOrder;
    std::vector<std::unique_ptr<class Vehicle>> vehicles;
    std::vector<u32> placements;
    std::vector<RaceResult> raceResults;

    bool debugCamera = false;
    glm::vec3 debugCameraPosition;
    PxScene* physicsScene = nullptr;
    TrackPreview2D trackPreview2D;
    TrackGraph trackGraph;
    u32 totalLaps = 4;
    glm::vec3 trackPreviewCameraFrom = { 0, 0, 0 };
    glm::vec3 trackPreviewCameraTarget = { 0, 0, 0 };
    glm::vec3 trackPreviewPosition = { 0, 0, 0 };
    glm::vec3 trackPreviewVelocity = { 0, 0, 0 };
    u32 currentTrackPreviewPoint = 0;
    f64 worldTime = 0.0;
    bool readyToGo = false;
    u32 numHumanDrivers = 0;

    bool allPlayersFinished = false;
    f32 finishTimer = 0.f;

    // physx callbacks
    void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count)  { PX_UNUSED(constraints); PX_UNUSED(count); }
    void onWake(PxActor** actors, PxU32 count)                          { PX_UNUSED(actors); PX_UNUSED(count); }
    void onSleep(PxActor** actors, PxU32 count)                         { PX_UNUSED(actors); PX_UNUSED(count); }
    void onTrigger(PxTriggerPair* pairs, PxU32 count);
    void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) {}
    void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs);

    void buildRaceResults();

public:
    std::string name;
    std::string filename;

    bool isDebugOverlayEnabled = false;
    bool isPhysicsDebugVisualizationEnabled = false;
    bool isTrackGraphDebugVisualizationEnabled = false;
    bool isRaceInProgress = false;
    bool isPaused = false;
    bool isCameraTourEnabled = true;

    RandomSeries randomSeries;
    PxMaterial* vehicleMaterial = nullptr;
    PxMaterial* trackMaterial = nullptr;
    PxMaterial* offroadMaterial = nullptr;
    PxMaterial* genericMaterial = nullptr;
    SmokeParticles smoke;
    RibbonRenderable ribbons;
    DebugDraw debugDraw;
    Terrain* terrain = nullptr;
    Track* track = nullptr;
    Start* start = nullptr;

    Scene(const char* name);
    ~Scene();

    f64 getWorldTime() const { return worldTime; }

    void startRace();
    void stopRace();

    DataFile::Value serialize();
    void deserialize(DataFile::Value& data);
    Entity* deserializeEntity(DataFile::Value& data);

    void onStart();
    void onEnd();
    void onUpdate(class Renderer* renderer, f32 deltaTime);

    void vehicleFinish(u32 n);
    Vehicle* getVehicle(u32 n) const { return vehicles.size() > n ? vehicles[n].get() : nullptr; }
    void attackCredit(u32 instigator, u32 victim);
    void applyAreaForce(glm::vec3 const& position, f32 strength) const;
    void createExplosion(glm::vec3 const& position, glm::vec3 const& velocity, f32 strength);
    u32 getNumHumanDrivers() const { return numHumanDrivers; }
    void drawTrackPreview(Renderer* renderer, u32 size, glm::vec2 pos);
    TrackPreview2D& getTrackPreview2D() { return trackPreview2D; }
    std::vector<RaceResult>& getRaceResults() { return raceResults; }

    glm::mat4 getStart() const { return start->transform; }
    PxScene* const& getPhysicsScene() const { return physicsScene; }
    TrackGraph const& getTrackGraph() const { return trackGraph; }
    u32 getTotalLaps() const { return totalLaps; }
    bool canGo() const { return readyToGo; }

    bool raycastStatic(glm::vec3 const& from, glm::vec3 const& dir, f32 dist,
            PxRaycastBuffer* hit=nullptr, u32 flags=COLLISION_FLAG_GROUND | COLLISION_FLAG_TRACK) const;
    bool raycast(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit=nullptr);
    bool sweepStatic(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist,
            PxSweepBuffer* hit=nullptr, u32 flags=COLLISION_FLAG_GROUND | COLLISION_FLAG_TRACK) const;
    bool sweep(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist,
            PxSweepBuffer* hit=nullptr, PxRigidActor* ignore=nullptr,
            u32 flags=COLLISION_FLAG_GROUND | COLLISION_FLAG_CHASSIS) const;

    void addEntity(Entity* entity) { newEntities.push_back(std::unique_ptr<Entity>(entity)); }
    std::vector<std::unique_ptr<Entity>>& getEntities() { return entities; }
    std::vector<std::unique_ptr<Vehicle>>& getVehicles() { return vehicles; }
};
