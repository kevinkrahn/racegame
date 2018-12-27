#pragma once

#include "misc.h"
#include <vector>
#include <map>
#include <string>

struct Mesh
{
    std::vector<f32> vertices;
    std::vector<u32> indices;
    u32 numColors;
    u32 numTexCoords;
    u32 elementSize;
    u32 stride;
    u32 renderHandle;
};

class Resources
{
private:
    std::map<std::string, Mesh> meshes;

public:
    void load();

    Mesh const& getMesh(const char* name) const {
        auto iter = meshes.find(name);
        if (iter == meshes.end()) FATAL_ERROR("Mesh no found: ", name);
        return iter->second;
    }
};
