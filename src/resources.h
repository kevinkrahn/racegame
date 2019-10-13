#pragma once

#include "datafile.h"
#include "math.h"
#include "font.h"
#include "audio.h"
#include "mesh.h"
#include "texture.h"
#include <vector>
#include <map>
#include <string>

class Resources
{
private:
    std::map<std::string, Mesh> meshes;
    std::map<std::string, Texture> textures;
    std::map<std::string, DataFile::Value::Dict> scenes;
    std::map<std::string, std::map<u32, Font>> fonts;
    std::map<std::string, std::unique_ptr<Sound>> sounds;

public:
    void load();

    Mesh* getMesh(const char* name)
    {
        auto iter = meshes.find(name);
        if (iter == meshes.end())
        {
            FATAL_ERROR("Mesh not found: ", name);
        }
        return &iter->second;
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

    Texture* getTexture(const char* name)
    {
        auto iter = textures.find(name);
        if (iter == textures.end())
        {
            FATAL_ERROR("Texture not found: ", name);
        }
        return &iter->second;
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
} g_resources;

