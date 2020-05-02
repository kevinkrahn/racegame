#pragma once

#include "math.h"
#include "datafile.h"
#include "model.h"
#include <functional>

struct ActorUserData
{
    enum
    {
        NONE,
        TRACK,
        SCENERY,
        VEHICLE,
        ENTITY,
        SELECTABLE_ENTITY,
    };
    u32 entityType = NONE;
    enum
    {
        BOOSTER = 1 << 0
    };
    u32 flags = 0;
    union
    {
        class Vehicle* vehicle;
        class Entity* entity;
        class PlaceableEntity* placeableEntity;
    };
};

#define UNSET_ENTITY_ID 0xF0F0F0F0
class Entity
{
public:
    u32 entityID = UNSET_ENTITY_ID;
    enum EntityFlags
    {
        NONE = 0,
        DESTROYED = 1,
        PERSISTENT = 1 << 1,
        DYNAMIC = 1 << 2,
        TRANSIENT = 1 << 3,
        PROP = 1 << 4,
    };
    u32 entityFlags = NONE;

    void destroy() { entityFlags |= DESTROYED; }
    bool isDestroyed() { return entityFlags & DESTROYED; }
    void setPersistent(bool persistent)
    {
        entityFlags = persistent ? (entityFlags | PERSISTENT) : (entityFlags & ~PERSISTENT);
    }
    bool isPersistent() const { return (entityFlags & PERSISTENT) == PERSISTENT; }

    virtual ~Entity() {}
    virtual void serializeState(Serializer& s) {}
    void serialize(Serializer& s)
    {
        assert(entityID != UNSET_ENTITY_ID);
        s.field(entityID);
        serializeState(s);
    }
    virtual void onTrigger(ActorUserData* userData) {}

    virtual void onCreate(class Scene* scene) {}
    virtual void onCreateEnd(class Scene* scene) {}
    virtual void onUpdate(class RenderWorld* rw, class Scene* scene, f32 deltaTime) {}
    virtual void onRender(class RenderWorld* rw, class Scene* scene, f32 deltaTime) {}
    virtual void onBatch(class Batcher& batcher) {}

    virtual void applyDecal(class Decal& decal) {}

    virtual void onEditModeRender(class RenderWorld* rw, class Scene* scene, bool isSelected) {}
};

struct PropPrefabData
{
    PropCategory category;
    std::string name;
    std::function<void(PlaceableEntity*)> doThing = [](auto){};
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

    virtual void serializeState(Serializer& s) override
    {
        s.field(position);
        s.field(rotation);
        s.field(scale);
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
    virtual const char* getName() const { return "Entity"; }
    virtual void showDetails(Scene* scene) {}
    virtual std::vector<PropPrefabData> generatePrefabProps() { return {}; }

    virtual void onPreview(class RenderWorld* rw) {}

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
    std::function<class Entity*()> create;
    bool isPlaceableInEditor;
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
            if (std::is_base_of<PlaceableEntity, T>::value)
            {
                e->entityFlags |= Entity::PROP;
            }
            return e;
        },
        std::is_base_of<PlaceableEntity, T>::value,
    });
}

void registerEntities();

