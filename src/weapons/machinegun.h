#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WMachineGun : public Weapon
{
    f32 repeatTimer = 0.f;

public:
    WMachineGun()
    {
        info.name = "Machine Gun";
        info.description = "It shoots";
        info.icon = "blaster_icon";
        info.price = 1100;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::FRONT_WEAPON;

        ammoUnitCount = 12;
        fireMode = FireMode::CONTINUOUS;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (!fireHold)
        {
            return;
        }

        if (ammo == 0)
        {
            if (fireBegin)
            {
                outOfAmmo(vehicle);
            }
            return;
        }

        if (repeatTimer > 0.f)
        {
            repeatTimer = glm::max(repeatTimer - deltaTime, 0.f);
            return;
        }

        glm::vec3 forward = vehicle->getForwardVector();
        glm::mat4 transform = vehicle->getTransform();
        f32 minSpeed = 90.f;
        glm::vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (glm::length2(vel) < square(minSpeed))
        {
            vel = glm::normalize(vel) * minSpeed;
        }
        glm::vec3 pos = translationOf(transform);
        scene->addEntity(new Projectile(pos + forward * 2.5f,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BULLET));

        g_audio.playSound3D(g_resources.getSound("mg2"),
                SoundType::GAME_SFX, vehicle->getPosition(), false,
                random(scene->randomSeries, 0.95f, 1.05f), 1.f,
                random(scene->randomSeries, -0.05f, 0.05f));

        ammo -= 1;

        repeatTimer = 0.09f;
    }
};
