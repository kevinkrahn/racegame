#pragma once

#include "../weapon.h"
#include "../vehicle.h"

class WAutoRepair : public Weapon
{
public:
    WAutoRepair()
    {
        info.name = "Auto Repair";
        info.description = "Slowly repairs damage over time.";
        info.icon = g_res.getTexture("icon_auto_repair");
        info.price = 2500;
        info.maxUpgradeLevel = 1;
        info.weaponType = WeaponType::SPECIAL_ABILITY;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        vehicle->repair(deltaTime * 3);
    }

    void render(class RenderWorld* rw, Mat4 const& vehicleTransform,
            struct VehicleConfiguration const& config, struct VehicleData const& vehicleData) override
    {
    }
};
