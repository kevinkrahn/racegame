#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WUnderPlating : public Weapon
{
public:
    WUnderPlating()
    {
        info.name = "Underplating";
        info.description = "Provides immunity to damage from mines.";
        info.icon = g_res.getTexture("icon_underplating");
        info.price = 2000;
        info.maxUpgradeLevel = 1;
        info.weaponType = WeaponType::SPECIAL_ABILITY;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
    }
};
