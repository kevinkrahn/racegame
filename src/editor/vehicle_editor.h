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
        u32 w = 300, h = 200;
        rw.setSize(w, h);
        rw.setViewportCount(1);
        rw.addDirectionalLight(Vec3(-0.5f, 0.2f, -1.f), Vec3(1.0));
        rw.setViewportCamera(0, Vec3(8.f, -8.f, 10.f), Vec3(0.f, 0.f, 1.f), 1.f, 50.f, 30.f);

        Mesh* quadMesh = g_res.getModel("misc")->getMeshByName("world.Quad");
        drawSimple(&rw, quadMesh, &g_res.white, Mat4::scaling(Vec3(20.f)), Vec3(0.02f));

        v->render(&rw, Mat4::translation(Vec3(0, 0, tuning.getRestOffset())), nullptr, config, &tuning);
        renderer->addRenderWorld(&rw);

        ImGui::Image((void*)(uintptr_t)rw.getTexture()->handle, { (f32)w, (f32)h },
                { 1.f, 1.f }, { 0.f, 0.f });

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
                config.reloadMaterials();
            }

            // TODO: model picker
            bool modelChanged = false;
            if (modelChanged)
            {
                v->initTuning(config, tuning);
            }

            dirty |= ImGui::InputText("Description", &v->description);
            dirty |= ImGui::InputInt("Price", &v->price, 200, 1000);
            dirty |= ImGui::InputFloat("Hit Points", &v->defaultTuning.maxHitPoints);

            // TODO: weapon slots
            // TODO: available upgrades

            dirty |= ImGui::InputFloatClamp("Mass", &v->defaultTuning.chassisMass, 1.f, 10000.f);
            dirty |= ImGui::InputFloat3("Center of Mass", (f32*)&v->defaultTuning.centerOfMass);
            dirty |= ImGui::InputFloatClamp("Top Speed", &v->defaultTuning.topSpeed, 1.f, 500.f);
            dirty |= ImGui::InputFloatClamp("Drift Boost", &v->defaultTuning.forwardDownforce, 0.f, 5.f);

            if (ImGui::TreeNode("Wheels"))
            {
                dirty |= ImGui::InputFloatClamp("Wheel Damping", &v->defaultTuning.wheelDampingRate, 0.f, 100.f);
                dirty |= ImGui::InputFloatClamp("Wheel Offroad Damping", &v->defaultTuning.wheelOffroadDampingRate, 0.f, 100.f);
                dirty |= ImGui::InputFloatClamp("Grip", &v->defaultTuning.trackTireFriction, 0.f, 20.f);
                dirty |= ImGui::InputFloatClamp("Offroad Grip", &v->defaultTuning.offroadTireFriction, 0.f, 20.f);
                dirty |= ImGui::InputFloatClamp("Rear Grip Multiplier", &v->defaultTuning.rearTireGripPercent, 0.f, 2.f);
                dirty |= ImGui::InputFloatClamp("Wheel Mass Front", &v->defaultTuning.wheelMassFront, 1.f, 100.f);
                dirty |= ImGui::InputFloatClamp("Wheel Mass Rear", &v->defaultTuning.wheelMassRear, 1.f, 100.f);
                dirty |= ImGui::InputFloatClamp("Max Brake Torque", &v->defaultTuning.engineDampingZeroThrottleClutchDisengaged, 0.f, 80000.f);
                dirty |= ImGui::InputFloatClamp("Camber at Rest", &v->defaultTuning.camberAngleAtRest, -2.f, 2.f);
                dirty |= ImGui::InputFloatClamp("Camber at Max Droop", &v->defaultTuning.camberAngleAtMaxDroop, -2.f, 2.f);
                dirty |= ImGui::InputFloatClamp("Camber at Max Compression", &v->defaultTuning.camberAngleAtMaxCompression, -2.f, 2.f);
                dirty |= ImGui::InputFloatClamp("Front Toe Angle", &v->defaultTuning.frontToeAngle, -2.f, 2.f);
                dirty |= ImGui::InputFloatClamp("Rear Toe Angle", &v->defaultTuning.rearToeAngle, -2.f, 2.f);
                dirty |= ImGui::InputFloatClamp("Max Steer Angle", &v->defaultTuning.maxSteerAngleDegrees, 0.f, 90.f);
                dirty |= ImGui::InputFloatClamp("Ackermann Accuracy", &v->defaultTuning.ackermannAccuracy, 0.f, 1.f);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Suspension"))
            {
                dirty |= ImGui::InputFloatClamp("Max Compression", &v->defaultTuning.suspensionMaxCompression, 0.f, 10.f);
                dirty |= ImGui::InputFloatClamp("Max Droop", &v->defaultTuning.suspensionMaxDroop, 0.f, 10.f);
                dirty |= ImGui::InputFloatClamp("Spring Strength", &v->defaultTuning.suspensionSpringStrength, 100.f, 100000.f);
                dirty |= ImGui::InputFloatClamp("Spring Damper Rate", &v->defaultTuning.suspensionSpringDamperRate, 100.f, 10000.f);
                dirty |= ImGui::InputFloatClamp("Anti-Rollbar Stiffness", &v->defaultTuning.frontAntiRollbarStiffness, 0.f, 100000.f);
                v->defaultTuning.rearAntiRollbarStiffness = v->defaultTuning.frontAntiRollbarStiffness;
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Engine"))
            {
                dirty |= ImGui::InputFloatClamp("Max Omega", &v->defaultTuning.maxEngineOmega, 100.f, 5000.f);
                dirty |= ImGui::InputFloatClamp("Peak Torque", &v->defaultTuning.peakEngineTorque, 50.f, 5000.f);
                dirty |= ImGui::InputFloatClamp("Damping Full Throttle", &v->defaultTuning.engineDampingFullThrottle, 0.f, 500.f);
                dirty |= ImGui::InputFloatClamp("Damping Zero Throttle With Clutch", &v->defaultTuning.engineDampingZeroThrottleClutchEngaged, 0.f, 500.f);
                dirty |= ImGui::InputFloatClamp("Damping Zero Throttle W/O Clutch", &v->defaultTuning.engineDampingZeroThrottleClutchDisengaged, 0.f, 500.f);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Downforce"))
            {
                dirty |= ImGui::InputFloatClamp("Downforce", &v->defaultTuning.constantDownforce, 0.f, 5.f);
                dirty |= ImGui::InputFloatClamp("Forward Downforce", &v->defaultTuning.forwardDownforce, 0.f, 5.f);
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Transmission"))
            {
                // TODO: gears
                dirty |= ImGui::InputFloatClamp("Final Gear Ratio", &v->defaultTuning.finalGearRatio, 0.1f, 20.f);
                dirty |= ImGui::InputFloatClamp("Clutch Strength", &v->defaultTuning.clutchStrength, 0.1f, 100.f);
                dirty |= ImGui::InputFloatClamp("Gear Switch Delay", &v->defaultTuning.gearSwitchTime, 0.f, 2.f);
                dirty |= ImGui::InputFloatClamp("Autobox Switch Delay", &v->defaultTuning.autoBoxSwitchTime, 0.f, 2.f);
                ImGui::TreePop();
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

