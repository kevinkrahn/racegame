#include "resource_manager.h"
#include "../game.h"
#include "../scene.h"

ResourceManager::ResourceManager()
{
}

void ResourceManager::onUpdate(Renderer *renderer, f32 deltaTime)
{
    if (filesStale)
    {
        files = readDirectory("../assets");
        filesStale = false;
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
        ImGui::EndMainMenuBar();
    }

    if (isResourceWindowOpen)
    {
        ImGui::Begin("Resources", &isResourceWindowOpen);

        for (auto& file : files)
        {
            drawFile(file);
        }

        /*
        if (ImGui::Selectable(file.path.c_str()))
        {
            activeEditor = EditorType::SCENE;
            g_game.changeScene(file.path.c_str());
            editor.reset();
        }
        */

        ImGui::End();
    }

    //showTextureWindow(renderer, deltaTime);

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

#if 0
void ResourceManager::showTextureWindow(Renderer* renderer, f32 deltaTime)
{
    if (isTextureWindowOpen)
    {
        Texture& tex = textures[textureViewIndex];

        ImGui::Begin("Texture Properties", &isTextureWindowOpen);

        if (ImGui::Button("Open File"))
        {
        }
        ImGui::SameLine();
        if (ImGui::Button("Reload File"))
        {
        }

        if (tex.handle)
        {
            ImGui::BeginChild("Entites",
                    { ImGui::GetWindowWidth(), (f32)glm::min(tex.height, 300u) }, false,
                    ImGuiWindowFlags_HorizontalScrollbar);
            // TODO: Add controls to pan the image view and zoom in and out
            ImGui::Image((void*)(uintptr_t)tex.handle, ImVec2(tex.width, tex.height));
            ImGui::EndChild();
        }

        ImGui::Text(tex.sourceFile.c_str());
        ImGui::Text("%i x %i", tex.width, tex.height);
        ImGui::InputText("Name", &tex.name);
        ImGui::Checkbox("Used For Decal", &tex.usedForDecal);
        ImGui::Checkbox("Used For Billboard", &tex.usedForBillboard);

        bool changed = false;
        changed |= ImGui::Checkbox("SRGB", &tex.srgb);
        ImGui::HelpMarker("Textures that contain non-color data (such as normal maps and heightmaps) should not be encoded as SRGB.");
        changed |= ImGui::Checkbox("Repeat", &tex.repeat);
        changed |= ImGui::Checkbox("Generate Mip Maps", &tex.generateMipMaps);
        changed |= ImGui::InputFloat("LOD Bias", &tex.lodBias, 0.1f);

        changed |= ImGui::InputInt("Anisotropy", &tex.anisotropy);
        tex.anisotropy = clamp(tex.anisotropy, 0, 16);

        const char* filterNames = { "Nearest\0Bilinear\0Trilinear" };
        changed |= ImGui::Combo("Texture Filtering", &tex.filter, filterNames);

        ImGui::End();

        if (changed)
        {
            tex.reload();
        }
    }
}
#endif

void ResourceManager::drawFile(FileItem& file)
{
    if (file.isDirectory)
    {
        if (ImGui::TreeNodeEx(file.path.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth))
        {
            for (auto& file : file.children)
            {
                drawFile(file);
            }
            ImGui::TreePop();
        }
    }
    else
    {
        ImGui::TreeNodeEx(file.path.c_str(),
                ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen);
        if (ImGui::IsItemClicked())
        {
            auto index = file.path.find_last_of('.');
            if (index != std::string::npos && file.path.substr(index) == ".dat")
            {
                activeEditor = EditorType::SCENE;
                g_game.changeScene(tstr("tracks/", file.path));
                editor.reset();
            }
        }
    }
}
