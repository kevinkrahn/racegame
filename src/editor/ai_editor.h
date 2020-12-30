#include "../resources.h"
#include "../imgui.h"
#include "resource_editor.h"
#include "resource_manager.h"

class AIEditor : public ResourceEditor
{
    RenderWorld rw;

public:
    ~AIEditor()
    {
        rw.destroy();
    }

    bool drawVehiclePreview(Renderer* renderer, AIDriverData& ai)
    {
        bool dirty = false;

        rw.setName("AI Vehicle Preview");
        rw.setBloomForceOff(true);
        rw.setSize(200, 200);
        rw.setViewportCount(1);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
        rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);

        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
        drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));

        const char* vehicleName = "None";
        if (ai.vehicleIndex != -1)
        {
            VehicleData* vehicle = g_vehicles[ai.vehicleIndex].get();
            VehicleTuning tuning;
            vehicle->initTuning(ai.config, tuning);
            vehicle->render(&rw, Mat4::translation(Vec3(0, 0, tuning.getRestOffset())), nullptr, ai.config);
            renderer->addRenderWorld(&rw);

            vehicleName = vehicle->name;
        }

        ImGui::Image((void*)(uintptr_t)rw.getTexture()->handle, { 200, 200 }, { 1.f, 1.f }, { 0.f, 0.f });

        if (ImGui::BeginCombo("Vehicle", vehicleName))
        {
            for (u32 i=0; i<g_vehicles.size(); ++i)
            {
                bool isSelected = i == ai.vehicleIndex;
                if (ImGui::Selectable(g_vehicles[i]->name))
                {
                    ai.vehicleIndex = i;
                    dirty = true;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (ImGui::TreeNodeEx("Paint", ImGuiTreeNodeFlags_DefaultOpen, "Paint"))
        {
            dirty |= ImGui::ColorEdit3("Color", (f32*)&ai.config.cosmetics.color);
            dirty |= ImGui::SliderFloat("Shininess", &ai.config.cosmetics.paintShininess, 0.f, 1.f);
            ImGui::TreePop();
        }

        for (u32 layerIndex=0; layerIndex<ARRAY_SIZE(ai.config.cosmetics.wrapTextureGuids); ++layerIndex)
        {
            if (ImGui::TreeNodeEx(tmpStr("Vinyl Layer %i", layerIndex+1), 0, tmpStr("Vinyl Layer %i", layerIndex+1)))
            {
                const char* layerVinylName = "";
                for (u32 i=0; i<ARRAY_SIZE(g_wrapTextures); ++i)
                {
                    Texture* t = g_res.getTexture(g_wrapTextures[i]);
                    if (t->guid == ai.config.cosmetics.wrapTextureGuids[layerIndex])
                    {
                        layerVinylName = g_wrapTextureNames[i];
                        break;
                    }
                }
                if (ImGui::BeginCombo("Texture", layerVinylName))
                {
                    for (u32 i=0; i<ARRAY_SIZE(g_wrapTextures); ++i)
                    {
                        Texture* t = g_res.getTexture(g_wrapTextures[i]);
                        bool isSelected = t->guid == ai.config.cosmetics.wrapTextureGuids[layerIndex];
                        if (ImGui::Selectable(g_wrapTextureNames[i]))
                        {
                            ai.config.cosmetics.wrapTextureGuids[layerIndex] = t->guid;
                            dirty = true;
                        }
                        if (isSelected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }
                dirty |= ImGui::ColorEdit4("Color", (f32*)&ai.config.cosmetics.wrapColors[layerIndex]);
                ImGui::TreePop();
            }
        }

        if (ai.vehicleIndex != -1)
        {
            VehicleData* vehicle = g_vehicles[ai.vehicleIndex].get();
            if (ImGui::TreeNodeEx("Weapons", 0, "Weapons"))
            {
                for (u32 slotIndex=0; slotIndex<vehicle->weaponSlots.size(); ++slotIndex)
                {
                    i32 installedWeaponIndex = ai.config.weaponIndices[slotIndex];
                    const char* installedName =
                        installedWeaponIndex != -1 ? g_weapons[installedWeaponIndex].info.name : "None";
                    WeaponSlot slot = vehicle->weaponSlots[slotIndex];
                    if (ImGui::BeginCombo(slot.name, installedName))
                    {
                        for (u32 i=0; i<g_weapons.size(); ++i)
                        {
                            auto& weapon = g_weapons[i];
                            if (weapon.info.weaponType != slot.weaponType
                                || (weapon.info.weaponClasses & slot.weaponClasses) != slot.weaponClasses)
                            {
                                continue;
                            }

                            bool isSelected = i == installedWeaponIndex;
                            if (ImGui::Selectable(weapon.info.name))
                            {
                                ai.config.weaponIndices[slotIndex] = i;
                                ai.config.weaponUpgradeLevel[slotIndex] = weapon.info.maxUpgradeLevel;
                                dirty = true;
                            }
                            if (isSelected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }
                }
                ImGui::TreePop();
            }
        }

        return dirty;
    }

    void onUpdate(Resource* r, ResourceManager* rm, Renderer* renderer, f32 deltaTime, u32 n) override
    {
        AIDriverData& ai = *(AIDriverData*)r;

        bool isOpen = true;
        bool dirty = false;
        if (ImGui::Begin(tmpStr("AI Driver Properties###AI Driver Properties %i", n), &isOpen))
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

            dirty |= drawVehiclePreview(renderer, ai);

            ai.config.reloadMaterials();
        }
        ImGui::End();

        if (!isOpen)
        {
            rm->markClosed(r);
        }

        if (dirty)
        {
            rm->markDirty(r->guid);
        }
    }
};

