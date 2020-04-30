#include "resources.h"
#include "renderer.h"
#include "game.h"
#include "driver.h"
#include "util.h"
#include <filesystem>

void Resources::loadResource(DataFile::Value& data)
{
    if (data.dict().hasValue())
    {
        auto& dict = data.dict().val();
        auto resourceType = (u32)dict["type"].integer().val();
        auto guid = dict["guid"].integer().val();
        switch ((ResourceType)resourceType)
        {
            case ResourceType::TEXTURE:
            {
                auto tex = std::make_unique<Texture>();
                ::loadResource(data, *tex);
                textureNameMap[tex->name] = tex.get();
                textures[guid] = std::move(tex);
            } break;
            case ResourceType::MODEL:
            {
                auto model = std::make_unique<Model>();
                ::loadResource(data, *model);
                modelNameMap[model->name] = model.get();
                models[model->guid] = std::move(model);
            } break;
            case ResourceType::SOUND:
            {
                auto sound = std::make_unique<Sound>();
                ::loadResource(data, *sound);
                soundNameMap[sound->name] = sound.get();
                sounds[sound->guid] = std::move(sound);
            } break;
            case ResourceType::FONT:
            {
            } break;
            case ResourceType::TRACK:
            {
                if (dict["name"].string().hasValue())
                {
                    trackNameMap[dict["name"].string().val()] = guid;
                }
                tracks[guid] = std::move(data);
            } break;
            case ResourceType::MATERIAL:
            {
                auto material = std::make_unique<Material>();
                ::loadResource(data, *material);
                materialNameMap[material->name] = material.get();
                materials[guid] = std::move(material);
            }
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
        if (std::filesystem::path(file.path).extension() == ".dat")
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

