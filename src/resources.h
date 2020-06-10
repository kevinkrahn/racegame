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
#include <string>

const char* DATA_DIRECTORY = "../editor_data";
const char* ASSET_DIRECTORY = "../assets";
const char* METADATA_FILE = "metadata.dat";

class Resources
{
private:
    // TODO: convert fonts into resource
    Map<const char*, Map<u32, Font>> fonts;

public:
    Map<i64, OwnedPtr<Resource>> resources;
    Map<std::string, Resource*> resourceNameMap;

    void addResource(OwnedPtr<Resource>&& resource);

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
        while (guid == 0 || !resources.get(guid));
        return guid;
    }

    void load();
    void loadResource(DataFile::Value& data);

    Font& getFont(const char* name, u32 height)
    {
        auto ptr = fonts.get(name);
        if (!ptr)
        {
            return fonts[name][height] = Font(std::string(name) + ".ttf", (f32)height);
        }

        auto ptr2 = ptr->get(height);
        if (!ptr2)
        {
            return fonts[name][height] = Font(std::string(name) + ".ttf", (f32)height);
        }

        return *ptr2;
    }

    Texture* getTexture(i64 guid)
    {
        auto iter = resources.get(guid);
        if (!iter || iter->get()->type != ResourceType::TEXTURE)
        {
            //FATAL_ERROR("Texture not found: ", guid);
            //print("Texture not found: ", guid, '\n');
            return &white;
        }
        return (Texture*)iter->get();
    }

    Texture* getTexture(const char* name)
    {
        auto iter = resourceNameMap.get(name);
        if (!iter || (*iter)->type != ResourceType::TEXTURE)
        {
            //FATAL_ERROR("Texture not found: ", guid);
            //print("Texture not found: ", guid, '\n');
            return &white;
        }
        return (Texture*)*iter;
    }

    Model* getModel(i64 guid)
    {
        auto iter = resources.get(guid);
        if (!iter || iter->get()->type != ResourceType::MODEL)
        {
            FATAL_ERROR("Model not found: ", guid);
        }
        return (Model*)iter->get();
    }

    Model* getModel(const char* name)
    {
        auto iter = resourceNameMap.get(name);
        if (!iter || (*iter)->type != ResourceType::MODEL)
        {
            FATAL_ERROR("Model not found: ", name);
        }
        return (Model*)*iter;
    }

    Sound* getSound(i64 guid)
    {
        auto iter = resources.get(guid);
        if (!iter || iter->get()->type != ResourceType::SOUND)
        {
            FATAL_ERROR("Sound not found: ", guid);
        }
        return (Sound*)iter->get();
    }

    Sound* getSound(const char* name)
    {
        auto iter = resourceNameMap.get(name);
        if (!iter || (*iter)->type != ResourceType::SOUND)
        {
            FATAL_ERROR("Sound not found: ", name);
        }
        return (Sound*)*iter;
    }

    Material* getMaterial(i64 guid)
    {
        auto iter = resources.get(guid);
        if (!iter || iter->get()->type != ResourceType::MATERIAL)
        {
            //FATAL_ERROR("Material not found: ", guid);
            //print("Material not found: ", guid, '\n');
            return &defaultMaterial;
        }
        return (Material*)iter->get();
    }

    Material* getMaterial(const char* name)
    {
        auto iter = resourceNameMap.get(name);
        if (!iter || (*iter)->type != ResourceType::MATERIAL)
        {
            //FATAL_ERROR("Material not found: ", name);
            //print("Material not found: ", name, '\n');
            return &defaultMaterial;
        }
        return (Material*)*iter;
    }

    TrackData* getTrackData(i64 guid)
    {
        auto iter = resources.get(guid);
        if (!iter || iter->get()->type != ResourceType::TRACK)
        {
            FATAL_ERROR("Track not found: ", guid);
        }
        return (TrackData*)iter->get();
    }

    i64 getTrackGuid(const char* name)
    {
        auto iter = resourceNameMap.get(name);
        if (!iter || (*iter)->type != ResourceType::TRACK)
        {
            FATAL_ERROR("Track not found: ", name);
        }
        return (*iter)->guid;
    }
} g_res;

