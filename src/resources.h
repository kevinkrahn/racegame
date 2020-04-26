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
    std::string filename = str(DATA_DIRECTORY, "/", std::hex, val.guid, ".dat");
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

struct Sounds
{
    Sound evironment = Sound("sounds/environment.ogg");
    Sound blaster = Sound("sounds/blaster.wav");
    Sound blaster_hit = Sound("sounds/blaster_hit.wav");
    Sound bouncer_bounce = Sound("sounds/bouncer_bounce.wav");
    Sound bouncer_fire = Sound("sounds/bouncer_fire.wav");
    Sound clapping = Sound("sounds/clapping.wav");
    Sound click = Sound("sounds/click.wav");
    Sound close = Sound("sounds/close.wav");
    Sound countdown_a = Sound("sounds/countdown_a.wav");
    Sound countdown_b = Sound("sounds/countdown_b.wav");
    //Sound engine = Sound("sounds/engine.wav");
    Sound engine2 = Sound("sounds/engine2.wav");
    //Sound engine_overlay = Sound("sounds/engine_overlay.wav");
    Sound explosion1 = Sound("sounds/explosion1.wav");
    Sound explosion2 = Sound("sounds/explosion2.wav");
    Sound explosion3 = Sound("sounds/explosion3.wav");
    Sound explosion4 = Sound("sounds/explosion4.wav");
    Sound explosion5 = Sound("sounds/explosion5.wav");
    Sound explosion6 = Sound("sounds/explosion6.wav");
    Sound explosion7 = Sound("sounds/explosion7.wav");
    Sound impact = Sound("sounds/impact.wav");
    Sound lap = Sound("sounds/lap.wav");
    //Sound mg = Sound("sounds/mg.wav");
    Sound mg2 = Sound("sounds/mg2.wav");
    Sound missile = Sound("sounds/missile.wav");
    Sound nono = Sound("sounds/nono.wav");
    Sound oil = Sound("sounds/oil.wav");
    Sound glue = Sound("sounds/glue.wav");
    Sound sticky = Sound("sounds/sticky.wav");
    Sound rocketboost = Sound("sounds/rocketboost.wav");
    Sound select = Sound("sounds/select.wav");
    Sound thunk = Sound("sounds/thunk.wav");
    Sound tires = Sound("sounds/tires.wav");
    Sound money = Sound("sounds/money.wav");
    Sound fixup = Sound("sounds/fixup.wav");
    Sound airdrill = Sound("sounds/airdrill.wav");
    Sound kinetic_armor = Sound("sounds/kinetic_armor.wav");
    Sound jumpjet = Sound("sounds/jumpjet.wav");
    Sound cheer = Sound("sounds/cheer.wav");

    Sound bullet_impact1 = Sound("sounds/impacts/bullet_impact1.wav");
    Sound bullet_impact2 = Sound("sounds/impacts/bullet_impact2.wav");
    Sound bullet_impact3 = Sound("sounds/impacts/bullet_impact3.wav");
    Sound richochet1 = Sound("sounds/impacts/richochet1.wav");
    Sound richochet2 = Sound("sounds/impacts/richochet2.wav");
    Sound richochet3 = Sound("sounds/impacts/richochet3.wav");
    Sound richochet4 = Sound("sounds/impacts/richochet4.wav");
};

const char* dataFiles[] = {
    "models/mine.dat",
    "models/railing.dat",
    "models/railing2.dat",
    "models/world.dat",
    "models/money.dat",
    "models/wrench.dat",

    "models/vehicles/mini.dat",
    "models/vehicles/coolcar.dat",
    "models/vehicles/muscle.dat",
    "models/vehicles/racecar.dat",
    "models/vehicles/sportscar.dat",
    "models/vehicles/stationwagon.dat",
    "models/vehicles/truck.dat",

    "models/weapons/minigun.dat",
    "models/weapons/blaster.dat",
    "models/weapons/missile.dat",
    "models/weapons/bouncer.dat",
};

class Resources
{
private:
    std::map<std::string, Mesh> meshes;
    std::map<std::string, DataFile::Value::Dict> scenes;
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

    std::unique_ptr<Sounds> sounds;

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
                materials.find(guid) == materials.end())
            {
                break;
            }
            print("GUID collision: ", guid, '\n');
        }

        return guid;
    }

    void load();
    void loadResource(DataFile::Value& data);

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

