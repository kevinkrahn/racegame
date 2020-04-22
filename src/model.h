#pragma once

#include "math.h"
#include "datafile.h"
#include "mesh.h"

class ModelObject
{
public:
    std::string name;
    glm::quat rotation;
    glm::vec3 position;
    glm::vec3 scale;

    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(rotation);
        s.field(position);
        s.field(scale);
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
