#pragma once

#include "math.h"
#include "datafile.h"
#include "model.h"

#if 0
enum EntityFlags
{
    ENTITY_NONE = 0,
    ENTITY_DESTROYED = 1,
    ENTITY_PERSISTENT = 1 << 1,
    ENTITY_DYNAMIC = 1 << 2,
    ENTITY_TRANSIENT = 1 << 3,
    ENTITY_PROP = 1 << 4,
};

struct ECS_Component
{
    struct ECS_Entity* entity;
    u32 componentID;

    virtual ~ECS_Component() {}
    virtual void serialize(Serializer& s);
};

struct ECS_Entity
{
    Str64 name;
    SmallArray<ECS_Component*, 8> components;
    Vec3 position;
    u32 flags = ENTITY_NONE;
    Quat rotation;
    Vec3 scale = Vec3(1.f);

    /*
    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(position);
        s.field(rotation);
        s.field(scale);
        s.field(flags);

        if (s.deserialize)
        {
            for (ECS_Component* component : components)
            {
                DataFile::Value componentData = DataFile::makeDict();
                Serializer componentSerializer(componentData, false);
                component->serialize(s);
            }
        }
        else
        {
            s.field(components);
        }
    }
    */
};
#endif

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

enum struct EntityFlags
{
    NONE = 0,
    DESTROYED = 1,
    PERSISTENT = 1 << 1,
    DYNAMIC = 1 << 2,
    TRANSIENT = 1 << 3,
    PROP = 1 << 4,
};
BITMASK_OPERATORS(EntityFlags);

#define UNSET_ENTITY_ID 0xF0F0F0F0
class Entity
{
public:
    u32 entityID = UNSET_ENTITY_ID;
    EntityFlags entityFlags = EntityFlags::NONE;
    u32 entityCounterID = 0;

    void destroy() { entityFlags |= EntityFlags::DESTROYED; }
    bool isDestroyed() { return (entityFlags & EntityFlags::DESTROYED) == EntityFlags::DESTROYED; }
    void setPersistent(bool persistent)
    {
        entityFlags = persistent ? (entityFlags | EntityFlags::PERSISTENT) : (entityFlags & ~EntityFlags::PERSISTENT);
    }
    bool isPersistent() const
    {
        return (entityFlags & EntityFlags::PERSISTENT) == EntityFlags::PERSISTENT;
    }

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

    virtual void onEditModeRender(class RenderWorld* rw, class Scene* scene, bool isSelected,
            u8 selectIndex=1) {}
};

struct PropPrefabData
{
    PropCategory category;
    Str64 name;
    std::function<void(PlaceableEntity*)> doThing = [](auto){};
};

class PlaceableEntity : public Entity
{
public:
    Vec3 position;
    Quat rotation;
    Vec3 scale = Vec3(1.f);
    Mat4 transform;
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
        transform = Mat4::translation(position) * Mat4(rotation) * Mat4::scaling(scale);
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
    virtual Array<PropPrefabData> generatePrefabProps() { return {}; }

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
    class Entity*(*create)(u32);
    bool isPlaceableInEditor;
};

Array<RegisteredEntity> g_entities;

u32 entityCounter = 0;
template<typename T>
void registerEntity()
{
    u32 entityID = (u32)g_entities.size();
    g_entities.push({
        entityID,
        [] (u32 entityID) {
            Entity* e = new T();
            e->entityID = entityID;
            if (std::is_base_of<PlaceableEntity, T>::value)
            {
                e->entityFlags |= EntityFlags::PROP;
            }
            e->entityCounterID = ++entityCounter;
            return e;
        },
        std::is_base_of<PlaceableEntity, T>::value,
    });
}

void registerEntities();

