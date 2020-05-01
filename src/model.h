#pragma once

#include "math.h"
#include "datafile.h"
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
    std::string name;
    i64 materialGuid = 0;
    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 scale;
    u32 meshIndex;
    bool isCollider = false;
    bool isVisible = true;
    glm::vec3 bounds;
    std::vector<std::string> collections;

    PxShape* mousePickShape = nullptr;

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
        s.field(collections);
    }

    void createMousePickCollisionShape(class Model* model);
    glm::mat4 getTransform() const
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.f), position)
            * glm::mat4_cast(rotation)
            * glm::scale(glm::mat4(1.f), scale);
        return transform;
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

class Model
{
public:
    // serialized
    i64 guid;
    std::string name;
    std::string sourceFilePath;
    std::string sourceSceneName;
    std::vector<Mesh> meshes;
    std::vector<ModelObject> objects;
    ModelUsage modelUsage = ModelUsage::STATIC_PROP;
    f32 density = 150.f;
    PropCategory category = PropCategory::NOT_NATURE;

    void serialize(Serializer& s);
    ModelObject* getObjByName(const char* name)
    {
        for (auto& obj : objects)
        {
            if (obj.name == name)
            {
                return &obj;
            }
        }
        FATAL_ERROR("Cannot find object with name \"", name, "\" in model \"", this->name, "\"");
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
        FATAL_ERROR("Cannot find mesh with name \"", name, "\" in model \"", this->name, "\"");
    }
    BoundingBox getBoundingbox(glm::mat4 const& m)
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
