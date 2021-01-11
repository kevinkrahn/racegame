#include "resource_manager.h"
#include "../game.h"
#include "../scene.h"

static void sortResources(ResourceFolder& folder)
{
    for (auto it = folder.childResources.begin(); it != folder.childResources.end();)
    {
        if (!g_res.getResource(*it))
        {
            showError("Resource %s does not exist, removing.", hex(*it));
            it = folder.childResources.erase(it);
        }
        else
        {
            ++it;
        }
    }
    folder.childResources.sort([](i64 a, i64 b) {
        Resource* rA = g_res.getResource(a);
        Resource* rB = g_res.getResource(b);
        if (rA->type != rB->type) return (u32)rA->type < (u32)rB->type;
        return rA->name < rB->name;
    });
    for (auto& childFolder : folder.childFolders)
    {
        sortResources(*childFolder);
    }
}

void ResourceManager::saveResources()
{
    if (g_game.currentScene)
    {
        g_game.currentScene->writeTrackData();
    }
    for (auto& r : resourcesModified)
    {
        if (auto res = g_res.getResource(r.key))
        {
            const char* guidHex = hex(r.key);
            auto filename = Str512::format("%s/%s.dat", DATA_DIRECTORY, guidHex);
            println("Saving resource %s, %s", filename.data(), res->name.data());
            Serializer::toFile(*res, filename.data());
        }
    }
    resourcesModified.clear();
    Serializer::toFile(resources, tmpStr("%s/%s", DATA_DIRECTORY, METADATA_FILE));
}

ResourceManager::ResourceManager()
{
    Serializer::fromFile(resources, tmpStr("%s/%s", DATA_DIRECTORY, METADATA_FILE));
    resources.setExpanded(true);

    Map<i64, bool> resourceFolderMap;
    Array<ResourceFolder*> folders;
    folders.push(&resources);
    while (folders.size() > 0)
    {
        ResourceFolder* folder = folders.back();
        folders.pop();
        for (auto& childResource : folder->childResources)
        {
            resourceFolderMap[childResource] = true;
        }
        for (auto& childFolder : folder->childFolders)
        {
            folders.push(childFolder.get());
        }
    }

    // find any resources that are not in any folder and append them to the root folder
    g_res.iterateResources([&](Resource* res){
        if (!resourceFolderMap.get(res->guid))
        {
            resources.childResources.push(res->guid);
        }
    });

    sortResources(resources);
}

bool ResourceManager::showFolder(ResourceFolder* folder)
{
    bool removed = false;
    bool isSearching = !searchText.empty();
    bool searchMatch = isSearching && folder->containsMatch(searchText.data());
    if (isSearching && !searchMatch)
    {
        return false;
    }
    ImGui::SetNextItemOpen(folder->isExpanded || searchMatch);
    if (folder == renameFolder)
    {
        bool isFolderOpen = ImGui::TreeNodeEx((void*)folder, 0, "");
        ImGui::SameLine();
        static u32 renameID = 0;
        if (firstFrameRename)
        {
            renameID++;
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0));
        ImGui::PushID(tmpStr("Rename %u", renameID));
        if (ImGui::InputText("", &renameText, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            renameFolder->name = renameText;
        }
        ImGui::PopID();
        ImGui::PopStyleVar();
        if (!firstFrameRename && !ImGui::IsItemActive())
        {
            renameFolder = nullptr;
        }
        firstFrameRename = false;
        if (isFolderOpen)
        {
            for (auto& childFolder : folder->childFolders)
            {
                showFolder(childFolder.get());
            }
            showFolderContents(folder);
            ImGui::TreePop();
        }
    }
    else
    {
        bool isFolderOpen = ImGui::TreeNodeEx((void*)folder, 0, "%s", folder->name.data());
        if (ImGui::IsItemToggledOpen())
        {
            folder->setExpanded(isFolderOpen);
        }
        if (folder->parent && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            DragDropPayload payload = {};
            payload.isFolder = true;
            payload.folderDragged = folder;
            payload.sourceFolder = folder->parent;
            ImGui::SetDragDropPayload("Drag Resource", &payload, sizeof(payload));
            ImGui::Text(folder->name.data());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget())
        {
            const ImGuiPayload* payload = ImGui::GetDragDropPayload();
            if (payload)
            {
                assert(payload->DataSize == sizeof(DragDropPayload));
                DragDropPayload data = *(DragDropPayload*)payload->Data;
                if (data.sourceFolder != folder && !(data.isFolder && folder->hasParent(data.folderDragged)))
                {
                    if (ImGui::AcceptDragDropPayload("Drag Resource"))
                    {
                        folderMove = { data, folder };
                    }
                }
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Expand"))
            {
                folder->setExpanded(true, true);
            }
            if (ImGui::MenuItem("Collapse"))
            {
                folder->setExpanded(false, true);
            }
            if (ImGui::MenuItem("Rename"))
            {
                renameText = folder->name;
                renameFolder = folder;
                firstFrameRename = true;
            }
            if (ImGui::MenuItem("New Folder"))
            {
                OwnedPtr<ResourceFolder> newFolder(new ResourceFolder);
                newFolder->name = "New Folder";
                newFolder->parent = folder;
                folder->childFolders.push(move(newFolder));
                folder->setExpanded(true);
                renameText = folder->childFolders.back()->name;
                renameFolder = folder->childFolders.back().get();
                firstFrameRename = true;
            }
            if (ImGui::BeginMenu("New Resource..."))
            {
                for (auto& r : g_resourceTypes)
                {
                    if (ImGui::MenuItem(tmpStr("New %s", r.value.name)))
                    {
                        folder->childResources.push(newResource(r.value.resourceType)->guid);
                    }
                }
                ImGui::EndMenu();
            }
            // TODO: Always show Delete option, but prompt for confirmation if folder is not empty
            if (folder->childFolders.empty() && folder->childResources.empty())
            {
                if (ImGui::MenuItem("Delete"))
                {
                    removed = true;
                }
            }
            ImGui::EndPopup();
        }
        if (isFolderOpen)
        {
            for (auto it = folder->childFolders.begin(); it != folder->childFolders.end();)
            {
                if (showFolder(it->get()))
                {
                    it = folder->childFolders.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            showFolderContents(folder);
            ImGui::TreePop();
        }
    }
    return removed;
}

void ResourceManager::showFolderContents(ResourceFolder* folder)
{
    const u32 selectedColor = 0x992299EE;
    for (auto it = folder->childResources.begin(); it != folder->childResources.end();)
    {
        if (!searchText.empty() && !resourceIsMatch(*it, searchText.data()))
        {
            ++it;
            continue;
        }
        Resource* childResource = g_res.getResource(*it);
        bool removed = false;
        ImGui::PushStyleColor(ImGuiCol_Header, selectedColor);
        if (childResource == renameResource)
        {
            ImGui::PushID("Rename Node");
            ImGui::TreeNodeEx("", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            ImGui::PopID();
            ImGui::SameLine();
            static u32 renameID = 0;
            ImGui::PushID(tmpStr("Rename %u", renameID));
            if (firstFrameRename)
            {
                renameID++;
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0));
            if (ImGui::InputText("", &renameText,
                    ImGuiInputTextFlags_EnterReturnsTrue))
            {
                g_res.renameResource(renameResource, renameText);
                markDirty(renameResource->guid);
            }
            ImGui::PopStyleVar();
            if (!firstFrameRename && !ImGui::IsItemActive())
            {
                renameResource = nullptr;
            }
            ImGui::PopID();
            firstFrameRename = false;
        }
        else
        {
            u32 flags = ImGuiTreeNodeFlags_Leaf
                | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            ImGui::PushID((void*)childResource->guid);
            f32 cursorPos = ImGui::GetCursorPosX();
            ImGui::TreeNodeEx("                                           ", flags);
            if (ImGui::IsItemClicked())
            {
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    openResource(childResource);
                }
            }
            ImGui::PopID();
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                DragDropPayload payload = {};
                payload.isFolder = false;
                payload.resourceDragged = childResource;
                payload.sourceFolder = folder;
                ImGui::SetDragDropPayload("Drag Resource", &payload, sizeof(payload));
                ImGui::Text(childResource->name.data());
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Duplicate"))
                {
                    // TODO: the duplicated resource doesn't seem to be saved under the right folder
                    DataFile::Value data = DataFile::makeDict();
                    Serializer s(data, false);
                    childResource->serialize(s);
                    Resource* resource = newResource(childResource->type);
                    data.dict().val()["guid"] = resource->guid;
                    data.dict().val()["name"].string().val()
                        = tmpStr("%s_copy", data.dict().val()["name"].string().val().data());
                    s.deserialize = true;
                    resource->serialize(s);
                }
                if (ImGui::MenuItem("Rename"))
                {
                    renameText = childResource->name;
                    renameResource = childResource;
                    firstFrameRename = true;
                }
                if (ImGui::MenuItem("Delete"))
                {
                    removed = true;
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 0);
            ImGui::SetCursorPosX(cursorPos + 4.f);
            ImGui::Image((void*)(uintptr_t)g_res.getTexture(
                    g_resourceTypes[childResource->type].resourceIcon)->getPreviewHandle(), { 16, 16 });
            ImGui::SameLine(0, 0);
            ImGui::SetCursorPosX(cursorPos + 24.f);
            ImGui::Text("%s%s", childResource->name.data(), isDirty(childResource->guid) ? "*" : "");
        }
        ImGui::PopStyleColor();
        if (removed)
        {
            // TODO: add confirmation dialog
            closeResource(childResource);

            const char* filename = tmpStr("%s/%s.dat", DATA_DIRECTORY, hex(*it));
            if (remove(filename) != 0)
            {
                error("Failed to delete file: %s", filename);
            }
            removed = true;
            it = folder->childResources.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void ResourceManager::openResource(Resource* resource)
{
    if (openedResources.findIf([resource](OpenedResource const& r) { return r.resource == resource; }))
    {
        return;
    }
    ResourceEditor* resourceEditor = g_resourceTypes.get(resource->type)->createEditor();
    if (g_resourceTypes.get(resource->type)->flags & ResourceFlags::EXCLUSIVE_EDITOR)
    {
        g_game.unloadScene();
        resourceEditor->init(resource);
        activeExclusiveEditor.reset(resourceEditor);
    }
    else
    {
        u32 windowID = 1;
        for (u32 i=1; i<99999; ++i)
        {
            if (!openedResources.findIf([i, resource](auto& r) {
                return r.resource->type == resource->type && r.windowID == i;
            }))
            {
                windowID = i;
                break;
            }
        }

        resourceEditor->init(resource);
        openedResources.push({
            resource,
            resourceEditor,
            windowID,
        });
    }
    markDirty(resource->guid);
}

void ResourceManager::closeResource(Resource* resource)
{
    auto r = openedResources.findIf([resource](OpenedResource const& r) { return r.resource == resource; });
    if (r)
    {
        openedResources.erase(r);
    }
    if (activeExclusiveResource == resource)
    {
        activeExclusiveResource = nullptr;
        activeExclusiveEditor.reset();
        if (g_game.currentScene && g_game.currentScene->guid == resource->guid)
        {
            g_game.unloadScene();
        }
    }
}

Resource* ResourceManager::newResource(ResourceType type)
{
    Resource* resource = g_res.newResource(type, true);
    g_res.registerResource(resource);
    markDirty(resource->guid);
    openResource(resource);
    return resource;
}

void ResourceManager::onUpdate(Renderer *renderer, f32 deltaTime)
{
    if (activeExclusiveEditor && activeExclusiveEditor->wantsExclusiveScreen())
    {
        return;
    }

    for (auto& r : resourcesClosed)
    {
        closeResource(r.key);
    }
    resourcesClosed.clear();

    renderer->getRenderWorld()->setHighlightColor(0, Vec4(1.f, 0.65f, 0.1f, 1.f));

    if (ImGui::BeginPopupModal("Exit Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::Text("Are you sure you want to exit the editor?");
        ImGui::Gap();
        ImGui::SetItemDefaultFocus();
        if (ImGui::Button("Yes", {120, 0}))
        {
            ImGui::CloseCurrentPopup();
            g_game.isEditing = false;
            g_game.menu.showMainMenu();
        }
        ImGui::SameLine();
        if (ImGui::Button("No", {120, 0}))
        {
            ImGui::CloseCurrentPopup();
        }
        if (g_input.isKeyPressed(KEY_ESCAPE))
        {
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    bool exitEditor = false;
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            // TODO: Make the shortcut actually work
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                saveResources();
            }
            if (ImGui::MenuItem("Exit"))
            {
                // TODO: Prompt for unsaved changes
                exitEditor = true;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::MenuItem("Resources", "Alt+R", isResourceWindowOpen))
            {
                isResourceWindowOpen = !isResourceWindowOpen;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
    if (exitEditor)
    {
        ImGui::OpenPopup("Exit Editor");
    }

    if (isResourceWindowOpen)
    {
        if (ImGui::Begin("Resources", &isResourceWindowOpen))
        {
            ImGui::InputText("Search", &searchText);
            ImGui::Gap();
            ImGui::BeginChild("Resources");
            showFolder(&resources);
            ImGui::EndChild();
        }
        ImGui::End();

        if (folderMove.dropFolder)
        {
            if (folderMove.payload.isFolder)
            {
                auto removeIt = folderMove.payload.sourceFolder->childFolders.findIf(
                            [&](auto& f) { return f.get() == folderMove.payload.folderDragged; });
                assert(removeIt);
                folderMove.dropFolder->childFolders.push(move(*removeIt));
                folderMove.dropFolder->childFolders.back()->parent = folderMove.dropFolder;
                folderMove.payload.sourceFolder->childFolders.erase(removeIt);
            }
            else
            {
                folderMove.payload.sourceFolder->childResources.erase(
                        folderMove.payload.sourceFolder->childResources.find(
                            folderMove.payload.resourceDragged->guid));
                folderMove.dropFolder->childResources.push(
                        folderMove.payload.resourceDragged->guid);
            }
        }
        folderMove.dropFolder = nullptr;
    }

    for (OpenedResource const& r : openedResources)
    {
        r.editor->onUpdate(r.resource, this, renderer, deltaTime, r.windowID);
    }

    // TODO: fix escape behavior (it only works when a window is focused)
    if (!ImGui::GetIO().WantCaptureKeyboard && g_input.isKeyPressed(KEY_ESCAPE))
    {
        ImGui::OpenPopup("Exit Editor");
    }

    renderer->getRenderWorld()->setClearColor(true, { 0.1f, 0.1f, 0.1f, 1.f });
    if (activeExclusiveEditor)
    {
        activeExclusiveEditor->onUpdate(activeExclusiveResource, this, renderer, deltaTime, 0);
    }
}
