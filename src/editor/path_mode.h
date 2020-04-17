#pragma once

#include "editor_mode.h"
#include "../input.h"
#include "../renderer.h"
#include "../imgui.h"
#include "../scene.h"
#include "../game.h"
#include "../mesh_renderables.h"

class PathMode : public EditorMode
{
    void subdividePath(Scene* scene)
    {

    }

public:
    PathMode() : EditorMode("Paths") {}

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
    }

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        ImGui::Spacing();

        ImGui::ListBoxHeader("AI Paths");
        for (auto& path : scene->getPaths())
        {
        }
        ImGui::ListBoxFooter();
        ImGui::HelpMarker("Paths that the AI drivers will follow. The most optimal paths should be first in the list.");

        auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);
        ImGui::Button("New Path", buttonSize);
        ImGui::Button("Delete Path", buttonSize);

        ImGui::Gap();
        if (ImGui::Button("Subdivide [n]", buttonSize) || g_input.isKeyPressed(KEY_N))
        {
            subdividePath(scene);
        }
    }
};
