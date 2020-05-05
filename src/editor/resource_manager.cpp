#include "resource_manager.h"
#include "../game.h"
#include "../scene.h"
#include "../mesh_renderables.h"
#include <filesystem>

static void sortResources(ResourceFolder& folder)
{
    std::sort(folder.childResources.begin(), folder.childResources.end(), [](i64 a, i64 b) {
        Resource* rA = g_res.resources.find(a)->second.get();
        Resource* rB = g_res.resources.find(b)->second.get();
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
        auto res = g_res.resources.find(r.first);
        if (res != g_res.resources.end())
        {
            std::string filename = str(DATA_DIRECTORY, "/", std::hex, r.first, ".dat", std::dec);
            print("Saving resource ", filename, '\n');
            Serializer::toFile(*res->second, filename);
        }
    }
    resourcesModified.clear();
    Serializer::toFile(resources, str(DATA_DIRECTORY, "/", METADATA_FILE));
}

ResourceManager::ResourceManager()
{
    Serializer::fromFile(resources, str(DATA_DIRECTORY, "/", METADATA_FILE));

    std::map<i64, bool> resourceFolderMap;
    std::vector<ResourceFolder*> folders = { &resources };
    while (folders.size() > 0)
    {
        ResourceFolder* folder = folders.back();
        folders.pop_back();
        for (auto& childResource : folder->childResources)
        {
            resourceFolderMap[childResource] = true;
        }
        for (auto& childFolder : folder->childFolders)
        {
            folders.push_back(childFolder.get());
        }
    }

    for (auto& res : g_res.resources)
    {
        if (resourceFolderMap.find(res.first) == resourceFolderMap.end())
        {
            resources.childResources.push_back(res.first);
        }
    }

    sortResources(resources);
}

void ResourceManager::showFolder(ResourceFolder* folder)
{
    u32 flags = folder == &resources ? ImGuiTreeNodeFlags_DefaultOpen : 0;
    if (folder == renameFolder)
    {
        bool isFolderOpen = ImGui::TreeNodeEx((void*)folder, flags, "");
        ImGui::SameLine();
        static u32 renameID = 0;
        if (firstFrameRename)
        {
            renameID++;
            ImGui::SetKeyboardFocusHere();
        }
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0));
        ImGui::PushID(tstr("Rename ", renameID));
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
        bool isFolderOpen = ImGui::TreeNodeEx((void*)folder, flags, "%s", folder->name.c_str());
        if (folder->parent && ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            DragDropPayload payload = {};
            payload.isFolder = true;
            payload.folderDragged = folder;
            payload.sourceFolder = folder->parent;
            ImGui::SetDragDropPayload("Drag Resource", &payload, sizeof(payload));
            ImGui::Text(folder->name.c_str());
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
            if (ImGui::MenuItem("New Folder"))
            {
                static u32 folderCount = 0;
                auto newFolder = std::make_unique<ResourceFolder>();
                newFolder->name = str("New Folder ", folderCount++);
                newFolder->parent = folder;
                folder->childFolders.push_back(std::move(newFolder));
            }
            if (ImGui::MenuItem("New Texture"))
            {
                folder->childResources.push_back(newResource(ResourceType::TEXTURE)->guid);
            }
            if (ImGui::MenuItem("New Model"))
            {
                folder->childResources.push_back(newResource(ResourceType::MODEL)->guid);
            }
            if (ImGui::MenuItem("New Material"))
            {
                folder->childResources.push_back(newResource(ResourceType::MATERIAL)->guid);
            }
            if (ImGui::MenuItem("New Sound"))
            {
                folder->childResources.push_back(newResource(ResourceType::SOUND)->guid);
            }
            if (ImGui::MenuItem("New Track"))
            {
                folder->childResources.push_back(newResource(ResourceType::TRACK)->guid);
            }
            if (ImGui::MenuItem("Rename"))
            {
                renameText = folder->name;
                renameFolder = folder;
                firstFrameRename = true;
            }
            if (ImGui::MenuItem("Delete"))
            {
                // TODO
            }
            ImGui::EndPopup();
        }
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
}

void ResourceManager::showFolderContents(ResourceFolder* folder)
{
    const u32 selectedColor = 0x992299EE;
    for (auto& childGUID : folder->childResources)
    {
        Resource* childResource = g_res.resources.find(childGUID)->second.get();
        ImGui::PushStyleColor(ImGuiCol_Header, selectedColor);
        if (childResource == renameResource)
        {
            ImGui::PushID("Rename Node");
            ImGui::TreeNodeEx("", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
            ImGui::PopID();
            ImGui::SameLine();
            static u32 renameID = 0;
            ImGui::PushID(tstr("Rename ", renameID));
            if (firstFrameRename)
            {
                renameID++;
                ImGui::SetKeyboardFocusHere();
            }
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 0));
            if (ImGui::InputText("", &renameText,
                    ImGuiInputTextFlags_EnterReturnsTrue))
            {
                g_res.resourceNameMap.erase(renameText);
                g_res.resourceNameMap[renameText] = renameResource;
                renameResource->name = renameText;
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
            /*
            if (selectedTexture == tex && isTextureWindowOpen)
            {
                flags |= ImGuiTreeNodeFlags_Selected;
            }
            */
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
                ImGui::Text(childResource->name.c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::MenuItem("Duplicate"))
                {
                    // TODO
                }
                if (ImGui::MenuItem("Rename"))
                {
                    renameText = childResource->name;
                    renameResource = childResource;
                    firstFrameRename = true;
                }
                if (ImGui::MenuItem("Delete"))
                {
                    // TODO
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 0);
            ImGui::SetCursorPosX(cursorPos + 4.f);
            const char* icons[] = {
                "not_used",
                "texture_icon",
                "model_icon",
                "sound_icon",
                "icon_track",
                "icon_track",
                "material_icon",
            };
            ImGui::Image((void*)(uintptr_t)g_res.getTexture(
                        icons[(u32)childResource->type])->getPreviewHandle(), { 16, 16 });
            ImGui::SameLine(0, 0);
            ImGui::SetCursorPosX(cursorPos + 24.f);
            ImGui::Text(childResource->name.c_str());
        }
        ImGui::PopStyleColor();
    }
}

void ResourceManager::openResource(Resource* resource)
{
    switch (resource->type)
    {
        case ResourceType::TEXTURE:
            isTextureWindowOpen = true;
            selectedTexture = (Texture*)resource;
            break;
        case ResourceType::SOUND:
            isSoundWindowOpen = true;
            selectedSound = (Sound*)resource;
            break;
        case ResourceType::MATERIAL:
            isMaterialWindowOpen = true;
            selectedMaterial = (Material*)resource;
            break;
        case ResourceType::MODEL:
            activeEditor = ResourceType::MODEL;
            g_game.unloadScene();
            modelEditor.setModel((Model*)resource);
            break;
        case ResourceType::TRACK:
            activeEditor = ResourceType::TRACK;
            trackEditor.reset();
            g_game.changeScene(resource->guid);
            break;
        case ResourceType::FONT:
            break;
    }
}

Resource* ResourceManager::newResource(ResourceType type)
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
        case ResourceType::FONT:
        {
        } break;
    }
    if (resource)
    {
        resource->type = type;
        resource->guid = g_res.generateGUID();
        resource->name = str(namePrefix, ' ', g_res.resources.size());
        g_res.addResource(std::unique_ptr<Resource>(resource));
    }
    return resource;
}

void ResourceManager::onUpdate(Renderer *renderer, f32 deltaTime)
{
    if (g_game.currentScene && g_game.currentScene->isRaceInProgress)
    {
        if (g_input.isKeyPressed(KEY_ESCAPE) || g_input.isKeyPressed(KEY_F5))
        {
            g_game.currentScene->stopRace();
            trackEditor.onEndTestDrive(g_game.currentScene.get());
        }
        return;
    }

    renderer->getRenderWorld()->setHighlightColor(0, glm::vec4(1.f, 0.65f, 0.1f, 1.f));

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            // TODO: Make the shortcut actually work
            if (ImGui::MenuItem("Save", "Ctrl+S"))
            {
                saveResources();
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

    if (isResourceWindowOpen)
    {
        if (ImGui::Begin("Resources", &isResourceWindowOpen))
        {
            showFolder(&resources);
        }
        ImGui::End();

        if (folderMove.dropFolder)
        {
            if (folderMove.payload.isFolder)
            {
                auto removeIt = std::find_if(folderMove.payload.sourceFolder->childFolders.begin(),
                                folderMove.payload.sourceFolder->childFolders.end(),
                                [&](auto& f) { return f.get() == folderMove.payload.folderDragged; });
                assert (removeIt != folderMove.payload.sourceFolder->childFolders.end());
                folderMove.dropFolder->childFolders.push_back(std::move(*removeIt));
                folderMove.dropFolder->childFolders.back()->parent = folderMove.dropFolder;
                folderMove.payload.sourceFolder->childFolders.erase(removeIt);
            }
            else
            {
                folderMove.payload.sourceFolder->childResources.erase(
                        std::find(folderMove.payload.sourceFolder->childResources.begin(),
                                folderMove.payload.sourceFolder->childResources.end(),
                                folderMove.payload.resourceDragged->guid));
                folderMove.dropFolder->childResources.push_back(folderMove.payload.resourceDragged->guid);
            }
        }
        folderMove.dropFolder = nullptr;
    }

    showTextureWindow(renderer, deltaTime);
    showMaterialWindow(renderer, deltaTime);
    showSoundWindow(renderer, deltaTime);

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

    // TODO: fix escape behavior (it only works when a window is focused)
    if (!ImGui::GetIO().WantCaptureKeyboard && g_input.isKeyPressed(KEY_ESCAPE))
    {
        ImGui::OpenPopup("Exit Editor");
    }

    if (activeEditor == ResourceType::TRACK && g_game.currentScene)
    {
        trackEditor.onUpdate(g_game.currentScene.get(), renderer, deltaTime);
        renderer->getRenderWorld()->setClearColor(true);
    }
    else if (activeEditor == ResourceType::MODEL)
    {
        modelEditor.onUpdate(renderer, deltaTime);
        renderer->getRenderWorld()->setClearColor(true, { 0.1f, 0.1f, 0.1f, 1.f });
    }
    else
    {
        renderer->getRenderWorld()->setClearColor(true, { 0.1f, 0.1f, 0.1f, 1.f });
    }
}

void ResourceManager::showTextureWindow(Renderer* renderer, f32 deltaTime)
{
    if (!isTextureWindowOpen)
    {
        return;
    }

    bool dirty = false;
    Texture& tex = *selectedTexture;
    bool changed = false;

    if (ImGui::Begin("Texture Properties", &isTextureWindowOpen))
    {
        ImGui::PushItemWidth(150);
        dirty |= ImGui::InputText("##Name", &tex.name);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (tex.getSourceFileCount() == 1)
        {
            if (ImGui::Button("Load Image"))
            {
                std::string filename = chooseFile(true, "Image Files", { "*.png", "*.jpg", "*.bmp" },
                        str(ASSET_DIRECTORY, "/textures"));
                if (!filename.empty())
                {
                    tex.setSourceFile(0, std::filesystem::relative(filename));
                    tex.regenerate();
                    dirty = true;
                }
            }
            ImGui::SameLine();
        }
        if (ImGui::Button("Reimport"))
        {
            tex.reloadSourceFiles();
            dirty = true;
        }

        ImGui::Gap();

        if (tex.getSourceFileCount() == 1 && tex.getPreviewHandle())
        {
            ImGui::BeginChild("Texture Preview",
                    { ImGui::GetWindowWidth(), (f32)glm::min(tex.height, 300u) }, false,
                    ImGuiWindowFlags_HorizontalScrollbar);
            // TODO: Add controls to pan the image view and zoom in and out
            ImGui::Image((void*)(uintptr_t)tex.getPreviewHandle(), ImVec2(tex.width, tex.height));
            ImGui::EndChild();

            ImGui::Text(tex.getSourceFile(0).path.c_str());
            ImGui::Text("%i x %i", tex.width, tex.height);
        }
        else if (tex.getSourceFileCount() > 1)
        {
            for (u32 i=0; i<tex.getSourceFileCount(); ++i)
            {
                ImGui::Columns(2, NULL, false);
                ImGui::SetColumnWidth(0, 80);
                auto const& sf = tex.getSourceFile(i);
                f32 ratio = 72.f / sf.width;
                ImGui::Image((void*)(uintptr_t)sf.previewHandle, ImVec2(sf.width * ratio, sf.height * ratio));
                ImGui::NextColumn();
                if (ImGui::Button("Load File"))
                {
                    std::string filename = chooseFile(true, "Image Files", { "*.png", "*.jpg", "*.bmp" },
                                str(ASSET_DIRECTORY, "/textures"));
                    if (!filename.empty())
                    {
                        tex.setSourceFile(i, std::filesystem::relative(filename));
                        tex.regenerate();
                        dirty = true;
                    }
                }
                if (!sf.path.empty())
                {
                    ImGui::Text(sf.path.c_str());
                }
                if (sf.width > 0 && sf.height > 0)
                {
                    ImGui::Text("%i x %i", sf.width, sf.height);
                }
                ImGui::Columns(1);
            }
            ImGui::Gap();
        }

        ImGui::Text(selectedTexture->name.c_str());

        const char* textureTypeNames = "Color\0Grayscale\0Normal Map\0Cube Map\0";
        i32 textureType = tex.getTextureType();
        changed |= ImGui::Combo("Type", &textureType, textureTypeNames);
        if (textureType != tex.getTextureType())
        {
            tex.setTextureType(textureType);
        }

        changed |= ImGui::Checkbox("Repeat", &tex.repeat);
        changed |= ImGui::Checkbox("Generate Mip Maps", &tex.generateMipMaps);
        changed |= ImGui::InputFloat("LOD Bias", &tex.lodBias, 0.1f);

        changed |= ImGui::InputInt("Anisotropy", &tex.anisotropy);
        tex.anisotropy = clamp(tex.anisotropy, 0, 16);

        const char* filterNames = "Nearest\0Bilinear\0Trilinear\0";
        changed |= ImGui::Combo("Filtering", &tex.filter, filterNames);

        ImGui::End();
    }

    if (changed)
    {
        tex.regenerate();
        dirty = true;
    }

    if (dirty)
    {
        markDirty(tex.guid);
    }
}

void ResourceManager::showMaterialWindow(Renderer* renderer, f32 deltaTime)
{
    if (!isMaterialWindowOpen)
    {
        return;
    }

    Material& mat = *selectedMaterial;

    static RenderWorld rw;
    static i32 previewMeshIndex = 0;

    rw.setName("Material Preview");
    rw.setSize(200, 200);
    const char* previewMeshes[] = { "world.Sphere", "world.UnitCube", "world.Quad" };
    Mesh* previewMesh = g_res.getModel("misc")->getMeshByName(previewMeshes[previewMeshIndex]);
    rw.addDirectionalLight(glm::vec3(-0.5f, 0.2f, -1.f), glm::vec3(1.5f));
    rw.setViewportCount(1);
    rw.updateWorldTime(30.f);
    rw.setClearColor(true, { 0.05f, 0.05f, 0.05f, 1.f });
    rw.setViewportCamera(0, glm::vec3(8.f, 8.f, 10.f),
            glm::vec3(0.f, 0.f, 1.f), 1.f, 200.f, 40.f);
    glm::mat4 transform = glm::scale(glm::mat4(1.f), glm::vec3(3.5f));
    rw.push(LitMaterialRenderable(previewMesh, transform, selectedMaterial));

    renderer->addRenderWorld(&rw);

    bool dirty = false;
    if (ImGui::Begin("Material Properties", &isMaterialWindowOpen))
    {
        ImGui::PushItemWidth(200);
        dirty |= ImGui::InputText("##Name", &mat.name);
        ImGui::PopItemWidth();
        ImGui::Gap();

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 208);
        ImGui::Image((void*)(uintptr_t)rw.getTexture()->handle, { 200, 200 }, { 1.f, 1.f }, { 0.f, 0.f });
        ImGui::NextColumn();

        ImGui::Text(selectedMaterial->name.c_str());

        const char* materialTypeNames = "Lit\0Unlit\0";
        dirty |= ImGui::Combo("Type", (i32*)&mat.materialType, materialTypeNames);

        const char* previewMeshNames = "Sphere\0Box\0Plane\0";
        dirty |= ImGui::Combo("Preview", &previewMeshIndex, previewMeshNames);

        ImGui::Columns(1);
        ImGui::Gap();

        ImGui::Image((void*)(uintptr_t)g_res.getTexture(mat.colorTexture)->getPreviewHandle(), { 48, 48 });
        ImGui::SameLine();
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f - 56);
        ImVec2 size = { ImGui::GetWindowWidth() * 0.65f - 56, 300.f };
        ImGui::SetNextWindowSizeConstraints(size, size);
        if (ImGui::BeginCombo("Color Texture",
                    mat.colorTexture == 0 ? "None" : g_res.getTexture(mat.colorTexture)->name.c_str()))
        {
            dirty = true;

            static std::string searchString;
            static std::vector<Texture*> searchResults;
            searchResults.clear();

            if (ImGui::IsWindowAppearing())
            {
                ImGui::SetKeyboardFocusHere();
                searchString = "";
            }
            ImGui::PushItemWidth(ImGui::GetWindowWidth() - 32);
            bool enterPressed = ImGui::InputText("", &searchString, ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button("!", ImVec2(16, 0)))
            {
                if (getSelectedTexture())
                {
                    mat.colorTexture = getSelectedTexture()->guid;
                    ImGui::CloseCurrentPopup();
                }
            }

            for (auto& res : g_res.resources)
            {
                if (res.second->type != ResourceType::TEXTURE)
                {
                    continue;
                }
                Texture* tex = (Texture*)res.second.get();
                if (tex->getTextureType() == TextureType::COLOR)
                {
                    if (searchString.empty() || tex->name.find(searchString) != std::string::npos)
                    {
                        searchResults.push_back(tex);
                    }
                }
            }
            std::sort(searchResults.begin(), searchResults.end(), [](auto a, auto b) {
                return a->name < b->name;
            });

            if (enterPressed)
            {
                mat.colorTexture = searchResults[0]->guid;
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::BeginChild("Search Results", { 0, 0 }))
            {
                for (Texture* tex : searchResults)
                {
                    ImGui::Image((void*)(uintptr_t)tex->getPreviewHandle(), { 16, 16 });
                    ImGui::SameLine();
                    ImGui::PushID(tex->guid);
                    if (ImGui::Selectable(tex->name.c_str()))
                    {
                        mat.colorTexture = tex->guid;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopID();
                }
                ImGui::EndChild();
            }

            ImGui::EndCombo();
        }

        ImGui::Columns(2, nullptr, false);
        dirty |= ImGui::Checkbox("Culling", &mat.isCullingEnabled);
        dirty |= ImGui::Checkbox("Cast Shadow", &mat.castsShadow);
        dirty |= ImGui::Checkbox("Depth Read", &mat.isDepthReadEnabled);
        dirty |= ImGui::Checkbox("Depth Write", &mat.isDepthWriteEnabled);
        ImGui::NextColumn();
        dirty |= ImGui::Checkbox("Visible", &mat.isVisible);
        dirty |= ImGui::Checkbox("Wireframe", &mat.displayWireframe);
        dirty |= ImGui::Checkbox("Transparent", &mat.isTransparent);
        dirty |= ImGui::Checkbox("Vertex Colors", &mat.useVertexColors);
        ImGui::Columns(1);

        dirty |= ImGui::DragFloat("Alpha Cutoff", &mat.alphaCutoff, 0.005f, 0.f, 1.f);
        dirty |= ImGui::DragFloat("Shadow Alpha Cutoff", &mat.shadowAlphaCutoff, 0.005f, 0.f, 1.f);
        dirty |= ImGui::InputFloat("Depth Offset", &mat.depthOffset);

        dirty |= ImGui::ColorEdit3("Base Color", (f32*)&mat.color);
        dirty |= ImGui::ColorEdit3("Emit", (f32*)&mat.emit);
        dirty |= ImGui::DragFloat("Emit Strength", (f32*)&mat.emitPower, 0.01f, 0.f, 80.f);
        dirty |= ImGui::ColorEdit3("Specular Color", (f32*)&mat.specularColor);
        dirty |= ImGui::DragFloat("Specular Power", (f32*)&mat.specularPower, 0.05f, 0.f, 1000.f);
        dirty |= ImGui::DragFloat("Specular Strength", (f32*)&mat.specularStrength, 0.005f, 0.f, 1.f);

        dirty |= ImGui::DragFloat("Fresnel Scale", (f32*)&mat.fresnelScale, 0.005f, 0.f, 1.f);
        dirty |= ImGui::DragFloat("Fresnel Power", (f32*)&mat.fresnelPower, 0.009f, 0.f, 200.f);
        dirty |= ImGui::DragFloat("Fresnel Bias", (f32*)&mat.fresnelBias, 0.005f, -1.f, 1.f);

        dirty |= ImGui::DragFloat("Reflection Strength", (f32*)&mat.reflectionStrength, 0.005f, 0.f, 1.f);
        dirty |= ImGui::DragFloat("Reflection LOD", (f32*)&mat.reflectionLod, 0.01f, 0.f, 10.f);
        dirty |= ImGui::DragFloat("Reflection Bias", (f32*)&mat.reflectionBias, 0.005f, -1.f, 1.f);
        dirty |= ImGui::DragFloat("Wind", (f32*)&mat.windAmount, 0.01f, 0.f, 5.f);

        ImGui::End();
    }

    if (dirty)
    {
        markDirty(mat.guid);
    }
}

void ResourceManager::showSoundWindow(Renderer* renderer, f32 deltaTime)
{
    if (!isSoundWindowOpen)
    {
        return;
    }

    Sound& sound = *selectedSound;

    bool dirty = false;
    if (ImGui::Begin("Sound Properties", &isSoundWindowOpen))
    {
        ImGui::PushItemWidth(150);
        dirty |= ImGui::InputText("##Name", &sound.name);
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Load Sound"))
        {
            std::string filename =
                chooseFile(true, "Audio Files", { "*.wav", "*.ogg" }, str(ASSET_DIRECTORY, "/sounds"));
            if (!filename.empty())
            {
                sound.sourceFilePath = std::filesystem::relative(filename);
                sound.loadFromFile(sound.sourceFilePath.c_str());
            }
            dirty = true;
        }
        ImGui::SameLine();
        if (!sound.sourceFilePath.empty() && ImGui::Button("Reimport"))
        {
            sound.loadFromFile(sound.sourceFilePath.c_str());
            dirty = true;
        }

        ImGui::Gap();

        ImGui::Text(sound.sourceFilePath.c_str());
        ImGui::Text("Format: %s", sound.format == AudioFormat::RAW ? "WAV" : "OGG VORBIS");

        dirty |= ImGui::SliderFloat("Volume", &sound.volume, 0.f, 1.f);
        dirty |= ImGui::SliderFloat("Falloff Distance", &sound.falloffDistance, 50.f, 1000.f);

        ImGui::Gap();

        static SoundHandle previewSoundHandle = 0;
        if (ImGui::Button("Preview"))
        {
            g_audio.stopSound(previewSoundHandle);
            previewSoundHandle = g_audio.playSound(&sound, SoundType::MENU_SFX);
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            g_audio.stopSound(previewSoundHandle);
        }

        ImGui::End();
    }
    if (dirty)
    {
        markDirty(sound.guid);
    }
}
