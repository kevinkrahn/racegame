#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "driver.h"
#include "util.h"

#include "texture.h"
#include "editor/texture_editor.h"
#include "model.h"
#include "editor/model_editor.h"
#include "trackdata.h"
#include "editor/track_editor.h"
#include "audio.h"
#include "editor/sound_editor.h"
#include "material.h"
#include "editor/material_editor.h"
#include "ai_driver_data.h"
#include "editor/ai_editor.h"
#include "vinyl_pattern.h"
#include "editor/vinyl_pattern_editor.h"
#include "vehicle_data.h"
#include "editor/vehicle_editor.h"

void Resources::initResourceTypes()
{
    registerResourceType<Texture, TextureEditor>(ResourceType::TEXTURE, "Texture", "texture_icon", ResourceFlags::NONE);
    registerResourceType<Model, ModelEditor>(ResourceType::MODEL, "Model", "model_icon", ResourceFlags::EXCLUSIVE_EDITOR);
    registerResourceType<Sound, SoundEditor>(ResourceType::SOUND, "Sound", "sound_icon", ResourceFlags::NONE);
    //registerResourceType<Font, FontEditor>(ResourceType::FONT, "Font", "icon_font", ResourceFlags::NONE);
    registerResourceType<TrackData, TrackEditor>(ResourceType::TRACK, "Track", "icon_track", ResourceFlags::EXCLUSIVE_EDITOR);
    registerResourceType<Material, MaterialEditor>(ResourceType::MATERIAL, "Material", "material_icon", ResourceFlags::NONE);
    registerResourceType<AIDriverData, AIEditor>(ResourceType::AI_DRIVER_DATA, "AI Driver", "ai_icon", ResourceFlags::NONE);
    registerResourceType<VinylPattern, VinylPatternEditor>(ResourceType::VINYL_PATTERN, "Vinyl Pattern", "vinyl_icon", ResourceFlags::NONE);
    registerResourceType<VehicleData, VehicleEditor>(ResourceType::VEHICLE, "Vehicle", "vehicle_icon", ResourceFlags::NONE);
}

Resource* Resources::newResource(ResourceType type, bool makeGUID)
{
    RegisteredResourceType* registeredResourceType = g_resourceTypes.get(type);
    Resource* resource = registeredResourceType->create();
    resource->type = type;
    if (makeGUID)
    {
        resource->guid = generateGUID();
        resource->name = Str64::format("%s %u", registeredResourceType->name, resources.size());
    }
    return resource;
}

void Resources::registerResource(OwnedPtr<Resource>&& resource)
{
    i64 guid = resource->guid;
    resourceNameMap.set(resource->name, resource.get());
    resources.set(guid, move(resource));
}

void Resources::loadResource(DataFile::Value& data)
{
    if (data.dict().hasValue())
    {
        auto& dict = data.dict().val();
        auto resourceType = (u32)dict["type"].integer().val();
        Resource* resource = newResource((ResourceType)resourceType, false);
        if (resource != nullptr)
        {
            Serializer s(data, true);
            resource->serialize(s);
            registerResource(OwnedPtr<Resource>(resource));
        }
    }
}

void Resources::load()
{
    constexpr u8 whiteBytes[] = { 255, 255, 255, 255 };
    constexpr u8 identityNormalBytes[] = { 128, 128, 255, 255 };
    white = Texture("white", 1, 1, (u8*)whiteBytes, sizeof(whiteBytes), TextureType::COLOR);
    white.guid = 0;
    identityNormal = Texture("identityNormal", 1, 1, (u8*)identityNormalBytes,
            sizeof(identityNormalBytes), TextureType::NORMAL_MAP);
    identityNormal.guid = 1;

    Array<FileItem> resourceFiles = readDirectory(DATA_DIRECTORY);
    for (auto& file : resourceFiles)
    {
        if (path::hasExt(file.path, ".dat") && !strstr(file.path, METADATA_FILE))
        {
            auto data = DataFile::load(tmpStr("%s/%s", DATA_DIRECTORY, file.path));
            /*
            println("GUID: %s : %s : %x",
                    file.path,
                    hex(data.dict(true).val()["guid"].integer(0)),
                    (u64)data.dict(true).val()["guid"].integer(0));
            */
            //println("Loading data file: %s, Asset Name: ", file.path, data.dict(true).val()["name"]);
            if (data.array().hasValue())
            {
                for (auto& el : data.array().val())
                {
                    loadResource(el);
                }
            }
            else
            {
                loadResource(data);
            }
        }
    }

    for (auto& r : resources)
    {
        if (r.value->type == ResourceType::MATERIAL)
        {
            ((Material*)r.value.get())->loadShaderHandles();
        }
    }
    defaultMaterial.loadShaderHandles();
}

