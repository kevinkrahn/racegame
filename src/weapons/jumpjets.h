#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WJumpJets : public Weapon
{
public:
    WJumpJets()
    {
        info.name = "Jump Jet";
        info.description =
            "Propels the vehicle upwards with a single burst of energy.\n"
            "Useful for avoiding hazards.";
        info.icon = "mine_icon";
        info.price = 900;
        info.maxUpgradeLevel = 5;
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
        vehicle->getRigidBody()->addForce(
                convert(zAxisOf(vehicle->getTransform()) * 12.f),
                PxForceMode::eVELOCITY_CHANGE);
        ammo -= 1;
    }
};
