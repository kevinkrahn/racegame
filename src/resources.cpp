#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "driver.h"
#include "util.h"
#include <filesystem>

void Resources::addResource(std::unique_ptr<Resource>&& resource)
{
    i64 guid = resource->guid;
    resourceNameMap[resource->name] = resource.get();
    resources[guid] = std::move(resource);
}

void Resources::loadResource(DataFile::Value& data)
{
    if (data.dict().hasValue())
    {
        auto& dict = data.dict().val();
        auto resourceType = (u32)dict["type"].integer().val();
        Resource* resource = nullptr;
        switch ((ResourceType)resourceType)
        {
            case ResourceType::TEXTURE:
                resource = new Texture();
                break;
            case ResourceType::MODEL:
                resource = new Model();
                break;
            case ResourceType::SOUND:
                resource = new Sound();
                break;
            case ResourceType::FONT:
                break;
            case ResourceType::TRACK:
                resource = new TrackData();
                break;
            case ResourceType::MATERIAL:
                resource = new Material();
                break;
        }
        if (resource != nullptr)
        {
            Serializer s(data, true);
            resource->serialize(s);
            addResource(std::unique_ptr<Resource>(resource));
        }
    }
}

void Resources::load()
{
    constexpr u8 whiteBytes[] = { 255, 255, 255, 255 };
    constexpr u8 identityNormalBytes[] = { 128, 128, 255, 255 };
    white = Texture("white", 1, 1, (u8*)whiteBytes, TextureType::COLOR);
    identityNormal = Texture("identityNormal", 1, 1, (u8*)identityNormalBytes, TextureType::NORMAL_MAP);

    std::vector<FileItem> resourceFiles = readDirectory(DATA_DIRECTORY);
    for (auto& file : resourceFiles)
    {
        auto path = std::filesystem::path(file.path);
        if (path.extension() == ".dat" && path.filename() != METADATA_FILE)
        {
            print("Loading data file: ", file.path, '\n');
            auto data = DataFile::load(str(DATA_DIRECTORY, "/", file.path));
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
}

