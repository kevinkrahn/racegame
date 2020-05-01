#pragma once

#include "../math.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../input.h"
#include "../util.h"
#include "editor.h"
#include "model_editor.h"

/*
struct ResourceFolder
{
    std::string name;
    std::vector<ResourceFolder> childFolders;
    std::vector<i64> childResources;

    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(childFolders);
        s.field(childResources);
    }
};

struct ResourceFolders
{
    ResourceFolder textures;
    ResourceFolder materials;
    ResourceFolder models;
    ResourceFolder sounds;
    ResourceFolder tracks;

    void serialize(Serializer& s)
    {
        s.field(textures);
        s.field(materials);
        s.field(models);
        s.field(sounds);
        s.field(tracks);
    }
};
*/

class ResourceManager
{
    bool isResourceWindowOpen = true;
    std::string editName;
    ResourceType activeEditor;
    std::string renameText;
    bool firstFrameRename = false;

    Editor trackEditor;
    ModelEditor modelEditor;

    std::vector<Texture*> textures;
    bool texturesStale = true;
    Texture* selectedTexture = nullptr;
    bool isTextureWindowOpen = false;
    Texture* renameTexture = nullptr;

    std::vector<Material*> materials;
    bool materialsStale = true;
    Material* selectedMaterial = nullptr;
    bool isMaterialWindowOpen = false;

    std::vector<DataFile::Value*> tracks;
    bool tracksStale = true;

    std::vector<Model*> models;
    bool modelsStale = true;

    std::vector<Sound*> sounds;
    bool soundsStale = true;
    Sound* selectedSound = nullptr;
    bool isSoundWindowOpen = false;

    void newModel();
    void newTexture();
    void newMaterial();
    void newSound();
    void showTextureWindow(Renderer* renderer, f32 deltaTime);
    void showMaterialWindow(Renderer* renderer, f32 deltaTime);
    void showSoundWindow(Renderer* renderer, f32 deltaTime);

public:
    ResourceManager();
    void onUpdate(Renderer* renderer, f32 deltaTime);
};
