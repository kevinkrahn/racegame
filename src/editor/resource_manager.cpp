#include "resource_manager.h"
#include "../game.h"
#include "../scene.h"

ResourceManager::ResourceManager()
{
    tracks = readDirectory("tracks", ".dat");
}

void ResourceManager::onUpdate(Renderer *renderer, f32 deltaTime)
{
    if (g_game.currentScene && g_game.currentScene->isRaceInProgress)
    {
        if (g_input.isKeyPressed(KEY_ESCAPE) || g_input.isKeyPressed(KEY_F5))
        {
            g_game.currentScene->stopRace();
            editor.onEndTestDrive(g_game.currentScene.get());
        }
        return;
    }

    ImGui::Begin("Resources");

    if (ImGui::CollapsingHeader("Textures"))
    {
        ImGui::Selectable("Texture 1");
        ImGui::Selectable("Texture 2");
        ImGui::Selectable("Texture 3");
        ImGui::Selectable("Texture 4");
        ImGui::Selectable("Texture 5");
        ImGui::Selectable("Texture 6");
    }

    if (ImGui::CollapsingHeader("Materials"))
    {
    }

    if (ImGui::CollapsingHeader("Audio"))
    {
    }

    if (ImGui::CollapsingHeader("Meshes"))
    {
    }

    if (ImGui::CollapsingHeader("Particle Systems"))
    {
    }

    if (ImGui::CollapsingHeader("Vehicles"))
    {
    }

    if (ImGui::CollapsingHeader("Drivers"))
    {
    }

    if (ImGui::CollapsingHeader("Track Lists"))
    {
    }

    if (ImGui::CollapsingHeader("Weapons"))
    {
    }

    if (ImGui::CollapsingHeader("Tracks"))
    {
        for (auto& trackName : tracks)
        {
            if (ImGui::Selectable(trackName.c_str()))
            {
                activeEditor = EditorType::SCENE;
                g_game.changeScene(("tracks/" + trackName).c_str());
                editor.reset();
            }
        }
    }

    ImGui::End();

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
    }
}
