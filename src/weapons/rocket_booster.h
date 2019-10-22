#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WRocketBooster : public Weapon
{
    f32 boostTimer = 0.f;
    SoundHandle boostSound;

public:
    WRocketBooster()
    {
        info.name = "Rocket Booster";
        info.description = "It's like nitrous, but better!";
        info.icon = "rocketbooster_icon";
        info.price = 1000;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::REAR_WEAPON;
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
            g_audio.setSoundPosition(boostSound, vehicle->getPosition());

            scene->smoke.spawn(vehicle->getPosition() - vehicle->getForwardVector() * 1.5f,
                    convert(vehicle->getRigidBody()->getLinearVelocity()) * 0.8f,
                    1.f, glm::vec4(1.f, 0.5f, 0.05f, 1.f), 1.5f);

            return;
        }

        if (fireBegin)
        {
            if (ammo == 0)
            {
                outOfAmmo(vehicle);
                return;
            }

            boostSound = g_audio.playSound3D(g_resources.getSound("rocketboost"),
                    SoundType::GAME_SFX, vehicle->getPosition(), false, 1.f, 0.8f);

            boostTimer = 1.25f;
            ammo -= 1;
        }
    }
};
