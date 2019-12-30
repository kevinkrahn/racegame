#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

// TODO: make AI drivers use this more intelligently
class WJumpJets : public Weapon
{
public:
    WJumpJets()
    {
        info.name = "Jump Jet";
        info.description =
            "Propels the vehicle upwards with a single burst of energy.\n"
            "Useful for avoiding hazards.";
        info.icon = &g_res.textures->icon_jumpjet;
        info.price = 900;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::REAR_WEAPON;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (!fireBegin)
        {
            return;
        }

        if (vehicle->isInAir || ammo == 0)
        {
            // TODO: play no-no sound
            return;
        }
        vehicle->getRigidBody()->setAngularVelocity(PxVec3(0));
        vehicle->getRigidBody()->addForce(PxVec3(0, 0, 12),
                //convert(zAxisOf(vehicle->getTransform()) * 12.f),
                PxForceMode::eVELOCITY_CHANGE);
        ammo -= 1;
    }
};
