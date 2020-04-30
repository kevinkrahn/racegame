#pragma once

#include "datafile.h"
#include "math.h"
#include "font.h"
#include "audio.h"
#include "mesh.h"
#include "texture.h"
#include "model.h"
#include "material.h"
#include <vector>
#include <map>
#include <string>

const char* DATA_DIRECTORY = "../editor_data";
const char* ASSET_DIRECTORY = "../assets";

template<typename T>
void saveResource(T& val)
{
    std::string filename = str(DATA_DIRECTORY, "/", std::hex, val.guid, ".dat", std::dec);
    print("Saving resource ", filename, '\n');
    Serializer::toFile(val, filename);
}

template<typename T>
void loadResource(DataFile::Value& data, T& val)
{
    //Serializer::fromFile(val, str(DATA_DIRECTORY, "/", std::hex, val.guid, ".dat"));
    Serializer s(data, true);
    val.serialize(s);
}

enum struct ResourceType
{
    TEXTURE = 1,
    MODEL = 2,
    SOUND = 3,
    FONT = 4,
    TRACK = 5,
    MATERIAL = 6,
};

class Resources
{
private:
    std::map<const char*, std::map<u32, Font>> fonts;
    RandomSeries series = randomSeed();

public:
    std::map<i64, std::unique_ptr<Texture>> textures;
    std::map<std::string, Texture*> textureNameMap;
    std::map<i64, DataFile::Value> tracks;
    std::map<std::string, i64> trackNameMap;
    std::map<i64, std::unique_ptr<Model>> models;
    std::map<std::string, Model*> modelNameMap;
    std::map<i64, std::unique_ptr<Material>> materials;
    std::map<std::string, Material*> materialNameMap;
    std::map<i64, std::unique_ptr<Sound>> sounds;
    std::map<std::string, Sound*> soundNameMap;

    Texture white;
    Texture identityNormal;
    Material defaultMaterial;

    i64 generateGUID()
    {
        i64 guid = 0;
        for(;;)
        {
            u32 guidHalf[2] = { xorshift32(series), xorshift32(series) };
            guid = *((i64*)guidHalf);

            if (textures.find(guid) == textures.end() &&
                tracks.find(guid) == tracks.end() &&
                models.find(guid) == models.end() &&
                sounds.find(guid) == sounds.end() &&
                materials.find(guid) == materials.end() &&
                guid != 0)
            {
                break;
            }
            print("GUID collision: ", guid, '\n');
        }

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
        auto iter = textures.find(guid);
        if (iter == textures.end())
        {
            //FATAL_ERROR("Texture not found: ", guid);
            //print("Texture not found: ", guid, '\n');
            return &white;
        }
        return iter->second.get();
    }

    Texture* getTexture(const char* name)
    {
        auto iter = textureNameMap.find(name);
        if (iter == textureNameMap.end())
        {
            //FATAL_ERROR("Texture not found: ", name);
            //print("Texture not found: ", name, '\n');
            return &white;
        }
        return iter->second;
    }

    Model* getModel(i64 guid)
    {
        auto iter = models.find(guid);
        if (iter == models.end())
        {
            FATAL_ERROR("Model not found: ", guid);
        }
        return iter->second.get();
    }

    Model* getModel(const char* name)
    {
        auto iter = modelNameMap.find(name);
        if (iter == modelNameMap.end())
        {
            FATAL_ERROR("Model not found: ", name);
        }
        return iter->second;
    }

    Sound* getSound(i64 guid)
    {
        auto iter = sounds.find(guid);
        if (iter == sounds.end())
        {
            FATAL_ERROR("Sound not found: ", guid);
        }
        return iter->second.get();
    }

    Sound* getSound(const char* name)
    {
        auto iter = soundNameMap.find(name);
        if (iter == soundNameMap.end())
        {
            FATAL_ERROR("Sound not found: ", name);
        }
        return iter->second;
    }

    Material* getMaterial(i64 guid)
    {
        auto iter = materials.find(guid);
        if (iter == materials.end())
        {
            //FATAL_ERROR("Material not found: ", guid);
            //print("Material not found: ", guid, '\n');
            return &defaultMaterial;
        }
        return iter->second.get();
    }

    Material* getMaterial(const char* name)
    {
        auto iter = materialNameMap.find(name);
        if (iter == materialNameMap.end())
        {
            //FATAL_ERROR("Material not found: ", name);
            //print("Material not found: ", name, '\n');
            return &defaultMaterial;
        }
        return iter->second;
    }

    DataFile::Value& getTrackData(i64 guid)
    {
        auto iter = tracks.find(guid);
        if (iter == tracks.end())
        {
            FATAL_ERROR("Track not found: ", guid);
        }
        return iter->second;
    }

    i64 getTrackGuid(const char* name)
    {
        auto iter = trackNameMap.find(name);
        if (iter == trackNameMap.end())
        {
            FATAL_ERROR("Track not found: ", name);
        }
        return iter->second;
    }
} g_res;

