#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "driver.h"
#include "util.h"

Resource* Resources::newResource(ResourceType type, bool makeGUID)
{
    Resource* resource = nullptr;
    const char* namePrefix = "";
    switch (type)
    {
        case ResourceType::TEXTURE:
        {
            resource = new Texture();
            namePrefix = "Texture";
        } break;
        case ResourceType::SOUND:
        {
            resource = new Sound();
            namePrefix = "Sound";
        } break;
        case ResourceType::MATERIAL:
        {
            resource = new Material();
            namePrefix = "Material";
        } break;
        case ResourceType::MODEL:
        {
            resource = new Model();
            namePrefix = "Model";
        } break;
        case ResourceType::TRACK:
        {
            resource = new TrackData();
            namePrefix = "Track";
        } break;
        default:
        {
            error("Cannot create resource with unknown type: ", (u32)type);
            return nullptr;
        }
    }
    resource->type = type;
    if (makeGUID)
    {
        resource->guid = generateGUID();
        resource->name = Str64::format("%s %u", namePrefix, resources.size());
    }
    return resource;
}

void Resources::registerResource(OwnedPtr<Resource>&& resource)
{
    i64 guid = resource->guid;
    resourceNameMap.set(resource->name, resource.get());
    resources.set(guid, std::move(resource));
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

