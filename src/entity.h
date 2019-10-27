#pragma once

#include "math.h"
#include "datafile.h"
#include <functional>

struct ActorUserData
{
    enum
    {
        TRACK,
        SCENERY,
        VEHICLE,
        ENTITY,
        SELECTABLE_ENTITY,
    };
    u32 entityType;
    union
    {
        class Vehicle* vehicle;
        class Entity* entity;
        class PlaceableEntity* placeableEntity;
    };
};

class Entity
{
public:
    u32 entityID = 0xF0F0F0F0;
    enum EntityFlags
    {
        NONE = 0,
        DESTROYED = 1,
        PERSISTENT = 1 << 1,
    };
    u32 entityFlags = NONE;

    void destroy() { entityFlags |= DESTROYED; }
    bool isDestroyed() { return (entityFlags & DESTROYED) == DESTROYED; }
    void setPersistent(bool persistent)
    {
        entityFlags = persistent ? (entityFlags | PERSISTENT) : (entityFlags & ~PERSISTENT);
    }
    bool isPersistent() const { return (entityFlags & PERSISTENT) == PERSISTENT; }

    virtual ~Entity() {}
    virtual DataFile::Value serializeState() { return {}; }
    virtual void deserializeState(DataFile::Value& data) {}
    DataFile::Value serialize()
    {
        assert(entityID != 0xF0F0F0F0);
        auto val = serializeState();
        val["entityID"] = DataFile::makeInteger(entityID);
        return val;
    }
    virtual void onTrigger(ActorUserData* userData) {}

    virtual void onCreate(class Scene* scene) {}
    virtual void onCreateEnd(class Scene* scene) {}
    virtual void onUpdate(class RenderWorld* rw, class Scene* scene, f32 deltaTime) {}
    virtual void onRender(class RenderWorld* rw, class Scene* scene, f32 deltaTime) {}

    virtual void applyDecal(class Decal& decal) {}

    virtual void onEditModeRender(class RenderWorld* rw, class Scene* scene, bool isSelected) {}
};

class PlaceableEntity : public Entity
{
public:
    glm::vec3 position;
    glm::quat rotation = glm::identity<glm::quat>();
    glm::vec3 scale = glm::vec3(1.f);
    glm::mat4 transform;
    PxRigidActor* actor = nullptr;
    ActorUserData physicsUserData;

    virtual DataFile::Value serializeState() override
    {
        DataFile::Value dict = DataFile::makeDict();
        dict["position"] = DataFile::makeVec3(position);
        dict["rotation"] = DataFile::makeVec4({ rotation.x, rotation.y, rotation.z, rotation.w });
        dict["scale"] = DataFile::makeVec3(scale);
        return dict;
    }

    virtual void deserializeState(DataFile::Value& data) override
    {
        position = data["position"].vec3();
        glm::vec4 r = data["rotation"].vec4();
        rotation = glm::quat(r.w, r.x, r.y, r.z);
        scale = data["scale"].vec3();
    }

    virtual void updateTransform(class Scene* scene)
    {
        transform = glm::translate(glm::mat4(1.f), position)
            * glm::mat4_cast(rotation)
            * glm::scale(glm::mat4(1.f), scale);
        if (actor)
        {
            actor->setGlobalPose(PxTransform(convert(position), convert(rotation)));
            u32 numShapes = actor->getNbShapes();
            if (numShapes > 0)
            {
                for (u32 i=0; i<numShapes; ++i)
                {
                    PxShape* shape = nullptr;
                    actor->getShapes(&shape, 1, i);
                    PxTriangleMeshGeometry geom;
                    PxConvexMeshGeometry convexGeom;
                    if (shape->getTriangleMeshGeometry(geom))
                    {
                        geom.scale = convert(scale);
                        shape->setGeometry(geom);
                    }
                    else if (shape->getConvexMeshGeometry(convexGeom))
                    {
                        convexGeom.scale = convert(scale);
                        shape->setGeometry(convexGeom);
                    }
                }
            }
        }
    }
    virtual const char* getName() const { return "Unknown Entity"; }
    virtual void showDetails(Scene* scene) {}

    ~PlaceableEntity()
    {
        if (actor)
        {
            actor->release();
        }
    }
};

struct RegisteredEntity
{
    u32 entityID;
    std::function<Entity*()> create;
};

std::vector<RegisteredEntity> g_entities;

template<typename T>
void registerEntity()
{
    u32 entityID = (u32)g_entities.size();
    g_entities.push_back({
        entityID,
        [entityID] {
            Entity* e = new T();
            e->entityID = entityID;
            return e;
        }
    });
}

void registerEntities();

