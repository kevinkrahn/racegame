#pragma once

#include "../resources.h"
#include "../imgui.h"
#include "../vehicle_data.h"
#include "resource_editor.h"
#include "resource_manager.h"

class VehicleEditor : public ResourceEditor
{
    RenderWorld rw;
    VehicleConfiguration config;
    VehicleTuning tuning;

public:
    ~VehicleEditor()
    {
        rw.destroy();
    }

    bool drawVehiclePreview(Renderer* renderer, RenderWorld& rw, VehicleData* v)
    {
        bool dirty = false;

        rw.setName("Vehicle Editor Preview");
        rw.setBloomForceOff(true);
        u32 rs = 160;
        rw.setSize(rs, rs);
        rw.setViewportCount(1);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
        rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);

        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
        drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));

        v->render(&rw, Mat4::translation(Vec3(0, 0, tuning.getRestOffset())), nullptr, config, &tuning);
        renderer->addRenderWorld(&rw);

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, rs + 8);
        ImGui::Image((void*)(uintptr_t)rw.getTexture()->handle, { (f32)rs, (f32)rs },
                { 1.f, 1.f }, { 0.f, 0.f });
        ImGui::NextColumn();

        Vec3 col = hsvToRgb(v->defaultColorHsv.r, v->defaultColorHsv.g, v->defaultColorHsv.b);
        dirty |= ImGui::ColorEdit3("Default Color", (f32*)&col);
        dirty |= ImGui::SliderFloat("Default Shininess", &v->defaultShininess, 0.f, 1.f);
        config.cosmetics.paintShininess = v->defaultShininess;
        ImGui::ColorConvertRGBtoHSV(col.r, col.g, col.b,
                v->defaultColorHsv[0], v->defaultColorHsv[1], v->defaultColorHsv[2]);
        config.cosmetics.color = col;
        config.cosmetics.hsv = v->defaultColorHsv;

        ImGui::Columns(1);

        return dirty;
    }

    void init(Resource* r) override
    {
    }

    void onUpdate(Resource* r, ResourceManager* rm, Renderer* renderer, f32 deltaTime, u32 n) override
    {
        VehicleData* v = (VehicleData*)r;

        bool isOpen = true;
        bool dirty = false;
        if (ImGui::Begin(tmpStr("Vehicle Properties###Vehicle Properties %i", n), &isOpen))
        {
            dirty |= ImGui::InputText("##Name", &v->name);
            ImGui::Guid(v->guid);
            ImGui::Gap();

            dirty |= drawVehiclePreview(renderer, rw, v);
            if (dirty)
            {
                v->initTuning(config, tuning);
                config.reloadMaterials();
            }
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

