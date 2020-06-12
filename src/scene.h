#pragma once

#include "math.h"
#include "track_graph.h"
#include "motion_grid.h"
#include "entity.h"
#include "ribbon.h"
#include "particle_system.h"
#include "debug_draw.h"
#include "driver.h"
#include "editor/editor_camera.h"
#include "2d.h"
#include "terrain.h"
#include "track.h"
#include "datafile.h"
#include "entities/start.h"
#include "collision_flags.h"
#include "racing_line.h"
#include "batcher.h"
#include "track_preview.h"

struct RaceBonus
{
    const char* name;
    u32 amount;
};

enum Bonuses {
    ATTACK_BONUS_AMOUNT = 110,
    LAPPING_BONUS_AMOUNT = 300,
    PICKUP_BONUS_AMOUNT = 150,
    AIR_BONUS_AMOUNT = 200,
    BIG_AIR_BONUS_AMOUNT = 350,
    VICTIM_BONUS_AMOUNT = 400,
};

struct RaceStatistics
{
    i32 frags = 0;
    i32 accidents = 0;
    i32 destroyed = 0;
    Array<RaceBonus> bonuses;
};

struct RaceResult
{
    i32 placement;
    Driver* driver;
    RaceStatistics statistics;
    bool finishedRace;

    i32 getBonus() const
    {
        i32 amount = 0;
        for (auto& bonus : statistics.bonuses)
        {
            amount += (i32)bonus.amount;
        }
        return amount;
    }

    i32 getLeaguePointsEarned() const
    {
        // TODO: have bonus points also affect league points earned
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
    EditorCamera editorCamera;

    Array<RacingLine> paths;
    bool hasGeneratedPaths = false;

    Array<OwnedPtr<Entity>> entities;
    Array<OwnedPtr<Entity>> newEntities;

    Array<u32> finishOrder;
    Array<OwnedPtr<class Vehicle>> vehicles;
    Array<u32> placements;
    Array<RaceResult> raceResults;

    PxScene* physicsScene = nullptr;
    TrackPreview2D trackPreview2D;
    glm::vec3 trackPreviewCameraFrom = { 0, 0, 0 };
    glm::vec3 trackPreviewCameraTarget = { 0, 0, 0 };
    glm::vec3 trackPreviewPosition = { 0, 0, 0 };
    glm::vec3 trackPreviewVelocity = { 0, 0, 0 };
    u32 currentTrackPreviewPoint = 0;
    f64 worldTime = 0.0;
    bool readyToGo = false;
    u32 numHumanDrivers = 0;
    TrackGraph trackGraph;
    MotionGrid motionGrid;
    PxDistanceJoint* dragJoint = nullptr;
    Batcher batcher;
    bool hasTrackPreview = false;

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
    void physicsMouseDrag(Renderer* renderer);

public:
    i64 guid = 0;
    std::string name;
    std::string notes;
    u32 totalLaps = 4;
    u32 version = 0;

    bool isRaceInProgress = false;
    bool isPaused = false;
    bool isCameraTourEnabled = true;
    bool isBatched = false;

    RandomSeries randomSeries;
    SoundHandle backgroundSound = 0;
    ParticleSystem smoke;
    ParticleSystem sparks;
    RibbonRenderer ribbons;
    DebugDraw debugDraw;
    Terrain* terrain = nullptr;
    Track* track = nullptr;
    Start* start = nullptr;

    Scene(TrackData* data=nullptr);
    ~Scene();

    f64 getWorldTime() const { return worldTime; }

    void startRace();
    void stopRace();
    void setPaused(bool paused);
    void forfeitRace();

    void serialize(Serializer& s);
    Entity* deserializeEntity(DataFile::Value& data);

    Array<DataFile::Value> serializeTransientEntities();
    void deserializeTransientEntities(Array<DataFile::Value>& entities);

    void onStart();
    void onEnd();
    void onUpdate(class Renderer* renderer, f32 deltaTime);
    void onEndUpdate();

    void buildBatches();
    void vehicleFinish(u32 n);
    Vehicle* getVehicle(u32 n) const { return vehicles.size() > n ? vehicles[n].get() : nullptr; }
    void attackCredit(u32 instigator, u32 victim);
    void applyAreaForce(glm::vec3 const& position, f32 strength) const;
    void createExplosion(glm::vec3 const& position, glm::vec3 const& velocity, f32 strength);
    u32 getNumHumanDrivers() const { return numHumanDrivers; }
    void updateTrackPreview(Renderer* renderer, u32 size);
    TrackPreview2D& getTrackPreview2D() { return trackPreview2D; }
    Array<RaceResult>& getRaceResults() { return raceResults; }
    EditorCamera& getEditorCamera() { return editorCamera; }
    Array<RacingLine>& getPaths() { return paths; }
    void showDebugInfo();
    glm::mat4 getStart() const { return start->transform; }
    PxScene* const& getPhysicsScene() const { return physicsScene; }
    TrackGraph& getTrackGraph() { return trackGraph; }
    MotionGrid& getMotionGrid() { return motionGrid; }
    u32 getTotalLaps() const { return totalLaps; }
    bool canGo() const;
    glm::vec3 findValidPosition(glm::vec3 const& pos, f32 collisionRadius, f32 checkRadius=10.f);

    bool raycastStatic(glm::vec3 const& from, glm::vec3 const& dir, f32 dist,
            PxRaycastBuffer* hit=nullptr, u32 flags=COLLISION_FLAG_TERRAIN | COLLISION_FLAG_OBJECT | COLLISION_FLAG_TRACK) const;
    bool raycast(glm::vec3 const& from, glm::vec3 const& dir, f32 dist, PxRaycastBuffer* hit=nullptr);
    bool sweepStatic(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist,
            PxSweepBuffer* hit=nullptr, u32 flags=COLLISION_FLAG_TERRAIN | COLLISION_FLAG_OBJECT | COLLISION_FLAG_TRACK) const;
    bool sweep(f32 radius, glm::vec3 const& from, glm::vec3 const& dir, f32 dist,
            PxSweepBuffer* hit=nullptr, PxRigidActor* ignore=nullptr,
            u32 flags=COLLISION_FLAG_TERRAIN | COLLISION_FLAG_OBJECT | COLLISION_FLAG_CHASSIS) const;

    void addEntity(Entity* entity) { newEntities.push_back(OwnedPtr<Entity>(entity)); }
    Array<OwnedPtr<Entity>>& getEntities() { return entities; }
    Array<OwnedPtr<Vehicle>>& getVehicles() { return vehicles; }

    void writeTrackData();
};
