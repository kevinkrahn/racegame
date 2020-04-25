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
    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 scale;
    u32 meshIndex;
    bool isCollider = false;
    i64 materialGuid = 0;

    PxShape* mousePickShape = nullptr;

    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(rotation);
        s.field(position);
        s.field(scale);
        s.field(meshIndex);
        s.field(isCollider);
        s.field(materialGuid);
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
    bool isPlaceableObject = true;
    bool isDynamic = false;
    f32 density = 150.f;

    void serialize(Serializer& s);
};
