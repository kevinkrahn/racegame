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
    u32* b = (u32*)&guid;
    Str32 guidStr = Str32::format("%x%x", b[1], b[0]);
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

    void rename(const char* newName)
    {
        // TODO: create fs path api for working with paths
        const char* relativePath = getParentPath();
        const char* oldPath = tmpStr("%s/%s/%s", DATA_DIRECTORY, relativePath, name.data());
        const char* newPath = tmpStr("%s/%s/%s", DATA_DIRECTORY, relativePath, newName);
        renameFile(oldPath, newPath);
        name = newName;
    }

    void deleteBackingDirectory()
    {
        const char* path = tmpStr("%s/%s/%s", DATA_DIRECTORY, getParentPath(), name.data());
        deleteFile(path);
    }

    void deleteChildResourceFile(i64 guid)
    {
        Str32 h = hex(guid);
        const char* filename = tmpStr("%s/%s/%s/%s.dat", DATA_DIRECTORY, getParentPath(),
                name.data(), h.data());
        deleteFile(filename);
    }

    const char* getParentPath()
    {
        const char* pth = "";
        ResourceFolder* folder = parent;
        // don't add the name of the root folder to the path
        while (folder && folder->parent)
        {
            pth = tmpStr("%s/%s", folder->name.data(), pth);
            folder = folder->parent;
        }
        return pth;
    }

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

    void load(StrBuf const& buf)
    {
        Array<FileItem> items = readDirectory(buf.data(), false);
        for (auto const& item : items)
        {
            if (item.isDirectory)
            {
                ResourceFolder* folder = new ResourceFolder();
                folder->name = item.path;
                StrBuf newBuf;
                newBuf.write(buf);
                newBuf.write('/');
                newBuf.write(folder->name);
                childFolders.push(OwnedPtr(folder));
                folder->load(newBuf);
            }
            else
            {
                // TODO: the resource files are loaded twice; here and in resources.cpp
                // should find some way to only do it once
                auto data = DataFile::load(tmpStr("%s/%s", buf.data(), item.path));
                auto guid = data.dict(true).val()["guid"].integer(0);
                assert(guid != 0);
                if (guid != 0)
                {
                    childResources.push(guid);
                }
            }
        }
    }

    void createBackingDirectory()
    {
        const char* dir = tmpStr("%s/%s/%s", DATA_DIRECTORY, getParentPath(), name.data());
        createDirectory(dir);
    }

    void save(StrBuf const& buf, Map<i64, bool> const& resourcesModified)
    {
        assert(name.size() > 0);

        for (auto guid : childResources)
        {
            if (resourcesModified.get(guid))
            {
                if (auto res = g_res.getResource(guid))
                {
                    Str32 guidHex = hex(guid);
                    StrBuf filenameBuf;
                    filenameBuf.writef("%s/%s.dat", buf.data(), guidHex.data());
                    println("Saving resource %s, %s", filenameBuf.data(), res->name.data());
                    Serializer::toFile(*res, filenameBuf.data());
                }
            }
        }

        for (auto& folder : childFolders)
        {
            StrBuf newBuf;
            newBuf.write(buf);
            newBuf.write('/');
            newBuf.write(folder->name);

            createDirectory(newBuf.data());

            folder->save(newBuf, resourcesModified);
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
    // TODO: Make a Set<> datatype
    Map<i64, bool> resourcesModified;
    Map<Resource*, bool> resourcesClosed;

    bool showFolder(ResourceFolder* folder);
    void showFolderContents(ResourceFolder* folder);

public:
    ResourceManager();
    void onUpdate(Renderer* renderer, f32 deltaTime);
    void markClosed(Resource* r) { resourcesClosed.set(r, true); }
    void markDirty(i64 guid) { resourcesModified.set(guid, true); }
    bool isResourceDirty(i64 guid) const { return resourcesModified.get(guid); }
    Resource* newResource(ResourceType type);
    void openResource(Resource* resource);
    void closeResource(Resource* resource);
    void saveResources();

    Resource* getOpenedResource(ResourceType resourceType)
    {
        auto result = openedResources.findIf([resourceType](OpenedResource const& r) {
            return r.resource->type == resourceType;
        });
        return result ? result->resource : nullptr;
    }
};

template <typename CB>
bool chooseResource(ResourceType resourceType, i64& current, const char* name,
        CB const& filter=[](Resource* r){ return true; })
{
    bool changed = false;
    Resource* resource = g_res.getResource(current);
    auto& style = ImGui::GetStyle();
    auto width = ImGui::CalcItemWidth();
    u32 previewTexture = 0;

    // TODO: Add option to hide the "X" button

    if (resource)
    {
        previewTexture = resource->getPreviewTexture();
    }
    if (previewTexture)
    {
        f32 cx = ImGui::GetCursorPosX();
        ImGui::Image((void*)(uintptr_t)previewTexture, { 48, 48 });
        ImGui::SameLine(cx + 48 - 17);
        ImGui::SetNextItemWidth(20);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.f, 0.8f, 0.8f));
        if (ImGui::Button(tmpStr("X###X%s", name)))
        {
            current = 0;
            changed = true;
        }
        ImGui::PopStyleColor(3);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(width - 56);
        ImVec2 size = { width - 56, 200.f };
        ImGui::SetNextWindowSizeConstraints(size, size);
    }
    else
    {
        if (resource)
        {
            ImGui::SetNextItemWidth(20);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0.f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0.f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0.f, 0.8f, 0.8f));
            if (ImGui::Button(tmpStr("X###X%s", name)))
            {
                current = 0;
                changed = true;
            }
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(width - 17 - style.ItemSpacing.x);
            ImVec2 size = { width - 17 - style.ItemSpacing.x, 200.f };
            ImGui::SetNextWindowSizeConstraints(size, size);
        }
        else
        {
            ImVec2 size = { width, 200.f };
            ImGui::SetNextWindowSizeConstraints(size, size);
        }
    }

    if (ImGui::BeginCombo(name, resource ? resource->name.data() : "None"))
    {
        static Str64 searchString;
        static Array<Resource*> searchResults;
        searchResults.clear();

        if (ImGui::IsWindowAppearing())
        {
            ImGui::SetKeyboardFocusHere();
            searchString = "";
        }

        ImGui::PushItemWidth(ImGui::GetWindowWidth() - style.ItemSpacing.x);
        bool enterPressed = ImGui::InputText("", &searchString, ImGuiInputTextFlags_EnterReturnsTrue);

        g_res.iterateResourceType(resourceType, [&filter](Resource* resource){
            if (filter(resource))
            {
                // TODO: make this case-insensitive
                if (searchString.empty() || resource->name.find(searchString))
                {
                    searchResults.push(resource);
                }
            }
        });
        searchResults.sort([](auto& a, auto& b) { return a->name < b->name; });

        if (enterPressed)
        {
            current = searchResults[0]->guid;
            changed = true;
            ImGui::CloseCurrentPopup();
        }

        if (ImGui::BeginChild("Search Results", { 0, 0 }))
        {
            for (Resource* r : searchResults)
            {
                auto previewTexture = r->getPreviewTexture();
                if (previewTexture)
                {
                    ImGui::Image((void*)(uintptr_t)previewTexture, { 16, 16 });
                    ImGui::SameLine();
                }
                ImGui::PushID(r->guid);
                if (ImGui::Selectable(r->name.data()))
                {
                    changed = true;
                    current = r->guid;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::PopID();
            }
        }
        ImGui::EndChild();

        ImGui::EndCombo();
    }
    else
    {
        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            if (payload)
            {
                assert(payload->DataSize == sizeof(DragDropPayload));
                DragDropPayload data = *(DragDropPayload*)payload->Data;
                if (!data.isFolder && data.resourceDragged->type == resourceType && filter(resource))
                {
                    if (ImGui::AcceptDragDropPayload("Drag Resource"))
                    {
                        current = data.resourceDragged->guid;
                        changed = true;
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    return changed;
}

bool chooseTexture(i32 type, i64& currentTexture, const char* name)
{
    return chooseResource(ResourceType::TEXTURE, currentTexture, name, [type](Resource* r){
        return ((Texture*)r)->getTextureType() == type;
    });
}
