#include "resource_manager.h"
#include "../game.h"
#include "../scene.h"
#include <filesystem>

ResourceManager::ResourceManager()
{
}

void ResourceManager::onUpdate(Renderer *renderer, f32 deltaTime)
{
    if (texturesStale)
    {
        for (auto& tex : g_res.textures)
        {
            textures.push_back(tex.second.get());
        }
        std::sort(textures.begin(), textures.end(), [](auto& a, auto& b) {
            return a->name < b->name;
        });
        texturesStale = false;
    }

    if (tracksStale)
    {
        for (auto& track : g_res.tracks)
        {
            tracks.push_back(&track.second);
        }
        std::sort(tracks.begin(), tracks.end(), [](auto& a, auto& b) {
            return a->dict().val()["name"].string().val() < b->dict().val()["name"].string().val();
        });
        tracksStale = false;
    }

    if (g_game.currentScene && g_game.currentScene->isRaceInProgress)
    {
        if (g_input.isKeyPressed(KEY_ESCAPE) || g_input.isKeyPressed(KEY_F5))
        {
            g_game.currentScene->stopRace();
            editor.onEndTestDrive(g_game.currentScene.get());
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
            }
            if (ImGui::MenuItem("New Sound"))
            {
            }
            if (ImGui::MenuItem("New Track"))
            {
                g_game.changeScene(nullptr);
                activeEditor = EditorType::SCENE;
                editor.reset();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (isResourceWindowOpen)
    {
        ImGui::Begin("Resources", &isResourceWindowOpen);

        if (ImGui::CollapsingHeader("Textures"))
        {
            for (auto tex : textures)
            {
                if (ImGui::Selectable(tex->name.c_str(),
                            selectedTexture == tex && isTextureWindowOpen))
                {
                    selectedTexture = tex;
                    editName = selectedTexture->name;
                    isTextureWindowOpen = true;
                }
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
                    ImGui::EndPopup();
                }
            }
        }

        if (ImGui::CollapsingHeader("Tracks"))
        {
            for (auto& track : tracks)
            {
                if (ImGui::Selectable(track->dict().val()["name"].string().val().c_str(),
                        g_game.currentScene &&
                        g_game.currentScene->guid == track->dict().val()["guid"].integer().val()))
                {
                    activeEditor = EditorType::SCENE;
                    g_game.changeScene(track->dict().val()["guid"].integer().val());
                    editor.reset();
                }
            }
        }

        ImGui::End();
    }

    showTextureWindow(renderer, deltaTime);

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

    if (activeEditor == EditorType::SCENE)
    {
        if (g_game.currentScene)
        {
            editor.onUpdate(g_game.currentScene.get(), renderer, deltaTime);
        }
        renderer->getRenderWorld()->setClearColor(true);
    }
    else
    {
        renderer->getRenderWorld()->setClearColor(true, { 0.1f, 0.1f, 0.1f, 1.f });
    }
}

void ResourceManager::newTexture(std::string const& filename)
{
    auto tex = std::make_unique<Texture>();
    tex->setTextureType(TextureType::COLOR);
    tex->guid = g_res.generateGUID();
    selectedTexture = tex.get();
    isTextureWindowOpen = true;
    if (filename.empty())
    {
        tex->name = str("Texture ", g_res.textures.size());
    }
    else
    {
        tex->name = std::filesystem::path(filename).stem().string();
        tex->setSourceFile(0, std::filesystem::relative(filename));
        tex->regenerate();
    }
    editName = tex->name;
    saveResource(*tex);
    g_res.textureNameMap[tex->name] = selectedTexture;
    g_res.textures[tex->guid] = std::move(tex);
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

    dirty |= ImGui::Checkbox("Used For Decal", &tex.usedForDecal);
    dirty |= ImGui::Checkbox("Used For Billboard", &tex.usedForBillboard);

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

