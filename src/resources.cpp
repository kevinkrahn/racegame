#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "driver.h"
#include "util.h"
#include <filesystem>

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
        resource->name = str(namePrefix, ' ', resources.size());
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
    identityNormal = Texture("identityNormal", 1, 1, (u8*)identityNormalBytes,
            sizeof(identityNormalBytes), TextureType::NORMAL_MAP);

    Array<FileItem> resourceFiles = readDirectory(DATA_DIRECTORY);
    for (auto& file : resourceFiles)
    {
        auto path = std::filesystem::path(file.path);
        if (path.extension() == ".dat" && path.filename() != METADATA_FILE)
        {
            auto data = DataFile::load(str(DATA_DIRECTORY, "/", file.path));
            //print("Loading data file: ", file.path, ", Asset Name: ", data.dict(true).val()["name"], '\n');
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

