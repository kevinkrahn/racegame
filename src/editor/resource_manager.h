#pragma once

#include "../math.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../input.h"
#include "../util.h"
#include "editor.h"
#include "model_editor.h"

struct ResourceFolder
{
    std::string name;
    std::vector<std::unique_ptr<ResourceFolder>> childFolders;
    std::vector<i64> childResources;
    ResourceFolder* parent = nullptr;

    bool hasParent(ResourceFolder* p)
    {
        if (!parent) { return false; }
        if (parent == p) { return true; }
        return parent->hasParent(p);
    }

    void serialize(Serializer& s)
    {
        s.field(name);
        s.field(childFolders);
        s.field(childResources);

        if (s.deserialize)
        {
            for (auto& childFolder : childFolders)
            {
                childFolder->parent = this;
            }
        }
    }
};

struct DragDropPayload
{
    bool isFolder;
    ResourceFolder* sourceFolder;
    Resource* resourceDragged;
    ResourceFolder* folderDragged;
};

class ResourceManager
{
    ResourceFolder resources = { "Resources" };

    bool isResourceWindowOpen = true;
    std::string renameText;
    ResourceFolder* renameFolder = nullptr;
    Resource* renameResource = nullptr;
    bool firstFrameRename = false;
    struct
    {
        DragDropPayload payload;
        ResourceFolder* dropFolder = nullptr;
    } folderMove;

    ResourceType activeEditor;
    Editor trackEditor;
    ModelEditor modelEditor;

    bool isTextureWindowOpen = false;
    Texture* selectedTexture = nullptr;
    bool isSoundWindowOpen = false;
    Sound* selectedSound = nullptr;
    bool isMaterialWindowOpen = false;
    Material* selectedMaterial = nullptr;

    std::map<i64, bool> resourcesModified;

    void showFolder(ResourceFolder* folder);
    void showFolderContents(ResourceFolder* folder);
    void openResource(Resource* resource);

    Resource* newResource(ResourceType type);
    void showTextureWindow(Renderer* renderer, f32 deltaTime);
    void showMaterialWindow(Renderer* renderer, f32 deltaTime);
    void showSoundWindow(Renderer* renderer, f32 deltaTime);

    ResourceFolder& getFolder(std::string const& path);
    void saveResources();

public:
    ResourceManager();
    void onUpdate(Renderer* renderer, f32 deltaTime);
    void markDirty(i64 guid) { resourcesModified[guid] = true; }
};
