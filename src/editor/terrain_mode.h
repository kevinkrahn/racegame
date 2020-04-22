#pragma once

#include "editor_mode.h"
#include "../input.h"
#include "../renderer.h"
#include "../imgui.h"
#include "../scene.h"

class TerrainMode : public EditorMode
{
    bool canUseTerrainTool = false;
    f32 brushRadius = 8.f;
    f32 brushFalloff = 1.f;
    f32 brushStrength = 15.f;
    f32 brushStartZ = 0.f;
    glm::vec3 mousePosition;

    enum struct TerrainTool : i32
    {
        RAISE,
        PERTURB,
        FLATTEN,
        SMOOTH,
        ERODE,
        MATCH_TRACK,
        PAINT,
        MAX
    } terrainTool = TerrainTool::RAISE;
    i32 paintMaterialIndex = 2;

public:
    TerrainMode() : EditorMode("Terrain") {}

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;
        bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

        if (!isKeyboardHandled && g_input.isKeyPressed(KEY_SPACE))
        {
            terrainTool = TerrainTool(((u32)terrainTool + 1) % (u32)TerrainTool::MAX);
        }

        if (!isMouseClickHandled && g_input.isMouseButtonPressed(MOUSE_LEFT))
        {
            canUseTerrainTool = true;
        }

        RenderWorld* rw = renderer->getRenderWorld();
        glm::vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
        Camera const& cam = scene->getEditorCamera().getCamera();

        if (g_input.isKeyDown(KEY_LCTRL) && g_input.getMouseScroll() != 0)
        {
            brushRadius = clamp(brushRadius + g_input.getMouseScroll(), 2.0f, 40.f);
        }

        const f32 step = 0.01f;
        glm::vec3 p = cam.position;
        u32 count = 50000;
        while (p.z > scene->terrain->getZ(glm::vec2(p)) && --count > 0)
        {
            p += rayDir * step;
        }
        mousePosition = p;

        if (!isMouseClickHandled && count > 0)
        {
            scene->terrain->setBrushSettings(brushRadius, brushFalloff, brushStrength, p);
            if (g_input.isMouseButtonPressed(MOUSE_LEFT))
            {
                brushStartZ = scene->terrain->getZ(glm::vec2(p));
                PxRaycastBuffer hit;
                if (scene->raycastStatic(cam.position, rayDir, 10000.f, &hit, COLLISION_FLAG_TRACK))
                {
                    brushStartZ = glm::max(brushStartZ, hit.block.position.z - 0.06f);
                }
            }
            if (g_input.isMouseButtonDown(MOUSE_LEFT) && canUseTerrainTool)
            {
                switch (terrainTool)
                {
                case TerrainTool::RAISE:
                    scene->terrain->raise(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
                    break;
                case TerrainTool::PERTURB:
                    scene->terrain->perturb(glm::vec2(p), brushRadius, brushFalloff, brushStrength * deltaTime);
                    break;
                case TerrainTool::FLATTEN:
                    scene->terrain->flatten(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength) * deltaTime, brushStartZ);
                    break;
                case TerrainTool::SMOOTH:
                    scene->terrain->smooth(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength) * deltaTime);
                    break;
                case TerrainTool::ERODE:
                    scene->terrain->erode(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength) * deltaTime);
                    break;
                case TerrainTool::MATCH_TRACK:
                    scene->terrain->matchTrack(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength) * deltaTime, scene);
                    break;
                case TerrainTool::PAINT:
                    scene->terrain->paint(glm::vec2(p), brushRadius, brushFalloff, glm::abs(brushStrength * 1.f) * deltaTime, paintMaterialIndex);
                    break;
                default:
                    break;
                }
            }
        }
        scene->terrain->regenerateMesh();
    }

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        ImGui::Spacing();
        if (ImGui::BeginCombo("Terrain Type", scene->terrain->surfaceMaterials[(i32)scene->terrain->terrainType].name))
        {
            for (i32 i=0; i<(i32)ARRAY_SIZE(scene->terrain->surfaceMaterials); ++i)
            {
                if (ImGui::Selectable(scene->terrain->surfaceMaterials[i].name))
                {
                    scene->terrain->terrainType = (Terrain::TerrainType)i;
                }
            }
            ImGui::EndCombo();
        }

        glm::vec2 terrainMin(scene->terrain->x1, scene->terrain->y1);
        glm::vec2 terrainMax(scene->terrain->x2, scene->terrain->y2);
        if (ImGui::InputFloat2("Terrain Min", (f32*)&terrainMin, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
        {
            terrainMin = glm::max(glm::min(terrainMin, terrainMax - 10.f), -400.f);
            scene->terrain->resize(terrainMin.x, terrainMin.y, terrainMax.x, terrainMax.y, true);
        }
        if (ImGui::InputFloat2("Terrain Max", (f32*)&terrainMax, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue))
        {
            terrainMax = glm::min(glm::max(terrainMax, terrainMin + 10.f), 400.f);
            scene->terrain->resize(terrainMin.x, terrainMin.y, terrainMax.x, terrainMax.y, true);
        }

        ImGui::Gap();
        if (ImGui::ListBoxHeader("Terrain Tool", {0, 145}))
        {
            const char* toolNames[] = { "Raise / Lower", "Perturb", "Flatten", "Smooth", "Erode", "Match Track", "Paint" };
            for (i32 i=0; i<(i32)TerrainTool::MAX; ++i)
            {
                if (ImGui::Selectable(toolNames[i], i == (i32)terrainTool))
                {
                    terrainTool = (TerrainTool)i;
                }
            }
            ImGui::ListBoxFooter();
        }

        ImGui::Gap();
        ImGui::SliderFloat("Brush Radius", &brushRadius, 2.f, 40.f);
        ImGui::SliderFloat("Brush Falloff", &brushFalloff, 0.2f, 10.f);
        ImGui::SliderFloat("Brush Strength", &brushStrength, -30.f, 30.f);

        if (terrainTool == TerrainTool::PAINT)
        {
            if (ImGui::ListBoxHeader("Paint Material", {0, 85}))
            {
                auto& m = scene->terrain->getSurfaceMaterial();
                for (i32 i=0; i<4; ++i)
                {
                    if (ImGui::Selectable(m.textureNames[i], i == paintMaterialIndex))
                    {
                        paintMaterialIndex = i;
                    }
                }
                ImGui::ListBoxFooter();
            }
        }

        ImGui::Gap();
        ImGui::Text("Mouse Position: %.3f, %.3f, %.3f", mousePosition.x, mousePosition.y, mousePosition.z);
    }

    void onBeginTest(Scene* scene) override
    {
        scene->terrain->regenerateCollisionMesh(scene);
    }

    void onSwitchFrom(Scene* scene) override
    {
        scene->terrain->regenerateCollisionMesh(scene);
    }
};
