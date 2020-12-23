#include "editor.h"
#include "../game.h"
#include "../input.h"
#include "../scene.h"
#include "../input.h"
#include "../imgui.h"
#include "terrain_mode.h"
#include "track_mode.h"
#include "spline_mode.h"
#include "decoration_mode.h"
#include "path_mode.h"

void Editor::reset()
{
    modes.clear();
    modes.push_back(new TerrainMode);
    modes.push_back(new TrackMode);
    modes.push_back(new SplineMode);
    modes.push_back(new DecorationMode);
    modes.push_back(new PathMode);

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

    Vec3 cameraTarget = scene->getEditorCamera().getCameraTarget();
    if (gridSettings.show)
    {
        i32 count = (i32)(40.f / gridSettings.cellSize);
        f32 gridSize = count * gridSettings.cellSize;
        Vec4 color = { 1.f, 1.f, 1.f, 0.2f };
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

    g_game.resourceManager->markDirty(scene->guid);

    if (ImGui::Begin("Track Editor"))
    {
        ImGui::InputText("##Name", &scene->name);

        ImGui::SameLine();
        if (ImGui::Button("Test Drive") || g_input.isKeyPressed(KEY_F5))
        {
            // TODO: Add options to include AI drivers and configure player vehicle

            g_game.state.drivers.clear();
            g_game.state.drivers.push_back(Driver(true, true, true, 0, 4));
            auto conf = g_game.state.drivers.back().getVehicleConfig();
            auto vd = g_game.state.drivers.back().getVehicleData();
            conf->color =
                srgb(hsvToRgb(vd->defaultColorHsv.x, vd->defaultColorHsv.y, vd->defaultColorHsv.z));
            conf->frontWeaponIndices[0] = 14;
            conf->frontWeaponUpgradeLevel[0] = 5;
            conf->rearWeaponIndices[0] = 6;
            conf->rearWeaponUpgradeLevel[0] = 5;
            //conf->specialAbilityIndex = 11;
            conf->specialAbilityIndex = 7;

            for (auto& mode : modes)
            {
                mode->onBeginTest(scene);
            }

            scene->startRace();
        }

        ImGui::Gap();
        i32 totalLaps = (i32)scene->totalLaps;
        ImGui::InputInt("Laps", &totalLaps);
        scene->totalLaps = (u32)clamp(totalLaps, 1, 10);
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
                if (ImGui::BeginTabItem(modes[i]->getName()))
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

    }
    ImGui::End();
}
