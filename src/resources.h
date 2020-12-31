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

const char* DATA_DIRECTORY = "../editor_data";
const char* ASSET_DIRECTORY = "../assets";
const char* METADATA_FILE = "metadata.dat";

namespace ResourceFlags
{
    enum
    {
        NONE = 0,
        EXCLUSIVE_EDITOR = 1 << 0,
    };
}

struct RegisteredResourceType
{
    const char* name;
    const char* resourceIcon;
    ResourceType resourceType;
    u32 flags;
    Resource*(*create)();
    class ResourceEditor*(*createEditor)();
};

Map<ResourceType, RegisteredResourceType> g_resourceTypes;

template <typename RESOURCE, typename EDITOR>
void registerResourceType(ResourceType resourceType, const char* name, const char* icon,
        u32 flags = ResourceFlags::NONE)
{
    RegisteredResourceType t = {
        name,
        icon,
        resourceType,
        flags,
        []() -> Resource* { return new RESOURCE(); },
        []() -> ResourceEditor* { return new EDITOR(); },
    };
    g_resourceTypes.set(resourceType, t);
}

class Resources
{
private:
    // TODO: convert fonts into resource
    Map<const char*, Map<u32, Font>> fonts;
    // TODO: use bigger map size than the default 64 because there are a lot of resources
    Map<i64, OwnedPtr<Resource>> resources;
    Map<Str64, Resource*> resourceNameMap;

public:
    void initResourceTypes();
    void load();
    void loadResource(DataFile::Value& data);
    Resource* newResource(ResourceType type, bool makeGUID);
    void registerResource(OwnedPtr<Resource>&& resource);
    void renameResource(Resource* resource, Str64 const& newName)
    {
        resourceNameMap.erase(resource->name);
        resource->name = newName;
        resourceNameMap.set(resource->name, resource);
    }

    template <typename T>
    void iterateResourceType(ResourceType type, T const& cb)
    {
        for (auto& res : resources)
        {
            if (res.value->type == type)
            {
                cb(res.value.get());
            }
        }
    }

    template <typename T>
    void iterateResources(T const& cb)
    {
        for (auto& res : resources)
        {
            cb(res.value.get());
        }
    }

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
        while (guid <= 1 || resources.get(guid));
        return guid;
    }

    Resource* getResource(i64 guid)
    {
        auto ptr = resources.get(guid);
        return ptr ? ptr->get() : nullptr;
    }

    Font& getFont(const char* name, u32 height)
    {
        auto ptr = fonts.get(name);
        if (!ptr)
        {
            return fonts[name][height] = Font(tmpStr("%s.ttf", name), (f32)height);
        }

        auto ptr2 = ptr->get(height);
        if (!ptr2)
        {
            return fonts[name][height] = Font(tmpStr("%s.ttf", name), (f32)height);
        }

        return *ptr2;
    }

    Texture* getTexture(i64 guid)
    {
        auto iter = resources.get(guid);
        if (!iter || iter->get()->type != ResourceType::TEXTURE)
        {
            return &white;
        }
        return (Texture*)iter->get();
    }

    Texture* getTexture(const char* name)
    {
        auto iter = resourceNameMap.get(name);
        if (!iter || (*iter)->type != ResourceType::TEXTURE)
        {
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
            return &defaultMaterial;
        }
        return (Material*)iter->get();
    }

    Material* getMaterial(const char* name)
    {
        auto iter = resourceNameMap.get(name);
        if (!iter || (*iter)->type != ResourceType::MATERIAL)
        {
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

