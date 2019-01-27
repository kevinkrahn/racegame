#pragma once

#include "datafile.h"
#include "math.h"
#include "font.h"
#include "audio.h"
#include "vehicle_data.h"
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
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};

struct Texture
{
    enum struct Format
    {
        RGBA8,
        R8,
    };
    Format format;
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
    std::map<std::string, std::map<u32, Font>> fonts;
    std::map<std::string, std::unique_ptr<Sound>> sounds;

    std::map<std::string, PxTriangleMesh*> collisionMeshCache;
    std::map<std::string, PxConvexMesh*> convexCollisionMeshCache;

    std::vector<VehicleData> vehicleData;

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
    PxConvexMesh* getConvexCollisionMesh(std::string const& name);

    Font& getFont(const char* name, u32 height)
    {
        auto iter = fonts.find(name);
        if (iter == fonts.end())
        {
            return fonts[name][height] = Font(std::string(name) + ".ttf", (f32)height);
        }

        auto iter2 = iter->second.find(height);
        if (iter2 == iter->second.end())
        {
            return fonts[name][height] = Font(std::string(name) + ".ttf", (f32)height);
        }

        return iter2->second;
    }

    Texture& getTexture(const char* name)
    {
        auto iter = textures.find(name);
        if (iter == textures.end())
        {
            FATAL_ERROR("Texture not found: ", name);
        }
        return iter->second;
    }

    Sound* getSound(const char* name)
    {
        auto iter = sounds.find(name);
        if (iter == sounds.end())
        {
            FATAL_ERROR("Sound not found: ", name);
        }
        return iter->second.get();
    }

    std::vector<VehicleData>& getVehicleData() { return vehicleData; }
};
