#pragma once

#include "editor.h"
#include "editor_mode.h"
#include "../input.h"
#include "../renderer.h"
#include "../imgui.h"
#include "../scene.h"

class TrackMode : public EditorMode
{
    bool clickHandledUntilRelease = false;

public:
    TrackMode() : EditorMode("Track") {}

    void onUpdate(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        bool isMouseClickHandled = ImGui::GetIO().WantCaptureMouse;

        if (clickHandledUntilRelease)
        {
            if (g_input.isMouseButtonReleased(MOUSE_LEFT))
            {
                clickHandledUntilRelease = false;
            }
            else
            {
                isMouseClickHandled = true;
            }
        }

        //RenderWorld* rw = renderer->getRenderWorld();
        //Vec3 rayDir = scene->getEditorCamera().getMouseRay(rw);
        //Camera const& cam = scene->getEditorCamera().getCamera();

        scene->track->trackModeUpdate(renderer, scene, deltaTime, isMouseClickHandled,
                &editor->getGridSettings());
    }

    void onEditorTabGui(Scene* scene, Renderer* renderer, f32 deltaTime) override
    {
        bool isKeyboardHandled = ImGui::GetIO().WantCaptureKeyboard;

        ImGui::Spacing();

        auto buttonSize = ImVec2(ImGui::GetWindowWidth() * 0.65f, 0);
        if (ImGui::Button("Connect Points [c]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_C)))
        {
            scene->track->connectPoints();
        }

        if (ImGui::Button("Subdivide [n]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_N)))
        {
            scene->track->subdividePoints();
        }

        if (ImGui::Button("Split [t]", buttonSize)
                || (!isKeyboardHandled && g_input.isKeyPressed(KEY_T)))
        {
            //scene->track->split();
        }

        if (ImGui::Button("Match Highest Z", buttonSize))
        {
            scene->track->matchZ(false);
        }

        if (ImGui::Button("Match Lowest Z", buttonSize))
        {
            scene->track->matchZ(true);
        }

        ImGui::Gap();
        if (scene->track->hasSelection())
        {
            if (scene->track->canExtendTrack())
            {
                for (u32 i=0; i<ARRAY_SIZE(Track::prefabTrackItems); ++i)
                {
                    if (i > 0)
                    {
                        ImGui::SameLine();
                    }
                    if (ImGui::ImageButton(
                        (void*)(uintptr_t)g_res.getTexture(scene->track->prefabTrackItems[i].icon)->handle,
                        { 64, 64 }))
                    {
                        scene->track->extendTrack(i);
                    }
                }
            }
        }

    }
};
