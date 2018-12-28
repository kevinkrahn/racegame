#pragma once

#include "datafile.h"
#include "math.h"
#include <vector>
#include <map>
#include <string>

struct Mesh
{
    std::vector<f32> vertices;
    std::vector<u32> indices;
    u32 numVertices;
    u32 numIndices;
    u32 numColors;
    u32 numTexCoords;
    u32 elementSize;
    u32 stride;
    u32 renderHandle;
};

struct Texture
{
    u8* data;
    u32 size;
    u32 width;
    u32 height;
    u32 renderHandle;
};

class Resources
{
private:
    std::map<std::string, Mesh> meshes;
    std::map<std::string, DataFile::Value::Dict> scenes;
    std::map<std::string, Texture> textures;

    std::map<std::string, PxTriangleMesh*> collisionMeshCache;

public:
    void load();

    Mesh const& getMesh(const char* name) const
    {
        auto iter = meshes.find(name);
        if (iter == meshes.end())
        {
            FATAL_ERROR("Mesh not found: ", name);
        }
        return iter->second;
    }

    DataFile::Value::Dict& getScene(const char* name)
    {
        auto iter = scenes.find(name);
        if (iter == scenes.end())
        {
            FATAL_ERROR("Scene not found: ", name);
        }
        return iter->second;
    }

    PxTriangleMesh* getCollisionMesh(std::string const& name);
};
