#pragma once

#include "../math.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../input.h"
#include "../util.h"
#include "../ai_driver_data.h"
#include "resource_editor.h"

struct ResourceFolder
{
    Str64 name;
    Array<OwnedPtr<ResourceFolder>> childFolders;
    Array<i64> childResources;
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

struct OpenedResource
{
    Resource* resource;
    OwnedPtr<ResourceEditor> editor;
};

struct RegisteredResourceEditor
{
    ResourceEditor*(*createEditor)();
};

class ResourceManager
{
    ResourceFolder resources = { "Resources" };

    bool isResourceWindowOpen = true;
    Str64 renameText;
    ResourceFolder* renameFolder = nullptr;
    Resource* renameResource = nullptr;
    bool firstFrameRename = false;
    struct
    {
        DragDropPayload payload;
        ResourceFolder* dropFolder = nullptr;
    } folderMove;

    OwnedPtr<ResourceEditor> activeExclusiveEditor;
    Resource* activeExclusiveResource = nullptr;

    Map<ResourceType, RegisteredResourceEditor> registeredResourceEditors;
    Array<OpenedResource> openedResources;
    Map<i64, bool> resourcesModified;

    bool showFolder(ResourceFolder* folder);
    void showFolderContents(ResourceFolder* folder);

public:
    ResourceManager();
    void onUpdate(Renderer* renderer, f32 deltaTime);
    void markDirty(i64 guid) { resourcesModified.set(guid, true); }
    bool isResourceDirty(i64 guid) const { return resourcesModified.get(guid); }
    Resource* newResource(ResourceType type);
    void openResource(Resource* resource);
    void closeResource(Resource* resource);
    void saveResources();
    bool chooseTexture(i32 type, i64& currentTexture, const char* name);

    Resource* getOpenedResource(ResourceType resourceType)
    {
        auto result = openedResources.findIf([resourceType](OpenedResource const& r) {
            return r.resource->type == resourceType;
        });
        return result ? result->resource : nullptr;
    }
};
