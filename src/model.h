#pragma once

#include "math.h"
#include "datafile.h"
#include "mesh.h"

enum struct CollisionType
{
    NONE,
    CONVEX_HULL,
    CUBE,
    SPHERE,
    CAPSULE
};

struct CollisionShape
{
    CollisionType collisionType;
    glm::quat rotation;
    glm::vec3 offset;
    glm::vec3 scale;

    void serialize(Serializer& s)
    {
        s.field(collisionType);
        s.field(rotation);
        s.field(offset);
        s.field(scale);
    }
};

class ModelObject
{
public:
    std::string name;
    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 scale;
    u32 meshIndex;
    CollisionShape collisionShape;

    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(rotation);
        s.field(position);
        s.field(scale);
        s.field(meshIndex);
        s.field(collisionShape);
    }
};

class Model
{
public:
    i64 guid;
    std::string name;
    std::string sourceFilePath;
    std::string sourceSceneName;
    std::vector<Mesh> meshes;
    std::vector<ModelObject> objects;

    void serialize(Serializer& s);
};
