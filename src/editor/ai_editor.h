#include "../resources.h"
#include "../imgui.h"
#include "resource_editor.h"
#include "resource_manager.h"

class AIEditor : public ResourceEditor
{
public:
    void onUpdate(Resource* r, ResourceManager* rm, class Renderer* renderer, f32 deltaTime) override
    {
        AIDriverData& ai = *(AIDriverData*)r;

        bool isOpen = true;
        bool dirty = false;
        if (ImGui::Begin("AI Driver Properties", &isOpen))
        {
            dirty |= ImGui::InputText("##Name", &ai.name);
            ImGui::Guid(ai.guid);
            ImGui::Gap();
            dirty |= ImGui::SliderFloat("Driving Skill", &ai.drivingSkill, 0.f, 1.f);
            ImGui::HelpMarker("How optimal of a path the AI takes on the track.");
            dirty |= ImGui::SliderFloat("Aggression", &ai.aggression, 0.f, 1.f);
            ImGui::HelpMarker("How often the AI will go out of its way to attack other drivers.");
            dirty |= ImGui::SliderFloat("Awareness", &ai.awareness, 0.f, 1.f);
            ImGui::HelpMarker("How much the AI will attempt to avoid hitting other drivers and obstacles.");
            dirty |= ImGui::SliderFloat("Fear", &ai.fear, 0.f, 1.f);
            ImGui::HelpMarker("How much the AI will try to evade other drivers.");

            ImGui::Gap();

            dirty |= ImGui::ColorEdit3("Paint Color", (f32*)&ai.cosmetics.color);
            dirty |= ImGui::SliderFloat("Paint Shininess", &ai.cosmetics.paintShininess, 0.f, 1.f);
        }
        ImGui::End();

        if (!isOpen)
        {
            rm->closeResource(r);
        }

        if (dirty)
        {
            rm->markDirty(r->guid);
        }
    }
};

