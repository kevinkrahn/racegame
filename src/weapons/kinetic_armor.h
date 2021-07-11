#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WKineticArmor : public Weapon
{
    f32 timer = 0.f;

public:
    WKineticArmor()
    {
        info.name = "Kinetic Armor";
        info.description = "Completely nullifies the first damage taken each lap.";
        info.icon = g_res.getTexture("icon_kinetic_armor");
        info.price = 2000;
        info.maxUpgradeLevel = 1;
        info.weaponType = WeaponType::SPECIAL_ABILITY;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (ammo > 0)
        {
            vehicle->setShield(Vec3(1.f, 0.2f, 0.f), 1.f);
            //Vec3(0.05f, 0.15f, 1.f);
        }
        else
        {
            timer = max(timer - deltaTime, 0.f);
        }
    }

    void render(class RenderWorld* rw, Mat4 const& vehicleTransform,
            struct VehicleConfiguration const& config, struct VehicleData const& vehicleData) override
    {
        // TODO: show effect when taking damage
    }

    f32 onDamage(f32 damage, Vehicle* vehicle) override
    {
        if (ammo > 0 && damage > 8.f)
        {
            ammo = 0;
            g_audio.playSound3D(g_res.getSound("kinetic_armor"), SoundType::GAME_SFX, vehicle->getPosition());

            // Add a small time window of invulnerability. Without it would only absorb one bullet
            // from a scattergun shot
            timer = 0.1f;
        }

        if (timer > 0.f)
        {
            return 0.f;
        }

        return damage;
    }
};
