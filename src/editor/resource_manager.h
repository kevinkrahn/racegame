#pragma once

#include "../math.h"
#include "../imgui.h"
#include "../renderer.h"
#include "../input.h"
#include "../util.h"
#include "../ai_driver_data.h"
#include "resource_editor.h"

inline bool resourceIsMatch(i64 guid, const char* searchText)
{
    Str16 guidStr = Str16::format("%x", guid);
    if (guidStr == searchText)
    {
        return true;
    }
    if (g_res.getResource(guid)->name.find(searchText))
    {
        return true;
    }
    return false;
}

struct ResourceFolder
{
    Str64 name;
    Array<OwnedPtr<ResourceFolder>> childFolders;
    Array<i64> childResources;
    ResourceFolder* parent = nullptr;
    bool isExpanded = false;

    void setExpanded(bool expanded, bool recurse=false)
    {
        isExpanded = expanded;
        if (recurse)
        {
            for (auto& childFolder : childFolders)
            {
                childFolder->setExpanded(expanded, true);
            }
        }
    }

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

    bool containsMatch(const char* searchText)
    {
        for (i64 guid : childResources)
        {
            if (resourceIsMatch(guid, searchText))
            {
                return true;
            }
        }
        for (auto& childFolder : childFolders)
        {
            if (childFolder->containsMatch(searchText))
            {
                return true;
            }
        }
        return false;
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
    u32 windowID;
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
    Str64 searchText;
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
    Map<Resource*, bool> resourcesClosed;

    bool showFolder(ResourceFolder* folder);
    void showFolderContents(ResourceFolder* folder);

public:
    ResourceManager();
    void onUpdate(Renderer* renderer, f32 deltaTime);
    void markDirty(i64 guid) { resourcesModified.set(guid, true); }
    void markClosed(Resource* r) { resourcesClosed.set(r, true); }
    bool isResourceDirty(i64 guid) const { return resourcesModified.get(guid); }
    Resource* newResource(ResourceType type);
    void openResource(Resource* resource);
    void closeResource(Resource* resource);
    void saveResources();
    // TODO: implement
    bool chooseResource(ResourceType resourceType, i64 current, const char* name);
    bool chooseTexture(i32 type, i64& currentTexture, const char* name);

    Resource* getOpenedResource(ResourceType resourceType)
    {
        auto result = openedResources.findIf([resourceType](OpenedResource const& r) {
            return r.resource->type == resourceType;
        });
        return result ? result->resource : nullptr;
    }
};
