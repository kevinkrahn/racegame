#pragma once

#include "datafile.h"
#include "math.h"
#include "font.h"
#include "resource.h"
#include "material.h"
#include "texture.h"
#include "model.h"
#include "audio.h"
#include "trackdata.h"
#include <vector>
#include <map>
#include <string>

const char* DATA_DIRECTORY = "../editor_data";
const char* ASSET_DIRECTORY = "../assets";
const char* METADATA_FILE = "metadata.dat";

class Resources
{
private:
    // TODO: convert fonts into resource
    std::map<const char*, std::map<u32, Font>> fonts;

public:
    std::map<i64, std::unique_ptr<Resource>> resources;
    std::map<std::string, Resource*> resourceNameMap;

    void addResource(std::unique_ptr<Resource>&& resource);

    Texture white;
    Texture identityNormal;
    Material defaultMaterial;

    i64 generateGUID()
    {
        static RandomSeries series = randomSeed();
        i64 guid = 0;
        do
        {
            u32 guidHalf[2] = { xorshift32(series), xorshift32(series) };
            guid = *((i64*)guidHalf);
        }
        while (guid == 0 || resources.find(guid) != resources.end());
        return guid;
    }

    void load();
    void loadResource(DataFile::Value& data);

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

    Texture* getTexture(i64 guid)
    {
        auto iter = resources.find(guid);
        if (iter == resources.end() || iter->second->type != ResourceType::TEXTURE)
        {
            //FATAL_ERROR("Texture not found: ", guid);
            //print("Texture not found: ", guid, '\n');
            return &white;
        }
        return (Texture*)iter->second.get();
    }

    Texture* getTexture(const char* name)
    {
        auto iter = resourceNameMap.find(name);
        if (iter == resourceNameMap.end() || iter->second->type != ResourceType::TEXTURE)
        {
            //FATAL_ERROR("Texture not found: ", guid);
            //print("Texture not found: ", guid, '\n');
            return &white;
        }
        return (Texture*)iter->second;
    }

    Model* getModel(i64 guid)
    {
        auto iter = resources.find(guid);
        if (iter == resources.end() || iter->second->type != ResourceType::MODEL)
        {
            FATAL_ERROR("Model not found: ", guid);
        }
        return (Model*)iter->second.get();
    }

    Model* getModel(const char* name)
    {
        auto iter = resourceNameMap.find(name);
        if (iter == resourceNameMap.end() || iter->second->type != ResourceType::MODEL)
        {
            FATAL_ERROR("Model not found: ", name);
        }
        return (Model*)iter->second;
    }

    Sound* getSound(i64 guid)
    {
        auto iter = resources.find(guid);
        if (iter == resources.end() || iter->second->type != ResourceType::SOUND)
        {
            FATAL_ERROR("Sound not found: ", guid);
        }
        return (Sound*)iter->second.get();
    }

    Sound* getSound(const char* name)
    {
        auto iter = resourceNameMap.find(name);
        if (iter == resourceNameMap.end() || iter->second->type != ResourceType::SOUND)
        {
            FATAL_ERROR("Sound not found: ", name);
        }
        return (Sound*)iter->second;
    }

    Material* getMaterial(i64 guid)
    {
        auto iter = resources.find(guid);
        if (iter == resources.end() || iter->second->type != ResourceType::MATERIAL)
        {
            //FATAL_ERROR("Material not found: ", guid);
            //print("Material not found: ", guid, '\n');
            return &defaultMaterial;
        }
        return (Material*)iter->second.get();
    }

    Material* getMaterial(const char* name)
    {
        auto iter = resourceNameMap.find(name);
        if (iter == resourceNameMap.end() || iter->second->type != ResourceType::MATERIAL)
        {
            //FATAL_ERROR("Material not found: ", name);
            //print("Material not found: ", name, '\n');
            return &defaultMaterial;
        }
        return (Material*)iter->second;
    }

    TrackData* getTrackData(i64 guid)
    {
        auto iter = resources.find(guid);
        if (iter == resources.end() || iter->second->type != ResourceType::TRACK)
        {
            FATAL_ERROR("Track not found: ", guid);
        }
        return (TrackData*)iter->second.get();
    }

    i64 getTrackGuid(const char* name)
    {
        auto iter = resourceNameMap.find(name);
        if (iter == resourceNameMap.end() || iter->second->type != ResourceType::TRACK)
        {
            FATAL_ERROR("Track not found: ", name);
        }
        return iter->second->guid;
    }
} g_res;

