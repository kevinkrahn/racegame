#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WRocketBooster : public Weapon
{
    f32 boostTimer = 0.f;

public:
    WRocketBooster()
    {
        info.name = "Rocket Booster";
        info.description = "Propels the vehicle forward with a burst of speed.";
        info.icon = "mine_icon";
        info.price = 1000;
        info.maxUpgradeLevel = 5;
    }

    void reset() override
    {
        boostTimer = 0.f;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (boostTimer > 0.f)
        {
            vehicle->getRigidBody()->addForce(
                    convert(vehicle->getForwardVector() * 12.f),
                    PxForceMode::eACCELERATION);
            boostTimer = glm::max(boostTimer - deltaTime, 0.f);
            // TODO: add flames shooting from the back of the vehicle
            return;
        }

        if (fireBegin)
        {
            if (ammo == 0)
            {
                // TODO: play no-no sound
                return;
            }

            // TODO: play sound

            boostTimer = 1.f;
            ammo -= 1;
        }
    }
};
