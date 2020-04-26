#pragma once

#include "transform_gizmo.h"
#include "editor_mode.h"
#include "editor.h"
#include "../input.h"
#include "../renderer.h"
#include "../imgui.h"
#include "../scene.h"
#include "../spline.h"

class SplineMode : public EditorMode
{
    struct Selection
    {
        Spline* spline;
        u16 pointIndex;
    };

    std::vector<Selection> selectedPoints;
    Spline* selectedSpline = nullptr;
    TransformGizmo transformGizmo;

public:
    SplineMode() : EditorMode("Splines") {}

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        /*
        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;

        RenderWorld* rw = renderer->getRenderWorld();
        glm::vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
        Camera const& cam = scene->getEditorCamera().getCamera();
        */

        for (auto& e : scene->getEntities())
        {
            if (e->entityID == 5)
            {

            }
        }
    }

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

        ImGui::Spacing();

        auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);
        if (ImGui::Button("Connect Points [c]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_C)))
        {
        }

        if (ImGui::Button("Subdivide [n]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_N)))
        {
        }

        if (ImGui::Button("Split [t]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_T)))
        {
        }

        if (ImGui::Button("Match Highest Z", buttonSize))
        {
        }

        if (ImGui::Button("Match Lowest Z", buttonSize))
        {
        }
    }
};
