#pragma once

#include "../weapon.h"
#include "../vehicle.h"
#include "../entities/projectile.h"

class WBlaster : public Weapon
{
public:
    WBlaster()
    {
        info.name = "Blaster";
        info.description = "It shoots";
        info.icon = "blaster_icon";
        info.price = 800;
        info.maxUpgradeLevel = 5;
        info.weaponType = WeaponInfo::FRONT_WEAPON;
    }

    void update(Scene* scene, Vehicle* vehicle, bool fireBegin, bool fireHold,
            f32 deltaTime) override
    {
        if (!fireBegin)
        {
            return;
        }

        if (ammo == 0)
        {
            // TODO: play no-no sound
            return;
        }

        glm::vec3 forward = vehicle->getForwardVector();
        glm::vec3 right = vehicle->getRightVector();
        glm::mat4 transform = vehicle->getTransform();
        f32 minSpeed = 40.f;
        glm::vec3 vel = convert(vehicle->getRigidBody()->getLinearVelocity())
            + vehicle->getForwardVector() * minSpeed;
        if (glm::length2(vel) < square(minSpeed))
        {
            vel = glm::normalize(vel) * minSpeed;
        }
        glm::vec3 pos = translationOf(transform);
        scene->addEntity(new Projectile(pos + forward * 3.f + right * 0.8f,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BLASTER));
        scene->addEntity(new Projectile(pos + forward * 3.f - right * 0.8f,
                vel, zAxisOf(transform), vehicle->vehicleIndex, Projectile::BLASTER));
        // TODO: play sound

        ammo -= 1;
    }
};
