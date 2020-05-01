#include "resource_manager.h"
#include "../game.h"
#include "../scene.h"
#include "../mesh_renderables.h"
#include <filesystem>

ResourceManager::ResourceManager()
{
}

void ResourceManager::onUpdate(Renderer *renderer, f32 deltaTime)
{
    if (texturesStale)
    {
        textures.clear();
        for (auto& tex : g_res.textures)
        {
            textures.push_back(tex.second.get());
        }
        std::sort(textures.begin(), textures.end(), [](auto& a, auto& b) {
            return a->name < b->name;
        });
        texturesStale = false;
    }

    if (materialsStale)
    {
        materials.clear();
        for (auto& mat : g_res.materials)
        {
            materials.push_back(mat.second.get());
        }
        std::sort(materials.begin(), materials.end(), [](auto& a, auto& b) {
            return a->name < b->name;
        });
        materialsStale = false;
    }

    if (tracksStale)
    {
        tracks.clear();
        for (auto& track : g_res.tracks)
        {
            tracks.push_back(&track.second);
        }
        std::sort(tracks.begin(), tracks.end(), [](auto& a, auto& b) {
            return a->dict().val()["name"].string().val() < b->dict().val()["name"].string().val();
        });
        tracksStale = false;
    }

    if (modelsStale)
    {
        models.clear();
        for (auto& model : g_res.models)
        {
            models.push_back(model.second.get());
        }
        std::sort(models.begin(), models.end(), [](auto& a, auto& b) {
            return a->name < b->name;
        });
        modelsStale = false;
    }

    if (soundsStale)
    {
        sounds.clear();
        for (auto& sound : g_res.sounds)
        {
            sounds.push_back(sound.second.get());
        }
        std::sort(sounds.begin(), sounds.end(), [](auto& a, auto& b) {
            return a->name < b->name;
        });
        soundsStale = false;
    }

    if (g_game.currentScene && g_game.currentScene->isRaceInProgress)
    {
        if (g_input.isKeyPressed(KEY_ESCAPE) || g_input.isKeyPressed(KEY_F5))
        {
            g_game.currentScene->stopRace();
            trackEditor.onEndTestDrive(g_game.currentScene.get());
        }
        return;
    }

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Window"))
        {
            if (ImGui::MenuItem("Resources", "Alt+R", isResourceWindowOpen))
            {
                isResourceWindowOpen = !isResourceWindowOpen;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Resources"))
        {
            if (ImGui::MenuItem("New Texture"))
            {
                newTexture();
            }
            if (ImGui::MenuItem("New Material"))
            {
                newMaterial();
            }
            if (ImGui::MenuItem("New Sound"))
            {
                newSound();
            }
            if (ImGui::MenuItem("New Model"))
            {
                newModel();
            }
            if (ImGui::MenuItem("New Track"))
            {
                g_game.changeScene(nullptr);
                activeEditor = ResourceType::TRACK;
                trackEditor.reset();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (isResourceWindowOpen)
    {
        const u32 selectedColor = 0x992299EE;

        ImGui::Begin("Resources", &isResourceWindowOpen);

        if (ImGui::CollapsingHeader("Textures"))
        {
            ImGui::PushStyleColor(ImGuiCol_Header, selectedColor);
            for (auto tex : textures)
            {
                if (tex == renameTexture)
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
                        renameTexture->name = renameText;
                    }
                    ImGui::PopStyleVar();
                    if (!firstFrameRename && !ImGui::IsItemActive())
                    {
                        renameTexture = nullptr;
                    }
                    ImGui::PopID();
                    firstFrameRename = false;
                }
                else
                {
                    u32 flags = ImGuiTreeNodeFlags_Leaf
                        | ImGuiTreeNodeFlags_NoTreePushOnOpen
                        | ImGuiTreeNodeFlags_SpanAvailWidth;
                    if (selectedTexture == tex && isTextureWindowOpen)
                    {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }
                    ImGui::PushID((void*)tex->guid);
                    ImGui::TreeNodeEx(tex->name.c_str(), flags);
                    if (ImGui::IsItemClicked())
                    {
                        if (ImGui::IsMouseDoubleClicked(0))
                        {
                            selectedTexture = tex;
                            editName = selectedTexture->name;
                            isTextureWindowOpen = true;
                        }
                    }
                    ImGui::PopID();
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::MenuItem("New Texture"))
                        {
                            newTexture();
                        }
                        if (ImGui::MenuItem("Duplicate"))
                        {
                            // TODO
                        }
                        if (ImGui::MenuItem("Rename"))
                        {
                            renameText = tex->name;
                            renameTexture = tex;
                            firstFrameRename = true;
                        }
                        if (ImGui::MenuItem("Delete"))
                        {
                            // TODO
                        }
                        ImGui::EndPopup();
                    }
                }
            }
            ImGui::PopStyleColor();
        }

        if (ImGui::CollapsingHeader("Materials"))
        {
            ImGui::PushStyleColor(ImGuiCol_Header, selectedColor);
            for (auto mat : materials)
            {
                u32 flags = ImGuiTreeNodeFlags_Leaf
                    | ImGuiTreeNodeFlags_NoTreePushOnOpen
                    | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selectedMaterial == mat && isMaterialWindowOpen)
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }
                ImGui::TreeNodeEx(mat->name.c_str(), flags);
                if (ImGui::IsItemClicked())
                {
                    selectedMaterial = mat;
                    editName = selectedMaterial->name;
                    isMaterialWindowOpen = true;
                }
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("New Material"))
                    {
                        newMaterial();
                    }
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        // TODO
                    }
                    if (ImGui::MenuItem("Rename"))
                    {
                        editName = mat->name;
                        // TODO
                    }
                    if (ImGui::MenuItem("Delete"))
                    {
                        // TODO
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::PopStyleColor();
        }

        if (ImGui::CollapsingHeader("Models"))
        {
            ImGui::PushStyleColor(ImGuiCol_Header, selectedColor);
            for (auto& model : models)
            {
                u32 flags = ImGuiTreeNodeFlags_Leaf
                    | ImGuiTreeNodeFlags_NoTreePushOnOpen
                    | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (activeEditor == ResourceType::MODEL && model == modelEditor.getCurrentModel())
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }
                ImGui::TreeNodeEx(model->name.c_str(), flags);
                if (ImGui::IsItemClicked())
                {
                    activeEditor = ResourceType::MODEL;
                    g_game.unloadScene();
                    modelEditor.setModel(model);
                }
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("New Model"))
                    {
                        newModel();
                    }
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        // TODO
                    }
                    if (ImGui::MenuItem("Rename"))
                    {
                        editName = model->name;
                        // TODO
                    }
                    if (ImGui::MenuItem("Delete"))
                    {
                        // TODO
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::PopStyleColor();
        }

        if (ImGui::CollapsingHeader("Sounds"))
        {
            ImGui::PushStyleColor(ImGuiCol_Header, selectedColor);
            for (auto sound : sounds)
            {
                u32 flags = ImGuiTreeNodeFlags_Leaf
                    | ImGuiTreeNodeFlags_NoTreePushOnOpen
                    | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (selectedSound == sound && isSoundWindowOpen)
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }
                ImGui::TreeNodeEx(sound->name.c_str(), flags);
                if (ImGui::IsItemClicked())
                {
                    selectedSound = sound;
                    editName = sound->name;
                    isSoundWindowOpen = true;
                }
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("New Sound"))
                    {
                        newSound();
                    }
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        // TODO
                    }
                    if (ImGui::MenuItem("Rename"))
                    {
                        editName = sound->name;
                        // TODO
                    }
                    if (ImGui::MenuItem("Delete"))
                    {
                        // TODO
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::PopStyleColor();
        }

        if (ImGui::CollapsingHeader("Tracks"))
        {
            ImGui::PushStyleColor(ImGuiCol_Header, selectedColor);
            for (auto& track : tracks)
            {
                u32 flags = ImGuiTreeNodeFlags_Leaf
                    | ImGuiTreeNodeFlags_NoTreePushOnOpen
                    | ImGuiTreeNodeFlags_SpanAvailWidth;
                if (g_game.currentScene &&
                        g_game.currentScene->guid == track->dict().val()["guid"].integer().val())
                {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }
                ImGui::TreeNodeEx(track->dict().val()["name"].string().val().c_str(), flags);
                if (ImGui::IsItemClicked())
                {
                    activeEditor = ResourceType::TRACK;
                    g_game.changeScene(track->dict().val()["guid"].integer().val());
                    trackEditor.reset();
                }
                if (ImGui::BeginPopupContextItem())
                {
                    if (ImGui::MenuItem("New Track"))
                    {
                        // TODO
                    }
                    if (ImGui::MenuItem("Duplicate"))
                    {
                        // TODO
                    }
                    if (ImGui::MenuItem("Rename"))
                    {
                        editName = track->dict().val()["name"].string("");
                        // TODO
                    }
                    if (ImGui::MenuItem("Delete"))
                    {
                        // TODO
                    }
                    ImGui::EndPopup();
                }
            }
            ImGui::PopStyleColor();
        }

        ImGui::End();
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

void ResourceManager::newModel()
{
    activeEditor = ResourceType::MODEL;
    auto model = std::make_unique<Model>();
    model->guid = g_res.generateGUID();
    model->name = str("Model ", models.size());
    modelEditor.setModel(model.get());
    saveResource(*model);
    g_res.modelNameMap[model->name] = model.get();
    g_res.models[model->guid] = std::move(model);
    modelsStale = true;
    g_game.unloadScene();
}

void ResourceManager::newTexture()
{
    auto tex = std::make_unique<Texture>();
    tex->setTextureType(TextureType::COLOR);
    tex->guid = g_res.generateGUID();
    selectedTexture = tex.get();
    isTextureWindowOpen = true;
    tex->name = str("Texture ", g_res.textures.size());
    editName = tex->name;
    saveResource(*tex);
    g_res.textureNameMap[tex->name] = selectedTexture;
    g_res.textures[tex->guid] = std::move(tex);
    texturesStale = true;
}

void ResourceManager::newMaterial()
{
    auto mat = std::make_unique<Material>();
    mat->guid = g_res.generateGUID();
    selectedMaterial = mat.get();
    isMaterialWindowOpen = true;
    mat->name = str("Material ", g_res.materials.size());
    editName = mat->name;
    saveResource(*mat);
    g_res.materialNameMap[mat->name] = selectedMaterial;
    g_res.materials[mat->guid] = std::move(mat);
    materialsStale = true;
}

void ResourceManager::newSound()
{
    auto sound = std::make_unique<Sound>();
    sound->guid = g_res.generateGUID();
    selectedSound = sound.get();
    isSoundWindowOpen = true;
    sound->name = str("Sound ", g_res.sounds.size());
    editName = sound->name;
    saveResource(*sound);
    g_res.soundNameMap[sound->name] = selectedSound;
    g_res.sounds[sound->guid] = std::move(sound);
    texturesStale = true;
}

void ResourceManager::showTextureWindow(Renderer* renderer, f32 deltaTime)
{
    if (!isTextureWindowOpen)
    {
        return;
    }

    bool dirty = false;
    Texture& tex = *selectedTexture;

    ImGui::Begin("Texture Properties", &isTextureWindowOpen);

    if (tex.getSourceFileCount() == 1)
    {
        if (ImGui::Button("Load Image"))
        {
            std::string filename = chooseFile(".", true, "Image Files", { "*.png", "*.jpg", "*.bmp" });
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
                std::string filename = chooseFile(".", true, "Image Files", { "*.png", "*.jpg", "*.bmp" });
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

    if (ImGui::InputText("Name", &editName, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        tex.name = editName;
        g_res.textureNameMap[tex.name] = selectedTexture;
        dirty = true;
    }
    if (!ImGui::IsItemFocused())
    {
        editName = tex.name;
    }

    const char* textureTypeNames = "Color\0Grayscale\0Normal Map\0Cube Map\0";
    bool changed = false;
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

    if (changed)
    {
        tex.regenerate();
        dirty = true;
    }

    if (dirty)
    {
        saveResource(tex);
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

    ImGui::Begin("Material Properties", &isMaterialWindowOpen);

    // TODO: Add keyboard shortcut
    if (ImGui::Button("Save"))
    {
        saveResource(mat);
    }
    //ImGui::SameLine();
    ImGui::Gap();

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 208);
    ImGui::Image((void*)(uintptr_t)rw.getTexture()->handle, { 200, 200 }, { 1.f, 1.f }, { 0.f, 0.f });
    ImGui::NextColumn();

    if (ImGui::InputText("Name", &editName, ImGuiInputTextFlags_EnterReturnsTrue))
    {
        mat.name = editName;
        g_res.materialNameMap[mat.name] = selectedMaterial;
    }
    if (!ImGui::IsItemFocused())
    {
        editName = mat.name;
    }

    const char* materialTypeNames = "Lit\0Unlit\0";
    ImGui::Combo("Type", (i32*)&mat.materialType, materialTypeNames);

    const char* previewMeshNames = "Sphere\0Box\0Plane\0";
    ImGui::Combo("Preview", &previewMeshIndex, previewMeshNames);

    ImGui::Columns(1);
    ImGui::Gap();

    ImGui::Image((void*)(uintptr_t)g_res.getTexture(mat.colorTexture)->getPreviewHandle(), { 48, 48 });
    ImGui::SameLine();
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f - 56);
    if (ImGui::BeginCombo("Color Texture",
                mat.colorTexture == 0 ? "None" : g_res.getTexture(mat.colorTexture)->name.c_str()))
    {
        for (auto& tex : g_res.textures)
        {
            if (tex.second->getTextureType() == TextureType::COLOR)
            {
                ImGui::Image((void*)(uintptr_t)tex.second->getPreviewHandle(), { 16, 16 });
                ImGui::SameLine();
                if (ImGui::Selectable(tex.second->name.c_str()))
                {
                    mat.colorTexture = tex.first;
                }
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Columns(2, nullptr, false);
    ImGui::Checkbox("Culling", &mat.isCullingEnabled);
    ImGui::Checkbox("Cast Shadow", &mat.castsShadow);
    ImGui::Checkbox("Depth Read", &mat.isDepthReadEnabled);
    ImGui::Checkbox("Depth Write", &mat.isDepthWriteEnabled);
    ImGui::NextColumn();
    ImGui::Checkbox("Visible", &mat.isVisible);
    ImGui::Checkbox("Wireframe", &mat.displayWireframe);
    ImGui::Checkbox("Transparent", &mat.isTransparent);
    ImGui::Checkbox("Vertex Colors", &mat.useVertexColors);
    ImGui::Columns(1);

    ImGui::DragFloat("Alpha Cutoff", &mat.alphaCutoff, 0.005f, 0.f, 1.f);
    ImGui::DragFloat("Shadow Alpha Cutoff", &mat.shadowAlphaCutoff, 0.005f, 0.f, 1.f);
    ImGui::InputFloat("Depth Offset", &mat.depthOffset);

    ImGui::ColorEdit3("Base Color", (f32*)&mat.color);
    ImGui::ColorEdit3("Emit", (f32*)&mat.emit);
    ImGui::DragFloat("Emit Strength", (f32*)&mat.emitPower, 0.01f, 0.f, 80.f);
    ImGui::ColorEdit3("Specular Color", (f32*)&mat.specularColor);
    ImGui::DragFloat("Specular Power", (f32*)&mat.specularPower, 0.05f, 0.f, 1000.f);
    ImGui::DragFloat("Specular Strength", (f32*)&mat.specularStrength, 0.005f, 0.f, 1.f);

    ImGui::DragFloat("Fresnel Scale", (f32*)&mat.fresnelScale, 0.005f, 0.f, 1.f);
    ImGui::DragFloat("Fresnel Power", (f32*)&mat.fresnelPower, 0.009f, 0.f, 200.f);
    ImGui::DragFloat("Fresnel Bias", (f32*)&mat.fresnelBias, 0.005f, -1.f, 1.f);

    ImGui::DragFloat("Reflection Strength", (f32*)&mat.reflectionStrength, 0.005f, 0.f, 1.f);
    ImGui::DragFloat("Reflection LOD", (f32*)&mat.reflectionLod, 0.01f, 0.f, 10.f);
    ImGui::DragFloat("Reflection Bias", (f32*)&mat.reflectionBias, 0.005f, -1.f, 1.f);
    ImGui::DragFloat("Wind", (f32*)&mat.windAmount, 0.01f, 0.f, 5.f);

    ImGui::End();
}

void ResourceManager::showSoundWindow(Renderer* renderer, f32 deltaTime)
{
    if (!isSoundWindowOpen)
    {
        return;
    }

    Sound& sound = *selectedSound;

    if (ImGui::Begin("Sound Properties", &isSoundWindowOpen))
    {
        if (ImGui::Button("Save"))
        {
            saveResource(sound);
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Sound"))
        {
            std::string filename = chooseFile(".", true, "Audio Files", { "*.wav", "*.ogg" });
            if (!filename.empty())
            {
                sound.sourceFilePath = std::filesystem::relative(filename);
                sound.loadFromFile(sound.sourceFilePath.c_str());
            }
        }
        ImGui::SameLine();
        if (!sound.sourceFilePath.empty() && ImGui::Button("Reimport"))
        {
            sound.loadFromFile(sound.sourceFilePath.c_str());
        }

        ImGui::Gap();

        ImGui::Text(sound.sourceFilePath.c_str());
        ImGui::Text("Format: %s", sound.format == AudioFormat::RAW ? "WAV" : "OGG VORBIS");

        if (ImGui::InputText("Name", &editName, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            sound.name = editName;
            g_res.soundNameMap[sound.name] = selectedSound;
        }
        if (!ImGui::IsItemFocused())
        {
            editName = sound.name;
        }

        ImGui::SliderFloat("Volume", &sound.volume, 0.f, 1.f);
        ImGui::SliderFloat("Falloff Distance", &sound.falloffDistance, 50.f, 1000.f);

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
}
