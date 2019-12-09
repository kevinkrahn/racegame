#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WRamBooster : public Weapon
{
public:
    WRamBooster()
    {
        info.name = "Ram Booster";
        info.description = "Bonus ramming damage against your opponents,\nbut less damage to you!";
        info.icon = &g_res.textures->icon_spikes;
        info.price = 4000;
        info.maxUpgradeLevel = 1;
        info.weaponType = WeaponInfo::SPECIAL_ABILITY;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
    }
};
