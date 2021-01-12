#include "track_editor.h"
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

TrackEditor::~TrackEditor()
{
}

void TrackEditor::init(Resource* resource)
{
    this->resource = resource;

    modes.clear();
    modes.push(new TerrainMode);
    modes.push(new TrackMode);
    modes.push(new SplineMode);
    modes.push(new DecorationMode);
    modes.push(new PathMode);

    for (auto& mode : modes)
    {
        mode->setEditor(this);
    }

    g_game.changeScene(resource->guid);
}

void TrackEditor::onUpdate(Resource* r, ResourceManager* rm, Renderer* renderer, f32 deltaTime, u32 n)
{
    Scene* scene = g_game.currentScene.get();
    if (!scene)
    {
        // scene hasn't loaded yet
        return;
    }
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

    if (ImGui::Begin("Track Editor"))
    {
        ImGui::InputText("##Name", &scene->name);

        ImGui::SameLine();
        if (ImGui::Button("Test Drive") || g_input.isKeyPressed(KEY_F5))
        {
            // TODO: Add options to include AI drivers and configure player vehicle
	        Array<i64> vehicles;
	        g_res.iterateResourceType(ResourceType::VEHICLE, [&](Resource* r){
                auto v = (VehicleData*)r;
                if (v->showInCarLot) { vehicles.push(v->guid); }
	        });

            g_game.state.drivers.clear();
#if 0
            g_game.state.drivers.push(Driver(true, true, true, 0,
                        vehicles[irandom(scene->randomSeries, 0, vehicles.size())]));
#else
            g_game.state.drivers.push(Driver(true, true, true, 0, vehicles[2]));
#endif
            auto conf = g_game.state.drivers.back().getVehicleConfig();
            auto vd = g_game.state.drivers.back().getVehicleData();
            conf->cosmetics.color =
                srgb(hsvToRgb(vd->defaultColorHsv.x, vd->defaultColorHsv.y, vd->defaultColorHsv.z));
            conf->weaponIndices[0] = 14;
            conf->weaponUpgradeLevel[0] = 5;
            conf->weaponIndices[1] = 6;
            conf->weaponUpgradeLevel[1] = 5;
            conf->weaponIndices[2] = 15;
            conf->weaponIndices[3] = 15;

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
        ImGui::SameLine(0.f, 16.f);
        ImGui::Checkbox("Snap to Grid", &gridSettings.snap);
        ImGui::InputFloat("Grid Size", &gridSettings.cellSize, 0.1f, 0.f, "%.1f");
        gridSettings.cellSize = clamp(gridSettings.cellSize, 0.1f, 20.f);

        // TODO: Should there be a seperate tab for sun, fog, .etc?
        ImGui::Gap();
        ImGui::SliderAngle("Sun Angle", &scene->sunDir);
        ImGui::SliderAngle("Sun Angle Z", &scene->sunDirZ, -89.9999f, 89.999f);
        ImGui::ColorEdit3("Sun Color", (f32*)&scene->sunColor);
        ImGui::SliderFloat("Sun Strength", &scene->sunStrength, 0.f, 5.f, "%.2f");

        ImGui::Gap();
        ImGui::ColorEdit3("Fog Color", (f32*)&scene->fogColor);
        ImGui::SliderFloat("Fog Density", &scene->fogDensity, 0.f, 0.018f, "%.5f");
        ImGui::SliderFloat("Fog Begin Distance", &scene->fogBeginDistance, 0.f, 100.f, "%.1f");

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

bool TrackEditor::wantsExclusiveScreen()
{
    if (g_game.currentScene && g_game.currentScene->isRaceInProgress)
    {
        if (g_input.isKeyPressed(KEY_ESCAPE) || g_input.isKeyPressed(KEY_F5))
        {
            g_game.currentScene->stopRace();
            for (auto& mode : modes)
            {
                mode->onEndTest(g_game.currentScene.get());
            }
        }
        return true;
    }
    return false;
}
