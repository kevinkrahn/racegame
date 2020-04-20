#include "editor.h"
#include "../game.h"
#include "../input.h"
#include "../scene.h"
#include "../input.h"
#include "../imgui.h"
#include "terrain_mode.h"
#include "track_mode.h"
#include "decoration_mode.h"
#include "path_mode.h"

void Editor::reset()
{
    modes.clear();
    modes.push_back(std::make_unique<TerrainMode>());
    modes.push_back(std::make_unique<TrackMode>());
    modes.push_back(std::make_unique<DecorationMode>());
    modes.push_back(std::make_unique<PathMode>());

    for (auto& mode : modes)
    {
        mode->setEditor(this);
    }
}

Editor::Editor()
{
    reset();
}

void Editor::onEndTestDrive(Scene* scene)
{
    for (auto& mode : modes)
    {
        mode->onEndTest(scene);
    }
}

void Editor::onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime)
{
    modes[activeModeIndex]->onUpdate(scene, renderer, deltaTime);

    glm::vec3 cameraTarget = scene->getEditorCamera().getCameraTarget();
    if (gridSettings.show)
    {
        i32 count = (i32)(40.f / gridSettings.cellSize);
        f32 gridSize = count * gridSettings.cellSize;
        glm::vec4 color = { 1.f, 1.f, 1.f, 0.2f };
        for (i32 i=-count; i<=count; i++)
        {
            f32 x = i * gridSettings.cellSize;
            f32 y = i * gridSettings.cellSize;
            scene->debugDraw.line(
                { snap(cameraTarget.x + x, gridSettings.cellSize), snap(cameraTarget.y - gridSize, gridSettings.cellSize), gridSettings.z },
                { snap(cameraTarget.x + x, gridSettings.cellSize), snap(cameraTarget.y + gridSize, gridSettings.cellSize), gridSettings.z },
                color, color);
            scene->debugDraw.line(
                { snap(cameraTarget.x - gridSize, gridSettings.cellSize), snap(cameraTarget.y + y, gridSettings.cellSize), gridSettings.z },
                { snap(cameraTarget.x + gridSize, gridSettings.cellSize), snap(cameraTarget.y + y, gridSettings.cellSize), gridSettings.z },
                color, color);
        }
    }

    ImGui::Begin("Editor");
    if (ImGui::Button("New"))
    {
        g_game.changeScene(nullptr);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load"))
    {
        std::string filename = chooseFile("tracks/track1.dat", true);
        if (!filename.empty())
        {
            g_game.changeScene(filename.c_str());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        std::string filename = scene->filename;
        if (filename.empty())
        {
            filename = chooseFile("tracks/untitled.dat", false);
        }
        if (!filename.empty())
        {
            print("Saving scene to file: ", scene->filename, '\n');
            auto data = DataFile::makeDict();
            Serializer s(data, false);
            scene->serialize(s);
            DataFile::save(data, filename.c_str());
            scene->filename = filename;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Save As"))
    {
        std::string filename = chooseFile("tracks/untitled.dat", false);
        if (!filename.empty())
        {
            print("Saving scene to file: ", scene->filename, '\n');
            auto data = DataFile::makeDict();
            Serializer s(data, false);
            scene->serialize(s);
            DataFile::save(data, filename.c_str());
            scene->filename = filename;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Test Drive") || g_input.isKeyPressed(KEY_F5))
    {
        // TODO: Add options to include AI drivers and configure player vehicle

        g_game.state.drivers.clear();
        g_game.state.drivers.push_back(Driver(true, true, true, 0, 0));
        auto conf = g_game.state.drivers.back().getVehicleConfig();
        conf->frontWeaponIndices[0] = 1;
        conf->frontWeaponUpgradeLevel[0] = 5;
        conf->rearWeaponIndices[0] = 5;
        conf->rearWeaponUpgradeLevel[0] = 5;
        conf->specialAbilityIndex = 11;

        for (auto& mode : modes)
        {
            mode->onBeginTest(scene);
        }

        scene->startRace();
    }

    ImGui::Gap();
    ImGui::InputText("Track Name", &scene->name);
    ImGui::InputTextMultiline("Notes", &scene->notes, {0, 70});
    i32 totalLaps = (i32)scene->totalLaps;
    ImGui::InputInt("Laps", &totalLaps);
    scene->totalLaps = (u32)glm::clamp(totalLaps, 1, 10);
    ImGui::Gap();
    ImGui::Checkbox("Show Grid", &gridSettings.show);
    ImGui::Checkbox("Snap to Grid", &gridSettings.snap);
    ImGui::InputFloat("Grid Size", &gridSettings.cellSize, 0.1f, 0.f, "%.1f");
    gridSettings.cellSize = clamp(gridSettings.cellSize, 0.1f, 20.f);

    auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);

    u32 previousModeIndex = activeModeIndex;
    ImGui::Gap();
    if (ImGui::BeginTabBar("Edit Mode"))
    {
        for (u32 i=0; i<modes.size(); ++i)
        {
            if (ImGui::BeginTabItem(modes[i]->getName().c_str()))
            {
                activeModeIndex = i;
                modes[i]->onEditorTabGui(scene, renderer, deltaTime);
                ImGui::EndTabItem();
            }
        }

        ImGui::EndTabBar();
    }

    if (activeModeIndex != previousModeIndex)
    {
        modes[previousModeIndex]->onSwitchFrom(scene);
        modes[activeModeIndex]->onSwitchTo(scene);
    }

    ImGui::End();
}

std::string chooseFile(const char* defaultSelection, bool open)
{
#if _WIN32
    char szFile[260];

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All\0*.*\0Tracks\0*.dat\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    if (open)
    {
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    }

    if (open)
    {
        if (GetOpenFileName(&ofn) == TRUE)
        {
            return std::string(szFile);
        }
        else
        {
            return {};
        }
    }
    else
    {
        if (GetSaveFileName(&ofn) == TRUE)
        {
            return std::string(szFile);
        }
        else
        {
            return {};
        }
    }
#else
    char filename[1024] = { 0 };
    std::string cmd = "zenity";
    if (open)
    {
        cmd += " --title 'Open Track' --file-selection --filename ";
        cmd += defaultSelection;
    }
    else
    {
        cmd += " --title 'Save Track' --file-selection --save --confirm-overwrite --filename ";
        cmd += defaultSelection;
    }
    FILE *f = popen(cmd.c_str(), "r");
    if (!f || !fgets(filename, sizeof(filename) - 1, f))
    {
        error("Unable to create file dialog\n");
        return {};
    }
    pclose(f);
    std::string file(filename);
    if (!file.empty())
    {
        file.pop_back();
    }
    return file;
#endif
}
