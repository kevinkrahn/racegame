#pragma once

#include "math.h"
#include "resource.h"
#include "mesh.h"

// TODO: implement these collision shapes
enum struct CollisionType
{
    NONE,
    CONVEX_HULL,
    CUBE,
    SPHERE,
    CAPSULE
};

class ModelObject
{
public:
    // serialized
    Str64 name;
    i64 materialGuid = 0;
    Quat rotation;
    Vec3 position;
    Vec3 scale;
    u32 meshIndex;
    bool isCollider = false;
    bool isVisible = true;
    Vec3 bounds;
    Array<u32> collectionIndexes;

    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(rotation);
        s.field(position);
        s.field(scale);
        s.field(meshIndex);
        s.field(isCollider);
        s.field(isVisible);
        s.field(materialGuid);
        s.field(bounds);
        s.field(collectionIndexes);
    }

    Mat4 getTransform() const
    {
        return Mat4::translation(position) * Mat4(rotation) * Mat4::scaling(scale);
    }
};

enum struct ModelUsage
{
    INTERNAL = 0,
    STATIC_PROP = 1,
    DYNAMIC_PROP = 2,
    SPLINE = 3,
    VEHICLE = 4,
};

enum struct PropCategory
{
    NATURE,
    NOT_NATURE,
    OBSTACLES,
    DECALS,
    PICKUPS,
    RACE,
    MAX
};

const char* propCategoryNames[] = {
    "Nature",
    "Not Nature",
    "Obstacles",
    "Decals",
    "Pickups",
    "Race",
};

struct ModelObjectCollection
{
    Str64 name;
    bool isVisible = true;

    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(isVisible);
    }
};

class Model : public Resource
{
public:
    // serialized
    Str64 sourceFilePath;
    Str64 sourceSceneName;
    Array<Mesh> meshes;
    Array<ModelObject> objects;
    Array<ModelObjectCollection> collections;
    ModelUsage modelUsage = ModelUsage::STATIC_PROP;
    f32 density = 150.f;
    PropCategory category = PropCategory::NOT_NATURE;

    void serialize(Serializer& s) override
    {
        Resource::serialize(s);

        s.field(sourceFilePath);
        s.field(sourceSceneName);
        s.field(meshes);
        s.field(objects);
        s.field(collections);
        s.field(modelUsage);
        s.field(density);
        s.field(category);
    }
    ModelObject* getObjByName(const char* name)
    {
        for (auto& obj : objects)
        {
            if (obj.name == name)
            {
                return &obj;
            }
        }
        FATAL_ERROR("Cannot find object with name \"%s\" in model \"%s\"", name, this->name.data());
    }
    Mesh* getMeshByName(const char* name)
    {
        for (auto& mesh : meshes)
        {
            if (mesh.name == name)
            {
                return &mesh;
            }
        }
        FATAL_ERROR("Cannot find mesh with name \"%s\" in model \"%s\"", name, this->name.data());
    }
    BoundingBox getBoundingbox(Mat4 const& m)
    {
        BoundingBox bb;
        bb.min = { FLT_MAX, FLT_MAX, FLT_MAX };
        bb.max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

        for (auto& obj : objects)
        {
            bb = bb.growToFit(meshes[obj.meshIndex].aabb.transform(m * obj.getTransform()));
        }

        return bb;
    }
    bool hasCollision()
    {
        for (auto& obj : objects)
        {
            if (obj.isCollider)
            {
                return true;
            }
        }
        return false;
    }
};
